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

#include "shell/sys/linux/vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED && VSF_LINUX_USE_SOCKET == ENABLED

#define __VSF_LINUX_CLASS_INHERIT__
#define __VSF_LINUX_FS_CLASS_INHERIT__
#define __VSF_LINUX_SOCKET_CLASS_IMPLEMENT
#if VSF_LINUX_CFG_RELATIVE_PATH == ENABLED
#   include "../../include/unistd.h"
#   include "../../include/fcntl.h"
#   if VSF_LINUX_USE_SIMPLE_STDIO == ENABLED
#       include "../../include/simple_libc/stdio.h"
#   endif
#   if VSF_LINUX_SOCKET_USE_INET == ENABLED
#       include "../../include/netinet/in.h"
#       include "../../include/arpa/inet.h"
#       include "../../include/netdb.h"
#       include "../../include/ifaddrs.h"
#   endif
#else
#   include <unistd.h>
#   include <stdio.h>
#   include <fcntl.h>
#   if VSF_LINUX_SOCKET_USE_INET == ENABLED
#       include <netinet/in.h>
#       include <arpa/inet.h>
#       include <netdb.h>
#       include <ifaddrs.h>
#   endif
#endif
#include "./vsf_linux_socket.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

#if VSF_LINUX_SOCKET_USE_UNIX == ENABLED
extern const vsf_linux_socket_op_t vsf_linux_socket_unix_op;
#endif
#if VSF_LINUX_SOCKET_USE_INET == ENABLED
extern const vsf_linux_socket_op_t vsf_linux_socket_inet_op;
#endif

/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

#if VSF_LINUX_SOCKET_USE_INET == ENABLED
// arpa/inet.h
int inet_aton(const char *cp, struct in_addr *addr)
{
    uint32_t parts[4];
    int num_parts = 0;
    char *endp;

    for (;;) {
        parts[num_parts++] = strtoul(cp, &endp, 0);
        if (cp == endp) {
            return 0;
        }

        if ((*endp != '.') || (num_parts >= 4)) {
            break;
        }
        cp = endp + 1;
    }

    uint32_t val = parts[num_parts - 1];
    switch (num_parts) {
    case 1:     // a        -- 32 bits
        break;
    case 2:     // a.b      -- 8.24 bits
        if (val > 0xFFFFFF) {
            return 0;
        }
        val |= parts[0] << 24;
        break;
    case 3:     // a.b.c    -- 8.8.16 bits
        if (val > 0xFFFF) {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;
    case 4:     // a.b.c.d  -- 8.8.8.8 bits
        if (val > 0xFF) {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    }
    if (addr) {
        addr->s_addr = htonl(val);
    }
    return 1;
}

in_addr_t inet_addr(const char *cp)
{
    struct in_addr addr;
    if (!inet_aton(cp, &addr)) {
        return INADDR_NONE;
    }
    return addr.s_addr;
}

in_addr_t inet_lnaof(struct in_addr in)
{
    uint32_t val = ntohl(in.s_addr);
    if (IN_CLASSA(val)) {
        return val & IN_CLASSA_HOST;
    } else if (IN_CLASSB(val)) {
        return val & IN_CLASSB_HOST;
    } else {
        return val & IN_CLASSC_HOST;
    }
}

struct in_addr inet_makeaddr(in_addr_t net, in_addr_t lna)
{
    in_addr_t addr;

    if (net < 128) {
        addr = (net << IN_CLASSA_NSHIFT) | (lna & IN_CLASSA_HOST);
    } else if (net < 0x10000) {
        addr = (net << IN_CLASSB_NSHIFT) | (lna & IN_CLASSB_HOST);
    } else if (net < 0x1000000L) {
        addr = (net << IN_CLASSC_NSHIFT) | (lna & IN_CLASSC_HOST);
    } else {
        addr = net | lna;
    }
    addr = htonl(addr);
    return (*(struct in_addr *)&addr);
}

in_addr_t inet_netof(struct in_addr in)
{
    uint32_t val = ntohl(in.s_addr);
    if (IN_CLASSA(val)) {
        return (val & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT;
    } else if (IN_CLASSB(val)) {
        return (val & IN_CLASSB_NET) >> IN_CLASSB_NSHIFT;
    } else {
        return (val & IN_CLASSC_NET) >> IN_CLASSC_NSHIFT;
    }
}

char * inet_ntoa(struct in_addr in)
{
    static char __inet_ntoa_buf[16];
    unsigned char *a = (void *)&in;
    snprintf(__inet_ntoa_buf, sizeof(__inet_ntoa_buf), "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
    return __inet_ntoa_buf;
}

in_addr_t inet_network(const char *cp)
{
    return ntohl(inet_addr(cp));
}

const char * inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    switch (af) {
    case AF_INET:
        strncpy(dst, inet_ntoa(*(struct in_addr *)src), size);
        break;
    case AF_INET6:
        // TODO: add ipv6 support
        return NULL;
    }
    return dst;
}

// fcntl, dedicated driver can over-write this common version
WEAK(__vsf_linux_socket_inet_fcntl)
int __vsf_linux_socket_inet_fcntl(vsf_linux_fd_t *sfd, int cmd, uintptr_t arg)
{
    switch (cmd) {
    case F_SETFL:
        if (arg & O_NONBLOCK) {
            setsockopt(sfd->fd, SOL_SOCKET, SO_NONBLOCK, &arg, sizeof(arg));
        }
        break;
    }
    return 0;
}

// netdb
int * __vsf_linux_h_errno(void)
{
    vsf_linux_thread_t *thread = vsf_linux_get_cur_thread();
    VSF_LINUX_ASSERT(thread != NULL);
    return &thread->__h_errno;
}

const char * gai_strerror(int errcode)
{
    return (const char *)"unknown error";
}

struct hostent * gethostbyaddr(const void *addr, size_t len, int type)
{
    return NULL;
}

struct servent * getservbyname(const char *name, const char *proto)
{
    VSF_LINUX_ASSERT(false);
    return NULL;
}

struct servent * getservbyport(int port, const char *proto)
{
    VSF_LINUX_ASSERT(false);
    return NULL;
}

struct servent * getservent(void)
{
    VSF_LINUX_ASSERT(false);
    return NULL;
}

struct hostent * gethostbyname(const char *name)
{
    static struct hostent __hostent;
    static char * __h_addr_list[2];
    static in_addr_t __addr;

    extern int __inet_gethostbyname(const char *name, in_addr_t *addr);
    if (__inet_gethostbyname(name, &__addr) < 0) {
        return NULL;
    }

    __h_addr_list[0] = (char *)&__addr;
    __h_addr_list[1] = NULL;
    memset(&__hostent, 0, sizeof(__hostent));
    __hostent.h_name = (char *)name;
    __hostent.h_addrtype = AF_INET;
    __hostent.h_length = 4;
    __hostent.h_addr_list = (char **)&__h_addr_list;
    return &__hostent;
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                        char *host, socklen_t hostlen,
                        char *serv, socklen_t servlen, int flags)
{
    return -1;
}

int getaddrinfo(const char *name, const char *service, const struct addrinfo *hints,
                        struct addrinfo **pai)
{
    static const struct addrinfo __default_hints = {
        .ai_flags = AI_V4MAPPED | AI_ADDRCONFIG,
        .ai_family = PF_UNSPEC,
        .ai_socktype = 0,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = NULL,
        .ai_canonname = NULL,
        .ai_next = NULL,
    };

    if (name != NULL && name[0] == '*' && name[1] == 0) {
        name = NULL;
    }
    if (service != NULL && service[0] == '*' && service[1] == 0) {
        service = NULL;
    }
    if (name == NULL && service == NULL) {
        return EAI_NONAME;
    }
    if (hints == NULL) {
        hints = &__default_hints;
    }
    if (hints->ai_flags
        & ~(    AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_ADDRCONFIG
            |   AI_V4MAPPED | AI_NUMERICSERV | AI_ALL)) {
        return EAI_BADFLAGS;
    }
    if ((hints->ai_flags & AI_CANONNAME) && name == NULL) {
        return EAI_BADFLAGS;
    }

    // TODO: re-implement
    struct in_addr addr;
    if (!inet_aton(name, &addr)) {
        return EAI_NONAME;
    }

    struct __addrinfo {
        struct addrinfo info;
        union {
            struct sockaddr sa;
            struct sockaddr_in sa_in;
            struct sockaddr_in6 sa_in6;
        };
    };
    struct __addrinfo *ai = malloc(sizeof(struct __addrinfo));
    if (NULL == ai) {
        return EAI_MEMORY;
    }
    memset(ai, 0, sizeof(struct __addrinfo));

    ai->info.ai_family      = AF_INET;
    ai->info.ai_addr        = &ai->sa;
    ai->sa_in.sin_family    = AF_INET;
    ai->sa_in.sin_addr      = addr;
    ai->info.ai_addrlen     = sizeof(ai->sa_in);
    *pai = &ai->info;
    return 0;
}

void freeaddrinfo(struct addrinfo *ai)
{
    struct addrinfo *p;
    while (ai != NULL) {
        p = ai;
        ai = ai->ai_next;

        if (p->ai_canonname != NULL) {
            free(p->ai_canonname);
        }
        free(p);
    }
}
#endif

int socket(int domain, int type, int protocol)
{
    const vsf_linux_socket_op_t *sockop = NULL;
    vsf_linux_fd_t *sfd;
    int fd;

    switch (domain) {
#if VSF_LINUX_SOCKET_USE_UNIX == ENABLED
    case AF_UNIX:
        sockop = &vsf_linux_socket_unix_op; break;
#endif
#if VSF_LINUX_SOCKET_USE_INET == ENABLED
    case AF_INET:
        sockop = &vsf_linux_socket_inet_op; break;
#endif
    default: return -1;
    }

    fd = vsf_linux_fd_create(&sfd, &sockop->fdop);
    if (fd >= 0) {
        vsf_linux_socket_priv_t *priv = (struct vsf_linux_socket_priv_t *)sfd->priv;
        priv->sockop = sockop;
        priv->domain = domain;
        priv->type = type;
        priv->protocol = protocol;

        if (sockop->fn_init != NULL) {
            if (sockop->fn_init(sfd) != 0) {
                vsf_linux_fd_delete(fd);
                return -1;
            }
        }
    }
    return fd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_connect != NULL));
    return priv->sockop->fn_connect(priv, addr, addrlen);
}

int listen(int sockfd, int backlog)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_listen != NULL));
    return priv->sockop->fn_listen(priv, backlog);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_accept != NULL));
    return priv->sockop->fn_accept(priv, addr, addrlen);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_bind != NULL));
    return priv->sockop->fn_bind(priv, addr, addrlen);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_getsockopt != NULL));
    return priv->sockop->fn_getsockopt(priv, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_setsockopt != NULL));
    int err = priv->sockop->fn_setsockopt(priv, level, optname, optval, optlen);

    switch (level) {
    case SOL_SOCKET:
        switch (optname) {
        case SO_NONBLOCK:
            if (*(const int *)optval) {
                sfd->priv->flags |= O_NONBLOCK;
            } else {
                sfd->priv->flags &= ~O_NONBLOCK;
            }
            break;
        }
        break;
    }
    return err;
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_getpeername != NULL));
    return priv->sockop->fn_getpeername(priv, addr, addrlen);
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_getsockname != NULL));
    return priv->sockop->fn_getsockname(priv, addr, addrlen);
}

int shutdown(int sockfd, int how)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    VSF_LINUX_ASSERT((priv->sockop != NULL) && (priv->sockop->fn_fini != NULL));
    return priv->sockop->fn_fini(priv, how);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    sfd->cur_wrflags = flags;
    return write(sockfd, buf, len);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    sfd->cur_rdflags = flags;
    return read(sockfd, buf, len);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    if ((msg->msg_control != NULL) && (msg->msg_controllen > 0)) {
        VSF_LINUX_ASSERT(NULL == priv->msg);
        struct msghdr *tx_msg = __malloc_ex(vsf_linux_resources_process(), sizeof(*msg) + msg->msg_controllen);
        if (NULL == priv->msg) {
            return -1;
        }

        *tx_msg = *msg;
        tx_msg->msg_control = (void *)&tx_msg[1];
        memcpy(tx_msg->msg_control, msg->msg_control, msg->msg_controllen);
        priv->msg = tx_msg;
        priv->sender_process = vsf_linux_get_cur_process();
    }

    sfd->cur_wrflags = flags;
    return writev(sockfd, msg->msg_iov, msg->msg_iovlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    vsf_linux_fd_t *sfd = vsf_linux_fd_get(sockfd);
    if (!sfd) {
        return -1;
    }
    sfd->cur_rdflags = flags;

    ssize_t result = readv(sockfd, msg->msg_iov, msg->msg_iovlen);

    vsf_linux_socket_priv_t *priv = (vsf_linux_socket_priv_t *)sfd->priv;
    if (priv->msg != NULL) {
        struct msghdr *rx_msg = priv->msg;
        priv->msg = NULL;
        if (msg != NULL) {
            *msg = *rx_msg;

            struct cmsghdr *cmsg = msg->msg_control;
            struct cmsghdr *rx_cmsg = rx_msg->msg_control;
            if ((cmsg != NULL) && (msg->msg_control != NULL)) {
                VSF_LINUX_ASSERT(msg->msg_controllen == rx_msg->msg_controllen);
                VSF_LINUX_ASSERT(cmsg->cmsg_len == rx_cmsg->cmsg_len);
                VSF_LINUX_ASSERT(cmsg->cmsg_level == rx_cmsg->cmsg_level);
                VSF_LINUX_ASSERT(cmsg->cmsg_type == rx_cmsg->cmsg_type);
                memcpy(cmsg, rx_cmsg, cmsg->cmsg_len);

                void *data = CMSG_DATA(cmsg), *rx_data = CMSG_DATA(rx_cmsg);
                int datalen = msg->msg_controllen - sizeof(*msg);
                vsf_linux_process_t *sender_process = priv->sender_process;
                priv->sender_process = NULL;

                switch (cmsg->cmsg_type) {
                case SCM_RIGHTS:
                    while (datalen > 0) {
                        int fd = *(int *)rx_data;
                        extern vsf_linux_fd_t * __vsf_linux_fd_get_ex(vsf_linux_process_t *process, int fd);
                        vsf_linux_fd_t *sfd = __vsf_linux_fd_get_ex(sender_process, fd);
                        if (NULL == sfd) {
                            result = -1;
                            break;
                        }

                        extern int __vsf_linux_fd_create_ex(vsf_linux_process_t *process, vsf_linux_fd_t **sfd,
                            const vsf_linux_fd_op_t *op, int fd_desired, vsf_linux_fd_priv_t *priv);
                        *(int *)data = __vsf_linux_fd_create_ex(NULL, NULL, sfd->op, -1, sfd->priv);
                        data = (void *)((uint8_t *)data + sizeof(int));
                        rx_data = (void *)((uint8_t *)rx_data + sizeof(int));
                        VSF_LINUX_ASSERT(datalen >= sizeof(int));
                        datalen -= sizeof(int);
                    }
                    break;
                default:
                    VSF_LINUX_ASSERT(false);
                    break;
                }
            }
        }
        __free_ex(vsf_linux_resources_process(), rx_msg);
    }
    return result;
}

int socketpair(int domain, int type, int protocol, int socket_vector[2])
{
    socket_vector[0] = socket(domain, type, protocol);
    socket_vector[1] = socket(domain, type, protocol);
    if ((socket_vector[0] < 0) || (socket_vector[1] < 0)) {
        goto fail;
    }

    vsf_linux_fd_t *rsfd = vsf_linux_fd_get(socket_vector[0]);
    vsf_linux_fd_t *wsfd = vsf_linux_fd_get(socket_vector[1]);
    if (NULL == ((vsf_linux_socket_op_t *)rsfd->op)->fn_socketpair) {
        goto fail;
    }

    return ((vsf_linux_socket_op_t *)rsfd->op)->fn_socketpair(rsfd, wsfd);

fail:
    if (socket_vector[0] >= 0) {
        close(socket_vector[0]);
    }
    if (socket_vector[1] >= 0) {
        close(socket_vector[1]);
    }
    return -1;
}

#if VSF_LINUX_SOCKET_USE_INET == ENABLED
// ifaddrs.h
WEAK(getifaddrs)
int getifaddrs(struct ifaddrs **ifaddrs)
{
    return 0;
}

WEAK(freeifaddrs)
void freeifaddrs(struct ifaddrs *ifaddrs)
{
}

#if VSF_LINUX_APPLET_USE_IFADDRS == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_IFADDRS_FUNC(__FUNC)        .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_ifaddrs_vplt_t vsf_linux_ifaddrs_vplt = {
    .info.entry_num = (sizeof(vsf_linux_ifaddrs_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_IFADDRS_FUNC(getifaddrs),
    VSF_LINUX_APPLET_IFADDRS_FUNC(freeifaddrs),
};
#endif
#endif

#if VSF_LINUX_APPLET_USE_NETDB == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_NETDB_FUNC(__FUNC)          .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_netdb_vplt_t vsf_linux_netdb_vplt = {
    .info.entry_num = (sizeof(vsf_linux_netdb_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_NETDB_FUNC(gethostbyaddr),
    VSF_LINUX_APPLET_NETDB_FUNC(gethostbyname),
    VSF_LINUX_APPLET_NETDB_FUNC(gai_strerror),
    VSF_LINUX_APPLET_NETDB_FUNC(getnameinfo),
    VSF_LINUX_APPLET_NETDB_FUNC(getaddrinfo),
    VSF_LINUX_APPLET_NETDB_FUNC(freeaddrinfo),
};
#endif

#if VSF_LINUX_APPLET_USE_SYS_SOCKET == ENABLED && !defined(__VSF_APPLET__)
#   define VSF_LINUX_APPLET_SYS_SOCKET_FUNC(__FUNC)     .__FUNC = __FUNC
__VSF_VPLT_DECORATOR__ vsf_linux_sys_socket_vplt_t vsf_linux_sys_socket_vplt = {
    .info.entry_num = (sizeof(vsf_linux_sys_socket_vplt_t) - sizeof(vsf_vplt_info_t)) / sizeof(void *),

    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(setsockopt),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(getsockopt),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(getpeername),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(getsockname),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(accept),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(bind),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(connect),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(listen),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(recv),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(recvmsg),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(recvfrom),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(send),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(sendmsg),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(sendto),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(shutdown),
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(socket),
    // TODO: add socketpair if supported
    VSF_LINUX_APPLET_SYS_SOCKET_FUNC(socketpair),
};
#endif

#endif
