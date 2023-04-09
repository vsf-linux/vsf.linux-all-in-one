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

#ifndef __VSF_NETDRV_H__
#define __VSF_NETDRV_H__

/*============================ INCLUDES ======================================*/

#include "component/tcpip/vsf_tcpip_cfg.h"

#if VSF_USE_TCPIP == ENABLED

#include "kernel/vsf_kernel.h"

#if     defined(__VSF_NETDRV_CLASS_IMPLEMENT)
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_NETDRV_CLASS_INHERIT_NETLINK__) || defined(__VSF_NETDRV_CLASS_INHERIT_NETIF__)
#   define __VSF_CLASS_INHERIT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

vsf_dcl_class(vk_netdrv_t)
vsf_dcl_class(vk_netlink_op_t)
vsf_dcl_class(vk_netdrv_adapter_op_t)

typedef enum vk_netdrv_feature_t {
    VSF_NETDRV_FEATURE_THREAD      = 1 << 0,
} vk_netdrv_feature_t;

vsf_class(vk_netlink_op_t) {
#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETLINK__)
    protected_member(
#else
    private_member(
#endif
        vsf_err_t (*init)(vk_netdrv_t *netdrv);
        vsf_err_t (*fini)(vk_netdrv_t *netdrv);

        // can_output returns slot pointer which will be passed to output
        void * (*can_output)(vk_netdrv_t *netdrv);
        vsf_err_t (*output)(vk_netdrv_t *netdrv, void *slot, void *netbuf);
    )
};

vsf_class(vk_netdrv_adapter_op_t) {
#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETIF__)
    protected_member(
#else
    private_member(
#endif
        vsf_err_t (*on_connect)(void *netif);
        void (*on_disconnect)(void *netif);
        void (*on_netbuf_outputted)(void *netif, void *netbuf);
        void (*on_netlink_outputted)(void *netif, vsf_err_t err);
        void (*on_inputted)(void *netif, void *netbuf, uint_fast32_t size);

        void * (*alloc_buf)(void *netif, uint_fast32_t len);
        void (*free_buf)(void *netbuf);
        void * (*read_buf)(void *netbuf, vsf_mem_t *mem);
        uint8_t * (*header)(void *netbuf, int32_t len);

        vk_netdrv_feature_t (*feature)(void);
        void * (*thread)(void (*entry)(void *param), void *param);
    )
};

typedef struct vk_netdrv_addr_t {
    uint32_t size;
    union {
        uint32_t addr32;
        uint64_t addr64;
        uint8_t addr_buf[16];
    };
} vk_netdrv_addr_t;

vsf_class(vk_netdrv_t) {

    protected_member(
        vk_netdrv_addr_t macaddr;
        uint16_t mac_header_size;
        uint16_t mtu;
        uint16_t hwtype;

        uint8_t is_to_free;
        uint8_t is_connected;
    )

#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETIF__)
    protected_member(
#else
    private_member(
#endif
        struct {
            void *netif;
            const vk_netdrv_adapter_op_t *op;
            vsf_dlist_t eda_pending_list;
        } adapter;
    )

#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETLINK__)
    protected_member(
#else
    private_member(
#endif
        struct {
            void *param;
            const vk_netlink_op_t *op;
        } netlink;
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

void vsf_pnp_on_netdrv_new(vk_netdrv_t *netdrv);
void vsf_pnp_on_netdrv_del(vk_netdrv_t *netdrv);
void vsf_pnp_on_netdrv_prepare(vk_netdrv_t *netdrv);
void vsf_pnp_on_netdrv_connected(vk_netdrv_t *netdrv);
void vsf_pnp_on_netdrv_disconnect(vk_netdrv_t *netdrv);

extern void vk_netdrv_prepare(vk_netdrv_t *netdrv);
extern vk_netdrv_feature_t vk_netdrv_feature(vk_netdrv_t *netdrv);

extern vsf_err_t vk_netdrv_connect(vk_netdrv_t *netdrv);
extern void vk_netdrv_disconnect(vk_netdrv_t *netdrv);
extern void vk_netdrv_set_netlink_op(vk_netdrv_t *netdrv, const vk_netlink_op_t *netlink_op, void *param);
extern void * vk_netdrv_get_netif(vk_netdrv_t *netdrv);

#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETLINK__)
extern uint8_t * vk_netdrv_header(vk_netdrv_t *netdrv, void *netbuf, int32_t len);
extern bool vk_netdrv_is_connected(vk_netdrv_t *netdrv);
extern void * vk_netdrv_read_buf(vk_netdrv_t *netdrv, void *netbuf, vsf_mem_t *mem);
extern void * vk_netdrv_alloc_buf(vk_netdrv_t *netdrv);
extern void vk_netdrv_on_netbuf_outputted(vk_netdrv_t *netdrv, void *netbuf);
extern void vk_netdrv_on_netlink_outputted(vk_netdrv_t *netdrv, vsf_err_t err);
extern void vk_netdrv_on_outputted(vk_netdrv_t *netdrv, void *netbuf, vsf_err_t err);
extern void vk_netdrv_on_inputted(vk_netdrv_t *netdrv, void *netbuf, int_fast32_t size);

extern void * vk_netdrv_thread(vk_netdrv_t *netdrv,void (*entry)(void *), void *param);
#endif

#if defined(__VSF_NETDRV_CLASS_IMPLEMENT) || defined(__VSF_NETDRV_CLASS_INHERIT_NETIF__)
extern vsf_err_t vk_netdrv_init(vk_netdrv_t *netdrv);
extern vsf_err_t vk_netdrv_fini(vk_netdrv_t *netdrv);
// vk_netdrv_can_output returns a slot pointer, which will be passed to vk_netdrv_output
extern void * vk_netdrv_can_output(vk_netdrv_t *netdrv);
extern vsf_err_t vk_netdrv_output(vk_netdrv_t *netdrv, void *slot, void *netbuf);
#endif

// for link driver

#ifdef __cplusplus
}
#endif

/*============================ INCLUDES ======================================*/

#if VSF_NETDRV_USE_WPCAP == ENABLED
#   include "./driver/wpcap/vsf_netdrv_wpcap.h"
#endif

#undef __VSF_NETDRV_CLASS_IMPLEMENT
#undef __VSF_NETDRV_CLASS_INHERIT_NETLINK__
#undef __VSF_NETDRV_CLASS_INHERIT_NETIF__

#endif      // VSF_USE_TCPIP
#endif      // __VSF_NETDRV_H__
