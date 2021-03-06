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

#ifndef __VSF_TASK_COMMON_H__
#define __VSF_TASK_COMMON_H__

/*============================ INCLUDES ======================================*/
#include "./kernel/vsf_kernel_cfg.h"
#if VSF_USE_KERNEL == ENABLED
#include "../vsf_eda.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/

#define __vsf_pt_begin_common(__state)                                          \
            enum {                                                              \
                count_offset = __COUNTER__ + 1,                                 \
            };                                                                  \
            switch ((__state)) {                                                \
                case __COUNTER__ - count_offset:

#define __vsf_pt_raw_next_common(__state)                                       \
            (__state) = (__COUNTER__ - count_offset + 1) >> 1
#define __vsf_pt_raw_entry_common(__state, ...)                                 \
            __VA_ARGS__;                                                        \
            case (__COUNTER__ - count_offset) >> 1:

#define __vsf_pt_entry_common(__state, ...)                                     \
            __vsf_pt_raw_entry_common(__state,                                  \
                __vsf_pt_raw_next_common(__state);                              \
                __VA_ARGS__                                                     \
            )

#define __vsf_pt_end_common()       } vsf_eda_return();

//#define __vsf_pt_end_closure_common()                                         \
//                }   /* for switch */                                          \
//            __vsf_pt_end_common()

#define __vsf_pt_wait_cond_common(__state, ...)                                 \
    do {                                                                        \
        evt = VSF_EVT_INVALID;                                                  \
        __vsf_pt_entry_common(__state);                                         \
        if (!(__VA_ARGS__)) {                                                   \
            return ;                                                            \
        }                                                                       \
    } while (0)


#define __vsf_pt_wfe_common(__state, __evt)                                     \
    __vsf_pt_wait_cond_common(__state, (evt == __evt))

#define __vsf_pt_func_common(__name)        vsf_pt_func_##__name
#define __vsf_pt_common(__name)             pt_cb_##__name

#define __def_vsf_pt_common(__name, ...)                                        \
        struct __vsf_pt_common(__name) {                                        \
            __VA_ARGS__                                                         \
        };                                                                      \
        struct __name {                                                         \
            implement(vsf_pt_t)                                                 \
            implement_ex(__vsf_pt_common(__name), param)                        \
        };

#define __declare_vsf_pt_common(__name)                                         \
            typedef struct __name __name;                                       \
            typedef struct __vsf_pt_common(__name) __vsf_pt_common(__name);


#define __implement_vsf_pt_common(__name, __arg0)                               \
        void __vsf_pt_func_common(__name)(__arg0, vsf_evt_t evt)

#define __extern_vsf_pt_common(__name, __arg0)                                  \
        extern void __vsf_pt_func_common(__name)(__arg0, vsf_evt_t evt);



/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

#ifdef __cplusplus
}
#endif

#endif
#endif
