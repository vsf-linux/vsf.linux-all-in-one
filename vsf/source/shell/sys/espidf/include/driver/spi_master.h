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
 * Clean-room re-implementation of ESP-IDF "driver/spi_master.h".
 *
 * Authored from ESP-IDF v5.1 public API only. No code copied from the
 * ESP-IDF source tree. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_spi.h
 *
 * via a caller-supplied pool of vsf_spi_t * instances (see
 *     vsf_espidf_cfg_t::spi
 * ).
 *
 * For source-compat this single header consolidates what ESP-IDF scatters
 * across driver/spi_master.h, driver/spi_common.h and hal/spi_types.h
 * -- callers that only #include "driver/spi_master.h" find everything
 * they need here.
 *
 * Implementation scope (initial drop):
 *   - Bus lifecycle: spi_bus_initialize / spi_bus_free
 *   - Device lifecycle: spi_bus_add_device / spi_bus_remove_device
 *   - Synchronous transfers: spi_device_transmit,
 *     spi_device_polling_transmit, spi_device_queue_trans /
 *     spi_device_get_trans_result (executed inline, non-FIFO).
 *   - Polling start/end: spi_device_polling_start / spi_device_polling_end
 *   - Bus locking: spi_device_acquire_bus / spi_device_release_bus
 *
 * Not implemented (returns ESP_ERR_NOT_SUPPORTED or best-effort ignore):
 *   - Dual / Quad / Octal line modes; only standard 1-line mode.
 *   - Variable-size cmd / addr / dummy phases; command_bits and
 *     address_bits must be a multiple of 8.
 *   - Half-duplex "write N then read M" as distinct phases -- emulated
 *     by one full-duplex transfer of max(N, M) bytes with appropriate
 *     buffer split. SPI_DEVICE_HALFDUPLEX sets read phase only.
 *
 * GPIO / pin-routing fields in spi_bus_config_t are accepted for source
 * compatibility but ignored: the underlying vsf_spi_t is already wired
 * by the BSP.
 */

#ifndef __VSF_ESPIDF_DRIVER_SPI_MASTER_H__
#define __VSF_ESPIDF_DRIVER_SPI_MASTER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/* Common SPI clock frequencies (Hz). Retained for source compatibility;
 * the VSF HAL only uses clock_speed_hz numerically. */
#define SPI_MASTER_FREQ_8M      (8  * 1000 * 1000)
#define SPI_MASTER_FREQ_10M     (10 * 1000 * 1000)
#define SPI_MASTER_FREQ_16M     (16 * 1000 * 1000)
#define SPI_MASTER_FREQ_20M     (20 * 1000 * 1000)
#define SPI_MASTER_FREQ_26M     (80 * 1000 * 1000 / 3)
#define SPI_MASTER_FREQ_40M     (40 * 1000 * 1000)
#define SPI_MASTER_FREQ_80M     (80 * 1000 * 1000)

/* Per-device flag bits (spi_device_interface_config_t::flags). */
#define SPI_DEVICE_TXBIT_LSBFIRST   (1U << 0)
#define SPI_DEVICE_RXBIT_LSBFIRST   (1U << 1)
#define SPI_DEVICE_BIT_LSBFIRST     (SPI_DEVICE_TXBIT_LSBFIRST | SPI_DEVICE_RXBIT_LSBFIRST)
#define SPI_DEVICE_3WIRE            (1U << 2)
#define SPI_DEVICE_POSITIVE_CS      (1U << 3)
#define SPI_DEVICE_HALFDUPLEX       (1U << 4)
#define SPI_DEVICE_CLK_AS_CS        (1U << 5)
#define SPI_DEVICE_NO_DUMMY         (1U << 6)
#define SPI_DEVICE_DDRCLK           (1U << 7)
#define SPI_DEVICE_NO_RETURN_RESULT (1U << 8)

/* Per-transaction flag bits (spi_transaction_t::flags). */
#define SPI_TRANS_MODE_DIO          (1U << 0)
#define SPI_TRANS_MODE_QIO          (1U << 1)
#define SPI_TRANS_USE_RXDATA        (1U << 2)
#define SPI_TRANS_USE_TXDATA        (1U << 3)
#define SPI_TRANS_MODE_DIOQIO_ADDR  (1U << 4)
#define SPI_TRANS_VARIABLE_CMD      (1U << 5)
#define SPI_TRANS_VARIABLE_ADDR     (1U << 6)
#define SPI_TRANS_VARIABLE_DUMMY    (1U << 7)
#define SPI_TRANS_CS_KEEP_ACTIVE    (1U << 8)
#define SPI_TRANS_MULTILINE_CMD     (1U << 9)
#define SPI_TRANS_MODE_OCT          (1U << 10)
#define SPI_TRANS_MULTILINE_ADDR    SPI_TRANS_MODE_DIOQIO_ADDR

/* Bus config flag bits (spi_bus_config_t::flags). Accepted and ignored. */
#define SPICOMMON_BUSFLAG_SLAVE     (1U << 0)
#define SPICOMMON_BUSFLAG_MASTER    (1U << 1)
#define SPICOMMON_BUSFLAG_IOMUX_PINS (1U << 2)
#define SPICOMMON_BUSFLAG_GPIO_PINS  (1U << 3)
#define SPICOMMON_BUSFLAG_SCLK      (1U << 4)
#define SPICOMMON_BUSFLAG_MISO      (1U << 5)
#define SPICOMMON_BUSFLAG_MOSI      (1U << 6)
#define SPICOMMON_BUSFLAG_DUAL      (1U << 7)
#define SPICOMMON_BUSFLAG_WPHD      (1U << 8)
#define SPICOMMON_BUSFLAG_QUAD      (SPICOMMON_BUSFLAG_DUAL | SPICOMMON_BUSFLAG_WPHD)
#define SPICOMMON_BUSFLAG_IO4_IO7   (1U << 9)
#define SPICOMMON_BUSFLAG_OCTAL     (SPICOMMON_BUSFLAG_QUAD | SPICOMMON_BUSFLAG_IO4_IO7)
#define SPICOMMON_BUSFLAG_NATIVE_PINS SPICOMMON_BUSFLAG_IOMUX_PINS

/* DMA channel selection (accepted; DMA is managed internally by VSF). */
typedef enum {
    SPI_DMA_DISABLED = 0,
    SPI_DMA_CH1      = 1,
    SPI_DMA_CH2      = 2,
    SPI_DMA_CH_AUTO  = 3,
} spi_common_dma_t;

#define SPI_DMA_CH_AUTO SPI_DMA_CH_AUTO

/*============================ TYPES =========================================*/

/**
 * @brief SPI host controller enumeration.
 *
 * Values map directly into the user-supplied pool index
 * (vsf_espidf_cfg_t::spi.pool[spi_host_device_t]).
 */
typedef enum {
    SPI1_HOST = 0,
    SPI2_HOST = 1,
    SPI3_HOST = 2,
    SPI_HOST_MAX,
} spi_host_device_t;

/* Clock source selector (accepted, ignored). */
typedef int spi_clock_source_t;
#define SPI_CLK_SRC_DEFAULT     0

/* Sample-point tuning (accepted, ignored). */
typedef int spi_sampling_point_t;
#define SPI_SAMPLING_POINT_PHASE_0  0

/**
 * @brief Bus-wide configuration.
 *
 * The pin numbers below are accepted for source compatibility but not
 * acted upon: the underlying vsf_spi_t instance is already routed by
 * the board's BSP. Set unused pins to -1.
 */
typedef struct {
    union { int mosi_io_num;   int data0_io_num; };
    union { int miso_io_num;   int data1_io_num; };
    int sclk_io_num;
    union { int quadwp_io_num; int data2_io_num; };
    union { int quadhd_io_num; int data3_io_num; };
    int data4_io_num;
    int data5_io_num;
    int data6_io_num;
    int data7_io_num;
    int max_transfer_sz;    /*!< Maximum bytes per transfer (info only). */
    uint32_t flags;         /*!< SPICOMMON_BUSFLAG_* (accepted, ignored). */
    int intr_flags;         /*!< ISR flags (accepted, ignored).           */
} spi_bus_config_t;

/* Forward declaration -- opaque device handle. */
typedef struct spi_device_t *spi_device_handle_t;

/* Forward declaration used by callback typedef below. */
struct spi_transaction_t;
typedef void (*transaction_cb_t)(struct spi_transaction_t *trans);

/**
 * @brief Per-device configuration.
 */
typedef struct {
    uint8_t             command_bits;       /*!< 0-16, must be multiple of 8 in this port. */
    uint8_t             address_bits;       /*!< 0-64, must be multiple of 8 in this port. */
    uint8_t             dummy_bits;         /*!< Dummy cycles, must be multiple of 8.      */
    uint8_t             mode;               /*!< SPI mode 0..3 (CPOL/CPHA).                */
    spi_clock_source_t  clock_source;       /*!< Clock source selector (ignored).          */
    uint16_t            duty_cycle_pos;     /*!< Duty cycle (ignored).                     */
    uint16_t            cs_ena_pretrans;    /*!< CS pre-transfer cycles (ignored).         */
    uint8_t             cs_ena_posttrans;   /*!< CS post-transfer cycles (ignored).        */
    int                 clock_speed_hz;     /*!< SPI clock frequency, Hz.                  */
    int                 input_delay_ns;     /*!< Slave input delay (ignored).              */
    spi_sampling_point_t sample_point;      /*!< Sample-point tuning (ignored).            */
    int                 spics_io_num;       /*!< CS GPIO -- shim: CS index, or -1.         */
    uint32_t            flags;              /*!< SPI_DEVICE_* bits.                        */
    int                 queue_size;         /*!< Trans queue depth (1..N).                 */
    transaction_cb_t    pre_cb;             /*!< Pre-transfer callback (ISR context).      */
    transaction_cb_t    post_cb;            /*!< Post-transfer callback (ISR context).     */
} spi_device_interface_config_t;

/**
 * @brief One SPI transaction descriptor.
 *
 * The descriptor must remain valid until the transaction completes
 * (spi_device_transmit returns, or spi_device_get_trans_result hands
 * the pointer back to the caller).
 */
typedef struct spi_transaction_t {
    uint32_t    flags;      /*!< SPI_TRANS_* bits.                         */
    uint16_t    cmd;        /*!< Command data, right-aligned.              */
    uint64_t    addr;       /*!< Address data, right-aligned.              */
    size_t      length;     /*!< Total transfer length, in bits.           */
    size_t      rxlength;   /*!< Receive length in bits (0 == length).     */
    void       *user;       /*!< Opaque user pointer, passed to cbs.       */
    union {
        const void *tx_buffer;
        uint8_t     tx_data[4];
    };
    union {
        void       *rx_buffer;
        uint8_t     rx_data[4];
    };
} spi_transaction_t;

/**
 * @brief Extended transaction with per-transfer bit widths.
 *
 * Only meaningful when SPI_TRANS_VARIABLE_CMD / _ADDR / _DUMMY are set
 * in base.flags.
 */
typedef struct {
    spi_transaction_t   base;
    uint8_t             command_bits;
    uint8_t             address_bits;
    uint8_t             dummy_bits;
} spi_transaction_ext_t;

/*============================ BUS LIFECYCLE =================================*/

/**
 * @brief Initialise an SPI bus.
 *
 * @param[in] host_id    SPI host (index into the injected vsf_spi_t pool).
 * @param[in] bus_config Bus configuration (pin fields ignored).
 * @param[in] dma_chan   DMA channel hint (accepted, ignored).
 *
 * @return ESP_OK / ESP_ERR_INVALID_ARG / ESP_ERR_NOT_FOUND /
 *         ESP_ERR_INVALID_STATE (already initialised).
 */
extern esp_err_t spi_bus_initialize(spi_host_device_t host_id,
                                     const spi_bus_config_t *bus_config,
                                     spi_common_dma_t dma_chan);

/**
 * @brief Release an SPI bus.
 *
 * All devices attached to the host must be removed first.
 */
extern esp_err_t spi_bus_free(spi_host_device_t host_id);

/*============================ DEVICE LIFECYCLE ==============================*/

/**
 * @brief Attach a device to an initialised bus.
 */
extern esp_err_t spi_bus_add_device(spi_host_device_t host_id,
                                     const spi_device_interface_config_t *dev_config,
                                     spi_device_handle_t *handle);

/**
 * @brief Detach a device from its bus.
 */
extern esp_err_t spi_bus_remove_device(spi_device_handle_t handle);

/*============================ DATA TRANSFER =================================*/

/**
 * @brief Queue a transaction for later completion.
 *
 * This port executes queued transactions inline (synchronously). The
 * ticks_to_wait parameter bounds the bus-lock wait; once the bus is
 * held the transfer runs to completion and a matching
 * spi_device_get_trans_result() call will return the same pointer
 * immediately.
 */
extern esp_err_t spi_device_queue_trans(spi_device_handle_t handle,
                                         spi_transaction_t *trans_desc,
                                         uint32_t ticks_to_wait);

/**
 * @brief Retrieve a previously queued transaction.
 */
extern esp_err_t spi_device_get_trans_result(spi_device_handle_t handle,
                                              spi_transaction_t **trans_desc,
                                              uint32_t ticks_to_wait);

/**
 * @brief Blocking, interrupt-backed transaction.
 */
extern esp_err_t spi_device_transmit(spi_device_handle_t handle,
                                      spi_transaction_t *trans_desc);

/**
 * @brief Blocking polling transaction (shim: same as spi_device_transmit).
 */
extern esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,
                                              spi_transaction_t *trans_desc);

/**
 * @brief Start a polling transaction without blocking.
 *
 * Must be matched by exactly one spi_device_polling_end() call.
 */
extern esp_err_t spi_device_polling_start(spi_device_handle_t handle,
                                           spi_transaction_t *trans_desc,
                                           uint32_t ticks_to_wait);

/**
 * @brief Wait for a previously started polling transaction.
 */
extern esp_err_t spi_device_polling_end(spi_device_handle_t handle,
                                         uint32_t ticks_to_wait);

/*============================ BUS LOCKING ===================================*/

/**
 * @brief Acquire the bus for a series of back-to-back transactions.
 *
 * While the bus is acquired by one device, other devices sharing the
 * same host will block inside spi_device_transmit() / queue_trans.
 */
extern esp_err_t spi_device_acquire_bus(spi_device_handle_t device,
                                         uint32_t wait);

/**
 * @brief Release a previously acquired bus.
 */
extern void spi_device_release_bus(spi_device_handle_t dev);

/*============================ UTILITIES =====================================*/

/**
 * @brief Return the actual clock frequency used for a device.
 *
 * This port returns dev->clock_speed_hz rounded through the HAL's
 * granularity (typically the configured value).
 */
extern esp_err_t spi_device_get_actual_freq(spi_device_handle_t handle,
                                             int *freq_khz);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_DRIVER_SPI_MASTER_H__ */
