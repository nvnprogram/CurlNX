#ifndef NN_ARPA_INET_H
#define NN_ARPA_INET_H

#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int         inet_pton(int af, const char *src, void *dst);
in_addr_t   inet_addr(const char *cp);

#ifdef __cplusplus
}
#endif

#endif /* NN_ARPA_INET_H */
