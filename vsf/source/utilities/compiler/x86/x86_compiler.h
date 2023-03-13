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

#ifndef __USE_X86_COMPILER_H_PART_1__
#define __USE_X86_COMPILER_H_PART_1__

/*============================ INCLUDES ======================================*/

/*============================ MACROS ========================================*/

#include "./x86_compiler_detect.h"

#endif /* end of __USE_X86_COMPILER_H_PART_1__ */

/*========================== Multiple-Entry Start ============================*/

#ifndef __VSF_HEADER_ONLY_SHOW_COMPILER_INFO__
#   include "./type.h"
#endif
#include "../__common/__compiler.h"

/*========================== Multiple-Entry End ==============================*/

#ifndef __USE_X86_COMPILER_H_PART_2__
#define __USE_X86_COMPILER_H_PART_2__

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------  Start of section using anonymous unions  -------------- */

/*! \note it's safe to ignore -Wcast-align in x86 platform */
#if __IS_COMPILER_LLVM__
#   pragma clang diagnostic ignored "-Wcast-align"
#elif __IS_COMPILER_GCC__
#   pragma GCC diagnostic ignored "-Wcast-align"
#endif

/*============================ MACROS ========================================*/

/*----------------------------------------------------------------------------*
 * Startup Source Code                                                        *
 *----------------------------------------------------------------------------*/

#define vsf_stdio_init(...)

/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

#ifdef __cplusplus
}
#endif

#endif /* end of __USE_X86_COMPILER_H_PART_2__ */
