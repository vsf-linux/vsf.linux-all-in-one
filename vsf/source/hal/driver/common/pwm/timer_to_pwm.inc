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

/*
 * timer_to_pwm.inc — reusable hw PWM driver that maps onto a vsf_timer_t.
 *
 * Usage (from a chip's pwm.c):
 *
 *   #include "hal/vsf_hal.h"
 *   #include "hal/driver/common/pwm/timer_to_pwm.h"
 *
 *   // optional: override per-instance mapping before including the .inc
 *   // #define VSF_PWM_CFG_IMP_LV0(__IDX, __HAL_OP) \
 *   //     vsf_hw_pwm_t vsf_hw_pwm##__IDX = {                      \
 *   //         .timer = (vsf_timer_t *)&vsf_hw_timer##__IDX,       \
 *   //         __HAL_OP                                            \
 *   //     };
 *
 *   #include "hal/driver/common/pwm/timer_to_pwm.inc"
 *
 * Required macros defined by the caller:
 *   VSF_HW_PWM_MASK        — bitmask of PWM instances to generate (e.g. 0xFF)
 *   VSF_HW_PWM_CFG_MULTI_CLASS — optional, defaults to VSF_PWM_CFG_MULTI_CLASS
 *
 * Model:
 *   One vsf_hw_pwm_t == one underlying timer (i.e. one PWM controller).
 *   Multi-channel PWM is expressed through the `channel` argument of
 *   vsf_pwm_set(); there is no compile-time binding between an instance
 *   and a single channel.
 *
 * Constraints:
 *   - Every selected PWM index __IDX must have a vsf_hw_timer##__IDX instance
 *     available (unless VSF_PWM_CFG_IMP_LV0 is overridden).
 *   - VSF_TIMER_CFG_MULTI_CLASS must be ENABLED, because the glue calls the
 *     dispatcher-side vsf_timer_* APIs through vsf_timer_t *.
 *   - The underlying timer driver must report vsf_timer_capability_t.support_pwm
 *     = 1 and accept VSF_TIMER_CHANNEL_MODE_PWM in channel_config.
 *   - Timer hardware has a single period register shared by all channels, so
 *     all channels active on the same vsf_hw_pwm_t MUST share the same period
 *     (i.e. same PWM frequency). A pwm_set() attempting to change period while
 *     another channel is already active on a different period returns
 *     VSF_ERR_NOT_SUPPORT and trips a VSF_HAL_ASSERT. Per-channel pulse
 *     (duty cycle) is independent and fully supported.
 *   - Not ISR-safe by itself: if an application calls these APIs from
 *     different contexts on the same instance, mutual exclusion is the
 *     application's responsibility.
 *
 * Call flow semantics:
 *   pwm_init()       → timer_init(period=0, min_freq=cfg.min_freq)
 *                      PWM output stays low until the first pwm_set().
 *   pwm_set(ch,p,d)  → validates period against any peer channels, then
 *                      timer_set_period() (only if changed) +
 *                      timer_channel_config(ch, PWM, pulse). If previously
 *                      enabled, auto-starts the channel.
 *   pwm_enable()     → starts every channel in configured_mask.
 *                      Channels configured after enable() are auto-started
 *                      inside pwm_set().
 *   pwm_disable()    → stops every channel in configured_mask; clears
 *                      the enabled latch.
 *   pwm_fini()       → stops every configured channel, then timer_fini().
 */

#if VSF_HAL_USE_PWM == ENABLED && VSF_HAL_USE_TIMER == ENABLED

/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/

#ifndef VSF_PWM_CFG_IMP_PREFIX
#   define VSF_PWM_CFG_IMP_PREFIX               vsf_hw
#endif
#ifndef VSF_PWM_CFG_IMP_UPCASE_PREFIX
#   define VSF_PWM_CFG_IMP_UPCASE_PREFIX        VSF_HW
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

vsf_err_t vsf_hw_pwm_init(vsf_hw_pwm_t *hw_pwm_ptr, vsf_pwm_cfg_t *cfg_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != cfg_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);

    hw_pwm_ptr->cfg             = *cfg_ptr;
    hw_pwm_ptr->enabled         = false;
    hw_pwm_ptr->configured_mask = 0;
    hw_pwm_ptr->period          = 0;

    // Period is unknown until pwm_set(); the timer driver treats period==0
    // as "don't start yet" and re-arms on the next set_period().
    vsf_timer_cfg_t timer_cfg = {
        .min_freq = cfg_ptr->min_freq,
        .period   = 0,
    };

    return vsf_timer_init(hw_pwm_ptr->timer, &timer_cfg);
}

void vsf_hw_pwm_fini(vsf_hw_pwm_t *hw_pwm_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);

    // Stop every channel that was configured, then tear down the timer.
    for (uint8_t ch = 0; ch < 32; ch++) {
        if (hw_pwm_ptr->configured_mask & (1u << ch)) {
            (void)vsf_timer_channel_stop(hw_pwm_ptr->timer, ch);
        }
    }
    vsf_timer_fini(hw_pwm_ptr->timer);

    hw_pwm_ptr->enabled         = false;
    hw_pwm_ptr->configured_mask = 0;
    hw_pwm_ptr->period          = 0;
}

fsm_rt_t vsf_hw_pwm_enable(vsf_hw_pwm_t *hw_pwm_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);

    hw_pwm_ptr->enabled = true;

    // Arm every channel that was already configured by a prior pwm_set().
    // Channels configured after this point will be auto-started inside
    // pwm_set() because it observes the latched .enabled flag.
    for (uint8_t ch = 0; ch < 32; ch++) {
        if (hw_pwm_ptr->configured_mask & (1u << ch)) {
            (void)vsf_timer_channel_start(hw_pwm_ptr->timer, ch);
        }
    }

    return fsm_rt_cpl;
}

fsm_rt_t vsf_hw_pwm_disable(vsf_hw_pwm_t *hw_pwm_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);

    hw_pwm_ptr->enabled = false;

    for (uint8_t ch = 0; ch < 32; ch++) {
        if (hw_pwm_ptr->configured_mask & (1u << ch)) {
            (void)vsf_timer_channel_stop(hw_pwm_ptr->timer, ch);
        }
    }

    return fsm_rt_cpl;
}

vsf_err_t vsf_hw_pwm_set(vsf_hw_pwm_t *hw_pwm_ptr, uint8_t channel,
                         uint32_t period, uint32_t pulse)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);
    VSF_HAL_ASSERT(pulse <= period);
    VSF_HAL_ASSERT(channel < 32);

    vsf_err_t err;
    uint32_t  ch_bit = 1u << channel;
    uint32_t  peers  = hw_pwm_ptr->configured_mask & ~ch_bit;

    // Timer hardware has a single period register shared by all channels.
    // Changing period is allowed only when the timer is unlocked, when this
    // is the first configuration, or when this channel is the sole active
    // user of the timer (peers == 0). Otherwise the request would silently
    // alter the frequency of the peer channels, which is hardware-impossible
    // to isolate on a shared counter — reject it explicitly.
    if ((hw_pwm_ptr->period != 0) && (peers != 0)
    &&  (hw_pwm_ptr->period != period)) {
        VSF_HAL_ASSERT(false);
        return VSF_ERR_NOT_SUPPORT;
    }

    // Stop first so the mode/period transition does not glitch a running PWM.
    err = vsf_timer_channel_stop(hw_pwm_ptr->timer, channel);
    if (err != VSF_ERR_NONE) {
        return err;
    }

    if (hw_pwm_ptr->period != period) {
        err = vsf_timer_set_period(hw_pwm_ptr->timer, period);
        if (err != VSF_ERR_NONE) {
            return err;
        }
        hw_pwm_ptr->period = period;
    }

    vsf_timer_channel_cfg_t ch_cfg = {
        .mode  = VSF_TIMER_CHANNEL_MODE_PWM,
        .pulse = pulse,
    };
    err = vsf_timer_channel_config(hw_pwm_ptr->timer, channel, &ch_cfg);
    if (err != VSF_ERR_NONE) {
        return err;
    }

    hw_pwm_ptr->configured_mask |= ch_bit;

    // Honour a previous pwm_enable() that was deferred for this channel,
    // or a fresh pwm_set() on an already-enabled instance.
    if (hw_pwm_ptr->enabled) {
        err = vsf_timer_channel_start(hw_pwm_ptr->timer, channel);
    }

    return err;
}

uint32_t vsf_hw_pwm_get_freq(vsf_hw_pwm_t *hw_pwm_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr->timer);

#ifdef VSF_TIMER_CTRL_GET_FREQ
    uint32_t  freq = 0;
    vsf_err_t err  = vsf_timer_ctrl(hw_pwm_ptr->timer, VSF_TIMER_CTRL_GET_FREQ,
                                    &freq);
    if (err != VSF_ERR_NONE) {
        return 0;
    }
    return freq;
#else
    // Timer driver does not expose current tick frequency.
    // Fall back to the configured min_freq which is the best approximation
    // available for callers using vsf_pwm_set_ns/us/ms.
    return hw_pwm_ptr->cfg.min_freq;
#endif
}

vsf_pwm_capability_t vsf_hw_pwm_capability(vsf_hw_pwm_t *hw_pwm_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);

    return (vsf_pwm_capability_t) {
        .max_freq = VSF_HW_PWM_CFG_CAPABILITY_MAX_FREQ,
        .min_freq = VSF_HW_PWM_CFG_CAPABILITY_MIN_FREQ,
    };
}

vsf_err_t vsf_hw_pwm_get_configuration(vsf_hw_pwm_t *hw_pwm_ptr,
                                       vsf_pwm_cfg_t *cfg_ptr)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    VSF_HAL_ASSERT(NULL != cfg_ptr);

    *cfg_ptr = hw_pwm_ptr->cfg;
    return VSF_ERR_NONE;
}

vsf_err_t vsf_hw_pwm_ctrl(vsf_hw_pwm_t *hw_pwm_ptr, vsf_pwm_ctrl_t ctrl,
                          void *param)
{
    VSF_HAL_ASSERT(NULL != hw_pwm_ptr);
    (void)ctrl;
    (void)param;

    // No ctrl commands defined by default. Chip-specific drivers can override
    // VSF_PWM_CFG_REIMPLEMENT_API_CTRL and supply their own vsf_hw_pwm_ctrl().
    return VSF_ERR_NOT_SUPPORT;
}

/*============================ INSTANTIATION =================================*/

/*
 * Default mapping: PWMx <-> TIMx. Channel is supplied at call time via
 * vsf_pwm_set()'s channel argument. Override by defining
 * VSF_PWM_CFG_IMP_LV0 before including this .inc.
 */
#ifndef VSF_PWM_CFG_IMP_LV0
#   define VSF_PWM_CFG_IMP_LV0(__IDX, __HAL_OP)                                 \
        VSF_MCONNECT(VSF_PWM_CFG_IMP_PREFIX, _pwm_t)                            \
            VSF_MCONNECT(VSF_PWM_CFG_IMP_PREFIX, _pwm, __IDX) = {               \
            .timer = VSF_TIMER_TO_PWM_DEFAULT_TIMER(__IDX),                     \
            __HAL_OP                                                            \
        };
#endif

// All APIs are implemented above; ask pwm_template.inc NOT to emit its weak
// default bodies for capability/ctrl/get_configuration.
#define VSF_PWM_CFG_REIMPLEMENT_API_CAPABILITY          ENABLED
#define VSF_PWM_CFG_REIMPLEMENT_API_CTRL                ENABLED
#define VSF_PWM_CFG_REIMPLEMENT_API_GET_CONFIGURATION   ENABLED

#include "hal/driver/common/pwm/pwm_template.inc"

#endif  /* VSF_HAL_USE_PWM == ENABLED && VSF_HAL_USE_TIMER == ENABLED */
/* EOF */
