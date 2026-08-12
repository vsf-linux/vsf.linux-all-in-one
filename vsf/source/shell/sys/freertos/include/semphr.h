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

// Clean-room FreeRTOS semphr.h shim for VSF.
//
// Subset covered (MVP aligned with ESP-IDF v5.x and common middleware):
//   xSemaphoreCreateBinary          -> vsf_sem_t (init=0, max=1)
//   xSemaphoreCreateCounting        -> vsf_sem_t (init=uxInitial, max=uxMax)
//   xSemaphoreCreateMutex           -> vsf_mutex_t
//   xSemaphoreTake                  -> vsf_thread_sem_pend / mutex_enter
//   xSemaphoreGive                  -> vsf_eda_sem_post    / mutex_leave
//   xSemaphoreTakeFromISR           -> not really supported; parameter guard
//   xSemaphoreGiveFromISR           -> vsf_eda_sem_post_isr
//   vSemaphoreDelete                -> vsf_heap_free
//
// Not covered yet: recursive mutex, StaticSemaphore variants, queue-set.
// The handle is polymorphic; an internal "kind" discriminator selects
// between sem and mutex backing stores.

#ifndef __VSF_FREERTOS_SEMPHR_H__
#define __VSF_FREERTOS_SEMPHR_H__

#include "FreeRTOS.h"

// PLOOC vsf_class hook: the shim .c file that implements this module
// defines __VSF_FREERTOS_SEMPHR_CLASS_IMPLEMENT before including this
// header to see the real field layout. External (user) consumers see
// StaticSemaphore_t as a correctly-sized but opaque byte blob.
#if defined(__VSF_FREERTOS_SEMPHR_CLASS_IMPLEMENT)
#   undef __VSF_FREERTOS_SEMPHR_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

// Static-create storage. PLOOC private_member() reserves exactly
// sizeof(struct{...}) bytes for external viewers so the size stays in
// sync with the internal layout without a magic number. Deviation from
// upstream FreeRTOS: we do NOT alias StaticSemaphore_t onto StaticQueue_t
// because the shim uses distinct backing types for the two primitives.
vsf_dcl_class(StaticSemaphore_t)
typedef StaticSemaphore_t *         SemaphoreHandle_t;

vsf_class(StaticSemaphore_t) {
    private_member(
        uint8_t         kind;       // __FRT_SEM_KIND_SEM / _MUTEX
        bool            is_static;  // caller-owned storage flag
        union {
            vsf_sem_t   sem;        // binary / counting
            vsf_mutex_t mutex;      // mutex
        } u;
    )
};

/*============================ PROTOTYPES ====================================*/

// Binary semaphore. Initial count is 0; the first Give raises it to 1 and
// subsequent Gives are clamped. Mirrors FreeRTOS semantics.
extern SemaphoreHandle_t xSemaphoreCreateBinary(void);

// Counting semaphore. uxMaxCount is the ceiling; uxInitialCount is the
// initial availability. NULL is returned on invalid args / heap exhaustion.
extern SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount,
                                                  UBaseType_t uxInitialCount);

// Non-recursive mutex with priority inheritance (inherited from vsf_mutex_t).
extern SemaphoreHandle_t xSemaphoreCreateMutex(void);

// Zero-heap variants. The caller owns the storage; vSemaphoreDelete
// will NOT release the buffer. Returns NULL on invalid args.
extern SemaphoreHandle_t xSemaphoreCreateBinaryStatic(
        StaticSemaphore_t *pxSemaphoreBuffer);
extern SemaphoreHandle_t xSemaphoreCreateCountingStatic(
        UBaseType_t uxMaxCount,
        UBaseType_t uxInitialCount,
        StaticSemaphore_t *pxSemaphoreBuffer);
extern SemaphoreHandle_t xSemaphoreCreateMutexStatic(
        StaticSemaphore_t *pxSemaphoreBuffer);

// Free a semaphore / mutex. Waiters are cancelled before the storage is
// released. vSemaphoreDelete(NULL) is a no-op.
extern void              vSemaphoreDelete(SemaphoreHandle_t xSemaphore);

// Acquire. xTicksToWait == 0 means non-blocking; portMAX_DELAY means wait
// forever. Returns pdTRUE on success / pdFALSE on timeout or invalid args.
extern BaseType_t        xSemaphoreTake(SemaphoreHandle_t xSemaphore,
                                        TickType_t xTicksToWait);

// Release. For a binary semaphore the count is clamped to 1.
// Returns pdTRUE on success; pdFALSE on invalid args or (for a mutex) when
// the caller does not own the lock.
extern BaseType_t        xSemaphoreGive(SemaphoreHandle_t xSemaphore);

// ISR-context give. Mirrors xQueueSendFromISR semantics: the
// pxHigherPriorityTaskWoken output is best-effort informational.
extern BaseType_t        xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,
                                               BaseType_t *pxHigherPriorityTaskWoken);

// ISR-context take. Only the parameter-validation path is supported; the
// FreeRTOS contract calls for this to run inside an actual interrupt
// handler, which cannot block on a vsf_sync. Returns pdFAIL with
// pxHigherPriorityTaskWoken set to pdFALSE. Provided for API compatibility.
extern BaseType_t        xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore,
                                               BaseType_t *pxHigherPriorityTaskWoken);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_SEMPHR_H__
