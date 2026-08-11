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
 * esp_flash.h -> component/mal bridge.
 *
 * The shim models a single "default chip" bound to the same vk_mal_t
 * root that vsf_espidf_init() was handed via the partition cfg. Every
 * esp_flash_*() entry point delegates directly to vk_mal_read / write /
 * erase on that mal (no mim_mal slice -- the chip IS the root). chip ==
 * NULL is accepted as shorthand for the default chip.
 *
 * Multiple chips: supported in principle. Callers can allocate their
 * own esp_flash_t, set `mal` / `size` on it, then call esp_flash_init().
 * Operations are routed through the caller's mal independent of the
 * default chip.
 *
 * Synchronisation: all APIs MUST be invoked from a vsf_thread_t context
 * so that the underlying vk_mal_* peda subcalls complete synchronously
 * on return. Calling from a bare vsf_eda_t handler is undefined.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_ESP_FLASH == ENABLED

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define __VSF_MAL_CLASS_INHERIT__
#include "component/mal/vsf_mal.h"

#include "../vsf_espidf.h"
#include "esp_flash.h"

#if !defined(VSF_USE_MAL) || VSF_USE_MAL != ENABLED
#   error "esp_flash port requires VSF_USE_MAL == ENABLED"
#endif

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

/* Published chip handle. Filled in by vsf_espidf_esp_flash_init() when a
 * non-NULL root mal is provided; stays NULL otherwise (callers that rely
 * on the default chip will see the same ESP_ERR_FLASH_NOT_INITIALISED
 * real ESP-IDF would return before the flash driver has been brought up).
 */
esp_flash_t *esp_flash_default_chip = NULL;

/*============================ LOCAL VARIABLES ===============================*/

/* Back-end storage for the default chip descriptor. Using a static slot
 * means the pointer exported through esp_flash_default_chip stays valid
 * for the whole process lifetime and does not depend on heap state. */
static esp_flash_t __default_chip = { 0 };
static bool        __is_inited    = false;

/*============================ PROTOTYPES ====================================*/

/* Called from vsf_espidf_init() after the partition sub-system has had a
 * chance to attach its own root mal. root_mal may be NULL, in which case
 * the default chip is left NULL. */
extern void vsf_espidf_esp_flash_init(vk_mal_t *root_mal);

/*============================ IMPLEMENTATION ================================*/

void vsf_espidf_esp_flash_init(vk_mal_t *root_mal)
{
    if (__is_inited) {
        return;
    }
    __is_inited = true;

    if (root_mal == NULL) {
        /* No backing store -- leave esp_flash_default_chip == NULL. Any
         * esp_flash_*() call will then surface ESP_ERR_INVALID_STATE. */
        return;
    }

    memset(&__default_chip, 0, sizeof(__default_chip));
    __default_chip.read_mode = SPI_FLASH_FASTRD;
    /* Chip size mirrors the root mal. 32-bit truncation is intentional:
     * esp_flash's public API uses uint32_t for offsets and sizes, and no
     * real ESP-IDF target exceeds 4 GiB. */
    __default_chip.size    = (uint32_t)root_mal->size;
    __default_chip.chip_id = 0;     /* unknown; callers rarely inspect this */
    __default_chip.busy    = 0;
    __default_chip.mal     = root_mal;

    esp_flash_default_chip = &__default_chip;
}

/* --- Internal helpers -------------------------------------------------- */

static esp_flash_t * __chip_resolve(esp_flash_t *chip)
{
    return (chip != NULL) ? chip : esp_flash_default_chip;
}

static esp_err_t __chip_bounds(esp_flash_t *chip, uint32_t addr, uint32_t len)
{
    if ((chip == NULL) || (chip->mal == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (chip->size == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((addr > chip->size) || (len > chip->size)
            || ((uint64_t)addr + (uint64_t)len > (uint64_t)chip->size)) {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

/* --- Lifecycle -------------------------------------------------------- */

esp_err_t esp_flash_init(esp_flash_t *chip)
{
    if (chip == NULL) {
        /* NULL means "re-init / validate the default chip". There is
         * nothing to probe; just confirm the backing mal is present. */
        chip = esp_flash_default_chip;
    }
    if (chip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (chip->mal == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vk_mal_t *mal = (vk_mal_t *)chip->mal;
    /* Auto-fill size when caller left it zero. */
    if (chip->size == 0) {
        chip->size = (uint32_t)mal->size;
    } else if ((uint64_t)chip->size > (uint64_t)mal->size) {
        /* Caller-declared size must fit inside the backing mal. */
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t esp_flash_init_os_functions(esp_flash_t *chip, int host_id,
                                      int *out_attached_id)
{
    (void)chip;
    (void)host_id;
    if (out_attached_id != NULL) {
        *out_attached_id = 0;
    }
    return ESP_OK;
}

esp_err_t esp_flash_deinit_os_functions(esp_flash_t *chip)
{
    (void)chip;
    return ESP_OK;
}

/* --- Identity --------------------------------------------------------- */

esp_err_t esp_flash_read_id(esp_flash_t *chip, uint32_t *out_id)
{
    if (out_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    chip = __chip_resolve(chip);
    if (chip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    *out_id = chip->chip_id;
    return ESP_OK;
}

esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size)
{
    if (out_size == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    chip = __chip_resolve(chip);
    if ((chip == NULL) || (chip->mal == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    *out_size = chip->size;
    return ESP_OK;
}

esp_err_t esp_flash_get_physical_size(esp_flash_t *chip, uint32_t *out_size)
{
    /* No distinction between configured and physical size in this shim. */
    return esp_flash_get_size(chip, out_size);
}

/* --- I/O -------------------------------------------------------------- */

esp_err_t esp_flash_read(esp_flash_t *chip, void *buffer,
                         uint32_t address, uint32_t length)
{
    if (buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    chip = __chip_resolve(chip);
    esp_err_t rc = __chip_bounds(chip, address, length);
    if (rc != ESP_OK) {
        return rc;
    }
    if (length == 0) {
        return ESP_OK;
    }
    vsf_err_t err = vk_mal_read((vk_mal_t *)chip->mal,
                                address, length, (uint8_t *)buffer);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_flash_write(esp_flash_t *chip, const void *buffer,
                          uint32_t address, uint32_t length)
{
    if (buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    chip = __chip_resolve(chip);
    esp_err_t rc = __chip_bounds(chip, address, length);
    if (rc != ESP_OK) {
        return rc;
    }
    if (length == 0) {
        return ESP_OK;
    }
    vsf_err_t err = vk_mal_write((vk_mal_t *)chip->mal,
                                 address, length, (uint8_t *)buffer);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_flash_erase_region(esp_flash_t *chip,
                                  uint32_t start, uint32_t len)
{
    chip = __chip_resolve(chip);
    esp_err_t rc = __chip_bounds(chip, start, len);
    if (rc != ESP_OK) {
        return rc;
    }
    if (len == 0) {
        return ESP_OK;
    }
    vk_mal_t *mal = (vk_mal_t *)chip->mal;
    /* Root mal may not expose an erase op (mem_mal / file_mal have no
     * flash-style sector erase). Probe feature + driver slot first so
     * callers get NOT_SUPPORTED instead of an assert in vk_mal_erase. */
    if (!(mal->feature & VSF_MAL_ERASABLE)
            || (mal->drv == NULL) || (mal->drv->erase == NULL)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    vsf_err_t err = vk_mal_erase(mal, start, len);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_flash_erase_chip(esp_flash_t *chip)
{
    chip = __chip_resolve(chip);
    if ((chip == NULL) || (chip->mal == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_flash_erase_region(chip, 0, chip->size);
}

/* --- Encrypted variants ----------------------------------------------- */

esp_err_t esp_flash_read_encrypted(esp_flash_t *chip, uint32_t address,
                                   void *out_buffer, uint32_t length)
{
    return esp_flash_read(chip, out_buffer, address, length);
}

esp_err_t esp_flash_write_encrypted(esp_flash_t *chip, uint32_t address,
                                    const void *buffer, uint32_t length)
{
    return esp_flash_write(chip, buffer, address, length);
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_ESP_FLASH */
