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

#ifndef __VSF_USART_STREAM_H__
#define __VSF_USART_STREAM_H__

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_USART == ENABLED && VSF_USE_SIMPLE_STREAM == ENABLED

// for vsf_usart_t
#include "hal/vsf_hal.h"
// for stream
#include "service/vsf_service.h"

#if     defined(__VSF_USART_STREAM_CLASS_IMPLEMENT)
#   undef __VSF_USART_STREAM_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_USART_STREAM_CLASS_INHERIT__)
#   undef __VSF_USART_STREAM_CLASS_INHERIT__
#   define __VSF_CLASS_INHERIT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

vsf_class(vsf_usart_stream_t) {
    public_member(
        vsf_usart_t *usart;
        // dataflow:
        // usart_rx --> stream_rx.tx
        // usart_tx <-- stream_tx.rx
        vsf_stream_t *stream_rx;
        vsf_stream_t *stream_tx;
    )
    private_member(
        struct {
            uint32_t size;
        } rx;
        struct {
            uint32_t size;
        } tx;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

extern vsf_err_t vsf_usart_stream_init(vsf_usart_stream_t *usart_stream, vsf_usart_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif      // VSF_HAL_USE_USART && VSF_USE_SIMPLE_STREAM
#endif      // __VSF_USART_STREAM_H__
