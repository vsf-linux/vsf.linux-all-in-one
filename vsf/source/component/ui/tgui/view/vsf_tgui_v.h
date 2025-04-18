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

/****************************************************************************
*  Copyright 2020 by Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)    *
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

#ifndef __VSF_TINY_GUI_V_H__
#define __VSF_TINY_GUI_V_H__

/*============================ INCLUDES ======================================*/

#include "../vsf_tgui_cfg.h"

#if VSF_USE_TINY_GUI == ENABLED
#include "../controls/vsf_tgui_controls.h"

#if VSF_TGUI_CFG_RENDERING_TEMPLATE_SEL == VSF_TGUI_V_TEMPLATE_SIMPLE_VIEW
#   define VSF_TGUI_V_TEMPLATE_HEADER_FILE      "./simple_view/vsf_tgui_v_template.h"
#elif VSF_TGUI_CFG_RENDERING_TEMPLATE_SEL == VSF_TGUI_V_TEMPLATE_SCGUI_VIEW
#   define VSF_TGUI_V_TEMPLATE_HEADER_FILE      "./scgui_view/vsf_tgui_v_template.h"
#else
#   undef VSF_TGUI_CFG_RENDERING_TEMPLATE_SEL
#   define VSF_TGUI_CFG_RENDERING_TEMPLATE_SEL  VSF_TGUI_V_TEMPLATE_EXAMPLE
#   define VSF_TGUI_V_TEMPLATE_HEADER_FILE      "./template/vsf_tgui_v_template.h"
#endif

#include VSF_TGUI_V_TEMPLATE_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/

// define unsupported view functions
#ifndef tgui_v_tile_trans_rate
#   define tgui_v_tile_trans_rate(...)          .dummy_bits = 0
#endif
#ifndef tgui_v_show_corner_tile
#   define tgui_v_show_corner_tile(...)         .dummy_bits = 0
#endif
#ifndef tgui_v_background_color
#   define tgui_v_background_color(...)         .dummy_bits = 0
#endif
#ifndef tgui_v_font
#   define tgui_v_font(...)                     .dummy_bits = 0
#endif
#ifndef tgui_v_text_color
#   define tgui_v_text_color(...)               .dummy_bits = 0
#endif
#ifndef tgui_v_border_width
#   define tgui_v_border_width(...)             .dummy_bits = 0
#endif
#ifndef tgui_v_border_radius
#   define tgui_v_border_radius(...)            .dummy_bits = 0
#endif
#ifndef tgui_v_border_color
#   define tgui_v_border_color(...)             .dummy_bits = 0
#endif

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

/*
extern
void vsf_tgui_location_get_in_sizes_with_align( vsf_tgui_location_t* ptLocation,
                                                vsf_tgui_size_t* ptCtrlSize,
                                                vsf_tgui_size_t* ptDrawSize,
                                                vsf_tgui_align_mode_t tMode);
*/

extern
void vsf_tgui_region_update_with_align( vsf_tgui_region_t* ptDrawRegion,
                                        vsf_tgui_region_t* ptResourceRegion,
                                        vsf_tgui_align_mode_t tMode);


extern 
vsf_tgui_size_t __vk_tgui_label_v_text_get_size(vsf_tgui_label_t* ptLabel,
                                                uint16_t *phwLineCount,
                                                uint8_t *pchCharHeight);

extern
vsf_tgui_size_t __vk_tgui_label_v_get_minimal_rendering_size(vsf_tgui_label_t* ptLabel);

// font APIs for all views

extern uint8_t vsf_tgui_font_get_char_height(const uint8_t font_index);
extern uint8_t vsf_tgui_font_get_char_width(const uint8_t font_index, uint32_t char_u32);
extern void vsf_tgui_font_release_char(const uint8_t font_index, uint32_t char_u32, void *bitmap);
extern void * vsf_tgui_font_get_char(const uint8_t font_index, uint32_t char_u32, vsf_tgui_region_t *char_region_ptr);

#ifdef __cplusplus
}
#endif

#endif
#endif
/* EOF */
