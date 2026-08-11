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
 * Port implementation for "driver/uart.h" on VSF.
 *
 * Uses the same ring-buffer data flow as
 *     hal/utilities/stream/usart/vsf_usart_stream:
 *
 *   RX:  VSF_USART_IRQ_MASK_RX  -> rxfifo_read -> vsf_mem_stream_t
 *   TX:  vsf_mem_stream_t -> vsf_stream_get_rbuf -> request_tx
 *        -> TX_CPL -> vsf_stream_read (consume) -> pump next chunk
 *
 * This matches the ESP-IDF UART model where the ISR continuously drains
 * the HW Rx FIFO into a software ring buffer, and uart_read_bytes reads
 * from that buffer.  The ISR logic mirrors vsf_usart_stream.c but is
 * inlined here for full control over reconfiguration and error events.
 *
 * Resource injection: same pool pattern as gptimer.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_UART == ENABLED

#include "driver/uart.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "hal/driver/driver.h"

#if !defined(VSF_USE_HEAP) || VSF_USE_HEAP != ENABLED
#   error "VSF_ESPIDF_CFG_USE_DRIVER_UART requires VSF_USE_HEAP"
#endif
#include "service/heap/vsf_heap.h"

#if !defined(VSF_USE_SIMPLE_STREAM) || VSF_USE_SIMPLE_STREAM != ENABLED
#   error "VSF_ESPIDF_CFG_USE_DRIVER_UART requires VSF_USE_SIMPLE_STREAM"
#endif
#define __VSF_SIMPLE_STREAM_CLASS_INHERIT__
#include "service/simple_stream/vsf_simple_stream.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include <string.h>

/*============================ MACROS ========================================*/

/* Tx buffer size when tx_buffer_size == 0 (blocking write mode). */
#define __UART_DEFAULT_TX_BUF_SIZE      256

/*============================ TYPES =========================================*/

typedef struct uart_port_state_t {
    vsf_usart_t            *hw;

    /* cached configuration (valid even before driver_install) */
    uint32_t                baudrate;
    uart_word_length_t      data_bits;
    uart_parity_t           parity;
    uart_stop_bits_t        stop_bits;
    uart_hw_flowcontrol_t   flow_ctrl;
    uint8_t                 rx_flow_ctrl_thresh;
    uart_mode_t             mode;
    uint8_t                 rx_timeout;
    bool                    config_valid;

    /* runtime (valid only when installed == true) */
    bool                    installed;
    bool                    tx_blocking;     /*!< tx_buffer_size was 0 */

    vsf_mem_stream_t       *stream_rx;       /*!< Rx ring buffer (heap) */
    vsf_mem_stream_t       *stream_tx;       /*!< Tx ring buffer (heap) */
    uint32_t                tx_pending;      /*!< bytes in current HW TX */

    SemaphoreHandle_t       rx_sem;          /*!< ISR gives on Rx data  */
    SemaphoreHandle_t       tx_sem;          /*!< ISR gives on Tx space */
    SemaphoreHandle_t       tx_mutex;        /*!< serialise write_bytes */
    SemaphoreHandle_t       rx_mutex;        /*!< serialise read_bytes  */
    QueueHandle_t           event_queue;     /*!< user event queue      */
} uart_port_state_t;

/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                            is_inited;
    vsf_usart_t *const             *pool;
    uint16_t                        pool_count;
    uart_port_state_t              *ports;     /*!< heap array [pool_count] */
} __vsf_espidf_uart = { 0 };

/*============================ PROTOTYPES ====================================*/

extern void vsf_espidf_uart_init(const vsf_espidf_uart_cfg_t *cfg);

/*============================ HELPERS =======================================*/

static inline bool __uart_port_valid(uart_port_t port)
{
    return __vsf_espidf_uart.is_inited
        && (port >= 0) && (port < (int)__vsf_espidf_uart.pool_count);
}

static inline uart_port_state_t * __uart_state(uart_port_t port)
{
    return &__vsf_espidf_uart.ports[port];
}

#define __stream_rx(__s)    ((vsf_stream_t *)(__s)->stream_rx)
#define __stream_tx(__s)    ((vsf_stream_t *)(__s)->stream_tx)

/*--------------------------- mode translation ------------------------------*/

static vsf_usart_mode_t __uart_translate_mode(const uart_port_state_t *s)
{
    vsf_usart_mode_t m = (vsf_usart_mode_t)0;

    switch (s->data_bits) {
    case UART_DATA_5_BITS:  m |= VSF_USART_5_BIT_LENGTH; break;
    case UART_DATA_6_BITS:  m |= VSF_USART_6_BIT_LENGTH; break;
    case UART_DATA_7_BITS:  m |= VSF_USART_7_BIT_LENGTH; break;
    case UART_DATA_8_BITS:
    default:                m |= VSF_USART_8_BIT_LENGTH;  break;
    }

    switch (s->parity) {
    case UART_PARITY_EVEN:      m |= VSF_USART_EVEN_PARITY; break;
    case UART_PARITY_ODD:       m |= VSF_USART_ODD_PARITY;  break;
    case UART_PARITY_DISABLE:
    default:                    m |= VSF_USART_NO_PARITY;    break;
    }

    switch (s->stop_bits) {
    case UART_STOP_BITS_1_5:    m |= VSF_USART_1_5_STOPBIT; break;
    case UART_STOP_BITS_2:      m |= VSF_USART_2_STOPBIT;   break;
    case UART_STOP_BITS_1:
    default:                    m |= VSF_USART_1_STOPBIT;    break;
    }

    switch (s->flow_ctrl) {
    case UART_HW_FLOWCTRL_RTS:      m |= VSF_USART_RTS_HWCONTROL;     break;
    case UART_HW_FLOWCTRL_CTS:      m |= VSF_USART_CTS_HWCONTROL;     break;
    case UART_HW_FLOWCTRL_CTS_RTS:  m |= VSF_USART_RTS_CTS_HWCONTROL; break;
    case UART_HW_FLOWCTRL_DISABLE:
    default:                         m |= VSF_USART_NO_HWCONTROL;      break;
    }

    m |= VSF_USART_TX_ENABLE | VSF_USART_RX_ENABLE;

    if (s->mode == UART_MODE_RS485_HALF_DUPLEX) {
        m |= VSF_USART_HALF_DUPLEX_ENABLE;
    }
    return m;
}

/*--------------------------- TX pump ---------------------------------------*/

static void __uart_tx_start(uart_port_state_t *s)
{
    vsf_protect_t orig = vsf_protect_int();
    if (s->tx_pending > 0) {
        vsf_unprotect_int(orig);
        return;
    }
    uint8_t *buf;
    uint32_t size = vsf_stream_get_rbuf(__stream_tx(s), &buf);
    if (size > 0) {
        s->tx_pending = size;
        vsf_unprotect_int(orig);
        vsf_usart_request_tx(s->hw, buf, size);
    } else {
        vsf_unprotect_int(orig);
    }
}

/* stream_tx rx-terminal evthandler (task context): kick TX pump when app
 * writes data into the Tx stream. */
static void __uart_stream_tx_evthandler(vsf_stream_t *stream,
                                         void *param,
                                         vsf_stream_evt_t evt)
{
    uart_port_state_t *s = (uart_port_state_t *)param;
    switch (evt) {
    case VSF_STREAM_ON_CONNECT:
    case VSF_STREAM_ON_IN:
        __uart_tx_start(s);
        break;
    default:
        break;
    }
}

/*--------------------------- ISR handler -----------------------------------*/

static void __uart_isr_handler(void *target_ptr,
                                vsf_usart_t *usart_ptr,
                                vsf_usart_irq_mask_t irq_mask)
{
    uart_port_state_t *s = (uart_port_state_t *)target_ptr;
    BaseType_t woken = pdFALSE;

    (void)usart_ptr;

    /* ---- RX: drain HW FIFO into stream_rx (same as vsf_usart_stream) ---- */
    if ((irq_mask & VSF_USART_IRQ_MASK_RX) && (s->stream_rx != NULL)) {
        bool got_data = false;
        while (true) {
            uint8_t *buffer;
            uint32_t total = 0;
            uint32_t bufsize = vsf_stream_get_wbuf(__stream_rx(s), &buffer);

            while (bufsize > 0) {
                uint32_t cnt = vsf_usart_rxfifo_get_data_count(s->hw);
                if (cnt == 0) {
                    break;
                }
                cnt = vsf_min(bufsize, cnt);
                uint32_t cur = vsf_usart_rxfifo_read(s->hw, buffer, cnt);
                total  += cur;
                buffer += cur;
                bufsize -= cur;
            }
            if (total == 0) {
                break;
            }
            vsf_stream_write(__stream_rx(s), NULL, total);
            got_data = true;
        }
        if (got_data) {
            if (s->rx_sem != NULL) {
                xSemaphoreGiveFromISR(s->rx_sem, &woken);
            }
            if (s->event_queue != NULL) {
                uart_event_t ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = UART_DATA;
                ev.size = (size_t)vsf_stream_get_data_size(__stream_rx(s));
                xQueueSendFromISR(s->event_queue, &ev, &woken);
            }
        }
        /* stream full and FIFO still has data -> BUFFER_FULL */
        if (s->event_queue != NULL
            && vsf_stream_get_free_size(__stream_rx(s)) == 0
            && vsf_usart_rxfifo_get_data_count(s->hw) > 0) {
            uart_event_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = UART_BUFFER_FULL;
            xQueueSendFromISR(s->event_queue, &ev, &woken);
        }
    }

    /* ---- TX_CPL: consume sent data, pump next chunk ---- */
    if ((irq_mask & VSF_USART_IRQ_MASK_TX_CPL) && (s->stream_tx != NULL)) {
        if (s->tx_pending > 0) {
            vsf_stream_read(__stream_tx(s), NULL, s->tx_pending);
            s->tx_pending = 0;

            uint8_t *buf;
            uint32_t size = vsf_stream_get_rbuf(__stream_tx(s), &buf);
            if (size > 0) {
                s->tx_pending = size;
                vsf_usart_request_tx(s->hw, buf, size);
            }
        }
        if (s->tx_sem != NULL) {
            xSemaphoreGiveFromISR(s->tx_sem, &woken);
        }
    }

    /* ---- error events ---- */
    if (s->event_queue != NULL) {
        uart_event_t ev;
        memset(&ev, 0, sizeof(ev));

        if (irq_mask & VSF_USART_IRQ_MASK_FRAME_ERR) {
            ev.type = UART_FRAME_ERR;
            xQueueSendFromISR(s->event_queue, &ev, &woken);
        }
        if (irq_mask & VSF_USART_IRQ_MASK_PARITY_ERR) {
            ev.type = UART_PARITY_ERR;
            xQueueSendFromISR(s->event_queue, &ev, &woken);
        }
        if (irq_mask & VSF_USART_IRQ_MASK_RX_OVERFLOW_ERR) {
            ev.type = UART_FIFO_OVF;
            xQueueSendFromISR(s->event_queue, &ev, &woken);
        }
        if (irq_mask & VSF_USART_IRQ_MASK_BREAK_ERR) {
            ev.type = UART_BREAK;
            xQueueSendFromISR(s->event_queue, &ev, &woken);
        }
    }

    (void)woken;
}

/*--------------------------- (re-)apply cached config ----------------------*/

static esp_err_t __uart_apply_config(uart_port_state_t *s)
{
    if (!s->installed) {
        return ESP_OK;
    }

    vsf_usart_irq_disable(s->hw,
        VSF_USART_IRQ_MASK_RX | VSF_USART_IRQ_MASK_TX_CPL
        | VSF_USART_IRQ_MASK_FRAME_ERR | VSF_USART_IRQ_MASK_PARITY_ERR
        | VSF_USART_IRQ_MASK_RX_OVERFLOW_ERR | VSF_USART_IRQ_MASK_BREAK_ERR);
    vsf_usart_cancel_tx(s->hw);
    vsf_usart_disable(s->hw);
    s->tx_pending = 0;

    vsf_usart_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.mode     = __uart_translate_mode(s);
    cfg.baudrate = s->baudrate;
    cfg.rx_timeout = 0;
    if (s->rx_timeout && s->baudrate) {
        cfg.rx_timeout = (uint32_t)s->rx_timeout * 10UL * 1000000UL
                         / s->baudrate;
    }
    cfg.isr.handler_fn = __uart_isr_handler;
    cfg.isr.target_ptr = (void *)s;
    cfg.isr.prio       = vsf_arch_prio_0;

    vsf_err_t err = vsf_usart_init(s->hw, &cfg);
    if (err != VSF_ERR_NONE) {
        return ESP_FAIL;
    }
    vsf_usart_enable(s->hw);

    vsf_usart_irq_mask_t irq = VSF_USART_IRQ_MASK_RX
                              | VSF_USART_IRQ_MASK_TX_CPL;
    if (s->event_queue != NULL) {
        irq |= VSF_USART_IRQ_MASK_FRAME_ERR
             | VSF_USART_IRQ_MASK_PARITY_ERR
             | VSF_USART_IRQ_MASK_RX_OVERFLOW_ERR
             | VSF_USART_IRQ_MASK_BREAK_ERR;
    }
    vsf_usart_irq_enable(s->hw, irq);

    /* re-kick TX pump if stream_tx still has data */
    if (s->stream_tx != NULL
        && vsf_stream_get_data_size(__stream_tx(s)) > 0) {
        __uart_tx_start(s);
    }

    return ESP_OK;
}

/*============================ IMPLEMENTATION ================================*/

/*------------------------- sub-system init ---------------------------------*/

void vsf_espidf_uart_init(const vsf_espidf_uart_cfg_t *cfg)
{
    if (__vsf_espidf_uart.is_inited) {
        return;
    }
    if ((cfg == NULL) || (cfg->pool == NULL) || (cfg->pool_count == 0)) {
        __vsf_espidf_uart.pool       = NULL;
        __vsf_espidf_uart.pool_count = 0;
        __vsf_espidf_uart.ports      = NULL;
        __vsf_espidf_uart.is_inited  = true;
        return;
    }

    uint16_t cnt = cfg->pool_count;
    __vsf_espidf_uart.pool       = cfg->pool;
    __vsf_espidf_uart.pool_count = cnt;

    size_t sz = (size_t)cnt * sizeof(uart_port_state_t);
    uart_port_state_t *p = (uart_port_state_t *)vsf_heap_malloc(sz);
    VSF_ASSERT(p != NULL);
    memset(p, 0, sz);

    for (uint16_t i = 0; i < cnt; i++) {
        p[i].hw = cfg->pool[i];
        p[i].baudrate   = 115200;
        p[i].data_bits  = UART_DATA_8_BITS;
        p[i].parity     = UART_PARITY_DISABLE;
        p[i].stop_bits  = UART_STOP_BITS_1;
        p[i].flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
        p[i].mode       = UART_MODE_UART;
    }

    __vsf_espidf_uart.ports     = p;
    __vsf_espidf_uart.is_inited = true;
}

/*------------------------- helper: allocate a mem_stream -------------------*/

static vsf_mem_stream_t * __uart_alloc_stream(uint32_t buf_size)
{
    size_t total = sizeof(vsf_mem_stream_t) + buf_size;
    vsf_mem_stream_t *ms = (vsf_mem_stream_t *)vsf_heap_malloc(total);
    if (ms == NULL) {
        return NULL;
    }
    memset(ms, 0, total);
    ms->op     = &vsf_mem_stream_op;
    ms->buffer = (uint8_t *)(ms + 1);      /* backing buffer follows struct */
    ms->size   = (int32_t)buf_size;
    vsf_stream_init((vsf_stream_t *)ms);
    return ms;
}

/*------------------------- driver lifecycle --------------------------------*/

esp_err_t uart_driver_install(uart_port_t uart_num,
                              int rx_buffer_size,
                              int tx_buffer_size,
                              int queue_size,
                              QueueHandle_t *uart_queue,
                              int intr_alloc_flags)
{
    (void)intr_alloc_flags;

    if (!__uart_port_valid(uart_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    uart_port_state_t *s = __uart_state(uart_num);
    if (s->installed) {
        return ESP_ERR_INVALID_STATE;
    }

    /* clamp minimum Rx buffer */
    if (rx_buffer_size < (int)UART_HW_FIFO_LEN) {
        rx_buffer_size = UART_HW_FIFO_LEN * 2;
    }

    /* tx_buffer_size == 0 means blocking-write mode */
    s->tx_blocking = (tx_buffer_size <= 0);
    int real_tx_size = (tx_buffer_size > 0)
                     ? tx_buffer_size
                     : __UART_DEFAULT_TX_BUF_SIZE;

    /* allocate streams */
    s->stream_rx = __uart_alloc_stream((uint32_t)rx_buffer_size);
    s->stream_tx = __uart_alloc_stream((uint32_t)real_tx_size);
    if (!s->stream_rx || !s->stream_tx) {
        goto fail;
    }

    /* set Tx stream evthandler: kicks TX pump when app writes data */
    __stream_tx(s)->rx.param      = s;
    __stream_tx(s)->rx.evthandler = __uart_stream_tx_evthandler;
    vsf_stream_connect_rx(__stream_tx(s));
    vsf_stream_connect_tx(__stream_tx(s));

    /* connect Rx stream terminals */
    vsf_stream_connect_tx(__stream_rx(s));
    vsf_stream_connect_rx(__stream_rx(s));

    /* sync primitives */
    s->rx_sem   = xSemaphoreCreateBinary();
    s->tx_sem   = xSemaphoreCreateBinary();
    s->tx_mutex = xSemaphoreCreateMutex();
    s->rx_mutex = xSemaphoreCreateMutex();
    if (!s->rx_sem || !s->tx_sem || !s->tx_mutex || !s->rx_mutex) {
        goto fail;
    }

    if (queue_size > 0) {
        s->event_queue = xQueueCreate((UBaseType_t)queue_size,
                                       sizeof(uart_event_t));
        if (s->event_queue == NULL) {
            goto fail;
        }
    }

    s->tx_pending = 0;
    s->installed  = true;

    esp_err_t ret = __uart_apply_config(s);
    if (ret != ESP_OK) {
        s->installed = false;
        goto fail;
    }

    if (uart_queue != NULL) {
        *uart_queue = s->event_queue;
    }
    return ESP_OK;

fail:
    if (s->stream_rx)   { vsf_heap_free(s->stream_rx);   s->stream_rx = NULL; }
    if (s->stream_tx)   { vsf_heap_free(s->stream_tx);   s->stream_tx = NULL; }
    if (s->rx_sem)      { vSemaphoreDelete(s->rx_sem);    s->rx_sem = NULL; }
    if (s->tx_sem)      { vSemaphoreDelete(s->tx_sem);    s->tx_sem = NULL; }
    if (s->tx_mutex)    { vSemaphoreDelete(s->tx_mutex);  s->tx_mutex = NULL; }
    if (s->rx_mutex)    { vSemaphoreDelete(s->rx_mutex);  s->rx_mutex = NULL; }
    if (s->event_queue) { vQueueDelete(s->event_queue);   s->event_queue = NULL; }
    return ESP_ERR_NO_MEM;
}

esp_err_t uart_driver_delete(uart_port_t uart_num)
{
    if (!__uart_port_valid(uart_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) {
        return ESP_ERR_INVALID_STATE;
    }

    /* quiesce hardware */
    vsf_usart_irq_disable(s->hw,
        VSF_USART_IRQ_MASK_RX | VSF_USART_IRQ_MASK_TX_CPL
        | VSF_USART_IRQ_MASK_FRAME_ERR | VSF_USART_IRQ_MASK_PARITY_ERR
        | VSF_USART_IRQ_MASK_RX_OVERFLOW_ERR | VSF_USART_IRQ_MASK_BREAK_ERR);
    vsf_usart_cancel_tx(s->hw);
    vsf_usart_disable(s->hw);
    vsf_usart_fini(s->hw);

    /* free streams (backing buffer is embedded after the struct) */
    vsf_heap_free(s->stream_rx);  s->stream_rx = NULL;
    vsf_heap_free(s->stream_tx);  s->stream_tx = NULL;

    vSemaphoreDelete(s->rx_sem);    s->rx_sem   = NULL;
    vSemaphoreDelete(s->tx_sem);    s->tx_sem   = NULL;
    vSemaphoreDelete(s->tx_mutex);  s->tx_mutex = NULL;
    vSemaphoreDelete(s->rx_mutex);  s->rx_mutex = NULL;
    if (s->event_queue) {
        vQueueDelete(s->event_queue);
        s->event_queue = NULL;
    }

    s->tx_pending = 0;
    s->installed  = false;
    return ESP_OK;
}

bool uart_is_driver_installed(uart_port_t uart_num)
{
    if (!__uart_port_valid(uart_num)) {
        return false;
    }
    return __uart_state(uart_num)->installed;
}

/*------------------------- configuration -----------------------------------*/

esp_err_t uart_param_config(uart_port_t uart_num,
                            const uart_config_t *uart_config)
{
    if (!__uart_port_valid(uart_num) || (uart_config == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    uart_port_state_t *s = __uart_state(uart_num);

    s->baudrate             = (uint32_t)uart_config->baud_rate;
    s->data_bits            = uart_config->data_bits;
    s->parity               = uart_config->parity;
    s->stop_bits            = uart_config->stop_bits;
    s->flow_ctrl            = uart_config->flow_ctrl;
    s->rx_flow_ctrl_thresh  = uart_config->rx_flow_ctrl_thresh;
    s->config_valid         = true;

    return __uart_apply_config(s);
}

esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baudrate)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->baudrate = baudrate;
    return __uart_apply_config(s);
}

esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t *baudrate)
{
    if (!__uart_port_valid(uart_num) || !baudrate) { return ESP_ERR_INVALID_ARG; }
    *baudrate = __uart_state(uart_num)->baudrate;
    return ESP_OK;
}

esp_err_t uart_set_word_length(uart_port_t uart_num,
                               uart_word_length_t data_bit)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->data_bits = data_bit;
    return __uart_apply_config(s);
}

esp_err_t uart_get_word_length(uart_port_t uart_num,
                               uart_word_length_t *data_bit)
{
    if (!__uart_port_valid(uart_num) || !data_bit) { return ESP_ERR_INVALID_ARG; }
    *data_bit = __uart_state(uart_num)->data_bits;
    return ESP_OK;
}

esp_err_t uart_set_stop_bits(uart_port_t uart_num,
                             uart_stop_bits_t stop_bits)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->stop_bits = stop_bits;
    return __uart_apply_config(s);
}

esp_err_t uart_get_stop_bits(uart_port_t uart_num,
                             uart_stop_bits_t *stop_bits)
{
    if (!__uart_port_valid(uart_num) || !stop_bits) { return ESP_ERR_INVALID_ARG; }
    *stop_bits = __uart_state(uart_num)->stop_bits;
    return ESP_OK;
}

esp_err_t uart_set_parity(uart_port_t uart_num,
                          uart_parity_t parity_mode)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->parity = parity_mode;
    return __uart_apply_config(s);
}

esp_err_t uart_get_parity(uart_port_t uart_num,
                          uart_parity_t *parity_mode)
{
    if (!__uart_port_valid(uart_num) || !parity_mode) { return ESP_ERR_INVALID_ARG; }
    *parity_mode = __uart_state(uart_num)->parity;
    return ESP_OK;
}

esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num,
                                uart_hw_flowcontrol_t flow_ctrl,
                                uint8_t rx_thresh)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->flow_ctrl = flow_ctrl;
    s->rx_flow_ctrl_thresh = rx_thresh;
    return __uart_apply_config(s);
}

esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num,
                                uart_hw_flowcontrol_t *flow_ctrl)
{
    if (!__uart_port_valid(uart_num) || !flow_ctrl) { return ESP_ERR_INVALID_ARG; }
    *flow_ctrl = __uart_state(uart_num)->flow_ctrl;
    return ESP_OK;
}

/*------------------------- I/O ---------------------------------------------*/

int uart_write_bytes(uart_port_t uart_num,
                     const void *src, size_t size)
{
    if (!__uart_port_valid(uart_num) || (src == NULL) || (size == 0)) {
        return -1;
    }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return -1; }

    xSemaphoreTake(s->tx_mutex, portMAX_DELAY);

    const uint8_t *p = (const uint8_t *)src;
    size_t remaining = size;

    while (remaining > 0) {
        uint32_t free_sz = vsf_stream_get_free_size(__stream_tx(s));
        if (free_sz > 0) {
            uint32_t to_write = (free_sz < (uint32_t)remaining)
                              ? free_sz : (uint32_t)remaining;
            uint32_t written = vsf_stream_write(__stream_tx(s),
                                                (uint8_t *)p, to_write);
            p         += written;
            remaining -= written;
            continue;
        }
        /* stream full -- drain stale signal, then wait */
        xSemaphoreTake(s->tx_sem, 0);
        if (vsf_stream_get_free_size(__stream_tx(s)) > 0) {
            continue;
        }
        xSemaphoreTake(s->tx_sem, portMAX_DELAY);
    }

    /* blocking mode: wait until data is physically sent */
    if (s->tx_blocking) {
        while (vsf_stream_get_data_size(__stream_tx(s)) > 0
               || s->tx_pending > 0) {
            xSemaphoreTake(s->tx_sem, 0);
            if (vsf_stream_get_data_size(__stream_tx(s)) == 0
                && s->tx_pending == 0) {
                break;
            }
            xSemaphoreTake(s->tx_sem, pdMS_TO_TICKS(10));
        }
        while (vsf_usart_status(s->hw).is_tx_busy) {
            vTaskDelay(1);
        }
    }

    xSemaphoreGive(s->tx_mutex);
    return (int)size;
}

int uart_write_bytes_with_break(uart_port_t uart_num,
                                const void *src, size_t size,
                                int brk_len)
{
    (void)brk_len;
    int written = uart_write_bytes(uart_num, src, size);
    if (written < 0) {
        return written;
    }
    uart_port_state_t *s = __uart_state(uart_num);
    vsf_usart_ctrl(s->hw, VSF_USART_CTRL_SEND_BREAK, NULL);
    return written;
}

int uart_read_bytes(uart_port_t uart_num,
                    void *buf, uint32_t length,
                    TickType_t ticks_to_wait)
{
    if (!__uart_port_valid(uart_num) || (buf == NULL) || (length == 0)) {
        return -1;
    }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return -1; }

    xSemaphoreTake(s->rx_mutex, portMAX_DELAY);

    /* fast path: data already available */
    uint32_t avail = vsf_stream_get_data_size(__stream_rx(s));
    if (avail > 0) {
        goto read_data;
    }

    if (ticks_to_wait == 0) {
        xSemaphoreGive(s->rx_mutex);
        return 0;
    }

    /* drain stale signal, double-check, then wait */
    xSemaphoreTake(s->rx_sem, 0);
    avail = vsf_stream_get_data_size(__stream_rx(s));
    if (avail > 0) {
        goto read_data;
    }

    {
        BaseType_t ok = xSemaphoreTake(s->rx_sem, ticks_to_wait);
        if (ok != pdTRUE) {
            xSemaphoreGive(s->rx_mutex);
            return 0;
        }
    }
    avail = vsf_stream_get_data_size(__stream_rx(s));

read_data:
    {
        uint32_t to_read = (avail < length) ? avail : length;
        int n = (int)vsf_stream_read(__stream_rx(s), (uint8_t *)buf, to_read);
        xSemaphoreGive(s->rx_mutex);
        return n;
    }
}

/*------------------------- buffer status -----------------------------------*/

esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t *size)
{
    if (!__uart_port_valid(uart_num) || !size) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return ESP_ERR_INVALID_STATE; }

    *size = (size_t)vsf_stream_get_data_size(__stream_rx(s));
    return ESP_OK;
}

esp_err_t uart_get_tx_buffer_free_size(uart_port_t uart_num, size_t *size)
{
    if (!__uart_port_valid(uart_num) || !size) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return ESP_ERR_INVALID_STATE; }

    *size = (size_t)vsf_stream_get_free_size(__stream_tx(s));
    return ESP_OK;
}

/*------------------------- flush -------------------------------------------*/

/* ESP-IDF: uart_flush_input discards all data in the Rx ring buffer. */
esp_err_t uart_flush_input(uart_port_t uart_num)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return ESP_ERR_INVALID_STATE; }

    /* discard all buffered Rx data */
    uint32_t avail = vsf_stream_get_data_size(__stream_rx(s));
    if (avail > 0) {
        vsf_stream_read(__stream_rx(s), NULL, avail);
    }
    /* drain stale semaphore */
    xSemaphoreTake(s->rx_sem, 0);
    return ESP_OK;
}

/* ESP-IDF: uart_flush() == uart_flush_input() */
esp_err_t uart_flush(uart_port_t uart_num)
{
    return uart_flush_input(uart_num);
}

/*------------------------- wait TX done ------------------------------------*/

esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    if (!s->installed) { return ESP_ERR_INVALID_STATE; }

    TickType_t start = xTaskGetTickCount();
    for (;;) {
        if (vsf_stream_get_data_size(__stream_tx(s)) == 0
            && s->tx_pending == 0
            && !vsf_usart_status(s->hw).is_tx_busy) {
            return ESP_OK;
        }
        TickType_t elapsed = xTaskGetTickCount() - start;
        if ((ticks_to_wait != portMAX_DELAY) && (elapsed >= ticks_to_wait)) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1);
    }
}

/*------------------------- pin routing (NOT SUPPORTED) ---------------------*/

esp_err_t uart_set_pin(uart_port_t uart_num,
                       int tx_io_num, int rx_io_num,
                       int rts_io_num, int cts_io_num)
{
    (void)uart_num;
    (void)tx_io_num; (void)rx_io_num;
    (void)rts_io_num; (void)cts_io_num;
    return ESP_ERR_NOT_SUPPORTED;
}

/*------------------------- mode / timeout ----------------------------------*/

esp_err_t uart_set_mode(uart_port_t uart_num, uart_mode_t mode)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->mode = mode;
    return __uart_apply_config(s);
}

esp_err_t uart_set_rx_timeout(uart_port_t uart_num, uint8_t tout)
{
    if (!__uart_port_valid(uart_num)) { return ESP_ERR_INVALID_ARG; }
    uart_port_state_t *s = __uart_state(uart_num);
    s->rx_timeout = tout;
    return __uart_apply_config(s);
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_UART */
