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
 * Clean-room re-implementation of ESP-IDF public API "driver/gpio.h".
 * Authored from ESP-IDF v5.1 public API only; no ESP-IDF source was
 * copied. The VSF port bridges onto
 *
 *     hal/driver/common/template/vsf_template_gpio.h
 *
 * via the global vsf_hw_io_mapper (hal/utilities/io_mapper).
 *
 * Pin numbering contract (shim-defined, deterministic across MCUs):
 *
 *     gpio_num = (port_index << VSF_HW_IO_MAPPER_PORT_BITS_LOG2)
 *                | pin_index_within_port;
 *
 * With the default VSF_HW_IO_MAPPER_PORT_BITS_LOG2 = 8 this is the
 * natural "port * 256 + pin" layout. Users that need the classic
 * ESP-IDF-style symbolic names (GPIO_NUM_0 ... GPIO_NUM_48) may either
 * rely on the numeric enum below or define their own port/pin macros in
 * the board configuration.
 */

#ifndef __VSF_ESPIDF_DRIVER_GPIO_H__
#define __VSF_ESPIDF_DRIVER_GPIO_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

/**
 * @brief GPIO pin number (flat encoding, see file comment).
 *
 * A negative value denotes "not assigned".
 */
typedef int32_t gpio_num_t;

#define GPIO_NUM_NC     (-1)

/**
 * @brief Pin direction.
 */
typedef enum {
    GPIO_MODE_DISABLE       = 0,
    GPIO_MODE_INPUT         = 1,
    GPIO_MODE_OUTPUT        = 2,
    GPIO_MODE_OUTPUT_OD     = 3,    /*!< open-drain output                */
    GPIO_MODE_INPUT_OUTPUT_OD = 4,  /*!< open-drain IO -- shim maps to OD */
    GPIO_MODE_INPUT_OUTPUT  = 5,    /*!< push-pull IO -- shim maps to PP  */
} gpio_mode_t;

/**
 * @brief Pull-resistor configuration.
 */
typedef enum {
    GPIO_PULLUP_DISABLE = 0,
    GPIO_PULLUP_ENABLE  = 1,
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_PULLDOWN_ENABLE  = 1,
} gpio_pulldown_t;

typedef enum {
    GPIO_FLOATING        = 0,       /*!< no internal pull                   */
    GPIO_PULLUP_ONLY     = 1,
    GPIO_PULLDOWN_ONLY   = 2,
    GPIO_PULLUP_PULLDOWN = 3,       /*!< both; shim collapses to FLOATING   */
} gpio_pull_mode_t;

/**
 * @brief External interrupt trigger type.
 */
typedef enum {
    GPIO_INTR_DISABLE    = 0,
    GPIO_INTR_POSEDGE    = 1,
    GPIO_INTR_NEGEDGE    = 2,
    GPIO_INTR_ANYEDGE    = 3,
    GPIO_INTR_LOW_LEVEL  = 4,
    GPIO_INTR_HIGH_LEVEL = 5,
} gpio_int_type_t;

/**
 * @brief Signal level passed to gpio_set_level().
 */
typedef enum {
    GPIO_LOW  = 0,
    GPIO_HIGH = 1,
} gpio_level_t;

/**
 * @brief Bit-mask of pin numbers used by gpio_config().
 *
 * Bit N corresponds to gpio_num_t == N. Callers use
 *     pin_bit_mask = 1ULL << GPIO_NUM_X
 * to include pin X.
 */
typedef uint64_t gpio_pin_bit_mask_t;

/**
 * @brief Bulk configuration structure used by gpio_config().
 */
typedef struct {
    gpio_pin_bit_mask_t pin_bit_mask;
    gpio_mode_t         mode;
    gpio_pullup_t       pull_up_en;
    gpio_pulldown_t     pull_down_en;
    gpio_int_type_t     intr_type;
} gpio_config_t;

/**
 * @brief Per-pin ISR callback handler.
 *
 * Invoked from the VSF EXTI dispatcher on the underlying HAL driver's
 * interrupt priority. ISR context constraints apply: do not call
 * blocking APIs from here.
 */
typedef void (*gpio_isr_t)(void *arg);

/*============================ PROTOTYPES ====================================*/

/**
 * @brief Apply a bulk configuration to every pin listed in
 *        config->pin_bit_mask.
 *
 * @retval ESP_OK                   success
 * @retval ESP_ERR_INVALID_ARG      config is NULL or the mask selects a
 *                                  pin that does not exist in the
 *                                  current io_mapper
 */
extern esp_err_t gpio_config(const gpio_config_t *config);

/**
 * @brief Reset a single pin to a safe post-boot state (input, no pull,
 *        no interrupt).
 */
extern esp_err_t gpio_reset_pin(gpio_num_t gpio_num);

/**
 * @brief Set the direction of a single pin.
 */
extern esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

/**
 * @brief Set the pull mode of a single pin.
 */
extern esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);

/**
 * @brief Set the output level of a single pin (0 = low, non-zero = high).
 */
extern esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/**
 * @brief Read the input level of a single pin (0 or 1).
 */
extern int gpio_get_level(gpio_num_t gpio_num);

/**
 * @brief Shortcut pull-up/pull-down enable/disable APIs.
 */
extern esp_err_t gpio_pullup_en(gpio_num_t gpio_num);
extern esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);
extern esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);
extern esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);

/**
 * @brief Configure the edge/level that generates an external interrupt
 *        on the pin. This does NOT enable the interrupt by itself --
 *        call gpio_intr_enable() afterwards.
 */
extern esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);

/**
 * @brief Per-pin interrupt arm/disarm.
 */
extern esp_err_t gpio_intr_enable(gpio_num_t gpio_num);
extern esp_err_t gpio_intr_disable(gpio_num_t gpio_num);

/**
 * @brief Install the shim's per-pin ISR dispatcher.
 *
 * Mirrors the ESP-IDF "ISR service" concept: a single central IRQ
 * handler fans out to per-pin callbacks registered via
 * gpio_isr_handler_add(). Must be called once before any
 * _add/_remove call.
 *
 * @param intr_alloc_flags accepted for source compatibility and
 *        otherwise ignored (VSF routes every EXTI through the same
 *        HAL priority level chosen by the board).
 *
 * @retval ESP_OK                 success or already installed
 * @retval ESP_ERR_NO_MEM         handler table could not be allocated
 * @retval ESP_ERR_INVALID_STATE  VSF io_mapper reports zero ports
 */
extern esp_err_t gpio_install_isr_service(int intr_alloc_flags);

/**
 * @brief Release the per-pin ISR dispatcher.
 *
 * All registered per-pin callbacks are dropped; per-port EXTI handlers
 * are disabled in the underlying HAL driver.
 */
extern void gpio_uninstall_isr_service(void);

/**
 * @brief Attach a per-pin callback under the ISR service.
 *
 * @retval ESP_OK                 success
 * @retval ESP_ERR_INVALID_STATE  the ISR service has not been installed
 * @retval ESP_ERR_INVALID_ARG    gpio_num is out of range or isr_handler
 *                                is NULL
 */
extern esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num,
                                        gpio_isr_t isr_handler,
                                        void *args);

/**
 * @brief Detach a per-pin callback previously registered with
 *        gpio_isr_handler_add().
 */
extern esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);

/*---------------------------- not supported --------------------------------*/

/**
 * @brief Not supported on the VSF shim. Returns ESP_ERR_NOT_SUPPORTED.
 *
 * The ESP-IDF "raw ISR" path bypasses the dispatcher and hands the IRQ
 * vector to a user-provided assembly-callable handler. VSF's HAL
 * interrupt surface is prio + handler_fn + per-port dispatch, so this
 * legacy path cannot be mapped cleanly.
 */
typedef void *gpio_isr_handle_t;

extern esp_err_t gpio_isr_register(void (*fn)(void *),
                                    void *arg,
                                    int intr_alloc_flags,
                                    gpio_isr_handle_t *handle);

/**
 * @brief Accepted for source compatibility. Returns ESP_ERR_NOT_SUPPORTED
 *        unless the underlying board enables the corresponding
 *        low-power feature -- which the shim cannot probe.
 */
extern esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num,
                                    gpio_int_type_t intr_type);
extern esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_ESPIDF_DRIVER_GPIO_H__
