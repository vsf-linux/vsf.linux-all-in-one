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

#include "service/vsf_service_cfg.h"

#if VSF_USE_LOADER == ENABLED && VSF_LOADER_USE_ELF == ENABLED && defined(__CPU_X64__)

#include "utilities/vsf_utilities.h"
#define __VSF_ELFLOADER_CLASS_INHERIT__
#include "../../vsf_loader.h"

/*============================ MACROS ========================================*/

#define R_X86_64_NONE       0   /* No reloc */
#define R_X86_64_64         1   /* Direct 64 bit  */
#define R_X86_64_PC32       2   /* PC relative 32 bit signed */
#define R_X86_64_GOT32      3   /* 32 bit GOT entry */
#define R_X86_64_PLT32      4   /* 32 bit PLT address */
#define R_X86_64_COPY       5   /* Copy symbol at runtime */
#define R_X86_64_GLOB_DAT   6   /* Create GOT entry */
#define R_X86_64_JUMP_SLOT  7   /* Create PLT entry */
#define R_X86_64_RELATIVE   8   /* Adjust by program base */
#define R_X86_64_GOTPCREL   9   /* 32 bit signed pc relative offset to GOT */
#define R_X86_64_32         10  /* Direct 32 bit zero extended */
#define R_X86_64_32S        11  /* Direct 32 bit sign extended */
#define R_X86_64_16         12  /* Direct 16 bit zero extended */
#define R_X86_64_PC16       13  /* 16 bit sign extended pc relative */
#define R_X86_64_8          14  /* Direct 8 bit sign extended  */
#define R_X86_64_PC8        15  /* 8 bit sign extended pc relative */

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

int vsf_elfloader_arch_relocate_sym(Elf_Addr tgtaddr, int type, Elf_Addr tgtvalue)
{
    switch (type) {
    case R_X86_64_64:
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT:
        *(uint64_t *)tgtaddr = tgtvalue;
        return 0;
    }
    return -1;
}

#ifdef __WIN__
// refer to https://github.com/303248153/HelloElfLoader
// ��System V AMD64 ABIת��ΪMicrosoft x64 calling convention
static const char generic_func_loader[] = {
    // �ò�������������ջ��
    // [��һ������] [�ڶ�������] [����������] ...
    0x58, // pop %rax �ݴ�ԭ���ص�ַ
    0x41, 0x51, // push %r9 ��ջ������������֮��Ĳ������ں�����ջ��
    0x41, 0x50, // push %r8 ��ջ���������
    0x51, // push %rcx ��ջ���ĸ�����
    0x52, // push %rdx ��ջ����������
    0x56, // push %rsi ��ջ�ڶ�������
    0x57, // push %rdi ��ջ��һ������

    // ����setOriginalReturnAddress����ԭ���ص�ַ
    0x48, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rcx ��һ��������ԭ�����ݴ��ַ
    0x48, 0x89, 0xc2, // mov %rax, %rdx �ڶ���������ԭ���ص�ַ
    0x48, 0x83, 0xec, 0x20, // sub $0x20, %rsp Ԥ��32�ֽڵ�Ӱ�ӿռ�
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
    0xff, 0xd0, // callq *%rax ����setOriginalReturnAddress
    0x48, 0x83, 0xc4, 0x20, // add %0x20, %rsp �ͷ�Ӱ�ӿռ�

    // ת����Microsoft x64 calling convention
    0x59, // pop %rcx ��ջ��һ������
    0x5a, // pop %rdx ��ջ�ڶ�������
    0x41, 0x58, // pop %r8 // ��ջ����������
    0x41, 0x59, // pop %r9 // ��ջ���ĸ�����

    // ����Ŀ�꺯��
    0x48, 0x83, 0xec, 0x20, // sub $0x20, %esp Ԥ��32�ֽڵ�Ӱ�ӿռ�
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs 0, %rax
    0xff, 0xd0, // callq *%rax ����ģ��ĺ���
    0x48, 0x83, 0xc4, 0x30, // add $0x30, %rsp �ͷ�Ӱ�ӿռ�Ͳ���(Ӱ�ӿռ�32 + ����8*2)
    0x50, // push %rax ���淵��ֵ

    // ����getOriginalReturnAddress��ȡԭ���ص�ַ
    0x48, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rcx ��һ��������ԭ�����ݴ��ַ
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
    0xff, 0xd0, // callq *%rax ����getOriginalReturnAddress
    0x48, 0x89, 0xc1, // mov %rax, %rcx ԭ���ص�ַ�浽rcx
    0x58, // �ָ�����ֵ
    0x51, // ԭ���ص�ַ��ջ��
    0xc3, // ����
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // �ݴ淵��ֵ�ռ�
};
static const int generic_func_loader_ra_set_slot_offset = 11;
static const int generic_func_loader_set_addr_offset = 28;
static const int generic_func_loader_target_offset = 54;
static const int generic_func_loader_ra_get_slot_offset = 71;
static const int generic_func_loader_get_addr_offset = 81;
static const int generic_func_loader_ra_slot_offset = 97;

void * getOriginalReturnAddress(void **slot) {
    void *result = *slot;
    *slot = NULL;
    return result;
}
void setOriginalReturnAddress(void **slot, void *address) {
    VSF_SERVICE_ASSERT(NULL == *slot);
    *slot = address;
}

typedef struct vsf_elfloader_arch_data_t {
    int pos;
    int num;
    char *entries;
} vsf_elfloader_arch_data_t;

int vsf_elfloader_arch_init_plt(vsf_elfloader_t *elfloader, int num)
{
    int entry_size = (sizeof(generic_func_loader) + 0xF) & ~0xF;
    vsf_elfloader_arch_data_t *arch_data = vsf_loader_malloc(elfloader, VSF_LOADER_MEM_RWX,
        sizeof(vsf_elfloader_arch_data_t) + num * entry_size, 0);
    if (NULL == arch_data) {
        return -1;
    }

    arch_data->pos = 0;
    arch_data->num = num;
    arch_data->entries = (char *)&arch_data[1];
    char *ptr = arch_data->entries;
    while (num-- > 0) {
        memcpy(ptr, generic_func_loader, sizeof(generic_func_loader));
        ptr += entry_size;
    }
    elfloader->arch_data = arch_data;
    return 0;
}

void vsf_elfloader_arch_fini_plt(vsf_elfloader_t *elfloader)
{
    if (elfloader->arch_data != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RWX, elfloader->arch_data);
        elfloader->arch_data = NULL;
    }
}

int vsf_elfloader_arch_link(vsf_elfloader_t *elfloader, char *symname, Elf_Addr *target)
{
    vsf_elfloader_arch_data_t *arch_data = elfloader->arch_data;
    VSF_SERVICE_ASSERT((arch_data != NULL) && (arch_data->pos < arch_data->num));

    Elf_Addr link_tgt;
    if (vsf_elfloader_link(elfloader, symname, &link_tgt) < 0) {
        return -1;
    }

    int entry_size = (sizeof(generic_func_loader) + 0xF) & ~0xF;
    char *ptr = arch_data->entries + arch_data->pos++ * entry_size;
    *(uint64_t *)(ptr + generic_func_loader_ra_set_slot_offset) = (uint64_t)ptr + generic_func_loader_ra_slot_offset;
    *(uint64_t *)(ptr + generic_func_loader_set_addr_offset) = (uint64_t)setOriginalReturnAddress;
    *(uint64_t *)(ptr + generic_func_loader_target_offset) = (uint64_t)link_tgt;
    *(uint64_t *)(ptr + generic_func_loader_ra_get_slot_offset) = (uint64_t)ptr + generic_func_loader_ra_slot_offset;
    *(uint64_t *)(ptr + generic_func_loader_get_addr_offset) = (uint64_t)getOriginalReturnAddress;
    *target = (Elf_Addr)ptr;
    return 0;
}
#endif

#endif      // VSF_USE_LOADER && VSF_LOADER_USE_ELF && defined(__ARM_ARCH_PROFILE)
