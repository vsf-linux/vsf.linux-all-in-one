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

#if VSF_HAL_USE_USART == ENABLED && VSF_HAL_DISTBUS_USE_USART == ENABLED

#define __VSF_HAL_DISTBUS_USART_CLASS_IMPLEMENT
#include "../driver.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

static bool __vsf_hal_distbus_usart_msghandler(vsf_distbus_t *distbus, vsf_distbus_service_t *service, vsf_distbus_msg_t *msg);

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

static const vsf_distbus_service_info_t __vsf_hal_distbus_usart_info = {
    .mtu                = VSF_HAL_DISTBUS_CFG_MTU,
    .type               = 0,
    .addr_range         = VSF_HAL_DISTBUS_USART_CMD_ADDR_RANGE,
    .handler            = __vsf_hal_distbus_usart_msghandler,
};

/*============================ IMPLEMENTATION ================================*/

static bool __vsf_hal_distbus_usart_msghandler(vsf_distbus_t *distbus, vsf_distbus_service_t *service, vsf_distbus_msg_t *msg)
{
    vsf_hal_distbus_usart_t *usart = container_of(service, vsf_hal_distbus_usart_t, service);
    uint8_t *data = (uint8_t *)&msg->header + sizeof(msg->header);
    bool retain_msg = false;

    switch (msg->header.addr) {
    default:
        VSF_HAL_ASSERT(false);
        break;
    }
    return retain_msg;
}

uint32_t vsf_hal_distbus_usart_register_service(vsf_distbus_t *distbus, vsf_hal_distbus_usart_t *usart, void *info, uint32_t infolen)
{
    usart->distbus = distbus;
    usart->service.info = &__vsf_hal_distbus_usart_info;
    vsf_distbus_register_service(distbus, &usart->service);
    return 0;
}

#endif
