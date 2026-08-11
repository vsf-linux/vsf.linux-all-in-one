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
 * Clean-room re-implementation of ESP-IDF "driver/ledc.h".
 *
 * Authored from ESP-IDF v5.1 public API only. No code copied from the
 * ESP-IDF source tree. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_pwm.h
 *
 * via a caller-supplied pool of vsf_pwm_t * instances (see
 *     vsf_espidf_cfg_t::ledc
 * ).
 *
 * Mapping (one-to-one with speed_mode):
 *   pool index 0 -> LEDC_LOW_SPEED_MODE
 *   pool index 1 -> LEDC_HIGH_SPEED_MODE (optional, some targets only)
 *
 * Each vsf_pwm_t instance is shared by all channels of that speed_mode.
 * A ledc_timer_config_t call re-initialises the vsf_pwm_t with the new
 * counter frequency (freq_hz * 2^duty_resolution). If multiple timers
 * on the same speed_mode are configured with different frequencies the
 * last configuration wins, which matches typical demo use-cases.
 *
 * Only the common duty / freq control path is implemented. Advanced
 * features (interrupt, fade service, sleep retention) are accepted at
 * the header level for source compatibility but ignored at runtime.
 */

#ifndef __VSF_ESPIDF_DRIVER_LEDC_H__
#define __VSF_ESPIDF_DRIVER_LEDC_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

/** @brief LEDC interrupt type (fade done). Accepted but ignored. */
typedef enum {
    LEDC_INTR_DISABLE = 0,
    LEDC_INTR_FADE_END,
    LEDC_INTR_MAX,
} ledc_intr_type_t;

/** @brief LEDC speed mode. */
typedef enum {
    LEDC_LOW_SPEED_MODE = 0,
    LEDC_HIGH_SPEED_MODE,
    LEDC_SPEED_MODE_MAX,
} ledc_mode_t;

/** @brief LEDC duty resolution (in bits). */
typedef enum {
    LEDC_TIMER_1_BIT  = 1,
    LEDC_TIMER_2_BIT,
    LEDC_TIMER_3_BIT,
    LEDC_TIMER_4_BIT,
    LEDC_TIMER_5_BIT,
    LEDC_TIMER_6_BIT,
    LEDC_TIMER_7_BIT,
    LEDC_TIMER_8_BIT,
    LEDC_TIMER_9_BIT,
    LEDC_TIMER_10_BIT,
    LEDC_TIMER_11_BIT,
    LEDC_TIMER_12_BIT,
    LEDC_TIMER_13_BIT,
    LEDC_TIMER_14_BIT,
    LEDC_TIMER_15_BIT,
    LEDC_TIMER_16_BIT,
    LEDC_TIMER_17_BIT,
    LEDC_TIMER_18_BIT,
    LEDC_TIMER_19_BIT,
    LEDC_TIMER_20_BIT,
    LEDC_TIMER_BIT_MAX,
} ledc_timer_bit_t;

/** @brief LEDC timer index. */
typedef enum {
    LEDC_TIMER_0 = 0,
    LEDC_TIMER_1,
    LEDC_TIMER_2,
    LEDC_TIMER_3,
    LEDC_TIMER_MAX,
} ledc_timer_t;

/** @brief LEDC channel index. */
typedef enum {
    LEDC_CHANNEL_0 = 0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2,
    LEDC_CHANNEL_3,
    LEDC_CHANNEL_4,
    LEDC_CHANNEL_5,
    LEDC_CHANNEL_6,
    LEDC_CHANNEL_7,
    LEDC_CHANNEL_MAX,
} ledc_channel_t;

/** @brief LEDC clock source. Accepted but ignored. */
typedef enum {
    LEDC_AUTO_CLK          = 0,
    LEDC_USE_APB_CLK       = 1,
    LEDC_USE_REF_TICK      = 2,
    LEDC_USE_RC_FAST_CLK   = 3,
    LEDC_USE_XTAL_CLK      = 4,
    LEDC_USE_PLL_DIV_CLK   = 5,
} ledc_clk_cfg_t;

/** @brief Fade direction. */
typedef enum {
    LEDC_DUTY_DIR_DECREASE = 0,
    LEDC_DUTY_DIR_INCREASE,
    LEDC_DUTY_DIR_MAX,
} ledc_duty_direction_t;

/** @brief Fade mode. */
typedef enum {
    LEDC_FADE_NO_WAIT  = 0,
    LEDC_FADE_WAIT_DONE,
    LEDC_FADE_MAX,
} ledc_fade_mode_t;

/*============================ TYPES =========================================*/

/**
 * @brief LEDC timer configuration.
 *
 * On this shim the (speed_mode, timer_num) pair only affects the
 * *latest* cached counter frequency of the backing vsf_pwm_t instance
 * (one vsf_pwm_t per speed_mode). Real ESP-IDF semantics where every
 * timer owns an independent frequency within the same speed_mode are
 * not modelled.
 */
typedef struct {
    ledc_mode_t         speed_mode;
    ledc_timer_bit_t    duty_resolution;
    ledc_timer_t        timer_num;
    uint32_t            freq_hz;
    ledc_clk_cfg_t      clk_cfg;
    bool                deconfigure;    /*!< v5.x: if true, release timer. Accepted but ignored. */
} ledc_timer_config_t;

/**
 * @brief LEDC channel configuration.
 *
 * gpio_num / intr_type / flags are accepted for source compatibility
 * but ignored at runtime: pin routing and interrupt priority are
 * managed by the underlying VSF HAL driver.
 */
typedef struct {
    int                 gpio_num;       /*!< accepted but ignored        */
    ledc_mode_t         speed_mode;
    ledc_channel_t      channel;
    ledc_intr_type_t    intr_type;      /*!< accepted but ignored        */
    ledc_timer_t        timer_sel;
    uint32_t            duty;
    int                 hpoint;         /*!< accepted but ignored        */
    struct {
        unsigned int output_invert: 1;  /*!< accepted but ignored        */
    } flags;
} ledc_channel_config_t;

/*============================ PROTOTYPES ====================================*/

/**
 * @brief Configure an LEDC timer (counter frequency + resolution).
 */
extern esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf);

/**
 * @brief Configure an LEDC channel (duty + timer binding).
 */
extern esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf);

/**
 * @brief Set duty cycle value for a channel.
 * @note Call ledc_update_duty() to apply.
 */
extern esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);

/**
 * @brief Apply the previously cached duty of a channel to hardware.
 */
extern esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel);

/**
 * @brief Set duty + hpoint and apply in one call (ESP-IDF v5.x helper).
 */
extern esp_err_t ledc_set_duty_with_hpoint(ledc_mode_t speed_mode, ledc_channel_t channel,
                                           uint32_t duty, uint32_t hpoint);

/**
 * @brief Get current duty cycle of a channel.
 * @return duty value, or -1 on invalid argument.
 */
extern uint32_t ledc_get_duty(ledc_mode_t speed_mode, ledc_channel_t channel);

/**
 * @brief Get hpoint of a channel. Always returns 0 on this shim.
 */
extern int ledc_get_hpoint(ledc_mode_t speed_mode, ledc_channel_t channel);

/**
 * @brief Re-program the timer frequency.
 */
extern esp_err_t ledc_set_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num, uint32_t freq_hz);

/**
 * @brief Get the configured frequency of a timer.
 * @return frequency in Hz, or 0 if not configured.
 */
extern uint32_t ledc_get_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num);

/**
 * @brief Stop a channel's output and hold it at the given idle level.
 */
extern esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level);

/**
 * @brief Pause a timer. Accepted but reduces to a channel-level stop on this shim.
 */
extern esp_err_t ledc_timer_pause(ledc_mode_t speed_mode, ledc_timer_t timer_num);

/**
 * @brief Resume a timer. Accepted but reduces to an internal re-enable on this shim.
 */
extern esp_err_t ledc_timer_resume(ledc_mode_t speed_mode, ledc_timer_t timer_num);

/**
 * @brief Reset a timer. Accepted but no-op on this shim.
 */
extern esp_err_t ledc_timer_rst(ledc_mode_t speed_mode, ledc_timer_t timer_num);

/**
 * @brief Bind a channel to a timer.
 */
extern esp_err_t ledc_bind_channel_timer(ledc_mode_t speed_mode, ledc_channel_t channel,
                                         ledc_timer_t timer_sel);

/*
 * Fade / ISR helpers below are declared for source compatibility but
 * return ESP_ERR_NOT_SUPPORTED from this shim (no fade hardware engine
 * is modelled here).
 */

/**
 * @brief Install the fade service. Returns ESP_ERR_NOT_SUPPORTED.
 */
extern esp_err_t ledc_fade_func_install(int intr_alloc_flags);

/**
 * @brief Uninstall the fade service. No-op on this shim.
 */
extern void ledc_fade_func_uninstall(void);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_DRIVER_LEDC_H__
