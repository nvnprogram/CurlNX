/*
 * nnsocket.c - BSD socket shim.
 */
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

/* ---- nn::socket C wrappers (exported by nnSdk) ------------------------------ */
extern int   nnsocketSocket(int domain, int type, int protocol);
extern int   nnsocketConnect(int fd, const struct sockaddr *addr, uint32_t len);
extern int   nnsocketBind(int fd, const struct sockaddr *addr, uint32_t len);
extern int   nnsocketListen(int fd, int backlog);
extern int   nnsocketAccept(int fd, struct sockaddr *addr, uint32_t *len);
extern long  nnsocketRecv(int fd, void *buf, size_t len, int flags);
extern long  nnsocketSend(int fd, const void *buf, size_t len, int flags);
extern long  nnsocketRecvFrom(int fd, void *buf, size_t len, int flags,
                              struct sockaddr *src, uint32_t *srclen);
extern long  nnsocketSendTo(int fd, const void *buf, size_t len, int flags,
                            const struct sockaddr *dst, uint32_t dstlen);
extern int   nnsocketClose(int fd);
extern int   nnsocketShutdown(int fd, int how);
extern int   nnsocketSetSockOpt(int fd, int level, int opt, const void *val, uint32_t len);
extern int   nnsocketGetSockOpt(int fd, int level, int opt, void *val, uint32_t *len);
extern int   nnsocketGetSockName(int fd, struct sockaddr *addr, uint32_t *len);
extern int   nnsocketGetPeerName(int fd, struct sockaddr *addr, uint32_t *len);
extern int   nnsocketFcntl(int fd, int cmd, ...);
extern int   nnsocketPoll(struct pollfd *fds, unsigned long nfds, int timeout);
extern int   nnsocketGetLastErrno(void);
extern void  nnsocketSetLastErrno(int err);
extern int   nnsocketGetHErrno(void);
extern struct hostent *nnsocketGetHostByName(const char *name);
extern uint16_t nnsocketInetHtons(uint16_t v);
extern uint32_t nnsocketInetHtonl(uint32_t v);
extern uint16_t nnsocketInetNtohs(uint16_t v);
extern uint32_t nnsocketInetNtohl(uint32_t v);
extern const char *nnsocketInetNtop(int af, const void *src, char *dst, uint32_t size);
extern int   nnsocketInetPton(int af, const char *src, void *dst);

#define NN_O_NONBLOCK 0x0800

int h_errno = 0;
const struct in6_addr in6addr_any = { { { 0 } } };

/* Map nn::socket errno to this toolchain's <errno.h> values. */
static int nnsk_xlate_errno(int e)
{
    switch(e) {
    case 0:   return 0;
    case 1:   return EPERM;
    case 2:   return ENOENT;
    case 4:   return EINTR;
    case 5:   return EIO;
    case 9:   return EBADF;
    case 11:  return EAGAIN;
    case 12:  return ENOMEM;
    case 13:  return EACCES;
    case 14:  return EFAULT;
    case 22:  return EINVAL;
    case 23:  return ENFILE;
    case 24:  return EMFILE;
    case 32:  return EPIPE;
    case 35:  return EDEADLK;
    case 88:  return ENOTSOCK;
    case 89:  return EDESTADDRREQ;
    case 90:  return EMSGSIZE;
    case 91:  return EPROTOTYPE;
    case 92:  return ENOPROTOOPT;
    case 93:  return EPROTONOSUPPORT;
    case 95:  return EOPNOTSUPP;
    case 97:  return EAFNOSUPPORT;
    case 98:  return EADDRINUSE;
    case 99:  return EADDRNOTAVAIL;
    case 100: return ENETDOWN;
    case 101: return ENETUNREACH;
    case 102: return ENETRESET;
    case 103: return ECONNABORTED;
    case 104: return ECONNRESET;
    case 105: return ENOBUFS;
    case 106: return EISCONN;
    case 107: return ENOTCONN;
    case 110: return ETIMEDOUT;
    case 111: return ECONNREFUSED;
    case 113: return EHOSTUNREACH;
    case 114: return EALREADY;
    case 115: return EINPROGRESS;
    default:  return e ? EIO : 0;
    }
}

static void nnsk_capture_errno(void)
{
    errno = nnsk_xlate_errno(nnsocketGetLastErrno());
}

#define NNSK_FAIL_INT(expr) do { int _r = (expr); if(_r < 0) nnsk_capture_errno(); return _r; } while(0)

int socket(int domain, int type, int protocol)
{
    NNSK_FAIL_INT(nnsocketSocket(domain, type, protocol));
}

int connect(int fd, const struct sockaddr *addr, socklen_t len)
{
    NNSK_FAIL_INT(nnsocketConnect(fd, addr, (uint32_t)len));
}

int bind(int fd, const struct sockaddr *addr, socklen_t len)
{
    NNSK_FAIL_INT(nnsocketBind(fd, addr, (uint32_t)len));
}

int listen(int fd, int backlog)
{
    NNSK_FAIL_INT(nnsocketListen(fd, backlog));
}

int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    NNSK_FAIL_INT(nnsocketAccept(fd, addr, (uint32_t *)len));
}

ssize_t recv(int fd, void *buf, size_t len, int flags)
{
    long r = nnsocketRecv(fd, buf, len, flags);
    if(r < 0) nnsk_capture_errno();
    return (ssize_t)r;
}

ssize_t send(int fd, const void *buf, size_t len, int flags)
{
    long r = nnsocketSend(fd, buf, len, flags);
    if(r < 0) nnsk_capture_errno();
    return (ssize_t)r;
}

ssize_t recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *srclen)
{
    long r = nnsocketRecvFrom(fd, buf, len, flags, src, (uint32_t *)srclen);
    if(r < 0) nnsk_capture_errno();
    return (ssize_t)r;
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dst, socklen_t dstlen)
{
    long r = nnsocketSendTo(fd, buf, len, flags, dst, (uint32_t)dstlen);
    if(r < 0) nnsk_capture_errno();
    return (ssize_t)r;
}

int closesocket(int fd)
{
    NNSK_FAIL_INT(nnsocketClose(fd));
}

int shutdown(int fd, int how)
{
    NNSK_FAIL_INT(nnsocketShutdown(fd, how));
}

int setsockopt(int fd, int level, int opt, const void *val, socklen_t len)
{
    NNSK_FAIL_INT(nnsocketSetSockOpt(fd, level, opt, val, (uint32_t)len));
}

int getsockopt(int fd, int level, int opt, void *val, socklen_t *len)
{
    NNSK_FAIL_INT(nnsocketGetSockOpt(fd, level, opt, val, (uint32_t *)len));
}

int getsockname(int fd, struct sockaddr *addr, socklen_t *len)
{
    NNSK_FAIL_INT(nnsocketGetSockName(fd, addr, (uint32_t *)len));
}

int getpeername(int fd, struct sockaddr *addr, socklen_t *len)
{
    NNSK_FAIL_INT(nnsocketGetPeerName(fd, addr, (uint32_t *)len));
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int r = nnsocketPoll(fds, (unsigned long)nfds, timeout);
    if(r < 0) nnsk_capture_errno();
    return r;
}

/*
 * Socket fcntl shim. curl uses this (via the curl `sfcntl` macro) only to toggle
 * O_NONBLOCK on a socket. We translate between the toolchain's O_NONBLOCK bit and
 * the nn nonblock bit so the stock curl logic works unchanged.
 */
int nnsk_fcntl(int fd, int cmd, int arg)
{
    if(cmd == F_GETFL) {
        int nnf = nnsocketFcntl(fd, F_GETFL);
        int out;
        if(nnf < 0) { nnsk_capture_errno(); return -1; }
        out = nnf & ~NN_O_NONBLOCK;
        if(nnf & NN_O_NONBLOCK)
            out |= O_NONBLOCK;
        return out;
    }
    if(cmd == F_SETFL) {
        int nnf = arg & ~O_NONBLOCK;
        int r;
        if(arg & O_NONBLOCK)
            nnf |= NN_O_NONBLOCK;
        r = nnsocketFcntl(fd, F_SETFL, nnf);
        if(r < 0) nnsk_capture_errno();
        return r;
    }
    return nnsocketFcntl(fd, cmd, arg);
}

uint32_t htonl(uint32_t v) { return nnsocketInetHtonl(v); }
uint16_t htons(uint16_t v) { return nnsocketInetHtons(v); }
uint32_t ntohl(uint32_t v) { return nnsocketInetNtohl(v); }
uint16_t ntohs(uint16_t v) { return nnsocketInetNtohs(v); }

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    const char *r = nnsocketInetNtop(af, src, dst, (uint32_t)size);
    if(!r) nnsk_capture_errno();
    return r;
}

int inet_pton(int af, const char *src, void *dst)
{
    return nnsocketInetPton(af, src, dst);
}

in_addr_t inet_addr(const char *cp)
{
    struct in_addr a;
    if(nnsocketInetPton(AF_INET, cp, &a) == 1)
        return a.s_addr;
    return INADDR_NONE;
}

struct hostent *gethostbyname(const char *name)
{
    struct hostent *he = nnsocketGetHostByName(name);
    if(!he) {
        h_errno = nnsocketGetHErrno();
        nnsk_capture_errno();
    }
    return he;
}
