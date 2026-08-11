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
 * esp_vfs_fat.h -- mount/unmount helpers binding a FAT volume stored in an
 * ESP-IDF partition to a POSIX path prefix.
 *
 * Differences from upstream ESP-IDF worth calling out:
 *
 *   - wl_handle_t is returned as an opaque id (>= 0) for API shape
 *     compatibility. VSF does not implement a wear-levelling layer yet,
 *     so _rw_wl is functionally equivalent to a raw FAT volume directly
 *     on top of the partition. Erase wear is the caller's responsibility.
 *     The handle is accepted by _unmount_rw_wl() for symmetry.
 *
 *   - The SD-card variants (esp_vfs_fat_sdmmc_*) are intentionally not
 *     provided here; they depend on Stage 4 peripheral drivers.
 *
 * Threading: all APIs MUST be invoked from a vsf_thread_t context. The
 * underlying vk_fs_mount / open / read / write peda sub-calls complete
 * synchronously on return only when the caller is a thread.
 */

#ifndef __VSF_ESPIDF_ESP_VFS_FAT_H__
#define __VSF_ESPIDF_ESP_VFS_FAT_H__

/*============================ INCLUDES ======================================*/

#include "esp_err.h"
#include "esp_vfs.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

// Opaque wear-levelling handle. Values >= 0 are valid; WL_INVALID_HANDLE
// (-1) means "no wear levelling bound" and is returned when the caller
// supplied NULL for the handle output.
typedef int32_t wl_handle_t;
#define WL_INVALID_HANDLE           (-1)

// Matches ESP-IDF layout. Fields not used by the VSF backend are still
// present so that struct initialisers compile unchanged.
typedef struct esp_vfs_fat_mount_config_t {
    bool     format_if_mount_failed;
    int      max_files;
    size_t   allocation_unit_size;
    bool     disk_status_check_enable;
    bool     use_one_fat;
} esp_vfs_fat_mount_config_t;

/*============================ PROTOTYPES ====================================*/

/*
 * Mount a FAT volume read/write on top of the partition identified by
 * partition_label, exposed at base_path (e.g. "/spiflash").
 *
 * Parameters
 *   base_path         Mount point, e.g. "/spiflash". Must start with '/'.
 *   partition_label   Target partition label, as registered in the
 *                     vsf_espidf_partition_cfg_t entries table.
 *   mount_config      Behavioural knobs. May be NULL for defaults.
 *   wl_handle         Output. Receives a non-negative id on success; the
 *                     same id must be passed to _unmount_rw_wl().
 *                     May be NULL if the caller has no use for it.
 *
 * Returns ESP_OK on success, otherwise:
 *   ESP_ERR_INVALID_ARG    base_path / partition_label invalid.
 *   ESP_ERR_NOT_FOUND      partition_label does not match any entry.
 *   ESP_ERR_NO_MEM         out-of-memory for fs info / cache allocation.
 *   ESP_ERR_INVALID_STATE  base_path already occupied or mount failed
 *                          (and format_if_mount_failed was false).
 *   ESP_FAIL               underlying vk_fs_mount returned an error.
 */
extern esp_err_t esp_vfs_fat_spiflash_mount_rw_wl(
        const char *base_path,
        const char *partition_label,
        const esp_vfs_fat_mount_config_t *mount_config,
        wl_handle_t *wl_handle);

/* Read-only mount. No format/rewrite is ever attempted. */
extern esp_err_t esp_vfs_fat_spiflash_mount_ro(
        const char *base_path,
        const char *partition_label,
        const esp_vfs_fat_mount_config_t *mount_config);

/* Tear down a mount created by _mount_rw_wl. wl_handle must match the
 * value returned by the paired mount call (or WL_INVALID_HANDLE if the
 * caller passed NULL into the mount). */
extern esp_err_t esp_vfs_fat_spiflash_unmount_rw_wl(
        const char *base_path,
        wl_handle_t wl_handle);

/* Tear down a read-only mount created by _mount_ro. */
extern esp_err_t esp_vfs_fat_spiflash_unmount_ro(
        const char *base_path,
        const char *partition_label);

/* Format the FAT volume at the given partition, without mounting it.
 * A subsequent _mount_* call will see a freshly-formatted volume. */
extern esp_err_t esp_vfs_fat_spiflash_format_rw_wl(
        const char *base_path,
        const char *partition_label);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_ESP_VFS_FAT_H__
