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
 * esp_vfs_fat.h -> component/fs/driver/fatfs bridge.
 *
 * Current coverage (deliberate, documented limitations)
 * -----------------------------------------------------
 * 1. Runtime mount (_mount_rw_wl / _mount_ro) relies on VSF's malfs
 *    auto-mounter, which scans for a valid MBR or VBR on the underlying
 *    vk_mal_t. On a *pre-formatted* partition image this works end to
 *    end (FAT12/16/32 layouts are all recognised). On a blank partition
 *    it returns ESP_FAIL.
 *
 * 2. _format_rw_wl / format_if_mount_failed = true path returns
 *    ESP_ERR_NOT_SUPPORTED. The VSF component/fs/driver/fatfs driver in
 *    this tree is an in-house FAT implementation that does not expose a
 *    standalone mkfs-style entry point. Implementing mkfs here would
 *    duplicate non-trivial MBR+BPB+FAT layout code; it is intentionally
 *    deferred until that driver grows a public format helper.
 *
 * 3. wl_handle_t is treated as a dense slot index. Wear-levelling is
 *    NOT inserted between the fs driver and the partition; the handle
 *    exists only so that caller code compiles. See esp_vfs_fat.h.
 *
 * 4. SD card variants (esp_vfs_fat_sdmmc_*) are not provided here.
 *
 * Threading: all APIs MUST be invoked from a vsf_thread_t context.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_FATFS == ENABLED

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "component/fs/vsf_fs.h"
#include "component/mal/vsf_mal.h"
#include "service/heap/vsf_heap.h"

#include <sys/mount.h>

#include "../vsf_espidf.h"
#include "esp_partition.h"
#include "esp_vfs_fat.h"

#if !defined(VSF_USE_LINUX) || VSF_USE_LINUX != ENABLED
#   error "esp_vfs_fat port requires VSF_USE_LINUX == ENABLED"
#endif
#if !defined(VSF_USE_FS) || VSF_USE_FS != ENABLED \
    || !defined(VSF_FS_USE_FATFS) || VSF_FS_USE_FATFS != ENABLED
#   error "esp_vfs_fat port requires VSF_FS_USE_FATFS == ENABLED"
#endif
#if !defined(VSF_USE_MAL) || VSF_USE_MAL != ENABLED
#   error "esp_vfs_fat port requires VSF_USE_MAL == ENABLED"
#endif

/*============================ MACROS ========================================*/

#ifndef VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES
#   define VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES   2
#endif

/*============================ TYPES =========================================*/

typedef struct __fat_slot_t {
    bool                        in_use;
    bool                        readonly;
    char                        base_path[32];
    char                        partition_label[16 + 1];
    vsf_linux_fsdata_auto_t    *fsdata;
} __fat_slot_t;

/*============================ LOCAL VARIABLES ===============================*/

static __fat_slot_t __fat_slots[VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES];

/*============================ PROTOTYPES ====================================*/

static __fat_slot_t * __fat_slot_find_by_base(const char *base_path);
static __fat_slot_t * __fat_slot_find_by_label(const char *partition_label);
static int32_t        __fat_slot_index(const __fat_slot_t *slot);
static __fat_slot_t * __fat_slot_alloc(void);
static void           __fat_slot_free(__fat_slot_t *slot);
static esp_err_t      __fat_mount_common(const char *base_path,
                                         const char *partition_label,
                                         bool readonly,
                                         wl_handle_t *wl_handle_out);

/*============================ IMPLEMENTATION ================================*/

static __fat_slot_t * __fat_slot_find_by_base(const char *base_path)
{
    if (base_path == NULL) {
        return NULL;
    }
    for (uint16_t i = 0; i < VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES; i++) {
        if (    __fat_slots[i].in_use
            &&  (strcmp(__fat_slots[i].base_path, base_path) == 0)) {
            return &__fat_slots[i];
        }
    }
    return NULL;
}

static __fat_slot_t * __fat_slot_find_by_label(const char *partition_label)
{
    if (partition_label == NULL) {
        return NULL;
    }
    for (uint16_t i = 0; i < VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES; i++) {
        if (    __fat_slots[i].in_use
            &&  (strcmp(__fat_slots[i].partition_label, partition_label) == 0)) {
            return &__fat_slots[i];
        }
    }
    return NULL;
}

static int32_t __fat_slot_index(const __fat_slot_t *slot)
{
    return (int32_t)(slot - __fat_slots);
}

static __fat_slot_t * __fat_slot_alloc(void)
{
    for (uint16_t i = 0; i < VSF_ESPIDF_CFG_FATFS_MAX_INSTANCES; i++) {
        if (!__fat_slots[i].in_use) {
            return &__fat_slots[i];
        }
    }
    return NULL;
}

static void __fat_slot_free(__fat_slot_t *slot)
{
    if (slot->fsdata != NULL) {
        vsf_heap_free(slot->fsdata);
        slot->fsdata = NULL;
    }
    memset(slot, 0, sizeof(*slot));
}

static esp_err_t __fat_mount_common(const char *base_path,
                                    const char *partition_label,
                                    bool readonly,
                                    wl_handle_t *wl_handle_out)
{
    if (    (base_path == NULL)
        ||  (base_path[0] != '/')
        ||  (partition_label == NULL)
        ||  (partition_label[0] == '\0')) {
        return ESP_ERR_INVALID_ARG;
    }
    if (__fat_slot_find_by_base(base_path) != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (__fat_slot_find_by_label(partition_label) != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const esp_partition_t *part = esp_partition_find_first(
            ESP_PARTITION_TYPE_ANY,
            ESP_PARTITION_SUBTYPE_ANY,
            partition_label);
    if (part == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    if ((part->mal == NULL) || (part->size == 0)) {
        return ESP_ERR_INVALID_STATE;
    }

    __fat_slot_t *slot = __fat_slot_alloc();
    if (slot == NULL) {
        return ESP_ERR_NO_MEM;
    }

    vsf_linux_fsdata_auto_t *fsdata = vsf_heap_calloc(1, sizeof(*fsdata));
    if (fsdata == NULL) {
        return ESP_ERR_NO_MEM;
    }
    fsdata->mal = (vk_mal_t *)part->mal;

    memset(slot, 0, sizeof(*slot));
    slot->in_use   = true;
    slot->readonly = readonly;
    strncpy(slot->base_path,       base_path,       sizeof(slot->base_path) - 1);
    strncpy(slot->partition_label, partition_label, sizeof(slot->partition_label) - 1);
    slot->fsdata = fsdata;

    // fsop == NULL selects the malfs auto-mounter (MBR/VBR probe). A
    // blank partition has neither and will fail here; see file header.
    if (mount(NULL, slot->base_path, NULL, 0, fsdata) != 0) {
        __fat_slot_free(slot);
        return ESP_FAIL;
    }
    if (wl_handle_out != NULL) {
        *wl_handle_out = __fat_slot_index(slot);
    }
    return ESP_OK;
}

esp_err_t esp_vfs_fat_spiflash_mount_rw_wl(
        const char *base_path,
        const char *partition_label,
        const esp_vfs_fat_mount_config_t *mount_config,
        wl_handle_t *wl_handle)
{
    if (wl_handle != NULL) {
        *wl_handle = WL_INVALID_HANDLE;
    }
    esp_err_t rc = __fat_mount_common(base_path, partition_label, false,
                                      wl_handle);
    if (    (rc != ESP_OK)
        &&  (mount_config != NULL)
        &&  mount_config->format_if_mount_failed) {
        // VSF's in-house fatfs driver does not expose a format helper;
        // see file header.
        return ESP_ERR_NOT_SUPPORTED;
    }
    return rc;
}

esp_err_t esp_vfs_fat_spiflash_mount_ro(
        const char *base_path,
        const char *partition_label,
        const esp_vfs_fat_mount_config_t *mount_config)
{
    (void)mount_config;
    return __fat_mount_common(base_path, partition_label, true, NULL);
}

esp_err_t esp_vfs_fat_spiflash_unmount_rw_wl(
        const char *base_path,
        wl_handle_t wl_handle)
{
    __fat_slot_t *slot = __fat_slot_find_by_base(base_path);
    if (slot == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    // wl_handle is either WL_INVALID_HANDLE (caller didn't track it) or
    // the slot index this API handed out.
    if ((wl_handle != WL_INVALID_HANDLE) && (wl_handle != __fat_slot_index(slot))) {
        return ESP_ERR_INVALID_ARG;
    }
    (void)umount(slot->base_path);
    __fat_slot_free(slot);
    return ESP_OK;
}

esp_err_t esp_vfs_fat_spiflash_unmount_ro(
        const char *base_path,
        const char *partition_label)
{
    __fat_slot_t *slot = __fat_slot_find_by_base(base_path);
    if (slot == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    if (    (partition_label != NULL)
        &&  (strcmp(partition_label, slot->partition_label) != 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    (void)umount(slot->base_path);
    __fat_slot_free(slot);
    return ESP_OK;
}

esp_err_t esp_vfs_fat_spiflash_format_rw_wl(
        const char *base_path,
        const char *partition_label)
{
    (void)base_path; (void)partition_label;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_FATFS
