#ifndef __VSF_LINUX_SYS_TYPES_H__
#define __VSF_LINUX_SYS_TYPES_H__

#include "shell/sys/linux/vsf_linux_cfg.h"

// do not include vsf_utilities.h, because type.h is actually belong to compiler layer,
//  and compiler layer can not include vsf_utilities.h directly
//#include "utilities/vsf_utilities.h"
#define __VSF_HEADER_ONLY_SHOW_COMPILER_INFO__
#include "utilities/compiler/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __IS_COMPILER_IAR__
//! begin of typedef name has already been declared (with same type)
#   pragma diag_suppress=pe301
#endif

#if defined(__CPU_X64__) || defined(__CPU_WEBASSEMBLY__)
typedef long long           ssize_t;
#   ifdef __WIN__
typedef unsigned long long  size_t;
#   else
typedef unsigned long int   size_t;
#   endif
#else
#   ifdef __VSF64__
typedef long long           ssize_t;
typedef unsigned long long  size_t;
#   ifndef SSIZE_MAX
#       define SSIZE_MAX    LLONG_MAX
#   endif
#   else
typedef int                 ssize_t;
typedef unsigned int        size_t;
#   ifndef SSIZE_MAX
#       define SSIZE_MAX    INT_MAX
#   endif
#   endif
#endif

typedef unsigned char       u_char;
typedef unsigned short      u_short;
typedef unsigned long       u_long;
typedef unsigned int        u_int;

typedef int                 key_t;
typedef int                 pid_t;
typedef int                 id_t;
typedef unsigned int        mode_t;
typedef unsigned short      umode_t;
typedef unsigned int        nlink_t;
typedef unsigned int        uid_t;
typedef unsigned int        gid_t;
#ifdef __VSF64__
typedef long long           off_t;
#else
typedef long                off_t;
#endif
typedef long long           off64_t;
typedef unsigned long       dev_t;
typedef unsigned long       ino_t;
typedef unsigned long long  ino64_t;
typedef unsigned int        blksiz_t;
typedef unsigned long       blkcnt_t;
typedef unsigned long long  blkcnt64_t;
typedef void *              timer_t;
typedef blkcnt_t            fsblkcnt_t;
typedef blkcnt64_t          fsblkcnt64_t;
typedef unsigned long       fsfilcnt_t;
typedef unsigned long long  fsfilcnt64_t;

typedef char *              caddr_t;

typedef enum {
    P_ALL,
    P_PGID,
    P_PID,
} idtype_t;

#if (VSF_LINUX_USE_SIMPLE_LIBC == ENABLED) && (VSF_LINUX_USE_SIMPLE_TIME == ENABLED)
#ifndef __TIME_T
#   define __TIME_T         long long
#endif
typedef __TIME_T            time_t;

#ifndef __CLOCK_T
#   define __CLOCK_T        long
#endif
typedef __CLOCK_T           clock_t;
#ifndef __USECONDS_T
#   define __USECONDS_T     unsigned long
#endif
typedef __USECONDS_T        useconds_t;

typedef enum {
    CLOCK_MONOTONIC,
    CLOCK_REALTIME,
} clockid_t;

#define CLOCKS_PER_SEC      ((clock_t)1000000)
#endif

#if __IS_COMPILER_IAR__
//! end of typedef name has already been declared (with same type)
//#   pragma diag_suppress=pe301
#endif

#ifdef __cplusplus
}
#endif

#endif
