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

#ifndef __HAL_DRIVER_COMMON_PWM_TIMER_TO_PWM_H__
#define __HAL_DRIVER_COMMON_PWM_TIMER_TO_PWM_H__

/*============================ INCLUDES ======================================*/

#include "hal/vsf_hal_cfg.h"

#if VSF_HAL_USE_PWM == ENABLED && VSF_HAL_USE_TIMER == ENABLED

#include "hal/driver/common/template/vsf_template_hal_driver.h"
#include "hal/driver/common/template/vsf_template_pwm.h"
#include "hal/driver/common/template/vsf_template_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/*
 * Default MULTI_CLASS follows VSF_PWM_CFG_MULTI_CLASS.
 * When ENABLED, vsf_hw_pwm_t inherits vsf_pwm_t so it can be accessed through
 * the class dispatcher; when DISABLED, callers must use vsf_hw_pwm_* directly.
 */
#ifndef VSF_HW_PWM_CFG_MULTI_CLASS
#   define VSF_HW_PWM_CFG_MULTI_CLASS           VSF_PWM_CFG_MULTI_CLASS
#endif

/*
 * Capability defaults (Hz). Can be overridden by the hw driver before
 * including timer_to_pwm.inc.
 */
#ifndef VSF_HW_PWM_CFG_CAPABILITY_MIN_FREQ
#   define VSF_HW_PWM_CFG_CAPABILITY_MIN_FREQ   (1ul * 1000)
#endif
#ifndef VSF_HW_PWM_CFG_CAPABILITY_MAX_FREQ
#   define VSF_HW_PWM_CFG_CAPABILITY_MAX_FREQ   (100ul * 1000 * 1000)
#endif

/*============================ TYPES =========================================*/

/*
 * Generic hw PWM instance that maps every call onto a vsf_timer_t.
 *
 * One vsf_hw_pwm_t corresponds to ONE underlying timer (i.e. one PWM
 * controller). Per-channel operations are driven by the `channel` argument
 * of vsf_pwm_set(); this struct does NOT bind itself to a single channel.
 *
 * .timer:           pointer to the underlying timer class instance (cast from
 *                   the concrete vsf_hw_timer_t* on init-time). Uses
 *                   vsf_timer_t so the glue stays chip-agnostic and goes
 *                   through the dispatcher.
 *                   REQUIRES VSF_TIMER_CFG_MULTI_CLASS == ENABLED.
 * .cfg:             cached user config (for get_configuration).
 * .period:          period currently programmed on the timer (in timer ticks).
 *                   0 = not locked yet by any pwm_set(). Timer hardware has
 *                   a single period register shared by all channels, so all
 *                   active channels on this instance must share this value.
 * .configured_mask: bitmask of channels that have been configured by a
 *                   previous pwm_set(). Used by pwm_enable()/pwm_disable()
 *                   to iterate, and by pwm_set() to detect frequency
 *                   conflicts across channels.
 * .enabled:         latched state of pwm_enable()/pwm_disable(). A pwm_set()
 *                   on a new channel will auto-start it when .enabled is true.
 */
typedef struct vsf_hw_pwm_t {
#if VSF_HW_PWM_CFG_MULTI_CLASS == ENABLED
    vsf_pwm_t           vsf_pwm;
#endif
    vsf_timer_t        *timer;
    vsf_pwm_cfg_t       cfg;
    uint32_t            period;
    uint32_t            configured_mask;
    bool                enabled;
} vsf_hw_pwm_t;

/*============================ MACROFIED FUNCTIONS ===========================*/

/*
 * Default per-instance initializer used by timer_to_pwm.inc when the caller
 * does not override VSF_PWM_CFG_IMP_LV0. Assumes "PWMx <-> TIMx" one-to-one
 * mapping. Channel is not bound at compile time; it is supplied by the
 * caller via vsf_pwm_set()'s channel argument. Override
 * VSF_PWM_CFG_IMP_LV0 before including timer_to_pwm.inc if a different
 * mapping is needed.
 */
#ifndef VSF_TIMER_TO_PWM_DEFAULT_TIMER
#   define VSF_TIMER_TO_PWM_DEFAULT_TIMER(__IDX) \
        (vsf_timer_t *)&VSF_MCONNECT(vsf_hw_timer, __IDX)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* VSF_HAL_USE_PWM && VSF_HAL_USE_TIMER */
#endif  /* __HAL_DRIVER_COMMON_PWM_TIMER_TO_PWM_H__ */
/* EOF */
