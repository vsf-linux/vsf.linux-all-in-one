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

#ifndef __HAL_DRIVER_COMMON_MULTIPLEX_RNG_H__
#define __HAL_DRIVER_COMMON_MULTIPLEX_RNG_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if (VSF_HAL_USE_RNG == ENABLED) && (VSF_HAL_USE_MULTIPLEX_RNG == ENABLED)

#if defined(__VSF_RNG_MULTIPLEX_CLASS_IMPLEMENT)
#   undef __VSF_RNG_MULTIPLEX_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

/*============================ MACROS ========================================*/

#ifndef VSF_MULTIPLEX_RNG_CFG_MULTI_CLASS
#   define VSF_MULTIPLEX_RNG_CFG_MULTI_CLASS        VSF_RNG_CFG_MULTI_CLASS
#endif

#ifndef VSF_MULTIPLEXER_RNG_CFG_MASK_TYPE
#   define VSF_MULTIPLEXER_RNG_CFG_MASK_TYPE        uint8_t
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/

#if VSF_MULTIPLEX_RNG_CFG_MULTI_CLASS == ENABLED
#   define __describe_multiplex_rng_op()        .op = &vsf_multiplex_rng_op,
#else
#   define __describe_multiplex_rng_op()
#endif

#define __describe_multiplex_rng(__multiplexer, __name)                         \
    vsf_multiplex_rng_t __name = {                                              \
        __describe_multiplex_rng_op()                                           \
        .multiplexer = &(__multiplexer),                                        \
    };

#define __describe_multiplexer_rng(__name, __rng, ...)                          \
    vsf_multiplexer_rng_t __name = {                                            \
        .rng_ptr = __rng,                                                       \
    };                                                                          \
    VSF_MFOREACH_ARG1(__describe_multiplex_rng, __name, __VA_ARGS__)

#define describe_multiplexer_rng(__name, __rng, ...)                            \
    __describe_multiplexer_rng(__name, __rng, __VA_ARGS__)

/*============================ TYPES =========================================*/

typedef VSF_MULTIPLEXER_RNG_CFG_MASK_TYPE vsf_rng_multiplex_mask_t;

vsf_declare_class(vsf_multiplex_rng_t)

vsf_class(vsf_multiplexer_rng_t) {
    public_member(
        vsf_rng_t *rng_ptr;
    )

    private_member(
        vsf_slist_queue_t waiting_queue;

        vsf_multiplex_rng_t *req_m_rng;
        vsf_rng_multiplex_mask_t init_mask;
    )
};

vsf_class(vsf_multiplex_rng_t) {
    public_member(
#if VSF_MULTIPLEX_RNG_CFG_MULTI_CLASS == ENABLED
        implement(vsf_rng_t)
#endif
        vsf_multiplexer_rng_t * const multiplexer;
    )

    private_member(
        vsf_slist_node_t slist_node;
        uint8_t id;

        struct {
            uint32_t *buffer;
            uint32_t num;
            void *param;
            vsf_rng_on_ready_callback_t *on_ready_cb;
        } request;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/

#define VSF_RNG_CFG_DEC_PREFIX              vsf_multiplex
#define VSF_RNG_CFG_DEC_UPCASE_PREFIX       VSF_MULTIPLEX
#define VSF_RNG_CFG_DEC_EXTERN_OP           ENABLED
#include "hal/driver/common/rng/rng_template.h"

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif // VSF_HAL_USE_RNG && VSF_HAL_USE_MULTIPLEX_RNG
#endif // __HAL_DRIVER_COMMON_MULTIPLEX_RNG_H__
