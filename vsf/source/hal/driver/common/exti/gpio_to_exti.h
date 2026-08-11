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

#ifndef __HAL_DRIVER_EXTI_GPIO_TO_EXTI_H__
#define __HAL_DRIVER_EXTI_GPIO_TO_EXTI_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_EXTI == ENABLED && VSF_HAL_USE_GPIO == ENABLED

#include "hal/driver/common/gpio/exti_gpio.h"
#include "hal/driver/common/template/vsf_template_exti.h"

/*============================ MACROS ========================================*/

// gpio_to_exti requires the underlying GPIO driver to provide a real
// (non-default) vsf_gpio_get_pin_configuration implementation, so that the
// EXTI glue layer can do read-modify-write on a pin's mode bits without
// destroying its other configuration.
#if VSF_GPIO_CFG_REIMPLEMENT_API_GET_PIN_CONFIGURATION != ENABLED
#   error "gpio_to_exti requires the underlying GPIO driver to implement vsf_gpio_get_pin_configuration (define VSF_GPIO_CFG_REIMPLEMENT_API_GET_PIN_CONFIGURATION = ENABLED)."
#endif

// Note: VSF_GPIO_EXTI_MODE_MASK (the OR of all VSF_GPIO_EXTI_MODE_* values)
// is automatically generated as an enum member by vsf_template_gpio.h, so we
// rely on it being available without a preprocessor check here.

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

// Per-channel (== per-pin) user EXTI handler slot.
// A vendor struct using gpio_to_exti.inc is expected to contain an array
// of these, one entry per pin (size == VSF_GPIO_CFG_PIN_COUNT).
typedef struct vsf_gpio_to_exti_channel_irq_t {
    vsf_exti_isr_handler_t  *handler_fn;
    void                    *target_ptr;
} vsf_gpio_to_exti_channel_irq_t;

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif  /* VSF_HAL_USE_EXTI && VSF_HAL_USE_GPIO */
#endif  /* __HAL_DRIVER_EXTI_GPIO_TO_EXTI_H__ */
/* EOF */
