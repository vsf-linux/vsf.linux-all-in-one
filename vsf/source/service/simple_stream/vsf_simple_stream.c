/*****************************************************************************
 *   Copyright(C)2009-2022 by VSF Team                                       *
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

/*============================ INCLUDES ======================================*/
#include "service/vsf_service_cfg.h"

#if VSF_USE_SIMPLE_STREAM == ENABLED

#define __VSF_SIMPLE_STREAM_CLASS_IMPLEMENT
#include "./vsf_simple_stream.h"

/*============================ MACROS ========================================*/

#define VSF_STREAM_TERM_TX                              0
#define VSF_STREAM_TERM_RX                              1

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

void __vsf_stream_on_write(vsf_stream_t *stream)
{
    uint_fast32_t data_len = stream->op->get_data_length(stream);

    stream->tx.data_notified = false;
    if (    stream->rx.ready
#if VSF_STREAM_CFG_THRESHOLD == ENABLED
        &&  (data_len >= stream->rx.threshold)
#else
        &&  (data_len >= 0)
#endif
        &&  !stream->rx.data_notified
        &&  (stream->rx.evthandler != NULL)) {

        stream->rx.data_notified = true;
        stream->rx.evthandler(stream, stream->rx.param, VSF_STREAM_ON_IN);
    }
}

void __vsf_stream_on_read(vsf_stream_t *stream)
{
    uint_fast32_t avail_len = stream->op->get_avail_length(stream);

    stream->rx.data_notified = false;
    if (    stream->tx.ready
#if VSF_STREAM_CFG_THRESHOLD == ENABLED
        &&  (avail_len >= stream->tx.threshold)
#else
        &&  (avail_len >= 0)
#endif
        &&  !stream->tx.data_notified
        &&  (stream->tx.evthandler != NULL)) {

        stream->tx.data_notified = true;
        stream->tx.evthandler(stream, stream->tx.param, VSF_STREAM_ON_OUT);
    }
}

uint_fast32_t vsf_stream_read(vsf_stream_t *stream, uint8_t *buf, uint_fast32_t size)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->read != NULL));
    uint_fast32_t count = stream->op->read(stream, buf, size);
    __vsf_stream_on_read(stream);
    return count;
}

uint_fast32_t vsf_stream_write(vsf_stream_t *stream, uint8_t *buf, uint_fast32_t size)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->write != NULL));
    uint_fast32_t count = stream->op->write(stream, buf, size);
    __vsf_stream_on_write(stream);
    return count;
}

#if VSF_STREAM_CFG_THRESHOLD == ENABLED
void vsf_stream_set_tx_threshold(vsf_stream_t *stream, uint_fast32_t threshold)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    stream->tx.threshold = threshold;
    __vsf_stream_on_read(stream);
}

void vsf_stream_set_rx_threshold(vsf_stream_t *stream, uint_fast32_t threshold)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    stream->rx.threshold = threshold;
    __vsf_stream_on_write(stream);
}
#endif

uint_fast32_t vsf_stream_get_buff_size(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->get_buff_length != NULL));
    return stream->op->get_buff_length(stream);
}

uint_fast32_t vsf_stream_get_data_size(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->get_data_length != NULL));
    return stream->op->get_data_length(stream);
}

uint_fast32_t vsf_stream_get_free_size(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->get_avail_length != NULL));
    return stream->op->get_avail_length(stream);
}

uint_fast32_t vsf_stream_get_wbuf(vsf_stream_t *stream, uint8_t **ptr)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->get_wbuf != NULL));
    uint_fast32_t size = stream->op->get_wbuf(stream, ptr);
#if VSF_STREAM_CFG_TICKTOCK == ENABLED
    uint_fast32_t half_buff_size = vsf_stream_get_buff_size(stream) >> 1;
    if (stream->is_ticktock_write && (size > half_buff_size)) {
        size = half_buff_size;
    }
#endif
    return size;
}

uint_fast32_t vsf_stream_get_rbuf(vsf_stream_t *stream, uint8_t **ptr)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL) && (stream->op->get_rbuf != NULL));
    uint_fast32_t size = stream->op->get_rbuf(stream, ptr);
#if VSF_STREAM_CFG_TICKTOCK == ENABLED
    uint_fast32_t half_buff_size = vsf_stream_get_buff_size(stream) >> 1;
    if (stream->is_ticktock_read && (size > half_buff_size)) {
        size = half_buff_size;
    }
#endif
    return size;
}

static void __vsf_stream_connect_terminal(vsf_stream_t *stream, uint_fast8_t terminal)
{
    vsf_stream_terminal_t *term_current = &stream->terminal[terminal];
    vsf_stream_terminal_t *term_another = &stream->terminal[terminal ^ 1];

    if (!term_current->ready) {
        if (term_another->ready) {
            if (term_another->evthandler != NULL) {
                term_another->evthandler(stream, term_another->param, VSF_STREAM_ON_CONNECT);
            }
            if (term_current->evthandler != NULL) {
                term_current->evthandler(stream, term_current->param, VSF_STREAM_ON_CONNECT);
            }
        }
        term_current->ready = true;
    }
}

void vsf_stream_connect_rx(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    __vsf_stream_connect_terminal(stream, VSF_STREAM_TERM_RX);
}

void vsf_stream_connect_tx(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    __vsf_stream_connect_terminal(stream, VSF_STREAM_TERM_TX);
}

bool vsf_stream_is_rx_connected(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    return stream->rx.ready;
}

bool vsf_stream_is_tx_connected(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    return stream->tx.ready;
}

static void __vsf_stream_disconnect_terminal(vsf_stream_t *stream, uint_fast8_t terminal)
{
    vsf_stream_terminal_t *term_current = &stream->terminal[terminal];
    vsf_stream_terminal_t *term_another = &stream->terminal[terminal ^ 1];

    if (term_current->ready && term_another->ready && (term_another->evthandler != NULL)) {
        term_another->evthandler(stream, term_another->param, VSF_STREAM_ON_DISCONNECT);
    }
    term_current->ready = false;
}

void vsf_stream_disconnect_rx(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    __vsf_stream_disconnect_terminal(stream, VSF_STREAM_TERM_RX);
}

void vsf_stream_disconnect_tx(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT(stream != NULL);
    __vsf_stream_disconnect_terminal(stream, VSF_STREAM_TERM_TX);
}

vsf_err_t vsf_stream_init(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL));
    stream->tx.ready = false;
    stream->tx.data_notified = false;
    stream->rx.ready = false;
    stream->rx.data_notified = false;
    if (stream->op->init != NULL) {
        stream->op->init(stream);
    }
    return VSF_ERR_NONE;
}

vsf_err_t vsf_stream_fini(vsf_stream_t *stream)
{
    VSF_SERVICE_ASSERT((stream != NULL) && (stream->op != NULL));
    if (stream->tx.ready) {
        vsf_stream_disconnect_tx(stream);
    }
    if (stream->rx.ready) {
        vsf_stream_disconnect_rx(stream);
    }
    if (stream->op->fini != NULL) {
        stream->op->fini(stream);
    }
    return VSF_ERR_NONE;
}

uint_fast32_t vsf_stream_adapter_evthandler(vsf_stream_t *stream, void *param, vsf_stream_evt_t evt)
{
    vsf_stream_adapter_t *adapter = param;
    uint_fast32_t tx_size, rx_size, all_size = 0;
    uint_fast32_t threshold_tx = adapter->threshold_tx > 0 ? adapter->threshold_tx-- : 0;
    uint_fast32_t threshold_rx = adapter->threshold_rx > 0 ? adapter->threshold_rx-- : 0;
    bool user_callback = adapter->on_data != NULL;

    if (    (VSF_STREAM_ON_DISCONNECT == evt)
        ||  ((stream == adapter->stream_rx) && (VSF_STREAM_ON_CONNECT == evt))) {
        return 0;
    }

    while ( ((tx_size = vsf_stream_get_data_size(adapter->stream_tx)) > threshold_tx)
        &&  ((rx_size = vsf_stream_get_free_size(adapter->stream_rx)) > threshold_rx)) {

        if (user_callback) {
            all_size += adapter->on_data(adapter, tx_size, rx_size);
        } else {
            uint8_t *buffer;
            tx_size = vsf_stream_get_rbuf(adapter->stream_tx, &buffer);
            rx_size = vsf_stream_write(adapter->stream_rx, buffer, tx_size);
            vsf_stream_read(adapter->stream_tx, NULL, rx_size);
            all_size += rx_size;
            if (rx_size < tx_size) {
                break;
            }
        }
    }

    // clear stream_tx->rx.data_notified if vsf_stream_read is not called,
    //  or VSF_STREAM_ON_IN will never be called again.
    if ((stream == adapter->stream_tx) && stream->rx.data_notified) {
        stream->rx.data_notified = false;
    }
    return all_size;
}

static void __vsf_stream_adapter_evthandler(vsf_stream_t *stream, void *param, vsf_stream_evt_t evt)
{
    vsf_stream_adapter_evthandler(stream, param, evt);
}

void vsf_stream_adapter_init(vsf_stream_adapter_t *adapter)
{
    adapter->stream_rx->tx.evthandler = __vsf_stream_adapter_evthandler;
    adapter->stream_rx->tx.param = adapter;
    vsf_stream_connect_tx(adapter->stream_rx);

    adapter->stream_tx->rx.evthandler = __vsf_stream_adapter_evthandler;
    adapter->stream_tx->rx.param = adapter;
    vsf_stream_connect_rx(adapter->stream_tx);
}

#endif      // VSF_USE_SIMPLE_STREAM
