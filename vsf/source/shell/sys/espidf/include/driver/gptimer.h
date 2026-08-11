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
 * Clean-room re-implementation of ESP-IDF public API "driver/gptimer.h"
 * plus its companion "driver/gptimer_types.h", collapsed into a single
 * header for the shim.
 *
 * Authored from ESP-IDF v5.1 public API only. No code copied from the
 * ESP-IDF source tree. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_timer.h
 *
 * via a caller-supplied pool of vsf_timer_t * instances (see
 *     vsf_espidf_cfg_t::gptimer
 * ).
 *
 * Notes on semantics (v5.1 public contract preserved where feasible):
 *
 *   - gptimer_new_timer() allocates a handle opaquely; the port draws
 *     one vsf_timer_t * from the injected pool. No allocation of VSF
 *     hardware is performed here.
 *   - gptimer_alarm_config_t::alarm_count is interpreted in "resolution
 *     ticks" just like in ESP-IDF: alarm fires when the internal counter
 *     reaches this value. It is wired to vsf_timer_cfg_t::period.
 *   - resolution_hz is wired to vsf_timer_cfg_t::freq (min_freq). The
 *     actual tick frequency depends on the underlying hardware and may
 *     not exactly match the request; ESP-IDF applications that rely on
 *     cycle-accurate timing should cross-check using the counter value
 *     returned by gptimer_get_raw_count() -- which, when the VSF backend
 *     cannot expose it, returns ESP_ERR_NOT_SUPPORTED.
 *   - Clock source selection is accepted but ignored: VSF's timer
 *     abstraction does not surface per-peripheral clock muxing.
 *   - The on_alarm callback's boolean return value is accepted but
 *     ignored -- there is no ISR-to-task yield semantics in the shim.
 */

#ifndef __VSF_ESPIDF_DRIVER_GPTIMER_H__
#define __VSF_ESPIDF_DRIVER_GPTIMER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/**
 * @brief Opaque handle for a general-purpose timer.
 */
typedef struct gptimer_t *gptimer_handle_t;

/**
 * @brief Timer clock source.
 *
 * The shim accepts every value for source compatibility but ignores the
 * selection: VSF's vsf_timer_t does not expose per-peripheral clock
 * muxing. Whichever source is wired up by the underlying hardware port
 * is used.
 */
typedef enum {
    GPTIMER_CLK_SRC_DEFAULT = 0,
    GPTIMER_CLK_SRC_APB     = 1,
    GPTIMER_CLK_SRC_XTAL    = 2,
    GPTIMER_CLK_SRC_RC_FAST = 3,
    GPTIMER_CLK_SRC_PLL_F40M= 4,
    GPTIMER_CLK_SRC_PLL_F80M= 5,
} gptimer_clock_source_t;

/**
 * @brief Count direction.
 *
 * Only GPTIMER_COUNT_UP is supported by the VSF backend. Passing
 * GPTIMER_COUNT_DOWN to gptimer_new_timer() yields ESP_ERR_NOT_SUPPORTED.
 */
typedef enum {
    GPTIMER_COUNT_DOWN = 0,
    GPTIMER_COUNT_UP   = 1,
} gptimer_count_direction_t;

/**
 * @brief Initial configuration for gptimer_new_timer().
 *
 * Only the fields the shim actually consumes are described below; the
 * remaining flags are parsed for source compatibility.
 */
typedef struct {
    gptimer_clock_source_t    clk_src;          /*!< accepted but ignored */
    gptimer_count_direction_t direction;        /*!< must be GPTIMER_COUNT_UP */
    uint32_t                  resolution_hz;    /*!< wired to cfg.freq */
    int                       intr_priority;    /*!< 0 = default */
    struct {
        uint32_t intr_shared : 1;               /*!< accepted, no-op */
        uint32_t backup_before_sleep : 1;       /*!< accepted, no-op */
    } flags;
} gptimer_config_t;

/**
 * @brief Data passed to gptimer_alarm_cb_t at each alarm event.
 *
 * count_value / alarm_value reflect the counter state sampled close to
 * the alarm. When the VSF backend cannot sample the counter, both
 * fields are filled with 0.
 */
typedef struct {
    uint64_t count_value;   /*!< counter at the moment the IRQ was taken */
    uint64_t alarm_value;   /*!< alarm_count that triggered the callback  */
} gptimer_alarm_event_data_t;

/**
 * @brief User callback invoked from the alarm ISR.
 *
 * The return value is interpreted by ESP-IDF as "yield to a woken task";
 * the VSF shim always ignores it.
 */
typedef bool (*gptimer_alarm_cb_t)(gptimer_handle_t timer,
                                    const gptimer_alarm_event_data_t *edata,
                                    void *user_ctx);

/**
 * @brief Event callback set attachable via gptimer_register_event_callbacks().
 *
 * Only on_alarm is meaningful in this shim; future callback hooks that
 * may be added upstream will be accepted and ignored until the backend
 * grows the corresponding capability.
 */
typedef struct {
    gptimer_alarm_cb_t on_alarm;
} gptimer_event_callbacks_t;

/**
 * @brief Behaviour applied to the counter when the alarm matches.
 *
 *   - If auto_reload_on_alarm is true, the counter is reloaded to
 *     reload_count and the alarm keeps firing periodically.
 *   - Otherwise the counter keeps running (if at all) and the user is
 *     responsible for re-arming via gptimer_set_alarm_action() again.
 */
typedef struct {
    uint64_t alarm_count;
    uint64_t reload_count;
    struct {
        uint32_t auto_reload_on_alarm : 1;
    } flags;
} gptimer_alarm_config_t;

/*============================ PROTOTYPES ====================================*/

/**
 * @brief Allocate a new gptimer handle drawn from the injected VSF pool.
 *
 * @retval ESP_OK                  success
 * @retval ESP_ERR_INVALID_ARG     config / ret_timer is NULL, direction != UP
 * @retval ESP_ERR_INVALID_STATE   shim not initialised or pool exhausted
 * @retval ESP_ERR_NO_MEM          heap exhausted for handle metadata
 */
extern esp_err_t gptimer_new_timer(const gptimer_config_t *config,
                                    gptimer_handle_t *ret_timer);

/**
 * @brief Release a gptimer handle, stopping the underlying timer first.
 *
 * @retval ESP_OK                  success
 * @retval ESP_ERR_INVALID_ARG     timer is NULL
 * @retval ESP_ERR_INVALID_STATE   timer is still in the enabled state
 */
extern esp_err_t gptimer_del_timer(gptimer_handle_t timer);

/**
 * @brief Register the alarm callback (and future per-event callbacks).
 *
 * Must be called while the timer is in the disabled state -- mirroring
 * the ESP-IDF contract that forbids hot-patching callbacks.
 */
extern esp_err_t gptimer_register_event_callbacks(gptimer_handle_t timer,
                                                    const gptimer_event_callbacks_t *cbs,
                                                    void *user_data);

/**
 * @brief Configure the alarm action applied at the next alarm event.
 *
 * Passing config == NULL disables alarm generation. Otherwise the shim
 * programs the underlying vsf_timer so that an overflow IRQ fires every
 * alarm_count ticks; reload_count is honoured only when the implemented
 * backend exposes a way to pre-load the counter (currently: only
 * auto_reload_on_alarm is supported by mapping to a continuous timer,
 * other combinations fall back to continuous semantics too).
 */
extern esp_err_t gptimer_set_alarm_action(gptimer_handle_t timer,
                                            const gptimer_alarm_config_t *config);

/**
 * @brief Transition the timer into the "enabled" state.
 *
 * The counter is not yet running; call gptimer_start() to start counting.
 */
extern esp_err_t gptimer_enable(gptimer_handle_t timer);

/**
 * @brief Transition the timer out of the "enabled" state.
 */
extern esp_err_t gptimer_disable(gptimer_handle_t timer);

/**
 * @brief Start the counter. The timer must already be enabled.
 */
extern esp_err_t gptimer_start(gptimer_handle_t timer);

/**
 * @brief Stop the counter. The counter value is retained.
 */
extern esp_err_t gptimer_stop(gptimer_handle_t timer);

/**
 * @brief Set the raw counter value.
 *
 * @retval ESP_ERR_NOT_SUPPORTED   the VSF backend does not expose this
 */
extern esp_err_t gptimer_set_raw_count(gptimer_handle_t timer, uint64_t value);

/**
 * @brief Read the raw counter value.
 *
 * @retval ESP_ERR_NOT_SUPPORTED   the VSF backend does not expose this
 */
extern esp_err_t gptimer_get_raw_count(gptimer_handle_t timer, uint64_t *value);

/**
 * @brief Query the actual resolution chosen by the underlying hardware.
 *
 * May differ from the requested resolution_hz when the clock divider is
 * discrete. The shim returns the value programmed by the backend when
 * available, otherwise the originally requested value.
 */
extern esp_err_t gptimer_get_resolution(gptimer_handle_t timer,
                                         uint32_t *out_resolution);

/**
 * @brief Return the number of alarms that have fired since enable.
 *
 * @retval ESP_ERR_NOT_SUPPORTED   the VSF backend does not track this
 */
extern esp_err_t gptimer_get_captured_count(gptimer_handle_t timer,
                                             uint64_t *value);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_DRIVER_GPTIMER_H__
