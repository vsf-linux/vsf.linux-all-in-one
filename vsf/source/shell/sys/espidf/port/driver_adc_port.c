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
 * Port implementation for ESP-IDF "esp_adc/adc_oneshot.h" (v5.1) on VSF.
 *
 * Bridges the ESP-IDF one-shot ADC API onto
 *     hal/driver/common/template/vsf_template_adc.h
 *
 * Data flow per adc_oneshot_read():
 *   1. Pick the cached vsf_adc_channel_cfg_t for the requested channel.
 *   2. Take the unit mutex.
 *   3. Issue vsf_adc_channel_request_once() (async).
 *   4. Block on a binary semaphore released by the ADC conversion-
 *      complete ISR.
 *   5. Copy the sample out of the internal buffer, mask it to the
 *      configured bit-width, and return.
 *
 * Resource injection: vsf_adc_t *const *pool plus pool_count, same
 * pattern as the other driver_*_port.c files.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_ADC == ENABLED

#include "driver/adc_oneshot.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "hal/driver/driver.h"

#if !defined(VSF_USE_HEAP) || VSF_USE_HEAP != ENABLED
#   error "VSF_ESPIDF_CFG_USE_DRIVER_ADC requires VSF_USE_HEAP"
#endif
#include "service/heap/vsf_heap.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

/*============================ MACROS ========================================*/

#define __ADC_MAX_CHANNELS          10  /* matches adc_channel_t range */

/*============================ TYPES =========================================*/

typedef struct __adc_chan_ctx_t {
    bool                    configured;
    adc_atten_t             atten;
    adc_bitwidth_t          bitwidth;       /* resolved (>= 9) */
} __adc_chan_ctx_t;

typedef struct adc_oneshot_unit_ctx_t {
    vsf_adc_t              *hw;
    adc_unit_t              unit_id;
    bool                    in_use;
    bool                    hw_inited;
    SemaphoreHandle_t       mutex;
    SemaphoreHandle_t       done;
    vsf_adc_irq_mask_t      last_irq;
    __adc_chan_ctx_t        channels[__ADC_MAX_CHANNELS];
    /* Scratch buffer for the conversion result (one 32-bit sample). */
    uint32_t                sample;
} adc_oneshot_unit_ctx_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                        inited;
    uint16_t                    pool_count;
    vsf_adc_t *const           *pool;
    adc_oneshot_unit_ctx_t      units[2];   /* at most ADC_UNIT_1 / ADC_UNIT_2 */
} __adc = { 0 };

/*============================ PROTOTYPES ====================================*/

extern void vsf_espidf_adc_init(const vsf_espidf_adc_cfg_t *cfg);

static void __adc_isr_handler(void *target_ptr, vsf_adc_t *adc_ptr,
                              vsf_adc_irq_mask_t irq_mask);

/*============================ IMPLEMENTATION ================================*/

void vsf_espidf_adc_init(const vsf_espidf_adc_cfg_t *cfg)
{
    if (__adc.inited) {
        return;
    }
    __adc.inited = true;

    if (cfg != NULL) {
        __adc.pool       = cfg->pool;
        __adc.pool_count = cfg->pool_count;
    }
    for (size_t i = 0; i < sizeof(__adc.units) / sizeof(__adc.units[0]); i++) {
        __adc.units[i].unit_id = (adc_unit_t)i;
    }
}

static void __adc_isr_handler(void *target_ptr, vsf_adc_t *adc_ptr,
                              vsf_adc_irq_mask_t irq_mask)
{
    (void)adc_ptr;
    adc_oneshot_unit_ctx_t *u = (adc_oneshot_unit_ctx_t *)target_ptr;
    if (u == NULL) {
        return;
    }
    u->last_irq = irq_mask;
    if (irq_mask & VSF_ADC_IRQ_MASK_CPL) {
        BaseType_t woken = pdFALSE;
        if (u->done != NULL) {
            xSemaphoreGiveFromISR(u->done, &woken);
        }
        portYIELD_FROM_ISR(woken);
    }
}

/* Bring the underlying vsf_adc instance online if not already. */
static esp_err_t __adc_hw_up(adc_oneshot_unit_ctx_t *u)
{
    if (u->hw_inited) {
        return ESP_OK;
    }

    vsf_adc_cfg_t cfg = { 0 };
    cfg.mode          = VSF_ADC_REF_VDD_1 | VSF_ADC_DATA_ALIGN_RIGHT
                      | VSF_ADC_SCAN_CONV_SINGLE_MODE;
    cfg.clock_hz      = 0;      /* driver-default */
    cfg.isr.handler_fn = __adc_isr_handler;
    cfg.isr.target_ptr = u;
    cfg.isr.prio      = (vsf_arch_prio_t)0;

    if (vsf_adc_init(u->hw, &cfg) != VSF_ERR_NONE) {
        return ESP_FAIL;
    }
    fsm_rt_t rt;
    do {
        rt = vsf_adc_enable(u->hw);
    } while (rt == fsm_rt_on_going);

    vsf_adc_irq_enable(u->hw, VSF_ADC_IRQ_MASK_CPL);
    u->hw_inited = true;
    return ESP_OK;
}

esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config,
                               adc_oneshot_unit_handle_t *ret_unit)
{
    if ((init_config == NULL) || (ret_unit == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t idx = (uint32_t)init_config->unit_id;
    if (idx >= sizeof(__adc.units) / sizeof(__adc.units[0])) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((__adc.pool == NULL) || (idx >= __adc.pool_count) ||
        (__adc.pool[idx] == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }

    adc_oneshot_unit_ctx_t *u = &__adc.units[idx];
    if (u->in_use) {
        return ESP_ERR_INVALID_STATE;
    }

    u->hw       = __adc.pool[idx];
    u->in_use   = true;
    u->mutex    = xSemaphoreCreateMutex();
    u->done     = xSemaphoreCreateBinary();
    if ((u->mutex == NULL) || (u->done == NULL)) {
        if (u->mutex != NULL) { vSemaphoreDelete(u->mutex); u->mutex = NULL; }
        if (u->done  != NULL) { vSemaphoreDelete(u->done);  u->done  = NULL; }
        u->in_use = false;
        return ESP_ERR_NO_MEM;
    }
    memset(u->channels, 0, sizeof(u->channels));

    esp_err_t err = __adc_hw_up(u);
    if (err != ESP_OK) {
        vSemaphoreDelete(u->mutex); u->mutex = NULL;
        vSemaphoreDelete(u->done);  u->done  = NULL;
        u->in_use = false;
        return err;
    }
    *ret_unit = u;
    return ESP_OK;
}

esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!handle->in_use) {
        return ESP_ERR_INVALID_STATE;
    }

    if (handle->hw_inited) {
        vsf_adc_irq_disable(handle->hw, VSF_ADC_IRQ_ALL_BITS_MASK);
        fsm_rt_t rt;
        do {
            rt = vsf_adc_disable(handle->hw);
        } while (rt == fsm_rt_on_going);
        vsf_adc_fini(handle->hw);
        handle->hw_inited = false;
    }
    if (handle->mutex != NULL) { vSemaphoreDelete(handle->mutex); handle->mutex = NULL; }
    if (handle->done  != NULL) { vSemaphoreDelete(handle->done);  handle->done  = NULL; }
    handle->in_use = false;
    handle->hw     = NULL;
    memset(handle->channels, 0, sizeof(handle->channels));
    return ESP_OK;
}

esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle,
                                     adc_channel_t channel,
                                     const adc_oneshot_chan_cfg_t *config)
{
    if ((handle == NULL) || (config == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!handle->in_use) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((uint32_t)channel >= __ADC_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    __adc_chan_ctx_t *c = &handle->channels[channel];
    c->atten      = config->atten;
    c->bitwidth   = (config->bitwidth == ADC_BITWIDTH_DEFAULT)
                        ? ADC_BITWIDTH_12
                        : config->bitwidth;
    c->configured = true;
    return ESP_OK;
}

esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle,
                           adc_channel_t channel, int *out_raw)
{
    if ((handle == NULL) || (out_raw == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!handle->in_use) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((uint32_t)channel >= __ADC_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }
    __adc_chan_ctx_t *c = &handle->channels[channel];
    if (!c->configured) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    vsf_adc_channel_cfg_t chan = { 0 };
    chan.mode          = VSF_ADC_CHANNEL_GAIN_1 | VSF_ADC_CHANNEL_REF_VDD_1;
    chan.sample_cycles = 0;
    chan.channel       = (uint8_t)channel;

    /* Drain any stale completion from a previous run. */
    (void)xSemaphoreTake(handle->done, 0);
    handle->sample   = 0;
    handle->last_irq = 0;

    vsf_err_t err = vsf_adc_channel_request_once(handle->hw, &chan, &handle->sample);
    if (err != VSF_ERR_NONE) {
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    /* Wait for ISR. If no ISR ever fires (driver ran synchronously and
     * the result is already in the buffer) give up after a generous
     * timeout so the caller still sees the data. */
    (void)xSemaphoreTake(handle->done, pdMS_TO_TICKS(100));

    /* Mask to the configured bit-width. */
    uint32_t mask = (c->bitwidth >= 32)
                       ? 0xFFFFFFFFu
                       : ((1u << c->bitwidth) - 1u);
    *out_raw = (int)(handle->sample & mask);

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_ADC */
