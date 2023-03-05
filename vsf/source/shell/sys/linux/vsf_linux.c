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

#include "./vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED

#define __VSF_EDA_CLASS_INHERIT__
#define __VSF_SIMPLE_STREAM_CLASS_INHERIT__
#define __VSF_FS_CLASS_INHERIT__
#define __VSF_LINUX_FS_CLASS_IMPLEMENT
#define __VSF_LINUX_CLASS_IMPLEMENT

#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "./include/unistd.h"
#   include "./include/sched.h"
#   include "./include/semaphore.h"
#   include "./include/signal.h"
#   include "./include/sys/wait.h"
#   include "./include/sys/ipc.h"
#   include "./include/sys/sem.h"
#   include "./include/sys/mman.h"
#   include "./include/sys/shm.h"
#   include "./include/sys/random.h"
#   include "./include/sys/stat.h"
#   include "./include/sys/times.h"
#   include "./include/sys/prctl.h"
#   include "./include/fcntl.h"
#   include "./include/errno.h"
#   include "./include/termios.h"
#   include "./include/pwd.h"
#   include "./include/sys/utsname.h"
#   include "./include/sys/ioctl.h"
#   include "./include/spawn.h"
#   include "./include/langinfo.h"
#   include "./include/poll.h"
#   include "./include/linux/limits.h"
#   include "./include/linux/futex.h"
#else
#   include <unistd.h>
#   include <sched.h>
#   include <semaphore.h>
#   include <signal.h>
#   include <sys/wait.h>
#   include <sys/ipc.h>
#   include <sys/sem.h>
#   include <sys/mman.h>
#   include <sys/shm.h>
#   include <sys/random.h>
#   include <sys/stat.h>
#   include <sys/times.h>
#   include <sys/prctl.h>
#   include <fcntl.h>
#   include <errno.h>
#   include <termios.h>
#   include <pwd.h>
#   include <sys/utsname.h>
#   include <sys/ioctl.h>
#   include <spawn.h>
#   include <langinfo.h>
#   include <poll.h>
// for MAX_PATH
#   include <linux/limits.h>
#   include <linux/futex.h>
#endif
#include <stdarg.h>
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED && VSF_LINUX_USE_SIMPLE_STDLIB == ENABLED
#   include "./include/simple_libc/stdlib.h"
#else
#   include <stdlib.h>
#endif
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED && VSF_LINUX_USE_SIMPLE_STRING == ENABLED
#   include "./include/simple_libc/string.h"
#else
#   include <string.h>
#endif
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED && VSF_LINUX_USE_SIMPLE_STDIO == ENABLED
#   include "./include/simple_libc/stdio.h"
#else
#   include <stdio.h>
#endif

#include "./kernel/fs/vsf_linux_fs.h"

#if __IS_COMPILER_IAR__
//! statement is unreachable
#   pragma diag_suppress=pe111
#endif

#if __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-align"
#endif

/*============================ MACROS ========================================*/

#if VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE != ENABLED
#   error VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE MUST be enbled
#endif

#if VSF_KERNEL_CFG_EDA_SUPPORT_SUB_CALL != ENABLED
#   error VSF_KERNEL_CFG_EDA_SUPPORT_SUB_CALL MUST be enabled
#endif

#if VSF_LINUX_CFG_STACKSIZE < 512
#   warning You sure use this stack size for linux?
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

#if VSF_LINUX_CFG_SHM_NUM > 0
dcl_vsf_bitmap(vsf_linux_shm_bitmap, VSF_LINUX_CFG_SHM_NUM);
typedef struct vsf_linux_shm_mem_t {
    key_t key;
    void *buffer;
    uint32_t size;
} vsf_linux_shm_mem_t;
#endif

#if VSF_LINUX_CFG_PLS_NUM > 0
dcl_vsf_bitmap(vsf_linux_pls_bitmap, VSF_LINUX_CFG_PLS_NUM);
#endif

#if VSF_LINUX_CFG_FUTEX_NUM > 0
typedef struct vsf_linux_futex_t {
    vsf_dlist_node_t node;
    uint32_t *futex;
    vsf_trig_t trig;
} vsf_linux_futex_t;

dcl_vsf_pool(vsf_linux_futex_pool)
def_vsf_pool(vsf_linux_futex_pool, vsf_linux_futex_t)
#endif

typedef struct vsf_linux_t {
    int cur_tid;
    int cur_pid;
    vsf_dlist_t process_list;

    vsf_linux_process_t process_for_resources;
    vsf_linux_process_t *kernel_process;

#if VSF_LINUX_CFG_SHM_NUM > 0
    struct {
        vsf_bitmap(vsf_linux_shm_bitmap) bitmap;
        vsf_linux_shm_mem_t mem[VSF_LINUX_CFG_SHM_NUM];
    } shm;
#endif

#if VSF_LINUX_CFG_PLS_NUM > 0
    struct {
        vsf_bitmap(vsf_linux_pls_bitmap) bitmap;
    } pls;
#endif

#if VSF_LINUX_CFG_FUTEX_NUM > 0
    struct {
        vsf_dlist_t list;
        vsf_pool(vsf_linux_futex_pool) pool;
    } futex;
#endif

    char hostname[HOST_NAME_MAX + 1];
} vsf_linux_t;

/*============================ GLOBAL VARIABLES ==============================*/

const struct passwd __vsf_default_passwd = {
    .pw_name            = "vsf",
    .pw_passwd          = "vsf",
    .pw_uid             = (uid_t)0,
    .pw_gid             = (gid_t)0,
    .pw_gecos           = "vsf",
    .pw_dir             = "/home",
    .pw_shell           = "vsh",
};

/*============================ PROTOTYPES ====================================*/

extern int vsf_linux_create_fhs(void);

#if VSF_LINUX_USE_SIMPLE_LIBC == ENABLED
extern void vsf_linux_glibc_init(void);
#endif

static void __vsf_linux_main_on_run(vsf_thread_cb_t *cb);
extern int __vsh_get_exe(char *pathname, int path_out_lenlen, char *cmd,
        vsf_linux_main_entry_t *entry);

#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
static void __vsf_linux_sighandler_on_run(vsf_thread_cb_t *cb);
#endif

// private APIs in other files
extern int __vsf_linux_fd_create_ex(vsf_linux_process_t *process, vsf_linux_fd_t **sfd,
            const vsf_linux_fd_op_t *op, int fd_desired, vsf_linux_fd_priv_t *priv);
extern vsf_linux_fd_t * __vsf_linux_fd_get_ex(vsf_linux_process_t *process, int fd);
extern void __vsf_linux_fd_delete_ex(vsf_linux_process_t *process, int fd);
extern int __vsf_linux_fd_close_ex(vsf_linux_process_t *process, int fd);
extern vk_file_t * __vsf_linux_get_fs_ex(vsf_linux_process_t *process, int fd);

extern void __vsf_linux_rx_stream_init(vsf_linux_stream_priv_t *priv_tx);
extern void __vsf_linux_tx_stream_init(vsf_linux_stream_priv_t *priv_rx);
extern void __vsf_linux_tx_stream_drop(vsf_linux_stream_priv_t *priv_tx);
extern void __vsf_linux_rx_stream_drop(vsf_linux_stream_priv_t *priv_rx);
extern void __vsf_linux_tx_stream_drain(vsf_linux_stream_priv_t *priv_tx);
extern void __vsf_linux_stream_evt(vsf_linux_stream_priv_t *priv, vsf_protect_t orig, short event, bool is_ready);
extern const vsf_linux_fd_op_t __vsf_linux_stream_fdop;

/*============================ LOCAL VARIABLES ===============================*/

static vsf_linux_t __vsf_linux = {
    .cur_pid            = -1,
};

static const vsf_linux_thread_op_t __vsf_linux_main_op = {
    .priv_size          = 0,
    .on_run             = __vsf_linux_main_on_run,
    .on_terminate       = vsf_linux_thread_on_terminate,
};

#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
static const vsf_linux_thread_op_t __vsf_linux_sighandler_op = {
    .priv_size          = 0,
    .on_run             = __vsf_linux_sighandler_on_run,
    .on_terminate       = vsf_linux_thread_on_terminate,
};
#endif

/*============================ IMPLEMENTATION ================================*/

#if VSF_LINUX_CFG_PLS_NUM > 0
int vsf_linux_pls_alloc(void)
{
    vsf_protect_t orig = vsf_protect_sched();
    int_fast32_t idx = vsf_bitmap_ffz(&__vsf_linux.pls.bitmap, VSF_LINUX_CFG_PLS_NUM);
    if (idx < 0) {
        vsf_unprotect_sched(orig);
        return -1;
    }
    vsf_bitmap_set(&__vsf_linux.pls.bitmap, idx);
    vsf_unprotect_sched(orig);
    return idx;
}

void vsf_linux_pls_free(int idx)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    vsf_protect_t orig = vsf_protect_sched();
        VSF_LINUX_ASSERT(vsf_bitmap_get(&__vsf_linux.pls.bitmap, idx));
        vsf_bitmap_clear(&__vsf_linux.pls.bitmap, idx);
        process->pls[idx].data = NULL;
        process->pls[idx].destructor = NULL;
    vsf_unprotect_sched(orig);
}

vsf_linux_localstorage_t * vsf_linux_pls_get(int idx)
{
    VSF_LINUX_ASSERT((idx >= 0) && (idx < VSF_LINUX_CFG_PLS_NUM));

    vsf_protect_t orig = vsf_protect_sched();
        VSF_LINUX_ASSERT(vsf_bitmap_get(&__vsf_linux.pls.bitmap, idx));
    vsf_unprotect_sched(orig);

    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);
    return &process->pls[idx];
}

vsf_err_t vsf_linux_library_init(int *lib_idx, void *lib_ctx, void (*destructor)(void *))
{
    VSF_LINUX_ASSERT(lib_idx != NULL);
    vsf_protect_t orig = vsf_protect_sched();
    if (*lib_idx < 0) {
        *lib_idx = vsf_linux_pls_alloc();
        if (*lib_idx < 0) {
            vsf_unprotect_sched(orig);
            vsf_trace_error("linux: fail to allocate pls for library" VSF_TRACE_CFG_LINEEND);
            return VSF_ERR_NOT_ENOUGH_RESOURCES;
        }
    }
    vsf_unprotect_sched(orig);

    vsf_linux_localstorage_t *pls = vsf_linux_pls_get(*lib_idx);
    VSF_LINUX_ASSERT(pls != NULL);
    if (pls->data != NULL) {
        vsf_trace_warning("linux: can not initiazlize a initizlized library" VSF_TRACE_CFG_LINEEND);
        return VSF_ERR_ALREADY_EXISTS;
    }
    pls->data = lib_ctx;
    pls->destructor = destructor;
    return VSF_ERR_NONE;
}

void * vsf_linux_library_ctx(int lib_idx)
{
    if (lib_idx < 0) {
        return NULL;
    }
    vsf_linux_localstorage_t *pls = vsf_linux_pls_get(lib_idx);
    VSF_LINUX_ASSERT(pls != NULL);
    return pls->data;
}

vsf_err_t vsf_linux_dynlib_init(int *lib_idx, int module_num, int bss_size)
{
    vsf_linux_dynlib_t *dynlib = calloc(1, sizeof(vsf_linux_dynlib_t) + module_num * sizeof(void *) + bss_size);
    if (NULL == dynlib) { return -1; }

    dynlib->module_num = module_num;
    dynlib->bss_size = bss_size;
    vsf_err_t err = vsf_linux_library_init(lib_idx, dynlib, free);
    if (err != VSF_ERR_NONE) {
        free(dynlib);
    }
    return err;
}

void * vsf_linux_dynlib_ctx(const vsf_linux_dynlib_mod_t *mod)
{
    VSF_LINUX_ASSERT(mod != NULL);
    vsf_linux_dynlib_t *dynlib = vsf_linux_library_ctx(*mod->lib_idx);

    if (NULL == dynlib) {
        if (vsf_linux_dynlib_init(mod->lib_idx, mod->module_num, mod->bss_size) < 0) {
            vsf_trace_error("linux: fail to allocate dynlib" VSF_TRACE_CFG_LINEEND);
            VSF_LINUX_ASSERT(false);
            return NULL;
        }
        dynlib = vsf_linux_library_ctx(*mod->lib_idx);
        VSF_LINUX_ASSERT(dynlib != NULL);
    }
    VSF_LINUX_ASSERT(mod->mod_idx < dynlib->module_num);

    void *ctx = dynlib->modules[mod->mod_idx];
    if (NULL == ctx) {
        uint32_t mod_size = (mod->mod_size + (sizeof(int) - 1)) & ~(sizeof(int) - 1);
        VSF_LINUX_ASSERT(dynlib->bss_size >= dynlib->bss_brk + mod_size);
        ctx = (void *)((uint8_t *)&dynlib->modules[dynlib->module_num] + dynlib->bss_brk);
        dynlib->bss_brk += mod_size;
        dynlib->modules[mod->mod_idx] = ctx;
        if (mod->init != NULL) {
            mod->init(ctx);
        }
    }
    return ctx;
}
#endif

#if VSF_LINUX_CFG_TLS_NUM > 0
int vsf_linux_tls_alloc(void (*destructor)(void*))
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    vsf_protect_t orig = vsf_protect_sched();
    int_fast32_t idx = vsf_bitmap_ffz(&process->tls.bitmap, VSF_LINUX_CFG_TLS_NUM);
    if (idx < 0) {
        vsf_unprotect_sched(orig);
        return -1;
    }
    vsf_bitmap_set(&process->tls.bitmap, idx);
    vsf_unprotect_sched(orig);

    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(thread != NULL);

    thread->tls[idx].destructor = destructor;
    thread->tls[idx].data = NULL;
    return idx;
}

void vsf_linux_tls_free(int idx)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    vsf_protect_t orig = vsf_protect_sched();
        VSF_LINUX_ASSERT(vsf_bitmap_get(&process->tls.bitmap, idx));
        vsf_bitmap_clear(&process->tls.bitmap, idx);
    vsf_unprotect_sched(orig);

    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(thread != NULL);

    thread->tls[idx].destructor = NULL;
    thread->tls[idx].data = NULL;
}

vsf_linux_localstorage_t * vsf_linux_tls_get(int idx)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    vsf_protect_t orig = vsf_protect_sched();
        VSF_LINUX_ASSERT(vsf_bitmap_get(&process->tls.bitmap, idx));
    vsf_unprotect_sched(orig);

    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(thread != NULL);

    return &thread->tls[idx];
}
#endif

#if VSF_LINUX_CFG_FUTEX_NUM > 0
//implement_vsf_pool(vsf_linux_futex_pool, vsf_linux_futex_t)
#define __name vsf_linux_futex_pool
#define __type vsf_linux_futex_t
#include "service/pool/impl_vsf_pool.inc"
#endif

#if VSF_LINUX_LIBC_USE_ENVIRON == ENABLED
int vsf_linux_merge_env(vsf_linux_process_t *process, char **env)
{
    extern int __putenv_ex(vsf_linux_process_t *process, char *string);
    char *cur_env;
    if (env != NULL) {
        while (*env != NULL) {
            cur_env = __strdup_ex(process, *env);
            if (cur_env != NULL) {
                __putenv_ex(process, cur_env);
            } else {
                vsf_trace_error("spawn: failed to dup env %s", VSF_TRACE_CFG_LINEEND, *env);
                return -1;
            }
            env++;
        }
    }
    return 0;
}

void vsf_linux_free_env(vsf_linux_process_t *process)
{
    char **environ_tmp = process->__environ;
    if (environ_tmp != NULL) {
        while (*environ_tmp != NULL) {
            __free_ex(process, *environ_tmp);
            environ_tmp++;
        }
        __free_ex(process, process->__environ);
    }
}
#endif

void vsf_linux_trigger_init(vsf_linux_trigger_t *trig)
{
    vsf_eda_trig_init(&trig->use_as__vsf_trig_t, false, true);
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    vsf_dlist_init_node(vsf_linux_trigger_t, node, trig);
    trig->pending_process = NULL;
#endif
}

int vsf_linux_trigger_signal(vsf_linux_trigger_t *trig, int sig)
{
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    vsf_linux_process_t *pending_process;
    vsf_protect_t orig = vsf_protect_sched();
    pending_process = trig->pending_process;
    if (NULL == pending_process) {
        vsf_unprotect_sched(orig);
        return 0;
    }

    trig->sig = sig;
    trig->pending_process = NULL;
    vsf_dlist_remove(vsf_linux_trigger_t, node, &pending_process->sig.trigger_list, trig);
    vsf_unprotect_sched(orig);
#endif
    vsf_eda_trig_set_isr(&trig->use_as__vsf_trig_t);
    return 0;
}

int vsf_linux_trigger_pend(vsf_linux_trigger_t *trig, vsf_timeout_tick_t timeout)
{
    vsf_protect_t orig;
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    trig->sig = 0;
    orig = vsf_protect_sched();
        vsf_dlist_add_to_tail(vsf_linux_trigger_t, node, &process->sig.trigger_list, trig);
        VSF_LINUX_ASSERT(NULL == trig->pending_process);
        trig->pending_process = process;
    vsf_unprotect_sched(orig);
#endif

    vsf_sync_reason_t r = vsf_thread_trig_pend(&trig->use_as__vsf_trig_t, timeout);
    orig = vsf_protect_sched();
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    trig->pending_process = NULL;
#endif
    if (VSF_SYNC_TIMEOUT == r) {
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
        vsf_dlist_remove(vsf_linux_trigger_t, node, &process->sig.trigger_list, trig);
#endif
        vsf_unprotect_sched(orig);
        return 1;
    }
    vsf_unprotect_sched(orig);
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    if (trig->sig) {
        errno = EINTR;
    }
    return trig->sig;
#else
    return 0;
#endif
}

WEAK(vsf_linux_create_fhs_user)
int vsf_linux_create_fhs_user(void)
{
    return 0;
}

WEAK(vsf_linux_create_fhs)
int vsf_linux_create_fhs(void)
{
    vsf_linux_vfs_init();
    busybox_install();
    return vsf_linux_create_fhs_user();
}

int * __vsf_linux_errno(void)
{
    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(thread != NULL);
    return &thread->__errno;
}

int vsf_linux_generate_path(char *path_out, int path_out_lenlen, char *dir, char *path_in)
{
    char working_dir[MAX_PATH];
    if (NULL == dir) {
        getcwd(working_dir, sizeof(working_dir));
        dir = working_dir;
    }

    if (path_in[0] == '/') {
        if (strlen(path_in) >= path_out_lenlen) {
            return -ENOMEM;
        }
        strcpy(path_out, path_in);
    } else {
        if (strlen(dir) + strlen(path_in) + 1 >= path_out_lenlen) {
            return -ENOMEM;
        }
        strcpy(path_out, dir);
        if (dir[strlen(dir) - 1] != '/') {
            strcat(path_out, "/");
        }
        strcat(path_out, path_in);
    }

    // process .. an .
    char *tmp, *tmp_replace;
    while ((tmp = (char *)strstr(path_out, "/..")) != NULL) {
        tmp[0] = '\0';
        tmp_replace = (char *)strrchr(path_out, '/');
        if (NULL == tmp_replace) {
            return -ENOENT;
        }
        strcpy(tmp_replace, &tmp[3]);
    }
    while ((tmp = (char *)strstr(path_out, "/./")) != NULL) {
        strcpy(tmp, &tmp[2]);
    }

    // fix surfix "/."
    size_t len = strlen(path_out);
    if ((len >= 2) && ('.' == path_out[len - 1]) && ('/' == path_out[len - 2])) {
        path_out[len - 2] = '\0';
    }
    return 0;
}

void __vsf_linux_process_free_arg(vsf_linux_process_t *process)
{
    vsf_linux_process_arg_t *arg = &process->ctx.arg;
    if (arg->is_dyn_argv) {
        arg->is_dyn_argv = false;
        for (int i = 0; i < arg->argc; i++) {
            __free_ex(process, (void *)arg->argv[i]);
            arg->argv[i] = NULL;
        }
    }
}

int __vsf_linux_process_parse_arg(vsf_linux_process_t *process, char * const * argv)
{
    vsf_linux_process_arg_t *arg = &process->ctx.arg;
    arg->is_dyn_argv = true;
    arg->argc = 0;
    while ((*argv != NULL) && (arg->argc <= VSF_LINUX_CFG_MAX_ARG_NUM)) {
        arg->argv[arg->argc] = __strdup_ex(process, *argv);
        if (NULL == arg->argv[arg->argc]) {
            vsf_trace_error("linux: fail to allocate space for %s" VSF_TRACE_CFG_LINEEND, *argv);
            return -1;
        }
        arg->argc++;
        argv++;
    }
    return 0;
}

#if VSF_ARCH_USE_THREAD_REG == ENABLED
static void __vsf_linux_thread_on_run(vsf_thread_cb_t *cb)
{
    vsf_linux_thread_t *thread = container_of(cb, vsf_linux_thread_t, use_as__vsf_thread_cb_t);
    vsf_linux_process_t *process = thread->process;

    VSF_LINUX_ASSERT(process != NULL);
    vsf_arch_set_thread_reg(process->reg);
    thread->op->on_run(cb);
}
#endif

static int __vsf_linux_init_thread(int argc, char *argv[])
{
    int err = vsf_linux_create_fhs();
    if (err) { return err; }
    return execlp(VSF_LINUX_CFG_BIN_PATH "/init", "init", NULL);
}

static int __vsf_linux_kernel_thread(int argc, char *argv[])
{
#if VSF_KERNEL_CFG_TRACE == ENABLED
    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    vsf_kernel_trace_eda_info(&thread->use_as__vsf_eda_t, "linux_kernel_thread",
                                thread->stack, thread->stack_size);
#endif

    __vsf_linux.kernel_process = vsf_linux_get_cur_process();
    __vsf_linux_init_thread(argc, argv);
    return 0;
}

vsf_linux_process_t * vsf_linux_resources_process(void)
{
    return &__vsf_linux.process_for_resources;
}


void * vsf_linux_malloc_res(size_t size)
{
    return __malloc_ex(vsf_linux_resources_process(), size);
}

void vsf_linux_free_res(void *ptr)
{
    __free_ex(vsf_linux_resources_process(), ptr);
}

static void __vsf_linux_stderr_on_evt(vsf_linux_stream_priv_t *priv, vsf_protect_t orig, short event, bool is_ready)
{
    if (is_ready) {
        vsf_linux_fd_set_status(&priv->use_as__vsf_linux_fd_priv_t, event, orig);
    } else {
        vsf_linux_fd_clear_status(&priv->use_as__vsf_linux_fd_priv_t, event, orig);
    }
    if (event & POLLIN) {
        vsf_linux_stream_priv_t *stdin_priv = priv->target;
        if (stdin_priv != NULL) {
            __vsf_linux_stream_evt(stdin_priv, vsf_protect_sched(), event, is_ready);
        }
    }
}

bool vsf_linux_is_inited(void)
{
    return __vsf_linux.cur_pid >= 0;
}

vsf_err_t vsf_linux_init(vsf_linux_stdio_stream_t *stdio_stream)
{
    VSF_LINUX_ASSERT(stdio_stream != NULL);
    __vsf_linux.cur_pid = 0;
    vk_fs_init();

#if VSF_LINUX_USE_SIMPLE_LIBC == ENABLED
    vsf_linux_glibc_init();
#endif
#if VSF_LINUX_CFG_FUTEX_NUM > 0
    VSF_POOL_INIT(vsf_linux_futex_pool, &__vsf_linux.futex.pool, VSF_LINUX_CFG_FUTEX_NUM);
#endif

    sethostname(VSF_LINUX_CFG_HOSTNAME, strlen(VSF_LINUX_CFG_HOSTNAME));

    // setup stdio
    vsf_linux_fd_t *sfd;
    vsf_linux_stream_priv_t *priv;
    int ret;

    ret = __vsf_linux_fd_create_ex(vsf_linux_resources_process(), &sfd,
        &vsf_linux_term_fdop, STDIN_FILENO, NULL);
    VSF_LINUX_ASSERT(ret == STDIN_FILENO);
    priv = (vsf_linux_stream_priv_t *)sfd->priv;
    priv->stream_rx = stdio_stream->in;
    priv->flags = O_RDONLY;
    __vsf_linux_rx_stream_init(priv);
    ((vsf_linux_term_priv_t *)priv)->subop = &__vsf_linux_stream_fdop;
    vsf_linux_term_fdop.fn_init(sfd);

    ret = __vsf_linux_fd_create_ex(vsf_linux_resources_process(), &sfd,
        &vsf_linux_term_fdop, STDOUT_FILENO, NULL);
    VSF_LINUX_ASSERT(ret == STDOUT_FILENO);
    priv = (vsf_linux_stream_priv_t *)sfd->priv;
    priv->stream_tx = stdio_stream->out;
    priv->flags = O_WRONLY;
    __vsf_linux_tx_stream_init(priv);
    ((vsf_linux_term_priv_t *)priv)->subop = &__vsf_linux_stream_fdop;
    vsf_linux_term_fdop.fn_init(sfd);

    ret = __vsf_linux_fd_create_ex(vsf_linux_resources_process(), &sfd,
        &vsf_linux_term_fdop, STDERR_FILENO, NULL);
    VSF_LINUX_ASSERT(ret == STDERR_FILENO);
    priv = (vsf_linux_stream_priv_t *)sfd->priv;
    // stderr is initialized after stdin, so stdin events will be bound to stderr
    //  inplement a evthandler to redirect to stdin if stdin stream_rx is same as stderr
    priv->on_evt = __vsf_linux_stderr_on_evt;
    priv->stream_rx = stdio_stream->in;
    priv->stream_tx = stdio_stream->err;
    __vsf_linux_rx_stream_init(priv);
    __vsf_linux_tx_stream_init(priv);
    ((vsf_linux_term_priv_t *)priv)->subop = &__vsf_linux_stream_fdop;
    vsf_linux_term_fdop.fn_init(sfd);

    // create kernel process(pid0)
    if (NULL != vsf_linux_start_process_internal(__vsf_linux_kernel_thread)) {
        return VSF_ERR_NONE;
    }
    return VSF_ERR_FAIL;
}

int isatty(int fd)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(fd);
    if (NULL == sfd) { return 0; }
    return sfd->op == &vsf_linux_term_fdop;
}

int ttyname_r(int fd, char *buf, size_t buflen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(fd);
    if ((NULL == sfd) || (sfd->op != &vsf_linux_term_fdop)) {
        return -1;
    }

    vsf_linux_fs_priv_t *fs_priv = (vsf_linux_fs_priv_t *)sfd->priv;
    strlcpy(buf, fs_priv->file->name, buflen);
    return 0;
}

char *ttyname(int fd)
{
    static char *__ttyname[64];
    ttyname_r(fd, __ttyname, sizeof(__ttyname));
    return __ttyname;
}

vsf_linux_thread_t * vsf_linux_create_raw_thread(const vsf_linux_thread_op_t *op,
            int stack_size, void *stack)
{
    vsf_linux_thread_t *thread;

    if (!stack_size) {
        stack_size = VSF_LINUX_CFG_STACKSIZE;
    }

    uint_fast32_t thread_size = op->priv_size + sizeof(*thread);
    uint_fast32_t alignment;
    uint_fast32_t all_size;
    if (NULL == stack) {
        stack_size += (1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT) - 1;
        stack_size &= ~((1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT) - 1);
        thread_size += (1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT) - 1;
        thread_size &= ~((1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT) - 1);
        all_size = thread_size + stack_size;
        all_size += (1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT) - 1;
        alignment = 1 << VSF_KERNEL_CFG_THREAD_STACK_ALIGN_BIT;
    } else {
        all_size = thread_size;
        alignment = 0;
    }

    thread = (vsf_linux_thread_t *)vsf_heap_malloc_aligned(all_size, alignment);
    if (thread != NULL) {
        memset(thread, 0, thread_size);

        // set entry and on_terminate
        thread->op = op;
        thread->on_terminate = (vsf_eda_on_terminate_t)op->on_terminate;
#if VSF_ARCH_USE_THREAD_REG == ENABLED
        thread->entry = __vsf_linux_thread_on_run;
#else
        thread->entry = (vsf_thread_entry_t *)op->on_run;
#endif

        // set stack
        thread->stack_size = stack_size;
        if (stack != NULL) {
            thread->stack = stack;
        } else {
            thread->stack = (void *)((uintptr_t)thread + thread_size);
        }
    }
    return thread;
}

vsf_linux_thread_t * vsf_linux_create_thread(vsf_linux_process_t *process,
            const vsf_linux_thread_op_t *op,
            int stack_size, void *stack)
{
    vsf_linux_thread_t *thread = vsf_linux_create_raw_thread(op, stack_size, stack);
    if (thread != NULL) {
        if (!process) {
            process = vsf_linux_get_cur_process();
        }
        thread->process = process;
        vsf_protect_t orig = vsf_protect_sched();
            thread->tid = __vsf_linux.cur_tid++;
            vsf_dlist_add_to_tail(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
        vsf_unprotect_sched(orig);
    }
    return thread;
}

int vsf_linux_start_thread(vsf_linux_thread_t *thread, vsf_prio_t priority)
{
    if (vsf_prio_inherit == priority) {
        priority = thread->process->prio;
    }
    vsf_thread_start(&thread->use_as__vsf_thread_t, &thread->use_as__vsf_thread_cb_t, priority);
    return 0;
}

static vsf_linux_process_t * __vsf_linux_create_process(int stack_size)
{
    vsf_linux_process_t *process = vsf_linux_malloc_res(sizeof(vsf_linux_process_t));
    if (process != NULL) {
        memset(process, 0, sizeof(*process));
        process->prio = vsf_prio_inherit;
        process->shell_process = process;

        vsf_linux_thread_t *thread = vsf_linux_create_thread(process, &__vsf_linux_main_op, stack_size, NULL);
        if (NULL == thread) {
            vsf_linux_free_res(process);
            return NULL;
        }

        vsf_protect_t orig = vsf_protect_sched();
            process->id.pid = __vsf_linux.cur_pid++;
            vsf_dlist_add_to_tail(vsf_linux_process_t, process_node, &__vsf_linux.process_list, process);
        vsf_unprotect_sched(orig);
        if (process->id.pid) {
            process->id.ppid = getpid();
        }
    }
    return process;
}

vsf_linux_process_t * vsf_linux_create_process(int stack_size)
{
    vsf_linux_process_t *process = __vsf_linux_create_process(stack_size);
    if (process != NULL) {
        vsf_linux_process_t *cur_process = vsf_linux_get_cur_process();
        VSF_LINUX_ASSERT(cur_process != NULL);
        process->parent_process = cur_process;
        process->shell_process = cur_process->shell_process;

        process->working_dir = __strdup_ex(process, cur_process->working_dir);
        if (NULL == process->working_dir) {
            goto delete_process_and_fail;
        }

        vsf_protect_t orig = vsf_protect_sched();
            vsf_dlist_add_to_tail(vsf_linux_process_t, child_node, &cur_process->child_list, process);
        vsf_unprotect_sched(orig);

#if VSF_LINUX_LIBC_USE_ENVIRON == ENABLED
        if (vsf_linux_merge_env(process, cur_process->__environ) < 0) {
            goto delete_process_and_fail;
        }
#endif
    }
    return process;
delete_process_and_fail:
    vsf_linux_delete_process(process);
    return NULL;
}

int vsf_linux_start_process(vsf_linux_process_t *process)
{
    // TODO: check if already started
    vsf_linux_thread_t *thread;
    vsf_dlist_peek_head(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
    process->status = PID_STATUS_RUNNING;
    return vsf_linux_start_thread(thread, vsf_prio_inherit);
}

void vsf_linux_cleanup_process(vsf_linux_process_t *process)
{
    vsf_linux_fd_t *sfd;
    for (int i = 0; i < VSF_LINUX_CFG_PLS_NUM; i++) {
        if (vsf_bitmap_get(&__vsf_linux.pls.bitmap, i)) {
            if (process->pls[i].destructor != NULL) {
                process->pls[i].destructor(process->pls[i].data);
            }
        }
    }

    do {
        vsf_dlist_peek_head(vsf_linux_fd_t, fd_node, &process->fd_list, sfd);
        if (sfd != NULL) {
            // do not use close because it depends on current process
            //  and vsf_linux_delete_process can be called in other processes
            __vsf_linux_fd_close_ex(process, sfd->fd);
        }
    } while (sfd != NULL);

#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    __vsf_dlist_foreach_next_unsafe(vsf_linux_sig_handler_t, node, &process->sig.handler_list) {
        vsf_dlist_remove(vsf_linux_sig_handler_t, node, &process->sig.handler_list, _);
        __free_ex(process, _);
    }
#endif
#if VSF_LINUX_LIBC_USE_ENVIRON == ENABLED
    vsf_linux_free_env(process);
#endif
    __vsf_linux_process_free_arg(process);
    if (process->working_dir != NULL) {
        __free_ex(process, process->working_dir);
    }

#if     VSF_LINUX_USE_SIMPLE_LIBC == ENABLED && VSF_LINUX_USE_SIMPLE_STDLIB == ENABLED\
    &&  VSF_LINUX_SIMPLE_STDLIB_CFG_HEAP_MONITOR == ENABLED
    if (process->heap_monitor.info.usage != 0) {
#   if VSF_LINUX_SIMPLE_STDLIB_HEAP_MONITOR_QUIET != ENABLED
        vsf_trace_warning("memory leak %d bytes detected in process 0x%p, balance = %d" VSF_TRACE_CFG_LINEEND,
                process->heap_monitor.info.usage, process, process->heap_monitor.info.balance);
#   endif
#   if VSF_LINUX_SIMPLE_STDLIB_HEAP_MONITOR_TRACE_DEPTH > 0
        int idx;
        while (true) {
            idx = vsf_bitmap_ffs(&process->heap_monitor.bitmap, VSF_LINUX_SIMPLE_STDLIB_HEAP_MONITOR_TRACE_DEPTH);
            if (idx < 0) { break; }

#       if VSF_LINUX_SIMPLE_STDLIB_HEAP_MONITOR_QUIET != ENABLED
            vsf_trace_warning("    0x%p(%d)" VSF_TRACE_CFG_LINEEND,
                process->heap_monitor.nodes[idx].ptr, process->heap_monitor.nodes[idx].size);
#       endif
            free(process->heap_monitor.nodes[idx].ptr);
        }
#   endif
    }
#endif
}

void vsf_linux_delete_process(vsf_linux_process_t *process)
{
    vsf_linux_cleanup_process(process);

    // DO NOT free process here, should be freed in waitpid in host process
    if (NULL == process->parent_process) {
        vsf_linux_free_res(process);
    }
}

void vsf_linux_exit_process(int status)
{
    vsf_linux_thread_t *thread2wait;
    vsf_linux_process_t *process2wait;
    vsf_linux_thread_t *cur_thread = vsf_linux_get_cur_thread();
    vsf_linux_process_t *process = cur_thread->process;
    VSF_LINUX_ASSERT(process != NULL);
    vsf_protect_t orig;

    orig = vsf_protect_sched();
        if (process->thread_pending_exit != NULL) {
            vsf_unprotect_sched(orig);
            goto end_no_return;
        }
        process->thread_pending_exit = cur_thread;

    // WIFEXITED
        process->status = status << 8;
    // 1. dequeue cur_thread and wait all other threads
        vsf_dlist_remove(vsf_linux_thread_t, thread_node, &process->thread_list, cur_thread);
    vsf_unprotect_sched(orig);

    // 2. call atexit, some resources maybe freed by user
    if (process->fn_atexit != NULL) {
        process->fn_atexit();
    }

    // 3. wait child threads and processes
    while (true) {
        orig = vsf_protect_sched();
            vsf_dlist_peek_head(vsf_linux_thread_t, thread_node, &process->thread_list, thread2wait);
        vsf_unprotect_sched(orig);
        if (NULL == thread2wait) {
            break;
        }

        // tid < 0 is not user thread, do not print warning
        if (thread2wait->tid >= 0) {
            vsf_trace_warning("linux: undetached thread %d, need pthread_join before exit\n", thread2wait->tid);
        }
        vsf_linux_wait_thread(thread2wait->tid, NULL);
    }

    while (true) {
        orig = vsf_protect_sched();
            vsf_dlist_peek_head(vsf_linux_process_t, child_node, &process->child_list, process2wait);
        vsf_unprotect_sched(orig);
        if (NULL == process2wait) {
            break;
        }

        vsf_trace_warning("linux: undetached process %d, need waitpid before exit\n", process2wait->id.pid);
        waitpid(process2wait->id.pid, NULL, 0);
    }

    // 5. cleanup process
    vsf_linux_cleanup_process(process);

    // 6. notify pending thread, MUST be the last before exiting current thread
    process->status &= ~PID_STATUS_RUNNING;
    if (process->thread_pending != NULL) {
        vsf_unprotect_sched(orig);
        process->thread_pending->retval = process->status;
        vsf_eda_post_evt(&process->thread_pending->use_as__vsf_eda_t, VSF_EVT_USER);
    } else {
        vsf_linux_process_t *parent_process = process->parent_process;
        if ((parent_process != NULL) && (parent_process->thread_pending_child != NULL)) {
            vsf_linux_thread_t *thread_pending = parent_process->thread_pending_child;
            parent_process->thread_pending_child = NULL;
            vsf_unprotect_sched(orig);
            thread_pending->retval = process->status;
            thread_pending->pid_exited = process->id.pid;
            vsf_eda_post_evt(&thread_pending->use_as__vsf_eda_t, VSF_EVT_USER);
        } else {
            vsf_unprotect_sched(orig);
        }
    }

    // 7. exit current thread
    cur_thread->process = NULL;
end_no_return:
    vsf_thread_exit();
}

void vsf_linux_set_process_reg(uintptr_t reg)
{
#if VSF_ARCH_USE_THREAD_REG == ENABLED
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);
    process->reg = reg;
    vsf_arch_set_thread_reg(reg);
#endif
}

vsf_linux_process_t * __vsf_linux_start_process_internal(
        vsf_linux_main_entry_t entry, char * const * argv, int stack_size, vsf_prio_t prio)
{
    VSF_LINUX_ASSERT((prio >= VSF_LINUX_CFG_PRIO_LOWEST) && (prio <= VSF_LINUX_CFG_PRIO_HIGHEST));
    vsf_linux_process_t *process = __vsf_linux_create_process(stack_size);
    if (process != NULL) {
        process->prio = prio;
        process->ctx.entry = entry;
        process->working_dir = __strdup_ex(process, "/");
        if (NULL == process->working_dir) {
            vsf_linux_delete_process(process);
            return NULL;
        }
        if (argv != NULL) {
            __vsf_linux_process_parse_arg(process, argv);
        }
        vsf_linux_start_process(process);
    }
    return process;
}

vsf_linux_process_t * vsf_linux_get_process(pid_t pid)
{
    vsf_protect_t orig = vsf_protect_sched();
    __vsf_dlist_foreach_unsafe(vsf_linux_process_t, process_node, &__vsf_linux.process_list) {
        if (_->id.pid == pid) {
            vsf_unprotect_sched(orig);
            return _;
        }
    }
    vsf_unprotect_sched(orig);
    return NULL;
}

vsf_linux_thread_t * vsf_linux_get_thread(pid_t pid, int tid)
{
    vsf_linux_process_t *process = pid < 0 ? vsf_linux_get_cur_process() : vsf_linux_get_process(pid);
    vsf_protect_t orig = vsf_protect_sched();
    __vsf_dlist_foreach_unsafe(vsf_linux_thread_t, thread_node, &process->thread_list) {
        if (_->tid == tid) {
            vsf_unprotect_sched(orig);
            return _;
        }
    }
    vsf_unprotect_sched(orig);
    return NULL;
}

vsf_linux_thread_t * vsf_linux_get_cur_thread(void)
{
    vsf_linux_thread_t *thread = (vsf_linux_thread_t *)vsf_eda_get_cur();
    VSF_LINUX_ASSERT(thread != NULL);
    return thread;
}

vsf_linux_process_t * vsf_linux_get_cur_process(void)
{
    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    vsf_linux_process_t *process = thread->process;
    VSF_LINUX_ASSERT(process != NULL);
    return process->id.pid != (pid_t)0 || NULL == __vsf_linux.kernel_process ?
                process : __vsf_linux.kernel_process;
}

#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
static vsf_linux_sig_handler_t * __vsf_linux_get_sighandler_ex(vsf_linux_process_t *process, int sig)
{
    vsf_linux_sig_handler_t *handler = NULL;
    VSF_LINUX_ASSERT(process != NULL);
    __vsf_dlist_foreach_unsafe(vsf_linux_sig_handler_t, node, &process->sig.handler_list) {
        if (_->sig == sig) {
            handler = _;
            break;
        }
    }
    return handler;
}

static void __vsf_linux_sighandler_on_run(vsf_thread_cb_t *cb)
{
    vsf_linux_thread_t *thread = container_of(cb, vsf_linux_thread_t, use_as__vsf_thread_cb_t);
    vsf_linux_process_t *process = thread->process;
    vsf_linux_sig_handler_t *handler;
    sighandler_t sighandler;
#if _NSIG > 32
    unsigned long long sig_mask;
#else
    unsigned long sig_mask;
#endif
    int sig;
    vsf_protect_t orig;
    bool is_all_done = false;

    while (true) {
        orig = vsf_protect_sched();
            sig_mask = process->sig.pending.sig[0] & ~process->sig.mask.sig[0];
            if (!sig_mask) {
                process->sig.sighandler_thread = NULL;
                is_all_done = true;
            } else {
#if _NSIG > 32
                sig = vsf_ffz32(~sig_mask);
                if (sig < 0) {
                    sig = vsf_ffz32(~(sig_mask >> 32)) + 32;
                }
                process->sig.pending.sig[0] &= ~(1ULL << sig);
                sig++;
#else
                sig = vsf_ffz32(~sig_mask);
                process->sig.pending.sig[0] &= ~(1 << sig);
                sig++;
#endif
            }
        vsf_unprotect_sched(orig);
        if (is_all_done) {
            break;
        }

        handler = __vsf_linux_get_sighandler_ex(process, sig);
        sighandler = SIG_DFL;
        if (handler != NULL) {
            if (handler->flags & SA_SIGINFO) {
                VSF_LINUX_ASSERT(handler->sigaction_handler != NULL);
                siginfo_t siginfo = {
                    .si_signo   = sig,
                    .si_errno   = errno,
                };
                handler->sigaction_handler(sig, &siginfo, NULL);
                // signal processed by sigaction, but still needs to interrupt slow syscall if necessary
                sighandler = SIG_IGN;
            } else {
                sighandler = handler->sighandler;
            }
        }

        if (SIG_DFL == sighandler) {
            // TODO: try to get default handler
            sighandler = SIG_IGN;
        }
        if (sighandler != SIG_IGN) {
            sighandler(sig);
        } else if ((NULL == handler) || !(handler->flags & SA_RESTART)) {
            // handler is not registered, or SA_RESTART is not set
            orig = vsf_protect_sched();
                vsf_linux_trigger_t *trigger;
                do {
                    vsf_dlist_remove_head(vsf_linux_trigger_t, node,  &process->sig.trigger_list, trigger);
                    if (trigger != NULL) {
                        vsf_linux_trigger_signal(trigger, sig);
                    }
                } while (trigger != NULL);
            vsf_unprotect_sched(orig);
        }
    }
    vsf_linux_detach_thread(thread);
}
#endif

static void __vsf_linux_main_on_run(vsf_thread_cb_t *cb)
{
    vsf_linux_thread_t *thread = container_of(cb, vsf_linux_thread_t, use_as__vsf_thread_cb_t);
    vsf_linux_process_t *process = thread->process;
    vsf_linux_process_t *shell_process = process->shell_process;
    vsf_linux_process_ctx_t *ctx = &process->ctx;
    vsf_linux_fd_priv_t *stdin_priv = NULL;
    vsf_linux_fd_t *sfd, *sfd_from;
    bool process_is_shell = false;
    int ret;

    if ((NULL == shell_process) || (shell_process == process)) {
        shell_process = &__vsf_linux.process_for_resources;
        process_is_shell = true;
    }

    sfd = vsf_linux_fd_get(STDIN_FILENO);
    if (NULL == sfd) {
        sfd_from = __vsf_linux_fd_get_ex(shell_process, STDIN_FILENO);
        VSF_LINUX_ASSERT(sfd_from != NULL);

        ret = __vsf_linux_fd_create_ex(process, &sfd, sfd_from->op, STDIN_FILENO, sfd_from->priv);
        VSF_LINUX_ASSERT(ret == STDIN_FILENO);
        stdin_priv = sfd->priv;
    }

    sfd = vsf_linux_fd_get(STDOUT_FILENO);
    if (NULL == sfd) {
        sfd_from = __vsf_linux_fd_get_ex(shell_process, STDOUT_FILENO);
        VSF_LINUX_ASSERT(sfd_from != NULL);

        ret = __vsf_linux_fd_create_ex(process, &sfd, sfd_from->op, STDOUT_FILENO, sfd_from->priv);
        VSF_LINUX_ASSERT(ret == STDOUT_FILENO);
    }

    sfd = vsf_linux_fd_get(STDERR_FILENO);
    if (NULL == sfd) {
        sfd_from = __vsf_linux_fd_get_ex(shell_process, STDERR_FILENO);
        VSF_LINUX_ASSERT(sfd_from != NULL);

        ret = __vsf_linux_fd_create_ex(process, &sfd, sfd_from->op, STDERR_FILENO, sfd_from->priv);
        VSF_LINUX_ASSERT(ret == STDERR_FILENO);
        if (process_is_shell) {
            sfd->priv->target = stdin_priv;
        }
    }

    vsf_linux_process_arg_t arg = ctx->arg;
    VSF_LINUX_ASSERT(ctx->entry != NULL);
    thread->retval = ctx->entry(arg.argc, (char **)arg.argv);

    vsf_linux_exit_process(thread->retval);
}

void vsf_linux_thread_on_terminate(vsf_linux_thread_t *thread)
{
    vsf_protect_t orig = vsf_protect_sched();
    vsf_linux_thread_t *thread_pending = thread->thread_pending;

    for (int i = 0; i < dimof(thread->tls); i++) {
        if ((thread->tls[i].destructor != NULL) && (thread->tls[i].data != NULL)) {
            thread->tls[i].destructor(thread->tls[i].data);
            thread->tls[i].destructor = NULL;
            thread->tls[i].data = NULL;
        }
    }

    thread->op = NULL;
    if (thread_pending != NULL) {
        thread->thread_pending = NULL;
        vsf_unprotect_sched(orig);
        thread_pending->retval = thread->retval;
        vsf_eda_post_evt(&thread_pending->use_as__vsf_eda_t, VSF_EVT_USER);
    } else {
        vsf_linux_process_t *process = thread->process;
        if (    (NULL == process) ||
                !vsf_dlist_is_in(vsf_linux_thread_t, thread_node, &process->thread_list, thread)) {
            vsf_unprotect_sched(orig);
            vsf_heap_free(thread);
            return;
        }
        vsf_unprotect_sched(orig);
    }
}

void vsf_linux_detach_process(vsf_linux_process_t *process)
{
    if (process->parent_process != NULL) {
        vsf_protect_t orig = vsf_protect_sched();
            vsf_dlist_remove(vsf_linux_process_t, child_node, &process->parent_process->child_list, process);
        vsf_unprotect_sched(orig);
        process->parent_process = NULL;
    }
}

void vsf_linux_detach_thread(vsf_linux_thread_t *thread)
{
    vsf_protect_t orig = vsf_protect_sched();
    vsf_linux_process_t *process = thread->process;
    if (process != NULL) {
        // if assert here, process exists before vsf_linux_detach_thread is called
        VSF_LINUX_ASSERT(NULL == thread->thread_pending);
        vsf_dlist_remove(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
    }
    vsf_unprotect_sched(orig);
}

int vsf_linux_wait_thread(int tid, int *retval)
{
    vsf_protect_t orig = vsf_protect_sched();
    vsf_linux_thread_t *thread = vsf_linux_get_thread(-1, tid);
    if (NULL == thread) {
        vsf_unprotect_sched(orig);
        return -1;
    }

    if (thread->op != NULL) {
        thread->thread_pending = vsf_linux_get_cur_thread();
        vsf_unprotect_sched(orig);
        vsf_thread_wfe(VSF_EVT_USER);
    } else {
        vsf_unprotect_sched(orig);
    }

    if (retval != NULL) {
        *retval = thread->retval;
    }
    vsf_linux_detach_thread(thread);
    vsf_heap_free(thread);
    return 0;
}

int daemon(int nochdir, int noclose)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);

    if (!nochdir) {
        free(process->working_dir);
        process->working_dir = strdup("/");
    }
    if (!noclose) {
        int fd = open("/dev/null", 0);
        VSF_LINUX_ASSERT(fd >= 0);
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    process->shell_process = process;
    vsf_linux_detach_process(process);

    vsf_linux_thread_t *thread_pending;
    vsf_protect_t orig = vsf_protect_sched();
        process->status = PID_STATUS_DAEMON;
        thread_pending = process->thread_pending;
    vsf_unprotect_sched(orig);
    if (thread_pending != NULL) {
        thread_pending->retval = process->status;
        vsf_eda_post_evt(&thread_pending->use_as__vsf_eda_t, VSF_EVT_USER);
    }
    return 0;
}

exec_ret_t execvpe(const char *file, char * const * argv, char  * const * envp)
{
    // fd will be closed after entry return
    vsf_linux_main_entry_t entry;
    int exefd = vsf_linux_fs_get_executable(file, &entry);
    if (exefd < 0) {
        return -1;
    }
    close(exefd);

    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    vsf_linux_process_ctx_t *ctx = &process->ctx;
    vsf_linux_thread_t *thread;

    __vsf_linux_process_free_arg(process);
    __vsf_linux_process_parse_arg(process, argv);
    vsf_linux_merge_env(process, (char **)envp);
    ctx->entry = entry;

    vsf_dlist_peek_head(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
    vsf_eda_post_evt(&thread->use_as__vsf_eda_t, VSF_EVT_INIT);
    vsf_thread_wfe(VSF_EVT_INVALID);
    return 0;
}

exec_ret_t execvp(const char *file, char * const * argv)
{
    return execvpe(file, argv, NULL);
}

exec_ret_t execve(const char *pathname, char * const * argv, char * const * envp)
{
    char fullpath[MAX_PATH];
    int fd = __vsh_get_exe(fullpath, sizeof(fullpath), (char *)pathname, NULL);
    if (fd < 0) {
        return -1;
    }
    close(fd);

    return execvpe(fullpath, argv, envp);
}

exec_ret_t execv(const char *pathname, char * const * argv)
{
    return execve(pathname, argv, NULL);
}

exec_ret_t __execlp_va(const char *pathname, const char *arg, va_list ap)
{
    // fd will be closed after entry return
    vsf_linux_main_entry_t entry;
    int exefd = vsf_linux_fs_get_executable(pathname, &entry);
    if (exefd < 0) {
        return -1;
    }
    close(exefd);

    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    vsf_linux_process_ctx_t *ctx = &process->ctx;
    vsf_linux_thread_t *thread;
    const char *args;

    __vsf_linux_process_free_arg(process);

    ctx->arg.argc = 1;
    ctx->arg.argv[0] = __strdup_ex(process, arg);
    args = va_arg(ap, const char *);
    while ((args != NULL) && (ctx->arg.argc <= VSF_LINUX_CFG_MAX_ARG_NUM)) {
        ctx->arg.argv[ctx->arg.argc++] = __strdup_ex(process, args);
        args = va_arg(ap, const char *);
    }
    ctx->entry = entry;

    vsf_dlist_peek_head(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
    vsf_eda_post_evt(&thread->use_as__vsf_eda_t, VSF_EVT_INIT);
    vsf_thread_wfe(VSF_EVT_INVALID);
    return 0;
}

exec_ret_t execlp(const char *pathname, const char *arg, ...)
{
    exec_ret_t ret;

    va_list ap;
    va_start(ap, arg);
        ret = __execlp_va(pathname, arg, ap);
    va_end(ap);
    return ret;
}

exec_ret_t __execl_va(const char *pathname, const char *arg, va_list ap)
{
    char fullpath[MAX_PATH];
    int fd = __vsh_get_exe(fullpath, sizeof(fullpath), (char *)pathname, NULL);
    if (fd < 0) {
        return -1;
    }
    close(fd);

    return __execlp_va(pathname, arg, ap);
}

exec_ret_t execl(const char *pathname, const char *arg, ...)
{
    exec_ret_t ret;

    va_list ap;
    va_start(ap, arg);
        ret = __execl_va(pathname, arg, ap);
    va_end(ap);
    return ret;
}

long sysconf(int name)
{
    switch (name) {
    case _SC_PAGESIZE:      return 256;
    case _SC_OPEN_MAX:      return 65535;
    case _SC_CLK_TCK:       return 100;
    }
    return 0;
}

long fpathconf(int fd, int name)
{
    switch (name) {
    case _PC_NAME_MAX:      return MAX_PATH;
    default:                return -1;
    }
}

long pathconf(const char *path, int name)
{
    if ((NULL == path) || ('\0' == *path)) {
        return -1;
    }

    int fd = open(path, 0);
    if (fd < 0) { return -1; }

    long res = fpathconf(fd, name);
    close(fd);
    return res;
}

char *realpath(const char *path, char *resolved_path)
{
    bool is_allocated = false;
    if (NULL == resolved_path) {
        resolved_path = malloc(MAX_PATH);
        if (NULL == resolved_path) {
            return NULL;
        }
        is_allocated = true;
    }
    if (vsf_linux_generate_path(resolved_path, MAX_PATH, NULL, (char *)path)) {
        if (is_allocated) {
            free(resolved_path);
        }
        return NULL;
    }
    return resolved_path;
}

#if __IS_COMPILER_IAR__
//! transfer of control bypasses initialization
#   pragma diag_suppress=pe546
#endif

int pipe(int pipefd[2])
{
    vsf_linux_fd_t *sfd_rx = NULL, *sfd_tx = NULL;

    sfd_rx = vsf_linux_rx_pipe(NULL);
    if (NULL == sfd_rx) {
        return -1;
    }

    sfd_tx = vsf_linux_tx_pipe((vsf_linux_pipe_rx_priv_t *)sfd_rx->priv);
    if (NULL == sfd_tx) {
        close(sfd_rx->fd);
        return -1;
    }

    pipefd[0] = sfd_rx->fd;
    pipefd[1] = sfd_tx->fd;
    return 0;
}

int pipe2(int pipefd[2], int flags)
{
    int res = pipe(pipefd);
    if (0 == res) {
        if (flags & O_CLOEXEC) {
            fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
            fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
        }
        if (flags & O_NONBLOCK) {
            fcntl(pipefd[0], F_SETFD, O_NONBLOCK);
            fcntl(pipefd[1], F_SETFD, O_NONBLOCK);
        }
    }
    return res;
}

int kill(pid_t pid, int sig)
{
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    if (sig <= 0) {
        return -1;
    }
    sig--;

    vsf_protect_t orig = vsf_protect_sched();
    vsf_linux_process_t *process = vsf_linux_get_process(pid);
    if ((NULL == process) || (process->thread_pending_exit != NULL)) {
        vsf_unprotect_sched(orig);
        return -1;
    }

#if _NSIG > 32
    process->sig.pending.sig[0] |= 1ULL << sig;
#else
    process->sig.pending.sig[0] |= 1 << sig;
#endif
    if (NULL == process->sig.sighandler_thread) {
        process->sig.sighandler_thread = vsf_linux_create_raw_thread(&__vsf_linux_sighandler_op, 0, NULL);
        if (NULL == process->sig.sighandler_thread) {
            vsf_unprotect_sched(orig);
            return -1;
        }

        vsf_linux_thread_t *thread = process->sig.sighandler_thread;
        thread->process = process;
        thread->tid = -1;
        vsf_dlist_add_to_tail(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
        vsf_unprotect_sched(orig);
        vsf_linux_start_thread(thread, VSF_LINUX_CFG_PRIO_SIGNAL);
        return 0;
    }
    vsf_unprotect_sched(orig);
    // priority of sighandler which is defined by VSF_LINUX_CFG_PRIO_SIGNAL should
    //  be higher than normal task, so no need to yield here, and thus kill can
    //  be called in non-thread environment.
//    vsf_thread_yield();
    return 0;
#else
    return -1;
#endif
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);
    vsf_linux_sig_handler_t *handler = __vsf_linux_get_sighandler_ex(process, signum);
    bool new_handler = NULL == handler;

    if (new_handler) {
        handler = calloc(1, sizeof(vsf_linux_sig_handler_t));
        if (NULL == handler) {
            return -1;
        }
        handler->sig = signum;
        handler->sighandler = SIG_DFL;
    }

    if (oldact != NULL) {
        oldact->sa_handler = handler->sighandler;
        oldact->sa_flags = handler->flags;
        oldact->sa_mask = handler->mask;
    }
    handler->sighandler = act->sa_handler;
    handler->flags = act->sa_flags;
    handler->mask = act->sa_mask;

    if (new_handler) {
        vsf_protect_t orig = vsf_protect_sched();
            vsf_dlist_add_to_tail(vsf_linux_sig_handler_t, node, &process->sig.handler_list, handler);
        vsf_unprotect_sched(orig);
    }
    return 0;
#else
    return -1;
#endif
}

#if !defined(__WIN__) || VSF_LINUX_CFG_WRAPPER == ENABLED
// conflicts with APIs in ucrt, need VSF_LINUX_CFG_WRAPPER
sighandler_t signal(int signum, sighandler_t handler)
{
#if VSF_LINUX_CFG_SUPPORT_SIG == ENABLED
    const struct sigaction in = {
        .sa_handler = handler,
    };
    struct sigaction out;
    if (!sigaction(signum, &in, &out)) {
        return out.sa_handler;
    }
    return SIG_DFL;
#else
    return NULL;
#endif
}

int raise(int sig)
{
    return kill(getpid(), sig);
}
#endif

useconds_t ualarm(useconds_t usecs, useconds_t interval)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

unsigned int alarm(unsigned int seconds)
{
    const struct itimerval it_new = {
        .it_value.tv_sec = seconds,
    };
    setitimer(ITIMER_REAL, &it_new, NULL);
    // TODO: return remaining timer of previous scheduled alarm
    return 0;
}

pid_t wait(int *status)
{
    vsf_linux_process_t *cur_process = vsf_linux_get_cur_process();
    vsf_linux_thread_t *cur_thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(cur_thread != NULL);

    VSF_LINUX_ASSERT(cur_process != NULL);
    vsf_protect_t orig = vsf_protect_sched();
        if (cur_process->thread_pending_child != NULL) {
            vsf_unprotect_sched(orig);
            return (pid_t)-1;
        }
        cur_process->thread_pending_child = cur_thread;
    vsf_unprotect_sched(orig);

    vsf_thread_wfe(VSF_EVT_USER);

    if (status != NULL) {
        *status = cur_thread->retval;
    }
    vsf_linux_process_t *process = vsf_linux_get_process(cur_thread->pid_exited);
    vsf_linux_detach_process(process);
    vsf_linux_free_res(process);
    return cur_thread->pid_exited;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    if (pid <= 0) {
        VSF_LINUX_ASSERT(false);
        return -1;
    }
    if (options != 0) {
        VSF_LINUX_ASSERT(false);
        return -1;
    }

    vsf_linux_thread_t *cur_thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(cur_thread != NULL);
    bool is_daemon;

    vsf_protect_t orig = vsf_protect_sched();
    vsf_linux_process_t *process = vsf_linux_get_process(pid);
    if ((NULL == process) || (process->parent_process != cur_thread->process)) {
        vsf_unprotect_sched(orig);
        return -1;
    }
    if (process->status & PID_STATUS_RUNNING) {
        process->thread_pending = cur_thread;
        vsf_unprotect_sched(orig);
        vsf_thread_wfe(VSF_EVT_USER);
        orig = vsf_protect_sched();
    } else {
        cur_thread->retval = process->status;
    }
    is_daemon = process->status == PID_STATUS_DAEMON;
    if (!is_daemon) {
        vsf_dlist_remove(vsf_linux_process_t, process_node, &__vsf_linux.process_list, process);
    }
    vsf_unprotect_sched(orig);

    if (status != NULL) {
        *status = cur_thread->retval;
    }
    if (!is_daemon) {
        vsf_linux_detach_process(process);
        vsf_linux_free_res(process);
    }
    return pid;
}

int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
    switch (idtype) {
    case P_PID:     return waitpid((pid_t)id, NULL, options);
    default:        return -1;
    }
}

gid_t getgid(void)
{
    return (gid_t)0;
}
gid_t getegid(void)
{
    return (gid_t)0;
}

uid_t getuid(void)
{
    return (uid_t)0;
}
uid_t geteuid(void)
{
    return (uid_t)0;
}

pid_t getpid(void)
{
    return vsf_linux_get_cur_process()->id.pid;
}

pid_t getppid(void)
{
    return vsf_linux_get_cur_process()->id.ppid;
}

pid_t gettid(void)
{
    return vsf_linux_get_cur_thread()->tid;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();

    if (oldset != NULL) {
        *oldset = process->sig.mask;
    }

    if (set != NULL) {
        vsf_protect_t orig = vsf_protect_sched();
            switch (how) {
            case SIG_BLOCK:     sigaddsetmask(&process->sig.mask, set->sig[0]); break;
            case SIG_UNBLOCK:   sigdelsetmask(&process->sig.mask, set->sig[0]); break;
            case SIG_SETMASK:   process->sig.mask = *set;                       break;
            }
        vsf_unprotect_sched(orig);
    }
    return 0;
}

char * getcwd(char *buffer, size_t maxlen)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    VSF_LINUX_ASSERT(process != NULL);
    if (strlen(process->working_dir) >= maxlen) {
        errno = ERANGE;
        return NULL;
    }
    strncpy(buffer, process->working_dir, maxlen);
    return buffer;
}

int vsf_linux_fs_get_executable(const char *pathname, vsf_linux_main_entry_t *entry)
{
    int fd = open(pathname, 0);
    if (fd < 0) {
        return -1;
    }

    uint_fast32_t feature;
    if ((vsf_linux_fd_get_feature(fd, &feature) < 0) || !(feature & VSF_FILE_ATTR_EXECUTE)) {
        close(fd);
        return -1;
    }

    // TODO: support other executable files?
    if (entry != NULL) {
        vsf_linux_fd_get_target(fd, (void **)entry);
    }
    return fd;
}

int vsf_linux_fd_bind_executable(int fd, vsf_linux_main_entry_t entry)
{
    int err = vsf_linux_fd_bind_target(fd, (void *)entry, NULL, NULL);
    if (!err) {
        return vsf_linux_fd_add_feature(fd, VSF_FILE_ATTR_EXECUTE);
    }
    return err;
}

char * getpass(const char *prompt)
{
    fprintf(stdout, "%s", prompt);

    struct termios orig, t;
    tcgetattr(STDIN_FILENO, &orig);
    t = orig;
    t.c_lflag &= ~(ECHO | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);

    // TODO: read passwd

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);

    return NULL;
}

#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
vsf_systimer_tick_t vsf_linux_sleep(vsf_systimer_tick_t ticks)
{
    vsf_systimer_tick_t realticks;
    vsf_linux_trigger_t trigger;
    vsf_linux_trigger_init(&trigger);

    realticks = vsf_systimer_get_tick();
    vsf_linux_trigger_pend(&trigger, (vsf_timeout_tick_t)ticks);
    realticks = vsf_systimer_get_elapsed(realticks);

    return ticks > realticks ? ticks - realticks : 0;
}

int usleep(int usec)
{
    errno = 0;
    vsf_linux_sleep(vsf_systimer_us_to_tick(usec));
    return errno != 0 ? -1 : 0;
}

// TODO: wakeup after signal
unsigned sleep(unsigned sec)
{
    vsf_systimer_tick_t ticks_remain = vsf_linux_sleep(vsf_systimer_ms_to_tick(sec * 1000));
    return vsf_systimer_tick_to_ms(ticks_remain) / 1000;
}
#endif

int chown(const char *pathname, uid_t owner, gid_t group)
{
    return 0;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    return 0;
}

int lchown(const char *pathname, uid_t owner, gid_t group)
{
    return 0;
}

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags)
{
    return 0;
}


// malloc.h
void * memalign(size_t alignment, size_t size)
{
    return aligned_alloc(alignment, size);
}

// ipc.h
key_t ftok(const char *pathname, int id)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

// sys/sem.h
int semctl(int semid, int semnum, int cmd, ...)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

int semget(key_t key, int nsems, int semflg)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

int semop(int semid, struct sembuf *sops, size_t nsops)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

int semtimedop(int semid, struct sembuf *sops, size_t nsops,
                    const struct timespec *timeout)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

// sys/time.h
#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
#ifdef VSF_LINUX_CFG_RTC
    vsf_rtc_tm_t rtc_tm;
    vsf_rtc_get(&VSF_LINUX_CFG_RTC, &rtc_tm);

    VSF_LINUX_ASSERT(rtc_tm.tm_year >= 1900);
    struct tm t = {
        .tm_sec = rtc_tm.tm_sec,
        .tm_min = rtc_tm.tm_min,
        .tm_hour = rtc_tm.tm_hour,
        .tm_mday = rtc_tm.tm_mday,
        .tm_mon = rtc_tm.tm_mon - 1,
        .tm_year = rtc_tm.tm_year - 1900,
    };

    tv->tv_sec = mktime(&t);
    tv->tv_usec = rtc_tm.tm_ms * 1000;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
#endif
    return 0;
}
#endif

clock_t times(struct tms *buf)
{
    if (buf != NULL) {
        memset(buf, 0, sizeof(*buf));
    }
    return (clock_t)(vsf_systimer_get_tick() * sysconf(_SC_CLK_TCK) / vsf_systimer_get_freq());
}

// futex.h
// TODO: use hasp map instead of vsf_dlist for better performance at the cost of resources taken
// TODO: use vsf_linux_trigger_t so that wait operation can be interrupted by signals

static vsf_linux_futex_t * __vsf_linux_futex_get(uint32_t *futex, bool is_to_alloc)
{
    vsf_linux_futex_t *linux_futex = NULL;
    __vsf_dlist_foreach_unsafe(vsf_linux_futex_t, node, &__vsf_linux.futex.list) {
        if (_->futex == futex) {
            linux_futex = _;
            break;
        }
    }
    if ((NULL == linux_futex) && is_to_alloc) {
        linux_futex = VSF_POOL_ALLOC(vsf_linux_futex_pool, &__vsf_linux.futex.pool);
        if (linux_futex != NULL) {
            vsf_dlist_init_node(vsf_linux_futex_t, node, linux_futex);
            linux_futex->futex = futex;
            vsf_eda_trig_init(&linux_futex->trig, false, true);
        }
    }
    return linux_futex;
}

long sys_futex(uint32_t *futex, int futex_op, uint32_t val, uintptr_t val2, uint32_t *futex2, uint32_t val3)
{
    VSF_LINUX_ASSERT(futex != NULL);
    vsf_linux_futex_t *linux_futex;
    vsf_timeout_tick_t timeout_ticks;
    vsf_sync_reason_t reason;
    vsf_protect_t orig;

    switch (futex_op & FUTEX_CMD_MASK) {
    case FUTEX_WAIT:
        timeout_ticks = !val2 ? -1 : vsf_linux_timespec2tick((const struct timespec *)val2);
        orig = vsf_protect_sched();
        if (*futex != val) {
            vsf_unprotect_sched(orig);
            break;
        }
        linux_futex = __vsf_linux_futex_get(futex, true);
        VSF_LINUX_ASSERT(linux_futex != NULL);

        extern void __vsf_eda_sync_pend(vsf_sync_t *sync, vsf_eda_t *eda, vsf_timeout_tick_t timeout);
        __vsf_eda_sync_pend(&linux_futex->trig, NULL, timeout_ticks);
        vsf_unprotect_sched(orig);

        do {
            reason = vsf_eda_sync_get_reason(&linux_futex->trig, vsf_thread_wait());
        } while (reason == VSF_SYNC_PENDING);
        switch (reason) {
        case VSF_SYNC_GET:          return 0;
        case VSF_SYNC_TIMEOUT:      return -ETIMEDOUT;
        default:                    VSF_LINUX_ASSERT(false);
        }
        break;
    case FUTEX_WAKE:
        orig = vsf_protect_sched();
        linux_futex = __vsf_linux_futex_get(futex, false);
        vsf_unprotect_sched(orig);

        VSF_LINUX_ASSERT(linux_futex != NULL);
        vsf_eda_trig_set(&linux_futex->trig);

        orig = vsf_protect_sched();
        if (vsf_dlist_is_empty(&linux_futex->trig.pending_list)) {
            VSF_POOL_FREE(vsf_linux_futex_pool, &__vsf_linux.futex.pool, linux_futex);
        }
        vsf_unprotect_sched(orig);
        break;
    default:
        // TODO: add support to futex_op if assert here
        VSF_LINUX_ASSERT(false);
        return -1;
    }
    return -1;
}

// prctl.h

int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
    switch (option) {
    case PR_SET_NAME:
        return 0;
    }
    VSF_LINUX_ASSERT(false);
    return -1;
}

#if VSF_LINUX_CFG_SHM_NUM > 0
// shm.h

int shmget(key_t key, size_t size, int shmflg)
{
    VSF_LINUX_ASSERT((IPC_PRIVATE == key) || (shmflg & IPC_CREAT));

    vsf_protect_t orig = vsf_protect_sched();
        key = vsf_bitmap_ffz(&__vsf_linux.shm.bitmap, VSF_LINUX_CFG_SHM_NUM);
        if (key >= 0) {
            vsf_bitmap_set(&__vsf_linux.shm.bitmap, key);
        }
    vsf_unprotect_sched(orig);

    if (key < 0) {
        return key;
    }

    vsf_linux_shm_mem_t *mem = &__vsf_linux.shm.mem[key++];
    mem->size = size;
    mem->key = key;
    mem->buffer = malloc(size);
    if (NULL == mem->buffer) {
        shmctl(key, IPC_RMID, NULL);
        return -1;
    }

    return key;
}

void * shmat(int shmid, const void *shmaddr, int shmflg)
{
    vsf_linux_shm_mem_t *mem = &__vsf_linux.shm.mem[shmid];
    return mem->buffer;
}

int shmdt(const void *shmaddr)
{
    return 0;
}

int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
    shmid--;
    VSF_LINUX_ASSERT(shmid < VSF_LINUX_CFG_SHM_NUM);

    vsf_linux_shm_mem_t *mem = &__vsf_linux.shm.mem[shmid];
    switch (cmd) {
    case IPC_STAT:
        memset(buf, 0, sizeof(*buf));
        buf->shm_segsz = mem->size;
        buf->shm_perm.key = mem->key;
        break;
    case IPC_SET:
        VSF_LINUX_ASSERT(false);
        break;
    case IPC_RMID: {
            if (mem->buffer != NULL) {
                free(mem->buffer);
                mem->buffer = NULL;
            }

            vsf_protect_t orig = vsf_protect_sched();
                vsf_bitmap_clear(&__vsf_linux.shm.bitmap, shmid);
            vsf_unprotect_sched(orig);
        }
        break;
    }
    return 0;
}
#endif      // VSF_LINUX_CFG_SHM_NUM

// sched

int sched_get_priority_max(int policy)
{
    return VSF_LINUX_CFG_PRIO_HIGHEST;
}

int sched_get_priority_min(int policy)
{
    return VSF_LINUX_CFG_PRIO_LOWEST;
}

int sched_getparam(pid_t pid, struct sched_param *param)
{
    vsf_linux_process_t *process = vsf_linux_get_process(pid);
    if (NULL == process) { return -1; }

    vsf_linux_thread_t *thread;
    vsf_dlist_peek_head(vsf_linux_thread_t, thread_node, &process->thread_list, thread);
    if (NULL == thread) { return -1; }

    param->sched_priority = __vsf_eda_get_cur_priority(&thread->use_as__vsf_eda_t);
    return 0;
}

int sched_setparam(pid_t pid, const struct sched_param *param)
{
    vsf_linux_process_t *process = vsf_linux_get_process(pid);
    if (NULL == process) { return -1; }

    __vsf_dlist_foreach_unsafe(vsf_linux_thread_t, thread_node, &process->thread_list) {
        __vsf_eda_set_priority(&_->use_as__vsf_eda_t, param->sched_priority);
    }
    return 0;
}

int sched_getscheduler(pid_t pid)
{
    return 0;
}

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
    return sched_setparam(pid, param);
}

int sched_yield(void)
{
    vsf_thread_yield();
    return 0;
}

int uname(struct utsname *name)
{
    static const struct utsname __name = {
        .sysname    = VSF_LINUX_SYSNAME,
        .nodename   = VSF_LINUX_NODENAME,
        .release    = VSF_LINUX_RELEASE,
        .version    = VSF_LINUX_VERSION,
        .machine    = VSF_LINUX_MACHINE,
    };
    if (name != NULL) {
        *name = __name;
    }
    return 0;
}

size_t getpagesize(void)
{
    return 1;
}

int gethostname(char *name, size_t len)
{
    strncpy(name, __vsf_linux.hostname, len);
    return 0;
}

int sethostname(const char *name, size_t len)
{
    if (len > HOST_NAME_MAX) {
        return -1;
    }
    memcpy(__vsf_linux.hostname, name, len);
    __vsf_linux.hostname[len] = '\0';
    return 0;
}

// sys/mman.h
void * mmap64(void *addr, size_t len, int prot, int flags, int fd, off64_t off)
{
    if (fd >= 0) {
        vsf_linux_fd_t *sfd = vsf_linux_fd_get(fd);
        if ((NULL == sfd) || (NULL == sfd->op) || (NULL == sfd->op->fn_mmap)) {
            return NULL;
        }
        if (sfd->mmapped_buffer != NULL) {
            return NULL;
        }

        sfd->mmapped_buffer = sfd->op->fn_mmap(sfd, off, len, prot);
        return sfd->mmapped_buffer;
    }

    return malloc(len);
}

void * mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    return mmap64(addr, len, prot, flags, fd, (off64_t)off);
}

int msync(void *addr, size_t len, int flags)
{
    vsf_linux_process_t *cur_process = vsf_linux_get_cur_process();
    __vsf_dlist_foreach_unsafe(vsf_linux_fd_t, fd_node, &cur_process->fd_list) {
        if (_->mmapped_buffer == addr) {
            return NULL == _->op->fn_msync ? 0 : _->op->fn_msync(_, _->mmapped_buffer);
        }
    }
    return 0;
}

int munmap(void *addr, size_t len)
{
    vsf_linux_process_t *cur_process = vsf_linux_get_cur_process();
    __vsf_dlist_foreach_unsafe(vsf_linux_fd_t, fd_node, &cur_process->fd_list) {
        if (_->mmapped_buffer == addr) {
            int ret = NULL == _->op->fn_munmap ? 0 : _->op->fn_munmap(_, _->mmapped_buffer);
            _->mmapped_buffer = NULL;
            return ret;
        }
    }

    free(addr);
    return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
    return 0;
}

int shm_open(const char *name, int oflag, mode_t mode)
{
    return open(name, oflag, mode);
}

int shm_unlink(const char *name)
{
    return unlink(name);
}

// sys/random.h
#if VSF_HAL_USE_RNG == ENABLED && VSF_HW_RNG_COUNT > 0
static void __getrandom_on_ready(void *param, uint32_t *buffer, uint32_t num)
{
    vsf_eda_post_evt((vsf_eda_t *)param, VSF_EVT_USER);
}
#endif

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
#if VSF_HAL_USE_RNG == ENABLED && VSF_HW_RNG_COUNT > 0
    vsf_eda_t *eda = vsf_eda_get_cur();
    VSF_LINUX_ASSERT(eda != NULL);
    if (VSF_ERR_NONE == vsf_hw_rng_generate_request(&vsf_hw_rng0, buf, buflen / (VSF_HW_RNG_BITLEN >> 3),
            eda, __getrandom_on_ready)) {
        vsf_thread_wfe(VSF_EVT_USER);
        return buflen;
    }
    return 0;
#else
    VSF_LINUX_ASSERT(false);
#endif
    return 0;
}

int getentropy(void *buf, size_t length)
{
    ssize_t length_ret = getrandom(buf, length, 0);
    return (length_ret == length) ? 0 : -1;
}

pid_t fork(void)
{
    VSF_LINUX_ASSERT(false);
    return -1;
}

// sysmacros
dev_t makedev(unsigned int maj, unsigned int min)
{
    return (dev_t)0;
}

unsigned int major(dev_t dev)
{
    VSF_LINUX_ASSERT(false);
    return 0;
}

unsigned int minor(dev_t dev)
{
    VSF_LINUX_ASSERT(false);
    return 0;
}

// spawn.h
int posix_spawnp(pid_t *pid, const char *file,
                const posix_spawn_file_actions_t *actions,
                const posix_spawnattr_t *attr,
                char * const argv[], char * const env[])
{
    vsf_linux_main_entry_t entry;
    int exefd = vsf_linux_fs_get_executable(file, &entry);
    if (exefd < 0) {
        if (pid != NULL) {
            *pid = -1;
        }
        return -1;
    }
    close(exefd);
    VSF_LINUX_ASSERT(entry != NULL);

    vsf_linux_process_t *process = vsf_linux_create_process(0);
    if (NULL == process) { return -ENOMEM; }
    vsf_linux_process_ctx_t *ctx = &process->ctx;
    ctx->entry = entry;
    VSF_LINUX_ASSERT(argv != NULL);
    __vsf_linux_process_parse_arg(process, argv);

    vsf_linux_process_t *cur_process = vsf_linux_get_cur_process();
    process->shell_process = cur_process->shell_process;

    // dup fds
    vsf_linux_fd_t *sfd, *sfd_new;
    vsf_protect_t orig;
    __vsf_dlist_foreach_unsafe(vsf_linux_fd_t, fd_node, &cur_process->fd_list) {
        if (!(_->fd_flags & FD_CLOEXEC)) {
            if (__vsf_linux_fd_create_ex(process, &sfd, _->op, _->fd, _->priv) != _->fd) {
                vsf_trace_error("spawn: failed to dup fd %d", VSF_TRACE_CFG_LINEEND, _->fd);
                goto delete_process_and_fail;
            }
        }
    }

#if VSF_LINUX_LIBC_USE_ENVIRON == ENABLED
    // env
    if (vsf_linux_merge_env(process, (char **)env) < 0) {
        goto delete_process_and_fail;
    }
#endif

    // apply actions
    if (actions != NULL) {
        char fullpath[MAX_PATH];
        struct spawn_action *a = actions->actions;
        for (int i = 0; i < actions->used; i++, a++) {
            switch (a->tag) {
            case spawn_do_close:
                if (__vsf_linux_fd_close_ex(process, a->action.close_action.fd) < 0) {
                    vsf_trace_error("spawn: action: failed to close fd %d", VSF_TRACE_CFG_LINEEND,
                        a->action.close_action.fd);
                    goto delete_process_and_fail;
                }
                break;
            case spawn_do_dup2:
                sfd = __vsf_linux_fd_get_ex(process, a->action.dup2_action.newfd);
                if (sfd != NULL) {
                    orig = vsf_protect_sched();
                        sfd->priv->ref--;
#if VSF_LINUX_CFG_FD_TRACE == ENABLED
                        vsf_trace_debug("%s action dup2_close: process 0x%p fd %d priv 0x%p ref %d" VSF_TRACE_CFG_LINEEND,
                            __FUNCTION__, process, sfd->fd, sfd->priv, sfd->priv->ref);
#endif
                    vsf_unprotect_sched(orig);
                    __vsf_linux_fd_delete_ex(process, sfd->fd);
                }

                sfd = __vsf_linux_fd_get_ex(process, a->action.dup2_action.fd);
                if (NULL == sfd) {
                action_fail_dup:
                    vsf_trace_error("spawn: action: failed to dup fd %d", VSF_TRACE_CFG_LINEEND,
                        a->action.dup2_action.fd);
                    goto delete_process_and_fail;
                }

                int ret = __vsf_linux_fd_create_ex(process, &sfd_new, sfd->op, a->action.dup2_action.newfd, sfd->priv);
                if (ret < 0) {
                    goto action_fail_dup;
                }
                break;
            case spawn_do_open:
                VSF_LINUX_ASSERT(false);
                break;
            case spawn_do_fchdir: {
                    vk_file_t *file = __vsf_linux_get_fs_ex(process, a->action.fchdir_action.fd);
                    if (NULL == file) {
                    action_fail_fchdir:
                        vsf_trace_error("spawn: action: failed to fchdir fd %d", VSF_TRACE_CFG_LINEEND,
                            a->action.fchdir_action.fd);
                        goto delete_process_and_fail;
                    }

                    char *ptr = &fullpath[sizeof(fullpath) - 1];
                    size_t namelen;
                    *ptr-- = '\0';
                    while (file != NULL) {
                        namelen = strlen(file->name);
                        ptr -= strlen(file->name);
                        if (ptr < fullpath) {
                            ptr = NULL;
                            break;
                        }
                        memcpy(ptr, file->name, namelen);
                        file = file->parent;
                    }

                    if (NULL == ptr) {
                        goto action_fail_fchdir;
                    }
                    a->action.chdir_action.path = ptr;
                }
                // fall through
            case spawn_do_chdir:
                if (vsf_linux_chdir(process, a->action.chdir_action.path) < 0) {
                    vsf_trace_error("spawn: action: failed to chdir fd %s", VSF_TRACE_CFG_LINEEND,
                        a->action.chdir_action.path);
                    goto delete_process_and_fail;
                }
                break;
            }
        }
    }

    if (pid != NULL) {
        *pid = process->id.pid;
    }
    return vsf_linux_start_process(process);
delete_process_and_fail:
    vsf_linux_delete_process(process);
    return -1;
}

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *actions,
                const posix_spawnattr_t *attr,
                char * const argv[], char * const env[])
{
    char fullpath[MAX_PATH];
    int fd = __vsh_get_exe(fullpath, sizeof(fullpath), (char *)path, NULL);
    if (fd < 0) {
        if (pid != NULL) {
            *pid = -1;
        }
        return -1;
    }
    close(fd);

    return posix_spawnp(pid, fullpath, actions, attr, argv, env);
}

int posix_spawnattr_init(posix_spawnattr_t *attr)
{
    memset(attr, 0, sizeof(*attr));
    return 0;
}

int posix_spawnattr_destroy(posix_spawnattr_t *attr)
{
    return 0;
}

int posix_spawnattr_getsigdefault(const posix_spawnattr_t *attr, sigset_t *sigdefault)
{
    return 0;
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t *attr, const sigset_t *sigdefault)
{
    return 0;
}

int posix_spawnattr_getsigmask(const posix_spawnattr_t *attr, sigset_t *sigmask)
{
    return 0;
}

int posix_spawnattr_setsigmask(posix_spawnattr_t *attr, const sigset_t *sigmask)
{
    return 0;
}

int posix_spawnattr_getflags(const posix_spawnattr_t *attr, short int *flags)
{
    return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t *attr, short int flags)
{
    return 0;
}

int posix_spawnattr_getpgroup(const posix_spawnattr_t *attr, pid_t *pgroup)
{
    return 0;
}

int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup)
{
    return 0;
}

int posix_spawnattr_getschedpolicy(const posix_spawnattr_t *attr, int *schedpolicy)
{
    return 0;
}

int posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int schedpolicy)
{
    return 0;
}

int posix_spawnattr_getschedparam(const posix_spawnattr_t *attr, struct sched_param *schedparam)
{
    return 0;
}

int posix_spawnattr_setschedparam(posix_spawnattr_t *attr, const struct sched_param *schedparam)
{
    return 0;
}

static struct spawn_action * __posix_spawn_file_actions_add(posix_spawn_file_actions_t *actions)
{
    if (actions->used == actions->allocated) {
        actions->allocated += 8;
        actions->actions = realloc(actions->actions, actions->allocated * sizeof(struct spawn_action));
    }
    if (NULL == actions->actions) {
        return NULL;
    }

    return &actions->actions[actions->used++];
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *actions)
{
    memset(actions, 0, sizeof(*actions));
    return 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *actions)
{
    if (actions->actions != NULL) {
        free(actions->actions);
    }
    return 0;
}

int posix_spawn_file_actions_addopen(
                posix_spawn_file_actions_t *actions,
                int fd, const char *path,
                int oflag, mode_t mode)
{
    struct spawn_action * action = __posix_spawn_file_actions_add(actions);
    if (NULL == action) {
        return -1;
    }

    action->tag = spawn_do_open;
    action->action.open_action.fd = fd;
    action->action.open_action.path = (char *)path;
    action->action.open_action.oflag = oflag;
    action->action.open_action.mode = mode;
    return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions, int fd)
{
    struct spawn_action * action = __posix_spawn_file_actions_add(actions);
    if (NULL == action) {
        return -1;
    }

    action->tag = spawn_do_close;
    action->action.close_action.fd = fd;
    return 0;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions, int fd, int newfd)
{
    struct spawn_action * action = __posix_spawn_file_actions_add(actions);
    if (NULL == action) {
        return -1;
    }

    action->tag = spawn_do_dup2;
    action->action.dup2_action.fd = fd;
    action->action.dup2_action.newfd = newfd;
    return 0;
}

int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *actions, const char *path)
{
    struct spawn_action * action = __posix_spawn_file_actions_add(actions);
    if (NULL == action) {
        return -1;
    }

    action->tag = spawn_do_chdir;
    action->action.chdir_action.path = (char *)path;
    return 0;
}

int posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t *actions, int fd)
{
    struct spawn_action * action = __posix_spawn_file_actions_add(actions);
    if (NULL == action) {
        return -1;
    }

    action->tag = spawn_do_fchdir;
    action->action.fchdir_action.fd = fd;
    return 0;
}

// termios.h
int tcgetattr(int fd, struct termios *termios)
{
    return ioctl(fd, TCGETS, termios);
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios)
{
    return ioctl(fd, TCSETS, termios);
}

pid_t tcgetpgrp(int fd)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    return process->id.gid;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
    vsf_linux_process_t *process = vsf_linux_get_cur_process();
    process->id.gid = pgrp;
    return 0;
}

int tcsendbreak(int fd, int duration)
{
    return 0;
}

int tcdrain(int fd)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(fd);
    if ((NULL == sfd) || (sfd->op != &vsf_linux_term_fdop)) { return -1; }
    vsf_linux_term_priv_t *term_priv = (vsf_linux_term_priv_t *)sfd->priv;
    __vsf_linux_tx_stream_drain(&term_priv->use_as__vsf_linux_stream_priv_t);
    return 0;
}

int tcflush(int fd, int queue_selector)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(fd);
    if ((NULL == sfd) || (sfd->op != &vsf_linux_term_fdop)) { return -1; }
    vsf_linux_term_priv_t *term_priv = (vsf_linux_term_priv_t *)sfd->priv;
    uint8_t op;

    if (queue_selector == TCIOFLUSH) {
        op = (1 << TCIFLUSH) | (1 << TCOFLUSH);
    } else {
        op = 1 << queue_selector;
    }

    if (op & (1 << TCIFLUSH)) {
        __vsf_linux_rx_stream_drop(&term_priv->use_as__vsf_linux_stream_priv_t);
    }
    if (op & (1 << TCOFLUSH)) {
        __vsf_linux_tx_stream_drop(&term_priv->use_as__vsf_linux_stream_priv_t);
    }
    return 0;
}

int tcflow(int fd, int action)
{
    return -1;
}

void cfmakeraw(struct termios *termios_p)
{
}

speed_t cfgetispeed(const struct termios *termios_p)
{
    return termios_p->c_ispeed;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
    return termios_p->c_ospeed;
}

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ospeed = speed;
    return 0;
}

int cfsetspeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ispeed = speed;
    termios_p->c_ospeed = speed;
    return 0;
}

int vsf_linux_expandenv(const char *str, char *output, size_t bufflen)
{
    size_t envlen = 0, copylen, tmplen, namelen;
    char ch, *env_start, *env_end, *name_start, *name_end, *env_value;

#define __copy2output(__str_from, __copylen)                                    \
        do {                                                                    \
            if (output != NULL) {                                               \
                tmplen = vsf_min(bufflen, __copylen);                           \
                if (tmplen > 0) {                                               \
                    memcpy(output, __str_from, tmplen);                         \
                    output += tmplen;                                           \
                }                                                               \
            }                                                                   \
            envlen += __copylen;                                                \
        } while (false)

    bufflen--;      // reserve one byte for '\0'
    while (true) {
        env_start = strchr(str, '$');
        if (NULL == env_start) {
            copylen = strlen(str);
            __copy2output(str, copylen);
            if (output != NULL) {
                *output = '\0';
            }
            return envlen;
        }
        copylen = env_start - str;
        __copy2output(str, copylen);

        if ('{' == env_start[1]) {
            name_start = env_end = env_start + 2;
        } else {
            name_start = env_end = env_start + 1;
        }
        while (true) {
            ch = *env_end;
            if (!(  (ch == '_')
                ||  ((ch >= '0') && (ch <= '9'))
                ||  ((ch >= 'a') && (ch <= 'z'))
                ||  ((ch >= 'A') && (ch <= 'Z')))) {
                break;
            }
            env_end++;
        }
        if ('{' == env_start[1]) {
            if ('}' != *env_end) {
                return -1;
            }
            name_end = env_end++ - 1;
        } else {
            name_end = env_end - 1;
        }
        if (name_end < name_start) {
            return -1;
        }

        namelen = name_end - name_start + 1;
        char env_name[namelen + 1];
        memcpy(env_name, name_start, namelen);
        env_name[namelen] = '\0';
        env_value = getenv(env_name);
        if (env_value != NULL) {
            copylen = strlen(env_value);
            __copy2output(env_value, copylen);
        }

        str = env_end;
    }

    return envlen;
}

// langinfo
char * nl_langinfo(nl_item item)
{
    switch (item) {
    case CODESET:   return "ANSI_X3.4-1968";
    default:        return NULL;
    }
}

// vplt
#if VSF_LINUX_APPLET_USE_SYS_RANDOM == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_RANDOM_FUNC(__FUNC)     .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_random_vplt_t vsf_linux_sys_random_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_random_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_RANDOM_FUNC(getrandom),
};
#endif

#if VSF_LINUX_APPLET_USE_SYS_SHM == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_SHM_FUNC(__FUNC)        .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_shm_vplt_t vsf_linux_sys_shm_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_shm_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_SHM_FUNC(shmget),
    VSF_LINUX_APPLET_SYS_SHM_FUNC(shmat),
    VSF_LINUX_APPLET_SYS_SHM_FUNC(shmdt),
    VSF_LINUX_APPLET_SYS_SHM_FUNC(shmctl),
};
#endif

#if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED
#if VSF_LINUX_APPLET_USE_SYS_TIME == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_TIME_FUNC(__FUNC)       .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_time_vplt_t vsf_linux_sys_time_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_time_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_TIME_FUNC(gettimeofday),
    VSF_LINUX_APPLET_SYS_TIME_FUNC(getitimer),
    VSF_LINUX_APPLET_SYS_TIME_FUNC(setitimer),
    VSF_LINUX_APPLET_SYS_TIME_FUNC(futimes),
    VSF_LINUX_APPLET_SYS_TIME_FUNC(utimes),
};
#endif
#endif

#if VSF_LINUX_APPLET_USE_SYS_UTSNAME == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_UTSNAME_FUNC(__FUNC)    .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_utsname_vplt_t vsf_linux_sys_utsname_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_utsname_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_UTSNAME_FUNC(uname),
};
#endif

#if VSF_LINUX_APPLET_USE_SYS_WAIT == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_WAIT_FUNC(__FUNC)       .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_wait_vplt_t vsf_linux_sys_wait_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_wait_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_WAIT_FUNC(wait),
    VSF_LINUX_APPLET_SYS_WAIT_FUNC(waitpid),
    VSF_LINUX_APPLET_SYS_WAIT_FUNC(waitid),
};
#endif

#if VSF_LINUX_APPLET_USE_UNISTD == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_UNISTD_FUNC(__FUNC)         .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_unistd_vplt_t vsf_linux_unistd_vplt = {
    .info.entry_num = (sizeof(vsf_linux_unistd_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_UNISTD_FUNC(__vsf_linux_errno),
    VSF_LINUX_APPLET_UNISTD_FUNC(usleep),
    VSF_LINUX_APPLET_UNISTD_FUNC(sleep),
    VSF_LINUX_APPLET_UNISTD_FUNC(alarm),
    VSF_LINUX_APPLET_UNISTD_FUNC(ualarm),
    VSF_LINUX_APPLET_UNISTD_FUNC(getgid),
    VSF_LINUX_APPLET_UNISTD_FUNC(getegid),
    VSF_LINUX_APPLET_UNISTD_FUNC(getuid),
    VSF_LINUX_APPLET_UNISTD_FUNC(geteuid),
    VSF_LINUX_APPLET_UNISTD_FUNC(getpid),
    VSF_LINUX_APPLET_UNISTD_FUNC(getppid),
    VSF_LINUX_APPLET_UNISTD_FUNC(gettid),
    VSF_LINUX_APPLET_UNISTD_FUNC(__execl_va),
    VSF_LINUX_APPLET_UNISTD_FUNC(execl),
    VSF_LINUX_APPLET_UNISTD_FUNC(__execlp_va),
    VSF_LINUX_APPLET_UNISTD_FUNC(execlp),
    VSF_LINUX_APPLET_UNISTD_FUNC(execv),
    VSF_LINUX_APPLET_UNISTD_FUNC(execve),
    VSF_LINUX_APPLET_UNISTD_FUNC(execvp),
    VSF_LINUX_APPLET_UNISTD_FUNC(execvpe),
    VSF_LINUX_APPLET_UNISTD_FUNC(daemon),
    VSF_LINUX_APPLET_UNISTD_FUNC(sysconf),
    VSF_LINUX_APPLET_UNISTD_FUNC(pathconf),
    VSF_LINUX_APPLET_UNISTD_FUNC(fpathconf),
    VSF_LINUX_APPLET_UNISTD_FUNC(realpath),
    VSF_LINUX_APPLET_UNISTD_FUNC(pipe),
    VSF_LINUX_APPLET_UNISTD_FUNC(pipe2),
    VSF_LINUX_APPLET_UNISTD_FUNC(creat),
    VSF_LINUX_APPLET_UNISTD_FUNC(__open_va),
    VSF_LINUX_APPLET_UNISTD_FUNC(open),
    VSF_LINUX_APPLET_UNISTD_FUNC(__openat_va),
    VSF_LINUX_APPLET_UNISTD_FUNC(openat),
    VSF_LINUX_APPLET_UNISTD_FUNC(access),
    VSF_LINUX_APPLET_UNISTD_FUNC(unlink),
    VSF_LINUX_APPLET_UNISTD_FUNC(unlinkat),
    VSF_LINUX_APPLET_UNISTD_FUNC(link),
    VSF_LINUX_APPLET_UNISTD_FUNC(mkdir),
    VSF_LINUX_APPLET_UNISTD_FUNC(mkdirat),
    VSF_LINUX_APPLET_UNISTD_FUNC(rmdir),
    VSF_LINUX_APPLET_UNISTD_FUNC(dup),
    VSF_LINUX_APPLET_UNISTD_FUNC(dup2),
    VSF_LINUX_APPLET_UNISTD_FUNC(dup3),
    VSF_LINUX_APPLET_UNISTD_FUNC(chroot),
    VSF_LINUX_APPLET_UNISTD_FUNC(chdir),
    VSF_LINUX_APPLET_UNISTD_FUNC(getcwd),
    VSF_LINUX_APPLET_UNISTD_FUNC(close),
    VSF_LINUX_APPLET_UNISTD_FUNC(lseek),
    VSF_LINUX_APPLET_UNISTD_FUNC(read),
    VSF_LINUX_APPLET_UNISTD_FUNC(write),
    VSF_LINUX_APPLET_UNISTD_FUNC(readv),
    VSF_LINUX_APPLET_UNISTD_FUNC(writev),
    VSF_LINUX_APPLET_UNISTD_FUNC(pread),
    VSF_LINUX_APPLET_UNISTD_FUNC(pwrite),
    VSF_LINUX_APPLET_UNISTD_FUNC(preadv),
    VSF_LINUX_APPLET_UNISTD_FUNC(pwritev),
    VSF_LINUX_APPLET_UNISTD_FUNC(fsync),
    VSF_LINUX_APPLET_UNISTD_FUNC(fdatasync),
    VSF_LINUX_APPLET_UNISTD_FUNC(isatty),
    VSF_LINUX_APPLET_UNISTD_FUNC(getpagesize),
    VSF_LINUX_APPLET_UNISTD_FUNC(symlink),
    VSF_LINUX_APPLET_UNISTD_FUNC(ftruncate),
    VSF_LINUX_APPLET_UNISTD_FUNC(truncate),
    VSF_LINUX_APPLET_UNISTD_FUNC(ftruncate64),
    VSF_LINUX_APPLET_UNISTD_FUNC(truncate64),
    VSF_LINUX_APPLET_UNISTD_FUNC(readlink),
    VSF_LINUX_APPLET_UNISTD_FUNC(tcgetpgrp),
    VSF_LINUX_APPLET_UNISTD_FUNC(tcsetpgrp),
    VSF_LINUX_APPLET_UNISTD_FUNC(getpass),
    VSF_LINUX_APPLET_UNISTD_FUNC(gethostname),
    VSF_LINUX_APPLET_UNISTD_FUNC(sethostname),
    VSF_LINUX_APPLET_UNISTD_FUNC(chown),
    VSF_LINUX_APPLET_UNISTD_FUNC(fchown),
    VSF_LINUX_APPLET_UNISTD_FUNC(lchown),
    VSF_LINUX_APPLET_UNISTD_FUNC(fchownat),
    VSF_LINUX_APPLET_UNISTD_FUNC(getentropy),
};
#endif

#if VSF_LINUX_USE_APPLET == ENABLED && !defined(__VSF_APPLET__)
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   if VSF_LINUX_APPLET_USE_PTHREAD == ENABLED
#       include "./include/pthread.h"
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_EPOLL == ENABLED
#       include "./include/sys/epoll.h"
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_EVENTFD == ENABLED
#       include "./include/sys/eventfd.h"
#   endif
#   if VSF_LINUX_APPLET_USE_NETDB == ENABLED
#       include "./include/netdb.h"
#   endif
#   if VSF_LINUX_APPLET_USE_LIBUSB == ENABLED
#       include "./include/libusb/libusb.h"
#   endif
#else
#   if VSF_LINUX_APPLET_USE_PTHREAD == ENABLED
#       include <pthread.h>
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_EPOLL == ENABLED
#       include <sys/epoll.h>
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_EVENTFD == ENABLED
#       include <sys/eventfd.h>
#   endif
#   if VSF_LINUX_APPLET_USE_NETDB == ENABLED
#       include <netdb.h>
#   endif
#   if VSF_LINUX_APPLET_USE_LIBUSB == ENABLED
#       include <libusb/libusb.h>
#   endif
#endif

__VSF_VPLT_DECORATOR__ vsf_linux_vplt_t vsf_linux_vplt = {
    .info.entry_num = (sizeof(vsf_linux_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

#   if VSF_LINUX_APPLET_USE_LIBC_STDIO == ENABLED
    .libc_stdio     = (void *)&vsf_linux_libc_stdio_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_LIBC_STDLIB == ENABLED
    .libc_stdlib    = (void *)&vsf_linux_libc_stdlib_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_LIBC_STRING == ENABLED
    .libc_string    = (void *)&vsf_linux_libc_string_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_LIBC_TIME == ENABLED
    .libc_time      = (void *)&vsf_linux_libc_time_vplt,
#   endif

#   if VSF_LINUX_APPLET_USE_SYS_EPOLL == ENABLED
    .sys_epoll      = (void *)&vsf_linux_sys_epoll_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_EVENTFD == ENABLED
    .sys_eventfd    = (void *)&vsf_linux_sys_eventfd_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_RANDOM == ENABLED
    .sys_random     = (void *)&vsf_linux_sys_random_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_SELECT == ENABLED
    .sys_select     = (void *)&vsf_linux_sys_select_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_RANDOM == ENABLED
    .sys_shm        = (void *)&vsf_linux_sys_shm_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_STAT == ENABLED
    .sys_stat       = (void *)&vsf_linux_sys_stat_vplt,
#   endif
#   if VSF_KERNEL_CFG_EDA_SUPPORT_TIMER == ENABLED && VSF_LINUX_APPLET_USE_SYS_TIME == ENABLED
    .sys_time       = (void *)&vsf_linux_sys_time_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SYS_UTSNAME == ENABLED
    .sys_stat       = (void *)&vsf_linux_sys_utsname_vplt,
#   endif
#   if VSF_LINUX_USE_SOCKET == ENABLED && VSF_LINUX_APPLET_USE_SYS_SOCKET == ENABLED
    .sys_socket     = (void *)&vsf_linux_sys_socket_vplt,
#   endif

#   if VSF_LINUX_APPLET_USE_UNISTD == ENABLED
    .unistd         = (void *)&vsf_linux_unistd_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_PTHREAD == ENABLED
    .pthread        = (void *)&vsf_linux_pthread_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_POLL == ENABLED
    .poll           = (void *)&vsf_linux_poll_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_SEMAPHORE == ENABLED
    .semaphore      = (void *)&vsf_linux_semaphore_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_DIRENT == EANBLED
    .dirent         = (void *)&vsf_linux_dirent_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_FCNTL == EANBLED
    .fcntl          = (void *)&vsf_linux_fcntl_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_IFADDRS == EANBLED
    .ifaddrs        = (void *)&vsf_linux_ifaddrs_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_NETDB == ENABLED
    .netdb          = (void *)&vsf_linux_netdb_vplt,
#   endif

#   if VSF_LINUX_USE_LIBUSB == ENABLED && VSF_LINUX_APPLET_USE_LIBUSB == ENABLED
    .libusb         = (void *)&vsf_linux_libusb_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_LIBGETOPT == EANBLED
    .libgetopt      = (void *)&vsf_linux_libgetopt_vplt,
#   endif
#   if VSF_LINUX_APPLET_USE_LIBGEN == EANBLED
    .libgen         = (void *)&vsf_linux_libgen_vplt,
#   endif
};
#endif

#if __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#endif

#endif      // VSF_USE_LINUX
