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

#include "sb.h"
#include "xalloc.h"

static int _sb_refill(ptcx_t,	socket_buffer *);
static void _sb_consume(socket_buffer *, int );

socket_buffer *
sb_new(sock, len)
	SOCKET		sock;
	int		len;
{
	socket_buffer *sb = xalloc(sizeof(socket_buffer) + len);
	sb->buf[len] = '\0';
	sb->len = len;
	sb->data = 0;
	sb->sock = sock;
	sb->eof = 0;
	return sb;
}

void
sb_delete(sb)
	socket_buffer	*sb;
{
	if (!BADSOCKET(sb->sock))
		NETCLOSE(sb->sock);
	xfree(sb);
}

static int
_sb_refill(ptcx, sb)
	ptcx_t		ptcx;
	socket_buffer	*sb;
{
	int len;
	if (sb->eof)
		return -1;
	len = retryRead(ptcx, sb->sock, sb->buf + sb->data,
			sb->len - sb->data);
	if (len <= 0)
	{
		D_PRINTF(debugfile, "_sb_refill: retryRead failed: %s\n",
			strerror(errno));
		sb->eof = 1;
		return -1;
	}
	sb->data += len;
	ptcx->bytesread += len;
	return len;
}

static void
_sb_consume(sb, len)
	socket_buffer	*sb;
	int		len;
{
	assert (len <= sb->data);
	sb->data -= len;
	if (sb->data > 0)
		memmove(sb->buf, sb->buf + len, sb->data);
}

int
sb_discard(ptcx, sb, pat)
	ptcx_t		ptcx;
	socket_buffer	*sb;
	char 		*pat;
{
	int patlen = strlen(pat);
	char *ptr;
	for (;;)
	{
		if (sb->data < patlen)
		{
			if (_sb_refill(ptcx, sb) < 0)
			{
				/* EOF */
				_sb_consume(sb, sb->data);
				return -1;
			}
			continue;
		}
		sb->buf[sb->data] = '\0';
		if ((ptr = strstr(sb->buf, pat)) != NULL)
		{
			ptr += patlen; /* discard delimiter as well */
			_sb_consume(sb, (ptr - sb->buf));
			return 0;
		}
		_sb_consume(sb, sb->data - (patlen - 1));
	}
	/* notreached */
	return -1;
}

void
sb_discardn(ptcx, sb, n)
	ptcx_t		ptcx;
	socket_buffer	*sb;
	int		n;
{
	for (;;)
	{
		if (n < sb->data)
		{
			_sb_consume(sb, n);
			return;
		}
		n -= sb->data;
		_sb_consume(sb, sb->data);
		if (_sb_refill(ptcx, sb) < 0)
			return;
	}
}

int
sb_get(ptcx, sb, pat)
	ptcx_t		ptcx;
	socket_buffer	*sb;
	char 		*pat;
{
	int 	patlen = strlen(pat);
	int	offset = 0;
	char 	*ptr;
	for (;;)
	{
		while (sb->data < sb->len && sb->data - offset < patlen)
			if (_sb_refill(ptcx, sb) < 0)
				return -1;
		if (sb->data == sb->len)
			return sb->len;
		if ((ptr = strstr(sb->buf + offset, pat)) != NULL)
			return (ptr - sb->buf);
		offset = sb->data - (patlen - 1);
	}
	/* notreached */
	return -1;
}

int
sb_read(ptcx, sb)
	ptcx_t		ptcx;
	socket_buffer	*sb;
{
	int ret;
	if (sb->data == 0)
		_sb_refill(ptcx, sb);
	ret = sb->data;
	_sb_consume(sb, sb->data);
	return ret;
}

