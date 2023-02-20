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

#ifndef __HAL_DRIVER_GIGADEVICE_GD32VF103_USBD_H__
#define __HAL_DRIVER_GIGADEVICE_GD32VF103_USBD_H__

/*============================ INCLUDES ======================================*/
#include "hal/vsf_hal_cfg.h"
#include "../usb.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ INCLUDES ======================================*/
/*============================ PROTOTYPES ====================================*/

extern vsf_err_t gd32vf103_usbd_init(gd32vf103_usb_t *dc, usb_dc_ip_cfg_t *cfg);
extern void gd32vf103_usbd_fini(gd32vf103_usb_t *dc);
extern void gd32vf103_usbd_get_info(gd32vf103_usb_t *dc, usb_dc_ip_info_t *info);
extern void gd32vf103_usbd_connect(gd32vf103_usb_t *dc);
extern void gd32vf103_usbd_disconnect(gd32vf103_usb_t *dc);
extern void gd32vf103_usbd_irq(gd32vf103_usb_t *dc);

#endif
/* EOF */
