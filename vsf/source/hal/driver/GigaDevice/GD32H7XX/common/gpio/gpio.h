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

#ifndef __HAL_DRIVER_GIGADEVICE_GD32H7XX_GPIO_H__
#define __HAL_DRIVER_GIGADEVICE_GD32H7XX_GPIO_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_GPIO == ENABLED

#include "../../__device.h"

/*\note Refer to template/README.md for usage cases.
 *      For peripheral drivers, blackbox mode is recommended but not required, reimplementation part MUST be open.
 *      For IPCore drivers, class structure, MULTI_CLASS configuration, reimplementation and class APIs should be open to user.
 *      For emulated drivers, **** No reimplementation ****.
 *
 *      Usually, there is no IPCore driver for GPIO.
 */

/*\note Includes CAN ONLY be put here. */

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

// HW
/*\note hw GPIO driver can reimplement vsf_gpio_mode_t/vsf_gpio_interrupt_mode_t.
 *      To enable reimplementation, please enable macro below:
 *          VSF_GPIO_CFG_REIMPLEMENT_TYPE_MODE for vsf_gpio_mode_t
 *      Reimplementation is used for optimization hw/IPCore drivers, reimplement the bit mask according to hw registers.
 *      *** DO NOT reimplement these in emulated drivers. ***
 */

#define VSF_GPIO_CFG_REIMPLEMENT_TYPE_MODE      ENABLED
// HW end

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

typedef enum vsf_gpio_mode_t {
    // 0..1, GPIO_CTL: INPUT(0)/OUTPUT(1)/AF(2)/ANALOG(3)
    // 2: GPIO_OMODE: PUSHPULL(0)/OPENDRAIN(1)
    VSF_GPIO_INPUT                              = (0 << 0),     //!< enable input mode
    VSF_GPIO_ANALOG                             = (3 << 0),     //!< enable analog function
    VSF_GPIO_OUTPUT_PUSH_PULL                   = (1 << 0) | (0 << 2),  //!< enable output push-pull mode
    VSF_GPIO_OUTPUT_OPEN_DRAIN                  = (1 << 0) | (1 << 2),  //!< enable output open-drain mode
    VSF_GPIO_AF                                 = (2 << 0),     //!< enable AF mode
    VSF_GPIO_AF_PUSH_PULL                       = (2 << 0) | (0 << 2),  //!< enable output push-pull mode
    VSF_GPIO_AF_OPEN_DRAIN                      = (2 << 0) | (1 << 2),  //!< enable output open-drain mode
    VSF_GPIO_EXTI                               = VSF_GPIO_INPUT,

    // 3..4, GPIO_PUD: FLOATING(0)/PULLUP(1)/PULLDOWN(2)
    VSF_GPIO_NO_PULL_UP_DOWN                    = (0 << 3),     //!< enable floating
    VSF_GPIO_PULL_UP                            = (1 << 3),     //!< enable pull-up resistor
    VSF_GPIO_PULL_DOWN                          = (2 << 3),     //!< enable pull-down resistor

    // 5..6, GPIO_OSPD: 12M(0)/60M(1)/85M(2)/100/220M(3)
    VSF_GPIO_SPEED_12MHZ                        = (0 << 5),
    VSF_GPIO_SPEED_60MHZ                        = (1 << 5),
    VSF_GPIO_SPEED_85MHZ                        = (2 << 5),
    VSF_GPIO_SPEED_100MHZ_220MHZ                = (3 << 5),
    VSF_GPIO_SPEED_MASK                         = (3 << 5),
#define VSF_GPIO_SPEED_LOW                      VSF_GPIO_SPEED_12MHZ
#define VSF_GPIO_SPEED_MEDIUM                   VSF_GPIO_SPEED_60MHZ
#define VSF_GPIO_SPEED_HIGH                     VSF_GPIO_SPEED_85MHZ
#define VSF_GPIO_SPEED_VERY_HIGH                VSF_GPIO_SPEED_100MHZ_220MHZ
#define VSF_GPIO_SPEED_MASK                     VSF_GPIO_SPEED_MASK

    // TODO: add input fileter modes

    // TODO: add exti support
    VSF_GPIO_EXTI_MODE_NONE                     = 0,
    VSF_GPIO_EXTI_MODE_LOW_LEVEL                = 1 << 16,
    VSF_GPIO_EXTI_MODE_HIGH_LEVEL               = 2 << 16,
    VSF_GPIO_EXTI_MODE_RISING                   = 3 << 16,
    VSF_GPIO_EXTI_MODE_FALLING                  = 4 << 16,
    VSF_GPIO_EXTI_MODE_RISING_FALLING           = 5 << 16,

    __VSF_HW_GPIO_MODE_ALL_BITS                 = 0x7F,
} vsf_gpio_mode_t;

typedef struct vsf_hw_gpio_t vsf_hw_gpio_t;

/*============================ INCLUDES ======================================*/
/*============================ PROTOTYPES ====================================*/

// __vsf_hw_gpio_get_regbase is used in io module to access af registers
extern uint32_t __vsf_hw_gpio_get_regbase(vsf_hw_gpio_t *gpio_ptr);

#ifdef __cplusplus
}
#endif

#endif      // VSF_HAL_USE_GPIO
#endif      // __HAL_DRIVER_GIGADEVICE_GD32H7XX_GPIO_H__
/* EOF */