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

#ifndef __HAL_DRIVER_BL_BL616_COMMON_H__
#define __HAL_DRIVER_BL_BL616_COMMON_H__

/* \note __common.h should only be included by device.h */

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"
// include vendor common headers here

#include "hal/arch/vsf_arch.h"

#include "service/simple_stream/vsf_simple_stream.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

extern vsf_stream_t VSF_DEBUG_STREAM_TX;
extern vsf_mem_stream_t VSF_DEBUG_STREAM_RX;

/*============================ LOCAL VARIABLES ===============================*/
/*============================ INCLUDES ======================================*/
/*============================ PROTOTYPES ====================================*/

#endif
/* EOF */
