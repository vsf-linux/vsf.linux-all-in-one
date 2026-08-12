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
 * esp_partition.h -> component/mal bridge.
 *
 * Each registered partition owns one vk_mim_mal_t slice over a user-supplied
 * root mal (flash / file / mem / any custom mal). All esp_partition_*
 * synchronous I/O delegates to vk_mal_read/write/erase on the slice, which
 * the mim_mal driver rewrites as (addr + offset) reads on the root mal.
 *
 * Storage for partitions lives in a fixed-size array: one slot per
 * compile-time entry from vsf_espidf_cfg_t.partition.entries plus an
 * optional pool of VSF_ESPIDF_CFG_PARTITION_MAX_DYNAMIC slots for
 * esp_partition_register_external() callers.
 *
 * Synchronisation: all APIs MUST be invoked from a vsf_thread_t context
 * so that the underlying vk_mal_* peda subcalls complete synchronously
 * on return. Calling from a bare vsf_eda_t handler is undefined.
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_PARTITION == ENABLED

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define __VSF_MAL_CLASS_INHERIT__
#define __VSF_MIM_MAL_CLASS_IMPLEMENT
#include "component/mal/vsf_mal.h"
#include "component/mal/driver/mim_mal/vsf_mim_mal.h"

#include "service/heap/vsf_heap.h"

#include "../vsf_espidf.h"
#include "esp_partition.h"

#if !defined(VSF_USE_MAL) || VSF_USE_MAL != ENABLED
#   error "esp_partition port requires VSF_USE_MAL == ENABLED"
#endif
#if !defined(VSF_MAL_USE_MIM_MAL) || VSF_MAL_USE_MIM_MAL != ENABLED
#   error "esp_partition port requires VSF_MAL_USE_MIM_MAL == ENABLED"
#endif

/*============================ MACROS ========================================*/

#define __PART_STATIC_MAX       32u

#if VSF_ESPIDF_CFG_PARTITION_DYNAMIC == ENABLED
#   define __PART_DYN_MAX       VSF_ESPIDF_CFG_PARTITION_MAX_DYNAMIC
#else
#   define __PART_DYN_MAX       0u
#endif

#define __PART_TOTAL_MAX        (__PART_STATIC_MAX + __PART_DYN_MAX)

/*============================ TYPES =========================================*/

typedef struct __part_slot_t {
    esp_partition_t  pub;      /* public descriptor handed out to callers */
    vk_mim_mal_t     mim;      /* private slice over root_mal */
    bool             in_use;
    bool             is_dynamic;
} __part_slot_t;

/* Opaque iterator published via esp_partition_iterator_t. */
struct esp_partition_iterator_opaque_t {
    esp_partition_type_t    type;
    esp_partition_subtype_t subtype;
    char                    label[ESP_PARTITION_LABEL_MAX + 1];
    bool                    label_any;
    uint16_t                cursor;         /* next slot index to examine */
};

/*============================ LOCAL VARIABLES ===============================*/

static struct {
    vk_mal_t       *root_mal;
    __part_slot_t   slots[__PART_TOTAL_MAX];
    uint16_t        static_count;   /* slots [0, static_count) are static */
    bool            is_inited;
} __part_state = { 0 };

/*============================ PROTOTYPES ====================================*/

static bool __part_match(const __part_slot_t *s,
                         esp_partition_type_t    type,
                         esp_partition_subtype_t subtype,
                         const char             *label);
static __part_slot_t * __part_slot_alloc_dynamic(void);
static esp_err_t __part_init_slot(__part_slot_t *slot,
                                  const char *label,
                                  uint8_t type, uint8_t subtype,
                                  uint32_t offset, uint32_t size,
                                  uint32_t erase_size,
                                  bool encrypted, bool readonly,
                                  bool is_dynamic);

/*============================ IMPLEMENTATION ================================*/

/* Must be visible to vsf_espidf.c. */
extern void vsf_espidf_partition_init(const vsf_espidf_partition_cfg_t *cfg);

void vsf_espidf_partition_init(const vsf_espidf_partition_cfg_t *cfg)
{
    if (__part_state.is_inited) {
        return;
    }
    memset(&__part_state, 0, sizeof(__part_state));
    __part_state.is_inited = true;

    if ((cfg == NULL) || (cfg->root_mal == NULL)) {
        return;     /* no partitions until a root mal is provided */
    }
    __part_state.root_mal = cfg->root_mal;

    uint16_t count = cfg->entry_count;
    if ((count == 0) || (cfg->entries == NULL)) {
        return;
    }
    if (count > __PART_STATIC_MAX) {
        count = __PART_STATIC_MAX;
    }
    for (uint16_t i = 0; i < count; i++) {
        const vsf_espidf_partition_entry_t *e = &cfg->entries[i];
        __part_slot_t *slot = &__part_state.slots[__part_state.static_count];
        if (__part_init_slot(slot, e->label, e->type, e->subtype,
                             e->offset, e->size, e->erase_size,
                             e->encrypted, e->readonly, false) == ESP_OK) {
            __part_state.static_count++;
        }
    }
}

static esp_err_t __part_init_slot(__part_slot_t *slot,
                                  const char *label,
                                  uint8_t type, uint8_t subtype,
                                  uint32_t offset, uint32_t size,
                                  uint32_t erase_size,
                                  bool encrypted, bool readonly,
                                  bool is_dynamic)
{
    if ((__part_state.root_mal == NULL) || (size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint64_t)offset + (uint64_t)size > __part_state.root_mal->size) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(slot, 0, sizeof(*slot));

    /* Set up the mim_mal slice. vk_mal_t base members first. */
    slot->mim.drv     = &vk_mim_mal_drv;
    slot->mim.size    = size;
    slot->mim.feature = __part_state.root_mal->feature;
    slot->mim.host_mal = __part_state.root_mal;
    slot->mim.offset   = offset;

    if (vk_mal_init((vk_mal_t *)&slot->mim) != VSF_ERR_NONE) {
        return ESP_FAIL;
    }

    /* Auto-detect erase block if caller did not override. */
    uint32_t eblk = erase_size;
    if (eblk == 0) {
        eblk = (uint32_t)vk_mal_blksz((vk_mal_t *)&slot->mim,
                                      0, 0, VSF_MAL_OP_ERASE);
        if (eblk == 0) {
            /* Back-end has no erase granularity (e.g. RAM mal): fall back
             * to the write block size so erase_range validation has a
             * reasonable unit. */
            eblk = (uint32_t)vk_mal_blksz((vk_mal_t *)&slot->mim,
                                          0, 0, VSF_MAL_OP_WRITE);
            if (eblk == 0) {
                eblk = 1;
            }
        }
    }

    slot->pub.type       = (esp_partition_type_t)type;
    slot->pub.subtype    = (esp_partition_subtype_t)subtype;
    slot->pub.address    = offset;
    slot->pub.size       = size;
    slot->pub.erase_size = eblk;
    slot->pub.encrypted  = encrypted;
    slot->pub.readonly   = readonly;
    if (label != NULL) {
        size_t n = strnlen(label, ESP_PARTITION_LABEL_MAX);
        memcpy(slot->pub.label, label, n);
        slot->pub.label[n] = '\0';
    }
    slot->pub.mal = &slot->mim;

    slot->in_use     = true;
    slot->is_dynamic = is_dynamic;
    return ESP_OK;
}

static __part_slot_t * __part_slot_alloc_dynamic(void)
{
#if VSF_ESPIDF_CFG_PARTITION_DYNAMIC == ENABLED
    for (uint16_t i = __PART_STATIC_MAX;
         i < __PART_STATIC_MAX + __PART_DYN_MAX; i++) {
        if (!__part_state.slots[i].in_use) {
            return &__part_state.slots[i];
        }
    }
#endif
    return NULL;
}

static bool __part_match(const __part_slot_t *s,
                         esp_partition_type_t    type,
                         esp_partition_subtype_t subtype,
                         const char             *label)
{
    if (!s->in_use) {
        return false;
    }
    if ((type != ESP_PARTITION_TYPE_ANY) && (s->pub.type != type)) {
        return false;
    }
    if ((subtype != ESP_PARTITION_SUBTYPE_ANY)
            && (s->pub.subtype != subtype)) {
        return false;
    }
    if (label != NULL) {
        if (strncmp(s->pub.label, label, ESP_PARTITION_LABEL_MAX) != 0) {
            return false;
        }
    }
    return true;
}

/* --- Discovery -------------------------------------------------------- */

esp_partition_iterator_t esp_partition_find(esp_partition_type_t    type,
                                             esp_partition_subtype_t subtype,
                                             const char             *label)
{
    if (!__part_state.is_inited) {
        return NULL;
    }
    /* Scan for a first match up front; if none, no iterator is allocated. */
    uint16_t total = __PART_STATIC_MAX + __PART_DYN_MAX;
    uint16_t first = UINT16_MAX;
    for (uint16_t i = 0; i < total; i++) {
        if (__part_match(&__part_state.slots[i], type, subtype, label)) {
            first = i;
            break;
        }
    }
    if (first == UINT16_MAX) {
        return NULL;
    }

    struct esp_partition_iterator_opaque_t *it =
        (struct esp_partition_iterator_opaque_t *)vsf_heap_malloc(sizeof(*it));
    if (it == NULL) {
        return NULL;
    }
    memset(it, 0, sizeof(*it));
    it->type       = type;
    it->subtype    = subtype;
    it->label_any  = (label == NULL);
    if (label != NULL) {
        size_t n = strnlen(label, ESP_PARTITION_LABEL_MAX);
        memcpy(it->label, label, n);
        it->label[n] = '\0';
    }
    it->cursor = first;
    return it;
}

const esp_partition_t * esp_partition_find_first(esp_partition_type_t    type,
                                                  esp_partition_subtype_t subtype,
                                                  const char             *label)
{
    esp_partition_iterator_t it = esp_partition_find(type, subtype, label);
    if (it == NULL) {
        return NULL;
    }
    const esp_partition_t *p = esp_partition_get(it);
    esp_partition_iterator_release(it);
    return p;
}

const esp_partition_t * esp_partition_get(esp_partition_iterator_t iterator)
{
    if ((iterator == NULL) || !__part_state.is_inited) {
        return NULL;
    }
    uint16_t total = __PART_STATIC_MAX + __PART_DYN_MAX;
    if (iterator->cursor >= total) {
        return NULL;
    }
    return &__part_state.slots[iterator->cursor].pub;
}

esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t iterator)
{
    if ((iterator == NULL) || !__part_state.is_inited) {
        return NULL;
    }
    uint16_t total = __PART_STATIC_MAX + __PART_DYN_MAX;
    const char *lbl = iterator->label_any ? NULL : iterator->label;
    for (uint16_t i = iterator->cursor + 1; i < total; i++) {
        if (__part_match(&__part_state.slots[i],
                         iterator->type, iterator->subtype, lbl)) {
            iterator->cursor = i;
            return iterator;
        }
    }
    esp_partition_iterator_release(iterator);
    return NULL;
}

void esp_partition_iterator_release(esp_partition_iterator_t iterator)
{
    if (iterator == NULL) {
        return;
    }
    vsf_heap_free(iterator);
}

bool esp_partition_check_identity(const esp_partition_t *a,
                                  const esp_partition_t *b)
{
    return (a != NULL) && (a == b);
}

/* --- I/O -------------------------------------------------------------- */

static esp_err_t __part_bounds(const esp_partition_t *p, size_t off, size_t size)
{
    if ((p == NULL) || (p->mal == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((off > p->size) || (size > p->size) || ((off + size) > p->size)) {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t esp_partition_read(const esp_partition_t *partition,
                              size_t src_offset, void *dst, size_t size)
{
    if (dst == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t rc = __part_bounds(partition, src_offset, size);
    if (rc != ESP_OK) {
        return rc;
    }
    if (size == 0) {
        return ESP_OK;
    }
    vsf_err_t err = vk_mal_read((vk_mal_t *)partition->mal,
                                src_offset, size, (uint8_t *)dst);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_partition_write(const esp_partition_t *partition,
                               size_t dst_offset, const void *src, size_t size)
{
    if (src == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t rc = __part_bounds(partition, dst_offset, size);
    if (rc != ESP_OK) {
        return rc;
    }
    if (partition->readonly) {
        return ESP_ERR_NOT_ALLOWED;
    }
    if (size == 0) {
        return ESP_OK;
    }
    vsf_err_t err = vk_mal_write((vk_mal_t *)partition->mal,
                                 dst_offset, size, (uint8_t *)src);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_partition_erase_range(const esp_partition_t *partition,
                                     size_t offset, size_t size)
{
    esp_err_t rc = __part_bounds(partition, offset, size);
    if (rc != ESP_OK) {
        return rc;
    }
    if (partition->readonly) {
        return ESP_ERR_NOT_ALLOWED;
    }
    if (size == 0) {
        return ESP_OK;
    }
    /* ESP-IDF requires offset and size to be multiples of erase_size. */
    if ((partition->erase_size > 1)
            && (((offset % partition->erase_size) != 0)
                || ((size % partition->erase_size) != 0))) {
        return ESP_ERR_INVALID_SIZE;
    }
    /* Root mal may not expose an erase op (mem_mal / file_mal have no
     * flash-style sector erase). Probe feature + driver slot first so
     * callers get NOT_SUPPORTED instead of an assert in vk_mal_erase. */
    vk_mal_t *slice = (vk_mal_t *)partition->mal;
    if (!(slice->feature & VSF_MAL_ERASABLE)
            || (slice->drv == NULL) || (slice->drv->erase == NULL)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    vsf_err_t err = vk_mal_erase(slice, offset, size);
    return (err == VSF_ERR_NONE) ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_partition_read_raw(const esp_partition_t *partition,
                                  size_t src_offset, void *dst, size_t size)
{
    /* Shim has no flash-encryption layer: raw == cooked. */
    return esp_partition_read(partition, src_offset, dst, size);
}

esp_err_t esp_partition_write_raw(const esp_partition_t *partition,
                                   size_t dst_offset, const void *src,
                                   size_t size)
{
    return esp_partition_write(partition, dst_offset, src, size);
}

/* --- Memory mapping --------------------------------------------------- */

esp_err_t esp_partition_mmap(const esp_partition_t *partition,
                              size_t offset, size_t size,
                              esp_partition_mmap_memory_t memory,
                              const void **out_ptr,
                              esp_partition_mmap_handle_t *out_handle)
{
    (void)memory;
    if ((partition == NULL) || (partition->mal == NULL) || (out_ptr == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t rc = __part_bounds(partition, offset, size);
    if (rc != ESP_OK) {
        return rc;
    }

    /* mmap requires the root mal to publish a directly-addressable buffer
     * window. Check the capability before asking for a pointer. */
    vk_mal_t *slice = (vk_mal_t *)partition->mal;
    if (!(slice->feature & VSF_MAL_LOCAL_BUFFER)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    vsf_mem_t mem = { 0 };
    if (!vk_mal_prepare_buffer(slice, offset, size, VSF_MAL_OP_READ, &mem)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    *out_ptr = (const void *)mem.buffer;
    if (out_handle != NULL) {
        /* No per-mmap state is kept: return a non-zero sentinel. */
        *out_handle = 1;
    }
    return ESP_OK;
}

void esp_partition_munmap(esp_partition_mmap_handle_t handle)
{
    (void)handle;
    /* Nothing to release: the pointer aliases the root mal's internal
     * buffer and remains valid as long as the mal does. */
}

/* --- Verification ----------------------------------------------------- */

const esp_partition_t * esp_partition_verify(const esp_partition_t *partition)
{
    if ((partition == NULL) || !__part_state.is_inited) {
        return NULL;
    }
    uint16_t total = __PART_STATIC_MAX + __PART_DYN_MAX;
    for (uint16_t i = 0; i < total; i++) {
        if (__part_state.slots[i].in_use
                && (&__part_state.slots[i].pub == partition)) {
            return partition;
        }
    }
    return NULL;
}

/* --- Runtime registration -------------------------------------------- */

esp_err_t esp_partition_register_external(void *flash_chip,
                                           size_t offset, size_t size,
                                           const char *label,
                                           esp_partition_type_t    type,
                                           esp_partition_subtype_t subtype,
                                           const esp_partition_t **out_partition)
{
    (void)flash_chip;
#if VSF_ESPIDF_CFG_PARTITION_DYNAMIC != ENABLED
    (void)offset; (void)size; (void)label; (void)type; (void)subtype;
    if (out_partition != NULL) {
        *out_partition = NULL;
    }
    return ESP_ERR_INVALID_STATE;
#else
    if (!__part_state.is_inited || (__part_state.root_mal == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((label == NULL) || (size == 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Reject duplicate label. */
    if (esp_partition_find_first(ESP_PARTITION_TYPE_ANY,
                                 ESP_PARTITION_SUBTYPE_ANY, label) != NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    __part_slot_t *slot = __part_slot_alloc_dynamic();
    if (slot == NULL) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t rc = __part_init_slot(slot, label, (uint8_t)type,
                                    (uint8_t)subtype,
                                    (uint32_t)offset, (uint32_t)size,
                                    0, false, false, true);
    if (rc != ESP_OK) {
        return rc;
    }
    if (out_partition != NULL) {
        *out_partition = &slot->pub;
    }
    return ESP_OK;
#endif
}

esp_err_t esp_partition_deregister_external(const esp_partition_t *partition)
{
#if VSF_ESPIDF_CFG_PARTITION_DYNAMIC != ENABLED
    (void)partition;
    return ESP_ERR_INVALID_STATE;
#else
    if (partition == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (uint16_t i = __PART_STATIC_MAX;
         i < __PART_STATIC_MAX + __PART_DYN_MAX; i++) {
        __part_slot_t *slot = &__part_state.slots[i];
        if (slot->in_use && (&slot->pub == partition)) {
            vk_mal_fini((vk_mal_t *)&slot->mim);
            memset(slot, 0, sizeof(*slot));
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
#endif
}

#endif      /* VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_PARTITION */
