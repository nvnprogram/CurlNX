/*
 * curl_config.h - libcurl build configuration for the Nintendo Switch target.
 *
 * HTTP/HTTPS only, TLS via the nn::ssl backend (USE_NNSSL), name resolution via
 * the synchronous IPv4 gethostbyname() path, socket I/O via the nnsocket* C
 * wrappers.
 */
#ifndef CURL_CONFIG_NX_H
#define CURL_CONFIG_NX_H

#define OS "aarch64-nintendo-switch-nvnmaxxing"
#define PACKAGE "curl"
#define PACKAGE_NAME "curl"
#define PACKAGE_VERSION "7.64.1"
#define VERSION "7.64.1"

/* ---- SSL/TLS backend ---- */
#define USE_NNSSL 1

/* ---- type sizes (aarch64 LP64) ---- */
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_TIME_T 8
#define SIZEOF_OFF_T 8
#define SIZEOF_CURL_OFF_T 8
#define CURL_SIZEOF_LONG 8
#define HAVE_LONGLONG 1

/* ---- available headers ---- */
#define HAVE_ASSERT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_NETDB_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_POLL_H 1

/* ---- available types ---- */
#define HAVE_BOOL_T 1
#define HAVE_SOCKLEN_T 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_SA_FAMILY_T 1
#define HAVE_IN_ADDR_T 1

/* ---- available functions ---- */
#define HAVE_SOCKET 1
#define HAVE_RECV 1
#define HAVE_SEND 1
#define HAVE_POLL 1
#define HAVE_POLL_FINE 1
#define HAVE_STRUCT_POLLFD 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_CLOSESOCKET 1
#define HAVE_FCNTL 1
#define HAVE_FCNTL_O_NONBLOCK 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GMTIME_R 1
#define HAVE_STRDUP 1
#define HAVE_STRTOK_R 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_VARIADIC_MACROS_C99 1
#define HAVE_VARIADIC_MACROS_GCC 1

/* ---- recv()/send() prototypes (match lib/nnsocket.c) ---- */
#define RECV_TYPE_ARG1 int
#define RECV_TYPE_ARG2 void *
#define RECV_TYPE_ARG3 size_t
#define RECV_TYPE_ARG4 int
#define RECV_TYPE_RETV ssize_t

#define SEND_TYPE_ARG1 int
#define SEND_QUAL_ARG2 const
#define SEND_TYPE_ARG2 void *
#define SEND_TYPE_ARG3 size_t
#define SEND_TYPE_ARG4 int
#define SEND_TYPE_RETV ssize_t

/* ---- disable protocols we don't ship (HTTP/HTTPS only) ---- */
#define CURL_DISABLE_DICT 1
#define CURL_DISABLE_FILE 1
#define CURL_DISABLE_FTP 1
#define CURL_DISABLE_GOPHER 1
#define CURL_DISABLE_IMAP 1
#define CURL_DISABLE_LDAP 1
#define CURL_DISABLE_LDAPS 1
#define CURL_DISABLE_POP3 1
#define CURL_DISABLE_RTSP 1
#define CURL_DISABLE_SMB 1
#define CURL_DISABLE_SMTP 1
#define CURL_DISABLE_TELNET 1
#define CURL_DISABLE_TFTP 1

/* ---- misc ---- */
#define CURL_EXTERN_SYMBOL
#ifndef CURL_DISABLE_PROXY
/* proxy support left enabled (compiles, but unused on this target) */
#endif

#endif /* CURL_CONFIG_NX_H */
