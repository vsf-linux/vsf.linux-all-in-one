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

#include "../../vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_SIMPLE_LIBC == ENABLED

#define __VSF_LINUX_CLASS_INHERIT__
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "../../include/unistd.h"
#   include "../../include/errno.h"
#   include "../../include/simple_libc/mntent.h"
#   include "../../include/simple_libc/dlfcn.h"
#else
#   include <unistd.h>
#   include <errno.h>
#   include <mntent.h>
#   include <dlfcn.h>
#endif
#include <stdio.h>

#if VSF_LINUX_APPLET_USE_LIBC_SETJMP == ENABLED && !defined(__VSF_APPLET__)
#   include <setjmp.h>
#   define __SIMPLE_LIBC_SETJMP_VPLT_ONLY__
#   include "../../include/simple_libc/setjmp/setjmp.h"
#endif
#if VSF_LINUX_APPLET_USE_LIBC_MATH == ENABLED && !defined(__VSF_APPLET__)
#   include <math.h>
#   define __SIMPLE_LIBC_MATH_VPLT_ONLY__
#   include "../../include/simple_libc/math/math.h"
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

// mntent

FILE * setmntent(const char *filename, const char *type)
{
    VSF_LINUX_ASSERT(false);
    return (FILE *)NULL;
}

struct mntent * getmntent(FILE *stream)
{
    VSF_LINUX_ASSERT(false);
    return (struct mntent *)NULL;
}

int addmntent(FILE *stream, const struct mntent *mnt)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

int endmntent(FILE *stream)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

char * hasmntopt(const struct mntent *mnt, const char *opt)
{
    VSF_LINUX_ASSERT(false);
    return (char *)NULL;
}

struct mntent * getmntent_r(FILE *stream, struct mntent *mntbuf, char *buf, int buflen)
{
    VSF_LINUX_ASSERT(false);
    return (struct mntent *)NULL;
}

// dlfcn

static void * __dlmalloc(int size)
{
    return malloc((size_t)size);
}

void * dlopen(const char *pathname, int mode)
{
#if VSF_USE_LOADER == ENABLED
    FILE *f = fopen(pathname, "r");
    if (NULL == f) {
        return NULL;
    }

    vsf_linux_dynloader_t *linux_loader = calloc(1, sizeof(vsf_linux_dynloader_t));
    if (NULL == linux_loader) {
        goto close_and_fail;
    }

    linux_loader->loader.generic.heap_op    = &vsf_loader_default_heap_op;
    linux_loader->loader.generic.vplt       = (void *)&vsf_linux_vplt;
    linux_loader->loader.generic.alloc_vplt = __dlmalloc;
    linux_loader->loader.generic.free_vplt  = free;
    linux_loader->target.object             = (uintptr_t)f;
    linux_loader->target.support_xip        = false;
    linux_loader->target.fn_read            = vsf_loader_stdio_read;

    uint8_t header[16];
    uint32_t size = vsf_loader_read(&linux_loader->target, 0, header, sizeof(header));
#if VSF_LOADER_USE_PE == ENABLED
    if ((size >= 2) && (header[0] == 'M') && (header[1] == 'Z')) {
        linux_loader->loader.generic.op     = &vsf_peloader_op;
    } else
#endif
    if ((size >= 4) && (header[0] == 0x7F) && (header[1] == 'E') && (header[2] == 'L') && (header[3] == 'F')) {
        linux_loader->loader.generic.op     = &vsf_elfloader_op;
    } else {
        printf("unsupported file format\n");
        goto close_and_fail;
    }

    if (!vsf_loader_load(&linux_loader->loader.generic, &linux_loader->target)) {
        vsf_loader_call_init_array(&linux_loader->loader.generic);
        return linux_loader;
    }

    vsf_loader_cleanup(&linux_loader->loader.generic);
    free(linux_loader);

close_and_fail:
    fclose(f);
    return NULL;
#else
    return NULL;
#endif
}

int dlclose(void *handle)
{
#if VSF_USE_LOADER == ENABLED
    vsf_linux_dynloader_t *linux_loader = handle;
    vsf_loader_call_fini_array(&linux_loader->loader.generic);
    vsf_loader_cleanup(&linux_loader->loader.generic);
    free(linux_loader);
    return 0;
#else
    return -1;
#endif
}

void * dlsym(void *handle, const char *name)
{
    void *vplt = NULL;

    if (RTLD_DEFAULT == handle) {
#if VSF_USE_APPLET == ENABLED
        vplt = (void *)&vsf_vplt;
#endif
    } else {
#if VSF_USE_APPLET == ENABLED && VSF_LINUX_USE_APPLET == ENABLED && VSF_APPLET_CFG_LINKABLE == ENABLED
        vsf_linux_dynloader_t *linux_loader = handle;
        vplt = (void*)linux_loader->loader.generic.vplt_out;
#endif
    }
    if (NULL == vplt) {
        return NULL;
    }

#if VSF_USE_APPLET == ENABLED && VSF_LINUX_USE_APPLET == ENABLED && VSF_APPLET_CFG_LINKABLE == ENABLED
    return vsf_vplt_link(vplt, (char *)name);
#else
    return NULL;
#endif
}

char * dlerror(void)
{
    return "known";
}

#if VSF_LINUX_APPLET_USE_LIBC_SETJMP == ENABLED && !defined(__VSF_APPLET__)
__VSF_VPLT_DECORATOR__ vsf_linux_libc_setjmp_vplt_t vsf_linux_libc_setjmp_vplt = {
    VSF_APPLET_VPLT_INFO(vsf_linux_libc_setjmp_vplt_t, 0, 0, true),

#ifdef VSF_ARCH_SETJMP
    VSF_APPLET_VPLT_ENTRY_FUNC_EX(fn_setjmp, "setjmp", VSF_ARCH_SETJMP),
#else
    VSF_APPLET_VPLT_ENTRY_FUNC(setjmp),
#endif
#ifdef VSF_ARCH_LONGJMP
    VSF_APPLET_VPLT_ENTRY_FUNC_EX(fn_longjmp, "longjmp", VSF_ARCH_LONGJMP),
#else
    VSF_APPLET_VPLT_ENTRY_FUNC(longjmp),
#endif
};
#endif

#if VSF_LINUX_APPLET_USE_LIBC_MATH == ENABLED && !defined(__VSF_APPLET__)
__VSF_VPLT_DECORATOR__ vsf_linux_libc_math_vplt_t vsf_linux_libc_math_vplt = {
    VSF_APPLET_VPLT_INFO(vsf_linux_libc_math_vplt_t, 0, 0, true),

    VSF_APPLET_VPLT_ENTRY_FUNC(atan),
    VSF_APPLET_VPLT_ENTRY_FUNC(cos),
    VSF_APPLET_VPLT_ENTRY_FUNC(sin),
    VSF_APPLET_VPLT_ENTRY_FUNC(tan),
    VSF_APPLET_VPLT_ENTRY_FUNC(tanh),
    VSF_APPLET_VPLT_ENTRY_FUNC(frexp),
    VSF_APPLET_VPLT_ENTRY_FUNC(modf),
    VSF_APPLET_VPLT_ENTRY_FUNC(ceil),
    VSF_APPLET_VPLT_ENTRY_FUNC(fabs),
    VSF_APPLET_VPLT_ENTRY_FUNC(floor),
    VSF_APPLET_VPLT_ENTRY_FUNC(acos),
    VSF_APPLET_VPLT_ENTRY_FUNC(asin),
    VSF_APPLET_VPLT_ENTRY_FUNC(atan2),
    VSF_APPLET_VPLT_ENTRY_FUNC(cosh),
    VSF_APPLET_VPLT_ENTRY_FUNC(sinh),
    VSF_APPLET_VPLT_ENTRY_FUNC(exp),
    VSF_APPLET_VPLT_ENTRY_FUNC(ldexp),
    VSF_APPLET_VPLT_ENTRY_FUNC(log),
    VSF_APPLET_VPLT_ENTRY_FUNC(log10),
    VSF_APPLET_VPLT_ENTRY_FUNC(pow),
    VSF_APPLET_VPLT_ENTRY_FUNC(sqrt),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmod),
    VSF_APPLET_VPLT_ENTRY_FUNC(nan),
    VSF_APPLET_VPLT_ENTRY_FUNC(copysign),
    VSF_APPLET_VPLT_ENTRY_FUNC(logb),
    VSF_APPLET_VPLT_ENTRY_FUNC(ilogb),
    VSF_APPLET_VPLT_ENTRY_FUNC(asinh),
    VSF_APPLET_VPLT_ENTRY_FUNC(cbrt),
    VSF_APPLET_VPLT_ENTRY_FUNC(nextafter),
    VSF_APPLET_VPLT_ENTRY_FUNC(rint),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalbn),
    VSF_APPLET_VPLT_ENTRY_FUNC(exp2),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalbln),
    VSF_APPLET_VPLT_ENTRY_FUNC(tgamma),
    VSF_APPLET_VPLT_ENTRY_FUNC(nearbyint),
    VSF_APPLET_VPLT_ENTRY_FUNC(lrint),
    VSF_APPLET_VPLT_ENTRY_FUNC(llrint),
    VSF_APPLET_VPLT_ENTRY_FUNC(round),
    VSF_APPLET_VPLT_ENTRY_FUNC(lround),
    VSF_APPLET_VPLT_ENTRY_FUNC(llround),
    VSF_APPLET_VPLT_ENTRY_FUNC(trunc),
    VSF_APPLET_VPLT_ENTRY_FUNC(remquo),
    VSF_APPLET_VPLT_ENTRY_FUNC(fdim),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmax),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmin),
    VSF_APPLET_VPLT_ENTRY_FUNC(fma),
    VSF_APPLET_VPLT_ENTRY_FUNC(log1p),
    VSF_APPLET_VPLT_ENTRY_FUNC(expm1),
    VSF_APPLET_VPLT_ENTRY_FUNC(acosh),
    VSF_APPLET_VPLT_ENTRY_FUNC(atanh),
    VSF_APPLET_VPLT_ENTRY_FUNC(remainder),
    VSF_APPLET_VPLT_ENTRY_FUNC(lgamma),
    VSF_APPLET_VPLT_ENTRY_FUNC(erf),
    VSF_APPLET_VPLT_ENTRY_FUNC(erfc),
    VSF_APPLET_VPLT_ENTRY_FUNC(log2),
    VSF_APPLET_VPLT_ENTRY_FUNC(hypot),
    VSF_APPLET_VPLT_ENTRY_FUNC(atanf),
    VSF_APPLET_VPLT_ENTRY_FUNC(cosf),
    VSF_APPLET_VPLT_ENTRY_FUNC(sinf),
    VSF_APPLET_VPLT_ENTRY_FUNC(tanf),
    VSF_APPLET_VPLT_ENTRY_FUNC(tanhf),
    VSF_APPLET_VPLT_ENTRY_FUNC(modff),
    VSF_APPLET_VPLT_ENTRY_FUNC(ceilf),
    VSF_APPLET_VPLT_ENTRY_FUNC(fabsf),
    VSF_APPLET_VPLT_ENTRY_FUNC(floorf),
    VSF_APPLET_VPLT_ENTRY_FUNC(acosf),
    VSF_APPLET_VPLT_ENTRY_FUNC(asinf),
    VSF_APPLET_VPLT_ENTRY_FUNC(atan2f),
    VSF_APPLET_VPLT_ENTRY_FUNC(coshf),
    VSF_APPLET_VPLT_ENTRY_FUNC(sinhf),
    VSF_APPLET_VPLT_ENTRY_FUNC(expf),
    VSF_APPLET_VPLT_ENTRY_FUNC(ldexpf),
    VSF_APPLET_VPLT_ENTRY_FUNC(logf),
    VSF_APPLET_VPLT_ENTRY_FUNC(log10f),
    VSF_APPLET_VPLT_ENTRY_FUNC(powf),
    VSF_APPLET_VPLT_ENTRY_FUNC(sqrtf),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmodf),
    VSF_APPLET_VPLT_ENTRY_FUNC(exp2f),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalblnf),
    VSF_APPLET_VPLT_ENTRY_FUNC(tgammaf),
    VSF_APPLET_VPLT_ENTRY_FUNC(nearbyintf),
    VSF_APPLET_VPLT_ENTRY_FUNC(lrintf),
    VSF_APPLET_VPLT_ENTRY_FUNC(llrintf),
    VSF_APPLET_VPLT_ENTRY_FUNC(roundf),
    VSF_APPLET_VPLT_ENTRY_FUNC(lroundf),
    VSF_APPLET_VPLT_ENTRY_FUNC(llroundf),
    VSF_APPLET_VPLT_ENTRY_FUNC(truncf),
    VSF_APPLET_VPLT_ENTRY_FUNC(remquof),
    VSF_APPLET_VPLT_ENTRY_FUNC(fdimf),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmaxf),
    VSF_APPLET_VPLT_ENTRY_FUNC(fminf),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmaf),
    VSF_APPLET_VPLT_ENTRY_FUNC(nanf),
    VSF_APPLET_VPLT_ENTRY_FUNC(copysignf),
    VSF_APPLET_VPLT_ENTRY_FUNC(logbf),
    VSF_APPLET_VPLT_ENTRY_FUNC(ilogbf),
    VSF_APPLET_VPLT_ENTRY_FUNC(asinhf),
    VSF_APPLET_VPLT_ENTRY_FUNC(cbrtf),
    VSF_APPLET_VPLT_ENTRY_FUNC(nextafterf),
    VSF_APPLET_VPLT_ENTRY_FUNC(rintf),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalbnf),
    VSF_APPLET_VPLT_ENTRY_FUNC(log1pf),
    VSF_APPLET_VPLT_ENTRY_FUNC(expm1f),
    VSF_APPLET_VPLT_ENTRY_FUNC(acoshf),
    VSF_APPLET_VPLT_ENTRY_FUNC(atanhf),
    VSF_APPLET_VPLT_ENTRY_FUNC(remainderf),
    VSF_APPLET_VPLT_ENTRY_FUNC(lgammaf),
    VSF_APPLET_VPLT_ENTRY_FUNC(erff),
    VSF_APPLET_VPLT_ENTRY_FUNC(erfcf),
    VSF_APPLET_VPLT_ENTRY_FUNC(log2f),
    VSF_APPLET_VPLT_ENTRY_FUNC(hypotf),
    VSF_APPLET_VPLT_ENTRY_FUNC(atanl),
    VSF_APPLET_VPLT_ENTRY_FUNC(cosl),
    VSF_APPLET_VPLT_ENTRY_FUNC(sinl),
    VSF_APPLET_VPLT_ENTRY_FUNC(tanl),
    VSF_APPLET_VPLT_ENTRY_FUNC(frexpl),
    VSF_APPLET_VPLT_ENTRY_FUNC(fabsl),
    VSF_APPLET_VPLT_ENTRY_FUNC(log1pl),
    VSF_APPLET_VPLT_ENTRY_FUNC(expm1l),
    VSF_APPLET_VPLT_ENTRY_FUNC(atan2l),
    VSF_APPLET_VPLT_ENTRY_FUNC(coshl),
    VSF_APPLET_VPLT_ENTRY_FUNC(sinhl),
    VSF_APPLET_VPLT_ENTRY_FUNC(expl),
    VSF_APPLET_VPLT_ENTRY_FUNC(ldexpl),
    VSF_APPLET_VPLT_ENTRY_FUNC(logl),
    VSF_APPLET_VPLT_ENTRY_FUNC(powl),
    VSF_APPLET_VPLT_ENTRY_FUNC(sqrtl),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmodl),
    VSF_APPLET_VPLT_ENTRY_FUNC(hypotl),
    VSF_APPLET_VPLT_ENTRY_FUNC(copysignl),
    VSF_APPLET_VPLT_ENTRY_FUNC(nanl),
    VSF_APPLET_VPLT_ENTRY_FUNC(ilogbl),
    VSF_APPLET_VPLT_ENTRY_FUNC(asinhl),
    VSF_APPLET_VPLT_ENTRY_FUNC(cbrtl),
    VSF_APPLET_VPLT_ENTRY_FUNC(nextafterl),
    VSF_APPLET_VPLT_ENTRY_FUNC(nexttowardf),
    VSF_APPLET_VPLT_ENTRY_FUNC(nexttoward),
    VSF_APPLET_VPLT_ENTRY_FUNC(nexttowardl),
    VSF_APPLET_VPLT_ENTRY_FUNC(logbl),
    VSF_APPLET_VPLT_ENTRY_FUNC(log2l),
    VSF_APPLET_VPLT_ENTRY_FUNC(rintl),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalbnl),
    VSF_APPLET_VPLT_ENTRY_FUNC(exp2l),
    VSF_APPLET_VPLT_ENTRY_FUNC(scalblnl),
    VSF_APPLET_VPLT_ENTRY_FUNC(tgammal),
    VSF_APPLET_VPLT_ENTRY_FUNC(nearbyintl),
    VSF_APPLET_VPLT_ENTRY_FUNC(lrintl),
    VSF_APPLET_VPLT_ENTRY_FUNC(llrintl),
    VSF_APPLET_VPLT_ENTRY_FUNC(roundl),
    VSF_APPLET_VPLT_ENTRY_FUNC(lroundl),
    VSF_APPLET_VPLT_ENTRY_FUNC(llroundl),
    VSF_APPLET_VPLT_ENTRY_FUNC(truncl),
    VSF_APPLET_VPLT_ENTRY_FUNC(remquol),
    VSF_APPLET_VPLT_ENTRY_FUNC(fdiml),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmaxl),
    VSF_APPLET_VPLT_ENTRY_FUNC(fminl),
    VSF_APPLET_VPLT_ENTRY_FUNC(fmal),
    VSF_APPLET_VPLT_ENTRY_FUNC(acoshl),
    VSF_APPLET_VPLT_ENTRY_FUNC(atanhl),
    VSF_APPLET_VPLT_ENTRY_FUNC(remainderl),
    VSF_APPLET_VPLT_ENTRY_FUNC(lgammal),
    VSF_APPLET_VPLT_ENTRY_FUNC(erfl),
    VSF_APPLET_VPLT_ENTRY_FUNC(erfcl),

    VSF_APPLET_VPLT_ENTRY_FUNC(frexpf),
    VSF_APPLET_VPLT_ENTRY_FUNC(log10l),
    VSF_APPLET_VPLT_ENTRY_FUNC(tanhl),
    VSF_APPLET_VPLT_ENTRY_FUNC(modfl),
    VSF_APPLET_VPLT_ENTRY_FUNC(ceill),
    VSF_APPLET_VPLT_ENTRY_FUNC(floorl),
    VSF_APPLET_VPLT_ENTRY_FUNC(acosl),
    VSF_APPLET_VPLT_ENTRY_FUNC(asinl),
};
#endif

void vsf_linux_glibc_init(void)
{

}

#endif      // VSF_USE_LINUX && VSF_LINUX_USE_SIMPLE_LIBC
