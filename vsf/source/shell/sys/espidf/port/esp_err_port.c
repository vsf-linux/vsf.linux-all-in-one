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
 * Port implementation for "esp_err.h" on VSF.
 *
 * - esp_err_to_name / esp_err_to_name_r: table-driven lookup, generic
 *   codes only. Component-specific ranges (WIFI/MESH/FLASH/...) return
 *   the generic "UNKNOWN ERROR" string until their owning ports populate
 *   additional tables via esp_err_to_name (future hook).
 *
 * - _esp_error_check_failed: emits diagnostic via vsf_trace_error (no
 *   dependency on libc stdout) and traps via VSF_ESPIDF_ASSERT. Does
 *   not return.
 * - _esp_error_check_failed_without_abort: same diagnostic via
 *   vsf_trace_warning, returns to caller.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_ERR == ENABLED

#include "esp_err.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "service/trace/vsf_trace.h"

#include <stdio.h>
#include <string.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/

typedef struct {
    esp_err_t       code;
    const char *    name;
} __vsf_espidf_err_entry_t;

/*============================ LOCAL VARIABLES ===============================*/

static const __vsf_espidf_err_entry_t __vsf_espidf_err_generic[] = {
    { ESP_OK,                       "ESP_OK"                        },
    { ESP_FAIL,                     "ESP_FAIL"                      },
    { ESP_ERR_NO_MEM,               "ESP_ERR_NO_MEM"                },
    { ESP_ERR_INVALID_ARG,          "ESP_ERR_INVALID_ARG"           },
    { ESP_ERR_INVALID_STATE,        "ESP_ERR_INVALID_STATE"         },
    { ESP_ERR_INVALID_SIZE,         "ESP_ERR_INVALID_SIZE"          },
    { ESP_ERR_NOT_FOUND,            "ESP_ERR_NOT_FOUND"             },
    { ESP_ERR_NOT_SUPPORTED,        "ESP_ERR_NOT_SUPPORTED"         },
    { ESP_ERR_TIMEOUT,              "ESP_ERR_TIMEOUT"               },
    { ESP_ERR_INVALID_RESPONSE,     "ESP_ERR_INVALID_RESPONSE"      },
    { ESP_ERR_INVALID_CRC,          "ESP_ERR_INVALID_CRC"           },
    { ESP_ERR_INVALID_VERSION,      "ESP_ERR_INVALID_VERSION"       },
    { ESP_ERR_INVALID_MAC,          "ESP_ERR_INVALID_MAC"           },
    { ESP_ERR_NOT_FINISHED,         "ESP_ERR_NOT_FINISHED"          },
    { ESP_ERR_NOT_ALLOWED,          "ESP_ERR_NOT_ALLOWED"           },
};

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

static const char * __vsf_espidf_err_lookup(esp_err_t code)
{
    for (size_t i = 0;
         i < sizeof(__vsf_espidf_err_generic) / sizeof(__vsf_espidf_err_generic[0]);
         i++) {
        if (__vsf_espidf_err_generic[i].code == code) {
            return __vsf_espidf_err_generic[i].name;
        }
    }
    return NULL;
}

const char * esp_err_to_name(esp_err_t code)
{
    const char *name = __vsf_espidf_err_lookup(code);
    return (name != NULL) ? name : "UNKNOWN ERROR";
}

const char * esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen)
{
    if ((buf == NULL) || (buflen == 0)) {
        return NULL;
    }

    const char *name = __vsf_espidf_err_lookup(code);
    if (name != NULL) {
        // known: copy as-is, guarantee NUL termination
        size_t n = strlen(name);
        if (n >= buflen) {
            n = buflen - 1;
        }
        memcpy(buf, name, n);
        buf[n] = '\0';
    } else {
        // unknown: render with hex value
        snprintf(buf, buflen, "UNKNOWN ERROR 0x%x", (unsigned int)code);
    }
    return buf;
}

void _esp_error_check_failed(esp_err_t rc, const char *file, int line,
                              const char *function, const char *expression)
{
    vsf_trace_error("ESP_ERROR_CHECK failed: rc=0x%x (%s) at %s:%d in %s(): %s"
                    VSF_TRACE_CFG_LINEEND,
                    (unsigned int)rc, esp_err_to_name(rc),
                    (file != NULL) ? file : "?", line,
                    (function != NULL) ? function : "?",
                    (expression != NULL) ? expression : "?");
    VSF_ESPIDF_ASSERT(0);
    // Belt-and-braces: guarantee no return even if VSF_ASSERT is a no-op.
    while (1) {}
}

void _esp_error_check_failed_without_abort(esp_err_t rc, const char *file,
                                            int line, const char *function,
                                            const char *expression)
{
    vsf_trace_warning("ESP_ERROR_CHECK_WITHOUT_ABORT failed: rc=0x%x (%s) at %s:%d in %s(): %s"
                      VSF_TRACE_CFG_LINEEND,
                      (unsigned int)rc, esp_err_to_name(rc),
                      (file != NULL) ? file : "?", line,
                      (function != NULL) ? function : "?",
                      (expression != NULL) ? expression : "?");
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_ERR
