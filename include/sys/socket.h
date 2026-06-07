#ifndef NN_SYS_SOCKET_H
#define NN_SYS_SOCKET_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __socklen_t_defined
typedef uint32_t socklen_t;
#define __socklen_t_defined 1
#endif
typedef uint8_t sa_family_t;

#define AF_UNSPEC   0
#define AF_INET     2
#define AF_ROUTE    17
#define AF_LINK     18
#define AF_INET6    28
#define PF_UNSPEC   AF_UNSPEC
#define PF_INET     AF_INET
#define PF_INET6    AF_INET6

#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3
#define SOCK_SEQPACKET 5

#define SOL_SOCKET  0xffff

#define SO_DEBUG       0x0001
#define SO_ACCEPTCONN  0x0002
#define SO_REUSEADDR   0x0004
#define SO_KEEPALIVE   0x0008
#define SO_DONTROUTE   0x0010
#define SO_BROADCAST   0x0020
#define SO_USELOOPBACK 0x0040
#define SO_LINGER      0x0080
#define SO_OOBINLINE   0x0100
#define SO_REUSEPORT   0x0200
#define SO_SNDBUF      0x1001
#define SO_RCVBUF      0x1002
#define SO_SNDLOWAT    0x1003
#define SO_RCVLOWAT    0x1004
#define SO_SNDTIMEO    0x1005
#define SO_RCVTIMEO    0x1006
#define SO_ERROR       0x1007
#define SO_TYPE        0x1008

#define SHUT_RD    0
#define SHUT_WR    1
#define SHUT_RDWR  2

#define MSG_OOB       0x00001
#define MSG_PEEK      0x00002
#define MSG_DONTROUTE 0x00004
#define MSG_EOR       0x00008
#define MSG_TRUNC     0x00010
#define MSG_CTRUNC    0x00020
#define MSG_WAITALL   0x00040
#define MSG_DONTWAIT  0x00080
#define MSG_NOSIGNAL  0x20000

#define SOMAXCONN   128

struct sockaddr {
    uint8_t     sa_len;
    sa_family_t sa_family;
    char        sa_data[14];
};

struct sockaddr_storage {
    uint8_t     ss_len;
    sa_family_t ss_family;
    char        __ss_pad1[6];
    int64_t     __ss_align;
    char        __ss_pad2[112];
};

struct linger {
    int l_onoff;
    int l_linger;
};

int     socket(int domain, int type, int protocol);
int     connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     listen(int sockfd, int backlog);
int     accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
int     getsockopt(int sockfd, int level, int optname, void *optval,
                   socklen_t *optlen);
int     setsockopt(int sockfd, int level, int optname, const void *optval,
                   socklen_t optlen);
int     getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     shutdown(int sockfd, int how);
int     closesocket(int sockfd);

#ifdef __cplusplus
}
#endif

#endif /* NN_SYS_SOCKET_H */
