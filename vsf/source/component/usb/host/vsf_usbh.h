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

#ifndef __VSF_USBH_H__
#define __VSF_USBH_H__

/*============================ INCLUDES ======================================*/
#include "component/usb/vsf_usb_cfg.h"

#if VSF_USE_USB_HOST == ENABLED

#include "kernel/vsf_kernel.h"
#include "../common/usb_common.h"

#if     defined(__VSF_USBH_CLASS_IMPLEMENT)
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_urb_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_urb_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_t
#   define __VSF_USBH_CLASS_IMPLEMENT_CLASS__
#elif   defined(__VSF_USBH_CLASS_IMPLEMENT_HCD__)
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_urb_t
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t
#   if  defined(__VSF_USBH_CLASS_IMPLEMENT_HUB__)
#       define __VSF_USBH_CLASS_INHERIT_vk_usbh_t
#   endif
#elif   defined(__VSF_USBH_CLASS_IMPLEMENT_CLASS__)
#   define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t
#   define __VSF_USBH_CLASS_INHERIT_vk_usbh_t
#   define __VSF_USBH_CLASS_INHERIT_vk_usbh_urb_t
#   if defined(__VSF_USBH_CLASS_IMPLEMENT_HUB__)
#       define __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t
#   endif
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/

#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER != ENABLED
#   error "USBH require VSF_KERNEL_CFG_EDA_SUPPORT_TIMER"
#endif

#if !defined(VSF_USBH_CFG_EDA_PRIORITY)
#   define VSF_USBH_CFG_EDA_PRIORITY    vsf_prio_0
#endif

#ifndef VSF_USBH_CFG_ISO_EN
#   define VSF_USBH_CFG_ISO_EN          ENABLED
#endif
#ifndef VSF_USBH_CFG_PIPE_HAS_EXTRA
#   define VSF_USBH_CFG_PIPE_HAS_EXTRA  ENABLED
#endif

#if VSF_USBH_CFG_ISO_EN == ENABLED
#ifndef VSF_USBH_CFG_ISO_PACKET_LIMIT
#define VSF_USBH_CFG_ISO_PACKET_LIMIT   1
#endif
#endif

#ifndef VSF_USBH_CFG_ENABLE_ROOT_HUB
#   define VSF_USBH_CFG_ENABLE_ROOT_HUB ENABLED
#endif

#ifndef VSF_USBH_CFG_MAX_DEVICE
#   define VSF_USBH_CFG_MAX_DEVICE      127
#endif

#ifdef VSF_USBH_CFG_HEAP_IFS
#   undef vsf_usbh_malloc
#   undef vsf_usbh_malloc_aligned
#   undef vsf_usbh_free
#   define vsf_usbh_malloc              VSF_USBH_CFG_HEAP_IFS.Malloc
#   define vsf_usbh_malloc_aligned      VSF_USBH_CFG_HEAP_IFS.MallocAligned
#   define vsf_usbh_free                VSF_USBH_CFG_HEAP_IFS.Free
#else
#   ifndef vsf_usbh_malloc
#       define vsf_usbh_malloc          vsf_heap_malloc
#   else
extern void * vsf_usbh_malloc(uint_fast32_t size);
#   endif
#   ifndef vsf_usbh_malloc_aligned
#       define vsf_usbh_malloc_aligned vsf_heap_malloc_aligned
#   else
extern void * vsf_usbh_malloc_aligned(uint_fast32_t size, uint_fast32_t alignment);
#   endif
#   ifndef vsf_usbh_free
#       define vsf_usbh_free            vsf_heap_free
#   else
extern void vsf_usbh_free(void * buffer);
#   endif
#endif




#define VSF_USBH_MATCH_VID_PID(__VID, __PID)                                    \
            .match_vendor       = true,                                         \
            .match_product      = true,                                         \
            .idVendor           = (__VID),                                      \
            .idProduct          = (__PID),
#define VSF_USBH_MATCH_DEV_CLASS(__CLASS, __SUB_CLASS, __PROTOCOL)              \
            .match_dev_class    = true,                                         \
            .match_dev_subclass = true,                                         \
            .match_dev_protocol = true,                                         \
            .bDeviceClass       = (__CLASS),                                    \
            .bDeviceSubClass    = (__SUB_CLASS),                                \
            .bDeviceProtocol    = (__PROTOCOL),
#define VSF_USBH_MATCH_IFS_CLASS(__CLASS, __SUB_CLASS, __PROTOCOL)              \
            .match_ifs_class    = true,                                         \
            .match_ifs_subclass = true,                                         \
            .match_ifs_protocol = true,                                         \
            .bInterfaceClass    = (__CLASS),                                    \
            .bInterfaceSubClass = (__SUB_CLASS),                                \
            .bInterfaceProtocol = (__PROTOCOL),

#define VSF_USBH_MATCH_FLAGS_VENDOR         (1 << 0)
#define VSF_USBH_MATCH_FLAGS_PRODUCT        (1 << 1)
#define VSF_USBH_MATCH_FLAGS_DEV_LO         (1 << 2)
#define VSF_USBH_MATCH_FLAGS_DEV_HI         (1 << 3)
#define VSF_USBH_MATCH_FLAGS_DEV_CLASS      (1 << 4)
#define VSF_USBH_MATCH_FLAGS_DEV_SUBCLASS   (1 << 5)
#define VSF_USBH_MATCH_FLAGS_DEV_PROTOCOL   (1 << 6)
#define VSF_USBH_MATCH_FLAGS_IFS_CLASS      (1 << 7)
#define VSF_USBH_MATCH_FLAGS_IFS_SUBCLASS   (1 << 8)
#define VSF_USBH_MATCH_FLAGS_IFS_PROTOCOL   (1 << 9)
#define VSF_USBH_MATCH_FLAGS_IFS_NUM        (1 << 10)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

vsf_dcl_class(vk_usbh_hcd_t)
vsf_dcl_class(vk_usbh_hcd_drv_t)
vsf_dcl_class(vk_usbh_hcd_urb_t)
vsf_dcl_class(vk_usbh_hcd_dev_t)

vsf_dcl_class(vk_usbh_class_drv_t)
vsf_dcl_class(vk_usbh_urb_t)
vsf_dcl_class(vk_usbh_dev_t)
vsf_dcl_class(vk_usbh_t)

enum {
    URB_OK                  = VSF_ERR_NONE,
    URB_FAIL                = VSF_ERR_FAIL,
    URB_PENDING             = VSF_ERR_NOT_READY,
    URB_CANCELED            = 2,
    URB_TIMEOUT             = VSF_ERR_TIMEOUT,
};

// struct vsf_hcd_urb_t.transfer_flags
enum {
    URB_SHORT_NOT_OK        = 0x01,
    URB_ISO_ASAP            = 0x02,
    URB_ZERO_PACKET         = 0x04,
    __URB_NEED_FREE         = 0x08,
    URB_HCD_SPECIFIED_FLAGS = 0x10,
};

typedef struct vk_usbh_ifs_t {
    const vk_usbh_class_drv_t *drv;
    void *param;

    uint8_t no;
    uint8_t num_of_alt;
    uint8_t cur_alt;
} vk_usbh_ifs_t;

typedef struct vk_usbh_dev_id_t {
    union {
        struct {
            uint16_t match_vendor       : 1;
            uint16_t match_product      : 1;
            uint16_t match_dev_lo       : 1;
            uint16_t match_dev_hi       : 1;
            uint16_t match_dev_class    : 1;
            uint16_t match_dev_subclass : 1;
            uint16_t match_dev_protocol : 1;
            uint16_t match_ifs_class    : 1;
            uint16_t match_ifs_subclass : 1;
            uint16_t match_ifs_protocol : 1;
            uint16_t match_ifs_num      : 1;
            uint16_t                    : 5;
        };
        uint16_t match_flags;
    };

    // naming convention: usb documents
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice_lo, bcdDevice_hi;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t bInterfaceNumber;
    //uint32_t driver_info;
} vk_usbh_dev_id_t;

typedef struct vk_usbh_ifs_alt_parser_t {
    struct usb_interface_desc_t *desc_ifs;
    struct usb_endpoint_desc_t *desc_ep;
    uint16_t desc_size;
    uint8_t num_of_ep;
} vk_usbh_ifs_alt_parser_t;

typedef struct vk_usbh_ifs_parser_t {
    vk_usbh_ifs_alt_parser_t *parser_alt;
    vk_usbh_ifs_t *ifs;
    const vk_usbh_dev_id_t *id;
} vk_usbh_ifs_parser_t;

typedef struct vk_usbh_dev_parser_t {
    struct usb_device_desc_t *desc_device;
    struct usb_config_desc_t *desc_config;
    vk_usbh_ifs_parser_t *parser_ifs;
    uint8_t num_of_ifs;
    uint8_t devnum_temp;
    uint8_t is_to_retry             : 1;
    uint8_t no_set_configuration    : 1;
    enum {
        VSF_USBH_PROBE_START,
        VSF_USBH_PROBE_WAIT_DEVICE_DESC,
        VSF_USBH_PROBE_WAIT_DEVICE_RESET = VSF_USBH_PROBE_WAIT_DEVICE_DESC,
        VSF_USBH_PROBE_WAIT_SET_ADDRESS,
        VSF_USBH_PROBE_WAIT_ADDRESS_STABLE,
        VSF_USBH_PROBE_WAIT_FULL_DEVICE_DESC,
        VSF_USBH_PROBE_WAIT_CONFIG_DESC_SIZE,
        VSF_USBH_PROBE_WAIT_FULL_CONFIG_DESC,
        VSF_USBH_PROBE_WAIT_SET_CONFIG,
    } probe_state;
} vk_usbh_dev_parser_t;

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_CLASS__)
#   define __VSF_CLASS_IMPLEMENT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

vsf_class(vk_usbh_class_drv_t) {
    protected_member(
        const char *name;
        uint8_t dev_id_num;
        const vk_usbh_dev_id_t *dev_ids;
        void * (*probe)(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
                        vk_usbh_ifs_parser_t *parser_ifs);
        void (*disconnect)(vk_usbh_t *usbh, vk_usbh_dev_t *dev, void *param);
    )
};

typedef struct vk_usbh_class_t {
    const vk_usbh_class_drv_t *drv;
    vsf_slist_node_t node;
} vk_usbh_class_t;

typedef union vk_usbh_pipe_flag_t {
    struct {
        uint32_t is_pipe            : 1;
        uint32_t size               : 11;
        uint32_t endpoint           : 4;
        uint32_t type               : 2;
        uint32_t speed              : 2;
        uint32_t address            : 7;
        uint32_t dir_in1out0        : 1;
        uint32_t toggle             : 1;
        uint32_t is_submitted       : 1;
    };
    uint32_t value;
} vk_usbh_pipe_flag_t;

typedef struct vk_usbh_pipe_t {
    implement(vk_usbh_pipe_flag_t)
    uint16_t interval;
    uint16_t last_frame;
#if VSF_USBH_CFG_PIPE_HAS_EXTRA == ENABLED
    void *extra;
#endif
} vk_usbh_pipe_t;

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_t)
#   undef __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_t
#   define __VSF_CLASS_IMPLEMENT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

vsf_class(vk_usbh_hcd_drv_t) {
    private_member(
        vsf_err_t (*init_evthandler)(vsf_eda_t *eda, vsf_evt_t evt, vk_usbh_hcd_t *hcd);
        vsf_err_t (*fini)(vk_usbh_hcd_t *hcd);
        vsf_err_t (*suspend)(vk_usbh_hcd_t *hcd);
        vsf_err_t (*resume)(vk_usbh_hcd_t *hcd);
        uint_fast16_t (*get_frame_number)(vk_usbh_hcd_t *hcd);
        vsf_err_t (*alloc_device)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_dev_t *dev);
        void (*free_device)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_dev_t *dev);
        vk_usbh_hcd_urb_t * (*alloc_urb)(vk_usbh_hcd_t *hcd);
        void (*free_urb)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_urb_t *urb);
        vsf_err_t (*submit_urb)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_urb_t *urb);
        vsf_err_t (*relink_urb)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_urb_t *urb);

        union {
            int (*rh_control)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_urb_t *urb);
            struct {
                // for virtual hcd without root hub
                vsf_err_t (*reset_dev)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_dev_t *dev);
                bool (*is_dev_reset)(vk_usbh_hcd_t *hcd, vk_usbh_hcd_dev_t *dev);
            };
        };
    )
};

vsf_class(vk_usbh_hcd_t) {
    public_member(
        const vk_usbh_hcd_drv_t *drv;
        void *param;
        enum usb_device_speed_t rh_speed;
    )

    private_member(
        void *priv;
    )
};

#if VSF_USBH_CFG_ISO_EN == ENABLED
typedef struct vk_usbh_hcd_iso_packet_descriptor_t {
    uint32_t offset;                /*!< Start offset in transfer buffer*/
    uint32_t length;                /*!< Length in transfer buffer      */
    uint32_t actual_length;         /*!< Actual transfer length         */
    int32_t status;                 /*!< Transfer status                */
} vk_usbh_hcd_iso_packet_descriptor_t;

typedef struct vk_usbh_hcd_iso_packet_t {
    uint32_t start_frame;           /*!< start frame (iso/irq only)     */
    uint32_t number_of_packets;     /*!< number of packets (iso)        */
    //uint32_t error_count;         /*!< number of errors (iso only)    */
    vk_usbh_hcd_iso_packet_descriptor_t frame_desc[VSF_USBH_CFG_ISO_PACKET_LIMIT];
} vk_usbh_hcd_iso_packet_t;
#endif

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_urb_t)
#   undef __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_hcd_urb_t
#   define __VSF_CLASS_IMPLEMENT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

vsf_class(vk_usbh_hcd_dev_t) {
    private_member(
        void *dev_priv;
    )
};

vsf_class(vk_usbh_hcd_urb_t) {

    private_member(
        vk_usbh_hcd_dev_t *dev_hcd;
        vk_usbh_pipe_t pipe;

        uint32_t transfer_length;
        uint32_t actual_length;
        uint16_t timeout;
        uint16_t transfer_flags;    /*!< USB_DISABLE_SPD | USB_ISO_ASAP | etc. */
        uint16_t interval;          /*!< polling interval (iso/irq only)    */
        int16_t status;             /*!< returned status                    */

        void *buffer;
        void (*free_buffer)(void *param);
        void *free_buffer_param;

        union {
            struct usb_ctrlrequest_t setup_packet;
#if VSF_USBH_CFG_ISO_EN == ENABLED
            // TODO: iso_packet will enlarge every urb, need optimization
            vk_usbh_hcd_iso_packet_t iso_packet;
#endif
        };
    )

    private_member(
        vsf_eda_t *eda_caller;
        uint32_t priv[0];
    )
};

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_vk_usbh_urb_t)
#   undef __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_urb_t
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_USBH_CLASS_INHERIT_vk_usbh_urb_t)
#   undef __VSF_USBH_CLASS_INHERIT_vk_usbh_urb_t
#   define __VSF_CLASS_INHERIT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

vsf_class(vk_usbh_urb_t) {
    protected_member(
        union {
            vk_usbh_pipe_t pipe;
            vk_usbh_hcd_urb_t *urb_hcd;
        };
    )
};

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t)
#   undef __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_dev_t
#   define __VSF_CLASS_IMPLEMENT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vk_usbh_ep0_t {
    __vsf_crit_npb_t crit;
    vk_usbh_urb_t urb;
} vk_usbh_ep0_t;

vsf_class(vk_usbh_dev_t) {

    public_member(
        implement(vk_usbh_hcd_dev_t)
    )

    protected_member(
        vk_usbh_ep0_t ep0;
        vk_usbh_ifs_t *ifs;

#if VSF_USBH_USE_HUB == ENABLED
        vk_usbh_dev_t *dev_parent;
        vsf_slist_node_t child_node;
        vsf_slist_t children_list;
#endif

        uint8_t devnum;
        uint8_t num_of_ifs;
        uint8_t speed       : 2;
        uint8_t cur_config  : 5;
        uint8_t is_ep0_rdy  : 1;
        uint8_t maxchild    : 4;
        uint8_t index       : 4;
    )
};

#ifdef __cplusplus
}
#endif

#if     defined(__VSF_USBH_CLASS_IMPLEMENT_vk_usbh_t)
#   undef __VSF_USBH_CLASS_IMPLEMENT_vk_usbh_t
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_USBH_CLASS_INHERIT_vk_usbh_t)
#   undef __VSF_USBH_CLASS_INHERIT_vk_usbh_t
#   define __VSF_CLASS_INHERIT__
#endif
#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

vsf_declare_bitmap(vk_usbh_devnum_bitmap, VSF_USBH_CFG_MAX_DEVICE + 1);

vsf_class(vk_usbh_t) {

    public_member(
        implement(vk_usbh_hcd_t)
    )

    private_member(
        vsf_bitmap(vk_usbh_devnum_bitmap) device_bitmap;
#if VSF_USBH_CFG_ENABLE_ROOT_HUB == ENABLED
        vk_usbh_dev_t *dev_rh;
#endif
        vsf_slist_t class_list;
    )

    protected_member(
        vk_usbh_dev_t *dev_new;
        vk_usbh_dev_parser_t *parser;

        vsf_teda_t teda;
    )
};


/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

// APIs to be called by user
extern vsf_err_t vk_usbh_init(vk_usbh_t *usbh);
extern vsf_err_t vk_usbh_fini(vk_usbh_t *usbh);
extern void vk_usbh_register_class(vk_usbh_t *usbh, vk_usbh_class_t *c);
extern void vk_usbh_unregister_class(vk_usbh_t *usbh, vk_usbh_class_t *c);

#if defined(__VSF_USBH_CLASS_IMPLEMENT) || defined(__VSF_USBH_CLASS_IMPLEMENT_HCD__)
// APIs to be called by hcd drivers
void vk_usbh_hcd_urb_free_buffer(vk_usbh_hcd_urb_t *urb_hcd);
#endif

#if defined(__VSF_USBH_CLASS_IMPLEMENT_HUB__)
extern vk_usbh_dev_t * vk_usbh_new_device(vk_usbh_t *usbh, enum usb_device_speed_t speed,
            vk_usbh_dev_t *dev_parent, uint_fast8_t idx);
extern void vk_usbh_disconnect_device(vk_usbh_t *usbh, vk_usbh_dev_t *dev);
#endif

#if defined(__VSF_USBH_CLASS_IMPLEMENT) || defined(__VSF_USBH_CLASS_IMPLEMENT_CLASS__)
// APIs to be called by class drivers
extern uint_fast16_t vk_usbh_get_frame(vk_usbh_t *usbh);

#define __vk_usbh_get_pipe5(...)        __vk_usbh_get_pipe(__VA_ARGS__)
#define __vk_usbh_get_pipe4(...)        __vk_usbh_get_pipe5(__VA_ARGS__, 0)
#define vk_usbh_get_pipe(...)                                                   \
            __PLOOC_EVAL(__vk_usbh_get_pipe, __VA_ARGS__)(__VA_ARGS__)
extern vk_usbh_pipe_t __vk_usbh_get_pipe(vk_usbh_dev_t *dev,
            uint8_t endpoint, uint8_t type, uint16_t size, uint8_t interval);
extern vk_usbh_pipe_t vk_usbh_get_pipe_from_ep_desc(vk_usbh_dev_t *dev,
            struct usb_endpoint_desc_t *desc_ep);
extern void vk_usbh_urb_prepare_by_pipe(vk_usbh_urb_t *urb, vk_usbh_dev_t *dev,
            vk_usbh_pipe_t pipe);
extern void vk_usbh_urb_prepare(vk_usbh_urb_t *urb, vk_usbh_dev_t *dev,
            struct usb_endpoint_desc_t *desc_ep);
extern bool vk_usbh_urb_is_valid(vk_usbh_urb_t *urb);
extern bool vk_usbh_urb_is_alloced(vk_usbh_urb_t *urb);
extern vk_usbh_pipe_t vk_usbh_urb_get_pipe(vk_usbh_urb_t *urb);
extern void vk_usbh_urb_set_pipe(vk_usbh_urb_t *urb, vk_usbh_pipe_t pipe);
extern vsf_err_t vk_usbh_alloc_urb(vk_usbh_t *usbh, vk_usbh_dev_t *dev, vk_usbh_urb_t *urb);
extern void vk_usbh_unlink_urb(vk_usbh_t *usbh, vk_usbh_urb_t *urb);
extern void vk_usbh_free_urb(vk_usbh_t *usbh, vk_usbh_urb_t *urb);
extern void * vk_usbh_urb_alloc_buffer(vk_usbh_urb_t *urb, uint_fast16_t size);
extern void vk_usbh_urb_free_buffer(vk_usbh_urb_t *urb);
extern void * vk_usbh_urb_take_buffer(vk_usbh_urb_t *urb);
extern void * vk_usbh_urb_peek_buffer(vk_usbh_urb_t *urb);
extern void vk_usbh_urb_set_buffer(vk_usbh_urb_t *urb, void *buffer,
            uint_fast32_t size);
extern int_fast16_t vk_usbh_urb_get_status(vk_usbh_urb_t *urb);
extern uint_fast32_t vk_usbh_urb_get_actual_length(vk_usbh_urb_t *urb);

extern void vk_usbh_reset_dev(vk_usbh_t *usbh, vk_usbh_dev_t *dev);
extern bool vk_usbh_is_dev_resetting(vk_usbh_t *usbh, vk_usbh_dev_t *dev);
extern void vk_usbh_update_address(vk_usbh_dev_t *dev, uint_fast8_t address);

extern vsf_err_t vk_usbh_submit_urb(vk_usbh_t *usbh, vk_usbh_urb_t *urb);
extern vsf_err_t vk_usbh_submit_urb_flags(vk_usbh_t *usbh, vk_usbh_urb_t *urb, uint_fast16_t flags);
extern vsf_err_t vk_usbh_submit_urb_ex(vk_usbh_t *usbh, vk_usbh_urb_t *urb, uint_fast16_t flags, vsf_eda_t *eda);

#if VSF_USBH_CFG_ISO_EN == ENABLED
extern vsf_err_t vk_usbh_submit_urb_iso(vk_usbh_t *usbh, vk_usbh_urb_t *urb, uint_fast16_t start_frame);
#endif

extern void vk_usbh_remove_interface(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
        vk_usbh_ifs_t *ifs);

extern vsf_err_t vk_usbh_control_msg(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
        struct usb_ctrlrequest_t *req);
extern vsf_err_t vk_usbh_control_msg_ex(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
        struct usb_ctrlrequest_t *req, uint_fast16_t flags, vsf_eda_t *eda);

extern vsf_err_t vk_usbh_get_descriptor(vk_usbh_t *usbh,
        vk_usbh_dev_t *dev, uint_fast8_t type, uint_fast8_t index,
        uint_fast16_t size);
extern vsf_err_t vk_usbh_get_class_descriptor(vk_usbh_t *usbh,
        vk_usbh_dev_t *dev, uint_fast16_t ifnum, uint_fast8_t type,
        uint_fast8_t id, uint_fast16_t size);
extern vsf_err_t vk_usbh_set_configuration(vk_usbh_t *usbh,
        vk_usbh_dev_t *dev, uint_fast8_t configuration);
extern vsf_err_t vk_usbh_set_interface(vk_usbh_t *usbh,
        vk_usbh_dev_t *dev, uint_fast8_t ifnum, uint_fast8_t alternate);
extern vsf_err_t vk_usbh_clear_interface_feature(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
            uint_fast8_t ifs, uint_fast8_t feature);
extern vsf_err_t vk_usbh_clear_endpoint_feature(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
            uint_fast8_t endpoint, uint_fast8_t feature);
extern vsf_err_t vk_usbh_clear_endpoint_halt(vk_usbh_t *usbh, vk_usbh_dev_t *dev,
            uint_fast8_t endpoint);

extern vsf_err_t vk_usbh_get_extra_descriptor(uint8_t *buf, uint_fast16_t size,
        uint_fast8_t type, void **ptr);

extern usb_endpoint_desc_t * vk_usbh_get_next_ep_descriptor(
        usb_endpoint_desc_t *desc_ep, uint_fast16_t size);
#endif

#ifdef __cplusplus
}
#endif

/*============================ INCLUDES ======================================*/

// class drivers
#if VSF_USBH_USE_HUB == ENABLED
#   include "./class/HUB/vsf_usbh_HUB.h"
#endif
#if VSF_USBH_USE_CDC == ENABLED
#   include "./class/CDC/vsf_usbh_CDC.h"
#endif
#if VSF_USBH_USE_ECM == ENABLED
#   include "./class/CDC/vsf_usbh_CDCECM.h"
#endif
#if VSF_USBH_USE_BTHCI == ENABLED
#   include "./class/BTHCI/vsf_usbh_BTHCI.h"
#endif
#if VSF_USBH_USE_HID == ENABLED
#   include "./class/HID/vsf_usbh_HID.h"
#endif
#if VSF_USBH_USE_DS4 == ENABLED
#   include "./class/HID/vsf_usbh_ds4.h"
#endif
#if VSF_USBH_USE_NSPRO == ENABLED
#   include "./class/HID/vsf_usbh_nspro.h"
#endif
#if VSF_USBH_USE_XB360 == ENABLED
#   include "./class/HID/vsf_usbh_xb360.h"
#endif
#if VSF_USBH_USE_XB1 == ENABLED
#   include "./class/XB1/vsf_usbh_xb1.h"
#endif
#if VSF_USBH_USE_LIBUSB == ENABLED
#   include "./class/libusb/vsf_usbh_libusb.h"
#endif
#if VSF_USBH_USE_MSC == ENABLED
#   include "./class/MSC/vsf_usbh_msc.h"
#endif
#if VSF_USBH_USE_UAC == ENABLED
#   include "./class/UAC/vsf_usbh_uac.h"
#endif
#if VSF_USBH_USE_DFU == ENABLED
#   include "./class/DFU/vsf_usbh_dfu.h"
#endif
#if VSF_USBH_USE_DL1X5 == ENABLED
#   include "./class/DL1x5/vsf_usbh_dl1x5.h"
#endif

// hcd drivers
#if VSF_USBH_USE_HCD_OHCI == ENABLED
#   include "../driver/hcd/ohci/vsf_ohci.h"
#endif
#if VSF_USBH_USE_HCD_DWCOTG == ENABLED
#   include "../driver/otg/dwcotg/vsf_dwcotg_hcd.h"
#endif
#if VSF_USBH_USE_HCD_MUSB_FDRC == ENABLED
#   include "../driver/otg/musb/fdrc/vsf_musb_fdrc_hcd.h"
#endif
#if VSF_USBH_USE_HCD_LIBUSB == ENABLED
#   include "../driver/hcd/libusb_hcd/vsf_libusb_hcd.h"
#endif
#if VSF_USBH_USE_HCD_WINUSB == ENABLED
#   include "../driver/hcd/winusb_hcd/vsf_winusb_hcd.h"
#endif

#undef __VSF_USBH_CLASS_IMPLEMENT
#undef __VSF_USBH_CLASS_IMPLEMENT_CLASS__
#undef __VSF_USBH_CLASS_IMPLEMENT_HCD__
#undef __VSF_USBH_CLASS_IMPLEMENT_HUB__

#endif
#endif    // __VSF_USBH_H__
