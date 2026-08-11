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
 * Port implementation for "driver/spi_master.h" on VSF.
 *
 * Bridges ESP-IDF SPI Master onto
 *     hal/driver/common/template/vsf_template_spi.h
 *
 * Data flow (blocking full-duplex, one transaction at a time):
 *
 *   1. Caller holds the bus mutex (automatic inside each xx_transmit,
 *      explicit via spi_device_acquire_bus for batch ops).
 *   2. If the device's (mode, clock_speed_hz, flags) differ from the
 *      bus's last configuration, the underlying vsf_spi_t is re-inited
 *      with a fresh vsf_spi_cfg_t.
 *   3. The port composes an internal tx scratch buffer if the device
 *      declares command_bits / address_bits / dummy_bits; data is then
 *      appended for the write phase.
 *   4. vsf_spi_cs_active() is called (hardware CS) or skipped
 *      (software CS -- caller drives CS externally).
 *   5. vsf_spi_request_transfer() kicks the transfer; the port blocks on
 *      a binary semaphore that the ISR callback fires on CPL/error.
 *   6. vsf_spi_cs_inactive() deasserts hardware CS unless
 *      SPI_TRANS_CS_KEEP_ACTIVE was set.
 *   7. pre_cb / post_cb, if registered, are invoked inline around the
 *      transfer (emulating ESP-IDF semantics -- same thread, not ISR).
 *
 * Resource injection: same pool pattern as gptimer / uart / i2c.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_SPI_MASTER == ENABLED

#include "driver/spi_master.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "hal/driver/driver.h"

#if !defined(VSF_USE_HEAP) || VSF_USE_HEAP != ENABLED
#   error "VSF_ESPIDF_CFG_USE_DRIVER_SPI_MASTER requires VSF_USE_HEAP"
#endif
#include "service/heap/vsf_heap.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

/*============================ MACROS ========================================*/

/* Max devices attachable to one host. Applications rarely need more. */
#define __SPI_MAX_DEVS_PER_HOST     8

/* Scratch buffer size for command / address / dummy prefix. A SPI flash
 * style transaction never needs more than 16 B of prefix. */
#define __SPI_PREFIX_SCRATCH_MAX    16

/* IRQ masks we wait on. CPL covers both TX and RX done. */
#define __SPI_IRQ_ALL   (VSF_SPI_IRQ_MASK_CPL | VSF_SPI_IRQ_MASK_ERR)
#define __SPI_IRQ_ERR   (VSF_SPI_IRQ_MASK_ERR)

/*============================ TYPES =========================================*/

typedef struct spi_host_state_t spi_host_state_t;

typedef struct spi_device_t {
    spi_host_state_t       *host;
    spi_device_interface_config_t cfg;  /* full copy                    */
    uint8_t                 cs_index;   /* hardware CS index, 0 default */
    bool                    in_use;

    /* Simple single-slot completion queue for queue_trans/get_trans_result.
     * ESP-IDF allows N queued; this port executes inline then stashes the
     * descriptor pointer so get_trans_result returns it. A deeper queue
     * can be added later without breaking callers that use transmit(). */
    spi_transaction_t      *done_desc;
} spi_device_t;

struct spi_host_state_t {
    vsf_spi_t              *hw;
    bool                    in_use;
    bool                    hw_inited;

    /* Last-applied configuration; re-init is skipped when a device's
     * requested settings already match. */
    uint8_t                 cur_mode;        /* 0..3                    */
    uint32_t                cur_clock_hz;
    uint32_t                cur_flags;       /* cached SPI_DEVICE_* set */

    SemaphoreHandle_t       bus_mutex;
    SemaphoreHandle_t       xfer_done;
    vsf_spi_irq_mask_t      irq_result;

    /* spi_device_acquire_bus: when non-NULL, bus_mutex is held on the
     * caller's behalf and released by spi_device_release_bus. */
    spi_device_handle_t     acquired_by;

    /* spi_device_polling_start/_end single-slot tracker. */
    spi_device_handle_t     polling_dev;

    spi_device_t           *devs[__SPI_MAX_DEVS_PER_HOST];
    uint8_t                 dev_count;
};

/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                    is_inited;
    vsf_spi_t *const       *pool;
    uint16_t                pool_count;
    spi_host_state_t       *hosts;  /* heap array, [pool_count]         */
} __vsf_espidf_spi = { 0 };

/*============================ PROTOTYPES ====================================*/

extern void vsf_espidf_spi_init(const vsf_espidf_spi_cfg_t *cfg);

/*============================ ISR ===========================================*/

static void __spi_isr_handler(void *target_ptr,
                               vsf_spi_t *spi_ptr,
                               vsf_spi_irq_mask_t irq_mask)
{
    spi_host_state_t *host = (spi_host_state_t *)target_ptr;
    host->irq_result = irq_mask;

    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(host->xfer_done, &woken);
    portYIELD_FROM_ISR(woken);
}

/*============================ HELPERS =======================================*/

static inline bool __spi_host_valid(spi_host_device_t host_id)
{
    return __vsf_espidf_spi.is_inited
        && ((int)host_id >= 0)
        && ((int)host_id < (int)__vsf_espidf_spi.pool_count)
        && (__vsf_espidf_spi.pool[host_id] != NULL);
}

static inline spi_host_state_t * __spi_host_get(spi_host_device_t host_id)
{
    return &__vsf_espidf_spi.hosts[host_id];
}

static inline bool __spi_dev_valid(spi_device_handle_t h)
{
    return (h != NULL) && h->in_use && (h->host != NULL);
}

static inline TickType_t __ticks_or_max(uint32_t ticks_to_wait)
{
    /* ESP-IDF conventions: portMAX_DELAY for infinite wait is passed
     * literally; any finite value is taken as-is. */
    return (TickType_t)ticks_to_wait;
}

/* Translate ESP-IDF (mode, flags) into a vsf_spi_mode_t. */
static vsf_spi_mode_t __build_spi_mode(const spi_device_interface_config_t *cfg)
{
    vsf_spi_mode_t m = (vsf_spi_mode_t)0;

    m = (vsf_spi_mode_t)(m | VSF_SPI_MASTER);

    /* Bit order: default MSB; LSB iff flag requests it (we use one flag
     * for both tx/rx ordering on the bus level). */
    if ((cfg->flags & SPI_DEVICE_BIT_LSBFIRST) == SPI_DEVICE_BIT_LSBFIRST) {
        m = (vsf_spi_mode_t)(m | VSF_SPI_LSB_FIRST);
    } else {
        m = (vsf_spi_mode_t)(m | VSF_SPI_MSB_FIRST);
    }

    switch (cfg->mode & 0x3) {
    case 0: m = (vsf_spi_mode_t)(m | VSF_SPI_MODE_0); break;
    case 1: m = (vsf_spi_mode_t)(m | VSF_SPI_MODE_1); break;
    case 2: m = (vsf_spi_mode_t)(m | VSF_SPI_MODE_2); break;
    case 3: m = (vsf_spi_mode_t)(m | VSF_SPI_MODE_3); break;
    }

    /* CS management: hardware if the device supplies a CS pin index
     * (>=0), else software (caller must drive CS out of band). */
    if (cfg->spics_io_num >= 0) {
        m = (vsf_spi_mode_t)(m | VSF_SPI_CS_HARDWARE_MODE);
    } else {
        m = (vsf_spi_mode_t)(m | VSF_SPI_CS_SOFTWARE_MODE);
    }

    /* Data size: byte-stream transfers are always 8-bit at the HAL. */
    m = (vsf_spi_mode_t)(m | VSF_SPI_DATASIZE_8);

    return m;
}

/* (Re-)configure the underlying vsf_spi_t to match the given device cfg.
 * Skipped when the bus is already at the requested (mode, clock, flags). */
static esp_err_t __spi_ensure_cfg(spi_host_state_t *host,
                                   const spi_device_interface_config_t *dcfg)
{
    uint32_t effective_flags = dcfg->flags & (SPI_DEVICE_BIT_LSBFIRST);

    if (host->hw_inited
        && (host->cur_mode     == dcfg->mode)
        && (host->cur_clock_hz == (uint32_t)dcfg->clock_speed_hz)
        && (host->cur_flags    == effective_flags)) {
        return ESP_OK;
    }

    vsf_spi_cfg_t cfg = {
        .mode       = __build_spi_mode(dcfg),
        .clock_hz   = (uint32_t)dcfg->clock_speed_hz,
        .isr        = {
            .handler_fn = __spi_isr_handler,
            .target_ptr = host,
            .prio       = vsf_arch_prio_0,
        },
        .auto_cs_index = 0,
    };

    if (host->hw_inited) {
        vsf_spi_irq_disable(host->hw, __SPI_IRQ_ALL);
        vsf_spi_disable(host->hw);
    }

    if (vsf_spi_init(host->hw, &cfg) != VSF_ERR_NONE) {
        return ESP_FAIL;
    }
    vsf_spi_enable(host->hw);
    vsf_spi_irq_enable(host->hw, __SPI_IRQ_ALL);

    host->hw_inited    = true;
    host->cur_mode     = dcfg->mode;
    host->cur_clock_hz = (uint32_t)dcfg->clock_speed_hz;
    host->cur_flags    = effective_flags;
    return ESP_OK;
}

/* Pack cmd (MSB-first right-aligned, command_bits wide) into bytes. */
static size_t __pack_prefix(uint8_t *dst, size_t cap,
                             uint64_t value, uint8_t bits)
{
    if (bits == 0) {
        return 0;
    }
    /* This port requires byte-aligned prefixes; validated in caller. */
    size_t n = (size_t)(bits / 8U);
    if (n > cap) {
        n = cap;
    }
    for (size_t i = 0; i < n; i++) {
        dst[i] = (uint8_t)((value >> ((n - 1 - i) * 8U)) & 0xFFU);
    }
    return n;
}

static inline const void * __trans_tx_ptr(const spi_transaction_t *t)
{
    if (t->flags & SPI_TRANS_USE_TXDATA) {
        return t->tx_data;
    }
    return t->tx_buffer;
}

static inline void * __trans_rx_ptr(spi_transaction_t *t)
{
    if (t->flags & SPI_TRANS_USE_RXDATA) {
        return t->rx_data;
    }
    return t->rx_buffer;
}

/* Perform the blocking transfer. Caller MUST hold host->bus_mutex. */
static esp_err_t __spi_do_transfer(spi_host_state_t *host,
                                    spi_device_t *dev,
                                    spi_transaction_t *t)
{
    esp_err_t err = __spi_ensure_cfg(host, &dev->cfg);
    if (err != ESP_OK) {
        return err;
    }

    /* Determine phase widths -- honour VARIABLE_* only when caller also
     * supplies a spi_transaction_ext_t. */
    uint8_t cmd_bits = dev->cfg.command_bits;
    uint8_t addr_bits = dev->cfg.address_bits;
    uint8_t dummy_bits = dev->cfg.dummy_bits;

    if (t->flags & (SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR
                  | SPI_TRANS_VARIABLE_DUMMY)) {
        const spi_transaction_ext_t *ext = (const spi_transaction_ext_t *)t;
        if (t->flags & SPI_TRANS_VARIABLE_CMD)   { cmd_bits   = ext->command_bits; }
        if (t->flags & SPI_TRANS_VARIABLE_ADDR)  { addr_bits  = ext->address_bits; }
        if (t->flags & SPI_TRANS_VARIABLE_DUMMY) { dummy_bits = ext->dummy_bits;   }
    }

    if (((cmd_bits   & 0x7) != 0)
     || ((addr_bits  & 0x7) != 0)
     || ((dummy_bits & 0x7) != 0)
     || ((t->length  & 0x7) != 0)
     || ((t->rxlength & 0x7) != 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t cmd_n   = cmd_bits   / 8U;
    size_t addr_n  = addr_bits  / 8U;
    size_t dummy_n = dummy_bits / 8U;
    size_t data_n  = t->length  / 8U;
    size_t rx_n    = (t->rxlength ? t->rxlength : t->length) / 8U;

    if ((cmd_n + addr_n + dummy_n) > __SPI_PREFIX_SCRATCH_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    /* pre_cb runs before CS / transfer. ESP-IDF calls this in ISR ctx;
     * in this port we run it in the caller's context. */
    if (dev->cfg.pre_cb != NULL) {
        dev->cfg.pre_cb(t);
    }

    /* Assert hardware CS. Software CS is the caller's responsibility. */
    bool hw_cs = (dev->cfg.spics_io_num >= 0);
    if (hw_cs) {
        vsf_spi_cs_active(host->hw, dev->cs_index);
    }

    esp_err_t ret = ESP_OK;

    /* Phase 1: prefix (cmd + addr + dummy) -- write-only. */
    if ((cmd_n + addr_n + dummy_n) > 0) {
        uint8_t scratch[__SPI_PREFIX_SCRATCH_MAX];
        size_t  off = 0;
        off += __pack_prefix(scratch + off, sizeof(scratch) - off,
                              t->cmd,  (uint8_t)(cmd_n * 8U));
        off += __pack_prefix(scratch + off, sizeof(scratch) - off,
                              t->addr, (uint8_t)(addr_n * 8U));
        /* Dummy bytes: clocked as zero. */
        for (size_t i = 0; i < dummy_n && off < sizeof(scratch); i++, off++) {
            scratch[off] = 0x00;
        }

        xSemaphoreTake(host->xfer_done, 0);
        host->irq_result = (vsf_spi_irq_mask_t)0;

        /* Full-duplex request: RX is clocked in but discarded here. */
        uint8_t rx_throwaway[__SPI_PREFIX_SCRATCH_MAX];
        vsf_err_t ve = vsf_spi_request_transfer(host->hw,
                                                  scratch, rx_throwaway,
                                                  (uint_fast32_t)off);
        if (ve != VSF_ERR_NONE) {
            ret = ESP_FAIL;
            goto L_finish;
        }
        if (xSemaphoreTake(host->xfer_done, portMAX_DELAY) != pdTRUE) {
            ret = ESP_ERR_TIMEOUT;
            goto L_finish;
        }
        if (host->irq_result & __SPI_IRQ_ERR) {
            ret = ESP_FAIL;
            goto L_finish;
        }
    }

    /* Phase 2: data. Four sub-cases:
     *   (a) no data at all                    -> skip.
     *   (b) full-duplex: data_n bytes of TX and RX simultaneously.
     *   (c) half-duplex read:  SPI_DEVICE_HALFDUPLEX set AND rx_buffer
     *                          is provided and data_n == 0 (rxlength
     *                          carries the read size).
     *   (d) half-duplex write: SPI_DEVICE_HALFDUPLEX set AND data_n > 0.
     */
    const void *tx_ptr = __trans_tx_ptr(t);
    void       *rx_ptr = __trans_rx_ptr(t);

    bool halfduplex = (dev->cfg.flags & SPI_DEVICE_HALFDUPLEX) != 0;
    size_t run_bytes = 0;

    if (halfduplex) {
        if (data_n > 0) {
            /* half-duplex write */
            run_bytes = data_n;
            rx_ptr = NULL;  /* RX not valid in write-only half-duplex. */
        } else if (rx_n > 0) {
            /* half-duplex read */
            run_bytes = rx_n;
            tx_ptr = NULL;
        }
    } else {
        /* full-duplex: length carries the transfer size. rxlength is
         * tolerated but not larger than length. */
        run_bytes = data_n;
    }

    if (run_bytes > 0) {
        xSemaphoreTake(host->xfer_done, 0);
        host->irq_result = (vsf_spi_irq_mask_t)0;

        vsf_err_t ve = vsf_spi_request_transfer(host->hw,
                                                  (void *)tx_ptr, rx_ptr,
                                                  (uint_fast32_t)run_bytes);
        if (ve != VSF_ERR_NONE) {
            ret = ESP_FAIL;
            goto L_finish;
        }
        if (xSemaphoreTake(host->xfer_done, portMAX_DELAY) != pdTRUE) {
            ret = ESP_ERR_TIMEOUT;
            goto L_finish;
        }
        if (host->irq_result & __SPI_IRQ_ERR) {
            ret = ESP_FAIL;
            goto L_finish;
        }
    }

L_finish:
    if (hw_cs && !(t->flags & SPI_TRANS_CS_KEEP_ACTIVE)) {
        vsf_spi_cs_inactive(host->hw, dev->cs_index);
    }

    if (dev->cfg.post_cb != NULL) {
        dev->cfg.post_cb(t);
    }

    return ret;
}

/*============================ BUS LIFECYCLE =================================*/

esp_err_t spi_bus_initialize(spi_host_device_t host_id,
                              const spi_bus_config_t *bus_config,
                              spi_common_dma_t dma_chan)
{
    (void)bus_config;   /* pin fields accepted for source compat, ignored */
    (void)dma_chan;

    if (!__vsf_espidf_spi.is_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!__spi_host_valid(host_id)) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_host_state_t *host = __spi_host_get(host_id);
    if (host->in_use) {
        return ESP_ERR_INVALID_STATE;
    }

    host->hw          = __vsf_espidf_spi.pool[host_id];
    host->hw_inited   = false;
    host->cur_mode    = 0xFF;   /* force re-init on first device use   */
    host->cur_clock_hz = 0;
    host->cur_flags   = 0;
    host->dev_count   = 0;
    host->acquired_by = NULL;
    host->polling_dev = NULL;
    host->irq_result  = (vsf_spi_irq_mask_t)0;
    memset(host->devs, 0, sizeof(host->devs));

    host->bus_mutex = xSemaphoreCreateMutex();
    host->xfer_done = xSemaphoreCreateBinary();
    if ((host->bus_mutex == NULL) || (host->xfer_done == NULL)) {
        if (host->bus_mutex != NULL) { vSemaphoreDelete(host->bus_mutex); }
        if (host->xfer_done != NULL) { vSemaphoreDelete(host->xfer_done); }
        host->bus_mutex = NULL;
        host->xfer_done = NULL;
        return ESP_ERR_NO_MEM;
    }

    host->in_use = true;
    return ESP_OK;
}

esp_err_t spi_bus_free(spi_host_device_t host_id)
{
    if (!__vsf_espidf_spi.is_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!__spi_host_valid(host_id)) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_host_state_t *host = __spi_host_get(host_id);
    if (!host->in_use) {
        return ESP_ERR_INVALID_STATE;
    }
    if (host->dev_count > 0) {
        return ESP_ERR_INVALID_STATE;
    }

    if (host->hw_inited) {
        vsf_spi_irq_disable(host->hw, __SPI_IRQ_ALL);
        vsf_spi_disable(host->hw);
        vsf_spi_fini(host->hw);
    }

    vSemaphoreDelete(host->bus_mutex);
    vSemaphoreDelete(host->xfer_done);
    host->bus_mutex = NULL;
    host->xfer_done = NULL;

    host->in_use    = false;
    host->hw_inited = false;
    return ESP_OK;
}

/*============================ DEVICE LIFECYCLE ==============================*/

esp_err_t spi_bus_add_device(spi_host_device_t host_id,
                              const spi_device_interface_config_t *dev_config,
                              spi_device_handle_t *handle)
{
    if ((dev_config == NULL) || (handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!__spi_host_valid(host_id)) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_host_state_t *host = __spi_host_get(host_id);
    if (!host->in_use) {
        return ESP_ERR_INVALID_STATE;
    }
    if (host->dev_count >= __SPI_MAX_DEVS_PER_HOST) {
        return ESP_ERR_NO_MEM;
    }
    if ((dev_config->mode & ~0x3) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (dev_config->clock_speed_hz <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *handle = NULL;

    spi_device_t *dev = (spi_device_t *)vsf_heap_malloc(sizeof(spi_device_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(dev, 0, sizeof(*dev));

    dev->host = host;
    dev->cfg  = *dev_config;
    /* Use the device slot index as hardware CS index. Callers who need a
     * specific auto_cs_index can remap via the BSP's CS routing table. */
    dev->cs_index = host->dev_count;
    dev->in_use   = true;

    host->devs[host->dev_count++] = dev;
    *handle = dev;
    return ESP_OK;
}

esp_err_t spi_bus_remove_device(spi_device_handle_t handle)
{
    if (!__spi_dev_valid(handle)) {
        return ESP_ERR_INVALID_ARG;
    }
    spi_host_state_t *host = handle->host;

    /* Best-effort removal: find and collapse. */
    for (uint8_t i = 0; i < host->dev_count; i++) {
        if (host->devs[i] == handle) {
            for (uint8_t j = i; j + 1 < host->dev_count; j++) {
                host->devs[j] = host->devs[j + 1];
            }
            host->devs[--host->dev_count] = NULL;
            break;
        }
    }

    handle->in_use = false;
    vsf_heap_free(handle);
    return ESP_OK;
}

/*============================ DATA TRANSFER =================================*/

static esp_err_t __spi_do_with_lock(spi_device_handle_t handle,
                                     spi_transaction_t *t,
                                     uint32_t ticks_to_wait)
{
    if (!__spi_dev_valid(handle) || (t == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    spi_host_state_t *host = handle->host;

    bool lock_held = (host->acquired_by == handle);
    if (!lock_held) {
        if (xSemaphoreTake(host->bus_mutex,
                           __ticks_or_max(ticks_to_wait)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }

    esp_err_t ret = __spi_do_transfer(host, handle, t);

    if (!lock_held) {
        xSemaphoreGive(host->bus_mutex);
    }
    return ret;
}

esp_err_t spi_device_transmit(spi_device_handle_t handle,
                               spi_transaction_t *trans_desc)
{
    return __spi_do_with_lock(handle, trans_desc, portMAX_DELAY);
}

esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,
                                       spi_transaction_t *trans_desc)
{
    return __spi_do_with_lock(handle, trans_desc, portMAX_DELAY);
}

esp_err_t spi_device_queue_trans(spi_device_handle_t handle,
                                   spi_transaction_t *trans_desc,
                                   uint32_t ticks_to_wait)
{
    if (!__spi_dev_valid(handle) || (trans_desc == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->done_desc != NULL) {
        /* Caller hasn't retrieved the previous result yet. */
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = __spi_do_with_lock(handle, trans_desc, ticks_to_wait);
    if (ret == ESP_OK) {
        handle->done_desc = trans_desc;
    }
    return ret;
}

esp_err_t spi_device_get_trans_result(spi_device_handle_t handle,
                                        spi_transaction_t **trans_desc,
                                        uint32_t ticks_to_wait)
{
    (void)ticks_to_wait;
    if (!__spi_dev_valid(handle) || (trans_desc == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->done_desc == NULL) {
        return ESP_ERR_TIMEOUT;
    }
    *trans_desc = handle->done_desc;
    handle->done_desc = NULL;
    return ESP_OK;
}

esp_err_t spi_device_polling_start(spi_device_handle_t handle,
                                    spi_transaction_t *trans_desc,
                                    uint32_t ticks_to_wait)
{
    if (!__spi_dev_valid(handle) || (trans_desc == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    spi_host_state_t *host = handle->host;

    if (host->polling_dev != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    bool lock_held = (host->acquired_by == handle);
    if (!lock_held) {
        if (xSemaphoreTake(host->bus_mutex,
                           __ticks_or_max(ticks_to_wait)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }

    /* Execute inline -- the HAL already serialises the HW transfer via
     * the ISR callback we wait on. polling_end is a no-op wait. */
    esp_err_t ret = __spi_do_transfer(host, handle, trans_desc);

    host->polling_dev = handle;
    /* Note: bus_mutex is released in polling_end to honour ESP-IDF
     * semantics that the bus is held across start/end. */
    (void)lock_held;
    return ret;
}

esp_err_t spi_device_polling_end(spi_device_handle_t handle,
                                   uint32_t ticks_to_wait)
{
    (void)ticks_to_wait;
    if (!__spi_dev_valid(handle)) {
        return ESP_ERR_INVALID_ARG;
    }
    spi_host_state_t *host = handle->host;
    if (host->polling_dev != handle) {
        return ESP_ERR_INVALID_STATE;
    }

    host->polling_dev = NULL;
    if (host->acquired_by != handle) {
        xSemaphoreGive(host->bus_mutex);
    }
    return ESP_OK;
}

/*============================ BUS LOCKING ===================================*/

esp_err_t spi_device_acquire_bus(spi_device_handle_t device,
                                   uint32_t wait)
{
    if (!__spi_dev_valid(device)) {
        return ESP_ERR_INVALID_ARG;
    }
    spi_host_state_t *host = device->host;

    if (xSemaphoreTake(host->bus_mutex, __ticks_or_max(wait)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    host->acquired_by = device;
    return ESP_OK;
}

void spi_device_release_bus(spi_device_handle_t dev)
{
    if (!__spi_dev_valid(dev)) {
        return;
    }
    spi_host_state_t *host = dev->host;
    if (host->acquired_by != dev) {
        return;
    }
    host->acquired_by = NULL;
    xSemaphoreGive(host->bus_mutex);
}

/*============================ UTILITIES =====================================*/

esp_err_t spi_device_get_actual_freq(spi_device_handle_t handle, int *freq_khz)
{
    if (!__spi_dev_valid(handle) || (freq_khz == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    /* The HAL does not expose a read-back divisor; report the configured
     * speed. This matches the typical "close enough" contract callers
     * observe on real ESP32 silicon. */
    *freq_khz = handle->cfg.clock_speed_hz / 1000;
    return ESP_OK;
}

/*============================ INIT ==========================================*/

void vsf_espidf_spi_init(const vsf_espidf_spi_cfg_t *cfg)
{
    if (__vsf_espidf_spi.is_inited) {
        return;
    }
    if ((cfg == NULL) || (cfg->pool == NULL) || (cfg->pool_count == 0)) {
        __vsf_espidf_spi.pool       = NULL;
        __vsf_espidf_spi.pool_count = 0;
        __vsf_espidf_spi.hosts      = NULL;
        __vsf_espidf_spi.is_inited  = true;
        return;
    }

    uint16_t cnt = cfg->pool_count;
    __vsf_espidf_spi.pool       = cfg->pool;
    __vsf_espidf_spi.pool_count = cnt;

    size_t sz = (size_t)cnt * sizeof(spi_host_state_t);
    spi_host_state_t *h = (spi_host_state_t *)vsf_heap_malloc(sz);
    VSF_ASSERT(h != NULL);
    memset(h, 0, sz);

    for (uint16_t i = 0; i < cnt; i++) {
        h[i].hw = cfg->pool[i];
    }

    __vsf_espidf_spi.hosts     = h;
    __vsf_espidf_spi.is_inited = true;
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_SPI_MASTER */
