#ifndef NN_NETINET_IN_H
#define NN_NETINET_IN_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define IPPROTO_RAW     255

#define INADDR_ANY        ((in_addr_t)0x00000000)
#define INADDR_LOOPBACK   ((in_addr_t)0x7f000001)
#define INADDR_BROADCAST  ((in_addr_t)0xffffffff)
#define INADDR_NONE       ((in_addr_t)0xffffffff)

#define INET_ADDRSTRLEN   16
#define INET6_ADDRSTRLEN  46

#define IP_TOS  3
#define IP_TTL  4

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    uint8_t        sin_len;
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct in6_addr {
    union {
        uint8_t  __u6_addr8[16];
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
    } __u6_addr;
};
#define s6_addr __u6_addr.__u6_addr8

struct sockaddr_in6 {
    uint8_t         sin6_len;
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

extern const struct in6_addr in6addr_any;

#ifdef __cplusplus
}
#endif

#endif /* NN_NETINET_IN_H */
