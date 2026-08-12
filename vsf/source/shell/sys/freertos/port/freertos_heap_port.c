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
 * Port implementation for FreeRTOS heap API ("portable.h") on VSF.
 *
 * FreeRTOS applications use pvPortMalloc/vPortFree as the kernel heap. We
 * route them to vsf_heap, which also powers xTaskCreate stack allocations
 * elsewhere in this shim. VSF_USE_HEAP is a hard requirement -- there is
 * no fallback to the C standard library because the FreeRTOS contract
 * expects allocations to come from the same managed heap that owns task
 * stacks / queues / semaphores.
 *
 * The *FreeHeapSize queries bridge to vsf_heap_statistics:
 *   xPortGetFreeHeapSize()             = all_size - used_size
 *   xPortGetMinimumEverFreeHeapSize()  = all_size - max_used_size
 *
 * NOTE: vsf_heap_statistics_t exposes {all_size, used_size, max_used_size}.
 * max_used_size tracks the historical peak of used_size across all alloc
 * paths, giving FreeRTOS its classic low-water "minimum ever free" metric.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_freertos_cfg.h"

#if VSF_USE_FREERTOS == ENABLED

#include "portable.h"

#if !defined(VSF_USE_HEAP) || VSF_USE_HEAP != ENABLED
#   error "vsf_freertos heap port requires VSF_USE_HEAP == ENABLED"
#endif

#include "service/heap/vsf_heap.h"

/*============================ IMPLEMENTATION ================================*/

void *pvPortMalloc(size_t xSize)
{
    return vsf_heap_malloc(xSize);
}

void vPortFree(void *pv)
{
    if (pv == NULL) {
        return;
    }
    vsf_heap_free(pv);
}

size_t xPortGetFreeHeapSize(void)
{
#if VSF_HEAP_CFG_STATISTICS == ENABLED
    vsf_heap_statistics_t stat = { 0 };
    vsf_heap_statistics(&stat);
    if (stat.all_size >= stat.used_size) {
        return (size_t)(stat.all_size - stat.used_size);
    }
    return 0;
#else
    // No statistics compiled in; documented sentinel for "unknown".
    return 0;
#endif
}

size_t xPortGetMinimumEverFreeHeapSize(void)
{
#if VSF_HEAP_CFG_STATISTICS == ENABLED
    vsf_heap_statistics_t stat = { 0 };
    vsf_heap_statistics(&stat);
    if (stat.all_size >= stat.max_used_size) {
        return (size_t)(stat.all_size - stat.max_used_size);
    }
    return 0;
#else
    return 0;
#endif
}

#endif      // VSF_USE_FREERTOS
