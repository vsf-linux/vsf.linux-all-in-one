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

#define __VSF_RNG_MULTIPLEX_CLASS_IMPLEMENT
#include "hal/vsf_hal_cfg.h"

#if (VSF_HAL_USE_RNG == ENABLED) && (VSF_HAL_USE_MULTIPLEX_RNG == ENABLED)

#include "hal/driver/driver.h"

/*============================ MACROS ========================================*/

#ifdef VSF_MULTIPLEXER_RNG_CFG_CALL_PREFIX
#   undef VSF_RNG_CFG_PREFIX
#   define VSF_RNG_CFG_PREFIX                   VSF_MULTIPLEXER_RNG_CFG_CALL_PREFIX
#endif

#ifndef VSF_MULTIPLEX_RNG_PROTECT_LEVEL
#   define VSF_MULTIPLEX_RNG_PROTECT_LEVEL      interrupt
#endif

#define VSF_RNG_CFG_IMP_EXTERN_OP               ENABLED

#define vsf_multiplex_rng_protect               vsf_protect(VSF_MULTIPLEX_RNG_PROTECT_LEVEL)
#define vsf_multiplex_rng_unprotect             vsf_unprotect(VSF_MULTIPLEX_RNG_PROTECT_LEVEL)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

static void __rng_on_ready_handler(void *param, uint32_t *buffer, uint32_t num);

/*============================ IMPLEMENTATION ================================*/

static void __m_rng_start_request(vsf_multiplex_rng_t *m_rng_ptr)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);

    vsf_err_t result = vsf_rng_generate_request(multiplexer->rng_ptr,
                                                m_rng_ptr->request.buffer,
                                                m_rng_ptr->request.num,
                                                m_rng_ptr,
                                                __rng_on_ready_handler);
    (void)result;
    VSF_HAL_ASSERT(result == VSF_ERR_NONE);
}

static void __rng_on_ready_handler(void *param, uint32_t *buffer, uint32_t num)
{
    vsf_multiplex_rng_t *m_rng_ptr = (vsf_multiplex_rng_t *)param;
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);

    // Snapshot the completing consumer's callback before slot is released.
    // This is needed because the same m_rng may be re-submitted by another
    // consumer (or by the user callback itself) once req_m_rng is cleared.
    vsf_rng_on_ready_callback_t *user_cb = m_rng_ptr->request.on_ready_cb;
    void *user_param = m_rng_ptr->request.param;

    vsf_multiplex_rng_t *next_m_rng;
    vsf_protect_t state = vsf_multiplex_rng_protect();
        vsf_slist_queue_dequeue(vsf_multiplex_rng_t, slist_node,
                                &multiplexer->waiting_queue, next_m_rng);
        multiplexer->req_m_rng = next_m_rng;
    vsf_multiplex_rng_unprotect(state);

    // Kick off the next queued consumer before invoking user callback to
    // minimize hardware idle time between back-to-back requests.
    if (next_m_rng != NULL) {
        __m_rng_start_request(next_m_rng);
    }

    if (user_cb != NULL) {
        user_cb(user_param, buffer, num);
    }
}

vsf_err_t vsf_multiplex_rng_init(vsf_multiplex_rng_t *m_rng_ptr)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);

    vsf_err_t result = VSF_ERR_NONE;
    bool need = false;
    vsf_protect_t state = vsf_multiplex_rng_protect();
        if (multiplexer->init_mask == 0) {                      // first init
            vsf_slist_queue_init(&multiplexer->waiting_queue);
            multiplexer->req_m_rng = NULL;
            need = true;
        }

        // Allocate a free id for the new instance.
        int_fast8_t id = vsf_ffz32(multiplexer->init_mask);
        if (id < 0) {
            vsf_multiplex_rng_unprotect(state);
            return VSF_ERR_NOT_ENOUGH_RESOURCES;
        }

        multiplexer->init_mask |= (1 << id);
        m_rng_ptr->id = id;
    vsf_multiplex_rng_unprotect(state);

    if (need) {
        result = vsf_rng_init(multiplexer->rng_ptr);
    }

    return result;
}

void vsf_multiplex_rng_fini(vsf_multiplex_rng_t *m_rng_ptr)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);
    VSF_HAL_ASSERT(multiplexer->init_mask & (1 << m_rng_ptr->id));

    bool need = false;
    vsf_protect_t state = vsf_multiplex_rng_protect();
        multiplexer->init_mask &= ~(1 << m_rng_ptr->id);
        need = (multiplexer->init_mask == 0);
    vsf_multiplex_rng_unprotect(state);

    if (need) {
        vsf_rng_fini(multiplexer->rng_ptr);
    }
}

vsf_rng_capability_t vsf_multiplex_rng_capability(vsf_multiplex_rng_t *m_rng_ptr)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);

    return vsf_rng_capability(multiplexer->rng_ptr);
}

vsf_err_t vsf_multiplex_rng_generate_request(vsf_multiplex_rng_t *m_rng_ptr,
                                             uint32_t *buffer, uint32_t num,
                                             void *param,
                                             vsf_rng_on_ready_callback_t *on_ready_cb)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);
    VSF_HAL_ASSERT(multiplexer->init_mask & (1 << m_rng_ptr->id));

    m_rng_ptr->request.buffer = buffer;
    m_rng_ptr->request.num = num;
    m_rng_ptr->request.param = param;
    m_rng_ptr->request.on_ready_cb = on_ready_cb;

    vsf_err_t result = VSF_ERR_NONE;
    bool need_start = false;
    vsf_protect_t state = vsf_multiplex_rng_protect();
        if ((multiplexer->req_m_rng == m_rng_ptr)
         || vsf_slist_queue_is_in(vsf_multiplex_rng_t, slist_node,
                                  &multiplexer->waiting_queue, m_rng_ptr)) {
            // The previous request from this consumer is not yet complete.
            VSF_HAL_ASSERT(0);
            result = VSF_ERR_FAIL;
        } else if (multiplexer->req_m_rng == NULL) {            // hardware idle
            multiplexer->req_m_rng = m_rng_ptr;
            need_start = true;
        } else {                                                // hardware busy
            vsf_slist_queue_enqueue(vsf_multiplex_rng_t, slist_node,
                                    &multiplexer->waiting_queue, m_rng_ptr);
        }
    vsf_multiplex_rng_unprotect(state);

    if (need_start) {
        __m_rng_start_request(m_rng_ptr);
    }

    return result;
}

vsf_err_t vsf_multiplex_rng_ctrl(vsf_multiplex_rng_t *m_rng_ptr, vsf_rng_ctrl_t ctrl, void *param)
{
    VSF_HAL_ASSERT(NULL != m_rng_ptr);
    vsf_multiplexer_rng_t * const multiplexer = m_rng_ptr->multiplexer;
    VSF_HAL_ASSERT(NULL != multiplexer);

    return vsf_rng_ctrl(multiplexer->rng_ptr, ctrl, param);
}

/*============================ GLOBAL VARIABLES ==============================*/

#define VSF_RNG_CFG_REIMPLEMENT_API_CAPABILITY          ENABLED
#define VSF_RNG_CFG_REIMPLEMENT_API_CTRL                ENABLED
#define VSF_RNG_CFG_IMP_PREFIX                          vsf_multiplex
#define VSF_RNG_CFG_IMP_UPCASE_PREFIX                   VSF_MULTIPLEX
#define VSF_RNG_CFG_IMP_EXTERN_OP                       ENABLED
#include "hal/driver/common/rng/rng_template.inc"

#endif // VSF_HAL_USE_RNG && VSF_HAL_USE_MULTIPLEX_RNG
