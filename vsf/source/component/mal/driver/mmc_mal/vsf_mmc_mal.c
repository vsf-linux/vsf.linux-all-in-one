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

#include "../../vsf_mal_cfg.h"

#if VSF_USE_MAL == ENABLED && VSF_MAL_USE_MMC_MAL == ENABLED && VSF_HAL_USE_MMC == ENABLED

#define __VSF_MAL_CLASS_INHERIT__
#define __VSF_MMC_MAL_CLASS_IMPLEMENT

#include "../../vsf_mal.h"
#include "./vsf_mmc_mal.h"

/*============================ MACROS ========================================*/

#define VSF_EVT_MMC_ERROR               (VSF_EVT_USER + 0)
#define VSF_EVT_MMC_DONE                (VSF_EVT_USER + 1)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

static uint_fast32_t __vk_mmc_mal_blksz(vk_mal_t *mal, uint_fast64_t addr, uint_fast32_t size, vsf_mal_op_t op);
dcl_vsf_peda_methods(static, __vk_mmc_mal_init)
dcl_vsf_peda_methods(static, __vk_mmc_mal_fini)
dcl_vsf_peda_methods(static, __vk_mmc_mal_read)
dcl_vsf_peda_methods(static, __vk_mmc_mal_write)

/*============================ GLOBAL VARIABLES ==============================*/

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

const vk_mal_drv_t vk_mmc_mal_drv = {
    .blksz          = __vk_mmc_mal_blksz,
    .init           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_mmc_mal_init),
    .fini           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_mmc_mal_fini),
    .read           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_mmc_mal_read),
    .write          = (vsf_peda_evthandler_t)vsf_peda_func(__vk_mmc_mal_write),
};

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#endif

/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

static void __vk_mmc_mal_on_timer(vsf_callback_timer_t *timer)
{
    vk_mmc_mal_t *pthis = container_of(timer, vk_mmc_mal_t, timer);
    if (pthis->is_probing) {
        vsf_err_t err = vsf_mmc_probe_irqhandler(pthis->mmc, &pthis->use_as__vsf_mmc_probe_t, 0, 0, NULL);
        if (err <= 0) {
            pthis->is_probing = false;
            vsf_eda_post_evt(pthis->eda, (err < 0) ? VSF_EVT_MMC_ERROR : VSF_EVT_MMC_DONE);
        }
    }
}

static void __vk_mmc_mal_irqhandler(void *target, vsf_mmc_t *mmc,
    vsf_mmc_irq_mask_t irq_mask, vsf_mmc_transact_status_t status, uint32_t resp[4])
{
    vk_mmc_mal_t *pthis = target;
    vsf_eda_t *eda;

    if (pthis->is_probing) {
        vsf_err_t err = vsf_mmc_probe_irqhandler(mmc, &pthis->use_as__vsf_mmc_probe_t, irq_mask, status, resp);
        if (err > 0) {
            if (pthis->delay_ms > 0) {
                vsf_callback_timer_add_ms(&pthis->timer, pthis->delay_ms);
            }
        } else {
            pthis->is_probing = false;
            if ((eda = pthis->eda) != NULL) {
                pthis->eda = NULL;
                vsf_eda_post_evt(eda, (err < 0) ? VSF_EVT_MMC_ERROR : VSF_EVT_MMC_DONE);
            }
        }
    } else {
        if (    (status & MMC_TRANSACT_STATUS_ERR_MASK)
            ||  (   (irq_mask & MMC_IRQ_MASK_HOST_RESP_DONE)
                 && ((resp[0] & (R1_READY_FOR_DATA | R1_CUR_STATE(R1_STATE_MASK))) != (R1_READY_FOR_DATA | R1_CUR_STATE(R1_STATE_TRAN)))
                )
           ) {
            vsf_mmc_host_transact_stop(pthis->mmc);
            if ((eda = pthis->eda) != NULL) {
                pthis->eda = NULL;
                vsf_eda_post_evt(eda, VSF_EVT_MMC_ERROR);
            }
        }
        if (irq_mask & MMC_IRQ_MASK_HOST_DATA_DONE) {
            if ((eda = pthis->eda) != NULL) {
                pthis->eda = NULL;
                vsf_eda_post_evt(eda, VSF_EVT_MMC_DONE);
            }
        }
    }
}

static uint_fast32_t __vk_mmc_mal_blksz(vk_mal_t *mal, uint_fast64_t addr, uint_fast32_t size, vsf_mal_op_t op)
{
    vk_mmc_mal_t *pthis = (vk_mmc_mal_t *)mal;
    return 1 << pthis->csd.sd_v2.READ_BL_LEN;
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-align"
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wcast-align"
#endif

__vsf_component_peda_ifs_entry(__vk_mmc_mal_init, vk_mal_init)
{
    vsf_peda_begin();

    vk_mmc_mal_t *pthis = (vk_mmc_mal_t *)&vsf_this;
    switch (evt) {
    case VSF_EVT_INIT:
        pthis->timer.on_timer = __vk_mmc_mal_on_timer;
        vsf_callback_timer_init(&pthis->timer);

        pthis->is_probing = true;
        pthis->eda = vsf_eda_get_cur();
        VSF_FS_ASSERT(pthis->eda != NULL);
        vsf_mmc_init(pthis->mmc, &(vsf_mmc_cfg_t) {
            .mode   = MMC_MODE_HOST,
            .isr    = {
                .prio       = pthis->hw_priority,
                .target_ptr = pthis,
                .handler_fn = __vk_mmc_mal_irqhandler,
            },
        });
        vsf_mmc_probe_start(pthis->mmc, &pthis->use_as__vsf_mmc_probe_t);
        break;
    case VSF_EVT_MMC_ERROR:
        vsf_eda_return(VSF_ERR_FAIL);
        break;
    case VSF_EVT_MMC_DONE:
        vsf_mmc_irq_enable(pthis->mmc, MMC_IRQ_MASK_HOST_RESP_DONE | MMC_IRQ_MASK_HOST_DATA_DONE);
        vsf_eda_return(VSF_ERR_NONE);
        break;
    }

    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_mmc_mal_fini, vk_mal_fini)
{
    vsf_peda_begin();
    vsf_eda_return(VSF_ERR_NONE);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_mmc_mal_read, vk_mal_read)
{
    vsf_peda_begin();
    vk_mmc_mal_t *pthis = (vk_mmc_mal_t *)&vsf_this;

    uint32_t block_size = (1 << pthis->csd.sd_v2.READ_BL_LEN);
    uint32_t block_start = vsf_local.addr / block_size;
    uint32_t block_num = vsf_local.size / block_size;

    switch (evt) {
    case VSF_EVT_INIT:
        pthis->eda = vsf_eda_get_cur();
        VSF_FS_ASSERT(pthis->eda != NULL);
        vsf_mmc_host_transact_start(pthis->mmc, &(vsf_mmc_trans_t){
            .cmd                = block_num > 1 ? MMC_READ_MULTIPLE_BLOCK : MMC_READ_SINGLE_BLOCK,
            .arg                = pthis->high_capacity ? block_start : vsf_local.addr,
            .op                 = block_num > 1 ? MMC_READ_MULTIPLE_BLOCK_OP : MMC_READ_SINGLE_BLOCK_OP,
            .block_size_bits    = pthis->csd.sd_v2.READ_BL_LEN,
            .buffer             = vsf_local.buff,
            .count              = vsf_local.size,
        });
        break;
    case VSF_EVT_MMC_ERROR:
        vsf_eda_return(-1);
        break;
    case VSF_EVT_MMC_DONE:
        vsf_eda_return(vsf_local.size);
        break;
    }
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_mmc_mal_write, vk_mal_write)
{
    vsf_peda_begin();
    vk_mmc_mal_t *pthis = (vk_mmc_mal_t *)&vsf_this;

    uint32_t block_size = (1 << pthis->csd.sd_v2.READ_BL_LEN);
    uint32_t block_start = vsf_local.addr / block_size;
    uint32_t block_num = vsf_local.size / block_size;

    switch (evt) {
    case VSF_EVT_INIT:
        pthis->eda = vsf_eda_get_cur();
        VSF_FS_ASSERT(pthis->eda != NULL);
        vsf_mmc_host_transact_start(pthis->mmc, &(vsf_mmc_trans_t){
            .cmd                = block_num > 1 ? MMC_WRITE_MULTIPLE_BLOCK : MMC_WRITE_BLOCK,
            .arg                = pthis->high_capacity ? block_start : vsf_local.addr,
            .op                 = block_num > 1 ? MMC_WRITE_MULTIPLE_BLOCK_OP : MMC_WRITE_BLOCK_OP,
            .block_size_bits    = pthis->csd.sd_v2.READ_BL_LEN,
            .buffer             = vsf_local.buff,
            .count              = vsf_local.size,
        });
        break;
    case VSF_EVT_MMC_ERROR:
        vsf_eda_return(-1);
        break;
    case VSF_EVT_MMC_DONE:
        vsf_eda_return(vsf_local.size);
        break;
    }
    vsf_peda_end();
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic pop
#endif

#endif
