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

#ifndef _checksum_h
#define _checksum_h

#include "bench.h"

typedef enum {
	CS_MD5,			/* MD5 hash */
	CS_NTYPES
} cs_type_t;

/*
**  CS_MARKER marks the start of a checksum.  It should be something
**  that will not otherwise appear in the text.
*/

#define CS_MARKER CRLF "=CS="

#define CS_MAXLEN 64

/*
**  CS_BEGIN(t) -- start cstype-type checksum
**
**	This routine will never return NULL.
*/

struct checksum_t *
cs_begin(cs_type_t cstype);

/*
**  CS_DATA(state, buf, len) -- sum another buffer
*/

void
cs_data(struct checksum_t * state, char * buf, int len);

/*
**  CS_END(state) -- return checksum value.
**
**	The value returned is a printable string starting with
**	CS_MARKER, which should be freed using xfree().  cs_end() may
**	be called any number of times, but cs_data may not be called
**	after the first call to cs_end().
*/

char *
cs_end(struct checksum_t * state);

/*
**  CS_RETRIEVE(ptcx, sock, msg_start, msg_term) -- verify a message
**
**	A convenience routine to allow retrieval protocols to
**	calculate checksums over retrieved messages, cs_retrieve reads
**	data from sock up to and including the first instance of
**	msg_term.  The checksum is calculated over all data beteween
**	the first instance of msg_start and the first subsequent
**	CS_MARKER.  If CS_MARKER is found, the computed checksum is
**	compared with the data at CS_MARKER.
**
**	Returns:
**		-1 -- read error.
**		0 -- invalid checksum found.
**		1 -- valid checksum found.
**		2 -- no checksum.
*/

int
cs_retrieve(ptcx_t ptcx, SOCKET sock, char *msg_start, char *msg_term);

#endif /* _checksum_h */
