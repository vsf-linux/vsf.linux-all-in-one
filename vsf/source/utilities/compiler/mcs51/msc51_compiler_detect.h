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

#ifndef __MSC51_COMPILER_DETECT_H__
#define __MSC51_COMPILER_DETECT_H__

//! \name The macros to identify the compiler
//! @{

//! \note for IAR
#ifdef __IS_COMPILER_IAR__
#   undef __IS_COMPILER_IAR__
#endif
#if defined(__IAR_SYSTEMS_ICC__)
#   define __IS_COMPILER_IAR__          1
#endif


//! \note for KEIL51
#ifdef __IS_COMPILER_51_KEIL__
#   undef __IS_COMPILER_51_KEIL__
#endif
#if defined(__C51__) || defined(__CX51__)
#   define __IS_COMPILER_51_KEIL__      1
#endif
//! @}

#endif /* __USE_MCS51_COMPILER_H_PART_1__ */

#endif      // __MSC51_COMPILER_DETECT_H__