/* chaosircd - Chaoz's IRC daemon daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: ssl.c,v 1.4 2006/09/28 08:38:31 roman Exp $
 */

#include "libchaos/defs.h"

/* ------------------------------------------------------------------------ *
 * Library headers                                                          *
 * ------------------------------------------------------------------------ */
#include "config.h"
#include "libchaos/io.h"
#include "libchaos/ssl.h"
#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/syscall.h"

/* ------------------------------------------------------------------------ *
 * System headers                                                           *
 * ------------------------------------------------------------------------ */
#include "config.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#endif

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int          ssl_log;
struct sheap ssl_heap;
struct list  ssl_list;
uint32_t     ssl_id;

/* ------------------------------------------------------------------------ */
int ssl_get_log() { return ssl_log; }

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#ifdef HAVE_SSL
static int ssl_bio_read(BIO *h, char *buf, int size)
{
  int ret = 0;
  int fd;

  if(buf != NULL)
  {
    errno = 0;
    BIO_get_fd(h, &fd);
    ret = recv(fd, buf, size, 0);
    BIO_clear_retry_flags(h);

    if(ret <= 0)
    {
      if(BIO_fd_should_retry(ret))
        BIO_set_retry_read(h);
    }
  }

  return ret;
}
#endif
/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#ifdef HAVE_SSL
static int ssl_bio_write(BIO *h, const char *buf, int size)
{
  int ret;
  int fd;

  errno = 0;
  BIO_get_fd(h, &fd);
  ret = send(fd, buf, size, 0);
  BIO_clear_retry_flags(h);

  if(ret <= 0)
  {
    if(BIO_fd_should_retry(ret))
      BIO_set_retry_write(h);
  }

  return ret;
}
#endif

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
#ifdef HAVE_SSL
static void ssl_set_sock(SSL *ssl, int fd)
{
  BIO *rbio;
  BIO *wbio;

  SSL_set_fd(ssl, fd);

  /*rbio = SSL_get_rbio(ssl);
  wbio = SSL_get_wbio(ssl);

  BIO_meth_set_read(rbio, &ssl_bio_read);
  BIO_meth_set_write(wbio, &ssl_bio_write);*/
  //rbio->method->bwrite = (void *)&ssl_bio_write;
}
#endif

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_init(void)
{
  ssl_log = log_source_register("ssl");

  ssl_id = 0;

#ifdef HAVE_SSL
  mem_static_create(&ssl_heap, sizeof(struct ssl_context), SSL_BLOCK_SIZE);
  mem_static_note(&ssl_heap, "ssl context heap");

#if OPENSSL_API_COMPAT >= 0x10100000L
  const OPENSSL_INIT_SETTINGS* settings = OPENSSL_INIT_new();
  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS,
                   settings);
#else
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
#endif
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_shutdown(void)
{
  struct ssl_context *scptr;
  struct node        *next;

  dlink_foreach_safe(&ssl_list, scptr, next)
    ssl_delete(scptr);

  mem_static_destroy(&ssl_heap);

  log_source_unregister(ssl_log);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_seed(void)
{
#ifdef HAVE_SSL
  int readbytes;

  readbytes = RAND_load_file(SSL_RANDOM_FILE, SSL_RANDOM_SIZE);

  if(readbytes <= 0)
  {
    log(ssl_log, L_warning,
        "Unable to retrieve any random data from "SSL_RANDOM_FILE);
  }
  else
  {
    log(ssl_log, L_status, "Read %d random bytes from "SSL_RANDOM_FILE,
        readbytes);
  }
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_default(struct ssl_context *scptr)
{
  scptr->name[0] = '\0';
  scptr->context = SSL_CONTEXT_SERVER;
  scptr->cert[0] = '\0';
  scptr->key[0] = '\0';
  strcpy(scptr->ciphers, "RSA+HIGH:RSA+MEDIUM");
}

int
ssl_verify(int ok, X509_STORE_CTX* cert) {
 
  if(!ok)
    return 0; // stop immediately
  return 1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ssl_context *ssl_add(const char *name, int         context,
                            const char *cert, const char *key,
                            const char *ciphers)
{
#ifdef HAVE_SSL
  struct ssl_context *scptr;
  SSL_CTX            *ctxt;
  const SSL_METHOD   *meth;

  /* Setup SSL method */
  meth = NULL;

  if(context == SSL_CONTEXT_SERVER)
  {
    meth = TLS_server_method();
  }
  if(context == SSL_CONTEXT_CLIENT)
  {
    meth = TLS_client_method();
  }

  if(meth == NULL)
  {
    log(ssl_log, L_warning, "Error getting %s method.",
        context ? "client" : "server");
    return NULL;
  }

  /* Setup SSL context */
  ctxt = SSL_CTX_new(meth);

  if(ctxt == NULL)
  {
    log(ssl_log, L_warning, "Error getting %s context.",
        context ? "client" : "server");
    SSL_CTX_free(ctxt);
    return NULL;
  }

  SSL_CTX_set_mode(ctxt,
                   SSL_MODE_ENABLE_PARTIAL_WRITE |
                   SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

  SSL_CTX_set_session_cache_mode(ctxt, SSL_SESS_CACHE_BOTH);
  SSL_CTX_set_timeout(ctxt, 360);

  if(!SSL_CTX_use_RSAPrivateKey_file(ctxt, key, SSL_FILETYPE_PEM))
  {
    log(ssl_log, L_warning, "Error loading RSA private key %s", key);
    SSL_CTX_free(ctxt);
    return NULL;
  }

  if(!SSL_CTX_use_certificate_file(ctxt, cert, SSL_FILETYPE_PEM))
  {
    log(ssl_log, L_warning, "Error loading x509 certificate %s", cert);
    SSL_CTX_free(ctxt);
    return NULL;
  }

  if(!SSL_CTX_check_private_key(ctxt))
  {
    log(ssl_log, L_warning, "Certificate and private key do not match");
    SSL_CTX_free(ctxt);
    return NULL;
  }

  if(!SSL_CTX_set_cipher_list(ctxt, ciphers))
  {
    log(ssl_log, L_warning, "Error setting cipher list %s.", ciphers);
    SSL_CTX_free(ctxt);
    return NULL;
  }
  SSL_CTX_set_verify(ctxt, SSL_VERIFY_NONE, ssl_verify);

  scptr = mem_static_alloc(&ssl_heap);

  strlcpy(scptr->name, name, sizeof(scptr->name));
  scptr->hash = str_ihash(scptr->name);
  scptr->context = context;
  scptr->id = ssl_id++;
  scptr->refcount = 1;
  strlcpy(scptr->cert, cert, sizeof(scptr->cert));
  strlcpy(scptr->key, key, sizeof(scptr->key));
  strlcpy(scptr->ciphers, ciphers, sizeof(scptr->ciphers));

  scptr->ctxt = ctxt;
  scptr->meth = meth;

  log(ssl_log, L_status, "Added SSL context: %s", scptr->name);

  dlink_add_tail(&ssl_list, &scptr->node, scptr);

  return scptr;
#else
  log(ssl_log, L_status, "No SSL support!");

  return NULL;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_update(struct ssl_context *scptr,   const char *name,
               int                 context, const char *cert,
               const char         *key,     const char *ciphers)
{
  strlcpy(scptr->name, name, sizeof(scptr->name));
  scptr->context = context;
  strlcpy(scptr->cert, cert, sizeof(scptr->cert));
  strlcpy(scptr->key, key, sizeof(scptr->key));
  strlcpy(scptr->ciphers, ciphers, sizeof(scptr->ciphers));

  log(ssl_log, L_status, "Updated SSL context: %s", scptr->name);

  return 0;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_delete(struct ssl_context *scptr)
{
#ifdef HAVE_SSL
  SSL_CTX_free((SSL_CTX *)scptr->ctxt);
#endif
  dlink_delete(&ssl_list, &scptr->node);

  mem_static_free(&ssl_heap, scptr);
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ssl_context *ssl_find_name(const char *name)
{
  struct ssl_context *scptr;
  hash_t              hash;
  
  hash = str_ihash(name);

  dlink_foreach(&ssl_list, scptr)
  {
    if(scptr->hash == hash)
    {
      if(!str_icmp(scptr->name, name))
        return scptr;
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ssl_context *ssl_find_id(uint32_t id)
{
  struct ssl_context *scptr;

  dlink_foreach(&ssl_list, scptr)
  {
    if(scptr->id == id)
      return scptr;
  }

  return NULL;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_new(int fd, struct ssl_context *scptr)
{
#ifdef HAVE_SSL
  io_list[fd].ssl = SSL_new((SSL_CTX *)scptr->ctxt);

  if(io_list[fd].ssl == NULL)
    return -1;

  ssl_set_sock(io_list[fd].ssl, fd);
  return 0;
#endif
  return -1;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_accept(int fd)
{
#ifdef HAVE_SSL
  int ret;
  int err;

  io_list[fd].sslerror = 0;
  io_list[fd].sslstate = 0;
  io_list[fd].sslwhere = SSL_ACCEPT;
  errno = 0;

  /* call the SSL handshake routine */
  ret = SSL_accept(io_list[fd].ssl);

  /* it was not done */
  if(ret <= 0)
  {
    /* get error code */
    err = SSL_get_error(io_list[fd].ssl, ret);

    /* call ssl_accept() again when socket gets readable */
    if(err == SSL_ERROR_WANT_READ)
    {
      io_unset_events(fd, IO_WRITE);
      io_list[fd].sslstate = SSL_ACCEPT_WANTS_READ;
      return 0;
    }

    /* call ssl_accept() again when socket gets writeable */
    if(err == SSL_ERROR_WANT_WRITE)
    {
      io_set_events(fd, IO_WRITE);
      io_unset_events(fd, IO_READ);
      io_list[fd].sslstate = SSL_ACCEPT_WANTS_WRITE;
      return 0;
    }

    /*
     * EWOULDBLOCK, EINTR, EAGAIN are ignored because
     * these say the handshake is in progress and needs
     * more events.
     */
    if(err == SSL_ERROR_SYSCALL)
    {
      /* ignore these */
      if(errno == EWOULDBLOCK ||
         errno == EINTR ||
         errno == EAGAIN)
      {
        io_list[fd].sslstate = SSL_ACCEPT_WANTS_READ;
        return 0;
      }

      return -1;
    }

    /* SSL handshake failed, report error! */
    io_list[fd].sslerror = err;
    return -1;
  }

  /* its in progress or done */
  return 0;
#else
  return -1;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_connect(int fd)
{
#ifdef HAVE_SSL
  int ret;
  int err;

  io_list[fd].sslerror = 0;
  io_list[fd].sslstate = 0;
  io_list[fd].sslwhere = SSL_CONNECT;
  errno = 0;

  /* call the SSL handshake routine */
  ret = SSL_connect(io_list[fd].ssl);

  /* it was not done */
  if(ret <= 0)
  {
    /* get error code */
    err = SSL_get_error(io_list[fd].ssl, ret);

    /* call ssl_accept() again when socket gets readable */
    if(err == SSL_ERROR_WANT_READ)
    {
      io_unset_events(fd, IO_WRITE);
      io_list[fd].sslstate = SSL_CONNECT_WANTS_READ;
      return 0;
    }

    /* call ssl_write() again when socket gets readable */
    if(err == SSL_ERROR_WANT_WRITE)
    {
      io_set_events(fd, IO_WRITE);
      io_unset_events(fd, IO_READ);
      io_list[fd].sslstate = SSL_CONNECT_WANTS_WRITE;
      return 0;
    }

    /*
     * EWOULDBLOCK, EINTR, EAGAIN are ignored because
     * these say the handshake is in progress and needs
     * more events.
     */
    if(err == SSL_ERROR_SYSCALL)
    {
      /* ignore these */
      if(errno == EWOULDBLOCK ||
         errno == EINTR ||
         errno == EAGAIN)
      {
        io_list[fd].sslstate = SSL_CONNECT_WANTS_READ;
        return 0;
      }

      return -1;
    }

    /* SSL handshake failed, report error! */
    io_list[fd].sslerror = err;
    return -1;
  }

  /* its in progress or done */
  return 0;
#else
  return -1;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_read(int fd, void *buf, size_t n)
{
#ifdef HAVE_SSL
  int ret;
  int err;

  io_list[fd].sslerror = 0;
  io_list[fd].sslstate = 0;
  io_list[fd].sslwhere = SSL_READ;
  io_list[fd].error = 0;
  io_list[fd].status.err = 0;
  errno = 0;

  ret = SSL_read(io_list[fd].ssl, buf, n);

  /* it was not done */
  if(ret < 0)
  {
    /* get error code */
    err = SSL_get_error(io_list[fd].ssl, ret);

    /* call ssl_read() again when socket gets readable */
    if(err == SSL_ERROR_WANT_READ)
    {
      io_unset_events(fd, IO_WRITE);
      io_list[fd].sslstate = 0;
      errno = EAGAIN;
      return -1;
    }

    /* call ssl_read() again when socket gets writeable */
    if(err == SSL_ERROR_WANT_WRITE)
    {
      io_set_events(fd, IO_WRITE);
      io_unset_events(fd, IO_READ);
      io_list[fd].sslstate = SSL_READ_WANTS_WRITE;
      errno = EAGAIN;
      return -1;
    }

    /*
     * EWOULDBLOCK, EINTR, EAGAIN are ignored because
     * these say the handshake is in progress and needs
     * more events.
     */
    if(err == SSL_ERROR_SYSCALL)
    {
      /* ignore these */
      if(errno == EWOULDBLOCK ||
         errno == EINTR ||
         errno == EAGAIN)
      {
        io_list[fd].sslstate = 0;
        return -1;
      }

      return -1;
    }

    if(err == SSL_ERROR_ZERO_RETURN)
      return 0;

    /* SSL handshake failed, report error! */
    io_list[fd].sslerror = err;
    io_list[fd].error = errno;
    io_list[fd].status.err = 1;
    return -1;
  }

  /* its in progress or done */
  return ret;
#else
  return -1;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_write(int fd, const void *buf, size_t n)
{
#ifdef HAVE_SSL
  int ret;
  int err;

  io_list[fd].sslerror = 0;
  io_list[fd].sslstate = 0;
  io_list[fd].sslwhere = SSL_WRITE;
  io_list[fd].error = 0;
  io_list[fd].status.err = 0;
  errno = 0;

  ret = SSL_write(io_list[fd].ssl, buf, n);

  /* it was not done */
  if(ret <= 0)
  {
    /* get error code */
    err = SSL_get_error(io_list[fd].ssl, ret);

    /* call ssl_write() again when socket gets writeable */
    if(err == SSL_ERROR_WANT_WRITE)
    {
      io_list[fd].sslstate = 0;
      errno = EWOULDBLOCK;
      return -1;
    }

    /* call ssl_write() again when socket gets writeable */
    if(err == SSL_ERROR_WANT_READ)
    {
      io_unset_events(fd, IO_WRITE);
      io_list[fd].sslstate = SSL_WRITE_WANTS_READ;
      errno = EWOULDBLOCK;
      return -1;
    }

    /*
     * EWOULDBLOCK, EINTR, EAGAIN are ignored because
     * these say the handshake is in progress and needs
     * more events.
     */
    if(err == SSL_ERROR_SYSCALL)
    {
      /* ignore these */
      if(errno == EWOULDBLOCK ||
         errno == EINTR ||
         errno == EAGAIN)
      {
        io_list[fd].sslstate = 0;
        return -1;
      }

      return -1;
    }

    if(err == SSL_ERROR_ZERO_RETURN)
      return 0;

    /* SSL handshake failed, report error! */
    io_list[fd].sslerror = err;
    io_list[fd].error = errno;
    io_list[fd].status.err = 1;
    return -1;
  }

  /* its in progress or done */
  return ret;
#else
  return -1;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
const char *ssl_strerror(int fd)
{
#ifdef HAVE_SSL
  switch(io_list[fd].sslerror)
  {
    case SSL_ERROR_NONE:
    {
      return "No error";
    }
    case SSL_ERROR_SSL:
    {
      long        err;
      const char *s;

      err = ERR_get_error();

      if(err)
        while(ERR_get_error());

      s = ERR_error_string(err, NULL);

      return s ? s : "Internal OpenSSL error or protocol error";
    }
    case SSL_ERROR_WANT_READ:
    {
      return "OpenSSL functions requested a read";
    }
    case SSL_ERROR_WANT_WRITE:
    {
      return "OpenSSL functions requested a write";
    }
    case SSL_ERROR_WANT_X509_LOOKUP:
    {
      return "OpenSSL requested a X509 lookup which didnt arrive";
    }
    case SSL_ERROR_SYSCALL:
    {
      return "Underlying syscall error";
    }
    case SSL_ERROR_ZERO_RETURN:
    {
      return "Underlying socket operation returned zero";
    }
    case SSL_ERROR_WANT_CONNECT:
    {
      return "OpenSSL functions wanted a connect";
    }
    default:
    {
      return "Unknown OpenSSL error (huh?)";
    }
  }
#else
  return "OpenSSL not supported";
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_close(int fd)
{
#ifdef HAVE_SSL
  if(io_list[fd].ssl)
  {
    SSL_shutdown(io_list[fd].ssl);
    io_list[fd].ssl = NULL;
  }
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
int ssl_handshake(int fd, struct io *iofd)
{
#ifdef HAVE_SSL
  /* Continue ssl_accept() */
  if(iofd->sslstate == SSL_ACCEPT_WANTS_READ ||
     iofd->sslstate == SSL_ACCEPT_WANTS_WRITE)
  {
    if(iofd->status.events & IO_READ)
    {
      if(iofd->sslstate == SSL_ACCEPT_WANTS_READ)
      {
        if(ssl_accept(fd))
        {
          log(ssl_log, L_warning, "SSL handshake error on %s: %s",
              iofd->note, ssl_strerror(fd));

          ssl_close(fd);
          return -1;
        }

        if(iofd->sendq.size)
          io_set_events(fd, IO_WRITE);
      }
    }

    if(iofd->status.events & IO_WRITE)
    {
      if(iofd->sslstate == SSL_ACCEPT_WANTS_WRITE)
      {
        if(ssl_accept(fd))
        {
          log(ssl_log, L_warning, "SSL handshake error on %s: %s",
              iofd->note, ssl_strerror(fd));

          ssl_close(fd);
          return -1;
        }

        if(iofd->sendq.size == 0)
          io_unset_events(fd, IO_WRITE);
        io_set_events(fd, IO_READ);
      }
    }

    return 0;
  }

  /* Continue ssl_connect() */
  if(iofd->sslstate == SSL_CONNECT_WANTS_READ ||
     iofd->sslstate == SSL_CONNECT_WANTS_WRITE)
  {
    if(iofd->status.events & IO_READ)
    {
      if(iofd->sslstate == SSL_CONNECT_WANTS_READ)
      {
        if(ssl_connect(fd))
        {
          log(ssl_log, L_warning, "SSL handshake error on %s: %s",
              iofd->note, ssl_strerror(fd));

          ssl_close(fd);
          return -1;
        }

        if(iofd->sendq.size)
          io_set_events(fd, IO_WRITE);
      }
    }

    if(iofd->status.events & IO_WRITE)
    {
      if(iofd->sslstate == SSL_CONNECT_WANTS_WRITE)
      {
        if(iofd->sendq.size == 0)
          io_unset_events(fd, IO_WRITE);

        if(ssl_connect(fd))
        {
          log(ssl_log, L_warning, "SSL handshake error on %s: %s",
              iofd->note, ssl_strerror(fd));

          ssl_close(fd);
          return -1;
        }

        if(iofd->sendq.size == 0)
          io_unset_events(fd, IO_WRITE);
        io_set_events(fd, IO_READ);
      }
    }

    return 0;
  }

  /* Continue ssl_read() */
  if(iofd->sslstate == SSL_READ_WANTS_WRITE)
  {
    if(iofd->status.events & IO_WRITE)
    {
      iofd->status.events &= ~IO_WRITE;
      iofd->status.events |= IO_READ;

      if(iofd->sendq.size == 0)
        io_unset_events(fd, IO_WRITE);
      io_set_events(fd, IO_READ);
    }

    return 0;
  }

  /* Continue ssl_write() */
  if(iofd->sslstate == SSL_WRITE_WANTS_READ)
  {
    if(iofd->status.events & IO_READ)
    {
      iofd->status.events &= ~IO_READ;
      iofd->status.events |= IO_WRITE;

      if(iofd->sendq.size)
        io_set_events(fd, IO_WRITE);
    }

    return 0;
  }

  return 0;
#else
  return -1;
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_cipher(int fd, char *ciphbuf, size_t n)
{
#ifdef HAVE_SSL
  const SSL_CIPHER *cipher;
  int               bits;
  char              ciphername[64];

  if(io_list[fd].ssl == NULL)
  {
#endif
    strlcpy(ciphbuf, "NONE", n);
    return;
#ifdef HAVE_SSL
  }

  cipher = SSL_get_current_cipher(io_list[fd].ssl);

  SSL_CIPHER_get_bits(cipher, &bits);

  strlcpy(ciphername, SSL_CIPHER_get_name(cipher), sizeof(ciphername));

  if(!str_ncmp(ciphername, "AES", 3))
  {
    ciphername[3] = '\0';
  }
  else if(!str_ncmp(ciphername, "IDEA", 4))
  {
    ciphername[4] = '\0';
  }
  else if(!str_ncmp(ciphername, "EXP1024", 7))
  {
    char *p;
    strlcpy(ciphername, &SSL_CIPHER_get_name(cipher)[8], sizeof(ciphername));
    if((p = strchr(ciphername, '-')))
      *p = '\0';
  }
  else if(!str_ncmp(ciphername, "EXP", 3))
  {
    char *p;
    strlcpy(ciphername, &SSL_CIPHER_get_name(cipher)[4], sizeof(ciphername));
    if((p = strchr(ciphername, '-')))
      *p = '\0';
  }
  else
  {
    char *p;
    if((p = strchr(ciphername, '-')))
      *p = '\0';
  }

  if(!str_cmp(ciphername, "DES") && bits == 168)
    strcpy(ciphername, "3DES");

  str_snprintf(ciphbuf, n, "%s/%u", ciphername, bits);
#endif
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ssl_context *ssl_pop(struct ssl_context *scptr)
{
  if(scptr)
  {
    if(!scptr->refcount)
      log(ssl_log, L_warning, "Poping deprecated ssl context #%u",
          scptr->id);

    scptr->refcount++;
  }

  return scptr;
}


/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
struct ssl_context *ssl_push(struct ssl_context **scptr)
{
  if(*scptr)
  {
    if((*scptr)->refcount == 0)
    {
      log(ssl_log, L_warning, "Trying to push deprecated ssl context #%u",
          (*scptr)->id);
    }
    else
    {
      if(--(*scptr)->refcount == 0)
        ssl_delete(*scptr);

      (*scptr) = NULL;
    }
  }

  return *scptr;
}

/* ------------------------------------------------------------------------ *
 * ------------------------------------------------------------------------ */
void ssl_dump(struct ssl_context *scptr)
{
  if(scptr == NULL)
  {
    dump(ssl_log, "[================ ssl summary ================]");

    dlink_foreach(&ssl_list, scptr)
    {
      dump(ssl_log, " #%u: [%u] %-20s (%s)",
           scptr->id, scptr->refcount, scptr->name,
           scptr->context == SSL_CONTEXT_SERVER ? "server" : "client");
    }

    dump(ssl_log, "[============= end of ssl summary ============]");
  }
  else
  {
    dump(ssl_log, "[================= ssl dump ==================]");

    dump(ssl_log, "         id: #%u", scptr->id);
    dump(ssl_log, "   refcount: %u", scptr->refcount);
    dump(ssl_log, "       hash: %p", scptr->hash);
    dump(ssl_log, "    context: %s",
         scptr->context == SSL_CONTEXT_SERVER ? "server" : "client");
    dump(ssl_log, "       name: %s", scptr->name);
    dump(ssl_log, "       cert: %s", scptr->cert);
    dump(ssl_log, "        key: %s", scptr->key);
    dump(ssl_log, "    ciphers: %s", scptr->ciphers);

    if(scptr->ctxt)
    {
/*      ASN1_UTCTIME *utc;
      time_t from, to, current = timer_systime;
      struct tm *tm;
      char v_from[32], v_to[32];
      char buf[256];
      long serial, version;
      ASN1_INTEGER *asn1_i;*/
/*      X509_STORE *cstore;*/
/*      X509_OBJECT *cobj;*/

/*      cstore = SSL_CTX_get_cert_store(scptr->ctxt);*/
      /*      dump(ssl_log, "x509: %u", sk_X509_num(scptr->ctxt->extra_certs));

      dump(ssl_log, "--------------- certificate info --------------");*/

      /* ugly time string formatting */
/*      utc = X509_get_notBefore(scptr->crt);
      from = ASN1_UTCTIME_get(utc);
      tm = localtime(&from);
      strftime(v_from, 32, "%Y-%m-%d %H:%M", tm);
      utc = X509_get_notAfter(scptr->crt);
      to = ASN1_UTCTIME_get(utc);
      tm = localtime(&to);
      strftime(v_to, 32, "%Y-%m-%d %H:%M", tm);

      asn1_i = X509_get_serialNumber(cert);
      serial = ASN1_INTEGER_get(asn1_i);
      version = X509_get_version(cert);

      dump(ssl_log, "serial #%lu (X509 version %lu)", serial, version);
      X509_NAME_oneline(X509_get_subject_name(cert), buf, 255);
      dump(ssl_log, "subject: %s", buf);
      X509_NAME_oneline(X509_get_issuer_name(cert), buf, 255);
      dump(ssl_log, "issuer:  %s", buf);
      dump(ssl_log, "valid from %s to %s (%s)",
           v_from, v_to,
           (from <= current && current < to) ? "valid" :
           (from > current) ? "not yet valid" : "expired");*/
    }

    dump(ssl_log, "[============== end of ssl dump ==============]");
  }
}
