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

#ifndef __VSF_LINUX_FS_DEVFS_INTERNAL_H__
#define __VSF_LINUX_FS_DEVFS_INTERNAL_H__

/*============================ INCLUDES ======================================*/

#include "../../../../vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_DEVFS == ENABLED

// for hardware info
#include "hal/vsf_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

#if VSF_HAL_USE_GPIO == ENABLED
typedef struct vsf_linux_gpio_chip_t {
    uint8_t port_num;
    vsf_gpio_t * ports[0];
} vsf_linux_gpio_chip_t;
#endif

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

#if VSF_HAL_USE_USART == ENABLED
extern int vsf_linux_fs_bind_uart(char *path, vsf_usart_t *uart);
#endif
#if VSF_HAL_USE_I2C == ENABLED
extern int vsf_linux_fs_bind_i2c(char *path, vsf_i2c_t *i2c);
#endif
#if VSF_HAL_USE_SPI == ENABLED
extern int vsf_linux_fs_bind_spi(char *path, vsf_spi_t *spi);
#endif

#if VSF_USE_MAL == ENABLED
extern int vsf_linux_fs_bind_mal(char *path, vk_mal_t *mal);
#endif
#if VSF_USE_INPUT == ENABLED && VSF_INPUT_CFG_REGISTRATION_MECHANISM == ENABLED
extern int vsf_linux_fs_bind_input(char *path, vk_input_notifier_t *notifier);
#endif
#if VSF_USE_UI == ENABLED
extern int vsf_linux_fs_bind_disp(char *path, vk_disp_t *disp);
#endif
#if VSF_HAL_USE_GPIO == ENABLED
extern int vsf_linux_fs_bind_gpio(char *path, vsf_linux_gpio_chip_t *gpio_chip);
#   if VSF_HW_GPIO_COUNT > 0
extern int vsf_linux_fs_bind_gpio_hw(char *path);
#   endif
#endif
extern int vsf_linux_devfs_init(void);

#ifdef __cplusplus
}
#endif

/*============================ INCLUDES ======================================*/

#if VSF_LINUX_DEVFS_USE_ALSA == ENABLED
#   include "./alsa/vsf_linux_devfs_alsa.h"
#endif

#endif      // VSF_USE_LINUX && VSF_LINUX_USE_DEVFS
#endif      // __VSF_LINUX_FS_DEV_INTERNAL_H__
/* EOF */