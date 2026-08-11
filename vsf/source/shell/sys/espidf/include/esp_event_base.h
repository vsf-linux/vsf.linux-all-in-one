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
 * Clean-room re-implementation of ESP-IDF public API "esp_event_base.h".
 *
 * Authored from ESP-IDF v5.x public API only. No code copied from the
 * ESP-IDF source tree.
 */

#ifndef __VSF_ESPIDF_ESP_EVENT_BASE_H__
#define __VSF_ESPIDF_ESP_EVENT_BASE_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/* An event base is a string-pointer tag that groups related event ids. The
 * ESP-IDF contract is that all users of a base reference the SAME global
 * variable, so that pointer equality identifies the base. This port also
 * falls back to strcmp when pointers differ, for defensive matching. */
typedef const char * esp_event_base_t;

/* Opaque loop handle (default loop is accessed via the _default APIs). */
typedef void * esp_event_loop_handle_t;

/* Opaque handler-instance handle returned by the *_instance_register APIs. */
typedef void * esp_event_handler_instance_t;

/* Handler entry point. event_data/event_data_size come from esp_event_post.
 * The buffer pointed to by event_data is only valid for the duration of the
 * handler call (the dispatcher owns the storage). */
typedef void (*esp_event_handler_t)(void *event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void *event_data);

/*============================ MACROS ========================================*/

/* Wildcards accepted by register/unregister/post. */
#define ESP_EVENT_ANY_BASE      ((esp_event_base_t)NULL)
#define ESP_EVENT_ANY_ID        (-1)

/* Declare a base symbol in a header; define it exactly once in one .c file
 * with ESP_EVENT_DEFINE_BASE. Matching relies on a unique symbol address. */
#define ESP_EVENT_DECLARE_BASE(id)      extern esp_event_base_t id
#define ESP_EVENT_DEFINE_BASE(id)       esp_event_base_t id = #id

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_ESP_EVENT_BASE_H__
