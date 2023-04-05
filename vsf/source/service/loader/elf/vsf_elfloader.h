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

/**
 * \file vsf_elfloader.h
 * \brief vsf elf loader
 *
 * provides a elf loader implementation
 */

/** @ingroup vsf_loader
 *  @{
 */

#ifndef __VSF_ELFLOADER_H__
#define __VSF_ELFLOADER_H__

/*============================ INCLUDES ======================================*/

#include "service/vsf_service_cfg.h"

#if VSF_USE_LOADER == ENABLED && VSF_LOADER_USE_ELF == ENABLED

#include <stdint.h>
#include "./elf.h"

#if     defined(__VSF_ELFLOADER_CLASS_IMPLEMENT)
#   undef __VSF_ELFLOADER_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#elif   defined(__VSF_ELFLOADER_CLASS_INHERIT__)
#   undef __VSF_ELFLOADER_CLASS_INHERIT__
#   define __VSF_CLASS_INHERIT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

vsf_class(vsf_elfloader_t) {
    public_member(
        implement(vsf_loader_t)
        void *static_base;
    )

    protected_member(
        void *arch_data;
    )

    private_member(
        void *ram_base;

        void *initarr;
        void *finiarr;
        uint32_t initarr_sz;
        uint32_t finiarr_sz;
    )
};

typedef enum vsf_elfloader_sym_type_t {
    VSF_ELFLOADER_SYM_NONE      = 0,    // STT_NOTYPE
    VSF_ELFLOADER_SYM_OBJECT    = 1,    // STT_OBJECT
    VSF_ELFLOADER_SYM_FUNC      = 2,    // STT_FUNC
    VSF_ELFLOADER_SYM_SECTION   = 3,    // STT_SECTION
    VSF_ELFLOADER_SYM_FILE      = 4,    // STT_FILE
} vsf_elfloader_sym_type_t;

enum {
    VSF_ELFLOADER_CB_FAIL       = -1,
    VSF_ELFLOADER_CB_GOON       = 0,
    VSF_ELFLOADER_CB_DONE       = 1,
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

extern void * vsf_elfloader_load(vsf_elfloader_t *elfloader, vsf_loader_target_t *target);
extern void vsf_elfloader_cleanup(vsf_elfloader_t *elfloader);
extern int vsf_elfloader_call_init_array(vsf_elfloader_t *elfloader);
extern void vsf_elfloader_call_fini_array(vsf_elfloader_t *elfloader);

// can be called before vsf_elfloader_load
extern int vsf_elfloader_foreach_program_header(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, void *param,
        int (*callback)(vsf_elfloader_t *, vsf_loader_target_t *, Elf_Phdr *header, int index, void *param));
extern int vsf_elfloader_foreach_section(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, void *param,
        int (*callback)(vsf_elfloader_t *, vsf_loader_target_t *, Elf_Shdr *header, char *name, int index, void *param));
extern uint32_t vsf_elfloader_get_section(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, const char *name, Elf_Shdr *header);

// CAN NOT be called before vsf_elfloader_load
extern void * vsf_elfloader_get_symbol(vsf_elfloader_t *elfloader,
        const char *symbol_name, vsf_elfloader_sym_type_t symbol_type);

#ifdef __cplusplus
}
#endif

/** @} */   // vsf_loader

#endif      // VSF_USE_ELFLOADER
#endif      // __VSF_ELFLOADER_H__
