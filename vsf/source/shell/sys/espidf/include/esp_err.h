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
 * Clean-room re-implementation of ESP-IDF public API "esp_err.h".
 *
 * This file is authored from the ESP-IDF public API reference only. No
 * code is copied or derived from the ESP-IDF source tree. Names, error
 * code values and macro semantics are kept compatible so that unmodified
 * ESP-IDF example applications can compile and link against this header.
 *
 * Baseline: ESP-IDF v5.x public API.
 */

#ifndef __VSF_ESPIDF_ESP_ERR_H__
#define __VSF_ESPIDF_ESP_ERR_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

typedef int esp_err_t;

/*============================ MACROS ========================================*/

#define ESP_OK                          0
#define ESP_FAIL                        -1

/* Generic error codes. Values are fixed by ESP-IDF public API contract. */
#define ESP_ERR_NO_MEM                  0x101
#define ESP_ERR_INVALID_ARG             0x102
#define ESP_ERR_INVALID_STATE           0x103
#define ESP_ERR_INVALID_SIZE            0x104
#define ESP_ERR_NOT_FOUND               0x105
#define ESP_ERR_NOT_SUPPORTED           0x106
#define ESP_ERR_TIMEOUT                 0x107
#define ESP_ERR_INVALID_RESPONSE        0x108
#define ESP_ERR_INVALID_CRC             0x109
#define ESP_ERR_INVALID_VERSION         0x10A
#define ESP_ERR_INVALID_MAC             0x10B
#define ESP_ERR_NOT_FINISHED            0x10C
#define ESP_ERR_NOT_ALLOWED             0x10D

/* Per-component error code bases. Component-specific errors add an offset
 * on top of these bases; the concrete codes are declared in each
 * component's own header. */
#define ESP_ERR_WIFI_BASE               0x3000
#define ESP_ERR_MESH_BASE               0x4000
#define ESP_ERR_FLASH_BASE              0x6000
#define ESP_ERR_HW_CRYPTO_BASE          0xc000
#define ESP_ERR_MEMPROT_BASE            0xd000

/*============================ PROTOTYPES ====================================*/

/* Returns a human-readable name for an esp_err_t value. If the code is not
 * recognized, returns the string "UNKNOWN ERROR". The returned pointer is
 * valid for the lifetime of the program. */
const char * esp_err_to_name(esp_err_t code);

/* Reentrant variant. Writes the error name into caller-provided buffer.
 * Unknown codes are rendered as "UNKNOWN ERROR 0x<hex>". Always returns
 * buf on success; buf is always NUL-terminated when buflen > 0. */
const char * esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);

/* Reports an ESP_ERROR_CHECK() failure and terminates execution.
 * Never returns. Implementation logs the failure through VSF trace and
 * then invokes VSF_ASSERT(0). */
void _esp_error_check_failed(esp_err_t rc, const char *file, int line,
                              const char *function, const char *expression);

/* Reports an ESP_ERROR_CHECK_WITHOUT_ABORT() failure. Returns normally so
 * that the caller may inspect the error code. */
void _esp_error_check_failed_without_abort(esp_err_t rc, const char *file,
                                            int line, const char *function,
                                            const char *expression);

/*============================ CHECK MACROS ==================================*/

/* Aborts when the wrapped expression does not evaluate to ESP_OK. */
#define ESP_ERROR_CHECK(x)                                                  \
    do {                                                                    \
        esp_err_t __err_rc = (esp_err_t)(x);                                \
        if (__err_rc != ESP_OK) {                                           \
            _esp_error_check_failed(__err_rc, __FILE__, __LINE__,           \
                                    __func__, #x);                          \
        }                                                                   \
    } while (0)

#if defined(__GNUC__) || defined(__clang__)
/* GCC statement-expression form: yields the esp_err_t value, so callers
 * may write  rc = ESP_ERROR_CHECK_WITHOUT_ABORT(f());  */
#define ESP_ERROR_CHECK_WITHOUT_ABORT(x)                                    \
    ({                                                                      \
        esp_err_t __err_rc = (esp_err_t)(x);                                \
        if (__err_rc != ESP_OK) {                                           \
            _esp_error_check_failed_without_abort(__err_rc, __FILE__,       \
                                                  __LINE__, __func__, #x);  \
        }                                                                   \
        __err_rc;                                                           \
    })
#else
/* Non-GCC fallback: statement form only, return value not available. */
#define ESP_ERROR_CHECK_WITHOUT_ABORT(x)                                    \
    do {                                                                    \
        esp_err_t __err_rc = (esp_err_t)(x);                                \
        if (__err_rc != ESP_OK) {                                           \
            _esp_error_check_failed_without_abort(__err_rc, __FILE__,       \
                                                  __LINE__, __func__, #x);  \
        }                                                                   \
    } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_ESP_ERR_H__
