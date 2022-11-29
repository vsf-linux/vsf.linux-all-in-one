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

#if     VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_SIMPLE_LIBC == ENABLED        \
    &&  VSF_LINUX_USE_SIMPLE_TIME == ENABLED

// for itimers
#define __VSF_LINUX_CLASS_INHERIT__
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "../../include/unistd.h"
#   include "../../include/sys/time.h"
#   include "../../include/simple_libc/time.h"
#else
#   include <unistd.h>
#   include <sys/time.h>
#   include <time.h>
#endif

#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

struct vsf_linux_glibc_time_t {
    struct tm tm;
} static __vsf_linux_glibc_time;

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

time_t mktime(struct tm *tm)
{
    static const uint16_t __yday_month[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    time_t t;

    // tm_year in struct tm starts from 1900, time_t is calculated from 1970
    VSF_LINUX_ASSERT(tm->tm_year >= 70);
    int year = tm->tm_year - 70;
    int leap_years = year / 4;
    leap_years -= year / 100;
    leap_years += (year + 300) / 400;

    t = year * 365 + leap_years;
    t += __yday_month[tm->tm_mon] + tm->tm_mday - 1;
    t *= (time_t)86400;
    t += (((tm->tm_hour * 60) + tm->tm_min) * 60) + tm->tm_sec;
    return t;
}

time_t time(time_t *t)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (t != NULL) {
        *t = tv.tv_sec;
    }
    return tv.tv_sec;
}

#if __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#ifndef __WIN__
size_t strftime(char *str, size_t maxsize, const char *format, const struct tm *tm)
{
    static const char *__day_name[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *__mon_name[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

    VSF_LINUX_ASSERT((str != NULL) && (format != NULL) && (tm != NULL) && (maxsize > 0));
    VSF_LINUX_ASSERT(tm->tm_wday >= 0 && tm->tm_wday < 7);
    VSF_LINUX_ASSERT(tm->tm_mon >= 0 && tm->tm_mon < 12);
    char *str_end = str + maxsize - 1, *str_cur = str;
    char *curfmt, *subfmt, ch;
    ssize_t curlen;
    int value, valuelen;

    while ((ch = *format++) != '\0') {
        if (ch == '%') {
            curfmt = subfmt = NULL;
            curlen = -1;
            valuelen = 2;
            switch (*format++) {
            case '%':
                curfmt = "%";
                break;
            case 'a':
                curlen = 3;
            case 'A':
                curfmt = (char *)__day_name[tm->tm_wday];
                break;
            case 'b':
                curlen = 3;
            case 'B':
                curfmt = (char *)__mon_name[tm->tm_mon];
                break;
            case 'C':
                value = tm->tm_year / 100;
                break;
            case 'c':
                subfmt = "%a %b %e %H:%M:%S %Y";
                break;
            case 'd':
            case 'e':
                value = tm->tm_mday;
                break;
            case 'H':
                value = tm->tm_hour;
                break;
            case 'I':
                if (!(value = tm->tm_hour % 12)) {
                    value = 12;
                }
                break;
            case 'j':
                value = tm->tm_yday + 1;
                valuelen = 3;
                break;
            case 'm':
                value = tm->tm_mon + 1;
                break;
            case 'M':
                value = tm->tm_min;
                break;
            case 'p':
                curfmt = (tm->tm_hour > 11) ? "PM" : "AM";
                break;
            case 'S':
                value = tm->tm_sec;
                break;
            case 'U':
                value = (tm->tm_yday + 7 - tm->tm_wday) / 7;
                break;
            case 'w':
                value = tm->tm_wday;
                valuelen = 1;
                break;
            case 'W':
                value = tm->tm_yday + 7 - (tm->tm_wday ? (tm->tm_wday - 1) : 6) / 7;
                break;
            case 'x':
                subfmt = "%m/%d/%y";
                break;
            case 'X':
                subfmt = "%H:%M:%S";
                break;
            case 'y':
                value = tm->tm_year % 100;
                break;
            case 'Y':
                value = tm->tm_year + 1900;
                valuelen = 4;
                break;
            default:
                vsf_trace_error("invalid fmt %c in strftime" VSF_TRACE_CFG_LINEEND, ch);
                return 0;
            }
            if (subfmt != NULL) {
                curlen = strftime(str_cur, str_end - str_cur, subfmt, tm);
                if (!curlen) {
                    break;
                }
                str_cur += curlen;
            } else if (curfmt != NULL) {
                while (((ch = *curfmt++) != '\0') && (curlen < 0 || (curlen-- > 0))) {
                    *str_cur++ = ch;
                }
            } else if (str_cur + valuelen >= str_end) {
                break;
            } else {
                int pos = -1, digit;
                str_cur += valuelen;
                while (valuelen-- > 0) {
                    digit = value % 10;
                    value = value / 10;
                    str_cur[pos--] = '0' + digit;
                }
            }
        } else if (str_cur >= str_end) {
            break;
        } else {
            *str_cur++ = ch;
        }
    }
    *str_cur = '\0';
    return str_cur - str;
}
#endif

#if __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#endif

static char * __asctime_r(const struct tm *tm, char *buf, size_t buflen)
{
    VSF_LINUX_ASSERT(tm->tm_wday >= 0 && tm->tm_wday < 7);
    VSF_LINUX_ASSERT(tm->tm_mon >= 0 && tm->tm_mon < 12);
    int n = strftime(buf, buflen, "%c", tm);
    return n < 0 ? NULL : buf;
}

char * asctime_r(const struct tm *tm, char *buf)
{
    return __asctime_r(tm, buf, 26);
}

char * asctime(const struct tm *tm)
{
    // format:      "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n\0"
    static char result[3+1+ 3+20+1+20+1+20+1+20+1+20+2];
    return __asctime_r(tm, result, sizeof(result));
}

char * ctime(const time_t *t)
{
    return asctime(localtime(t));
}

char * ctime_r(const time_t *t, char *buf)
{
    return asctime_r(localtime_r(t, &(struct tm){ 0 }), buf);
}

struct tm * gmtime_r(const time_t *timep, struct tm *result)
{
    static const uint16_t __lyday_month[13] = {-1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
    static const uint16_t __yday_month[13] = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};
    VSF_LINUX_ASSERT((timep != NULL) && (result != NULL));
    memset(result, 0, sizeof(struct tm));

    time_t caltime = *timep;
    bool is_leap_year = false;
    const uint16_t *yday_month;

#define SECONDS_PER_DAY             (24 * 60 * 60)
#define SECONDS_PER_YEAR            (365 * SECONDS_PER_DAY)
#define SECONDS_PER_4YEARS          ((4 * 365 + 1) * SECONDS_PER_DAY)
#define IS_LEAP_YEAR(__Y)           ((!((__Y) % 4) && ((__Y) % 100)) || !(((__Y) + 1900) % 400))
#define LEAP_YEARS_SINCE(__Y)       ((((__Y) - 1) / 4) - (((__Y) - 1) / 100) + (((__Y) + 299) / 400) - 17)

    int tmp = caltime / SECONDS_PER_YEAR;
    caltime = caltime % SECONDS_PER_YEAR;
    tmp += 70;
    caltime -= LEAP_YEARS_SINCE(tmp) * SECONDS_PER_DAY;
    if (caltime < 0) {
        caltime += SECONDS_PER_YEAR;
        tmp--;
        if (IS_LEAP_YEAR(tmp)) {
            caltime += SECONDS_PER_DAY;
            is_leap_year = true;
        }
    } else if (IS_LEAP_YEAR(tmp)) {
        is_leap_year = true;
    }

    result->tm_year = tmp;
    result->tm_yday = caltime / SECONDS_PER_DAY;
    caltime %= SECONDS_PER_DAY;

    yday_month = is_leap_year ? __lyday_month : __yday_month;
    for (tmp = 1; yday_month[tmp] < result->tm_yday; tmp++);

    result->tm_mon = --tmp;
    result->tm_mday = result->tm_yday - yday_month[tmp];
    result->tm_wday = ((*timep / SECONDS_PER_DAY) + 4/* wday of 01-01-1970 is 4*/) % 7;
    result->tm_hour = caltime / 3600;
    caltime = caltime % 3600;
    result->tm_min = caltime / 60;
    result->tm_sec = caltime % 60;

    return result;
}

struct tm * gmtime(const time_t *timep)
{
    return gmtime_r(timep, &__vsf_linux_glibc_time.tm);
}

struct tm * localtime_r(const time_t *timep, struct tm *result)
{
    return gmtime_r(timep, result);
}

struct tm * localtime(const time_t *timep)
{
    return localtime_r(timep, &__vsf_linux_glibc_time.tm);
}

int nanosleep(const struct timespec *requested_time, struct timespec *remaining)
{
    vsf_timeout_tick_t ticks;
    ticks = 1000ULL * 1000 * requested_time->tv_sec + requested_time->tv_nsec / 1000;
    ticks = vsf_systimer_us_to_tick(ticks);

    vsf_systimer_tick_t remain_ticks = vsf_linux_sleep(ticks);

    if (remaining != NULL) {
        vsf_timeout_tick_t us = vsf_systimer_tick_to_us(remain_ticks);
        remaining->tv_sec = us / (1000 * 1000);
        remaining->tv_nsec = (us % (1000 * 1000)) * 1000;
    }
    return 0;
}

clock_t clock(void)
{
    return vsf_systimer_get_ms();
}

int clock_getres(clockid_t clk_id, struct timespec *res)
{
    switch (clk_id) {
    case CLOCK_MONOTONIC:
        if (res != NULL) {
            res->tv_sec = 0;
            res->tv_nsec = 1000;
        }
        return 0;
    default:
        return -1;
    }
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    switch (clk_id) {
    case CLOCK_MONOTONIC: {
            vsf_systimer_tick_t us = vsf_systimer_get_us();
            time_t sec = us / 1000000;
            us = us % 1000000;
            tp->tv_sec = sec;
            tp->tv_nsec = us * 1000;
        }
        return 0;
    default:
        return -1;
    }
}

int clock_nanosleep(clockid_t clockid, int flags, const struct timespec *request,
                    struct timespec *remain)
{
    return nanosleep(request, remain);
}

#if __IS_COMPILER_LLVM__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wvisibility"
#elif __IS_COMPILER_IAR__
//! statement is unreachable
#   pragma diag_suppress=pe111
#endif

static void __vsf_linux_timespec2timeval(struct timespec *spec, struct timeval *val)
{
    val->tv_sec = spec->tv_sec;
    val->tv_usec = spec->tv_nsec / 1000;
}

static void __vsf_linux_itimerspec2itimerval(struct itimerspec *spec, struct itimerval *val)
{
    __vsf_linux_timespec2timeval(&spec->it_interval, &val->it_interval);
    __vsf_linux_timespec2timeval(&spec->it_value, &val->it_value);
}

static void __vsf_linux_timeval2timespec(struct timeval *val, struct timespec *spec)
{
    spec->tv_sec = val->tv_sec;
    spec->tv_nsec = val->tv_usec * 1000;
}

static void __vsf_linux_itimerval2itimerspec(struct itimerval *val, struct itimerspec *spec)
{
    __vsf_linux_timeval2timespec(&val->it_interval, &spec->it_interval);
    __vsf_linux_timeval2timespec(&val->it_value, &spec->it_value);
}

static void __vsf_linux_prepare_timer(vsf_linux_timer_t *linux_timer)
{
    vsf_systimer_tick_t ticks = linux_timer->value.it_value.tv_sec * 1000 * 1000
                            +   linux_timer->value.it_value.tv_nsec / 1000;
    ticks = vsf_systimer_us_to_tick(ticks);
    if (ticks != 0) {
        linux_timer->start = vsf_systimer_get_tick();
        vsf_callback_timer_add(&linux_timer->timer, ticks);
    }
}

static void __vsf_linux_on_timer(vsf_callback_timer_t *timer)
{
    vsf_linux_timer_t *linux_timer = container_of(timer, vsf_linux_timer_t, timer);
    linux_timer->overrun++;

    switch (linux_timer->evt.sigev_notify) {
    case SIGEV_SIGNAL:
        kill(linux_timer->evt.sigev_notify_thread_id, linux_timer->evt.sigev_signo);
        break;
    case SIGEV_THREAD:
        // TODO: add support
        VSF_LINUX_ASSERT(false);
        break;
    }

    linux_timer->value.it_value = linux_timer->value.it_interval;
    __vsf_linux_prepare_timer(linux_timer);
}

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);
    VSF_LINUX_ASSERT(which < ITIMER_NUM);
    vsf_linux_timer_t *linux_timer = &process->timers[which];

    struct itimerspec new_spec;
    struct itimerspec old_spec;
    int result;

    __vsf_linux_itimerval2itimerspec((struct itimerval *)new_value, &new_spec);
    linux_timer->evt.sigev_notify = SIGEV_SIGNAL;
    linux_timer->evt.sigev_signo = SIGALRM;
    result = timer_settime((void *)linux_timer, 0, (const struct itimerspec *)&new_spec, &old_spec);

    if (old_value != NULL) {
        __vsf_linux_itimerspec2itimerval(&old_spec, old_value);
    }
    return result;
}

int getitimer(int which, struct itimerval *curr_value)
{
    if (curr_value != NULL) {
        vsf_linux_process_t *process = vsf_linux_get_cur_process();
        VSF_LINUX_ASSERT(process != NULL);
        VSF_LINUX_ASSERT(which < ITIMER_NUM);
        vsf_linux_timer_t *linux_timer = &process->timers[which];

        struct itimerspec curr_spec;
        timer_gettime((void *)linux_timer, &curr_spec);
        __vsf_linux_itimerspec2itimerval(&curr_spec, curr_value);
    }
    return 0;
}

int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    vsf_linux_timer_t *linux_timer = malloc(sizeof(vsf_linux_timer_t));
    if (NULL == linux_timer) {
        return -1;
    }

    if (NULL == sevp) {
        linux_timer->evt.sigev_notify = SIGEV_SIGNAL;
        linux_timer->evt.sigev_signo = SIGALRM;
    } else {
        linux_timer->evt = *sevp;
    }
    linux_timer->evt.sigev_notify_thread_id = process->id.pid;
    *timerid = linux_timer;
    return 0;
}

int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,
        struct itimerspec *old_value)
{
    vsf_linux_timer_t *linux_timer = timerid;
    if (old_value != NULL) {
        *old_value = linux_timer->value;
    }
    linux_timer->value = *new_value;

    vsf_callback_timer_init(&linux_timer->timer);
    linux_timer->timer.on_timer = __vsf_linux_on_timer;
    __vsf_linux_prepare_timer(linux_timer);
    return 0;
}

int timer_gettime(timer_t timerid, struct itimerspec *curr_value)
{
    if (curr_value != NULL) {
        vsf_linux_timer_t *linux_timer = timerid;
        vsf_systimer_tick_t elapsed_value = vsf_systimer_get_elapsed(linux_timer->start);
        elapsed_value = vsf_systimer_tick_to_us(elapsed_value);

        curr_value->it_interval = linux_timer->value.it_interval;
        curr_value->it_value.tv_sec = elapsed_value / (1000 * 1000);
        curr_value->it_value.tv_nsec = (elapsed_value % (1000 * 1000)) * 1000;
    }
    return 0;
}

int timer_delete(timer_t timerid)
{
    vsf_linux_timer_t *linux_timer = timerid;
    vsf_callback_timer_remove(&linux_timer->timer);
    free(linux_timer);
    return 0;
}

int timer_getoverrun(timer_t timerid)
{
    vsf_linux_timer_t *timer = timerid;
    return timer->overrun;
}

#if __IS_COMPILER_LLVM__
#   pragma clang diagnostic pop
#endif

#if VSF_LINUX_APPLET_USE_LIBC_TIME == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_LIBC_TIME_FUNC(__FUNC)          .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_libc_time_vplt_t vsf_linux_libc_time_vplt = {
    .info.entry_num = (sizeof(vsf_linux_libc_time_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_LIBC_TIME_FUNC(clock),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(clock_gettime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(clock_getres),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(clock_nanosleep),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(time),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(difftime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(asctime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(asctime_r),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(ctime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(ctime_r),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(gmtime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(gmtime_r),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(localtime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(localtime_r),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(mktime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(strftime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(nanosleep),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(timer_create),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(timer_settime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(timer_gettime),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(timer_delete),
    VSF_LINUX_APPLET_LIBC_TIME_FUNC(timer_getoverrun),
};
#endif

#endif      // VSF_KERNEL_CFG_EDA_SUPPORT_TIMER
#endif      // VSF_USE_LINUX && VSF_LINUX_USE_SIMPLE_LIBC && VSF_LINUX_USE_SIMPLE_TIME
