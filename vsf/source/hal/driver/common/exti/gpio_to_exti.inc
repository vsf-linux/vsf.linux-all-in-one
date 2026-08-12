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

/* gpio_to_exti.inc ----------------------------------------------------------
 *
 * Glue template that builds a full EXTI driver on top of an existing GPIO
 * driver whose EXTI hardware is integrated into the GPIO block (1 EXTI
 * channel == 1 GPIO pin).
 *
 * Expected vendor setup (inside e.g. chip-level exti.c):
 *
 *     typedef struct vsf_hw_exti_t {
 *     #if VSF_EXTI_CFG_MULTI_CLASS == ENABLED
 *         implement(vsf_exti_t);
 *     #endif
 *         vsf_exti_gpio_t                exti_gpio;
 *         vsf_gpio_to_exti_channel_irq_t channel_irqs[VSF_GPIO_CFG_PIN_COUNT];
 *     } vsf_hw_exti_t;
 *
 *     #define VSF_EXTI_CFG_IMP_PREFIX          vsf_hw
 *     #define VSF_EXTI_CFG_IMP_UPCASE_PREFIX   VSF_HW
 *     #include "hal/driver/common/exti/gpio_to_exti.inc"
 *
 * This file defines all 10 vsf_<prefix>_exti_* APIs plus a per-pin trampoline,
 * and finally pulls in exti_template.inc to emit the op table and instance
 * arrays.
 * --------------------------------------------------------------------------*/

/*============================ INCLUDES ======================================*/

#include "hal/driver/driver.h"

#if VSF_HAL_USE_EXTI == ENABLED && VSF_HAL_USE_GPIO == ENABLED

#include "hal/driver/common/exti/gpio_to_exti.h"
#include "hal/driver/common/gpio/exti_gpio.h"

/*============================ MACROS ========================================*/

#if !defined(VSF_EXTI_CFG_IMP_PREFIX)
#   error "Please define VSF_EXTI_CFG_IMP_PREFIX before including gpio_to_exti.inc"
#endif

#if !defined(VSF_EXTI_CFG_IMP_UPCASE_PREFIX)
#   error "Please define VSF_EXTI_CFG_IMP_UPCASE_PREFIX before including gpio_to_exti.inc"
#endif

#define __vsf_gpio_to_exti_t                VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_t)
#define __vsf_gpio_to_exti_init             VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_init)
#define __vsf_gpio_to_exti_fini             VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_fini)
#define __vsf_gpio_to_exti_status           VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_status)
#define __vsf_gpio_to_exti_capability       VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_capability)
#define __vsf_gpio_to_exti_trigger          VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_trigger)
#define __vsf_gpio_to_exti_config_channels  VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_config_channels)
#define __vsf_gpio_to_exti_irq_enable       VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_irq_enable)
#define __vsf_gpio_to_exti_irq_disable      VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_irq_disable)
#define __vsf_gpio_to_exti_irq_clear        VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_irq_clear)
#define __vsf_gpio_to_exti_ctrl             VSF_MCONNECT(VSF_EXTI_CFG_IMP_PREFIX, _exti_ctrl)
#define __vsf_gpio_to_exti_trampoline       VSF_MCONNECT(__, VSF_EXTI_CFG_IMP_PREFIX, _gpio_to_exti_trampoline)

/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

// Distributor -> per-channel EXTI handler trampoline.
// The gpio distributor always invokes this with a pin_mask containing
// exactly one bit set (see __gpio_irq_distributor_handler in exti_gpio.c),
// so a simple scan is fine.
static void __vsf_gpio_to_exti_trampoline(void *target_ptr, vsf_gpio_t *gpio_ptr,
                                          vsf_gpio_pin_mask_t pin_mask)
{
    VSF_UNUSED_PARAM(gpio_ptr);
    VSF_HAL_ASSERT(target_ptr != NULL);
    VSF_HAL_ASSERT(pin_mask != 0);

    __vsf_gpio_to_exti_t *hw_exti_ptr = (__vsf_gpio_to_exti_t *)target_ptr;

    for (uint_fast8_t pin = 0; pin < VSF_GPIO_CFG_PIN_COUNT; pin++) {
        if (pin_mask & ((vsf_gpio_pin_mask_t)1 << pin)) {
            vsf_gpio_to_exti_channel_irq_t *slot = &hw_exti_ptr->channel_irqs[pin];
            if (slot->handler_fn != NULL) {
                slot->handler_fn(slot->target_ptr,
                                 (vsf_exti_t *)hw_exti_ptr,
                                 (vsf_exti_channel_mask_t)((vsf_gpio_pin_mask_t)1 << pin));
            }
        }
    }
}

vsf_err_t __vsf_gpio_to_exti_init(__vsf_gpio_to_exti_t *exti_ptr)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    return VSF_ERR_NONE;
}

void __vsf_gpio_to_exti_fini(__vsf_gpio_to_exti_t *exti_ptr)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    // tear down: clear every channel's EXTI mode and handler slot.
    vsf_gpio_pin_mask_t all_pins = (vsf_gpio_pin_mask_t)VSF_GPIO_CFG_PIN_MASK;
    (void)vsf_exti_gpio_config_mode(&exti_ptr->exti_gpio, all_pins, VSF_GPIO_EXTI_MODE_NONE);
    (void)vsf_exti_gpio_exti_irq_disable(&exti_ptr->exti_gpio, all_pins);
    for (uint_fast8_t pin = 0; pin < VSF_GPIO_CFG_PIN_COUNT; pin++) {
        exti_ptr->channel_irqs[pin].handler_fn = NULL;
        exti_ptr->channel_irqs[pin].target_ptr = NULL;
    }
}

vsf_exti_status_t __vsf_gpio_to_exti_status(__vsf_gpio_to_exti_t *exti_ptr)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    vsf_exti_status_t status = { 0 };
#ifdef VSF_GPIO_CTRL_EXTI_GET_STATUS
    vsf_gpio_pin_mask_t status_mask = 0;
    (void)vsf_exti_gpio_ctrl(&exti_ptr->exti_gpio,
                             VSF_GPIO_CTRL_EXTI_GET_STATUS, &status_mask);
    status.status_mask = (vsf_exti_channel_mask_t)status_mask;
#endif
    return status;
}

vsf_exti_capability_t __vsf_gpio_to_exti_capability(__vsf_gpio_to_exti_t *exti_ptr)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    vsf_exti_capability_t cap = { 0 };
    cap.support_edge_trigger = 1;
#if defined(VSF_EXTI_MODE_HIGH_LEVEL) || defined(VSF_EXTI_MODE_LOW_LEVEL)
    cap.support_level_trigger = 1;
#endif
#ifdef VSF_GPIO_CTRL_EXTI_TRIGGER
    cap.support_sw_trigger = 1;
#endif
    cap.irq_num      = 1;
    cap.channel_mask = (vsf_exti_channel_mask_t)VSF_GPIO_CFG_PIN_MASK;
    return cap;
}

vsf_err_t __vsf_gpio_to_exti_trigger(__vsf_gpio_to_exti_t *exti_ptr,
                                     vsf_exti_channel_mask_t channel_mask)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    VSF_UNUSED_PARAM(channel_mask);
#ifdef VSF_GPIO_CTRL_EXTI_TRIGGER
    vsf_gpio_pin_mask_t pin_mask = (vsf_gpio_pin_mask_t)channel_mask;
    return vsf_exti_gpio_ctrl(&exti_ptr->exti_gpio,
                              VSF_GPIO_CTRL_EXTI_TRIGGER, &pin_mask);
#else
    return VSF_ERR_NOT_SUPPORT;
#endif
}

vsf_err_t __vsf_gpio_to_exti_config_channels(__vsf_gpio_to_exti_t *exti_ptr,
                                             vsf_exti_channel_mask_t channel_mask,
                                             vsf_exti_channel_cfg_t *cfg_ptr)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    VSF_HAL_ASSERT(cfg_ptr != NULL);
    VSF_HAL_ASSERT(channel_mask != 0);

    vsf_gpio_pin_mask_t pin_mask = (vsf_gpio_pin_mask_t)channel_mask;

    // Map EXTI mode -> GPIO-embedded EXTI mode.
    vsf_gpio_mode_t gpio_mode;
    switch (cfg_ptr->mode) {
    case VSF_EXTI_MODE_NONE:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_NONE;
        break;
    case VSF_EXTI_MODE_RISING:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_RISING;
        break;
    case VSF_EXTI_MODE_FALLING:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_FALLING;
        break;
    case VSF_EXTI_MODE_RISING_FALLING:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_RISING_FALLING;
        break;
#ifdef VSF_EXTI_MODE_HIGH_LEVEL
    case VSF_EXTI_MODE_HIGH_LEVEL:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_HIGH_LEVEL;
        break;
#endif
#ifdef VSF_EXTI_MODE_LOW_LEVEL
    case VSF_EXTI_MODE_LOW_LEVEL:
        gpio_mode = (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_LOW_LEVEL;
        break;
#endif
    default:
        return VSF_ERR_INVALID_PARAMETER;
    }

    // 1. Disable EXTI on affected pins first, so no spurious interrupt fires
    //    between handler update and final mode programming.
    vsf_err_t err = vsf_exti_gpio_config_mode(&exti_ptr->exti_gpio, pin_mask,
                                              (vsf_gpio_mode_t)VSF_GPIO_EXTI_MODE_NONE);
    if (err != VSF_ERR_NONE) {
        return err;
    }

    // 2. Store per-channel user handlers into our slot array.
    for (uint_fast8_t pin = 0; pin < VSF_GPIO_CFG_PIN_COUNT; pin++) {
        if (pin_mask & ((vsf_gpio_pin_mask_t)1 << pin)) {
            exti_ptr->channel_irqs[pin].handler_fn = cfg_ptr->handler_fn;
            exti_ptr->channel_irqs[pin].target_ptr = cfg_ptr->target_ptr;
        }
    }

    // 3. Register our trampoline into the GPIO distributor for these pins.
    vsf_gpio_exti_irq_cfg_t gpio_irq_cfg = {
        .handler_fn = __vsf_gpio_to_exti_trampoline,
        .target_ptr = exti_ptr,
        .prio       = cfg_ptr->prio,
    };
    err = vsf_exti_gpio_config_pin(&exti_ptr->exti_gpio, pin_mask, &gpio_irq_cfg);
    if (err != VSF_ERR_NONE) {
        return err;
    }

    // 4. Finally program the requested EXTI mode (unless NONE, which keeps it disabled).
    if (cfg_ptr->mode != VSF_EXTI_MODE_NONE) {
        err = vsf_exti_gpio_config_mode(&exti_ptr->exti_gpio, pin_mask, gpio_mode);
    }
    return err;
}

vsf_err_t __vsf_gpio_to_exti_irq_enable(__vsf_gpio_to_exti_t *exti_ptr,
                                        vsf_exti_channel_mask_t channel_mask)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    return vsf_exti_gpio_exti_irq_enable(&exti_ptr->exti_gpio,
                                         (vsf_gpio_pin_mask_t)channel_mask);
}

vsf_err_t __vsf_gpio_to_exti_irq_disable(__vsf_gpio_to_exti_t *exti_ptr,
                                         vsf_exti_channel_mask_t channel_mask)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    return vsf_exti_gpio_exti_irq_disable(&exti_ptr->exti_gpio,
                                          (vsf_gpio_pin_mask_t)channel_mask);
}

vsf_exti_channel_mask_t __vsf_gpio_to_exti_irq_clear(__vsf_gpio_to_exti_t *exti_ptr,
                                                    vsf_exti_channel_mask_t channel_mask)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    // Pending bits on GPIO-integrated EXTI are normally cleared by the GPIO
    // driver's own ISR. The best we can do here is return the requested mask.
    VSF_UNUSED_PARAM(exti_ptr);
    return channel_mask;
}

vsf_err_t __vsf_gpio_to_exti_ctrl(__vsf_gpio_to_exti_t *exti_ptr,
                                  vsf_exti_ctrl_t ctrl, void *param)
{
    VSF_HAL_ASSERT(exti_ptr != NULL);
    VSF_UNUSED_PARAM(ctrl);
    VSF_UNUSED_PARAM(param);
    return VSF_ERR_NOT_SUPPORT;
}

/*============================ MACROS ========================================*/

// Tell exti_template.inc not to emit default irq_clear / ctrl; we already did.
#undef  VSF_EXTI_CFG_REIMPLEMENT_API_IRQ_CLEAR
#undef  VSF_EXTI_CFG_REIMPLEMENT_API_CTRL
#define VSF_EXTI_CFG_REIMPLEMENT_API_IRQ_CLEAR      ENABLED
#define VSF_EXTI_CFG_REIMPLEMENT_API_CTRL           ENABLED

#undef __vsf_gpio_to_exti_t
#undef __vsf_gpio_to_exti_init
#undef __vsf_gpio_to_exti_fini
#undef __vsf_gpio_to_exti_status
#undef __vsf_gpio_to_exti_capability
#undef __vsf_gpio_to_exti_trigger
#undef __vsf_gpio_to_exti_config_channels
#undef __vsf_gpio_to_exti_irq_enable
#undef __vsf_gpio_to_exti_irq_disable
#undef __vsf_gpio_to_exti_irq_clear
#undef __vsf_gpio_to_exti_ctrl
#undef __vsf_gpio_to_exti_trampoline

/*============================ INSTANCE GENERATION ===========================*/

#include "hal/driver/common/exti/exti_template.inc"

#endif  /* VSF_HAL_USE_EXTI && VSF_HAL_USE_GPIO */
/* EOF */
