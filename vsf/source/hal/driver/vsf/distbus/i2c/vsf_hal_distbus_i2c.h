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

#ifndef __VSF_HAL_DISTBUS_I2C_H__
#define __VSF_HAL_DISTBUS_I2C_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_I2C == ENABLED && VSF_HAL_DISTBUS_USE_I2C == ENABLED

#include "hal/driver/common/template/vsf_template_i2c.h"

#if     defined(__VSF_HAL_DISTBUS_I2C_CLASS_IMPLEMENT)
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_HAL_DISTBUS_I2C_CLASS_INHERIT__)
#   define __VSF_CLASS_INHERIT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

#ifndef VSF_HAL_DISTBUS_I2C_CFG_MULTI_CLASS
#   define VSF_HAL_DISTBUS_I2C_CFG_MULTI_CLASS      VSF_I2C_CFG_MULTI_CLASS
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

#if defined(__VSF_HAL_DISTBUS_I2C_CLASS_IMPLEMENT) || defined(__VSF_HAL_DISTBUS_I2C_CLASS_INHERIT__)
enum {
    VSF_HAL_DISTBUS_I2C_CMD_ADDR_RANGE,
};
#endif

vsf_class(vsf_hal_distbus_i2c_t) {
#if VSF_HAL_DISTBUS_I2C_CFG_MULTI_CLASS == ENABLED
    public_member(
        implement(vsf_i2c_t)
    )
#endif
    protected_member(
        vsf_distbus_service_t               service;
    )
    private_member(
        vsf_distbus_t                       *distbus;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ INCLUDES ======================================*/
/*============================ PROTOTYPES ====================================*/

extern uint32_t vsf_hal_distbus_i2c_register_service(vsf_distbus_t *distbus, vsf_hal_distbus_i2c_t *i2c, void *info, uint32_t infolen);

/*============================ INCLUDES ======================================*/

#define VSF_I2C_CFG_DEC_PREFIX              vsf_hal_distbus
#define VSF_I2C_CFG_DEC_UPCASE_PREFIX       VSF_HAL_DISTBUS
#include "hal/driver/common/i2c/i2c_template.h"

#ifdef __cplusplus
}
#endif

#undef __VSF_HAL_DISTBUS_I2C_CLASS_IMPLEMENT
#undef __VSF_HAL_DISTBUS_I2C_CLASS_INHERIT__

#endif
#endif
/* EOF */

