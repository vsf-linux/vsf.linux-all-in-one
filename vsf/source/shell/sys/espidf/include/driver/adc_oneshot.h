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
 * Clean-room re-implementation of ESP-IDF "esp_adc/adc_oneshot.h" (v5.1).
 *
 * Authored from the ESP-IDF v5.1 public API only. No code copied from
 * the ESP-IDF source tree. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_adc.h
 *
 * via a caller-supplied pool of vsf_adc_t * instances (see
 *     vsf_espidf_cfg_t::adc
 * ). Pool index equals adc_unit_t (0 -> ADC_UNIT_1, 1 -> ADC_UNIT_2).
 *
 * Implemented subset:
 *   - adc_oneshot_new_unit / adc_oneshot_del_unit
 *   - adc_oneshot_config_channel (cached; applied on each read)
 *   - adc_oneshot_read (synchronous blocking)
 *
 * Continuous (DMA) mode is not implemented; callers requiring that path
 * will receive ESP_ERR_NOT_SUPPORTED from the companion adc_continuous.h.
 */

#ifndef __VSF_ESPIDF_DRIVER_ADC_ONESHOT_H__
#define __VSF_ESPIDF_DRIVER_ADC_ONESHOT_H__

#include <stdint.h>

#include "esp_err.h"
#include "driver/adc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/** @brief Opaque handle for an ADC oneshot unit. */
typedef struct adc_oneshot_unit_ctx_t *adc_oneshot_unit_handle_t;

/** @brief Unit-level init configuration. */
typedef struct {
    adc_unit_t              unit_id;
    adc_oneshot_clk_src_t   clk_src;        /*!< Accepted but ignored.    */
    adc_ulp_mode_t          ulp_mode;       /*!< Accepted but ignored.    */
} adc_oneshot_unit_init_cfg_t;

/** @brief Per-channel configuration. */
typedef struct {
    adc_atten_t      atten;                 /*!< Accepted but ignored.    */
    adc_bitwidth_t   bitwidth;              /*!< DEFAULT falls back to 12 bits. */
} adc_oneshot_chan_cfg_t;

/*============================ PROTOTYPES ====================================*/

/**
 * @brief Allocate an ADC oneshot unit handle from the pool entry
 *        matching init_config->unit_id.
 */
extern esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config,
                                      adc_oneshot_unit_handle_t *ret_unit);

/**
 * @brief Release an ADC oneshot unit handle allocated by
 *        adc_oneshot_new_unit().
 */
extern esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);

/**
 * @brief Cache a channel configuration on the handle. The actual
 *        vsf_adc channel_request_once call happens in adc_oneshot_read().
 */
extern esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle,
                                            adc_channel_t channel,
                                            const adc_oneshot_chan_cfg_t *config);

/**
 * @brief Perform one blocking conversion on a channel and return the
 *        raw result.
 */
extern esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle,
                                  adc_channel_t channel,
                                  int *out_raw);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_DRIVER_ADC_ONESHOT_H__
