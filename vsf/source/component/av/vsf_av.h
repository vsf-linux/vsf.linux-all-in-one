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

#ifndef __VSF_AV_H__
#define __VSF_AV_H__

#include "./vsf_av_cfg.h"

#if VSF_USE_AUDIO == ENABLED || VSF_USE_VIDEO == ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/*============================ INCLUDES ======================================*/

// for stdint.h
#include "utilities/vsf_utilities.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

typedef enum vk_av_control_type_t {
    // vk_av_control_value_t.sval16:
    // 32767:   127.9961db
    // ......
    // -32767:  -127.9961db
    VSF_AUDIO_CTRL_VOLUME_DB,

    // vk_av_control_value_t.uval16
    // 65535
    // 0
    VSF_AUDIO_CTRL_VOLUME_PERCENTAGE,

    // vk_av_control_value_t.enable
    VSF_AUDIO_CTRL_MUTE,
} vk_av_control_type_t;

typedef struct vk_av_control_value_t {
    union {
        void *buffer;
        uint8_t uval8;
        int8_t sval8;
        uint16_t uval16;
        int16_t sval16;
        uint32_t uval32;
        int32_t sval32;
        bool enable;
    };
} vk_av_control_value_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

#ifdef __cplusplus
}
#endif

/*============================ INCLUDES ======================================*/

#if VSF_USE_AUDIO == ENABLED
#   include "./audio/vsf_audio.h"
#endif

#endif      // VSF_USE_AUDIO || VSF_USE_VIDEO
#endif      // __VSF_AV_H__
