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

#ifndef __VSF_FREERTOS_INTERNAL_H__
#define __VSF_FREERTOS_INTERNAL_H__

/*============================ INCLUDES ======================================*/

#include "kernel/vsf_kernel.h"
#include "./vsf_freertos_cfg.h"

#if VSF_USE_FREERTOS == ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/

typedef struct vsf_freertos_t {
    bool is_inited;
} vsf_freertos_t;

// The task control block used by task / notify ports is now
// StaticTask_t in include/task.h (vsf_class-based). The vsf_thread_t
// remains the first private member so that (vsf_thread_t *) and
// (StaticTask_t *) are bit-wise interchangeable -- xTaskGetCurrentTaskHandle
// returns vsf_thread_get_cur() and ports cast it back.

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

// Sub-system lifecycle. Called from user init (once).
extern void vsf_freertos_init(void);

#ifdef __cplusplus
}
#endif

#endif      // VSF_USE_FREERTOS
#endif      // __VSF_FREERTOS_INTERNAL_H__
