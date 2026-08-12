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
 * Port implementation for "esp_system.h" on VSF.
 *
 * - esp_restart         -> vsf_arch_reset (implemented per-arch under
 *                          vsf/source/hal/arch/.../*_generic.c).
 * - esp_reset_reason    -> static var, flipped to ESP_RST_SW just before
 *                          vsf_arch_reset so a post-reboot run reads it
 *                          via its own boot-time init. In the current
 *                          (non-persistent) Windows host port the value
 *                          is reset to POWERON every process launch.
 * - esp_get_free_heap_size / esp_get_minimum_free_heap_size: use
 *   vsf_heap_statistics when VSF heap statistics are enabled.
 * - esp_random / esp_fill_random: if the user has injected a vsf_rng_t
 *   via vsf_espidf_cfg_t.rng, requests are routed there via
 *   vsf_rng_generate_request with an event-based async->sync wrapper
 *   (on_ready_cb wakes the caller through vsf_eda_post_evt). Otherwise
 *   falls back to a xorshift32 PRNG seeded from vsf_systimer_get_tick().
 * - esp_get_idf_version: composed from VSF_ESPIDF_CFG_VERSION_* macros.
 * - esp_chip_info: reports a single-core CHIP_VSF_HOST.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_SYSTEM == ENABLED

#include "esp_system.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "service/trace/vsf_trace.h"
#if defined(VSF_USE_HEAP) && VSF_USE_HEAP == ENABLED
#   include "service/heap/vsf_heap.h"
#endif

#include <stdio.h>
#include <string.h>

/*============================ MACROS ========================================*/

#define __VSF_ESPIDF_STR(x)                 #x
#define __VSF_ESPIDF_XSTR(x)                __VSF_ESPIDF_STR(x)

#define __VSF_ESPIDF_IDF_VERSION_STRING                                     \
    "v" __VSF_ESPIDF_XSTR(VSF_ESPIDF_CFG_VERSION_MAJOR) "."                 \
        __VSF_ESPIDF_XSTR(VSF_ESPIDF_CFG_VERSION_MINOR) "."                 \
        __VSF_ESPIDF_XSTR(VSF_ESPIDF_CFG_VERSION_PATCH) "-vsf"

/*============================ LOCAL VARIABLES ===============================*/

static esp_reset_reason_t   __vsf_espidf_reset_reason = ESP_RST_POWERON;
static uint32_t             __vsf_espidf_rand_state;
static bool                 __vsf_espidf_rand_seeded;

/*============================ IMPLEMENTATION ================================*/

void esp_restart(void)
{
    __vsf_espidf_reset_reason = ESP_RST_SW;
    vsf_trace_info("esp_restart(): invoking vsf_arch_reset()" VSF_TRACE_CFG_LINEEND);
    vsf_arch_reset();
    // vsf_arch_reset() should not return; loop defensively.
    while (1) {}
}

esp_reset_reason_t esp_reset_reason(void)
{
    return __vsf_espidf_reset_reason;
}

uint32_t esp_get_free_heap_size(void)
{
#if defined(VSF_USE_HEAP) && VSF_USE_HEAP == ENABLED && VSF_HEAP_CFG_STATISTICS == ENABLED
    vsf_heap_statistics_t stats;
    vsf_heap_statistics(&stats);
    if (stats.all_size >= stats.used_size) {
        return (uint32_t)(stats.all_size - stats.used_size);
    }
    return 0;
#else
    return 0;
#endif
}

uint32_t esp_get_minimum_free_heap_size(void)
{
    // VSF does not track a historical low-water value; report current free.
    return esp_get_free_heap_size();
}

static uint32_t __vsf_espidf_xorshift32(void)
{
    if (!__vsf_espidf_rand_seeded) {
        __vsf_espidf_rand_state = (uint32_t)vsf_systimer_get_tick();
        if (__vsf_espidf_rand_state == 0) {
            __vsf_espidf_rand_state = 0xA5A5A5A5u;
        }
        __vsf_espidf_rand_seeded = true;
    }
    uint32_t x = __vsf_espidf_rand_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    __vsf_espidf_rand_state = x;
    return x;
}

#if VSF_HAL_USE_RNG == ENABLED
// Async->sync bridge for vsf_rng_generate_request. Each caller keeps a
// per-invocation context on its own stack so concurrent callers don't
// trample each other; the underlying multiplex_rng serialises hardware
// access via its own waiting_queue. The callback may fire from ISR or
// kernel task; vsf_eda_post_evt is safe in both contexts.
typedef struct {
    vsf_eda_t *eda;
} __vsf_espidf_rng_ctx_t;

static void __vsf_espidf_rng_on_ready(void *param, uint32_t *buffer, uint32_t num)
{
    (void)buffer;
    (void)num;
    __vsf_espidf_rng_ctx_t *ctx = (__vsf_espidf_rng_ctx_t *)param;
    vsf_eda_post_evt(ctx->eda, VSF_EVT_USER);
}

// Fill `num` uint32_t words using the injected vsf_rng_t. Must be called
// from a task/thread context (the wait is implemented via
// vsf_thread_wait_for_evt). Returns true on success.
static bool __vsf_espidf_rng_fill_u32(vsf_rng_t *rng, uint32_t *buffer, uint32_t num)
{
    vsf_eda_t *cur = vsf_eda_get_cur();
    if (cur == NULL) {
        // Not in task context -> cannot block for the async reply. Leave
        // the decision to the caller (it will fall back to xorshift).
        return false;
    }
    __vsf_espidf_rng_ctx_t ctx = { .eda = cur };
    vsf_err_t err = vsf_rng_generate_request(rng, buffer, num,
                                             &ctx, __vsf_espidf_rng_on_ready);
    if (err != VSF_ERR_NONE) {
        return false;
    }
    vsf_thread_wait_for_evt(VSF_EVT_USER);
    return true;
}
#endif

uint32_t esp_random(void)
{
#if VSF_HAL_USE_RNG == ENABLED
    vsf_rng_t *rng = vsf_espidf_get_rng();
    if (rng != NULL) {
        uint32_t v;
        if (__vsf_espidf_rng_fill_u32(rng, &v, 1)) {
            return v;
        }
    }
#endif
    return __vsf_espidf_xorshift32();
}

void esp_fill_random(void *buf, size_t len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }
    uint8_t *dst = (uint8_t *)buf;
#if VSF_HAL_USE_RNG == ENABLED
    vsf_rng_t *rng = vsf_espidf_get_rng();
    if (rng != NULL) {
        // Issue one aligned request for the bulk, then a single-word
        // request for any trailing bytes. Keeps memcpy overhead minimal
        // while staying agnostic to caller buffer alignment.
        uint32_t words = (uint32_t)(len / sizeof(uint32_t));
        if (words > 0) {
            if (((uintptr_t)dst & (sizeof(uint32_t) - 1)) == 0) {
                if (!__vsf_espidf_rng_fill_u32(rng,
                                               (uint32_t *)dst, words)) {
                    goto __fallback;
                }
                dst += words * sizeof(uint32_t);
                len -= words * sizeof(uint32_t);
            } else {
                while (words-- > 0) {
                    uint32_t v;
                    if (!__vsf_espidf_rng_fill_u32(rng, &v, 1)) {
                        goto __fallback;
                    }
                    memcpy(dst, &v, sizeof(v));
                    dst += sizeof(v);
                    len -= sizeof(v);
                }
            }
        }
        if (len > 0) {
            uint32_t v;
            if (!__vsf_espidf_rng_fill_u32(rng, &v, 1)) {
                goto __fallback;
            }
            memcpy(dst, &v, len);
        }
        return;
    }
__fallback:
#endif
    while (len >= sizeof(uint32_t)) {
        uint32_t v = __vsf_espidf_xorshift32();
        memcpy(dst, &v, sizeof(v));
        dst += sizeof(v);
        len -= sizeof(v);
    }
    if (len > 0) {
        uint32_t v = __vsf_espidf_xorshift32();
        memcpy(dst, &v, len);
    }
}

const char * esp_get_idf_version(void)
{
    return __VSF_ESPIDF_IDF_VERSION_STRING;
}

void esp_chip_info(esp_chip_info_t *out_info)
{
    if (out_info == NULL) {
        return;
    }
    out_info->model     = CHIP_VSF_HOST;
    out_info->features  = 0;        // No wireless features on host.
    out_info->revision  = 0;
    out_info->cores     = 1;
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_SYSTEM
