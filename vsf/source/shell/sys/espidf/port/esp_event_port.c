/*****************************************************************************
 *   Copyright(C)2009-2026 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*
 * Port implementation for "esp_event.h" on VSF.
 *
 * Topology per loop:
 *
 *   esp_event_post()                handler list (singly-linked)
 *         |                              ^
 *         v                              |
 *   +-----------+   dispatcher task  +--------+
 *   | xQueue    | ----------------->  dispatch  --> user handler
 *   +-----------+                    +--------+
 *
 * Storage:
 *   - event_data is malloc'd (vsf_heap) on post and copied into the message
 *     slot in the queue. The dispatcher frees the copy after handler return.
 *   - Handler nodes are allocated from vsf_heap and chained on the loop.
 *
 * Matching:
 *   - Pointer equality on base is tried first (ESP-IDF canonical contract).
 *   - strcmp is used as a defensive fallback when pointers differ.
 *   - Wildcards: base == NULL matches any base; id == -1 matches any id.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_EVENT == ENABLED

#include "esp_event.h"
#include "esp_err.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "service/trace/vsf_trace.h"
#if defined(VSF_USE_HEAP) && VSF_USE_HEAP == ENABLED
#   include "service/heap/vsf_heap.h"
#else
#   error "VSF_ESPIDF_CFG_USE_EVENT requires VSF_USE_HEAP"
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>
#include <stdlib.h>

/*============================ MACROS ========================================*/

#ifndef VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_QUEUE_SIZE
#   define VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_QUEUE_SIZE    32
#endif
#ifndef VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_STACK
#   define VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_STACK         4096
#endif
#ifndef VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_PRIO
#   define VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_PRIO          0
#endif

/*============================ TYPES =========================================*/

typedef struct handler_node_t {
    struct handler_node_t  *next;
    esp_event_base_t        base;       /*!< NULL == ESP_EVENT_ANY_BASE        */
    int32_t                 id;         /*!< -1   == ESP_EVENT_ANY_ID          */
    esp_event_handler_t     handler;
    void *                  arg;
    bool                    is_instance; /*!< Registered via instance API      */
    bool                    pending_delete; /*!< Marked for removal mid-dispatch */
} handler_node_t;

/* Queue message layout. event_data is heap-copied by the poster. */
typedef struct {
    esp_event_base_t        base;
    int32_t                 id;
    void *                  data;       /*!< NULL if data_size == 0            */
    size_t                  data_size;
} event_msg_t;

typedef struct event_loop_s {
    QueueHandle_t           queue;
    TaskHandle_t            task;       /*!< NULL for inline loops             */
    handler_node_t *        handlers;   /*!< Head of handler chain             */
    bool                    exit_flag;  /*!< Set by esp_event_loop_delete      */
    volatile bool           dispatching; /*!< Currently in handler chain walk  */
} event_loop_t;

/*============================ GLOBAL VARIABLES ==============================*/

static event_loop_t *__vsf_espidf_default_loop = NULL;

/*============================ PROTOTYPES ====================================*/

static void __dispatch_one(event_loop_t *loop, event_msg_t *msg);
static void __event_task_entry(void *arg);

/*============================ IMPLEMENTATION ================================*/

/* Match a handler entry against an outgoing (base,id) pair. */
static bool __handler_matches(const handler_node_t *node,
                              esp_event_base_t base, int32_t id)
{
    if ((node->base != NULL) && (base != NULL)) {
        if ((node->base != base) && (strcmp(node->base, base) != 0)) {
            return false;
        }
    } else if ((node->base != NULL) || (base != NULL)) {
        /* One side wildcard, the other concrete: base mismatch unless the
         * registered side is the wildcard (NULL). A post with base==NULL is
         * malformed but we treat it as "no match" rather than crashing. */
        if (node->base != NULL) {
            return false;
        }
    }
    if ((node->id != ESP_EVENT_ANY_ID) && (node->id != id)) {
        return false;
    }
    return true;
}

/* Same-registration predicate used by unregister/dedup. */
static bool __same_registration(const handler_node_t *n,
                                esp_event_base_t base, int32_t id,
                                esp_event_handler_t handler)
{
    if (n->handler != handler) { return false; }
    if (n->id != id)           { return false; }
    if (n->base == base)       { return true;  }
    if ((n->base == NULL) || (base == NULL)) { return false; }
    return strcmp(n->base, base) == 0;
}

/* Append a handler, optionally as "instance". The caller owns synchronization
 * with the dispatcher; we keep it simple here since handlers are usually
 * registered before posting begins. */
static esp_err_t __register_common(event_loop_t *loop,
                                   esp_event_base_t base, int32_t id,
                                   esp_event_handler_t handler, void *arg,
                                   bool is_instance, void **out_instance)
{
    if ((loop == NULL) || (handler == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Non-instance duplicate registrations are no-ops per ESP-IDF docs. */
    if (!is_instance) {
        for (handler_node_t *n = loop->handlers; n != NULL; n = n->next) {
            if (!n->is_instance && !n->pending_delete
                && __same_registration(n, base, id, handler)) {
                /* Refresh the arg (ESP-IDF replaces on re-register). */
                n->arg = arg;
                return ESP_OK;
            }
        }
    }

    handler_node_t *node = (handler_node_t *)vsf_heap_malloc(sizeof(*node));
    if (node == NULL) {
        return ESP_ERR_NO_MEM;
    }
    node->next          = loop->handlers;
    node->base          = base;
    node->id            = id;
    node->handler       = handler;
    node->arg           = arg;
    node->is_instance   = is_instance;
    node->pending_delete = false;
    loop->handlers      = node;

    if (out_instance != NULL) {
        *out_instance = node;
    }
    return ESP_OK;
}

/* Unlink+free any nodes marked pending_delete. Called when dispatch is idle. */
static void __purge_deleted(event_loop_t *loop)
{
    handler_node_t **pp = &loop->handlers;
    while (*pp != NULL) {
        handler_node_t *n = *pp;
        if (n->pending_delete) {
            *pp = n->next;
            vsf_heap_free(n);
        } else {
            pp = &n->next;
        }
    }
}

static esp_err_t __unregister_common(event_loop_t *loop,
                                     esp_event_base_t base, int32_t id,
                                     esp_event_handler_t handler)
{
    if ((loop == NULL) || (handler == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    bool found = false;
    for (handler_node_t *n = loop->handlers; n != NULL; n = n->next) {
        if (!n->is_instance && !n->pending_delete
            && __same_registration(n, base, id, handler)) {
            n->pending_delete = true;
            found = true;
            break;
        }
    }
    if (!loop->dispatching) {
        __purge_deleted(loop);
    }
    return found ? ESP_OK : ESP_ERR_NOT_FOUND;
}

static esp_err_t __unregister_instance(event_loop_t *loop,
                                       esp_event_base_t base, int32_t id,
                                       esp_event_handler_instance_t instance)
{
    if ((loop == NULL) || (instance == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    (void)base;
    (void)id;
    handler_node_t *target = (handler_node_t *)instance;
    for (handler_node_t *n = loop->handlers; n != NULL; n = n->next) {
        if (n == target) {
            n->pending_delete = true;
            if (!loop->dispatching) {
                __purge_deleted(loop);
            }
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/* Walk the handler chain once for msg, invoking every matching handler. */
static void __dispatch_one(event_loop_t *loop, event_msg_t *msg)
{
    loop->dispatching = true;
    for (handler_node_t *n = loop->handlers; n != NULL; n = n->next) {
        if (n->pending_delete) { continue; }
        if (!__handler_matches(n, msg->base, msg->id)) { continue; }
        n->handler(n->arg, msg->base, msg->id, msg->data);
    }
    loop->dispatching = false;
    __purge_deleted(loop);

    if (msg->data != NULL) {
        vsf_heap_free(msg->data);
        msg->data = NULL;
    }
}

/* Background dispatcher. Exits when loop->exit_flag becomes true. */
static void __event_task_entry(void *arg)
{
    event_loop_t *loop = (event_loop_t *)arg;
    while (!loop->exit_flag) {
        event_msg_t msg;
        if (xQueueReceive(loop->queue, &msg, pdMS_TO_TICKS(50)) == pdTRUE) {
            __dispatch_one(loop, &msg);
        }
    }
    vTaskDelete(NULL);
}

/* Create a loop. If task_name != NULL, spawn a dispatcher task. Otherwise
 * the caller is expected to drive the loop via esp_event_loop_run(). */
static esp_err_t __loop_create(const esp_event_loop_args_t *args,
                               event_loop_t **out)
{
    if ((args == NULL) || (out == NULL) || (args->queue_size <= 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    event_loop_t *loop = (event_loop_t *)vsf_heap_malloc(sizeof(*loop));
    if (loop == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(loop, 0, sizeof(*loop));

    loop->queue = xQueueCreate((UBaseType_t)args->queue_size, sizeof(event_msg_t));
    if (loop->queue == NULL) {
        vsf_heap_free(loop);
        return ESP_ERR_NO_MEM;
    }

    if (args->task_name != NULL) {
        uint32_t stack = (args->task_stack_size != 0)
                       ? args->task_stack_size
                       : VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_STACK;
        BaseType_t rc = xTaskCreate(__event_task_entry, args->task_name,
                                    stack, loop, args->task_priority,
                                    &loop->task);
        if (rc != pdPASS) {
            vQueueDelete(loop->queue);
            vsf_heap_free(loop);
            return ESP_FAIL;
        }
    }

    *out = loop;
    return ESP_OK;
}

static esp_err_t __loop_delete(event_loop_t *loop)
{
    if (loop == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Signal dispatcher to exit; it self-deletes via vTaskDelete(NULL). */
    loop->exit_flag = true;
    if (loop->task != NULL) {
        /* Give the dispatcher time to observe the flag and exit cleanly.
         * This mirrors the ESP-IDF contract that a deleted loop no longer
         * invokes handlers, while acknowledging our shim has no join API. */
        vTaskDelay(pdMS_TO_TICKS(80));
    }

    /* Drain any undelivered messages so their data copies are freed. */
    if (loop->queue != NULL) {
        event_msg_t msg;
        while (xQueueReceive(loop->queue, &msg, 0) == pdTRUE) {
            if (msg.data != NULL) {
                vsf_heap_free(msg.data);
            }
        }
        vQueueDelete(loop->queue);
    }

    /* Release handler chain (including nodes marked pending_delete). */
    handler_node_t *n = loop->handlers;
    while (n != NULL) {
        handler_node_t *next = n->next;
        vsf_heap_free(n);
        n = next;
    }

    vsf_heap_free(loop);
    return ESP_OK;
}

static esp_err_t __post_event(event_loop_t *loop, esp_event_base_t base,
                              int32_t id, const void *data, size_t data_size,
                              TickType_t ticks_to_wait)
{
    if (loop == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((data == NULL) && (data_size != 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    event_msg_t msg = {
        .base       = base,
        .id         = id,
        .data       = NULL,
        .data_size  = data_size,
    };

    if (data_size > 0) {
        msg.data = vsf_heap_malloc(data_size);
        if (msg.data == NULL) {
            return ESP_ERR_NO_MEM;
        }
        memcpy(msg.data, data, data_size);
    }

    if (xQueueSend(loop->queue, &msg, ticks_to_wait) != pdTRUE) {
        if (msg.data != NULL) {
            vsf_heap_free(msg.data);
        }
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

/*============================ PUBLIC API ====================================*/

esp_err_t esp_event_loop_create_default(void)
{
    if (__vsf_espidf_default_loop != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_event_loop_args_t args = {
        .queue_size         = VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_QUEUE_SIZE,
        .task_name          = "esp_event_def",
        .task_priority      = VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_PRIO,
        .task_stack_size    = VSF_ESPIDF_CFG_DEFAULT_EVENT_LOOP_STACK,
        .task_core_id       = 0,
    };
    return __loop_create(&args, &__vsf_espidf_default_loop);
}

esp_err_t esp_event_loop_delete_default(void)
{
    if (__vsf_espidf_default_loop == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    event_loop_t *loop = __vsf_espidf_default_loop;
    __vsf_espidf_default_loop = NULL;
    return __loop_delete(loop);
}

esp_err_t esp_event_loop_create(const esp_event_loop_args_t *event_loop_args,
                                esp_event_loop_handle_t *event_loop)
{
    if (event_loop == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    event_loop_t *loop = NULL;
    esp_err_t err = __loop_create(event_loop_args, &loop);
    if (err == ESP_OK) {
        *event_loop = (esp_event_loop_handle_t)loop;
    }
    return err;
}

esp_err_t esp_event_loop_delete(esp_event_loop_handle_t event_loop)
{
    return __loop_delete((event_loop_t *)event_loop);
}

esp_err_t esp_event_loop_run(esp_event_loop_handle_t event_loop,
                             TickType_t ticks_to_run)
{
    event_loop_t *loop = (event_loop_t *)event_loop;
    if ((loop == NULL) || (loop->task != NULL)) {
        /* run() is only meaningful for loops without a dispatcher task. */
        return ESP_ERR_INVALID_ARG;
    }
    event_msg_t msg;
    while (xQueueReceive(loop->queue, &msg, ticks_to_run) == pdTRUE) {
        __dispatch_one(loop, &msg);
        ticks_to_run = 0;   /* Non-blocking thereafter: drain what's ready. */
    }
    return ESP_OK;
}

esp_err_t esp_event_handler_register(esp_event_base_t event_base,
                                     int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *event_handler_arg)
{
    return __register_common(__vsf_espidf_default_loop, event_base, event_id,
                             event_handler, event_handler_arg,
                             false, NULL);
}

esp_err_t esp_event_handler_unregister(esp_event_base_t event_base,
                                       int32_t event_id,
                                       esp_event_handler_t event_handler)
{
    return __unregister_common(__vsf_espidf_default_loop, event_base, event_id,
                               event_handler);
}

esp_err_t esp_event_handler_register_with(esp_event_loop_handle_t event_loop,
                                          esp_event_base_t event_base,
                                          int32_t event_id,
                                          esp_event_handler_t event_handler,
                                          void *event_handler_arg)
{
    return __register_common((event_loop_t *)event_loop, event_base, event_id,
                             event_handler, event_handler_arg,
                             false, NULL);
}

esp_err_t esp_event_handler_unregister_with(esp_event_loop_handle_t event_loop,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            esp_event_handler_t event_handler)
{
    return __unregister_common((event_loop_t *)event_loop, event_base,
                               event_id, event_handler);
}

esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base,
                                              int32_t event_id,
                                              esp_event_handler_t event_handler,
                                              void *event_handler_arg,
                                              esp_event_handler_instance_t *instance)
{
    if (instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return __register_common(__vsf_espidf_default_loop, event_base, event_id,
                             event_handler, event_handler_arg,
                             true, (void **)instance);
}

esp_err_t esp_event_handler_instance_register_with(esp_event_loop_handle_t event_loop,
                                                   esp_event_base_t event_base,
                                                   int32_t event_id,
                                                   esp_event_handler_t event_handler,
                                                   void *event_handler_arg,
                                                   esp_event_handler_instance_t *instance)
{
    if (instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return __register_common((event_loop_t *)event_loop, event_base, event_id,
                             event_handler, event_handler_arg,
                             true, (void **)instance);
}

esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base,
                                                int32_t event_id,
                                                esp_event_handler_instance_t instance)
{
    return __unregister_instance(__vsf_espidf_default_loop, event_base,
                                 event_id, instance);
}

esp_err_t esp_event_handler_instance_unregister_with(esp_event_loop_handle_t event_loop,
                                                     esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     esp_event_handler_instance_t instance)
{
    return __unregister_instance((event_loop_t *)event_loop, event_base,
                                 event_id, instance);
}

esp_err_t esp_event_post(esp_event_base_t event_base, int32_t event_id,
                         const void *event_data, size_t event_data_size,
                         TickType_t ticks_to_wait)
{
    if (__vsf_espidf_default_loop == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return __post_event(__vsf_espidf_default_loop, event_base, event_id,
                  event_data, event_data_size, ticks_to_wait);
}

esp_err_t esp_event_post_to(esp_event_loop_handle_t event_loop,
                            esp_event_base_t event_base, int32_t event_id,
                            const void *event_data, size_t event_data_size,
                            TickType_t ticks_to_wait)
{
    return __post_event((event_loop_t *)event_loop, event_base, event_id,
                  event_data, event_data_size, ticks_to_wait);
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_EVENT
