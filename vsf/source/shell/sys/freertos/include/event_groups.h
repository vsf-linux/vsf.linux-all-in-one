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

// Clean-room FreeRTOS event_groups.h shim for VSF.
//
// Backed by vsf_bmpevt_t (VSF_KERNEL_CFG_SUPPORT_BITMAP_EVENT). Each event
// group is a 32-bit bitmap; FreeRTOS documents EventBits_t as 24 bits when
// configUSE_16_BIT_TICKS is 0 with the top 8 reserved for the kernel, but
// this shim exposes the full 32-bit width since VSF owns the bitmap.
//
// Subset covered (MVP):
//   xEventGroupCreate / vEventGroupDelete
//   xEventGroupSetBits / xEventGroupClearBits / xEventGroupGetBits
//   xEventGroupSetBitsFromISR / xEventGroupClearBitsFromISR
//   xEventGroupWaitBits (AND / OR, with optional clear-on-exit)
//
// Not covered yet: xEventGroupSync, static allocation, EventGroupHandle_t
// pending the timer-task deferral that FreeRTOS uses for FromISR paths.

#ifndef __VSF_FREERTOS_EVENT_GROUPS_H__
#define __VSF_FREERTOS_EVENT_GROUPS_H__

#include "FreeRTOS.h"

#if defined(__VSF_FREERTOS_EVENT_GROUPS_CLASS_IMPLEMENT)
#   undef __VSF_FREERTOS_EVENT_GROUPS_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

typedef uint32_t                  EventBits_t;

// Forward-declared vsf_class. Layout is computed by PLOOC from the
// private_member list below so the public view carries the correct
// size/alignment while all fields remain inaccessible to consumers.
vsf_dcl_class(StaticEventGroup_t)
typedef StaticEventGroup_t *      EventGroupHandle_t;

vsf_class(StaticEventGroup_t) {
    private_member(
        vsf_bmpevt_t    bmp;
        bool            is_static;
    )
};

/*============================ PROTOTYPES ====================================*/

// Allocate an event group with all bits cleared. NULL on heap exhaustion.
extern EventGroupHandle_t xEventGroupCreate(void);

// Zero-heap variant. The caller owns the buffer; vEventGroupDelete
// will cancel waiters but will NOT release the storage.
extern EventGroupHandle_t xEventGroupCreateStatic(
        StaticEventGroup_t *pxEventGroupBuffer);

// Free an event group. Waiters are cancelled before storage is released.
extern void               vEventGroupDelete(EventGroupHandle_t xEventGroup);

// Atomically set bits and return the bits in the group after the set
// operation. Pending waiters whose criteria are met are released.
extern EventBits_t        xEventGroupSetBits(EventGroupHandle_t xEventGroup,
                                             const EventBits_t uxBitsToSet);

// ISR-safe variant. FreeRTOS internally defers set to the timer task so it
// can run from a hard ISR; this shim forwards directly to the set routine
// (which already performs its own protect-region locking). The
// pxHigherPriorityTaskWoken output is informational best-effort.
// Returns pdPASS on success, pdFAIL on invalid args.
extern BaseType_t         xEventGroupSetBitsFromISR(EventGroupHandle_t xEventGroup,
                                                    const EventBits_t uxBitsToSet,
                                                    BaseType_t *pxHigherPriorityTaskWoken);

// Clear bits and return the bits in the group BEFORE the clear operation.
extern EventBits_t        xEventGroupClearBits(EventGroupHandle_t xEventGroup,
                                               const EventBits_t uxBitsToClear);

// ISR-safe clear. FreeRTOS defers this to the timer task; we forward it
// directly because vsf_eda_bmpevt_reset is already protect-region safe.
// Returns pdPASS on success, pdFAIL on invalid args.
extern BaseType_t         xEventGroupClearBitsFromISR(EventGroupHandle_t xEventGroup,
                                                      const EventBits_t uxBitsToClear);

// Read-only sample of the current bits without blocking.
extern EventBits_t        xEventGroupGetBits(EventGroupHandle_t xEventGroup);
extern EventBits_t        xEventGroupGetBitsFromISR(EventGroupHandle_t xEventGroup);

// Wait for bits. If xWaitForAllBits == pdTRUE the caller blocks until ALL
// requested bits are set; otherwise any single requested bit suffices.
// Returns the bits snapshot at the time the wait completes (or on timeout).
// If xClearOnExit == pdTRUE the matched bits are cleared atomically on
// successful return. 0 ticks implies non-blocking poll.
extern EventBits_t        xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                              const EventBits_t uxBitsToWaitFor,
                                              const BaseType_t xClearOnExit,
                                              const BaseType_t xWaitForAllBits,
                                              TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_EVENT_GROUPS_H__
