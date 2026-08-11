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
 * Port implementation for "driver/gpio.h" on VSF.
 *
 * Bridges the ESP-IDF GPIO API onto
 *     hal/driver/common/template/vsf_template_gpio.h
 * via the global vsf_hw_io_mapper (hal/utilities/io_mapper).
 *
 * Pin numbering contract (see include/driver/gpio.h):
 *
 *     gpio_num = (port_idx << VSF_HW_IO_MAPPER_PORT_BITS_LOG2) | pin_idx
 *
 * Unlike the gptimer port this file does NOT take a caller-supplied
 * instance pool -- vsf_hw_io_mapper is already a single, board-owned
 * global, so routing through it is unambiguous.
 *
 * ISR service model:
 *   gpio_install_isr_service() allocates a (port_num << LOG2)-entry
 *   table of per-pin callbacks and installs one shared
 *   __gpio_exti_dispatcher per port (via vsf_gpio_exti_irq_config).
 *   The dispatcher then fans out to the per-pin callback looked up
 *   from the table.
 *
 *   gpio_set_intr_type() rewrites the pin's mode to VSF_GPIO_EXTI
 *   combined with the requested edge/level; gpio_intr_enable /
 *   _disable just toggle the HAL-level irq mask.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_DRIVER_GPIO == ENABLED

#include "driver/gpio.h"

#include "hal/driver/driver.h"
#include "kernel/vsf_kernel.h"
#include "hal/utilities/io_mapper/vsf_io_mapper.h"

#if defined(VSF_USE_HEAP) && VSF_USE_HEAP == ENABLED
#   include "service/heap/vsf_heap.h"
#else
#   error "VSF_ESPIDF_CFG_USE_DRIVER_GPIO requires VSF_USE_HEAP"
#endif

#include <string.h>

#if VSF_HW_GPIO_COUNT == 0
#   error "VSF_ESPIDF_CFG_USE_DRIVER_GPIO requires VSF_HW_GPIO_COUNT > 0"
#endif

/*============================ TYPES =========================================*/

typedef struct {
    gpio_isr_t          cb;
    void               *arg;
} gpio_isr_entry_t;

/*============================ LOCAL VARIABLES ===============================*/

static struct {
    bool                installed;
    uint16_t            pins_per_port;      /*!< 1 << port_bits_log2    */
    uint16_t            total_pins;         /*!< port_num * pins_per_port */
    gpio_isr_entry_t   *table;
} __vsf_espidf_gpio = { 0 };

/*============================ PROTOTYPES ====================================*/

static void __gpio_exti_dispatcher(void *target_ptr,
                                    vsf_gpio_t *gpio_ptr,
                                    vsf_gpio_pin_mask_t pin_mask);

/*============================ HELPERS =======================================*/

// Cast helper: the concrete vsf_hw_io_mapper type is vsf_hw_io_mapper_t, but
// its in-memory layout (port_num, port_bits_log2, pin_mask, void *io[N]) is
// an exact prefix of the generic vsf_io_mapper_t (which declares io[0]).
// Casting to the generic type gives us run-time access via .port_num and the
// flexible io[] array.
static inline const vsf_io_mapper_t *__io_mapper(void)
{
    return (const vsf_io_mapper_t *)&vsf_hw_io_mapper;
}

static inline uint8_t __gpio_port_idx(gpio_num_t n)
{
    return (uint8_t)(n >> VSF_HW_IO_MAPPER_PORT_BITS_LOG2);
}

static inline uint8_t __gpio_pin_idx(gpio_num_t n)
{
    return (uint8_t)(n & ((1U << VSF_HW_IO_MAPPER_PORT_BITS_LOG2) - 1U));
}

static inline vsf_gpio_t *__gpio_hw(gpio_num_t n)
{
    const vsf_io_mapper_t *m = __io_mapper();
    uint8_t p = __gpio_port_idx(n);
    if (p >= m->port_num) {
        return NULL;
    }
    return (vsf_gpio_t *)m->io[p];
}

static inline bool __gpio_valid(gpio_num_t n)
{
    if (n < 0) {
        return false;
    }
    const vsf_io_mapper_t *m = __io_mapper();
    if ((uint32_t)n >= ((uint32_t)m->port_num << VSF_HW_IO_MAPPER_PORT_BITS_LOG2)) {
        return false;
    }
    return m->io[__gpio_port_idx(n)] != NULL;
}

/*---------------------------- mode translation -----------------------------*/

// Translate ESP-IDF mode + pull + optional EXTI trigger into a
// vsf_gpio_mode_t bag (direction | pull | exti_mode).
static vsf_gpio_mode_t __gpio_translate_mode(gpio_mode_t mode,
                                              bool pull_up_en,
                                              bool pull_down_en,
                                              gpio_int_type_t intr_type)
{
    vsf_gpio_mode_t m;

    switch (mode) {
    case GPIO_MODE_OUTPUT:
    case GPIO_MODE_INPUT_OUTPUT:
        m = VSF_GPIO_OUTPUT_PUSH_PULL;
        break;
    case GPIO_MODE_OUTPUT_OD:
    case GPIO_MODE_INPUT_OUTPUT_OD:
        m = VSF_GPIO_OUTPUT_OPEN_DRAIN;
        break;
    case GPIO_MODE_DISABLE:
        m = VSF_GPIO_ANALOG;
        break;
    case GPIO_MODE_INPUT:
    default:
        m = VSF_GPIO_INPUT;
        break;
    }

    if (pull_up_en && !pull_down_en) {
        m = (vsf_gpio_mode_t)(m | VSF_GPIO_PULL_UP);
    } else if (pull_down_en && !pull_up_en) {
        m = (vsf_gpio_mode_t)(m | VSF_GPIO_PULL_DOWN);
    } else {
        m = (vsf_gpio_mode_t)(m | VSF_GPIO_NO_PULL_UP_DOWN);
    }

    // EXTI trigger type overrides the direction bits -- the VSF HAL
    // treats VSF_GPIO_EXTI as its own mode. Only collapse to EXTI when
    // the caller actually asked for an active trigger.
    switch (intr_type) {
    case GPIO_INTR_POSEDGE:
        m = (vsf_gpio_mode_t)(VSF_GPIO_EXTI
                              | (m & (VSF_GPIO_PULL_UP | VSF_GPIO_PULL_DOWN))
                              | VSF_GPIO_EXTI_MODE_RISING);
        break;
    case GPIO_INTR_NEGEDGE:
        m = (vsf_gpio_mode_t)(VSF_GPIO_EXTI
                              | (m & (VSF_GPIO_PULL_UP | VSF_GPIO_PULL_DOWN))
                              | VSF_GPIO_EXTI_MODE_FALLING);
        break;
    case GPIO_INTR_ANYEDGE:
        m = (vsf_gpio_mode_t)(VSF_GPIO_EXTI
                              | (m & (VSF_GPIO_PULL_UP | VSF_GPIO_PULL_DOWN))
                              | VSF_GPIO_EXTI_MODE_RISING_FALLING);
        break;
    case GPIO_INTR_LOW_LEVEL:
        m = (vsf_gpio_mode_t)(VSF_GPIO_EXTI
                              | (m & (VSF_GPIO_PULL_UP | VSF_GPIO_PULL_DOWN))
                              | VSF_GPIO_EXTI_MODE_LOW_LEVEL);
        break;
    case GPIO_INTR_HIGH_LEVEL:
        m = (vsf_gpio_mode_t)(VSF_GPIO_EXTI
                              | (m & (VSF_GPIO_PULL_UP | VSF_GPIO_PULL_DOWN))
                              | VSF_GPIO_EXTI_MODE_HIGH_LEVEL);
        break;
    case GPIO_INTR_DISABLE:
    default:
        /* keep direction */
        break;
    }

    return m;
}

// Configure a single pin via port_config_pins().
static esp_err_t __gpio_config_one(gpio_num_t n,
                                    gpio_mode_t mode,
                                    bool pull_up_en,
                                    bool pull_down_en,
                                    gpio_int_type_t intr_type)
{
    vsf_gpio_t *hw = __gpio_hw(n);
    if (hw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    vsf_gpio_cfg_t cfg = {
        .mode = __gpio_translate_mode(mode, pull_up_en, pull_down_en, intr_type),
    };
    vsf_gpio_pin_mask_t mask = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(n);
    if (VSF_ERR_NONE != vsf_gpio_port_config_pins(hw, mask, &cfg)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*============================ PUBLIC API ====================================*/

esp_err_t gpio_config(const gpio_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // The ESP-IDF contract uses bit N of pin_bit_mask to denote gpio_num==N.
    // Scan bits 0..63 and group pins sharing the same port into one
    // port_config_pins() call.
    const vsf_io_mapper_t *m = __io_mapper();
    uint64_t remaining = config->pin_bit_mask;
    if (remaining == 0) {
        return ESP_OK;
    }

    const uint16_t pins_per_port = (uint16_t)(1U << VSF_HW_IO_MAPPER_PORT_BITS_LOG2);
    const uint64_t per_port_mask = (pins_per_port >= 64)
                                    ? (uint64_t)-1
                                    : (((uint64_t)1 << pins_per_port) - 1);

    vsf_gpio_cfg_t cfg = {
        .mode = __gpio_translate_mode(
                    config->mode,
                    config->pull_up_en == GPIO_PULLUP_ENABLE,
                    config->pull_down_en == GPIO_PULLDOWN_ENABLE,
                    config->intr_type),
    };

    while (remaining != 0) {
        // Lowest still-set pin -> identifies the port we service this pass.
        uint32_t lsb = 0;
        while (((remaining >> lsb) & 1ULL) == 0ULL) {
            lsb++;
        }
        uint8_t port = (uint8_t)(lsb >> VSF_HW_IO_MAPPER_PORT_BITS_LOG2);
        if (port >= m->port_num) {
            // No such port in this board's mapping -- silently drop the
            // bits we cannot service; user asked for a non-existent pin.
            uint64_t drop_mask = per_port_mask << (port * pins_per_port);
            remaining &= ~drop_mask;
            continue;
        }

        uint64_t port_bits = (remaining >> (port * pins_per_port)) & per_port_mask;
        vsf_gpio_t *hw = (vsf_gpio_t *)m->io[port];
        if (hw != NULL) {
            if (VSF_ERR_NONE != vsf_gpio_port_config_pins(hw,
                                    (vsf_gpio_pin_mask_t)port_bits,
                                    &cfg)) {
                return ESP_FAIL;
            }
        }
        remaining &= ~(port_bits << (port * pins_per_port));
    }

    return ESP_OK;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    // Drop ISR binding, if any.
    if (__vsf_espidf_gpio.installed
        && (gpio_num < __vsf_espidf_gpio.total_pins)) {
        __vsf_espidf_gpio.table[gpio_num].cb  = NULL;
        __vsf_espidf_gpio.table[gpio_num].arg = NULL;
    }
    // Disable the per-pin EXTI in the HAL, regardless of prior state.
    vsf_gpio_t *hw = __gpio_hw(gpio_num);
    vsf_gpio_pin_mask_t pm = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(gpio_num);
    (void)vsf_gpio_exti_irq_disable(hw, pm);
    (void)vsf_gpio_exti_irq_clear(hw, pm);

    return __gpio_config_one(gpio_num, GPIO_MODE_INPUT, false, false,
                              GPIO_INTR_DISABLE);
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    return __gpio_config_one(gpio_num, mode, false, false, GPIO_INTR_DISABLE);
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    bool pu = (pull == GPIO_PULLUP_ONLY) || (pull == GPIO_PULLUP_PULLDOWN);
    bool pd = (pull == GPIO_PULLDOWN_ONLY) || (pull == GPIO_PULLUP_PULLDOWN);
    // pull_up + pull_down simultaneously cannot be expressed in VSF HAL;
    // __gpio_translate_mode() collapses it to no-pull. Direction stays
    // as input here because set_pull_mode() is pull-only per ESP-IDF.
    return __gpio_config_one(gpio_num, GPIO_MODE_INPUT, pu, pd,
                              GPIO_INTR_DISABLE);
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    vsf_gpio_t *hw = __gpio_hw(gpio_num);
    vsf_gpio_pin_mask_t m = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(gpio_num);
    if (level) {
        vsf_gpio_set(hw, m);
    } else {
        vsf_gpio_clear(hw, m);
    }
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    if (!__gpio_valid(gpio_num)) {
        return 0;
    }
    vsf_gpio_t *hw = __gpio_hw(gpio_num);
    vsf_gpio_pin_mask_t v = vsf_gpio_read(hw);
    return (int)((v >> __gpio_pin_idx(gpio_num)) & 1U);
}

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    return gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    return gpio_set_pull_mode(gpio_num, GPIO_FLOATING);
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    return gpio_set_pull_mode(gpio_num, GPIO_PULLDOWN_ONLY);
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    return gpio_set_pull_mode(gpio_num, GPIO_FLOATING);
}

esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    // Program EXTI trigger on the pin. The pin is implicitly moved to
    // VSF_GPIO_EXTI mode (input-side only). The actual IRQ line stays
    // disabled until gpio_intr_enable() arms it.
    esp_err_t err = __gpio_config_one(gpio_num, GPIO_MODE_INPUT, false, false,
                                        intr_type);
    if (err != ESP_OK) {
        return err;
    }
    if (intr_type == GPIO_INTR_DISABLE) {
        vsf_gpio_t *hw = __gpio_hw(gpio_num);
        vsf_gpio_pin_mask_t m = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(gpio_num);
        (void)vsf_gpio_exti_irq_disable(hw, m);
    }
    return ESP_OK;
}

esp_err_t gpio_intr_enable(gpio_num_t gpio_num)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    vsf_gpio_t *hw = __gpio_hw(gpio_num);
    vsf_gpio_pin_mask_t m = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(gpio_num);
    (void)vsf_gpio_exti_irq_clear(hw, m);
    return (VSF_ERR_NONE == vsf_gpio_exti_irq_enable(hw, m))
            ? ESP_OK : ESP_FAIL;
}

esp_err_t gpio_intr_disable(gpio_num_t gpio_num)
{
    if (!__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    vsf_gpio_t *hw = __gpio_hw(gpio_num);
    vsf_gpio_pin_mask_t m = (vsf_gpio_pin_mask_t)1U << __gpio_pin_idx(gpio_num);
    return (VSF_ERR_NONE == vsf_gpio_exti_irq_disable(hw, m))
            ? ESP_OK : ESP_FAIL;
}

/*---------------------------- ISR service ----------------------------------*/

static void __gpio_exti_dispatcher(void *target_ptr,
                                    vsf_gpio_t *gpio_ptr,
                                    vsf_gpio_pin_mask_t pin_mask)
{
    uintptr_t port_idx = (uintptr_t)target_ptr;
    (void)vsf_gpio_exti_irq_clear(gpio_ptr, pin_mask);

    if (!__vsf_espidf_gpio.installed || (__vsf_espidf_gpio.table == NULL)) {
        return;
    }

    uint64_t m = (uint64_t)pin_mask;
    while (m != 0) {
        // Find lowest set bit without relying on compiler builtins.
        uint32_t pin = 0;
        while (((m >> pin) & 1ULL) == 0ULL) {
            pin++;
        }
        m &= ~((uint64_t)1U << pin);

        uint32_t flat = (uint32_t)((port_idx << VSF_HW_IO_MAPPER_PORT_BITS_LOG2) | pin);
        if (flat >= __vsf_espidf_gpio.total_pins) {
            continue;
        }
        gpio_isr_entry_t e = __vsf_espidf_gpio.table[flat];
        if (e.cb != NULL) {
            e.cb(e.arg);
        }
    }
}

esp_err_t gpio_install_isr_service(int intr_alloc_flags)
{
    (void)intr_alloc_flags;   // VSF routes all EXTI through one HAL prio.

    vsf_protect_t orig = vsf_protect_sched();
    if (__vsf_espidf_gpio.installed) {
        vsf_unprotect_sched(orig);
        return ESP_OK;
    }
    vsf_unprotect_sched(orig);

    const vsf_io_mapper_t *m = __io_mapper();
    if ((m->port_num == 0) || (m->port_bits_log2 == 0)) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t pins_per_port = (uint16_t)(1U << m->port_bits_log2);
    uint32_t total         = (uint32_t)m->port_num * pins_per_port;

    gpio_isr_entry_t *tab = (gpio_isr_entry_t *)vsf_heap_malloc(
                                    total * sizeof(gpio_isr_entry_t));
    if (tab == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(tab, 0, total * sizeof(gpio_isr_entry_t));

    orig = vsf_protect_sched();
    if (__vsf_espidf_gpio.installed) {
        vsf_unprotect_sched(orig);
        vsf_heap_free(tab);
        return ESP_OK;
    }
    __vsf_espidf_gpio.table         = tab;
    __vsf_espidf_gpio.pins_per_port = pins_per_port;
    __vsf_espidf_gpio.total_pins    = (uint16_t)total;
    __vsf_espidf_gpio.installed     = true;
    vsf_unprotect_sched(orig);

    // Install the fan-out dispatcher on every populated port.
    for (uint8_t p = 0; p < m->port_num; p++) {
        vsf_gpio_t *hw = (vsf_gpio_t *)m->io[p];
        if (hw == NULL) {
            continue;
        }
        vsf_gpio_exti_irq_cfg_t cfg = {
            .handler_fn = __gpio_exti_dispatcher,
            .target_ptr = (void *)(uintptr_t)p,
            .prio       = vsf_arch_prio_0,
        };
        (void)vsf_gpio_exti_irq_config(hw, &cfg);
    }
    return ESP_OK;
}

void gpio_uninstall_isr_service(void)
{
    vsf_protect_t orig = vsf_protect_sched();
    if (!__vsf_espidf_gpio.installed) {
        vsf_unprotect_sched(orig);
        return;
    }
    gpio_isr_entry_t *tab = __vsf_espidf_gpio.table;
    __vsf_espidf_gpio.table     = NULL;
    __vsf_espidf_gpio.installed = false;
    vsf_unprotect_sched(orig);

    // Detach the dispatcher from every port.
    const vsf_io_mapper_t *m = __io_mapper();
    for (uint8_t p = 0; p < m->port_num; p++) {
        vsf_gpio_t *hw = (vsf_gpio_t *)m->io[p];
        if (hw == NULL) {
            continue;
        }
        vsf_gpio_exti_irq_cfg_t cfg = {
            .handler_fn = NULL,
            .target_ptr = NULL,
            .prio       = vsf_arch_prio_0,
        };
        (void)vsf_gpio_exti_irq_config(hw, &cfg);
    }

    vsf_heap_free(tab);
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num,
                                gpio_isr_t isr_handler,
                                void *args)
{
    if (!__vsf_espidf_gpio.installed) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((isr_handler == NULL)
        || (gpio_num < 0)
        || (gpio_num >= __vsf_espidf_gpio.total_pins)
        || !__gpio_valid(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    __vsf_espidf_gpio.table[gpio_num].cb  = isr_handler;
    __vsf_espidf_gpio.table[gpio_num].arg = args;
    return ESP_OK;
}

esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num)
{
    if (!__vsf_espidf_gpio.installed) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((gpio_num < 0) || (gpio_num >= __vsf_espidf_gpio.total_pins)) {
        return ESP_ERR_INVALID_ARG;
    }
    __vsf_espidf_gpio.table[gpio_num].cb  = NULL;
    __vsf_espidf_gpio.table[gpio_num].arg = NULL;
    return ESP_OK;
}

/*---------------------------- not supported --------------------------------*/

esp_err_t gpio_isr_register(void (*fn)(void *),
                             void *arg,
                             int intr_alloc_flags,
                             gpio_isr_handle_t *handle)
{
    (void)fn;
    (void)arg;
    (void)intr_alloc_flags;
    (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    (void)gpio_num;
    (void)intr_type;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num)
{
    (void)gpio_num;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_DRIVER_GPIO
