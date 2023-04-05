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

#include "hal/vsf_hal_cfg.h"

/*============================ MACROS ========================================*/

/*\note first define basic info for arch. */
#if defined(__VSF_HEADER_ONLY_SHOW_ARCH_INFO__)

#ifndef VSF_ARCH_SWI_NUM
#   define VSF_ARCH_SWI_NUM                     4
#endif

#ifndef VSF_ARCH_RTOS_CFG_STACK_DEPTH
#   define VSF_ARCH_RTOS_CFG_STACK_DEPTH        4096
#endif

#define VSF_ARCH_FREERTOS_CFG_IS_IN_ISR         xPortIsInsideInterrupt
#ifndef VSF_ARCH_PROVIDE_HEAP
#   define VSF_ARCH_PROVIDE_HEAP                ENABLED
#   define VSF_ARCH_HEAP_HAS_STATISTICS         ENABLED
#endif

// there is malloc/free/realloc/calloc/memalign in bl_sdk
#define VSF_LINUX_SIMPLE_LIBC_CFG_NO_MM         ENABLED

// software interrupt provided by a dedicated device
#define VSF_DEV_SWI_NUM                         0

#ifndef __RTOS__
#   define __RTOS__
#endif
#ifndef __FREERTOS__
#   define __FREERTOS__
#endif

// alu types, should be defined in target-specified compiler header
// but xtensa uses generic compiler header, so define alu types here
typedef unsigned int                            uintalu_t;
typedef int                                     intalu_t;

#define VSF_ARCH_STACK_ALIGN_BIT                4

#else

#ifndef __HAL_DEVICE_BL_BL616_H__
#define __HAL_DEVICE_BL_BL616_H__

/*============================ INCLUDES ======================================*/

// TODO: include header from vendor here

/*\note this is should be the only place where __common.h is included.*/
#include "../common/__common.h"

/*============================ MACROS ========================================*/

#define VSF_HW_IO_PORT_COUNT            1
#define VSF_HW_IO_PIN_COUNT             35
#define VSF_HW_IO_FUNCTION_MAX          32
#define VSF_HW_IO_PORT0_DEVNAME         "gpio"

#define VSF_HW_GPIO_COUNT               VSF_HW_IO_PORT_COUNT
#define VSF_HW_GPIO_PIN_COUNT           VSF_HW_IO_PIN_COUNT

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

#endif      // __HAL_DEVICE_BL_BL616_H__
#endif      // __VSF_HEADER_ONLY_SHOW_ARCH_INFO__
/* EOF */
