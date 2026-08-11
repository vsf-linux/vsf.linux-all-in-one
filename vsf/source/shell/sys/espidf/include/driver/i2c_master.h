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
 * Clean-room re-implementation of ESP-IDF "driver/i2c_master.h" (new driver,
 * v5.2+).
 *
 * Authored from ESP-IDF v5.2 public API only. No code copied from the
 * ESP-IDF source tree. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_i2c.h
 *
 * via a caller-supplied pool of vsf_i2c_t * instances (see
 *     vsf_espidf_cfg_t::i2c
 * ).
 *
 * The new ESP-IDF I2C driver uses a bus-device model:
 *   - Bus  = one I2C controller (port 0 / 1), mapped to a vsf_i2c_t*.
 *   - Device = a soft handle carrying (bus, address, speed).
 *
 * Only Master mode is implemented (matches the vast majority of ESP-IDF
 * application use cases). Slave mode can be added in a future i2c_slave.h.
 *
 * All transfer functions are synchronous / blocking, matching ESP-IDF
 * semantics. The timeout parameter is forwarded as a FreeRTOS tick limit.
 */

#ifndef __VSF_ESPIDF_DRIVER_I2C_MASTER_H__
#define __VSF_ESPIDF_DRIVER_I2C_MASTER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "driver/i2c_types.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/**
 * @brief Opaque handle for an I2C master bus.
 */
typedef struct i2c_master_bus_t *i2c_master_bus_handle_t;

/**
 * @brief Opaque handle for an I2C device attached to a master bus.
 */
typedef struct i2c_master_dev_t *i2c_master_dev_handle_t;

/**
 * @brief I2C master bus configuration.
 *
 * @note  sda_io_num / scl_io_num / glitch_ignore_cnt / intr_priority /
 *        flags.enable_internal_pullup are accepted for source compatibility
 *        but ignored: pin routing and interrupt priority are managed by the
 *        underlying VSF HAL driver.
 */
typedef struct {
    i2c_clock_source_t  clk_source;         /*!< Clock source (ignored)     */
    i2c_port_num_t      i2c_port;           /*!< Port number, or -1 = auto  */
    gpio_num_t          scl_io_num;         /*!< SCL GPIO (ignored)         */
    gpio_num_t          sda_io_num;         /*!< SDA GPIO (ignored)         */
    uint8_t             glitch_ignore_cnt;  /*!< Glitch filter (ignored)    */
    int                 intr_priority;      /*!< ISR priority (ignored)     */
    size_t              trans_queue_depth;   /*!< Async queue depth (unused) */
    struct {
        uint32_t enable_internal_pullup : 1;/*!< Pull-up (ignored)          */
    } flags;
} i2c_master_bus_config_t;

/**
 * @brief Per-device configuration attached to a master bus.
 */
typedef struct {
    i2c_addr_bit_len_t  dev_addr_length;    /*!< 7-bit or 10-bit address    */
    uint16_t            device_address;     /*!< Raw slave address (no R/W) */
    uint32_t            scl_speed_hz;       /*!< SCL frequency for device   */
    uint32_t            scl_wait_us;        /*!< SCL stretch timeout (ign.) */
    struct {
        uint32_t disable_ack_check : 1;     /*!< Disable ACK check (ign.)   */
    } flags;
} i2c_device_config_t;

/*============================ BUS LIFECYCLE =================================*/

/**
 * @brief Create a new I2C master bus.
 *
 * Allocates a bus handle from the injected pool. The underlying vsf_i2c_t
 * is NOT initialised until the first transfer (lazy init on first device
 * use), because the clock speed is a per-device property in this API.
 *
 * @param[in]  bus_config     Bus configuration.
 * @param[out] ret_bus_handle Returned bus handle on success.
 * @return ESP_OK / ESP_ERR_INVALID_ARG / ESP_ERR_NOT_FOUND /
 *         ESP_ERR_INVALID_STATE (port already in use).
 */
extern esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config,
                                     i2c_master_bus_handle_t *ret_bus_handle);

/**
 * @brief Delete the I2C master bus and release the port.
 *
 * All devices attached to this bus must be removed first via
 * i2c_master_bus_rm_device(); otherwise ESP_ERR_INVALID_STATE is returned.
 */
extern esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle);

/*============================ DEVICE LIFECYCLE ==============================*/

/**
 * @brief Attach a device to the master bus.
 *
 * The returned handle is used for all subsequent transfer calls.
 * Multiple devices with different addresses / speeds may share one bus.
 *
 * @param[in]  bus_handle  Master bus handle.
 * @param[in]  dev_config  Per-device configuration.
 * @param[out] ret_handle  Returned device handle on success.
 */
extern esp_err_t i2c_master_bus_add_device(
                        i2c_master_bus_handle_t bus_handle,
                        const i2c_device_config_t *dev_config,
                        i2c_master_dev_handle_t *ret_handle);

/**
 * @brief Remove a device from the master bus.
 */
extern esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle);

/*============================ DATA TRANSFER =================================*/

/**
 * @brief Master write (START + addr/W + data + STOP).
 *
 * @param[in] i2c_dev          Device handle.
 * @param[in] write_buffer     Data to transmit.
 * @param[in] write_size       Number of bytes to write.
 * @param[in] xfer_timeout_ms  Timeout in ms (-1 = wait forever).
 */
extern esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev,
                                      const uint8_t *write_buffer,
                                      size_t write_size,
                                      int xfer_timeout_ms);

/**
 * @brief Master read (START + addr/R + data + STOP).
 *
 * @param[in]  i2c_dev          Device handle.
 * @param[out] read_buffer      Receive buffer.
 * @param[in]  read_size        Number of bytes to read.
 * @param[in]  xfer_timeout_ms  Timeout in ms (-1 = wait forever).
 */
extern esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev,
                                     uint8_t *read_buffer,
                                     size_t read_size,
                                     int xfer_timeout_ms);

/**
 * @brief Combined write-then-read with repeated START.
 *
 * Sequence: START + addr/W + write_data + Sr + addr/R + read_data + STOP.
 *
 * @param[in]  i2c_dev          Device handle.
 * @param[in]  write_buffer     Data to transmit (register address, etc.).
 * @param[in]  write_size       Write byte count.
 * @param[out] read_buffer      Receive buffer.
 * @param[in]  read_size        Read byte count.
 * @param[in]  xfer_timeout_ms  Timeout in ms (-1 = wait forever).
 */
extern esp_err_t i2c_master_transmit_receive(
                        i2c_master_dev_handle_t i2c_dev,
                        const uint8_t *write_buffer,
                        size_t write_size,
                        uint8_t *read_buffer,
                        size_t read_size,
                        int xfer_timeout_ms);

/*============================ BUS UTILITIES =================================*/

/**
 * @brief Probe a device on the bus (write 0 bytes).
 *
 * @return ESP_OK if the device ACKed its address, ESP_ERR_NOT_FOUND if not.
 */
extern esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle,
                                   uint16_t address,
                                   int xfer_timeout_ms);

/**
 * @brief Reset the I2C bus (toggle SCL to release SDA).
 */
extern esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle);

/**
 * @brief Wait until all queued transfers are complete.
 *
 * Since all transfers in this shim are synchronous, this is always an
 * immediate success.
 */
extern esp_err_t i2c_master_bus_wait_all_done(
                        i2c_master_bus_handle_t bus_handle,
                        int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_DRIVER_I2C_MASTER_H__ */
