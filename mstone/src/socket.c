/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Netscape Mailstone utility,
 * released March 17, 2000.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):     Sean O'Rourke <sean@sendmail.com>
 *                     Sendmail, Inc.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of
 * those above.  If you wish to allow use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the NPL or the GPL.
 */

#include <sched.h>
#include "bench.h"
#include "pish.h"
#include "xalloc.h"

SOCKET BADSOCKET_VALUE = (SOCKET)-1;

/*
**  Real sockets
*/

struct sock_socket
{
	socktype_t	type;
	_SOCKET		fd;
};

SOCKET
sock_open(int fd)
{
	struct sock_socket *ret = XALLOC(struct sock_socket);
	ret->type = &SOCK_SOCKET;
	ret->fd = fd;
	return (SOCKET)ret;
}

static int
sock_read(SOCKET this, char *buf, int len)
{
	struct sock_socket *s = (struct sock_socket*)this;
	assert(this != BADSOCKET_VALUE);
	return _NETREAD(s->fd, buf, len);
}

static int
sock_write(SOCKET this, char *buf, int len)
{
	struct sock_socket *s = (struct sock_socket*)this;
	assert(this != BADSOCKET_VALUE);
	return _NETWRITE(s->fd, buf, len);
}

static void
sock_close(SOCKET this)
{
	struct sock_socket *s = (struct sock_socket*)this;
	if (this == BADSOCKET_VALUE)
		return;
	_NETCLOSE(s->fd);
	xfree(s);
}

static _SOCKET
sock_getfd(SOCKET this)
{
	struct sock_socket *s = (struct sock_socket*)this;
	assert(this != BADSOCKET_VALUE);
	assert(s->fd >= -1);
	return s->fd;
}

struct socktype SOCK_SOCKET =
{
	&sock_read,
	&sock_write,
	&sock_close,
	&sock_getfd
};

#ifdef SOCK_LINESPEED

/*
**  Slow sockets
*/

typedef struct ls_socket
{
	socktype_t	type;
	SOCKET		socket;
	int		latency;
	int		bandwidth;
} ls_socket_t;

/* time_left < LS_DONT_WAIT => return right away */
#define LS_DONT_WAIT 	10
/* LS_DONT_WAIT <= time_left < LS_SCHED_YIELD => just sched_yield() */
#define LS_SCHED_YIELD 	100
#define LS_BLOCKSIZE	(8192) /* largest block-size to transfer at once */

#define OPTIME(S, N) (((S)->bandwidth > 0) \
		      ?((S)->latency + ((N)*1000)/(S)->bandwidth) \
		      :0)

#define TV_SUB(A, B) (((A).tv_sec - (B).tv_sec)*1000 \
                      + ((A).tv_usec - (B).tv_usec)/1000)

#define WASTE_TIME(T) do { \
	if((T) >= LS_DONT_WAIT) \
	{ \
		if((T) < LS_SCHED_YIELD) \
                        sched_yield(); \
		else \
			MS_usleep((T)*1000); \
	} \
} while (0)

#define MIGHT_WAIT(T) ((T) >= LS_DONT_WAIT)

SOCKET
ls_open(SOCKET fd, void *_cmd)
{
	pish_command_t *cmd = (pish_command_t*)_cmd;
	ls_socket_t *s;
	int buf = LS_BLOCKSIZE;

	assert(fd != BADSOCKET_VALUE);

	if((setsockopt(SOCK_FD(fd), SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf)) < 0)
	   || (setsockopt(SOCK_FD(fd), SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf)) < 0))
		return NULL;

	s = XALLOC(struct ls_socket);
	s->type = &LS_SOCKET;
	s->socket = fd;
	s->latency = sample(cmd->latency);
	s->bandwidth = sample(cmd->bandwidth);
	return (SOCKET)s;
}

static int
delayed_read(SOCKET this, char *buf, int count)
{
	struct timeval start, stop;
	int ret;
	int time_left;
	struct ls_socket *sock = (struct ls_socket*)this;

	assert(this != BADSOCKET_VALUE);

	if(count > LS_BLOCKSIZE)
		count = LS_BLOCKSIZE;
	time_left = OPTIME(sock, count);
	if(MIGHT_WAIT(time_left))
	{
		int optime, save_errno;
		gettimeofday(&start, NULL);
		ret = NETREAD(sock->socket, buf, count);
		if(ret < 0)
			return ret;
		
		save_errno = errno;
		/* update to reflect actual bytes read: */
		gettimeofday(&stop, NULL);
		time_left = OPTIME(sock, ret);
		optime = TV_SUB(stop, start);
		time_left -= optime;
		WASTE_TIME(time_left);
		errno = save_errno;
		return ret;
	}
	return NETREAD(sock->socket, buf, count);
}

static int
delayed_write(SOCKET this, char *buf, int count)
{
	struct timeval start, stop;
	int ret;
	int time_left;
	struct ls_socket *sock = (struct ls_socket*)this;

	assert(this != BADSOCKET_VALUE);

	if(count > LS_BLOCKSIZE)
		count = LS_BLOCKSIZE;
	time_left = OPTIME(sock, count);
	if(MIGHT_WAIT(time_left))
	{
		int optime, save_errno;
		gettimeofday(&start, NULL);
		ret = NETWRITE(sock->socket, buf, count);
		if(ret <= 0)
			return ret;
		save_errno = errno;
		gettimeofday(&stop, NULL);
		optime = TV_SUB(stop, start);
		time_left -= optime;
		WASTE_TIME(time_left);
		return ret;
	}
	return NETWRITE(sock->socket, buf, count);
}

static void
ls_close(SOCKET this)
{
	struct ls_socket *sock = (struct ls_socket*)this;
	if (this != BADSOCKET_VALUE && sock->socket)
	{
		NETCLOSE(sock->socket);
	}
	xfree(sock);
}

static _SOCKET
ls_getfd(SOCKET this)
{
	struct ls_socket *sock = (struct ls_socket*)this;
	int fd;

	assert(this != BADSOCKET_VALUE);

	fd = SOCK_FD(sock->socket);
	assert(fd >= -1);
	return fd;
}

struct socktype LS_SOCKET =
{
	&delayed_read,
	&delayed_write,
	&ls_close,
	&ls_getfd
};

#endif /* SOCK_LINESPEED */

#ifdef SOCK_SSL
/*
**  SSL sockets
*/

#include <openssl/ssl.h>
#include <openssl/err.h>

struct ssl_socket
{
	socktype_t	type;
	SSL		*ssl;
	SOCKET		sock;
};

static void
mysslerr(FILE *f, SSL *ssl, int ret)
{
	unsigned long err = SSL_get_error(ssl, ret);
	fprintf(f, "ssl_socket(%d): ", SSL_get_fd(ssl));
	switch(err)
	{
	  case SSL_ERROR_NONE:
		fprintf(f, "no error\n");
		break;
	  case SSL_ERROR_ZERO_RETURN:
		fprintf(f, "zero return\n");
		break;
	  case SSL_ERROR_WANT_READ:
		fprintf(f, "want read\n");
		break;
	  case SSL_ERROR_WANT_WRITE:
		fprintf(f, "want write\n");
		break;
	  case SSL_ERROR_WANT_X509_LOOKUP:
		fprintf(f, "want x509\n");
		break;
	  case SSL_ERROR_SYSCALL:
		fprintf(f, "syscall: %s\n", strerror(errno));
		break;
	  case SSL_ERROR_SSL:
		fprintf(f, "ssl problem...\n");
		ERR_print_errors_fp(stderr);
		break;
	};
}

static RSA *
tmp_rsa_key(SSL *ssl, int export, int keylen)
{
	static RSA *rsa_tmp = NULL;
	if (rsa_tmp == NULL &&
	    (rsa_tmp = RSA_generate_key(keylen, RSA_F4,
					NULL, NULL)) == NULL)
	{
		fprintf(stderr, "tmp_rsa_key(%p, %d, %d): RSA error\n",
			ssl, export, keylen);
	}
	return rsa_tmp;
}

int
eternal_1(X509_STORE_CTX * x509)
{
	/* ignore cert failures */
	return 1;
}

SOCKET
ssl_open(SOCKET sock, void *_cmd)
{
	pish_command_t *cmd = (pish_command_t*)_cmd;
	static int sslinit = 0;
	SSL_CTX *ctx = NULL;
	SSL *ssl = NULL;
	int ret = 0;
	int fd;
	struct ssl_socket *retval = NULL;
	char *blame = "?";
	char *my_private_key = cmd->key;
	char *my_cert = cmd->cert;

	assert(sock != BADSOCKET_VALUE);
	assert(_cmd != NULL);

	fd = SOCK_FD(sock);
	/* initialize ctx */
	/* XXX: not threadsafe */
	if (ctx == NULL)
	{
		if (!sslinit)
		{
			SSL_library_init();
			/* just go ahead and do this now. */
			SSL_load_error_strings();
			sslinit = 1;
		}
		if ((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
		{
			blame = "SSL_CTX_new";
			goto error;
		}
		/* do other initialization: */
		SSL_CTX_set_tmp_rsa_callback(ctx, tmp_rsa_key);
		/* Private Key */
		if (my_private_key != NULL)
		{
			if (SSL_CTX_use_PrivateKey_file(ctx, my_private_key,
							SSL_FILETYPE_PEM) <= 0)
			{
				blame = "SSL_CTX_use_PrivateKey_file";
				goto error;
			}
			if (SSL_CTX_check_private_key(ctx) <= 0)
			{
				fprintf(stderr,
					"ssl_open(%d): Warning: invalid private key\n", fd);
				ERR_print_errors_fp(stderr);
			}
		}
		/* Cert */
		if (my_cert != NULL
		    && SSL_CTX_use_certificate_file(ctx, my_cert, SSL_FILETYPE_PEM) <= 0)
		{
			blame = "SSL_CTX_use_certificate_file";
			goto error;
		}

		/* CA's */
		SSL_CTX_set_tmp_rsa_callback(ctx, tmp_rsa_key);
		SSL_CTX_set_cert_verify_callback(ctx, eternal_1, NULL);
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	}
	/* if we get here, we know ctx has been successfully initialized */
	/* initialize ssl */
	if ((ssl = SSL_new(ctx)) == NULL)
	{
		fprintf(stderr, "ssl_open(%d): ssl_new error\n", fd);
		/* error */
		return BADSOCKET_VALUE;
	}
	if ((ret = SSL_set_fd(ssl, fd)) <= 0)
	{
		blame = "SSL_set_fd";
		goto error;
	}
	SSL_set_connect_state(ssl);
	if ((ret = SSL_connect(ssl)) <= 0)
	{
		blame = "SSL_connect";
		goto error;
	}
	retval = XALLOC(struct ssl_socket);
	retval->type = &SSL_SOCKET;
	retval->ssl = ssl;
	retval->sock = sock;
	return (SOCKET)retval;
 error:
	{
		fprintf(stderr, "ssl_open(%d) failed in %s:\n", fd, blame);
		mysslerr(stderr, ssl, ret);
		return BADSOCKET_VALUE;
	}
}	

static int
ssl_read(SOCKET this, char *buf, int len)
{
	struct ssl_socket *s = (struct ssl_socket*)this;
	assert(this != BADSOCKET_VALUE);
	return SSL_read(s->ssl, buf, len);
}

static int
ssl_write(SOCKET this, char *buf, int len)
{
	struct ssl_socket *s = (struct ssl_socket*)this;
	assert(this != BADSOCKET_VALUE);
	return SSL_write(s->ssl, buf, len);
}

static void
ssl_close(SOCKET this)
{
	struct ssl_socket *s = (struct ssl_socket*)this;
	int ret;
	assert(this != BADSOCKET_VALUE);
	if ((ret = SSL_shutdown(s->ssl)) <= 0)
	{
		fprintf(stderr, "ssl_close(%d): ", SOCK_FD(this));
		mysslerr(stderr, s->ssl, ret);
	}
	SSL_free(s->ssl);
	NETCLOSE(s->sock);
}

static _SOCKET
ssl_getfd(SOCKET this)
{
	struct ssl_socket *s = (struct ssl_socket*)this;
	int fd;

	assert(this != BADSOCKET_VALUE);

	fd = SSL_get_fd(s->ssl);
	assert(fd >= -1);
	return fd;
}

struct socktype SSL_SOCKET =
{
	&ssl_read,
	&ssl_write,
	&ssl_close,
	&ssl_getfd
};

#endif /* SOCK_SSL */
