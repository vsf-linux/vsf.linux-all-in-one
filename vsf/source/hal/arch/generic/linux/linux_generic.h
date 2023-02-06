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

#ifndef __LINUX_GENERIC_H__
#define __LINUX_GENERIC_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"
#include "utilities/vsf_utilities.h"

#if     defined(__VSF_ARCH_LINUX_IMPLEMENT)
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

#ifndef __LITTLE_ENDIAN
#   define __LITTLE_ENDIAN              1
#endif
#ifndef __BYTE_ORDER
#   define __BYTE_ORDER                 __LITTLE_ENDIAN
#endif

#ifndef VSF_ARCH_PRI_NUM
#   define VSF_ARCH_PRI_NUM             1
#endif
#if VSF_ARCH_PRI_NUM != 1
#   error VSF_ARCH_PRI_NUM MUST be 1 for linux
#endif

#ifndef VSF_SYSTIMER_CFG_IMPL_MODE
#   define VSF_SYSTIMER_CFG_IMPL_MODE   VSF_SYSTIMER_IMPL_REQUEST_RESPONSE
#endif

// software interrupt provided by arch
#ifndef VSF_ARCH_SWI_NUM
#   define VSF_ARCH_SWI_NUM             0
#endif
#if VSF_ARCH_SWI_NUM != 0
#   error VSF_ARCH_SWI_NUM MUST be 0 for linux
#endif

#define VSF_ARCH_STACK_ALIGN_BIT        4
#define VSF_ARCH_LIMIT_NO_SET_STACK

#define VSF_ARCH_STACK_PAGE_SIZE        4096
#define VSF_ARCH_STACK_GUARDIAN_SIZE    4096

/*============================ MACROFIED FUNCTIONS ===========================*/

#define __VSF_ARCH_PRI(__N, __BIT)                                              \
            VSF_ARCH_PRIO_##__N = (__N),                                        \
            vsf_arch_prio_##__N = (__N),

#define vsf_arch_wakeup()

/*============================ TYPES =========================================*/

// unit is us
typedef uint64_t vsf_systimer_tick_t;

typedef enum vsf_arch_prio_t {
    VSF_ARCH_PRIO_INVALID = -1,
    vsf_arch_prio_invalid = -1,
    VSF_MREPEAT(VSF_ARCH_PRI_NUM, __VSF_ARCH_PRI, VSF_ARCH_PRI_BIT)
    vsf_arch_prio_highest = VSF_ARCH_PRI_NUM - 1,
} vsf_arch_prio_t;

typedef void (*vsf_arch_irq_entry_t)(void*);

vsf_class(vsf_arch_irq_request_t) {
    private_member(
        int id;
        void *irq_thread_to_exit;
        void *arch_thread;
        bool is_triggered;
        bool is_inited;
    )
};

typedef volatile bool vsf_gint_state_t;

/*============================ INCLUDES ======================================*/

#include "hal/arch/common/arch_without_thread_suspend/vsf_arch_without_thread_suspend_template.h"

/*============================ TYPES =========================================*/

vsf_class(vsf_arch_irq_thread_t) {
    private_member(
        implement(vsf_arch_irq_thread_common_t)
        void *exit;
        bool is_to_restart;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern void __vsf_arch_irq_sleep(uint32_t ms);

extern void __vsf_arch_irq_request_init(vsf_arch_irq_request_t *request);
extern void __vsf_arch_irq_request_fini(vsf_arch_irq_request_t *request);
extern void __vsf_arch_irq_request_pend(vsf_arch_irq_request_t *request);
extern void __vsf_arch_irq_request_send(vsf_arch_irq_request_t *request);

extern void __vsf_arch_irq_init(vsf_arch_irq_thread_t *irq_thread, char *name,
    vsf_arch_irq_entry_t entry, vsf_arch_prio_t priority);
extern void __vsf_arch_irq_fini(vsf_arch_irq_thread_t *irq_thread);
extern void __vsf_arch_irq_set_background(vsf_arch_irq_thread_t *irq_thread);
extern void __vsf_arch_irq_start(vsf_arch_irq_thread_t *irq_thread);
extern void __vsf_arch_irq_end(vsf_arch_irq_thread_t *irq_thread, bool is_terminate);

extern void __vsf_arch_irq_exit(vsf_arch_irq_thread_t *irq_thread);
extern vsf_err_t __vsf_arch_irq_restart(vsf_arch_irq_thread_t *irq_thread,
                    vsf_arch_irq_request_t *request_pending);

#ifdef __cplusplus
}
#endif

#endif
/* EOF */
