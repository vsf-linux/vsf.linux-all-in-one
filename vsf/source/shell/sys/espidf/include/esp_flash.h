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
 * Clean-room re-implementation of ESP-IDF public API "esp_flash.h".
 *
 * Baseline: ESP-IDF v5.x public API. Only the surface area that
 * real-world application / middleware code actually touches is exposed:
 * read / write / erase_region / erase_chip / get_size / get_chip_id /
 * plus the default-chip singleton.
 *
 * This shim layers directly on the same vk_mal_t "root mal" that the
 * esp_partition port already consumes (see esp_partition_port.c). The
 * full ESP-IDF chip stack (spi_flash_host_driver_t + chip_driver_t +
 * os_functions) is not modelled: there is no physical SPI controller
 * in the host environment.
 *
 * Encrypted variants (read_encrypted / write_encrypted) simply alias
 * the plain variants; the shim has no flash-encryption layer.
 *
 * mmap is intentionally NOT exposed here. It is a cache/MMU facility
 * on real targets and only meaningful for the main flash; applications
 * that need a direct pointer should use esp_partition_mmap() which is
 * already wired through the root mal's VSF_MAL_LOCAL_BUFFER capability.
 */

#ifndef __VSF_ESPIDF_ESP_FLASH_H__
#define __VSF_ESPIDF_ESP_FLASH_H__

/*============================ INCLUDES ======================================*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/* esp_flash-specific error codes. Values follow ESP-IDF v5.x. */
#define ESP_ERR_FLASH_NOT_INITIALISED   (ESP_ERR_FLASH_BASE + 1)
#define ESP_ERR_FLASH_UNSUPPORTED_HOST  (ESP_ERR_FLASH_BASE + 2)
#define ESP_ERR_FLASH_UNSUPPORTED_CHIP  (ESP_ERR_FLASH_BASE + 3)
#define ESP_ERR_FLASH_PROTECTED         (ESP_ERR_FLASH_BASE + 4)

/*============================ TYPES =========================================*/

/* Forward declarations. The root mal is opaque at this header. */
struct vk_mal_t;

/* SPI flash I/O mode hint. Tracked as a field on esp_flash_t for API
 * compatibility; the shim ignores the value. */
typedef enum {
    SPI_FLASH_SLOWRD = 0,
    SPI_FLASH_FASTRD,
    SPI_FLASH_DOUT,
    SPI_FLASH_DIO,
    SPI_FLASH_QOUT,
    SPI_FLASH_QIO,
    SPI_FLASH_OPI_STR,
    SPI_FLASH_OPI_DTR,
    SPI_FLASH_READ_MODE_MAX,
} esp_flash_io_mode_t;

/* Runtime descriptor of one flash chip. Field layout matches the public
 * portion of the ESP-IDF struct; application code only reads size / chip_id.
 *
 * Shim-specific fields:
 *   mal      vk_mal_t backing the whole chip address space. Caller MUST
 *            keep this mal valid for the lifetime of the chip descriptor.
 */
typedef struct esp_flash_t {
    void                   *host;        /* unused; kept for ABI parity */
    void                   *chip_drv;    /* unused; kept for ABI parity */
    void                   *os_func;     /* unused; kept for ABI parity */
    void                   *os_func_data;

    esp_flash_io_mode_t     read_mode;
    uint32_t                size;        /* chip size in bytes */
    uint32_t                chip_id;     /* JEDEC ID or user-supplied */
    uint32_t                busy;        /* kept for ABI parity, always 0 */

    /* Opaque pointer to the backing vk_mal_t. Callers must NOT dereference
     * this field directly; it is exposed to keep esp_flash_t a complete
     * type so static allocation and struct embedding work. */
    void                   *mal;
} esp_flash_t;

/* Process-wide default chip handle. Points at the same root_mal that
 * vsf_espidf_init() was given via vsf_espidf_cfg_t::partition.root_mal.
 * NULL until the esp_flash sub-system has been initialised. All
 * esp_flash_*() entry points accept a chip == NULL shorthand meaning
 * "use the default chip".
 */
extern esp_flash_t *esp_flash_default_chip;

/*============================ PROTOTYPES ====================================*/

/* --- Lifecycle -------------------------------------------------------- */

/* Probe a chip descriptor. On real ESP-IDF this queries the physical flash
 * for its JEDEC ID, unique ID, SFDP tables etc.; in this shim it is a
 * validation pass: the caller-supplied esp_flash_t must have `mal` set and
 * `size` must fit inside the backing mal. Returns ESP_OK on success. */
extern esp_err_t esp_flash_init(esp_flash_t *chip);

/* No-op in this shim (present for ABI parity with ESP-IDF). Returns
 * ESP_OK unconditionally. */
extern esp_err_t esp_flash_init_os_functions(esp_flash_t *chip, int host_id,
                                             int *out_attached_id);
extern esp_err_t esp_flash_deinit_os_functions(esp_flash_t *chip);

/* --- Identity --------------------------------------------------------- */

/* Retrieve the flash chip ID. chip == NULL selects the default chip. */
extern esp_err_t esp_flash_read_id(esp_flash_t *chip, uint32_t *out_id);

/* Retrieve the flash chip size in bytes. Equivalent to chip->size.
 * chip == NULL selects the default chip. */
extern esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size);

/* On real ESP-IDF this returns the chip's physical size even when the
 * configured size is smaller. In this shim it always matches get_size().
 */
extern esp_err_t esp_flash_get_physical_size(esp_flash_t *chip,
                                             uint32_t *out_size);

/* --- I/O -------------------------------------------------------------- */

/* Synchronous read / write / erase against the chip's address space.
 * All offsets are absolute chip addresses; bounds are validated against
 * chip->size. Must be called from a stack-owner thread (vsf_thread_t):
 * the underlying vk_mal_t I/O is a peda subcall and only short-circuits
 * to synchronous completion in that context. */
extern esp_err_t esp_flash_read(esp_flash_t *chip, void *buffer,
                                uint32_t address, uint32_t length);
extern esp_err_t esp_flash_write(esp_flash_t *chip, const void *buffer,
                                 uint32_t address, uint32_t length);
extern esp_err_t esp_flash_erase_region(esp_flash_t *chip,
                                        uint32_t start, uint32_t len);
extern esp_err_t esp_flash_erase_chip(esp_flash_t *chip);

/* Encrypted variants: alias to plain read/write. Provided for API
 * compatibility only -- there is no flash-encryption layer in this shim.
 */
extern esp_err_t esp_flash_read_encrypted(esp_flash_t *chip, uint32_t address,
                                          void *out_buffer, uint32_t length);
extern esp_err_t esp_flash_write_encrypted(esp_flash_t *chip, uint32_t address,
                                           const void *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_ESP_FLASH_H__ */
