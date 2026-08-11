# ESP-IDF public headers (clean-room)

This directory holds headers that mimic the shape of ESP-IDF public API
(`esp_log.h`, `esp_err.h`, `esp_event.h`, `esp_timer.h`, `driver/gpio.h`, ...).

Rules:

1. **Clean-room only.** No file in this tree is copied or derived from the
   ESP-IDF source tree. Each header is re-authored from the ESP-IDF public
   API reference documentation, using only the API signatures, enums and
   struct layouts required for binary/source compatibility with ESP-IDF
   example applications.

2. **Baseline version.** Header shape targets the ESP-IDF version declared
   in `vsf_espidf_cfg.h` via `VSF_ESPIDF_CFG_VERSION_*`. When a new baseline
   is adopted, diff public API changes and update accordingly.

3. **Naming.** Preserve ESP-IDF public names exactly (`esp_err_t`,
   `ESP_OK`, `gpio_set_level`, ...). Do not add VSF-specific prefixes to
   public symbols here; VSF-internal helpers live in `../port/`.

4. **Layout.** Match ESP-IDF include path structure so applications can
   `#include "driver/gpio.h"` unchanged.

Planned subdirectories (populated per-module):

- `esp_err.h`, `esp_log.h`, `esp_check.h`, `esp_common.h`
- `esp_event.h`, `esp_event_base.h`
- `esp_timer.h`
- `esp_system.h`, `esp_mac.h`, `esp_random.h`, `esp_chip_info.h`
- `esp_partition.h`, `nvs.h`, `nvs_flash.h`
- `esp_netif.h`, `esp_netif_types.h`
- `esp_http_client.h`, `esp_http_server.h`
- `esp_wifi.h`, `esp_wifi_types.h` (stage C, requires external radio)
- `freertos/FreeRTOS.h` and friends (forwarders to VSF FreeRTOS compat)
- `driver/gptimer.h` (present)
- `driver/gpio.h` (present)
- `driver/uart.h` (present)
- `driver/i2c_master.h`, `driver/spi_master.h`,
  `driver/ledc.h`, `driver/adc.h` (pending)
- `hal/gpio_types.h`, `hal/uart_types.h`, ...
