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

#ifndef _sb_h
#define _sb_h

#include "socket.h"
#include "xalloc.h"
#include "bench.h"

/* sb -- poor man's STDIO */

/*
**  SOCKET_BUFFER -- buffered sockets to make parsing easier.
*/

typedef struct socket_buffer {
	int	data;		/* data in buffer */
	int	len;		/* length of buffer */
	SOCKET	sock;		/* underlying socket */
	int	eof;		/* have we seen eof? */
	char	buf[1];		/* actually len + 1 bytes */
} socket_buffer;

socket_buffer * sb_new(SOCKET, int);
void sb_delete(socket_buffer *);
int sb_discard(ptcx_t, socket_buffer *, char *);
void sb_discardn(ptcx_t, socket_buffer *, int);
int sb_get(ptcx_t ptcx, socket_buffer *, char *);

int sb_read(ptcx_t ptcx, socket_buffer *sb);

#endif /* _sb_h */
