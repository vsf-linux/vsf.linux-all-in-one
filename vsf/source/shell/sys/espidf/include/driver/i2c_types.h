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
 * Clean-room public types shared between the new I2C master driver
 * (i2c_master.h) and future slave driver (i2c_slave.h).
 *
 * Authored from ESP-IDF v5.2 public API only.
 */

#ifndef __VSF_ESPIDF_I2C_TYPES_H__
#define __VSF_ESPIDF_I2C_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/**
 * @brief I2C port number.
 *
 * Use -1 for automatic port selection in i2c_new_master_bus().
 * Valid port indices start at 0.
 */
typedef int i2c_port_num_t;

/**
 * @brief Maximum number of I2C ports that the shim can manage.
 *
 * This only sets an upper limit for static arrays inside the port layer.
 * The actual number of usable ports is determined by the pool injected
 * via vsf_espidf_cfg_t::i2c at runtime.
 */
#ifndef I2C_NUM_MAX
#   define I2C_NUM_MAX      2
#endif

/**
 * @brief I2C device address bit length.
 */
typedef enum {
    I2C_ADDR_BIT_LEN_7  = 0,   /*!< 7-bit address */
    I2C_ADDR_BIT_LEN_10 = 1,   /*!< 10-bit address */
} i2c_addr_bit_len_t;

/**
 * @brief I2C clock source.
 *
 * Accepted for source compatibility; the VSF I2C HAL does not expose
 * per-peripheral clock muxing, so the actual clock is whatever the
 * underlying hardware driver selects.
 */
typedef int i2c_clock_source_t;
#define I2C_CLK_SRC_DEFAULT     0

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_I2C_TYPES_H__ */
