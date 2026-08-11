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
 * esp_littlefs.h -- mount/unmount helpers binding a LittleFS volume stored
 * in an ESP-IDF partition to a POSIX path prefix. API matches the widely
 * used joltwallet/esp_littlefs component so that code written against it
 * compiles unchanged.
 *
 * Threading: all APIs MUST be invoked from a vsf_thread_t context.
 */

#ifndef __VSF_ESPIDF_ESP_LITTLEFS_H__
#define __VSF_ESPIDF_ESP_LITTLEFS_H__

/*============================ INCLUDES ======================================*/

#include "esp_err.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

typedef struct esp_vfs_littlefs_conf_t {
    const char *base_path;          // mount point, e.g. "/littlefs"
    const char *partition_label;    // target partition label
    uint8_t     format_if_mount_failed : 1;
    uint8_t     read_only              : 1;
    uint8_t     dont_mount             : 1;  // prepare only, no mount
    uint8_t     grow_on_mount          : 1;  // accepted for compat; no-op
} esp_vfs_littlefs_conf_t;

/*============================ PROTOTYPES ====================================*/

/*
 * Register and (by default) mount a LittleFS volume.
 *
 * Returns ESP_OK on success, otherwise:
 *   ESP_ERR_INVALID_ARG    conf / base_path / partition_label invalid.
 *   ESP_ERR_NOT_FOUND      partition_label does not match any entry.
 *   ESP_ERR_NO_MEM         out-of-memory for fs info allocation.
 *   ESP_ERR_INVALID_STATE  base_path already in use.
 *   ESP_FAIL               underlying vk_fs_mount returned an error.
 */
extern esp_err_t esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t *conf);

/* Unregister a previously registered LittleFS instance, identified by its
 * partition label. */
extern esp_err_t esp_vfs_littlefs_unregister(const char *partition_label);

/* Force-format the LittleFS volume on the given partition. The volume
 * MUST be unregistered when this is called. */
extern esp_err_t esp_littlefs_format(const char *partition_label);

/* Query used/total bytes of a mounted LittleFS volume. Either pointer may
 * be NULL to skip that field.
 *
 * Accuracy note: VSF's current lfs port does not expose a cheap used-bytes
 * counter; total_bytes is derived from the partition size and used_bytes
 * is reported as 0 until a proper traversal helper lands. Callers relying
 * on exact values should treat them as lower bounds.
 */
extern esp_err_t esp_littlefs_info(const char *partition_label,
                                   size_t *total_bytes,
                                   size_t *used_bytes);

/* Return true iff a LittleFS instance is currently registered against
 * this partition label. */
extern bool      esp_littlefs_mounted(const char *partition_label);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_ESP_LITTLEFS_H__
