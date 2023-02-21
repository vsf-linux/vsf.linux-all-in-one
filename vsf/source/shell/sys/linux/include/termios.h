#ifndef __VSF_LINUX_TERMIOS_H__
#define __VSF_LINUX_TERMIOS_H__

#include "shell/sys/linux/vsf_linux_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VSF_LINUX_CFG_WRAPPER == ENABLED
#define tcgetattr   VSF_LINUX_WRAPPER(tcgetattr)
#define tcsetattr   VSF_LINUX_WRAPPER(tcsetattr)
#define tcsendbreak VSF_LINUX_WRAPPER(tcsendbreak)
#define tcdrain     VSF_LINUX_WRAPPER(tcdrain)
#define tcflush     VSF_LINUX_WRAPPER(tcflush)
#define tcflow      VSF_LINUX_WRAPPER(tcflow)
#define cfmakeraw   VSF_LINUX_WRAPPER(cfmakeraw)
#define cfgetispeed VSF_LINUX_WRAPPER(cfgetispeed)
#define cfgetospeed VSF_LINUX_WRAPPER(cfgetospeed)
#define cfsetispeed VSF_LINUX_WRAPPER(cfsetispeed)
#define cfsetospeed VSF_LINUX_WRAPPER(cfsetospeed)
#define cfsetspeed  VSF_LINUX_WRAPPER(cfsetspeed)
#endif

// for ioctrl
#define TCGETS      (('t' << 8) | 0)
#define TCSETS      (('t' << 8) | 1)

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;

// tcsetattr optional_actions
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

#define IGNBRK      0000001
#define BRKINT      0000002
#define IGNPAR      0000004
#define PARMRK      0000010
#define INPCK       0000020
#define ISTRIP      0000040
#define INLCR       0000100
#define IGNCR       0000200
#define ICRNL       0000400
#define IUCLC       0001000
#define IXON        0002000
#define IXANY       0004000
#define IXOFF       0010000
#define IMAXBEL     0020000
#define IUTF8       0040000

#define OPOST       0000001
#define OLCUC       0000002
#define ONLCR       0000004
#define OCRNL       0000010
#define ONOCR       0000020
#define ONLRET      0000040
#define OFILL       0000100
#define OFDEL       0000200

#define ISIG        0000001
#define ICANON      0000002
#define ECHO        0000010
#define ECHOE       0000020
#define ECHOK       0000040
#define ECHONL      0000100
#define NOFLSH      0000200
#define TOSTOP      0000400
#define IEXTEN      0100000

/* tcflow() and TCXONC use these */
#define TCOOFF      0
#define TCOON       1
#define TCIOFF      2
#define TCION       3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH    0
#define TCOFLUSH    1
#define TCIOFLUSH   2

/* tcsetattr uses these */
#define TCSANOW     0
#define TCSADRAIN   1
#define TCSAFLUSH   2

#define NCCS        32
#define VINTR       0
#define VQUIT       1
#define VERASE      2
#define VKILL       3
#define VEOF        4
#define VTIME       5
#define VMIN        6
#define VSWTC       7
#define VSTART      8
#define VSTOP       9
#define VSUSP       10
#define VEOL        11
#define VREPRINT    12
#define VDISCARD    13
#define VWERASE     14
#define VLNEXT      15
#define VEOL2       16

// baudrate
#define CBAUD       0x100F
// use enum to avoid conflict
enum {
        B0          = 0,
        B50         = 1,
#define B50         B50
        B75         = 2,
#define B75         B75
        B110        = 3,
#define B110        B110
        B134        = 4,
#define B134        B134
        B150        = 5,
#define B150        B150
        B200        = 6,
#define B200        B200
        B300        = 7,
#define B300        B300
        B600        = 8,
#define B600        B600
        B1200       = 9,
#define B1200       B1200
        B1800       = 10,
#define B1800       B1800
        B2400       = 11,
#define B2400       B2400
        B4800       = 12,
#define B4800       B4800
        B9600       = 13,
#define B9600       B9600
        B19200      = 14,
#define B19200      B19200
        B38400      = 15,
#define B38400      B38400
        B57600      = 16,
#define B57600      B57600
        B115200     = 17,
#define B115200     B115200
        B230400     = 18,
#define B230400     B230400
        B460800     = 19,
#define B460800     B460800
        B500000     = 20,
#define B500000     B500000
        B576000     = 21,
#define B576000     B576000
        B921600     = 22,
#define B921600     B921600
        B1000000    = 23,
#define B1000000    B1000000
        B1152000    = 24,
#define B1152000    B1152000
        B1500000    = 25,
#define B1500000    B1500000
        B2000000    = 26,
#define B2000000    B2000000
        B2500000    = 27,
#define B2500000    B2500000
        B3000000    = 28,
#define B3000000    B3000000
        B3500000    = 29,
#define B3500000    B3500000
        B4000000    = 30,
#define B4000000    B4000000
};

#define CSIZE       0x0030
#define CS5         0x0000
#define CS6         0x0010
#define CS7         0x0020
#define CS8         0x0030

#define CSTOPB      0x0040
#define CREAD       0x0080
#define PARENB      0x0100
#define PARODD      0x0200
#define CLOCAL      0X0400

#define CRTSCTS     0x0400

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

#if VSF_LINUX_APPLET_USE_TERMIOS == ENABLED
typedef struct vsf_linux_termios_vplt_t {
    vsf_vplt_info_t info;
} vsf_linux_termios_vplt_t;
#   ifndef __VSF_APPLET__
extern __VSF_VPLT_DECORATOR__ vsf_linux_termios_vplt_t vsf_linux_termios_vplt;
#   endif
#endif

#if defined(__VSF_APPLET__) && VSF_LINUX_APPLET_USE_TERMIOS == ENABLED

#ifndef VSF_LINUX_APPLET_TERMIOS_VPLT
#   if VSF_LINUX_USE_APPLET == ENABLED
#       define VSF_LINUX_APPLET_TERMIOS_VPLT                                    \
            ((vsf_linux_termios_vplt_t *)(VSF_LINUX_APPLET_VPLT->termios))
#   else
#       define VSF_LINUX_APPLET_TERMIOS_VPLT                                    \
            ((vsf_linux_termios_vplt_t *)vsf_vplt((void *)0))
#   endif
#endif

#else       // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_TERMIOS

int tcgetattr(int fd, struct termios *termios);
int tcsetattr(int fd, int optional_actions, const struct termios *termios);
int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
void cfmakeraw(struct termios *termios_p);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetspeed(struct termios *termios_p, speed_t speed);

#endif      // __VSF_APPLET__ && VSF_LINUX_APPLET_USE_TERMIOS

#ifdef __cplusplus
}
#endif

#endif
