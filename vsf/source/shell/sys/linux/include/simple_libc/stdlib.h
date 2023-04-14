#ifndef __SIMPLE_LIBC_STDLIB_H__
#define __SIMPLE_LIBC_STDLIB_H__

#include "shell/sys/linux/vsf_linux_cfg.h"

#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED && VSF_LINUX_USE_SIMPLE_LIBC == ENABLED
#   include "./stddef.h"
#else
#   include <stddef.h>
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if VSF_LINUX_LIBC_CFG_WRAPPER == ENABLED
#define malloc              VSF_LINUX_LIBC_WRAPPER(malloc)
#define aligned_alloc       VSF_LINUX_LIBC_WRAPPER(aligned_alloc)
#define realloc             VSF_LINUX_LIBC_WRAPPER(realloc)
#define free                VSF_LINUX_LIBC_WRAPPER(free)
#define calloc              VSF_LINUX_LIBC_WRAPPER(calloc)
#define memalign            VSF_LINUX_LIBC_WRAPPER(memalign)
#define posix_memalign      VSF_LINUX_LIBC_WRAPPER(posix_memalign)
#define malloc_usable_size  VSF_LINUX_LIBC_WRAPPER(malloc_usable_size)
#define exit                VSF_LINUX_LIBC_WRAPPER(exit)
#define atexit              VSF_LINUX_LIBC_WRAPPER(atexit)
#define system              VSF_LINUX_LIBC_WRAPPER(system)
#   if VSF_LINUX_LIBC_USE_ENVIRON
#define getenv              VSF_LINUX_LIBC_WRAPPER(getenv)
#define putenv              VSF_LINUX_LIBC_WRAPPER(putenv)
#define setenv              VSF_LINUX_LIBC_WRAPPER(setenv)
#define unsetenv            VSF_LINUX_LIBC_WRAPPER(unsetenv)
#   endif
#define mktemps             VSF_LINUX_LIBC_WRAPPER(mktemps)
#define mktemp              VSF_LINUX_LIBC_WRAPPER(mktemp)
#define mkstemp             VSF_LINUX_LIBC_WRAPPER(mkstemp)
#define mkostemp            VSF_LINUX_LIBC_WRAPPER(mkostemp)
#define mkstemps            VSF_LINUX_LIBC_WRAPPER(mkstemps)
#define mkostemps           VSF_LINUX_LIBC_WRAPPER(mkostemps)
#define mkdtemp             VSF_LINUX_LIBC_WRAPPER(mkdtemp)
#elif defined(__WIN__) && !defined(__VSF_APPLET__)
// avoid conflicts with APIs in ucrt
#define exit                VSF_LINUX_LIBC_WRAPPER(exit)
#define atexit              VSF_LINUX_LIBC_WRAPPER(atexit)
#define getenv              VSF_LINUX_LIBC_WRAPPER(getenv)
// system("chcp 65001"); will be called in debug_stream driver, wrapper here
#define system              VSF_LINUX_LIBC_WRAPPER(system)
#endif

// syscalls

#define __NR_exit           exit

#ifndef RAND_MAX
#   ifdef __WIN__
#       define RAND_MAX     0x7FFF
#   else
#       define RAND_MAX     0x7FFF
#   endif
#endif

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        -1

typedef struct {
  int quot;
  int rem;
} div_t;
typedef struct {
  long quot;
  long rem;
} ldiv_t;
typedef struct {
  long long quot;
  long long rem;
} lldiv_t;

#if VSF_LINUX_APPLET_USE_LIBC_STDLIB == ENABLED
typedef struct vsf_linux_libc_stdlib_vplt_t {
    vsf_vplt_info_t info;

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(malloc);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(realloc);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(free);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(aligned_alloc);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(calloc);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(memalign);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(posix_memalign);
    // malloc_usable_size should be in malloc.h
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(malloc_usable_size);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(putenv);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(getenv);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(setenv);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(unsetenv);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mktemps);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mktemp);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mkstemp);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mkostemp);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mkstemps);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mkostemps);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(mkdtemp);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(div);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(ldiv);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(lldiv);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(itoa);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(atoi);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(atol);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(atoll);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(atof);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtol);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtoul);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtoll);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtoull);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtof);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtod);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(strtold);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(bsearch);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(qsort);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(rand);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(srand);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(abort);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(system);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(exit);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(atexit);

    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(abs);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(labs);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(llabs);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(imaxabs);
    VSF_APPLET_VPLT_ENTRY_FUNC_DEF(getloadavg);
} vsf_linux_libc_stdlib_vplt_t;
#   ifndef __VSF_APPLET__
extern __VSF_VPLT_DECORATOR__ vsf_linux_libc_stdlib_vplt_t vsf_linux_libc_stdlib_vplt;
#   endif
#endif

#if defined(__VSF_APPLET__) && VSF_APPLET_CFG_ABI_PATCH != ENABLED && VSF_LINUX_APPLET_USE_LIBC_STDLIB == ENABLED

#ifndef VSF_LINUX_APPLET_LIBC_STDLIB_VPLT
#   if VSF_LINUX_USE_APPLET == ENABLED
#       define VSF_LINUX_APPLET_LIBC_STDLIB_VPLT                                \
            ((vsf_linux_libc_stdlib_vplt_t *)(VSF_LINUX_APPLET_VPLT->libc_stdlib_vplt))
#   else
#       define VSF_LINUX_APPLET_LIBC_STDLIB_VPLT                                \
            ((vsf_linux_libc_stdlib_vplt_t *)vsf_vplt((void *)0))
#   endif
#endif

#define VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(__NAME)                              \
            VSF_APPLET_VPLT_ENTRY_FUNC_ENTRY(VSF_LINUX_APPLET_LIBC_STDLIB_VPLT, __NAME)
#define VSF_LINUX_APPLET_LIBC_STDLIB_IMP(...)                                   \
            VSF_APPLET_VPLT_ENTRY_FUNC_IMP(VSF_LINUX_APPLET_LIBC_STDLIB_VPLT, __VA_ARGS__)

VSF_LINUX_APPLET_LIBC_STDLIB_IMP(malloc, void *, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(malloc)(size);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(realloc, void *, void *p, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(realloc)(p, size);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(free, void, void *p) {
    VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(free)(p);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(aligned_alloc, void *, size_t alignment, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(aligned_alloc)(alignment, size);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(calloc, void *, size_t n, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(calloc)(n, size);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(memalign, void *, size_t alignment, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(memalign)(alignment, size);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(posix_memalign, int, void **memptr, size_t alignment, size_t size) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(posix_memalign)(memptr, alignment, size);
}
// malloc_usable_size should be in malloc.h
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(malloc_usable_size, size_t, void *p) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(malloc_usable_size)(p);
}

#if VSF_LINUX_LIBC_USE_ENVIRON
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(putenv, int, char *string) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(putenv)(string);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(getenv, char *, const char *name) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(getenv)(name);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(setenv, int, const char *name, const char *value, int replace) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(setenv)(name, value, replace);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(unsetenv, int, const char *name) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(unsetenv)(name);
}
#endif
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mktemps, char *, char *template_str, int suffixlen) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mktemps)(template_str, suffixlen);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mktemp, char *, char *template_str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mktemp)(template_str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mkstemp, int, char *template_str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mkstemp)(template_str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mkostemp, int, char *template_str, int flags) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mkostemp)(template_str, flags);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mkstemps, int, char *template_str, int suffixlen) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mkstemps)(template_str, suffixlen);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mkostemps, int, char *template_str, int suffixlen, int flags) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mkostemps)(template_str, suffixlen, flags);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(mkdtemp, char *, char *template_str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(mkdtemp)(template_str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(div, div_t, int numer, int denom) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(div)(numer, denom);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(ldiv, ldiv_t, long int numer, long int denom) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(ldiv)(numer, denom);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(lldiv, lldiv_t, long long int numer, long long int denom) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(lldiv)(numer, denom);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(itoa, char *, int num, char *str, int radix) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(itoa)(num, str, radix);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(atoi, int, const char * str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(atoi)(str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(atol, long int, const char *str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(atol)(str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(atoll, long long int, const char *str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(atoll)(str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(atof, double, const char *str) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(atof)(str);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtol, long, const char *str, char **endptr, int base) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtol)(str, endptr, base);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtoul, unsigned long, const char *str, char **endptr, int base) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtoul)(str, endptr, base);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtoll, long long, const char *str, char **endptr, int base) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtoll)(str, endptr, base);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtoull, unsigned long long, const char *str, char **endptr, int base) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtoull)(str, endptr, base);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtof, float, const char *str, char **endptr) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtof)(str, endptr);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtod, double, const char *str, char **endptr) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtod)(str, endptr);
}
//VSF_LINUX_APPLET_LIBC_STDLIB_IMP(strtold, long double, const char *str, char **endptr) {
//    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(strtold)(str, endptr);
//}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(bsearch, void *, const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *)) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(bsearch)(key, base, nitems, size, compar);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(qsort, void, void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*)) {
    VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(qsort)(base, nitems, size, compar);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(rand, int, void) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(rand)();
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(srand, void, unsigned int seed) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(srand)(seed);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(srandom, void, unsigned int seed) {
    VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(srand)(seed);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(random, long int, void) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(rand)();
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(abort, void, void) {
    VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(abort)();
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(system, int, const char *command) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(system)(command);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(exit, void, int status) {
    VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(exit)(status);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(atexit, int, void (*func)(void)) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(atexit)(func);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(abs, int, int j) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(abs)(j);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(labs, long, long j) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(labs)(j);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(llabs, long long, long long j) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(llabs)(j);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(imaxabs, intmax_t, intmax_t j) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(imaxabs)(j);
}
VSF_LINUX_APPLET_LIBC_STDLIB_IMP(getloadavg, int, double loadavg[], int nelem) {
    return VSF_LINUX_APPLET_LIBC_STDLIB_ENTRY(getloadavg)(loadavg, nelem);
}

#else       // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_LIBC_STDLIB

void * malloc(size_t size);
void * realloc(void *p, size_t size);
void free(void *p);

void * aligned_alloc(size_t alignment, size_t size);
void * calloc(size_t n, size_t size);
void * memalign(size_t alignment, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size);

// malloc_usable_size should be in malloc.h
size_t malloc_usable_size(void *p);

#if VSF_LINUX_LIBC_USE_ENVIRON
int putenv(char *string);
char * getenv(const char *name);
int setenv(const char *name, const char *value, int replace);
int unsetenv(const char *name);
#endif
char * mktemps(char *template_str, int suffixlen);
char * mktemp(char *template_str);
int mkstemp(char *template_str);
int mkostemp(char *template_str, int flags);
int mkstemps(char *template_str, int suffixlen);
int mkostemps(char *template_str, int suffixlen, int flags);
char * mkdtemp(char *template_str);

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
lldiv_t lldiv(long long int numer, long long int denom);

char * itoa(int num, char *str, int radix);
int atoi(const char * str);
long int atol(const char *str);
long long int atoll(const char *str);
double atof(const char *str);
long strtol(const char *str, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
long long strtoll(const char *str, char **endptr, int base);
unsigned long long strtoull(const char *str, char **endptr, int base);
float strtof(const char *str, char **endptr);
double strtod(const char *str, char **endptr);
//long double strtold(const char *str, char **endptr);

void * bsearch(const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *));

int rand(void);
void srand(unsigned int seed);
void srandom(unsigned int seed);
long int random(void);

void qsort(void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*));

void abort(void);
int system(const char *command);

void exit(int status);
int atexit(void (*func)(void));

int abs(int j);
long labs(long j);
long long llabs(long long j);
intmax_t imaxabs(intmax_t j);

int getloadavg(double loadavg[], int nelem);

#endif      // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_LIBC_STDLIB

int at_quick_exit(void (*func)(void));
void quick_exit(int status);

int mblen(const char *str, size_t n);
// TODO: it seems that IAR does not support wchar_t even if it's defined in stddef.h
#if !__IS_COMPILER_IAR__
size_t mbstowcs(wchar_t *dst, const char *src, size_t len);
int mbtowc(wchar_t *pwc, const char *str, size_t n);
size_t wcstombs(char *str, const wchar_t *pwcs, size_t n);
int wctomb(char *str, wchar_t wchar);
#endif

// _exit should be implemented in retarget
void _exit(int status);
void _Exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif
