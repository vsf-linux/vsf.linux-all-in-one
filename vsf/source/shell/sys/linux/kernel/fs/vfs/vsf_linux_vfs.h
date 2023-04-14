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

#ifndef __VSF_LINUX_FS_VFS_INTERNAL_H__
#define __VSF_LINUX_FS_VFS_INTERNAL_H__

/*============================ INCLUDES ======================================*/

#include "../../../vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern int vsf_linux_vfs_init(void);

#ifdef __cplusplus
}
#endif

/*============================ INCLUDES ======================================*/

#if VSF_LINUX_USE_DEVFS == ENABLED
#   include "devfs/vsf_linux_devfs.h"
#endif

#endif      // VSF_USE_LINUX && (VSF_LINUX_USE_DEVFS)
#endif      // __VSF_LINUX_FS_VFS_INTERNAL_H__
/* EOF */