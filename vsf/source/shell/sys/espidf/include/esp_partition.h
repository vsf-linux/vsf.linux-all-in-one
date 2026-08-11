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

#ifndef __VSF_ESPIDF_ESP_PARTITION_H__
#define __VSF_ESPIDF_ESP_PARTITION_H__

/*============================ INCLUDES ======================================*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/* Label storage is fixed at 16 bytes in the ESP-IDF partition table binary,
 * plus one byte NUL terminator in the runtime struct.
 */
#define ESP_PARTITION_LABEL_MAX     16

/*============================ TYPES =========================================*/

/* Partition top-level type. Matches ESP-IDF v5.x. */
typedef enum {
    ESP_PARTITION_TYPE_APP  = 0x00,
    ESP_PARTITION_TYPE_DATA = 0x01,
    ESP_PARTITION_TYPE_ANY  = 0xFF,
} esp_partition_type_t;

/* Partition subtype. Only the values consumed by the currently implemented
 * upstream modules are defined; additional subtypes can be added as new
 * modules come online.
 */
typedef enum {
    /* APP subtypes. */
    ESP_PARTITION_SUBTYPE_APP_FACTORY    = 0x00,
    ESP_PARTITION_SUBTYPE_APP_OTA_MIN    = 0x10,
    ESP_PARTITION_SUBTYPE_APP_OTA_0      = 0x10,
    ESP_PARTITION_SUBTYPE_APP_OTA_1      = 0x11,
    ESP_PARTITION_SUBTYPE_APP_OTA_MAX    = 0x1F,
    ESP_PARTITION_SUBTYPE_APP_TEST       = 0x20,

    /* DATA subtypes. */
    ESP_PARTITION_SUBTYPE_DATA_OTA       = 0x00,
    ESP_PARTITION_SUBTYPE_DATA_PHY       = 0x01,
    ESP_PARTITION_SUBTYPE_DATA_NVS       = 0x02,
    ESP_PARTITION_SUBTYPE_DATA_COREDUMP  = 0x03,
    ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS  = 0x04,
    ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM  = 0x05,
    ESP_PARTITION_SUBTYPE_DATA_UNDEFINED = 0x06,
    ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD  = 0x80,
    ESP_PARTITION_SUBTYPE_DATA_FAT       = 0x81,
    ESP_PARTITION_SUBTYPE_DATA_SPIFFS    = 0x82,
    ESP_PARTITION_SUBTYPE_DATA_LITTLEFS  = 0x83,

    ESP_PARTITION_SUBTYPE_ANY            = 0xFF,
} esp_partition_subtype_t;

/* Flash / memory-capability hint for esp_partition_mmap(). This enum exists
 * in the ESP-IDF public API but in this shim only SPI/DATA/INSTRUCTION are
 * distinguished; all are collapsed onto the root mal's local buffer.
 */
typedef enum {
    ESP_PARTITION_MMAP_DATA         = 0,
    ESP_PARTITION_MMAP_INST         = 1,
} esp_partition_mmap_memory_t;

typedef uint32_t esp_partition_mmap_handle_t;

/* Forward declarations. */
struct vk_mal_t;

/* Runtime description of one partition. Field layout follows ESP-IDF so that
 * application code that peeks into it (type/subtype/address/size/label)
 * keeps working.
 *
 * Fields specific to this shim:
 *   mal   internal vk_mim_mal_t slice over the root mal. Callers MUST NOT
 *         dereference this field; it is exposed here only because the
 *         public esp_partition_t must be a complete type.
 */
typedef struct esp_partition_t {
    esp_partition_type_t    type;
    esp_partition_subtype_t subtype;
    uint32_t                address;
    uint32_t                size;
    uint32_t                erase_size;
    char                    label[ESP_PARTITION_LABEL_MAX + 1];
    bool                    encrypted;
    bool                    readonly;
    /* Opaque back-end handle (vk_mim_mal_t *). */
    void                   *mal;
} esp_partition_t;

/* Opaque iterator handle returned by esp_partition_find(). */
typedef struct esp_partition_iterator_opaque_t *esp_partition_iterator_t;

/*============================ PROTOTYPES ====================================*/

/* --- Discovery --------------------------------------------------------- */

/* Create an iterator over partitions matching (type, subtype, label).
 * A NULL label matches every label. subtype == ESP_PARTITION_SUBTYPE_ANY
 * matches every subtype; type == ESP_PARTITION_TYPE_ANY matches every type.
 *
 * The iterator must be released with esp_partition_iterator_release().
 * Returns NULL if no partition matches or if the partition sub-system has
 * not been initialised.
 */
extern esp_partition_iterator_t esp_partition_find(esp_partition_type_t    type,
                                                    esp_partition_subtype_t subtype,
                                                    const char             *label);

/* Convenience form returning the first match directly, or NULL. The
 * returned pointer is owned by the sub-system and stays valid until the
 * partition is deregistered.
 */
extern const esp_partition_t * esp_partition_find_first(esp_partition_type_t    type,
                                                         esp_partition_subtype_t subtype,
                                                         const char             *label);

extern const esp_partition_t * esp_partition_get(esp_partition_iterator_t iterator);
extern esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t iterator);
extern void esp_partition_iterator_release(esp_partition_iterator_t iterator);

/* Identity check (two pointers describe the same partition). */
extern bool esp_partition_check_identity(const esp_partition_t *a,
                                         const esp_partition_t *b);

/* --- I/O --------------------------------------------------------------- */

/* Synchronous read / write / erase over the partition's address space.
 * All offsets are relative to the start of the partition; bounds are
 * validated against partition->size. Must be called from a stack-owner
 * thread (vsf_thread_t): the underlying vk_mal_t I/O is a peda subcall
 * and only short-circuits to synchronous completion in that context.
 */
extern esp_err_t esp_partition_read(const esp_partition_t *partition,
                                    size_t src_offset, void *dst, size_t size);
extern esp_err_t esp_partition_write(const esp_partition_t *partition,
                                     size_t dst_offset, const void *src,
                                     size_t size);
extern esp_err_t esp_partition_erase_range(const esp_partition_t *partition,
                                           size_t offset, size_t size);

/* read_raw / write_raw on ESP-IDF bypass the flash encryption layer. This
 * shim has no flash encryption, so they are aliases of read / write and
 * are provided for API compatibility only.
 */
extern esp_err_t esp_partition_read_raw(const esp_partition_t *partition,
                                        size_t src_offset, void *dst,
                                        size_t size);
extern esp_err_t esp_partition_write_raw(const esp_partition_t *partition,
                                         size_t dst_offset, const void *src,
                                         size_t size);

/* --- Memory mapping ---------------------------------------------------- */

/* Map a [offset, offset+size) region of the partition into a directly
 * addressable pointer. Succeeds only when the root mal advertises the
 * VSF_MAL_LOCAL_BUFFER feature (otherwise returns ESP_ERR_NOT_SUPPORTED):
 * mmap is a hardware capability and not every back-end provides it.
 */
extern esp_err_t esp_partition_mmap(const esp_partition_t *partition,
                                    size_t offset, size_t size,
                                    esp_partition_mmap_memory_t memory,
                                    const void **out_ptr,
                                    esp_partition_mmap_handle_t *out_handle);
extern void esp_partition_munmap(esp_partition_mmap_handle_t handle);

/* --- Verification ------------------------------------------------------ */

/* Re-validate a partition pointer against the live registry. Returns the
 * canonical partition pointer on success, NULL if the pointer no longer
 * references a registered partition.
 */
extern const esp_partition_t * esp_partition_verify(const esp_partition_t *partition);

/* --- Runtime registration --------------------------------------------- */

/* Register an externally-located partition at runtime. The backing storage
 * is the sub-system's configured root mal; `flash_chip` is accepted for API
 * parity but ignored (the shim has a single root store).
 *
 * Returns ESP_OK on success; ESP_ERR_NO_MEM when the dynamic-registration
 * slot pool is exhausted; ESP_ERR_INVALID_ARG on bad arguments;
 * ESP_ERR_INVALID_STATE when dynamic registration is disabled at build
 * time (VSF_ESPIDF_CFG_PARTITION_DYNAMIC == DISABLED).
 */
extern esp_err_t esp_partition_register_external(void *flash_chip,
                                                 size_t offset, size_t size,
                                                 const char *label,
                                                 esp_partition_type_t    type,
                                                 esp_partition_subtype_t subtype,
                                                 const esp_partition_t **out_partition);
extern esp_err_t esp_partition_deregister_external(const esp_partition_t *partition);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_ESP_PARTITION_H__ */
