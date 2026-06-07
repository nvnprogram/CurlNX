/***************************************************************************
 *                     libcurl nnssl (Nintendo nn::ssl) backend
 *
 * TLS backend implemented on top of the nn::ssl service via its C
 * "nnssl*" wrappers (exported by nnSdk). The application supplies the
 * nn::ssl::Context through CURLOPT_SSL_CTX_FUNCTION, this backend creates the
 * nn::ssl::Connection, drives a non-blocking handshake, and routes recv/send
 * through the connection.
 ***************************************************************************/
#include "curl_setup.h"

#ifdef USE_NNSSL

#include <stdint.h>
#include <string.h>

#include "urldata.h"
#include "vtls.h"
#include "connect.h"
#include "select.h"
#include "sendf.h"
#include "curl_printf.h"
#include "curl_memory.h"
#include "memdebug.h"

/* ---- nn::ssl C wrappers (exported by nnSdk) ---------------------- */
typedef uint32_t nnResult;

extern nnResult nnsslInitialize(void);
extern nnResult nnsslFinalize(void);

extern nnResult nnsslContextGetContextId(void *ctx, uint64_t *pOutId);
extern nnResult nnsslContextDestroy(void *ctx);

extern nnResult nnsslConnectionCreate(void *conn, void *ctx);
extern nnResult nnsslConnectionDestroy(void *conn);
extern nnResult nnsslConnectionGetConnectionId(void *conn, uint64_t *pOutId);
extern nnResult nnsslConnectionSetSocketDescriptor(void *conn, int fd);
extern nnResult nnsslConnectionSetHostName(void *conn, const char *name, uint32_t len);
extern nnResult nnsslConnectionSetOption(void *conn, int optionType, int enable);
extern nnResult nnsslConnectionSetVerifyOption(void *conn, uint32_t verifyOption);
extern nnResult nnsslConnectionSetSessionCacheMode(void *conn, int mode);
extern nnResult nnsslConnectionSetIoMode(void *conn, int mode);
extern nnResult nnsslConnectionDoHandshakeWithCertBuffer(void *conn,
                                                         uint32_t *pCertDataSize,
                                                         uint32_t *pCertCount);
extern nnResult nnsslConnectionRead(void *conn, void *buf, int *pOutRead, uint32_t len);
extern nnResult nnsslConnectionWrite(void *conn, const void *buf, int *pOutWritten, uint32_t len);
extern nnResult nnsslConnectionPending(void *conn, int *pOut);
extern nnResult nnsslConnectionPeek(void *conn, void *buf, int *pOut, uint32_t len);
extern nnResult nnsslConnectionGetVerifyCertError(void *conn, uint32_t *pOut);

extern int nnsocketFcntl(int fd, int cmd, ...);

#define NNSSL_OPT_DONOTCLOSESOCKET  0
#define NNSSL_OPT_SKIPDEFAULTVERIFY 2

#define NNSSL_IOMODE_NONBLOCKING    2

#define NNSSL_VERIFY_PEERCA   0x01
#define NNSSL_VERIFY_HOSTNAME 0x02
#define NNSSL_VERIFY_DATE     0x04

#define NNSSL_RESULT_WOULDBLOCK     104571  /* WANT_READ/WRITE - retry */
#define NNSSL_RESULT_ALREADY_INIT   103035  /* already initialized */
#define NNSSL_RESULT_VERIFY_FAILED  106107  /* peer verification failed */

static int nnr_success(nnResult r) { return r == 0; }
static int nnr_fail(nnResult r)    { return r != 0; }
static int nnr_module(nnResult r)  { return (int)(r & 0x1FF); }
static int nnr_desc(nnResult r)    { return (int)((r >> 9) & 0x1FFF); }
static int nnr_equal(nnResult a, nnResult b)
{
  return nnr_module(a) == nnr_module(b) && nnr_desc(a) == nnr_desc(b);
}

static int nnssl_verify_error_is_cert_problem(nnResult verr)
{
  static const nnResult cert_problems[] = { 165499, 772731, 774267, 773755 };
  size_t i;
  for(i = 0; i < sizeof(cert_problems) / sizeof(cert_problems[0]); i++)
    if(nnr_equal(verr, cert_problems[i]))
      return 1;
  return 0;
}

struct ssl_backend_data {
  unsigned char connection[128];           /* nn::ssl::Connection */
  unsigned char context[128];              /* nn::ssl::Context (embedded) */
  void         *pnnssl_context;            /* active context (embedded or external) */
  bool          using_external_ssl_context;
};

static ssize_t nnssl_recv(struct connectdata *conn, int sockindex,
                          char *buf, size_t buffersize, CURLcode *curlcode);
static ssize_t nnssl_send(struct connectdata *conn, int sockindex,
                          const void *buf, size_t buffersize, CURLcode *curlcode);

static int Curl_nnssl_init(void)
{
  nnResult r = nnsslInitialize();
  if(nnr_fail(r) && !nnr_equal(r, NNSSL_RESULT_ALREADY_INIT))
    return 0;
  return 1;
}

static void Curl_nnssl_cleanup(void)
{
  /* The nn::ssl service stays initialized for the process lifetime */
}

static size_t Curl_nnssl_version(char *buffer, size_t size)
{
  return msnprintf(buffer, size, "nn::ssl");
}

static void nnssl_destroy_ssl_connection(struct ssl_connect_data *connssl)
{
  struct ssl_backend_data *backend = connssl->backend;
  uint64_t id = 0;
  if(!backend)
    return;
  if(nnr_success(nnsslConnectionGetConnectionId(backend->connection, &id)) && id)
    (void)nnsslConnectionDestroy(backend->connection);
}

static void nnssl_destroy_ssl_context(struct ssl_connect_data *connssl)
{
  struct ssl_backend_data *backend = connssl->backend;
  uint64_t id = 0;
  if(!backend || !backend->pnnssl_context)
    return;
  if(nnr_success(nnsslContextGetContextId(backend->pnnssl_context, &id)) && id)
    (void)nnsslContextDestroy(backend->pnnssl_context);
}

static void Curl_nnssl_close(struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  if(!backend)
    return;
  nnssl_destroy_ssl_connection(connssl);
  if(!backend->using_external_ssl_context)
    nnssl_destroy_ssl_context(connssl);
}

static CURLcode nnssl_connect_common(struct connectdata *conn, int sockindex,
                                     bool *done)
{
  struct Curl_easy *data = conn->data;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  curl_socket_t sockfd = conn->sock[sockindex];
  nnResult r;
  uint64_t id;

  if(!backend)
    return CURLE_SSL_INVALID_REFERENCE;

  if(connssl->state == ssl_connection_complete) {
    *done = TRUE;
    return CURLE_OK;
  }

  if(connssl->connecting_state == ssl_connect_1) {
    void *psslctx = SSL_SET_OPTION(psslctx);

    if(psslctx) {
      /* application supplied a pre-created nn::ssl::Context */
      backend->pnnssl_context = psslctx;
      backend->using_external_ssl_context = true;
    }
    else {
      curl_ssl_ctx_callback cb = SSL_SET_OPTION(fsslctx);
      backend->using_external_ssl_context = false;
      backend->pnnssl_context = backend->context;

      /* tear down any context left over from a previous handshake on this
         backend before the callback creates a fresh one */
      id = 0;
      r = nnsslContextGetContextId(backend->pnnssl_context, &id);
      if(nnr_fail(r))
        return CURLE_SSL_CTX_FATAL;
      if(id) {
        r = nnsslContextDestroy(backend->pnnssl_context);
        if(nnr_fail(r))
          return CURLE_SSL_CTX_FATAL;
      }

      if(!cb)
        return CURLE_SSL_CTX_FUNCTION_NOT_SET;
      /* The callback receives the nn::ssl::Context and must Create() it. */
      if(cb(data, backend->context, SSL_SET_OPTION(fsslctxp)))
        return CURLE_ABORTED_BY_CALLBACK;
    }

    /* validate the active context (external or callback-created) has a real id */
    id = 0;
    r = nnsslContextGetContextId(backend->pnnssl_context, &id);
    if(nnr_fail(r))
      return CURLE_SSL_CTX_FATAL;
    if(!id)
      return CURLE_SSL_CTX_INVALID;

    r = nnsslConnectionCreate(backend->connection, backend->pnnssl_context);
    if(nnr_fail(r))
      goto connect_error;

    /* nn::ssl manages its own non-blocking IO; hand it curl's socket but tell
       it not to close it, and clear the descriptor flags */
    (void)nnsocketFcntl((int)sockfd, 4 /* F_SETFL */, 0);

    r = nnsslConnectionSetOption(backend->connection, NNSSL_OPT_DONOTCLOSESOCKET, 1);
    if(nnr_fail(r))
      goto connect_error;
    r = nnsslConnectionSetSocketDescriptor(backend->connection, (int)sockfd);
    if(nnr_fail(r))
      goto connect_error;
    r = nnsslConnectionSetHostName(backend->connection, conn->host.dispname,
                                   (uint32_t)strlen(conn->host.dispname));
    if(nnr_fail(r))
      goto connect_error;
    r = nnsslConnectionSetOption(backend->connection, NNSSL_OPT_SKIPDEFAULTVERIFY,
                                 SSL_CONN_CONFIG(skipssldefaultverify) ? 1 : 0);
    if(nnr_fail(r))
      goto connect_error;

    {
      uint32_t vopt = 0;
      if(SSL_CONN_CONFIG(verifypeer))
        vopt |= NNSSL_VERIFY_PEERCA;
      if(SSL_CONN_CONFIG(verifyhost))
        vopt |= NNSSL_VERIFY_HOSTNAME;
      if(SSL_CONN_CONFIG(verifydate))
        vopt |= NNSSL_VERIFY_DATE;
      r = nnsslConnectionSetVerifyOption(backend->connection, vopt);
      if(nnr_fail(r))
        goto connect_error;
    }

    r = nnsslConnectionSetSessionCacheMode(backend->connection,
                                           SSL_CONN_CONFIG(sessionid) ? 1 : 0);
    if(nnr_fail(r))
      goto connect_error;

    {
      curl_ssl_conn_callback_before before =
        (curl_ssl_conn_callback_before)SSL_SET_OPTION(fsslconnbefore);
      if(before && before(data, backend->connection, SSL_SET_OPTION(psslcondata)))
        return CURLE_ABORTED_BY_CALLBACK;
    }

    r = nnsslConnectionSetIoMode(backend->connection, NNSSL_IOMODE_NONBLOCKING);
    if(nnr_fail(r))
      goto connect_error;

    connssl->state = ssl_connection_negotiating;
    connssl->connecting_state = ssl_connect_2;
  }

  /* ssl_connect_2: drive the (non-blocking) handshake */
  if(Curl_timeleft(data, NULL, TRUE) < 0)
    return CURLE_OPERATION_TIMEDOUT;

  data->set.ssl.dohandshakeresult = 0;
  data->set.ssl.certverifyresult = 0;
  {
    uint32_t cert_data_size = 0;
    uint32_t cert_count = 0;
    curl_ssl_conn_callback_after after;
    bool handshake_ok;
    CURLcode result;

    r = nnsslConnectionDoHandshakeWithCertBuffer(backend->connection,
                                                 &cert_data_size, &cert_count);
    if(nnr_fail(r) && nnr_equal(r, NNSSL_RESULT_WOULDBLOCK)) {
      *done = FALSE;            /* EAGAIN: call again when the socket is ready */
      return CURLE_OK;
    }

    handshake_ok = nnr_success(r);
    if(handshake_ok) {
      result = CURLE_OK;
    }
    else {
      data->set.ssl.dohandshakeresult = (long)r;
      result = nnr_equal(r, NNSSL_RESULT_VERIFY_FAILED)
                 ? CURLE_PEER_FAILED_VERIFICATION : CURLE_SSL_CONNECT_ERROR;
    }

    /* after-DoHandshake() application callback: invoked on success and failure
       and may override the outcome by returning non-zero */
    after = (curl_ssl_conn_callback_after)SSL_SET_OPTION(fsslconnafter);
    if(after) {
      curl_handshake_info info;
      info.isHandshakeSuccess = handshake_ok ? 1 : 0;
      info.certDataSize = cert_data_size;
      info.certCount = cert_count;
      /* 24-byte struct: the AArch64 ABI passes it indirectly */
      if(after(data, backend->connection, SSL_SET_OPTION(psslcondata), info))
        result = CURLE_ABORTED_BY_CALLBACK;
    }

    if(result == CURLE_OK) {
      /* handshake complete - route IO through the connection */
      conn->recv[sockindex] = nnssl_recv;
      conn->send[sockindex] = nnssl_send;
      connssl->connecting_state = ssl_connect_1;
      connssl->state = ssl_connection_complete;
      *done = TRUE;
      return CURLE_OK;
    }

    /* refine a verification failure into a cert problem where appropriate */
    if(result == CURLE_PEER_FAILED_VERIFICATION) {
      uint32_t verr = 0;
      if(nnr_success(nnsslConnectionGetVerifyCertError(backend->connection,
                                                       &verr))) {
        data->set.ssl.certverifyresult = (long)verr;
        if(nnssl_verify_error_is_cert_problem(verr))
          result = CURLE_SSL_CERTPROBLEM;
      }
    }

    nnssl_destroy_ssl_connection(connssl);
    if(!backend->using_external_ssl_context)
      nnssl_destroy_ssl_context(connssl);
    return result;
  }

connect_error:
  nnssl_destroy_ssl_connection(connssl);
  if(!backend->using_external_ssl_context)
    nnssl_destroy_ssl_context(connssl);
  return CURLE_SSL_CONNECT_ERROR;
}

static CURLcode Curl_nnssl_connect_nonblocking(struct connectdata *conn,
                                               int sockindex, bool *done)
{
  return nnssl_connect_common(conn, sockindex, done);
}

static CURLcode Curl_nnssl_connect(struct connectdata *conn, int sockindex)
{
  CURLcode result;
  bool done = FALSE;

  while(!done) {
    result = nnssl_connect_common(conn, sockindex, &done);
    if(result)
      return result;
    if(!done) {
      curl_socket_t sockfd = conn->sock[sockindex];
      timediff_t timeout_ms = Curl_timeleft(conn->data, NULL, TRUE);
      if(timeout_ms < 0)
        return CURLE_OPERATION_TIMEDOUT;
      /* wait for the socket to become ready, then retry the handshake */
      if(Curl_socket_check(sockfd, CURL_SOCKET_BAD, sockfd,
                           timeout_ms > 1000 ? 1000 : timeout_ms) < 0)
        return CURLE_SSL_CONNECT_ERROR;
    }
  }
  return CURLE_OK;
}

static ssize_t nnssl_recv(struct connectdata *conn, int sockindex,
                          char *buf, size_t buffersize, CURLcode *curlcode)
{
  struct ssl_backend_data *backend = conn->ssl[sockindex].backend;
  int nread = 0;
  nnResult r;

  if(!backend || !buf) {
    *curlcode = CURLE_RECV_ERROR;
    return -1;
  }
  r = nnsslConnectionRead(backend->connection, buf, &nread, (uint32_t)buffersize);
  if(nnr_success(r))
    return (ssize_t)nread;

  *curlcode = nnr_equal(r, NNSSL_RESULT_WOULDBLOCK) ? CURLE_AGAIN : CURLE_RECV_ERROR;
  return -1;
}

static ssize_t nnssl_send(struct connectdata *conn, int sockindex,
                          const void *buf, size_t buffersize, CURLcode *curlcode)
{
  struct ssl_backend_data *backend = conn->ssl[sockindex].backend;
  int nwritten = 0;
  nnResult r;

  if(!backend || !buf) {
    *curlcode = CURLE_SEND_ERROR;
    return -1;
  }
  r = nnsslConnectionWrite(backend->connection, buf, &nwritten, (uint32_t)buffersize);
  if(nnr_success(r))
    return (ssize_t)nwritten;
  
  *curlcode = nnr_equal(r, NNSSL_RESULT_WOULDBLOCK) ? CURLE_AGAIN : CURLE_WRITE_ERROR;
  return -1;
}

static bool Curl_nnssl_data_pending(const struct connectdata *conn, int sockindex)
{
  struct ssl_backend_data *backend = conn->ssl[sockindex].backend;
  int pending = 0;
  if(!backend)
    return FALSE;
  if(nnr_success(nnsslConnectionPending(backend->connection, &pending)) && pending > 0)
    return TRUE;
  return FALSE;
}

static int Curl_nnssl_check_cxn(struct connectdata *conn)
{
  struct ssl_backend_data *backend = conn->ssl[FIRSTSOCKET].backend;
  char buf[4];
  int got = 0;
  nnResult r;

  if(!backend)
    return -1;
  r = nnsslConnectionPeek(backend->connection, buf, &got, 1);
  if(nnr_fail(r))
    return nnr_equal(r, NNSSL_RESULT_WOULDBLOCK) ? 1 : -1;
  return got > 0 ? 1 : 0;
}

static void *Curl_nnssl_get_internals(struct ssl_connect_data *connssl,
                                      CURLINFO info)
{
  struct ssl_backend_data *backend = connssl->backend;
  (void)info;
  if(!backend)
    return NULL;
  return backend->pnnssl_context;
}

const struct Curl_ssl Curl_ssl_nnssl = {
  { CURLSSLBACKEND_NNSSL, "nnssl" },  /* info */
  SSLSUPP_SSL_CTX,                    /* supports CURLOPT_SSL_CTX_FUNCTION */
  sizeof(struct ssl_backend_data),

  Curl_nnssl_init,                    /* init */
  Curl_nnssl_cleanup,                 /* cleanup */
  Curl_nnssl_version,                 /* version */
  Curl_nnssl_check_cxn,               /* check_cxn */
  Curl_none_shutdown,                 /* shut_down */
  Curl_nnssl_data_pending,            /* data_pending */
  Curl_none_random,                   /* random */
  Curl_none_cert_status_request,      /* cert_status_request */
  Curl_nnssl_connect,                 /* connect_blocking */
  Curl_nnssl_connect_nonblocking,     /* connect_nonblocking */
  Curl_nnssl_get_internals,           /* get_internals */
  Curl_nnssl_close,                   /* close_one */
  Curl_none_close_all,                /* close_all */
  Curl_none_session_free,             /* session_free */
  Curl_none_set_engine,               /* set_engine */
  Curl_none_set_engine_default,       /* set_engine_default */
  Curl_none_engines_list,             /* engines_list */
  Curl_none_false_start,              /* false_start */
  Curl_none_md5sum,                   /* md5sum */
  NULL                                /* sha256sum */
};

#endif /* USE_NNSSL */
