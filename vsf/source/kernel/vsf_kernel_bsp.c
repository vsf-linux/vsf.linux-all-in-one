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

#include "kernel/vsf_kernel_cfg.h"

#if VSF_USE_KERNEL == ENABLED
#define __VSF_EDA_CLASS_INHERIT__
#include "./vsf_kernel_common.h"
#include "./vsf_eda.h"
#include "./vsf_evtq.h"
#include "./vsf_os.h"

#if     VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_THREAD                     \
    &&  VSF_KERNEL_CFG_SUPPORT_THREAD == ENABLED
#   include "./task/vsf_thread.h"
#endif

#include "utilities/vsf_utilities.h"

/*============================ MACROS ========================================*/

// for __RTOS__ arch, VSF_KERNEL_CFG_NON_STANDALONE MUST be enabled
//  because the standard entry is for RTOS, not for VSF
#ifdef __RTOS__
#   ifndef VSF_KERNEL_CFG_NON_STANDALONE
#       define VSF_KERNEL_CFG_NON_STANDALONE                ENABLED
#   endif
#   if VSF_KERNEL_CFG_NON_STANDALONE != ENABLED
#       error VSF_KERNEL_CFG_NON_STANDALONE MUST be enabled for __RTOS__ support
#   endif
#endif

#define __VSF_OS_EVTQ_SWI_PRIO_INIT(__INDEX, __UNUSED)                          \
    VSF_ARCH_PRIO_##__INDEX,

#if VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_THREAD
#   ifndef VSF_OS_CFG_MAIN_STACK_SIZE
#       warning VSF_OS_CFG_MAIN_STACK_SIZE not defined, set to 4K by default
#       define VSF_OS_CFG_MAIN_STACK_SIZE                   (4096)
#   endif
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

#if     VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_THREAD                     \
    &&  VSF_KERNEL_CFG_SUPPORT_THREAD == ENABLED
dcl_vsf_thread(app_main_thread_t)
def_vsf_thread(app_main_thread_t, VSF_OS_CFG_MAIN_STACK_SIZE)
#endif

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern void vsf_main_entry(void);
extern int VSF_USER_ENTRY(void);

/*============================ IMPLEMENTATION ================================*/

VSF_CAL_ROOT
const vsf_kernel_resource_t * vsf_kernel_get_resource_on_init(void)
{

#if __VSF_OS_SWI_NUM > 0

/*! \note it is important to understand the differences between following
 *!       configurations when __VSF_OS_SWI_NUM == 1
 *!
 *!       1. VSF_KERNEL_CFG_ALLOW_KERNEL_BEING_PREEMPTED == ENABLED
 *!          This configuration means user interrupt handlers and systimer
 *!          can have higher priorities and preempt the kernel and kernel
 *!          managed tasks.
 *!
 *!       2. VSF_KERNEL_CFG_ALLOW_KERNEL_BEING_PREEMPTED != ENABLED
 *!          This configuration assumes that all system works in cooperative
 *!          way, no user interrupt or systimer interrupt can have priority
 *!          different from the one used by kernel.
 *!          NOTE: This configuration doesn't stop kernel managed tasks from
 *!          preempting idle task.
 */

#   if  (__VSF_OS_SWI_NUM < VSF_ARCH_PRI_NUM)                                   \
    &&  (   (__VSF_OS_SWI_NUM > 1)                                              \
        ||  (__VSF_OS_SWI_NUM == 1 && VSF_KERNEL_CFG_ALLOW_KERNEL_BEING_PREEMPTED == ENABLED))
/*! \note including (vsf_prio_highest + 1) into the lookup table
 */
#       define MFUNC_IN_U8_DEC_VALUE                   (__VSF_OS_SWI_NUM + 1)
#   else
#       define MFUNC_IN_U8_DEC_VALUE                   (__VSF_OS_SWI_NUM)
#   endif

#   include "utilities/preprocessor/mf_u8_dec2str.h"
    static const vsf_arch_prio_t __vsf_os_swi_priority[] = {
        VSF_MREPEAT(MFUNC_OUT_DEC_STR, __VSF_OS_EVTQ_SWI_PRIO_INIT, NULL)
    };
#endif

#if __VSF_KERNEL_CFG_EVTQ_EN == ENABLED

    static VSF_CAL_NO_INIT vsf_evtq_t __vsf_os_evt_queue[VSF_OS_CFG_PRIORITY_NUM];

#   if defined(__VSF_OS_CFG_EVTQ_LIST) && defined(VSF_OS_CFG_EVTQ_POOL_SIZE)
    static VSF_CAL_NO_INIT vsf_pool_item(vsf_evt_node_pool)
        __evt_node_buffer[VSF_OS_CFG_EVTQ_POOL_SIZE];
#   endif

#   if defined(__VSF_OS_CFG_EVTQ_ARRAY)
    static VSF_CAL_NO_INIT vsf_evt_node_t
        __vsf_os_nodes[VSF_OS_CFG_PRIORITY_NUM][1 << VSF_OS_CFG_EVTQ_BITSIZE];
#   endif
#endif

#if     __VSF_KERNEL_CFG_EDA_FRAME_POOL == ENABLED
    static VSF_CAL_NO_INIT vsf_pool_item(vsf_eda_frame_pool)
        __vsf_eda_frame_buffer[VSF_OS_CFG_EDA_FRAME_POOL_SIZE];
#endif

    static const vsf_kernel_resource_t __res = {
#if __VSF_OS_SWI_NUM > 0
        {
            __vsf_os_swi_priority,                  // os_swi_priorities_ptr
            dimof(__vsf_os_swi_priority),           // swi_priority_cnt
            {__VSF_OS_SWI_PRIORITY_BEGIN, vsf_prio_highest},
        },
#else
        {
            {vsf_prio_highest},
        },
#endif

#if __VSF_KERNEL_CFG_EVTQ_EN == ENABLED
        {
            __vsf_os_evt_queue,                     // queue_array
#   if defined(__VSF_OS_CFG_EVTQ_ARRAY)
            (vsf_evt_node_t **)__vsf_os_nodes,      // nodes
            VSF_OS_CFG_EVTQ_BITSIZE,                // node_bit_sz
#   endif
#   if defined(__VSF_OS_CFG_EVTQ_LIST)
#       if defined(VSF_OS_CFG_EVTQ_POOL_SIZE)
            __evt_node_buffer,                      // nodes_buf_ptr
            (uint16_t)dimof(__evt_node_buffer),     // node_cnt
#       else
            NULL,                                   // nodes_buf_ptr
            0,                                      // node_cnt
#       endif
#   endif
            (uint16_t)dimof(__vsf_os_evt_queue),    // queue_cnt
        },

#endif

#if     __VSF_KERNEL_CFG_EDA_FRAME_POOL == ENABLED
        {
            __vsf_eda_frame_buffer,                 // frame_buf_ptr
            dimof(__vsf_eda_frame_buffer),          // frame_cnt
        },
#endif

    };

    return &__res;
}

void vsf_kernel_err_report(vsf_kernel_error_t err)
{
    switch (err) {

        case VSF_KERNEL_ERR_INVALID_CONTEXT:
            /*! \note
            This should not happen. Two possible reasons could be:
            1. Forgeting to set VSF_OS_CFG_MAIN_MODE to VSF_OS_CFG_MAIN_MODE_THREAD
               and using vsf kernel APIs, e.g. vsf_delay_ms, vsf_sem_pend and etc.
            2. When VSF_OS_CFG_MAIN_MODE is not VSF_OS_CFG_MAIN_MODE_THREAD, using
               any vsf_eda_xxxx APIs.
            3. call APIs depends on eda context in non-eda context
            */

        case VSF_KERNEL_ERR_SHOULD_NOT_USE_PRIO_INHERIT_IN_IDLE_OR_ISR:
            /*! \note
             This should not happen. One possible reason is:
             > You start task, e.g. eda, vsf_task, vsf_pt or vsf_thread in the idle
               task. Please use vsf_prio_0 when you do this in idle task.
             */
        default:
            {
                vsf_disable_interrupt();
                while(1);
            }
            //break;

        case VSF_KERNEL_ERR_NONE:
            break;
    }

}


#if     VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_THREAD                     \
    &&  VSF_KERNEL_CFG_SUPPORT_THREAD == ENABLED
implement_vsf_thread(app_main_thread_t)
{
    VSF_UNUSED_PARAM(vsf_pthis);
    VSF_USER_ENTRY();
}
#elif   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_EDA                        \
    ||  (   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_IDLE                   \
        &&  VSF_OS_CFG_ADD_EVTQ_TO_IDLE == ENABLED)
static void __app_main_evthandler(vsf_eda_t *eda, vsf_evt_t evt)
{
    VSF_UNUSED_PARAM(eda);
    VSF_UNUSED_PARAM(evt);
    VSF_USER_ENTRY();
}
#endif

VSF_CAL_ROOT void __post_vsf_kernel_init(void)
{
#if     VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_THREAD                     \
    &&  VSF_KERNEL_CFG_SUPPORT_THREAD == ENABLED

#   if VSF_KERNEL_THREAD_USE_HOST == ENABLED
    // no set stack, host_thread is used, remove VSF_CAL_NO_INIT because host_thread need to be initialized to 0
    static app_main_thread_t __app_main;
#   else
    static VSF_CAL_NO_INIT app_main_thread_t __app_main;
#   endif

#   if VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE == ENABLED
    __app_main  .use_as__vsf_thread_t
#       if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
                .use_as__vsf_teda_t
#       endif
                .use_as__vsf_eda_t
                .on_terminate = NULL;
#   endif
    init_vsf_thread(app_main_thread_t, &__app_main, vsf_prio_0);
#   if VSF_KERNEL_CFG_TRACE == ENABLED
    vsf_kernel_trace_eda_info(&__app_main.use_as__vsf_eda_t, "main_task",
                __app_main.stack_arr, sizeof(__app_main.stack_arr));
#   endif
#elif   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_EDA                        \
    ||  (   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_IDLE                   \
        &&  VSF_OS_CFG_ADD_EVTQ_TO_IDLE == ENABLED)
    const vsf_eda_cfg_t cfg = {
        .fn.evthandler = __app_main_evthandler,
        .priority = vsf_prio_0,
    };
#   if  VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
    static VSF_CAL_NO_INIT vsf_teda_t __app_main;
#       if VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE == ENABLED
    __app_main  .use_as__vsf_eda_t
                .on_terminate = NULL;
#       endif
    vsf_teda_start(&__app_main, (vsf_eda_cfg_t *)&cfg);
#   else
    static VSF_CAL_NO_INIT vsf_eda_t __app_main;
#       if VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE == ENABLED
    __app_main  .on_terminate = NULL;
#       endif
    vsf_eda_start(&__app_main, (vsf_eda_cfg_t *)&cfg);
#   endif
#   if VSF_KERNEL_CFG_TRACE == ENABLED
    vsf_kernel_trace_eda_info((vsf_eda_t *)&__app_main, "main_task", NULL, 0);
#   endif
#elif   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_IDLE
    VSF_USER_ENTRY();
#elif   VSF_OS_CFG_MAIN_MODE == VSF_OS_CFG_MAIN_MODE_NONE
#else
#   error Please define VSF_OS_CFG_MAIN_MODE!!! and make sure there is no\
conflict configurations. E.g. When C89/90 is used, VSF_KERNEL_CFG_SUPPORT_THREAD \
will be forced to DISABLED.
#endif
}

#if VSF_USE_HEAP == ENABLED && VSF_USE_ARCH_HEAP != ENABLED
VSF_CAL_WEAK(vsf_service_req___heap_memory_buffer___from_usr)
vsf_mem_t vsf_service_req___heap_memory_buffer___from_usr(void)
{
#ifndef VSF_HEAP_SIZE
#   warning \
"VSF_USE_HEAP is enabled but VSF_HEAP_SIZE hasn't been defined. You can define \
this macro in vsf_usr_cfg.h or you can call vsf_heap_add()/vsf_heap_add_memory()\
 to add memory buffers to heap."
    return (vsf_mem_t){0};
#else
#if     defined(VSF_HEAP_ADDR)
    uint8_t *__heap_buffer = (uint8_t *)(VSF_HEAP_ADDR);
#elif   defined(VSF_HEAP_SECTION)
    VSF_CAL_NO_INIT static uint_fast8_t __heap_buffer[
        (VSF_HEAP_SIZE + sizeof(uint_fast8_t) - 1) / sizeof(uint_fast8_t)] VSF_CAL_SECTION(VSF_HEAP_SECTION);
#else
    VSF_CAL_NO_INIT static uint_fast8_t __heap_buffer[
        (VSF_HEAP_SIZE + sizeof(uint_fast8_t) - 1) / sizeof(uint_fast8_t)];
#endif

    return (vsf_mem_t){
        .ptr.src_ptr = (uint8_t *)__heap_buffer,
        .s32_size = VSF_HEAP_SIZE,
    };
#endif
}
#endif

// call vsf_main_entry in a standalone host-os thread
#if VSF_KERNEL_CFG_NON_STANDALONE != ENABLED

#if __IS_COMPILER_ARM_COMPILER_6__
__asm(".global __use_no_semihosting\n\t");

#   ifndef __MICROLIB
__asm(".global __ARM_use_no_argv\n\t");
#   endif
#endif

/*----------------------------------------------------------------------------*
 * Compiler Specific Code to run vsf_main_entry() before main()               *
 *----------------------------------------------------------------------------*/
#if __IS_COMPILER_IAR__
VSF_CAL_WEAK(__low_level_init)
LOW_LEVEL_INIT_RET_T __low_level_init(void)
{
    return 1;
}

extern void __IAR_STARTUP_DATA_INIT(void);
extern void exit(int arg);

#   if defined(__CPU_MCS51__)
__root
__noreturn
__stackless void __cmain(void)
{
    vsf_main_entry();
    exit(0);
}

#   else

__root
__noreturn
#ifndef __VSF_CPP__
__stackless
#endif
void __cmain(void)
{
    if (__low_level_init() != 0) {
        // no need to initialize heap here anymore, call vsf_arch_cpp_startup when suitable,
        //  which will can cpp_ctors securely
        __IAR_STARTUP_DATA_INIT();
    }
    vsf_main_entry();
    exit(0);
}
#   endif
#elif   __IS_COMPILER_GCC__                                                     \
    ||  __IS_COMPILER_LLVM__                                                    \
    ||  __IS_COMPILER_ARM_COMPILER_5__                                          \
    ||  __IS_COMPILER_ARM_COMPILER_6__

#if VSF_KERNEL_CFG_ENTRY_IS_MAIN == ENABLED
int main(void)
{
    vsf_main_entry();
#   ifdef VSF_ARCH_ENTRY_NO_PENDING
    vsf_arch_main();
#   endif
    return 0;
}
#else
#   if __IS_COMPILER_SUPPORT_GNUC_EXTENSION__
__attribute__((constructor(MAX_CONSTRUCTOR_PRIORITY)))
#   endif
void __vsf_main_entry(void)
{
    vsf_main_entry();
}
#endif


#else

#warning please call vsf_main_entry() before entering the main.
#endif

#endif      // VSF_KERNEL_CFG_NON_STANDALONE

#endif
/*EOF*/
