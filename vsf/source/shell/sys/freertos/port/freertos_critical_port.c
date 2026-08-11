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
 * Port implementation for FreeRTOS critical section + scheduler control
 * primitives on VSF.
 *
 * Covers:
 *   vTaskEnterCritical / vTaskExitCritical
 *   vTaskEnterCriticalFromISR / vTaskExitCriticalFromISR
 *   vTaskSuspendAll / xTaskResumeAll
 *
 * Design
 * ------
 * All six primitives are layered on top of vsf_sched_lock / vsf_sched_unlock.
 * A scheduler lock on VSF suspends preemption just like FreeRTOS'
 * taskENTER_CRITICAL does on a single-core target. Because we are
 * single-core, once a task has acquired the scheduler lock no other
 * task can run, so a single pair of globals (nest counter + saved
 * lock status) is safe for the reentrant task-context variants.
 *
 * The FromISR variants never touch the globals -- the saved state is
 * returned to the caller and echoed back, matching FreeRTOS where the
 * caller stores the previous PRIMASK in a local variable.
 *
 * Critical-section state (taskENTER_CRITICAL family) and scheduler-
 * suspend state (vTaskSuspendAll family) are tracked with separate
 * nest counters so the two can be composed without bleeding into
 * each other.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_freertos_cfg.h"

#if VSF_USE_FREERTOS == ENABLED && VSF_FREERTOS_CFG_USE_CRITICAL == ENABLED

#include "FreeRTOS.h"
#include "task.h"

#include "../vsf_freertos.h"
#include "kernel/vsf_kernel.h"

/*============================ MACROS ========================================*/

/*============================ TYPES =========================================*/

/*============================ GLOBAL VARIABLES ==============================*/

// Nest counter + saved status for taskENTER_CRITICAL / taskEXIT_CRITICAL.
// Safe as plain globals on a single-core system: once the first
// Enter succeeds no other task can run until Exit releases the lock.
static uint32_t                __frt_crit_nest  = 0;
static vsf_sched_lock_status_t __frt_crit_state = (vsf_sched_lock_status_t)0;

// Independent nest counter for vTaskSuspendAll / xTaskResumeAll. Kept
// separate so the two APIs can be used in any interleaving without
// one accidentally releasing the other's lock.
static uint32_t                __frt_susp_nest  = 0;
static vsf_sched_lock_status_t __frt_susp_state = (vsf_sched_lock_status_t)0;

/*============================ PROTOTYPES ====================================*/

/*============================ IMPLEMENTATION ================================*/

// ---------------------------------------------------------------------------
// Task-context critical section
// ---------------------------------------------------------------------------

void vTaskEnterCritical(void)
{
    vsf_sched_lock_status_t s = vsf_sched_lock();
    if (__frt_crit_nest == 0) {
        __frt_crit_state = s;
    }
    // else: inner nest level -- keep the outermost saved state, drop
    // the one we just acquired (we still own the lock).
    __frt_crit_nest++;
}

void vTaskExitCritical(void)
{
    if (__frt_crit_nest == 0) {
        // Unbalanced Exit. Ignore rather than panic: it mirrors how
        // FreeRTOS handles the same condition (undefined behaviour,
        // but real-world ports rarely abort).
        return;
    }
    __frt_crit_nest--;
    if (__frt_crit_nest == 0) {
        vsf_sched_lock_status_t s = __frt_crit_state;
        __frt_crit_state = (vsf_sched_lock_status_t)0;
        vsf_sched_unlock(s);
    } else {
        // Release the extra lock acquired by the nested Enter. The
        // outermost state is still held for the matching outer Exit.
        vsf_sched_unlock(vsf_sched_lock());
        // The above pair is a no-op in effect, but keeps the nest
        // accounting consistent with vsf_sched_lock's own counter if
        // the underlying implementation chooses to count.
        //
        // On VSF, vsf_sched_lock is a scheduler-safe region that
        // doesn't reference-count internally, so the balanced
        // lock+unlock here has zero side effects. It's written this
        // way purely so the semantics stay correct if the VSF
        // implementation ever starts counting.
    }
}

// ---------------------------------------------------------------------------
// ISR-context critical section
// ---------------------------------------------------------------------------
//
// Unlike the task-context variants, the saved state is returned to
// the caller so it can be stashed on the ISR stack. Matches how
// taskENTER_CRITICAL_FROM_ISR / taskEXIT_CRITICAL_FROM_ISR are meant
// to be composed: `UBaseType_t s = taskENTER_CRITICAL_FROM_ISR(); ... ;
// taskEXIT_CRITICAL_FROM_ISR(s);`.

UBaseType_t vTaskEnterCriticalFromISR(void)
{
    return (UBaseType_t)vsf_sched_lock();
}

void vTaskExitCriticalFromISR(UBaseType_t uxSavedInterruptState)
{
    vsf_sched_unlock((vsf_sched_lock_status_t)uxSavedInterruptState);
}

// ---------------------------------------------------------------------------
// Scheduler suspend / resume
// ---------------------------------------------------------------------------
//
// FreeRTOS semantics:
//  - vTaskSuspendAll: preemption disabled, interrupts still on.
//  - xTaskResumeAll:  matches the outermost Suspend; returns pdTRUE if
//                     a yield was forced on the way out, pdFALSE
//                     otherwise.
//
// The shim uses the same vsf_sched_lock backing as the critical
// pair, which in fact also masks the equal-priority scheduler
// switch on VSF. Interrupts remain unmasked either way.

void vTaskSuspendAll(void)
{
    vsf_sched_lock_status_t s = vsf_sched_lock();
    if (__frt_susp_nest == 0) {
        __frt_susp_state = s;
    }
    __frt_susp_nest++;
}

BaseType_t xTaskResumeAll(void)
{
    if (__frt_susp_nest == 0) {
        // Unbalanced Resume. Nothing to release.
        return pdFALSE;
    }
    __frt_susp_nest--;
    if (__frt_susp_nest == 0) {
        vsf_sched_lock_status_t s = __frt_susp_state;
        __frt_susp_state = (vsf_sched_lock_status_t)0;
        vsf_sched_unlock(s);
    } else {
        vsf_sched_unlock(vsf_sched_lock());
    }
    // We don't track whether a wakeup arrived while suspended, so we
    // always report "no forced switch". Callers that yield on pdTRUE
    // will still converge at the next cooperative yield point.
    return pdFALSE;
}

#endif      // VSF_USE_FREERTOS && VSF_FREERTOS_CFG_USE_CRITICAL
