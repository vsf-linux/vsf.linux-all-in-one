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

/*============================ INCLUDES ======================================*/

#include "./vsf_input_cfg.h"

#if VSF_USE_INPUT == ENABLED

#define __VSF_INPUT_CLASS_IMPLEMENT
#include "./vsf_input.h"
#include "kernel/vsf_kernel.h"
#include "utilities/vsf_utilities.h"

/*============================ MACROS ========================================*/

#ifndef VSF_INPUT_CFG_PROTECT_LEVEL
/*! \note   By default, the driver tries to make all APIs interrupt-safe,
 *!
 *!         in the case when you want to disable it,
 *!         please use following macro:
 *!         #define VSF_INPUT_CFG_PROTECT_LEVEL  none
 *!
 *!         in the case when you want to use scheduler-safe,
 *!         please use following macro:
 *!         #define VSF_INPUT_CFG_PROTECT_LEVEL  scheduler
 *!
 *!         NOTE: This macro should be defined in vsf_usr_cfg.h
 */
#   define VSF_INPUT_CFG_PROTECT_LEVEL      interrupt
#endif

#define vsf_input_protect                   vsf_protect(VSF_INPUT_CFG_PROTECT_LEVEL)
#define vsf_input_unprotect                 vsf_unprotect(VSF_INPUT_CFG_PROTECT_LEVEL)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

#if VSF_INPUT_CFG_REGISTRATION_MECHANISM == ENABLED
typedef struct vsf_input_t {
    vsf_slist_t notifier_list;
} vsf_input_t;
#endif

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

#if VSF_INPUT_CFG_REGISTRATION_MECHANISM == ENABLED
static vsf_input_t __vsf_input;
#endif

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

void vk_input_buf_set(uint8_t *buf, uint_fast8_t offset, uint_fast8_t len)
{
    uint_fast8_t bitlen, bitpos, bytepos, mask, result_len = 0;

    while (result_len < len) {
        bytepos = offset >> 3;
        bitpos = offset & 7;
        bitlen = vsf_min(len - result_len, 8 - bitpos);
        mask = (1 << bitlen) - 1;

        buf[bytepos] |= ((~0UL >> result_len) & mask) << bitpos;

        offset += bitlen;
        result_len += bitlen;
    }
}

void vk_input_buf_clear(uint8_t *buf, uint_fast8_t offset, uint_fast8_t len)
{
    uint_fast8_t bitlen, bitpos, bytepos, mask, result_len = 0;

    while (result_len < len) {
        bytepos = offset >> 3;
        bitpos = offset & 7;
        bitlen = vsf_min(len - result_len, 8 - bitpos);
        mask = (1 << bitlen) - 1;

        buf[bytepos] &= ~(((~0UL >> result_len) & mask) << bitpos);

        offset += bitlen;
        result_len += bitlen;
    }
}

uint_fast32_t vk_input_buf_get_value(uint8_t *buf, uint_fast8_t offset, uint_fast8_t len)
{
    uint_fast8_t bitlen, bitpos, bytepos, mask, result_len = 0;
    uint_fast32_t result = 0;

    while (result_len < len) {
        bytepos = offset >> 3;
        bitpos = offset & 7;
        bitlen = vsf_min(len - result_len, 8 - bitpos);
        mask = (1 << bitlen) - 1;

        result |= ((buf[bytepos] >> bitpos) & mask) << result_len;

        offset += bitlen;
        result_len += bitlen;
    }
    return result;
}

void vk_input_buf_set_value(uint8_t *buf, uint_fast8_t offset, uint_fast8_t len, uint_fast32_t value)
{
    uint_fast8_t bitlen, bitpos, bytepos, mask, result_len = 0;

    while (result_len < len) {
        bytepos = offset >> 3;
        bitpos = offset & 7;
        bitlen = vsf_min(len - result_len, 8 - bitpos);
        mask = (1 << bitlen) - 1;

        buf[bytepos] &= ~(((~0UL >> result_len) & mask) << bitpos);
        buf[bytepos] |= ((value >> result_len) & mask) << bitpos;

        offset += bitlen;
        result_len += bitlen;
    }
}

vk_input_item_info_t * vk_input_parse(vk_input_parser_t *parser, uint8_t *pre, uint8_t *cur)
{
    vk_input_item_info_t *info = &parser->info[parser->num - 1];
    uint_fast32_t pre_value, cur_value;

    while (parser->num-- > 0) {
        cur_value = vk_input_buf_get_value(cur, info->offset, info->bitlen);
        pre_value = vk_input_buf_get_value(pre, info->offset, info->bitlen);

        if (cur_value != pre_value) {
            parser->cur.valu32 = cur_value;
            parser->pre.valu32 = pre_value;
            return info;
        }
        info--;
    }
    return NULL;
}

VSF_CAL_WEAK(vsf_input_on_new_dev)
void vsf_input_on_new_dev(vk_input_type_t type, void *dev)
{
}

VSF_CAL_WEAK(vsf_input_on_free_dev)
void vsf_input_on_free_dev(vk_input_type_t type, void *dev)
{
}

VSF_CAL_WEAK(vsf_input_on_evt)
void vsf_input_on_evt(vk_input_type_t type, vk_input_evt_t *evt)
{
#if VSF_INPUT_CFG_REGISTRATION_MECHANISM == ENABLED
    vsf_protect_t orig = vsf_input_protect();
        __vsf_slist_foreach_unsafe(vk_input_notifier_t, notifier_node, &__vsf_input.notifier_list) {
            if ((NULL == _->dev) || (_->dev == evt->dev)) {
                if (_->mask & (1 << type)) {
                    VSF_INPUT_ASSERT(_->on_evt != NULL);
                    _->on_evt(_, type, evt);
                }
            }
        }
    vsf_input_unprotect(orig);
#endif
}

uint_fast32_t vk_input_update_timestamp(vk_input_timestamp_t *timestamp)
{
#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
    vk_input_timestamp_t cur = vsf_systimer_get_tick();
    uint_fast32_t duration = *timestamp > 0 ? vsf_systimer_get_duration(*timestamp, cur) : 0;
    *timestamp = cur;
    return duration;
#else
    return 0;
#endif
}

#if VSF_INPUT_CFG_REGISTRATION_MECHANISM == ENABLED
void vk_input_notifier_register(vk_input_notifier_t *notifier)
{
    notifier->mask |= 1 << VSF_INPUT_TYPE_SYNC;
    vsf_protect_t orig = vsf_input_protect();
        VSF_INPUT_ASSERT(!vsf_slist_is_in(vk_input_notifier_t, notifier_node, &__vsf_input.notifier_list, notifier));
        vsf_slist_add_to_head(vk_input_notifier_t, notifier_node, &__vsf_input.notifier_list, notifier);
    vsf_input_unprotect(orig);
}

void vk_input_notifier_unregister(vk_input_notifier_t *notifier)
{
    vsf_protect_t orig = vsf_input_protect();
        VSF_INPUT_ASSERT(vsf_slist_is_in(vk_input_notifier_t, notifier_node, &__vsf_input.notifier_list, notifier));
        vsf_slist_remove(vk_input_notifier_t, notifier_node, &__vsf_input.notifier_list, notifier);
    vsf_input_unprotect(orig);
}
#endif

#endif      // VSF_USE_INPUT
