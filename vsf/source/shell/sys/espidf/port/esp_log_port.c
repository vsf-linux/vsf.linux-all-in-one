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
 * Port implementation for "esp_log.h" on VSF.
 *
 * Design:
 * - Level filtering uses a fixed-size tag table. Tags are compared by
 *   pointer first (fast path for __FILE__-style string literals) and fall
 *   back to strcmp. Tag "*" stores the default. Overflow beyond the table
 *   capacity is not remembered, but the tag will still log at the default
 *   level (documented in esp_log_level_set()).
 * - Output is funneled through esp_log_writev(). If a user vprintf hook is
 *   installed via esp_log_set_vprintf(), the formatted line is delivered
 *   to that function. Otherwise the line is routed to vsf_trace with the
 *   level mapped to VSF_TRACE_ERROR/WARNING/INFO/DEBUG.
 * - Timestamp is derived from vsf_systimer via vsf_systimer_get_ms().
 */

/*============================ INCLUDES ======================================*/

#include "../vsf_espidf_cfg.h"

#if VSF_USE_ESPIDF == ENABLED && VSF_ESPIDF_CFG_USE_LOG == ENABLED

#include "esp_log.h"

#include "../vsf_espidf.h"
#include "kernel/vsf_kernel.h"
#include "service/trace/vsf_trace.h"

#include <stdio.h>
#include <string.h>

/*============================ MACROS ========================================*/

#ifndef VSF_ESPIDF_CFG_LOG_TAG_TABLE_SIZE
#   define VSF_ESPIDF_CFG_LOG_TAG_TABLE_SIZE    16
#endif

#ifndef VSF_ESPIDF_CFG_LOG_DEFAULT_LEVEL
#   define VSF_ESPIDF_CFG_LOG_DEFAULT_LEVEL     ESP_LOG_INFO
#endif

#ifndef VSF_ESPIDF_CFG_LOG_LINE_BUF_SIZE
#   define VSF_ESPIDF_CFG_LOG_LINE_BUF_SIZE     256
#endif

/*============================ TYPES =========================================*/

typedef struct {
    const char *tag;
    esp_log_level_t level;
} __vsf_espidf_log_tag_t;

/*============================ LOCAL VARIABLES ===============================*/

static __vsf_espidf_log_tag_t __vsf_espidf_log_tags[VSF_ESPIDF_CFG_LOG_TAG_TABLE_SIZE];
static uint8_t                __vsf_espidf_log_tag_num;
static esp_log_level_t        __vsf_espidf_log_default = VSF_ESPIDF_CFG_LOG_DEFAULT_LEVEL;
static vprintf_like_t         __vsf_espidf_log_hook;

/*============================ IMPLEMENTATION ================================*/

static __vsf_espidf_log_tag_t * __vsf_espidf_log_find(const char *tag)
{
    if (tag == NULL) {
        return NULL;
    }
    for (uint8_t i = 0; i < __vsf_espidf_log_tag_num; i++) {
        const char *t = __vsf_espidf_log_tags[i].tag;
        if ((t == tag) || ((t != NULL) && (strcmp(t, tag) == 0))) {
            return &__vsf_espidf_log_tags[i];
        }
    }
    return NULL;
}

void esp_log_level_set(const char *tag, esp_log_level_t level)
{
    if (tag == NULL) {
        return;
    }
    // "*" sets the fallback level for all unregistered tags.
    if ((tag[0] == '*') && (tag[1] == '\0')) {
        __vsf_espidf_log_default = level;
        return;
    }

    __vsf_espidf_log_tag_t *slot = __vsf_espidf_log_find(tag);
    if (slot != NULL) {
        slot->level = level;
        return;
    }
    if (__vsf_espidf_log_tag_num < VSF_ESPIDF_CFG_LOG_TAG_TABLE_SIZE) {
        slot = &__vsf_espidf_log_tags[__vsf_espidf_log_tag_num++];
        slot->tag = tag;
        slot->level = level;
    }
    // Silent overflow: unregistered tag keeps the default level.
}

esp_log_level_t esp_log_level_get(const char *tag)
{
    __vsf_espidf_log_tag_t *slot = __vsf_espidf_log_find(tag);
    return (slot != NULL) ? slot->level : __vsf_espidf_log_default;
}

vprintf_like_t esp_log_set_vprintf(vprintf_like_t func)
{
    vprintf_like_t prev = __vsf_espidf_log_hook;
    __vsf_espidf_log_hook = func;
    return prev;
}

uint32_t esp_log_timestamp(void)
{
    return (uint32_t)vsf_systimer_tick_to_ms(vsf_systimer_get_tick());
}

char * esp_log_system_timestamp(void)
{
    static char buf[16];
    uint32_t ms = esp_log_timestamp();
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)ms);
    return buf;
}

static vsf_trace_level_t __vsf_espidf_log_to_trace(esp_log_level_t level)
{
    switch (level) {
    case ESP_LOG_ERROR:                 return VSF_TRACE_ERROR;
    case ESP_LOG_WARN:                  return VSF_TRACE_WARNING;
    case ESP_LOG_INFO:                  return VSF_TRACE_INFO;
    case ESP_LOG_DEBUG:                 return VSF_TRACE_DEBUG;
    case ESP_LOG_VERBOSE:               return VSF_TRACE_DEBUG;
    default:                            return VSF_TRACE_INFO;
    }
}

static char __vsf_espidf_log_letter(esp_log_level_t level)
{
    switch (level) {
    case ESP_LOG_ERROR:                 return 'E';
    case ESP_LOG_WARN:                  return 'W';
    case ESP_LOG_INFO:                  return 'I';
    case ESP_LOG_DEBUG:                 return 'D';
    case ESP_LOG_VERBOSE:               return 'V';
    default:                            return '?';
    }
}

void esp_log_writev(esp_log_level_t level, const char *tag, const char *format, va_list args)
{
    if ((level == ESP_LOG_NONE) || (format == NULL)) {
        return;
    }
    if (level > esp_log_level_get(tag)) {
        return;
    }

    // User hook takes precedence. Hook receives caller-shaped format only,
    // matching ESP-IDF expectation (no injected prefix).
    if (__vsf_espidf_log_hook != NULL) {
        va_list copy;
        va_copy(copy, args);
        (void)__vsf_espidf_log_hook(format, copy);
        va_end(copy);
        return;
    }

    // Default route: format once into a line buffer, prepend a compact
    // "<L> <tag>: " prefix, terminate with VSF trace line-end, and submit
    // to vsf_trace_string to preserve level-colored output.
    char line[VSF_ESPIDF_CFG_LOG_LINE_BUF_SIZE];
    int prefix = snprintf(line, sizeof(line), "%c (%lu) %s: ",
                          __vsf_espidf_log_letter(level),
                          (unsigned long)esp_log_timestamp(),
                          (tag != NULL) ? tag : "?");
    if ((prefix < 0) || ((size_t)prefix >= sizeof(line))) {
        // Extremely long tag or timestamp; fall through with minimal prefix.
        prefix = snprintf(line, sizeof(line), "%c: ", __vsf_espidf_log_letter(level));
        if ((prefix < 0) || ((size_t)prefix >= sizeof(line))) {
            return;
        }
    }

    int body = vsnprintf(line + prefix, sizeof(line) - prefix, format, args);
    if (body < 0) {
        return;
    }
    size_t total = (size_t)prefix + (size_t)body;
    if (total >= sizeof(line)) {
        total = sizeof(line) - 1;
    }
    // Append VSF line-end if there is room; otherwise ensure NUL.
    const char *eol = VSF_TRACE_CFG_LINEEND;
    size_t eol_len = strlen(eol);
    if (total + eol_len < sizeof(line)) {
        memcpy(line + total, eol, eol_len);
        total += eol_len;
    }
    line[total] = '\0';

    vsf_trace_string(__vsf_espidf_log_to_trace(level), line);
}

void esp_log_write(esp_log_level_t level, const char *tag, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    esp_log_writev(level, tag, format, ap);
    va_end(ap);
}

#endif      // VSF_USE_ESPIDF && VSF_ESPIDF_CFG_USE_LOG
