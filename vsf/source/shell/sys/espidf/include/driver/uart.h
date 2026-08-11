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
 * Clean-room re-implementation of ESP-IDF public API "driver/uart.h".
 * Authored from ESP-IDF v5.1 public API only; no ESP-IDF source was
 * copied. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_usart.h
 *
 * via a cfg-injected pool of vsf_usart_t* instances (same pattern as
 * driver/gptimer.h). uart_port_t (UART_NUM_0, 1, ...) indexes into
 * this pool.
 *
 * Supported API:
 *   uart_driver_install, uart_driver_delete, uart_is_driver_installed,
 *   uart_param_config,
 *   uart_set_baudrate/get_baudrate,
 *   uart_set_word_length/get_word_length,
 *   uart_set_stop_bits/get_stop_bits,
 *   uart_set_parity/get_parity,
 *   uart_set_hw_flow_ctrl/get_hw_flow_ctrl,
 *   uart_write_bytes, uart_write_bytes_with_break,
 *   uart_read_bytes,
 *   uart_get_buffered_data_len, uart_get_tx_buffer_free_size,
 *   uart_flush, uart_flush_input,
 *   uart_set_mode, uart_set_rx_timeout.
 *
 * NOT_SUPPORTED:
 *   uart_set_pin        (VSF does not manage external pin routing),
 *   uart_set_rts        (hardware-managed when flow-ctrl enabled),
 *   uart_set_dtr,
 *   uart_set_tx_idle_num,
 *   uart_wait_tx_done   (use uart_flush instead),
 *   uart_pattern_*      (ESP-IDF pattern detect is HW-specific).
 */

#ifndef __VSF_ESPIDF_DRIVER_UART_H__
#define __VSF_ESPIDF_DRIVER_UART_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

/* FreeRTOS types used by the driver API (QueueHandle_t, TickType_t). */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/** Symbolic value for uart_set_pin: "do not change this pin". */
#define UART_PIN_NO_CHANGE      (-1)

/** Default hardware FIFO depth (best-effort; actual depends on MCU). */
#define UART_HW_FIFO_LEN       (128)

/*============================ TYPES =========================================*/

/** UART port number (index into the cfg-injected vsf_usart_t pool). */
typedef int uart_port_t;

/** Well-known port numbers. UART_NUM_MAX equals the pool size at runtime. */
#define UART_NUM_0              (0)
#define UART_NUM_1              (1)
#define UART_NUM_2              (2)
#define UART_NUM_MAX            (3)     /*!< compile-time ceiling; real max = pool_count */

/** UART word length. */
typedef enum {
    UART_DATA_5_BITS = 0,               /*!< 5-bit data */
    UART_DATA_6_BITS = 1,               /*!< 6-bit data */
    UART_DATA_7_BITS = 2,               /*!< 7-bit data */
    UART_DATA_8_BITS = 3,               /*!< 8-bit data */
    UART_DATA_BITS_MAX = 4,
} uart_word_length_t;

/** UART stop bits. */
typedef enum {
    UART_STOP_BITS_1   = 1,             /*!< 1 stop bit */
    UART_STOP_BITS_1_5 = 2,             /*!< 1.5 stop bits */
    UART_STOP_BITS_2   = 3,             /*!< 2 stop bits */
    UART_STOP_BITS_MAX = 4,
} uart_stop_bits_t;

/** UART parity mode. */
typedef enum {
    UART_PARITY_DISABLE = 0,            /*!< No parity */
    UART_PARITY_EVEN    = 2,            /*!< Even parity */
    UART_PARITY_ODD     = 3,            /*!< Odd parity */
} uart_parity_t;

/** UART hardware flow control mode. */
typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0,       /*!< No HW flow control */
    UART_HW_FLOWCTRL_RTS     = 1,       /*!< RTS only */
    UART_HW_FLOWCTRL_CTS     = 2,       /*!< CTS only */
    UART_HW_FLOWCTRL_CTS_RTS = 3,       /*!< Both CTS and RTS */
    UART_HW_FLOWCTRL_MAX     = 4,
} uart_hw_flowcontrol_t;

/** UART communication mode. */
typedef enum {
    UART_MODE_UART           = 0,       /*!< Normal UART mode */
    UART_MODE_RS485_HALF_DUPLEX = 1,    /*!< RS-485 half duplex */
    UART_MODE_IRDA           = 2,       /*!< IrDA (not supported in VSF shim) */
    UART_MODE_RS485_COLLISION_DETECT = 3,
    UART_MODE_RS485_APP_CTRL = 4,
} uart_mode_t;

/** UART clock source (informational only; VSF manages clocking internally). */
typedef enum {
    UART_SCLK_DEFAULT = 0,
    UART_SCLK_APB     = 1,
    UART_SCLK_XTAL    = 2,
    UART_SCLK_RTC     = 3,
} uart_sclk_t;

/** UART configuration structure (passed to uart_param_config). */
typedef struct {
    int                     baud_rate;
    uart_word_length_t      data_bits;
    uart_parity_t           parity;
    uart_stop_bits_t        stop_bits;
    uart_hw_flowcontrol_t   flow_ctrl;
    uint8_t                 rx_flow_ctrl_thresh;    /*!< HW-RTS threshold (bytes) */
    uart_sclk_t             source_clk;             /*!< informational */
} uart_config_t;

/** UART event types posted to the user event queue. */
typedef enum {
    UART_DATA              = 0,         /*!< Data ready in RX buffer */
    UART_BREAK             = 1,         /*!< BREAK detected */
    UART_BUFFER_FULL       = 2,         /*!< RX ring buffer full */
    UART_FIFO_OVF          = 3,         /*!< FIFO overflow */
    UART_FRAME_ERR         = 4,         /*!< Frame error */
    UART_PARITY_ERR        = 5,         /*!< Parity error */
    UART_DATA_BREAK        = 6,         /*!< TX data + BREAK sent */
    UART_PATTERN_DET       = 7,         /*!< Pattern detected (NOT_SUPPORTED) */
    UART_EVENT_MAX         = 8,
} uart_event_type_t;

/** UART event structure posted via the event queue. */
typedef struct {
    uart_event_type_t       type;       /*!< Event type */
    size_t                  size;       /*!< Bytes available / transferred */
    bool                    timeout_flag; /*!< RX timeout occurred */
} uart_event_t;

/*============================ DRIVER LIFECYCLE ==============================*/

/**
 * Install the UART driver and allocate internal resources.
 *
 * @param uart_num       UART port number (index into the cfg pool).
 * @param rx_buffer_size RX ring buffer size in bytes. Must be > UART_HW_FIFO_LEN.
 * @param tx_buffer_size TX ring buffer size (0 = blocking write mode).
 * @param queue_size     UART event queue depth. 0 = no event queue.
 * @param uart_queue     [out] If non-NULL, receives the event queue handle.
 * @param intr_alloc_flags Ignored by VSF (interrupt allocation is internal).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG / ESP_ERR_NO_MEM / ESP_FAIL.
 */
extern esp_err_t uart_driver_install(uart_port_t uart_num,
                                     int rx_buffer_size,
                                     int tx_buffer_size,
                                     int queue_size,
                                     QueueHandle_t *uart_queue,
                                     int intr_alloc_flags);

/** Delete the UART driver and free all resources. */
extern esp_err_t uart_driver_delete(uart_port_t uart_num);

/** Check whether the UART driver is installed. */
extern bool      uart_is_driver_installed(uart_port_t uart_num);

/*============================ CONFIGURATION =================================*/

/** Apply a full configuration (baud, data bits, parity, stop, flow control). */
extern esp_err_t uart_param_config(uart_port_t uart_num,
                                   const uart_config_t *uart_config);

extern esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baudrate);
extern esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t *baudrate);

extern esp_err_t uart_set_word_length(uart_port_t uart_num,
                                      uart_word_length_t data_bit);
extern esp_err_t uart_get_word_length(uart_port_t uart_num,
                                      uart_word_length_t *data_bit);

extern esp_err_t uart_set_stop_bits(uart_port_t uart_num,
                                    uart_stop_bits_t stop_bits);
extern esp_err_t uart_get_stop_bits(uart_port_t uart_num,
                                    uart_stop_bits_t *stop_bits);

extern esp_err_t uart_set_parity(uart_port_t uart_num,
                                 uart_parity_t parity_mode);
extern esp_err_t uart_get_parity(uart_port_t uart_num,
                                 uart_parity_t *parity_mode);

extern esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num,
                                       uart_hw_flowcontrol_t flow_ctrl,
                                       uint8_t rx_thresh);
extern esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num,
                                       uart_hw_flowcontrol_t *flow_ctrl);

/*============================ I/O ===========================================*/

/**
 * Write data to the UART TX FIFO. Blocks until all bytes are queued.
 * @return Number of bytes written, or -1 on error.
 */
extern int uart_write_bytes(uart_port_t uart_num,
                            const void *src, size_t size);

/**
 * Write data followed by a BREAK signal.
 * @return Number of bytes written, or -1 on error.
 */
extern int uart_write_bytes_with_break(uart_port_t uart_num,
                                       const void *src, size_t size,
                                       int brk_len);

/**
 * Read data from the UART RX buffer.
 * @param ticks_to_wait Maximum time to block (FreeRTOS ticks).
 * @return Number of bytes read, or -1 on error.
 */
extern int uart_read_bytes(uart_port_t uart_num,
                           void *buf, uint32_t length,
                           TickType_t ticks_to_wait);

/*============================ BUFFER STATUS =================================*/

/** Get the number of bytes buffered in the RX ring buffer. */
extern esp_err_t uart_get_buffered_data_len(uart_port_t uart_num,
                                            size_t *size);

/** Get the free space in the TX ring buffer. */
extern esp_err_t uart_get_tx_buffer_free_size(uart_port_t uart_num,
                                              size_t *size);

/** Discard all data in the RX ring buffer (same as uart_flush_input). */
extern esp_err_t uart_flush(uart_port_t uart_num);

/** Discard all data in the RX ring buffer. */
extern esp_err_t uart_flush_input(uart_port_t uart_num);

/*============================ PIN ROUTING ===================================*/

/**
 * Set UART pin assignment. NOT_SUPPORTED in this VSF shim -- pin muxing
 * is the responsibility of the board BSP / io_mapper.
 *
 * @return ESP_ERR_NOT_SUPPORTED always.
 */
extern esp_err_t uart_set_pin(uart_port_t uart_num,
                              int tx_io_num, int rx_io_num,
                              int rts_io_num, int cts_io_num);

/*============================ MODE / TIMEOUT ================================*/

/** Set UART mode (normal / RS-485 / IrDA). */
extern esp_err_t uart_set_mode(uart_port_t uart_num, uart_mode_t mode);

/** Set RX timeout threshold (in UART symbol periods). */
extern esp_err_t uart_set_rx_timeout(uart_port_t uart_num, uint8_t tout);

/*============================ WAIT TX DONE ==================================*/

/**
 * Wait until TX FIFO is empty (best-effort via status polling).
 * @param ticks_to_wait  Maximum wait time in FreeRTOS ticks.
 * @return ESP_OK or ESP_ERR_TIMEOUT.
 */
extern esp_err_t uart_wait_tx_done(uart_port_t uart_num,
                                   TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif

#endif      /* __VSF_ESPIDF_DRIVER_UART_H__ */
