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
 * Port implementation for "driver/i2c_master.h" on VSF.
 *
 * Bridges the ESP-IDF new I2C master driver (v5.2+ bus-device model) onto
 *     hal/driver/common/template/vsf_template_i2c.h
 *
 * Data flow (all transfers are synchronous / blocking):
 *
 *   1. Caller takes the bus mutex.
 *   2. If the device's scl_speed_hz differs from the bus's current speed,
 *      reconfigure the underlying vsf_i2c_t (init + enable + irq_enable).
 *   3. Issue vsf_i2c_master_request() with the appropriate cmd flags.
 *   4. Block on a binary semaphore until the ISR callback fires.
 *   5. Inspect irq_result for NACK / arbitration-loss errors.
 *   6. Release the bus mutex.
 *
 * Resource injection: same pool pattern as gptimer / uart.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_I2C == ENABLED

#include "driver/i2c_master.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "hal/driver/driver.h"

#if !defined(VSF_USE_HEAP) || VSF_USE_HEAP != ENABLED
#   error "VSF_ESPIDF_CFG_USE_DRIVER_I2C requires VSF_USE_HEAP"
#endif
#include "service/heap/vsf_heap.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

/*============================ MACROS ========================================*/

/* Maximum number of devices that can be attached to one bus. */
#define __I2C_MAX_DEVS_PER_BUS      16

/* Convenience: all IRQ masks we care about for master mode. */
#define __I2C_MASTER_IRQ_ALL    (                                           \
        VSF_I2C_IRQ_MASK_MASTER_TRANSFER_COMPLETE                           \
      | VSF_I2C_IRQ_MASK_MASTER_ADDRESS_NACK                                \
      | VSF_I2C_IRQ_MASK_MASTER_ARBITRATION_LOST                            \
      | VSF_I2C_IRQ_MASK_MASTER_TX_NACK_DETECT                              \
    )

/* Error bits within the IRQ mask. */
#define __I2C_MASTER_IRQ_ERR    (                                           \
        VSF_I2C_IRQ_MASK_MASTER_ADDRESS_NACK                                \
      | VSF_I2C_IRQ_MASK_MASTER_ARBITRATION_LOST                            \
      | VSF_I2C_IRQ_MASK_MASTER_TX_NACK_DETECT                              \
    )

/*============================ TYPES =========================================*/

struct i2c_master_bus_t {
    vsf_i2c_t              *hw;
    i2c_port_num_t          port_num;
    bool                    in_use;
    bool                    hw_inited;          /*!< vsf_i2c_init() called? */
    uint32_t                cur_speed_hz;       /*!< last configured speed  */

    SemaphoreHandle_t       bus_mutex;          /*!< serialise bus access   */
    SemaphoreHandle_t       xfer_done;          /*!< ISR gives on complete  */
    vsf_i2c_irq_mask_t     irq_result;         /*!< mask from last ISR     */

    uint8_t                 dev_count;          /*!< attached device count  */
};

struct i2c_master_dev_t {
    i2c_master_bus_t       *bus;
    uint16_t                address;
    i2c_addr_bit_len_t      addr_bit_len;
    uint32_t                scl_speed_hz;
    bool                    disable_ack_check;
};

/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                            is_inited;
    vsf_i2c_t *const               *pool;
    uint16_t                        pool_count;
    i2c_master_bus_t               *buses;      /*!< heap array [pool_count]*/
} __vsf_espidf_i2c = { 0 };

/*============================ PROTOTYPES ====================================*/

extern void vsf_espidf_i2c_init(const vsf_espidf_i2c_cfg_t *cfg);

/*============================ ISR HANDLER ===================================*/

static void __i2c_isr_handler(void *target_ptr,
                               vsf_i2c_t *i2c_ptr,
                               vsf_i2c_irq_mask_t irq_mask)
{
    i2c_master_bus_t *bus = (i2c_master_bus_t *)target_ptr;
    bus->irq_result = irq_mask;

    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(bus->xfer_done, &woken);
    portYIELD_FROM_ISR(woken);
}

/*============================ HELPERS =======================================*/

static inline bool __i2c_bus_valid(i2c_master_bus_handle_t h)
{
    return (h != NULL) && h->in_use && (h->hw != NULL);
}

/**
 * (Re-)configure the underlying vsf_i2c_t for the given clock speed.
 * Skipped when the bus is already at the requested speed.
 */
static esp_err_t __i2c_ensure_speed(i2c_master_bus_t *bus, uint32_t speed_hz)
{
    if (bus->hw_inited && (bus->cur_speed_hz == speed_hz)) {
        return ESP_OK;
    }

    vsf_i2c_mode_t mode_bits;
    if (speed_hz <= 100000) {
        mode_bits = VSF_I2C_MODE_MASTER | VSF_I2C_SPEED_STANDARD_MODE;
    } else if (speed_hz <= 400000) {
        mode_bits = VSF_I2C_MODE_MASTER | VSF_I2C_SPEED_FAST_MODE;
    } else if (speed_hz <= 1000000) {
        mode_bits = VSF_I2C_MODE_MASTER | VSF_I2C_SPEED_FAST_MODE_PLUS;
    } else {
        mode_bits = VSF_I2C_MODE_MASTER | VSF_I2C_SPEED_HIGH_SPEED_MODE;
    }
    mode_bits = (vsf_i2c_mode_t)(mode_bits | VSF_I2C_ADDR_7_BITS);

    vsf_i2c_cfg_t cfg = {
        .mode       = mode_bits,
        .clock_hz   = speed_hz,
        .isr        = {
            .handler_fn = __i2c_isr_handler,
            .target_ptr = bus,
            .prio       = vsf_arch_prio_0,
        },
    };

    if (bus->hw_inited) {
        vsf_i2c_irq_disable(bus->hw, __I2C_MASTER_IRQ_ALL);
        vsf_i2c_disable(bus->hw);
    }

    if (vsf_i2c_init(bus->hw, &cfg) != VSF_ERR_NONE) {
        return ESP_FAIL;
    }
    vsf_i2c_enable(bus->hw);
    vsf_i2c_irq_enable(bus->hw, __I2C_MASTER_IRQ_ALL);

    bus->hw_inited    = true;
    bus->cur_speed_hz = speed_hz;
    return ESP_OK;
}

/**
 * Convert ms timeout to FreeRTOS ticks.  -1 → portMAX_DELAY.
 */
static inline TickType_t __ms_to_ticks(int timeout_ms)
{
    if (timeout_ms < 0) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS((uint32_t)timeout_ms);
}

/**
 * Build vsf_i2c_cmd_t from a device and operation flags.
 */
static inline vsf_i2c_cmd_t __build_cmd(const i2c_master_dev_t *dev,
                                          bool is_read,
                                          bool start,
                                          bool stop,
                                          bool restart)
{
    vsf_i2c_cmd_t cmd = (vsf_i2c_cmd_t)0;

    if (is_read)    { cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_READ);  }
    else            { cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_WRITE); }

    if (start)      { cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_START);   }
    if (stop)       { cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_STOP);    }
    if (restart)    { cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_RESTART); }

    if (dev->addr_bit_len == I2C_ADDR_BIT_LEN_10) {
        cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_10_BITS);
    } else {
        cmd = (vsf_i2c_cmd_t)(cmd | VSF_I2C_CMD_7_BITS);
    }
    return cmd;
}

/**
 * Low-level single-segment transfer (blocking).
 * Caller MUST hold bus_mutex.
 */
static esp_err_t __i2c_do_transfer(i2c_master_bus_t *bus,
                                    uint16_t address,
                                    vsf_i2c_cmd_t cmd,
                                    uint16_t count,
                                    uint8_t *buffer,
                                    TickType_t ticks)
{
    /* Drain stale semaphore signals. */
    xSemaphoreTake(bus->xfer_done, 0);

    bus->irq_result = (vsf_i2c_irq_mask_t)0;

    vsf_err_t err = vsf_i2c_master_request(bus->hw, address, cmd,
                                            count, buffer);
    if (err != VSF_ERR_NONE) {
        return ESP_FAIL;
    }

    /* Block until ISR fires or timeout. */
    if (xSemaphoreTake(bus->xfer_done, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    /* Check for errors. */
    if (bus->irq_result & VSF_I2C_IRQ_MASK_MASTER_ADDRESS_NACK) {
        return ESP_ERR_NOT_FOUND;
    }
    if (bus->irq_result & __I2C_MASTER_IRQ_ERR) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*============================ BUS LIFECYCLE =================================*/

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config,
                              i2c_master_bus_handle_t *ret_bus_handle)
{
    if ((bus_config == NULL) || (ret_bus_handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!__vsf_espidf_i2c.is_inited || (__vsf_espidf_i2c.pool == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }

    *ret_bus_handle = NULL;

    int port = bus_config->i2c_port;
    if (port < 0) {
        /* Auto-select: first unused port with a valid HW instance. */
        port = -1;
        for (int i = 0; i < (int)__vsf_espidf_i2c.pool_count; i++) {
            if ((__vsf_espidf_i2c.pool[i] != NULL)
                && !__vsf_espidf_i2c.buses[i].in_use) {
                port = i;
                break;
            }
        }
        if (port < 0) {
            return ESP_ERR_NOT_FOUND;
        }
    }

    if ((port < 0) || (port >= (int)__vsf_espidf_i2c.pool_count)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (__vsf_espidf_i2c.pool[port] == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_t *bus = &__vsf_espidf_i2c.buses[port];
    if (bus->in_use) {
        return ESP_ERR_INVALID_STATE;
    }

    bus->hw           = __vsf_espidf_i2c.pool[port];
    bus->port_num     = (i2c_port_num_t)port;
    bus->cur_speed_hz = 0;
    bus->hw_inited    = false;
    bus->dev_count    = 0;
    bus->irq_result   = (vsf_i2c_irq_mask_t)0;

    bus->bus_mutex = xSemaphoreCreateMutex();
    bus->xfer_done = xSemaphoreCreateBinary();
    if ((bus->bus_mutex == NULL) || (bus->xfer_done == NULL)) {
        if (bus->bus_mutex != NULL)  { vSemaphoreDelete(bus->bus_mutex); }
        if (bus->xfer_done != NULL)  { vSemaphoreDelete(bus->xfer_done); }
        return ESP_ERR_NO_MEM;
    }

    bus->in_use = true;
    *ret_bus_handle = bus;
    return ESP_OK;
}

esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle)
{
    if (!__i2c_bus_valid(bus_handle)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (bus_handle->dev_count > 0) {
        return ESP_ERR_INVALID_STATE;
    }

    if (bus_handle->hw_inited) {
        vsf_i2c_irq_disable(bus_handle->hw, __I2C_MASTER_IRQ_ALL);
        vsf_i2c_disable(bus_handle->hw);
        vsf_i2c_fini(bus_handle->hw);
    }

    vSemaphoreDelete(bus_handle->bus_mutex);
    vSemaphoreDelete(bus_handle->xfer_done);
    bus_handle->bus_mutex  = NULL;
    bus_handle->xfer_done  = NULL;

    bus_handle->in_use     = false;
    bus_handle->hw_inited  = false;
    return ESP_OK;
}

/*============================ DEVICE LIFECYCLE ==============================*/

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle,
                                     const i2c_device_config_t *dev_config,
                                     i2c_master_dev_handle_t *ret_handle)
{
    if (!__i2c_bus_valid(bus_handle)
        || (dev_config == NULL)
        || (ret_handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (bus_handle->dev_count >= __I2C_MAX_DEVS_PER_BUS) {
        return ESP_ERR_NO_MEM;
    }

    *ret_handle = NULL;

    i2c_master_dev_t *dev = (i2c_master_dev_t *)vsf_heap_malloc(
                                sizeof(i2c_master_dev_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(dev, 0, sizeof(*dev));

    dev->bus              = bus_handle;
    dev->address          = dev_config->device_address;
    dev->addr_bit_len     = dev_config->dev_addr_length;
    dev->scl_speed_hz     = dev_config->scl_speed_hz;
    dev->disable_ack_check = (dev_config->flags.disable_ack_check != 0);

    bus_handle->dev_count++;
    *ret_handle = dev;
    return ESP_OK;
}

esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle)
{
    if ((handle == NULL) || (handle->bus == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->bus->dev_count > 0) {
        handle->bus->dev_count--;
    }
    vsf_heap_free(handle);
    return ESP_OK;
}

/*============================ DATA TRANSFER =================================*/

esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev,
                               const uint8_t *write_buffer,
                               size_t write_size,
                               int xfer_timeout_ms)
{
    if ((i2c_dev == NULL) || (write_buffer == NULL) || (write_size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_t *bus = i2c_dev->bus;
    TickType_t ticks = __ms_to_ticks(xfer_timeout_ms);

    if (xSemaphoreTake(bus->bus_mutex, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = __i2c_ensure_speed(bus, i2c_dev->scl_speed_hz);
    if (ret != ESP_OK) {
        xSemaphoreGive(bus->bus_mutex);
        return ret;
    }

    vsf_i2c_cmd_t cmd = __build_cmd(i2c_dev,
                                     /*read*/ false,
                                     /*start*/ true,
                                     /*stop*/ true,
                                     /*restart*/ false);

    ret = __i2c_do_transfer(bus, i2c_dev->address, cmd,
                             (uint16_t)write_size,
                             (uint8_t *)write_buffer, ticks);

    xSemaphoreGive(bus->bus_mutex);
    return ret;
}

esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev,
                              uint8_t *read_buffer,
                              size_t read_size,
                              int xfer_timeout_ms)
{
    if ((i2c_dev == NULL) || (read_buffer == NULL) || (read_size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_t *bus = i2c_dev->bus;
    TickType_t ticks = __ms_to_ticks(xfer_timeout_ms);

    if (xSemaphoreTake(bus->bus_mutex, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = __i2c_ensure_speed(bus, i2c_dev->scl_speed_hz);
    if (ret != ESP_OK) {
        xSemaphoreGive(bus->bus_mutex);
        return ret;
    }

    vsf_i2c_cmd_t cmd = __build_cmd(i2c_dev,
                                     /*read*/ true,
                                     /*start*/ true,
                                     /*stop*/ true,
                                     /*restart*/ false);

    ret = __i2c_do_transfer(bus, i2c_dev->address, cmd,
                             (uint16_t)read_size, read_buffer, ticks);

    xSemaphoreGive(bus->bus_mutex);
    return ret;
}

esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev,
                                       const uint8_t *write_buffer,
                                       size_t write_size,
                                       uint8_t *read_buffer,
                                       size_t read_size,
                                       int xfer_timeout_ms)
{
    if ((i2c_dev == NULL)
        || (write_buffer == NULL) || (write_size == 0)
        || (read_buffer == NULL) || (read_size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_t *bus = i2c_dev->bus;
    TickType_t ticks = __ms_to_ticks(xfer_timeout_ms);

    if (xSemaphoreTake(bus->bus_mutex, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = __i2c_ensure_speed(bus, i2c_dev->scl_speed_hz);
    if (ret != ESP_OK) {
        xSemaphoreGive(bus->bus_mutex);
        return ret;
    }

    /* Phase 1: START + addr/W + write_data (no STOP) */
    vsf_i2c_cmd_t cmd_w = __build_cmd(i2c_dev,
                                        /*read*/ false,
                                        /*start*/ true,
                                        /*stop*/ false,
                                        /*restart*/ false);

    ret = __i2c_do_transfer(bus, i2c_dev->address, cmd_w,
                             (uint16_t)write_size,
                             (uint8_t *)write_buffer, ticks);
    if (ret != ESP_OK) {
        xSemaphoreGive(bus->bus_mutex);
        return ret;
    }

    /* Phase 2: RESTART + addr/R + read_data + STOP */
    vsf_i2c_cmd_t cmd_r = __build_cmd(i2c_dev,
                                        /*read*/ true,
                                        /*start*/ false,
                                        /*stop*/ true,
                                        /*restart*/ true);

    ret = __i2c_do_transfer(bus, i2c_dev->address, cmd_r,
                             (uint16_t)read_size, read_buffer, ticks);

    xSemaphoreGive(bus->bus_mutex);
    return ret;
}

/*============================ BUS UTILITIES =================================*/

esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle,
                            uint16_t address,
                            int xfer_timeout_ms)
{
    if (!__i2c_bus_valid(bus_handle)) {
        return ESP_ERR_INVALID_ARG;
    }

    TickType_t ticks = __ms_to_ticks(xfer_timeout_ms);

    if (xSemaphoreTake(bus_handle->bus_mutex, ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    /* Use a reasonable default speed for probing. */
    esp_err_t ret = __i2c_ensure_speed(bus_handle, 100000);
    if (ret != ESP_OK) {
        xSemaphoreGive(bus_handle->bus_mutex);
        return ret;
    }

    /* Write 0 bytes: START + addr/W + STOP.
     * If the device ACKs its address, we get TRANSFER_COMPLETE.
     * If no device, we get ADDRESS_NACK. */
    vsf_i2c_cmd_t cmd = (vsf_i2c_cmd_t)(
            VSF_I2C_CMD_WRITE | VSF_I2C_CMD_START
          | VSF_I2C_CMD_STOP  | VSF_I2C_CMD_7_BITS);

    ret = __i2c_do_transfer(bus_handle, address, cmd,
                             0, NULL, ticks);

    xSemaphoreGive(bus_handle->bus_mutex);
    return ret;
}

esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle)
{
    if (!__i2c_bus_valid(bus_handle)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Re-init the I2C peripheral to clear any stuck state. */
    if (bus_handle->hw_inited) {
        vsf_i2c_irq_disable(bus_handle->hw, __I2C_MASTER_IRQ_ALL);
        vsf_i2c_disable(bus_handle->hw);
        vsf_i2c_fini(bus_handle->hw);
        bus_handle->hw_inited    = false;
        bus_handle->cur_speed_hz = 0;
    }
    return ESP_OK;
}

esp_err_t i2c_master_bus_wait_all_done(i2c_master_bus_handle_t bus_handle,
                                        int timeout_ms)
{
    /* All transfers in this shim are synchronous, so there is nothing to
     * wait for. */
    (void)timeout_ms;
    if (!__i2c_bus_valid(bus_handle)) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

/*============================ INIT ==========================================*/

void vsf_espidf_i2c_init(const vsf_espidf_i2c_cfg_t *cfg)
{
    if (__vsf_espidf_i2c.is_inited) {
        return;
    }
    if ((cfg == NULL) || (cfg->pool == NULL) || (cfg->pool_count == 0)) {
        __vsf_espidf_i2c.pool       = NULL;
        __vsf_espidf_i2c.pool_count = 0;
        __vsf_espidf_i2c.buses      = NULL;
        __vsf_espidf_i2c.is_inited  = true;
        return;
    }

    uint16_t cnt = cfg->pool_count;
    __vsf_espidf_i2c.pool       = cfg->pool;
    __vsf_espidf_i2c.pool_count = cnt;

    size_t sz = (size_t)cnt * sizeof(i2c_master_bus_t);
    i2c_master_bus_t *b = (i2c_master_bus_t *)vsf_heap_malloc(sz);
    VSF_ASSERT(b != NULL);
    memset(b, 0, sz);

    for (uint16_t i = 0; i < cnt; i++) {
        b[i].hw       = cfg->pool[i];
        b[i].port_num = (i2c_port_num_t)i;
    }

    __vsf_espidf_i2c.buses     = b;
    __vsf_espidf_i2c.is_inited = true;
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_I2C */
