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

// DID: display in display
#ifndef __VSF_DISP_DID_H__
#define __VSF_DISP_DID_H__

/*============================ INCLUDES ======================================*/

#include "component/ui/vsf_ui_cfg.h"

#if VSF_USE_UI == ENABLED && VSF_DISP_USE_DID == ENABLED

#include "kernel/vsf_kernel.h"

#if     defined(__VSF_DISP_DID_CLASS_IMPLEMENT)
#   undef __VSF_DISP_DID_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

vsf_class(vk_disp_did_t) {
    public_member(
        implement(vk_disp_t)
        vk_disp_t *disp;
        vk_disp_point_t pos;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/

extern const vk_disp_drv_t vk_disp_drv_did;

/*============================ PROTOTYPES ====================================*/

#ifdef __cplusplus
}
#endif

#endif  // VSF_USE_UI
#endif  // __VSF_DISP_DID_H__
