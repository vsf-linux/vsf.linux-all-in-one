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

#include "kernel/vsf_kernel.h"
#include "utilities/compiler/compiler.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#if __IS_COMPILER_IAR__ || __IS_COMILER_GCC__
char * strcasestr(const char *str, const char *substr)
{
    char c, sc;
    size_t len;

    if ((c = *substr++) != 0) {
        c = (char)tolower((unsigned char)c);
        len = strlen(substr);
        do {
            do {
                if ((sc = *str++) == 0) {
                    return NULL;
                }
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(str, substr, len) != 0);
        str--;
    }
    return ((char *)str);
}
#endif

#if __IS_COMPILER_ARM_COMPILER_5__ || __IS_COMPILER_ARM_COMPILER_6__
size_t strnlen(const char *str, size_t maxlen)
{
	const char * sc;

	for (sc = str; maxlen-- && *sc != '\0'; ++sc);
	return sc - str;
}

VSF_CAL_WEAK(itoa)
char * itoa(int num, char *str, int radix)
{
    char index[] = "0123456789ABCDEF";
    unsigned unum;
    int i = 0, j, k;

    if (radix == 10 && num < 0) {
        unum = (unsigned)-num;
        str[i++] = '-';
    } else {
        unum = (unsigned)num;
    }

    do{
        str[i++] = index[unum % (unsigned)radix];
        unum /= radix;
   } while (unum);

    str[i] = '\0';
    if (str[0] == '-') {
        k = 1;
    } else {
        k = 0;
    }

    for (j = k; j <= (i - 1) / 2; j++) {
        char temp;
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }
    return str;
}
#endif

#if __IS_COMPILER_IAR__
size_t strlcpy(char *dst, const char *src, size_t dsize)
{
    const char *osrc = src;
    size_t nleft = dsize;
    /* Copy as many bytes as will fit. */
    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }
    /* Not enough room in dst, add NUL and traverse rest of src. */
    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0';        /* NUL-terminate dst */
        while (*src++)
            ;
    }
    return(src - osrc - 1);     /* count does not include NUL */
}

size_t strlcat(char *dst, const char *src, size_t dsize)
{
    char *d = dst;
    const char *s = src;
    size_t n = dsize;
    size_t dlen;

    while (n-- != 0 && *d != '\0') {
        d++;
    }
    dlen = d - dst;
    n = dsize - dlen;

    if (n == 0) {
        return (dlen + strlen(s));
    }

    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));
}

char * strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

// implement APIs not supported in time.h in IAR
#   if !(VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_SIMPLE_LIBC == ENABLED && VSF_LINUX_USE_SIMPLE_TIME == ENABLED)
#       if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED && VSF_KERNEL_CFG_SUPPORT_THREAD == ENABLED
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    switch (clk_id) {
    case CLOCK_MONOTONIC: {
            vsf_systimer_tick_t us = vsf_systimer_get_us();
            tp->tv_sec = us / 1000000;
            tp->tv_nsec = us * 1000;
        }
        return 0;
    default:
        return -1;
    }
}

int nanosleep(const struct timespec *requested_time, struct timespec *remaining)
{
    vsf_timeout_tick_t ticks;
    ticks = 1000ULL * 1000 * requested_time->tv_sec + requested_time->tv_nsec / 1000;
    ticks = vsf_systimer_us_to_tick(ticks);

    vsf_thread_delay(ticks);

    if (remaining != NULL) {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    return 0;
}
#       endif           // VSF_KERNEL_CFG_EDA_SUPPORT_TIMER
#   endif

#if !(VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_SIMPLE_LIBC == ENABLED && VSF_LINUX_USE_SIMPLE_STDIO == ENABLED) && _DLIB_FILE_DESCRIPTOR
int fseeko(FILE *f, off_t offset, int whence)
{
    return fseek(f, (long)offset, whence);
}

off_t ftello(FILE *f)
{
    return (off_t)ftell(f);
}

int fseeko64(FILE *f, off64_t offset, int whence)
{
    return fseek(f, (long)offset, whence);
}

off64_t ftello64(FILE *f)
{
    return (off_t)ftell(f);
}
#endif      // !(VSF_USE_LINUX && VSF_LINUX_USE_SIMPLE_LIBC && VSF_LINUX_USE_SIMPLE_STDIO)

#endif