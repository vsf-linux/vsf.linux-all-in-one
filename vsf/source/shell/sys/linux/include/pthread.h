#ifndef __VSF_LINUX_PTHREAD_H__
#define __VSF_LINUX_PTHREAD_H__

#include "shell/sys/linux/vsf_linux_cfg.h"
#include "vsf.h"

#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED && VSF_LINUX_USE_SIMPLE_TIME == ENABLED
#   include "./simple_libc/time.h"
#else
#   include <time.h>
#endif
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "./sched.h"
#else
#   include <sched.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if VSF_LINUX_CFG_WRAPPER == ENABLED
#define pthread_atfork                  VSF_LINUX_WRAPPER(pthread_atfork)
#define pthread_self                    VSF_LINUX_WRAPPER(pthread_self)
#define pthread_equal                   VSF_LINUX_WRAPPER(pthread_equal)
#define pthread_create                  VSF_LINUX_WRAPPER(pthread_create)
#define pthread_join                    VSF_LINUX_WRAPPER(pthread_join)
#define pthread_detach                  VSF_LINUX_WRAPPER(pthread_detach)
#define pthread_exit                    VSF_LINUX_WRAPPER(pthread_exit)
#define pthread_cancel                  VSF_LINUX_WRAPPER(pthread_cancel)
#define pthread_kill                    VSF_LINUX_WRAPPER(pthread_kill)
#define pthread_once                    VSF_LINUX_WRAPPER(pthread_once)
#define pthread_testcancel              VSF_LINUX_WRAPPER(pthread_testcancel)
#define pthread_setcancelstate          VSF_LINUX_WRAPPER(pthread_setcancelstate)
#define pthread_setcanceltype           VSF_LINUX_WRAPPER(pthread_setcanceltype)
#define pthread_setschedparam           VSF_LINUX_WRAPPER(pthread_setschedparam)
#define pthread_getschedparam           VSF_LINUX_WRAPPER(pthread_getschedparam)
#define pthread_cleanup_push            VSF_LINUX_WRAPPER(pthread_cleanup_push)
#define pthread_cleanup_pop             VSF_LINUX_WRAPPER(pthread_cleanup_pop)
#define pthread_attr_init               VSF_LINUX_WRAPPER(pthread_attr_init)
#define pthread_attr_destroy            VSF_LINUX_WRAPPER(pthread_attr_destroy)
#define pthread_attr_setstack           VSF_LINUX_WRAPPER(pthread_attr_setstack)
#define pthread_attr_getstack           VSF_LINUX_WRAPPER(pthread_attr_getstack)
#define pthread_attr_setstackaddr       VSF_LINUX_WRAPPER(pthread_attr_setstackaddr)
#define pthread_attr_getstackaddr       VSF_LINUX_WRAPPER(pthread_attr_getstackaddr)
#define pthread_attr_setstacksize       VSF_LINUX_WRAPPER(pthread_attr_setstacksize)
#define pthread_attr_getstacksize       VSF_LINUX_WRAPPER(pthread_attr_getstacksize)
#define pthread_attr_setguardsize       VSF_LINUX_WRAPPER(pthread_attr_setguardsize)
#define pthread_attr_getguardsize       VSF_LINUX_WRAPPER(pthread_attr_getguardsize)
#define pthread_attr_setdetachstate     VSF_LINUX_WRAPPER(pthread_attr_setdetachstate)
#define pthread_attr_getdetachstate     VSF_LINUX_WRAPPER(pthread_attr_getdetachstate)
#define pthread_attr_setinheritsched    VSF_LINUX_WRAPPER(pthread_attr_setinheritsched)
#define pthread_attr_getinheritsched    VSF_LINUX_WRAPPER(pthread_attr_getinheritsched)
#define pthread_attr_setschedparam      VSF_LINUX_WRAPPER(pthread_attr_setschedparam)
#define pthread_attr_getschedparam      VSF_LINUX_WRAPPER(pthread_attr_getschedparam)
#define pthread_attr_setschedpolicy     VSF_LINUX_WRAPPER(pthread_attr_setschedpolicy)
#define pthread_attr_getschedpolicy     VSF_LINUX_WRAPPER(pthread_attr_getschedpolicy)
#define pthread_attr_setscope           VSF_LINUX_WRAPPER(pthread_attr_setscope)
#define pthread_attr_getscope           VSF_LINUX_WRAPPER(pthread_attr_getscope)

#define pthread_key_create              VSF_LINUX_WRAPPER(pthread_key_create)
#define pthread_key_delete              VSF_LINUX_WRAPPER(pthread_key_delete)
#define pthread_setspecific             VSF_LINUX_WRAPPER(pthread_setspecific)
#define pthread_getspecific             VSF_LINUX_WRAPPER(pthread_getspecific)

#define pthread_mutex_init              VSF_LINUX_WRAPPER(pthread_mutex_init)
#define pthread_mutex_destroy           VSF_LINUX_WRAPPER(pthread_mutex_destroy)
#define pthread_mutex_lock              VSF_LINUX_WRAPPER(pthread_mutex_lock)
#define pthread_mutex_trylock           VSF_LINUX_WRAPPER(pthread_mutex_trylock)
#define pthread_mutex_unlock            VSF_LINUX_WRAPPER(pthread_mutex_unlock)
#define pthread_mutexattr_init          VSF_LINUX_WRAPPER(pthread_mutexattr_init)
#define pthread_mutexattr_destroy       VSF_LINUX_WRAPPER(pthread_mutexattr_destroy)
#define pthread_mutexattr_setpshared    VSF_LINUX_WRAPPER(pthread_mutexattr_setpshared)
#define pthread_mutexattr_getpshared    VSF_LINUX_WRAPPER(pthread_mutexattr_getpshared)
#define pthread_mutexattr_settype       VSF_LINUX_WRAPPER(pthread_mutexattr_settype)
#define pthread_mutexattr_gettype       VSF_LINUX_WRAPPER(pthread_mutexattr_gettype)

#define pthread_cond_init               VSF_LINUX_WRAPPER(pthread_cond_init)
#define pthread_cond_destroy            VSF_LINUX_WRAPPER(pthread_cond_destroy)
#define pthread_cond_signal             VSF_LINUX_WRAPPER(pthread_cond_signal)
#define pthread_cond_broadcast          VSF_LINUX_WRAPPER(pthread_cond_broadcast)
#define pthread_cond_wait               VSF_LINUX_WRAPPER(pthread_cond_wait)
#define pthread_cond_timedwait          VSF_LINUX_WRAPPER(pthread_cond_timedwait)
#define pthread_condattr_init           VSF_LINUX_WRAPPER(pthread_condattr_init)
#define pthread_condattr_destroy        VSF_LINUX_WRAPPER(pthread_condattr_destroy)
#define pthread_condattr_setpshared     VSF_LINUX_WRAPPER(pthread_condattr_setpshared)
#define pthread_condattr_getpshared     VSF_LINUX_WRAPPER(pthread_condattr_getpshared)
#define pthread_condattr_setclock       VSF_LINUX_WRAPPER(pthread_condattr_setclock)
#define pthread_condattr_getclock       VSF_LINUX_WRAPPER(pthread_condattr_getclock)

#define pthread_rwlock_init             VSF_LINUX_WRAPPER(pthread_rwlock_init)
#define pthread_rwlock_destroy          VSF_LINUX_WRAPPER(pthread_rwlock_destroy)
#define pthread_rwlock_rdlock           VSF_LINUX_WRAPPER(pthread_rwlock_rdlock)
#define pthread_rwlock_tryrdlock        VSF_LINUX_WRAPPER(pthread_rwlock_tryrdlock)
#define pthread_rwlock_timedrdlock      VSF_LINUX_WRAPPER(pthread_rwlock_timedrdlock)
#define pthread_rwlock_wrlock           VSF_LINUX_WRAPPER(pthread_rwlock_wrlock)
#define pthread_rwlock_trywrlock        VSF_LINUX_WRAPPER(pthread_rwlock_trywrlock)
#define pthread_rwlock_timedwrlock      VSF_LINUX_WRAPPER(pthread_rwlock_timedwrlock)
#define pthread_rwlock_unlock           VSF_LINUX_WRAPPER(pthread_rwlock_unlock)
#endif

// to use PTHREAD_MUTEX_INITIALIZER, __VSF_EDA_CLASS_INHERIT__ is needed or ooc is disabled
#if __IS_COMPILER_IAR__
#define PTHREAD_MUTEX_INITIALIZER       {                                       \
                                            .use_as__vsf_mutex_t.use_as__vsf_sync_t.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                            .use_as__vsf_mutex_t.use_as__vsf_sync_t.cur_union.bits.cur = 1 | VSF_SYNC_HAS_OWNER,\
                                        }
#define PTHREAD_COND_INITIALIZER        {                                       \
                                             .use_as__vsf_mutex_t.use_as__vsf_sync_t.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                        }
#else
#define PTHREAD_MUTEX_INITIALIZER       (pthread_mutex_t) {                     \
                                            .use_as__vsf_mutex_t.use_as__vsf_sync_t.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                            .use_as__vsf_mutex_t.use_as__vsf_sync_t.cur_union.bits.cur = 1 | VSF_SYNC_HAS_OWNER,\
                                        }
#define PTHREAD_COND_INITIALIZER        (pthread_cond_t) {                      \
                                             .use_as__vsf_mutex_t.use_as__vsf_sync_t.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                        }
#endif


typedef int pthread_key_t;

enum {
    // pshared
    PTHREAD_PROCESS_SHARED              = 0,
#define PTHREAD_PROCESS_SHARED          PTHREAD_PROCESS_SHARED
    PTHREAD_PROCESS_PRIVATE             = 1 << 0,
#define PTHREAD_PROCESS_PRIVATE         PTHREAD_PROCESS_PRIVATE

    PTHREAD_CREATE_JOINABLE             = 0,
#define PTHREAD_CREATE_JOINABLE         PTHREAD_CREATE_JOINABLE
    PTHREAD_CREATE_DETACHED             = 1,
#define PTHREAD_CREATE_DETACHED         PTHREAD_CREATE_DETACHED

    // mutex
    PTHREAD_MUTEX_ERRORCHECK            = 1 << 1,
    PTHREAD_MUTEX_RECURSIVE             = 1 << 2,
    PTHREAD_MUTEX_NORMAL                = 0,
    PTHREAD_MUTEX_DEFAULT               = 0,

    // cond
};
typedef struct {
    int                                 attr;
} pthread_mutexattr_t;
typedef struct pthread_mutex_t {
    implement(vsf_mutex_t)
    int                                 attr;
    int                                 recursive_cnt;
} pthread_mutex_t;

typedef vsf_trig_t pthread_cond_t;
typedef struct {
    int                                 attr;
    clockid_t                           clockid;
} pthread_condattr_t;

#define PTHREAD_RWLOCK_INITIALIZER      { 0 }
typedef struct pthread_rwlock_t {
    uint16_t rdref;
    uint16_t wrref;
    uint16_t rdpend;
    uint16_t wrpend;
    vsf_dlist_t rdlist;
    vsf_dlist_t wrlist;
    vsf_sync_t rdsync;
    vsf_sync_t wrsync;
} pthread_rwlock_t;
typedef struct {
    int                                 attr;
} pthread_rwlockattr_t;

typedef int pthread_t;
typedef struct pthread_once_t {
    pthread_mutex_t                     mutex;
    bool                                is_inited;
} pthread_once_t;
#if __IS_COMPILER_IAR__
#define PTHREAD_ONCE_INIT               {                                       \
                                            .mutex.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                            .mutex.cur_union.cur_value = 1 | VSF_SYNC_HAS_OWNER,\
                                            .is_inited = false,                 \
                                        }
#else
#define PTHREAD_ONCE_INIT               (pthread_once_t) {                      \
                                            .mutex.use_as__vsf_mutex_t.use_as__vsf_sync_t.max_union.max_value = 1 | VSF_SYNC_AUTO_RST,\
                                            .mutex.use_as__vsf_mutex_t.use_as__vsf_sync_t.cur_union.cur_value = 1 | VSF_SYNC_HAS_OWNER,\
                                            .is_inited = false,                 \
                                        }
#endif
typedef struct {
    int                                 detachstate;
    int                                 schedpolicy;
    struct sched_param                  schedparam;
    int                                 inheritsched;
    int                                 scope;
    size_t                              guardsize;
    void                               *stackaddr;
    size_t                              stacksize;
} pthread_attr_t;

enum {
    PTHREAD_CANCEL_ENABLE,              // default
    PTHREAD_CANCEL_DISABLE,
};
enum {
    PTHREAD_CANCEL_DEFERRED,            // default
    PTHREAD_CANCEL_ASYNCHRONOUS,
};

#ifndef PTHREAD_STACK_MIN
#   define PTHREAD_STACK_MIN            1024
#endif

#if VSF_LINUX_APPLET_USE_PTHREAD == ENABLED
typedef struct vsf_linux_pthread_vplt_t {
    vsf_vplt_info_t info;

    int (*pthread_key_create)(pthread_key_t *key, void (*destructor)(void*));
    int (*pthread_key_delete)(pthread_key_t key);
    int (*pthread_setspecific)(pthread_key_t key, const void *value);
    void * (*pthread_getspecific)(pthread_key_t key);

    int (*pthread_mutex_init)(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr);
    int (*pthread_mutex_destroy)(pthread_mutex_t *mutex);
    int (*pthread_mutex_lock)(pthread_mutex_t *mutex);
    int (*pthread_mutex_trylock)(pthread_mutex_t *mutex);
    int (*pthread_mutex_unlock)(pthread_mutex_t *mutex);
    int (*pthread_mutexattr_init)(pthread_mutexattr_t *mattr);
    int (*pthread_mutexattr_destroy)(pthread_mutexattr_t *mattr);
    int (*pthread_mutexattr_setpshared)(pthread_mutexattr_t *mattr, int pshared);
    int (*pthread_mutexattr_getpshared)(pthread_mutexattr_t *mattr, int *pshared);
    int (*pthread_mutexattr_settype)(pthread_mutexattr_t *mattr , int type);
    int (*pthread_mutexattr_gettype)(pthread_mutexattr_t *mattr , int *type);

    int (*pthread_cond_init)(pthread_cond_t *cond, const pthread_condattr_t *attr);
    int (*pthread_cond_destroy)(pthread_cond_t *cond);
    int (*pthread_cond_signal)(pthread_cond_t *cond);
    int (*pthread_cond_broadcast)(pthread_cond_t *cond);
    int (*pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
    int (*pthread_cond_timedwait)(pthread_cond_t *cond, pthread_mutex_t *mutex,
        const struct timespec *abstime);
    int (*pthread_condattr_init)(pthread_condattr_t *cattr);
    int (*pthread_condattr_destroy)(pthread_condattr_t *cattr);
    int (*pthread_condattr_setpshared)(pthread_condattr_t *cattr, int pshared);
    int (*pthread_condattr_getpshared)(pthread_condattr_t *cattr, int *pshared);
    int (*pthread_condattr_getclock)(const pthread_condattr_t *cattr, clockid_t *clock_id);
    int (*pthread_condattr_setclock)(pthread_condattr_t *cattr, clockid_t clock_id);

    int (*pthread_rwlock_init)(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
    int (*pthread_rwlock_destroy)(pthread_rwlock_t *rwlock);
    int (*pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock);
    int (*pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
    int (*pthread_rwlock_timedrdlock)(pthread_rwlock_t *rwlock, const struct timespec *abstime);
    int (*pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock);
    int (*pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock);
    int (*pthread_rwlock_timedwrlock)(pthread_rwlock_t *rwlock, const struct timespec *abstime);
    int (*pthread_rwlock_unlock)(pthread_rwlock_t *rwlock);

    int (*pthread_atfork)(void (*prepare)(void), void (*parent)(void), void (*child)(void));
    pthread_t (*pthread_self)(void);
    int (*pthread_equal)(pthread_t t1, pthread_t t2);
    int (*pthread_create)(pthread_t *tidp, const pthread_attr_t *attr, void * (*start_rtn)(void *), void *arg);
    int (*pthread_join)(pthread_t tid, void **retval);
    int (*pthread_detach)(pthread_t thread);
    void (*pthread_exit)(void *retval);
    int (*pthread_cancel)(pthread_t thread);
    int (*pthread_kill)(pthread_t thread, int sig);
    int (*pthread_once)(pthread_once_t *once_control, void (*init_routine)(void));
    void (*pthread_testcancel)(void);
    int (*pthread_setcancelstate)(int state, int *oldstate);
    int (*pthread_setcanceltype)(int type, int *oldtype);
    int (*pthread_setschedparam)(pthread_t thread, int policy, const struct sched_param *param);
    int (*pthread_getschedparam)(pthread_t thread, int *policy, struct sched_param *param);
    void (*pthread_cleanup_push)(void (*routine)(void *), void *arg);
    void (*pthread_cleanup_pop)(int execute);

    int (*pthread_attr_init)(pthread_attr_t *attr);
    int (*pthread_attr_destroy)(pthread_attr_t *attr);
    int (*pthread_attr_setstack)(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
    int (*pthread_attr_getstack)(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
    int (*pthread_attr_setstackaddr)(pthread_attr_t *attr, void *stackaddr);
    int (*pthread_attr_getstackaddr)(const pthread_attr_t *attr, void **stackaddr);
    int (*pthread_attr_setstacksize)(pthread_attr_t *attr, size_t stacksize);
    int (*pthread_attr_getstacksize)(const pthread_attr_t *attr, size_t *stacksize);
    int (*pthread_attr_setguardsize)(pthread_attr_t *attr, size_t guardsize);
    int (*pthread_attr_getguardsize)(const pthread_attr_t *attr, size_t *guardsize);
    int (*pthread_attr_setdetachstate)(pthread_attr_t *attr, int detachstate);
    int (*pthread_attr_getdetachstate)(const pthread_attr_t *attr, int *detachstate);
    int (*pthread_attr_setinheritsched)(pthread_attr_t *attr, int inheritsched);
    int (*pthread_attr_getinheritsched)(const pthread_attr_t *attr, int *inheritsched);
    int (*pthread_attr_setschedparam)(pthread_attr_t *attr, const struct sched_param *param);
    int (*pthread_attr_getschedparam)(pthread_attr_t *attr, struct sched_param *param);
    int (*pthread_attr_setschedpolicy)(pthread_attr_t *attr, int policy);
    int (*pthread_attr_getschedpolicy)(const pthread_attr_t *attr, int *policy);
    int (*pthread_attr_setscope)(pthread_attr_t *attr, int contentionscope);
    int (*pthread_attr_getscope)(const pthread_attr_t *attr, int *contentionscope);
} vsf_linux_pthread_vplt_t;
#   ifndef __VSF_APPLET__
extern __VSF_VPLT_DECORATOR__ vsf_linux_pthread_vplt_t vsf_linux_pthread_vplt;
#   endif
#endif

#if defined(__VSF_APPLET__) && VSF_LINUX_APPLET_USE_PTHREAD == ENABLED

#ifndef VSF_LINUX_APPLET_PTHREAD_VPLT
#   if VSF_LINUX_USE_APPLET == ENABLED
#       define VSF_LINUX_APPLET_PTHREAD_VPLT                                    \
            ((vsf_linux_pthread_vplt_t *)(VSF_LINUX_APPLET_VPLT->pthread))
#   else
#       define VSF_LINUX_APPLET_PTHREAD_VPLT                                    \
            ((vsf_linux_pthread_vplt_t *)vsf_vplt((void *)0))
#   endif
#endif

static inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_key_create(key, destructor);
}
static inline int pthread_key_delete(pthread_key_t key) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_key_delete(key);
}
static inline int pthread_setspecific(pthread_key_t key, const void *value) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_setspecific(key, value);
}
static inline void * pthread_getspecific(pthread_key_t key) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_getspecific(key);
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutex_init(mutex, mattr);
}
static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutex_destroy(mutex);
}
static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutex_lock(mutex);
}
static inline int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutex_trylock(mutex);
}
static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutex_unlock(mutex);
}
static inline int pthread_mutexattr_init(pthread_mutexattr_t *mattr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_init(mattr);
}
static inline int pthread_mutexattr_destroy(pthread_mutexattr_t *mattr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_destroy(mattr);
}
static inline int pthread_mutexattr_setpshared(pthread_mutexattr_t *mattr, int pshared) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_setpshared(mattr, pshared);
}
static inline int pthread_mutexattr_getpshared(pthread_mutexattr_t *mattr, int *pshared) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_getpshared(mattr, pshared);
}
static inline int pthread_mutexattr_settype(pthread_mutexattr_t *mattr , int type) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_settype(mattr, type);
}
static inline int pthread_mutexattr_gettype(pthread_mutexattr_t *mattr , int *type) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_mutexattr_gettype(mattr, type);
}

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_init(cond, attr);
}
static inline int pthread_cond_destroy(pthread_cond_t *cond) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_destroy(cond);
}
static inline int pthread_cond_signal(pthread_cond_t *cond) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_signal(cond);
}
static inline int pthread_cond_broadcast(pthread_cond_t *cond) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_broadcast(cond);
}
static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_wait(cond, mutex);
}
static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
        const struct timespec *abstime) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cond_timedwait(cond, mutex, abstime);
}
static inline int pthread_condattr_init(pthread_condattr_t *cattr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_init(cattr);
}
static inline int pthread_condattr_destroy(pthread_condattr_t *cattr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_destroy(cattr);
}
static inline int pthread_condattr_setpshared(pthread_condattr_t *cattr, int pshared) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_setpshared(cattr, pshared);
}
static inline int pthread_condattr_getpshared(pthread_condattr_t *cattr, int *pshared) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_getpshared(cattr, pshared);
}
static inline int pthread_condattr_getclock(const pthread_condattr_t *cattr, clockid_t *clock_id) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_getclock(cattr, clock_id);
}
static inline int pthread_condattr_setclock(pthread_condattr_t *cattr, clockid_t clock_id) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_condattr_setclock(cattr, clock_id);
}

static inline int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_init(rwlock, attr);
}
static inline int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_destroy(rwlock);
}
static inline int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_rdlock(rwlock);
}
static inline int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_tryrdlock(rwlock);
}
static inline int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abstime) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_timedrdlock(rwlock, abstime);
}
static inline int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_wrlock(rwlock);
}
static inline int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_trywrlock(rwlock);
}
static inline int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abstime) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_timedwrlock(rwlock, abstime);
}
static inline int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_rwlock_unlock(rwlock);
}

static inline int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void)) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_atfork(prepare, parent, child);
}
static inline pthread_t pthread_self(void) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_self();
}
static inline int pthread_equal(pthread_t t1, pthread_t t2) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_equal(t1, t2);
}
static inline int pthread_create(pthread_t *tidp, const pthread_attr_t *attr, void * (*start_rtn)(void *), void *arg) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_create(tidp, attr, start_rtn, arg);
}
static inline int pthread_join(pthread_t tid, void **retval) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_join(tid, retval);
}
static inline int pthread_detach(pthread_t thread) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_detach(thread);
}
static inline void pthread_exit(void *retval) {
    VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_exit(retval);
}
static inline int pthread_cancel(pthread_t thread) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cancel(thread);
}
static inline int pthread_kill(pthread_t thread, int sig) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_kill(thread, sig);
}
static inline int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_once(once_control, init_routine);
}
static inline void pthread_testcancel(void) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_testcancel();
}
static inline int pthread_setcancelstate(int state, int *oldstate) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_setcancelstate(state, oldstate);
}
static inline int pthread_setcanceltype(int type, int *oldtype) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_setcanceltype(type, oldtype);
}
static inline int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_setschedparam(thread, policy, param);
}
static inline int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_getschedparam(thread, policy, param);
}
static inline void pthread_cleanup_push(void (*routine)(void *), void *arg) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cleanup_push(rountine, arg);
}
static inline void pthread_cleanup_pop(int execute) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_cleanup_pop(execute);
}

static inline int pthread_attr_init(pthread_attr_t *attr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_init(attr);
}
static inline int pthread_attr_destroy(pthread_attr_t *attr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_destroy(attr);
}
static inline int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setstack(attr, stackaddr, stacksize);
}
static inline int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getstack(attr, stackaddr, stacksize);
}
static inline int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setstackaddr(attr, stackaddr);
}
static inline int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getstackaddr(attr, stackaddr);
}
static inline int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setstacksize(attr, stacksize);
}
static inline int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getstacksize(attr, stacksize);
}
static inline int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setguardsize(attr, guardsize);
}
static inline int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getguardsize(attr, guardsize);
}
static inline int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setdetachstate(attr, detachstate);
}
static inline int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getdetachstate(attr, detachstate);
}
static inline int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setinheritsched(attr, inheritsched);
}
static inline int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getinheritsched(attr, inheritsched);
}
static inline int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setschedparam(attr, param);
}
static inline int pthread_attr_getschedparam(pthread_attr_t *attr, struct sched_param *param) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getschedparam(attr, param);
}
static inline int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setschedpolicy(attr, policy);
}
static inline int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getschedpolicy(attr, policy);
}
static inline int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_setscope(attr, contentionscope);
}
static inline int pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope) {
    return VSF_LINUX_APPLET_PTHREAD_VPLT->pthread_attr_getscope(attr, contentionscope);
}

#else       // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_PTHREAD

#if VSF_LINUX_CFG_TLS_NUM > 0
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void * pthread_getspecific(pthread_key_t key);
#endif

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutexattr_init(pthread_mutexattr_t *mattr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *mattr);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *mattr, int pshared);
int pthread_mutexattr_getpshared(pthread_mutexattr_t *mattr, int *pshared);
int pthread_mutexattr_settype(pthread_mutexattr_t *mattr , int type);
int pthread_mutexattr_gettype(pthread_mutexattr_t *mattr , int *type);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
        const struct timespec *abstime);
int pthread_condattr_init(pthread_condattr_t *cattr);
int pthread_condattr_destroy(pthread_condattr_t *cattr);
int pthread_condattr_setpshared(pthread_condattr_t *cattr, int pshared);
int pthread_condattr_getpshared(pthread_condattr_t *cattr, int *pshared);
int pthread_condattr_getclock(const pthread_condattr_t *cattr, clockid_t *clock_id);
int pthread_condattr_setclock(pthread_condattr_t *cattr, clockid_t clock_id);

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);
int pthread_create(pthread_t *tidp, const pthread_attr_t *attr, void * (*start_rtn)(void *), void *arg);
int pthread_join(pthread_t tid, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval);
int pthread_cancel(pthread_t thread);
int pthread_kill(pthread_t thread, int sig);
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
void pthread_testcancel(void);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);
int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param);
void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);
int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched);
int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param);
int pthread_attr_getschedparam(pthread_attr_t *attr, struct sched_param *param);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);
int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope);
int pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope);

#endif      // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_PTHREAD

#ifdef __cplusplus
}
#endif

#endif
