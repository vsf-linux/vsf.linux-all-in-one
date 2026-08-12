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
 * Clean-room re-implementation of ESP-IDF common ADC enumerations.
 * Authored from ESP-IDF v5.1 public headers only.
 */

#ifndef __VSF_ESPIDF_DRIVER_ADC_TYPES_H__
#define __VSF_ESPIDF_DRIVER_ADC_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ADC peripheral unit. */
typedef enum {
    ADC_UNIT_1 = 0,
    ADC_UNIT_2 = 1,
} adc_unit_t;

/** @brief ADC channel identifiers. Range is chip-dependent; channels
 *  beyond the target device's capability are rejected by the driver.  */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
} adc_channel_t;

/** @brief Effective resolution of a conversion result. Most targets
 *  support 12 or 13 bits; DEFAULT yields the chip's preferred value. */
typedef enum {
    ADC_BITWIDTH_DEFAULT = 0,
    ADC_BITWIDTH_9  = 9,
    ADC_BITWIDTH_10 = 10,
    ADC_BITWIDTH_11 = 11,
    ADC_BITWIDTH_12 = 12,
    ADC_BITWIDTH_13 = 13,
} adc_bitwidth_t;

/** @brief Input attenuation. Accepted but interpreted only as a hint
 *  for maximum measurable voltage -- this shim does not adjust gain. */
typedef enum {
    ADC_ATTEN_DB_0   = 0,
    ADC_ATTEN_DB_2_5 = 1,
    ADC_ATTEN_DB_6   = 2,
    ADC_ATTEN_DB_11  = 3,    /*!< Deprecated in v5.2, alias of DB_12.  */
    ADC_ATTEN_DB_12  = 3,
} adc_atten_t;

/** @brief ADC clock source. Accepted but ignored. */
typedef enum {
    ADC_DIGI_CLK_SRC_DEFAULT = 0,
} adc_oneshot_clk_src_t;

typedef adc_oneshot_clk_src_t adc_continuous_clk_src_t;

/** @brief ULP mode (not modelled by this shim). */
typedef enum {
    ADC_ULP_MODE_DISABLE = 0,
    ADC_ULP_MODE_FSM,
    ADC_ULP_MODE_RISCV,
} adc_ulp_mode_t;

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_DRIVER_ADC_TYPES_H__
