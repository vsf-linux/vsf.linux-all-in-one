/*****************************************************************************
 *   Copyright(C)2009-2026 by VSF Team                                       *
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
 * Port implementation for "driver/ledc.h" on VSF.
 *
 * Bridges the ESP-IDF LEDC driver onto
 *     hal/driver/common/template/vsf_template_pwm.h
 *
 * Model mapping:
 *   - One vsf_pwm_t instance per ESP-IDF ledc_mode_t (LOW / HIGH).
 *   - Inside a speed_mode, ledc_channel_t is forwarded as the
 *     vsf_pwm_set() channel index.
 *   - duty_resolution and freq_hz collapse into a single counter
 *     frequency (freq_hz << duty_resolution) passed to vsf_pwm_init().
 *     ledc_channel_config() rewrites (period, duty) on the underlying
 *     vsf_pwm_t for that channel.
 *
 * Limitations:
 *   - Multiple timers within the same speed_mode share the same
 *     underlying vsf_pwm_t counter frequency (last writer wins). Most
 *     ESP-IDF demos only use one timer per speed_mode.
 *   - Fade service, interrupts, sleep retention are not modelled.
 *
 * Resource injection: vsf_pwm_t *const *pool plus pool_count, same
 * pattern as gptimer / uart / i2c / spi_master.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_LEDC == ENABLED

#include "driver/ledc.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "hal/driver/driver.h"

#include <string.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/

typedef struct __ledc_timer_state_t {
    bool            configured;
    uint8_t         duty_resolution;   /* bits */
    uint32_t        freq_hz;
    uint32_t        period;            /* 1 << duty_resolution */
} __ledc_timer_state_t;

typedef struct __ledc_channel_state_t {
    bool            configured;
    ledc_timer_t    timer_sel;
    uint32_t        duty;              /* cached, pushed on update_duty */
    bool            stopped;
    uint32_t        last_pulse;        /* last value written via vsf_pwm_set */
} __ledc_channel_state_t;

typedef struct __ledc_mode_state_t {
    vsf_pwm_t              *hw;
    bool                    hw_inited;
    bool                    hw_enabled;
    uint32_t                cur_counter_freq;
    __ledc_timer_state_t    timers[LEDC_TIMER_MAX];
    __ledc_channel_state_t  channels[LEDC_CHANNEL_MAX];
} __ledc_mode_state_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                inited;
    uint16_t            pool_count;
    vsf_pwm_t *const   *pool;
    __ledc_mode_state_t modes[LEDC_SPEED_MODE_MAX];
} __ledc = { 0 };

/*============================ PROTOTYPES ====================================*/

extern void vsf_espidf_ledc_init(const vsf_espidf_ledc_cfg_t *cfg);

/*============================ IMPLEMENTATION ================================*/

void vsf_espidf_ledc_init(const vsf_espidf_ledc_cfg_t *cfg)
{
    if (__ledc.inited) {
        return;
    }
    __ledc.inited = true;

    if (cfg != NULL) {
        __ledc.pool       = cfg->pool;
        __ledc.pool_count = cfg->pool_count;
    }
}

static __ledc_mode_state_t * __ledc_mode_get(ledc_mode_t speed_mode)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX) {
        return NULL;
    }
    __ledc_mode_state_t *m = &__ledc.modes[speed_mode];
    if (m->hw == NULL) {
        /* bind hw lazily from pool */
        if ((__ledc.pool == NULL) || ((uint32_t)speed_mode >= __ledc.pool_count)) {
            return NULL;
        }
        m->hw = __ledc.pool[speed_mode];
        if (m->hw == NULL) {
            return NULL;
        }
    }
    return m;
}

/* Ensure the underlying PWM counter is running at `counter_freq`. Re-init
 * only when the target frequency actually changes. */
static esp_err_t __ledc_ensure_counter(__ledc_mode_state_t *m, uint32_t counter_freq)
{
    if (m->hw_inited && (m->cur_counter_freq == counter_freq)) {
        return ESP_OK;
    }

    vsf_pwm_cfg_t cfg = { 0 };
    cfg.freq = counter_freq;
    vsf_err_t err = vsf_pwm_init(m->hw, &cfg);
    if (err != VSF_ERR_NONE) {
        return ESP_FAIL;
    }
    m->hw_inited         = true;
    m->cur_counter_freq  = counter_freq;

    if (!m->hw_enabled) {
        fsm_rt_t rt;
        do {
            rt = vsf_pwm_enable(m->hw);
        } while (rt == fsm_rt_on_going);
        m->hw_enabled = true;
    }
    return ESP_OK;
}

esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf)
{
    if (timer_conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)timer_conf->speed_mode >= LEDC_SPEED_MODE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)timer_conf->timer_num >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)timer_conf->duty_resolution == 0 ||
        (uint32_t)timer_conf->duty_resolution >= LEDC_TIMER_BIT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (timer_conf->freq_hz == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    __ledc_mode_state_t *m = __ledc_mode_get(timer_conf->speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t period       = 1u << timer_conf->duty_resolution;
    uint32_t counter_freq = timer_conf->freq_hz * period;

    esp_err_t err = __ledc_ensure_counter(m, counter_freq);
    if (err != ESP_OK) {
        return err;
    }

    __ledc_timer_state_t *t = &m->timers[timer_conf->timer_num];
    t->configured     = true;
    t->duty_resolution = (uint8_t)timer_conf->duty_resolution;
    t->freq_hz        = timer_conf->freq_hz;
    t->period         = period;
    return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf)
{
    if (ledc_conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)ledc_conf->speed_mode >= LEDC_SPEED_MODE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)ledc_conf->channel >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t)ledc_conf->timer_sel >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    __ledc_mode_state_t *m = __ledc_mode_get(ledc_conf->speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_timer_state_t *t = &m->timers[ledc_conf->timer_sel];
    if (!t->configured) {
        return ESP_ERR_INVALID_STATE;
    }

    __ledc_channel_state_t *c = &m->channels[ledc_conf->channel];
    c->configured = true;
    c->timer_sel  = ledc_conf->timer_sel;
    c->duty       = ledc_conf->duty;
    c->stopped    = false;

    /* Clamp duty to [0, period]. */
    uint32_t pulse = c->duty;
    if (pulse > t->period) {
        pulse = t->period;
    }
    c->last_pulse = pulse;

    vsf_err_t err = vsf_pwm_set(m->hw, (uint8_t)ledc_conf->channel, t->period, pulse);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)channel    >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_channel_state_t *c = &m->channels[channel];
    if (!c->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    c->duty = duty;
    return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)channel    >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_channel_state_t *c = &m->channels[channel];
    if (!c->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_timer_state_t *t = &m->timers[c->timer_sel];
    if (!t->configured) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t pulse = c->stopped ? 0 : c->duty;
    if (pulse > t->period) {
        pulse = t->period;
    }
    c->last_pulse = pulse;

    vsf_err_t err = vsf_pwm_set(m->hw, (uint8_t)channel, t->period, pulse);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t ledc_set_duty_with_hpoint(ledc_mode_t speed_mode, ledc_channel_t channel,
                                    uint32_t duty, uint32_t hpoint)
{
    (void)hpoint;   /* hpoint offset not modelled. */
    esp_err_t err = ledc_set_duty(speed_mode, channel, duty);
    if (err != ESP_OK) {
        return err;
    }
    return ledc_update_duty(speed_mode, channel);
}

uint32_t ledc_get_duty(ledc_mode_t speed_mode, ledc_channel_t channel)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)channel    >= LEDC_CHANNEL_MAX) {
        return (uint32_t)-1;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return (uint32_t)-1;
    }
    return m->channels[channel].duty;
}

int ledc_get_hpoint(ledc_mode_t speed_mode, ledc_channel_t channel)
{
    (void)speed_mode;
    (void)channel;
    return 0;
}

esp_err_t ledc_set_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num, uint32_t freq_hz)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)timer_num  >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (freq_hz == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_timer_state_t *t = &m->timers[timer_num];
    if (!t->configured) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t counter_freq = freq_hz * t->period;
    esp_err_t err = __ledc_ensure_counter(m, counter_freq);
    if (err != ESP_OK) {
        return err;
    }
    t->freq_hz = freq_hz;

    /* Re-push currently bound channels so their period/pulse stay consistent. */
    for (int ch = 0; ch < LEDC_CHANNEL_MAX; ch++) {
        __ledc_channel_state_t *c = &m->channels[ch];
        if (c->configured && (c->timer_sel == timer_num)) {
            uint32_t pulse = c->stopped ? 0 : c->duty;
            if (pulse > t->period) {
                pulse = t->period;
            }
            c->last_pulse = pulse;
            (void)vsf_pwm_set(m->hw, (uint8_t)ch, t->period, pulse);
        }
    }
    return ESP_OK;
}

uint32_t ledc_get_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)timer_num  >= LEDC_TIMER_MAX) {
        return 0;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return 0;
    }
    __ledc_timer_state_t *t = &m->timers[timer_num];
    return t->configured ? t->freq_hz : 0;
}

esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)channel    >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_channel_state_t *c = &m->channels[channel];
    if (!c->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_timer_state_t *t = &m->timers[c->timer_sel];
    if (!t->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    c->stopped = true;

    /* idle_level == 0 -> pulse 0; idle_level != 0 -> pulse == period. */
    uint32_t pulse = (idle_level != 0) ? t->period : 0u;
    c->last_pulse = pulse;
    (void)vsf_pwm_set(m->hw, (uint8_t)channel, t->period, pulse);
    return ESP_OK;
}

esp_err_t ledc_timer_pause(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)timer_num  >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    for (int ch = 0; ch < LEDC_CHANNEL_MAX; ch++) {
        __ledc_channel_state_t *c = &m->channels[ch];
        if (c->configured && (c->timer_sel == timer_num)) {
            c->stopped = true;
            __ledc_timer_state_t *t = &m->timers[timer_num];
            if (t->configured) {
                (void)vsf_pwm_set(m->hw, (uint8_t)ch, t->period, 0);
            }
        }
    }
    return ESP_OK;
}

esp_err_t ledc_timer_resume(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)timer_num  >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_timer_state_t *t = &m->timers[timer_num];
    if (!t->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    for (int ch = 0; ch < LEDC_CHANNEL_MAX; ch++) {
        __ledc_channel_state_t *c = &m->channels[ch];
        if (c->configured && (c->timer_sel == timer_num)) {
            c->stopped = false;
            uint32_t pulse = (c->duty > t->period) ? t->period : c->duty;
            c->last_pulse = pulse;
            (void)vsf_pwm_set(m->hw, (uint8_t)ch, t->period, pulse);
        }
    }
    return ESP_OK;
}

esp_err_t ledc_timer_rst(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    (void)speed_mode;
    (void)timer_num;
    return ESP_OK;
}

esp_err_t ledc_bind_channel_timer(ledc_mode_t speed_mode, ledc_channel_t channel,
                                  ledc_timer_t timer_sel)
{
    if ((uint32_t)speed_mode >= LEDC_SPEED_MODE_MAX ||
        (uint32_t)channel    >= LEDC_CHANNEL_MAX   ||
        (uint32_t)timer_sel  >= LEDC_TIMER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    __ledc_mode_state_t *m = __ledc_mode_get(speed_mode);
    if (m == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    __ledc_channel_state_t *c = &m->channels[channel];
    if (!c->configured) {
        return ESP_ERR_INVALID_STATE;
    }
    c->timer_sel = timer_sel;
    return ledc_update_duty(speed_mode, channel);
}

esp_err_t ledc_fade_func_install(int intr_alloc_flags)
{
    (void)intr_alloc_flags;
    return ESP_ERR_NOT_SUPPORTED;
}

void ledc_fade_func_uninstall(void)
{
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_LEDC */
