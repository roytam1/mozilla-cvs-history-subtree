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

#ifndef _socket_h
#define _socket_h

/*
**  Socket wrapper support
*/

#include "sysdep.h"

struct socktype;

typedef struct SOCKET_s
{
	struct socktype*type;
	void		*sock[1];
} *SOCKET;

extern SOCKET BADSOCKET_VALUE;

/* Functions a socket must implement: */
typedef int 	(*read_t)(SOCKET, char *, int);
typedef int 	(*write_t)(SOCKET, char *, int);
typedef void 	(*close_t)(SOCKET);
typedef _SOCKET	(*getfd_t)(SOCKET);

typedef struct socktype
{
	read_t	read;
	write_t	write;
	close_t	close;
	getfd_t	getfd;
} * socktype_t;

#define NETREAD(S, B, L)	((S)->type->read((S), (B), (L)))
#define NETWRITE(S, B, L)	((S)->type->write((S), (B), (L)))
#define NETCLOSE(S)		((S)->type->close(S))
#define BADSOCKET(S)		(((S) == BADSOCKET_VALUE) || _BADSOCKET(SOCK_FD(S)))
#define BADSOCKET_ERRNO(sock)	BADSOCKET(sock)
/*  #define INIT_SOCKET(sock, host) init_socket((sock), (host)) */
#define SOCK_FD(s)	((s)->type->getfd(s))

/* Basic sockets */
extern struct socktype SOCK_SOCKET;
SOCKET sock_open(int fd);

#ifdef SOCK_SSL
extern struct socktype SSL_SOCKET;
extern SOCKET ssl_open(SOCKET sock, void* pish);
#define SSL_INIT(S, cmd) (void)((S) = ssl_open((S), cmd))
#else /* SOCK_SSL */
#define SSL_INIT(S, cmd) (void)0
#endif /* SOCK_SSL */

#ifdef SOCK_LINESPEED
extern struct socktype LS_SOCKET;
extern SOCKET ls_open(SOCKET fd, void*);
#define LS_INIT(S, cmd) (void)((S) = ls_open((S), (cmd)))
#else /* SOCK_LINESPEED */
#define LS_INIT(S, cmd) (void)0
#endif /* SOCK_LINESPEED */

#endif /* _socket_h */
