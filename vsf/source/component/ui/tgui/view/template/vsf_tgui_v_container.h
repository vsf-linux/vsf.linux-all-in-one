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

#ifndef __VSF_TINY_GUI_V_CONTAINER_H__
#define __VSF_TINY_GUI_V_CONTAINER_H__

/*============================ INCLUDES ======================================*/
#include "../../vsf_tgui_cfg.h"

#if     VSF_USE_TINY_GUI == ENABLED                                             \
    &&  VSF_TGUI_CFG_RENDERING_TEMPLATE_SEL == VSF_TGUI_V_TEMPLATE_EXAMPLE

#include "./vsf_tgui_v_control.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

extern fsm_rt_t vsf_tgui_container_v_init(vsf_tgui_t* gui_ptr, vsf_tgui_container_t* container_ptr);

extern
fsm_rt_t vsf_tgui_container_v_render(   vsf_tgui_t* gui_ptr,
                                        vsf_tgui_container_t* container_ptr,
                                        vsf_tgui_region_t* ptDirtyRegion,
                                        vsf_tgui_control_refresh_mode_t tMode);
extern
fsm_rt_t vsf_tgui_container_v_post_render(  vsf_tgui_t* gui_ptr,
                                            vsf_tgui_container_t* container_ptr,
                                            vsf_tgui_region_t* ptDirtyRegion,
                                            vsf_tgui_control_refresh_mode_t tMode);

extern fsm_rt_t vsf_tgui_container_v_depose(vsf_tgui_t* gui_ptr, vsf_tgui_container_t* container_ptr);
extern fsm_rt_t vsf_tgui_container_v_update(vsf_tgui_t* gui_ptr, vsf_tgui_container_t* container_ptr);

#endif

#endif
/* EOF */
