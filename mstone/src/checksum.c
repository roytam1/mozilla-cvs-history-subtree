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

#include "checksum.h"
#include "xalloc.h"
#include "sb.h"
/* openldap md5 functions */
#include "md5.h"

struct checksum_t
{
	cs_type_t	type;
	union {
		struct lutil_MD5Context	md5;
	} u;
};

struct checksum_t *
cs_begin(cs_type_t cstype)
{
	struct checksum_t * ret = XALLOC(struct checksum_t);

	ret->type = cstype;
	switch (cstype)
	{
	case CS_MD5:
		lutil_MD5Init(&ret->u.md5);
		break;
	default:
		assert(0);
	};
	return ret;
}

void
cs_data(struct checksum_t * state, char * buf, int len)
{
	assert(state != NULL);
	assert(buf != NULL);

	switch (state->type)
	{
	case CS_MD5:
		lutil_MD5Update(&state->u.md5, (unsigned char *)buf, len);
		break;
	default:
		assert(0);
	};
}

char *
cs_end(struct checksum_t * state)
{
	char *ret;

	assert(state != NULL);

	switch (state->type)
	{
	case CS_MD5:
	{
		unsigned char digest[16];
		lutil_MD5Final(digest, &state->u.md5);
		ret = xalloc(32 + strlen(CS_MARKER "MD5=") + 1);
		sprintf(ret, CS_MARKER "MD5=%08lx%08lx%08lx%08lx",
			*(unsigned long*)digest,
			*(unsigned long*)(digest + 4),
			*(unsigned long*)(digest + 8),
			*(unsigned long*)(digest + 12));
		break;
	}
	default:
		assert(0);
	};
	return ret;
}

int
cs_retrieve(ptcx_t ptcx, SOCKET sock, char *msg_start, char *msg_term)
{
	socket_buffer * sb;
	struct checksum_t * cs;
	char *cs_value;
	char *cs_seen;

	int nread;
	int msg_term_len;
	int ret = -1;

	cs = NULL;
	cs_value = NULL;
	sb = sb_new(sock, 4096);
	msg_term_len = strlen(msg_term);

	/* skip to start of message */
	if (sb_discard(ptcx, sb, msg_start) < 0)
		goto CLEANUP;

	/* checksum over body */
	cs = cs_begin(CS_MD5);
	for (;;)
	{
		nread = sb_get(ptcx, sb, msg_term);
		if (nread < 0)
		{
			D_PRINTF (debugfile, "EOF looking for checksum\n");
			ret = 2; /* XXX: might be error */
			goto CLEANUP;
		}
		else if (strncmp(sb->buf + nread,
				 msg_term, msg_term_len) == 0)
			break;

		if (nread > CS_MAXLEN)
		{
			/* do more checksumming */
			cs_data(cs, sb->buf, nread - CS_MAXLEN);
			sb_discardn(ptcx, sb, nread - CS_MAXLEN);
		}
	}			
	/* sb->buf + nread is the start of msg_term */

	sb->buf[nread] = '\0';
	if (nread < CS_MAXLEN)
		cs_seen = sb->buf;
	else
		cs_seen = sb->buf + nread - CS_MAXLEN;

	cs_seen = strstr(cs_seen, CS_MARKER);

	if (cs_seen == NULL)
	{
		ret = 2;
		goto CLEANUP;
	}

	/* remember the last bit of message left in buffer */
	cs_data(cs, sb->buf, cs_seen - sb->buf);
	cs_value = cs_end(cs);

	if (strncmp(cs_seen, cs_value, strlen(cs_value)) == 0)
		ret = 1;
	else
		ret = 0;

 CLEANUP:
	switch (ret)
	{
	case -1:
		D_PRINTF(stderr, "retrMsg: ERROR checksum\n");
		break;
	case 0:
		D_PRINTF(stderr, "retrMsg: BAD checksum\n");
		break;
	case 1:
		D_PRINTF(stderr, "retrMsg: OK checksum\n");
		break;
	case 2:
		D_PRINTF(stderr, "retrMsg: NO checksum\n");
		break;
	};

	if (cs)
		xfree(cs);
	if (cs_value)
		xfree(cs_value);
	if (sb)
		sb_delete(sb);
	return ret;
}
