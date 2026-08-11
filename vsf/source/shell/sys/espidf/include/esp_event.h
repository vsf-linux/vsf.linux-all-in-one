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
 * Clean-room re-implementation of ESP-IDF public API "esp_event.h".
 *
 * Authored from ESP-IDF v5.x public API only. No code copied from the
 * ESP-IDF source tree. The VSF port drives the event loop with the VSF
 * FreeRTOS shim (xQueue + xTaskCreate).
 */

#ifndef __VSF_ESPIDF_ESP_EVENT_H__
#define __VSF_ESPIDF_ESP_EVENT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_event_base.h"

/* FreeRTOS typedefs required by the loop arguments / ticks-to-wait parms. */
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/* Arguments for esp_event_loop_create(). Mirrors ESP-IDF v5.x layout so that
 * user code can be dropped in as-is. Unknown fields (queue size too small,
 * zero stack, etc.) return ESP_ERR_INVALID_ARG. */
typedef struct {
    int32_t         queue_size;         /*!< Dispatch queue depth             */
    const char *    task_name;          /*!< Dispatcher task name (may NULL)  */
    UBaseType_t     task_priority;      /*!< Dispatcher task priority         */
    uint32_t        task_stack_size;    /*!< Dispatcher task stack in bytes   */
    BaseType_t      task_core_id;       /*!< Accepted but ignored on VSF host */
} esp_event_loop_args_t;

/*============================ PROTOTYPES ====================================*/

/* Lifecycle -------------------------------------------------------------- */

/* Create / delete the process-wide default loop. All _default APIs target it. */
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_loop_delete_default(void);

/* Create / delete a user-owned loop with an explicit dispatcher task. */
esp_err_t esp_event_loop_create(const esp_event_loop_args_t *event_loop_args,
                                esp_event_loop_handle_t *event_loop);
esp_err_t esp_event_loop_delete(esp_event_loop_handle_t event_loop);

/* Run a user-owned loop's dispatcher inline for up to ticks_to_run. Only
 * meaningful for loops created with task_name==NULL (no background task).
 * Returns ESP_OK when time budget elapsed, ESP_ERR_TIMEOUT immediately when
 * ticks_to_run is 0 and the queue is empty. */
esp_err_t esp_event_loop_run(esp_event_loop_handle_t event_loop,
                             TickType_t ticks_to_run);

/* Register / unregister handlers on the default loop -------------------- */

esp_err_t esp_event_handler_register(esp_event_base_t event_base,
                                     int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *event_handler_arg);

esp_err_t esp_event_handler_unregister(esp_event_base_t event_base,
                                       int32_t event_id,
                                       esp_event_handler_t event_handler);

/* Register / unregister handlers on a specific loop --------------------- */

esp_err_t esp_event_handler_register_with(esp_event_loop_handle_t event_loop,
                                          esp_event_base_t event_base,
                                          int32_t event_id,
                                          esp_event_handler_t event_handler,
                                          void *event_handler_arg);

esp_err_t esp_event_handler_unregister_with(esp_event_loop_handle_t event_loop,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            esp_event_handler_t event_handler);

/* Handler instance registration (returns an opaque handle used to unregister
 * when multiple instances of the same handler fn/arg share a base+id). */
esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base,
                                              int32_t event_id,
                                              esp_event_handler_t event_handler,
                                              void *event_handler_arg,
                                              esp_event_handler_instance_t *instance);

esp_err_t esp_event_handler_instance_register_with(esp_event_loop_handle_t event_loop,
                                                   esp_event_base_t event_base,
                                                   int32_t event_id,
                                                   esp_event_handler_t event_handler,
                                                   void *event_handler_arg,
                                                   esp_event_handler_instance_t *instance);

esp_err_t esp_event_handler_instance_unregister(esp_event_base_t event_base,
                                                int32_t event_id,
                                                esp_event_handler_instance_t instance);

esp_err_t esp_event_handler_instance_unregister_with(esp_event_loop_handle_t event_loop,
                                                     esp_event_base_t event_base,
                                                     int32_t event_id,
                                                     esp_event_handler_instance_t instance);

/* Post events ------------------------------------------------------------ */

/* Post to the default loop. event_data is copied into an internally-owned
 * buffer (size event_data_size); the handler receives a pointer to that
 * copy. ticks_to_wait bounds the enqueue wait on a full queue. */
esp_err_t esp_event_post(esp_event_base_t event_base,
                         int32_t event_id,
                         const void *event_data,
                         size_t event_data_size,
                         TickType_t ticks_to_wait);

esp_err_t esp_event_post_to(esp_event_loop_handle_t event_loop,
                            esp_event_base_t event_base,
                            int32_t event_id,
                            const void *event_data,
                            size_t event_data_size,
                            TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_ESP_EVENT_H__
