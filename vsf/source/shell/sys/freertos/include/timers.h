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

// Clean-room FreeRTOS timers.h shim for VSF.
//
// Model:
//  - Each software timer is backed by a vsf_callback_timer_t whose
//    on_timer hook posts the timer pointer to a shared service queue.
//  - A single "timer daemon task" (lazily spawned on first create) drains
//    that queue and invokes the user-supplied TimerCallbackFunction_t in
//    a regular vsf_thread context, so callbacks may call blocking APIs.
//  - Auto-reload timers are re-armed by the daemon after the callback
//    returns, honouring any xTimerChangePeriod / xTimerStop made during
//    the callback body.

#ifndef __VSF_FREERTOS_TIMERS_H__
#define __VSF_FREERTOS_TIMERS_H__

#include "FreeRTOS.h"

#if defined(__VSF_FREERTOS_TIMERS_CLASS_IMPLEMENT)
#   undef __VSF_FREERTOS_TIMERS_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

// Forward-declared vsf_class. StaticTimer_t is both the caller-visible
// opaque-but-correctly-sized backing buffer and the internal control
// block; TimerHandle_t is an alias pointer into it.
vsf_dcl_class(StaticTimer_t)
typedef StaticTimer_t *         TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);

vsf_class(StaticTimer_t) {
    private_member(
        // vsf_callback_timer_t MUST stay first: __frt_timer_on_cb
        // upcasts from vsf_callback_timer_t * back to StaticTimer_t *
        // through a plain cast (offset 0).
        vsf_callback_timer_t    cb_timer;
        TickType_t              period_ticks;
        UBaseType_t             auto_reload;
        void *                  pvTimerID;
        TimerCallbackFunction_t pxCallbackFunction;
        const char *            name;
        bool                    is_active;
        bool                    is_static;
    )
};

/*============================ API ===========================================*/

// Creates a software timer in the "dormant" state. The caller must then
// call xTimerStart to actually arm it. uxAutoReload == pdTRUE selects a
// periodic timer; pdFALSE selects one-shot.
// pxCallbackFunction runs in the timer daemon task context.
// Returns NULL on allocation failure.
extern TimerHandle_t xTimerCreate(const char * const pcTimerName,
                                  const TickType_t xTimerPeriodInTicks,
                                  const UBaseType_t uxAutoReload,
                                  void * const pvTimerID,
                                  TimerCallbackFunction_t pxCallbackFunction);

// Zero-heap variant. The caller owns pxTimerBuffer; xTimerDelete will
// stop the timer but will NOT release the buffer.
extern TimerHandle_t xTimerCreateStatic(const char * const pcTimerName,
                                        const TickType_t xTimerPeriodInTicks,
                                        const UBaseType_t uxAutoReload,
                                        void * const pvTimerID,
                                        TimerCallbackFunction_t pxCallbackFunction,
                                        StaticTimer_t *pxTimerBuffer);

// Start the timer (or re-arm it from the current tick). xTicksToWait is
// ignored (there is no command queue to block on in this shim).
// Returns pdPASS.
extern BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
extern BaseType_t xTimerStartFromISR(TimerHandle_t xTimer,
                                     BaseType_t *pxHigherPriorityTaskWoken);

// Stop an armed timer. Already-stopped timers are a no-op.
extern BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
extern BaseType_t xTimerStopFromISR(TimerHandle_t xTimer,
                                    BaseType_t *pxHigherPriorityTaskWoken);

// Stop + start with the same period. Auto-reload and ID are preserved.
extern BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);
extern BaseType_t xTimerResetFromISR(TimerHandle_t xTimer,
                                     BaseType_t *pxHigherPriorityTaskWoken);

// Update the period and re-arm from the current tick. If the timer was
// not previously running, it is started.
extern BaseType_t xTimerChangePeriod(TimerHandle_t xTimer,
                                     TickType_t xNewPeriod,
                                     TickType_t xTicksToWait);
extern BaseType_t xTimerChangePeriodFromISR(TimerHandle_t xTimer,
                                            TickType_t xNewPeriod,
                                            BaseType_t *pxHigherPriorityTaskWoken);

// Stop (if running) and free the timer control block.
extern BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t xTicksToWait);

// Queries.
extern BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer);
extern TickType_t xTimerGetPeriod(TimerHandle_t xTimer);
extern void *     pvTimerGetTimerID(const TimerHandle_t xTimer);
extern void       vTimerSetTimerID(TimerHandle_t xTimer, void *pvNewID);
extern const char *pcTimerGetName(TimerHandle_t xTimer);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_TIMERS_H__
