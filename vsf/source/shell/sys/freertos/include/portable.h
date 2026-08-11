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

// Clean-room FreeRTOS portable.h shim for VSF.
//
// FreeRTOS applications use pvPortMalloc/vPortFree as a heap. We route them
// to vsf_heap when available, otherwise to the C library malloc/free.

#ifndef __VSF_FREERTOS_PORTABLE_H__
#define __VSF_FREERTOS_PORTABLE_H__

#include "portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

void *pvPortMalloc(size_t xSize);
void  vPortFree(void *pv);
size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_PORTABLE_H__
