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

#ifdef AUTOGEN
#include <assert.h>
#include "generate.h"
#include "idle.h"
#include "xalloc.h"
#include "checksum.h"

#define GEN_MSGIDS

/*
**  Data used for generated messages.
*/

/* headers to always put on a messages */
#ifdef GEN_MSGIDS
/* pass through printf(X, starttime.tv_sec, procnum, threadnum, msgnum) */
static char GEN_HDRS[] =
"From: mailstone-autogen" CRLF
"To: test-user" CRLF
"Message-ID: <p%d.t%d.m%d@mstone-generated>" CRLF
"Date: %s" CRLF
"Subject: benchmark message (auto-generated)" CRLF;
#else
static char GEN_HDRS[] =
"From: mailstone-autogen" CRLF
"To: test-user" CRLF
"Message-ID: <12345678.123@nowhere>" CRLF
"Date: Fri, 10 Jul 1999 00:00:00 -0800" CRLF
"Subject: benchmark message (auto-generated)" CRLF;
#endif
static int N_GEN_HDRS = 5;

/* format to use for added headers (%d replaced by header index) */
static char GEN_HDRFORMAT[] = "X-generated-%d: value of generated header" CRLF;

/* line of generated mesage */
static char GEN_LINE[] =
"012345678901234567890123456789012345678901234567890123456789" CRLF;

/* end-of-message sequence */
static char EOM[] = CRLF "." CRLF;

/* boundary string format */
#define MIME_BOUNDARY 		"autogen-boundary-string:%d"

/* attachment file name */
#define MIME_FILE		"foo.dat"

/* attachment MIME type */
#define MIME_TYPE		"application/x-generated"

/* plain text before first MIME subpart */
#define MIME_WARNING		"This message is MIME." CRLF

/* section boundary (subjected to printf(X, int)) */
static char MIME_SEPARATOR[] =
CRLF
"--" MIME_BOUNDARY CRLF;

/* End of MIME message (passed through printf(X, int)) */
static char MIME_TERMINATOR[] =
CRLF
"--" MIME_BOUNDARY "--" CRLF;

/* multipart header (passed through printf(X, int)). */
static char MIME_MULTIPART_HDR[] =
"MIME-Version: 1.0" CRLF
"Content-Type: multipart/mixed;" CRLF
" boundary=\"" MIME_BOUNDARY "\"" CRLF;

/* multipart header (passed through printf(X, char *, char *)). */
static char MIME_PART_HDR[] =
"Content-Type: %s" CRLF
"Content-Transfer-Encoding: base64" CRLF
"Content-Disposition: attachment; filename=\"%s\"" CRLF;

/* actual body blocksize == gen_blksize - gen_blksize % strlen(GEN_LINE) */
static int	gen_bodylen = -1;
static int	gen_blksize = 8192;

/* generated body data */
static char	*gen_body = NULL;

#define GEN_WRITE(BUF, LEN) (retryWrite(ptcx, ptcx->sock, (BUF), (LEN)))

#define GEN_CONSTWRITE(BUF) GEN_WRITE(BUF, strlen(BUF))

#define DIE_UNLESS(X) do { if((X) < 0) \
{ \
	fprintf(stderr, "generateMessage: Error: `" #X "' failed: %s\n", \
		strerror(errno)); \
	return -1; \
} } while (0)

#define WRITE_CRLF DIE_UNLESS(GEN_CONSTWRITE(CRLF)); nwritten += strlen(CRLF)

/*
**  Prototypes
*/

void gen_init();
static int gen_write_body(ptcx_t ptcx, int ntowrite, struct checksum_t *cs);
static int gen_write_hdrs(ptcx_t ptcx, int nhdrs);
static int mime_separator(ptcx_t ptcx, int n, int term, struct checksum_t *cs);
static int mime_multipart_headers(ptcx_t ptcx, int n, struct checksum_t *cs);
static int mime_part_headers(ptcx_t ptcx, char *type, char *name,
			     struct checksum_t *cs);


/*
**  GEN_INIT -- initialize body data buffer.
*/

void
gen_init()
{
	int i;
	int glsize = strlen(GEN_LINE);
	int n = gen_blksize / glsize;
	gen_bodylen = n*glsize;
	gen_body = xalloc(gen_bodylen);

	for(i=0; i < n; i++)
		memcpy(gen_body + i*glsize,
		       GEN_LINE,
		       glsize);
}

/*
**  GEN_WRITE_BODY -- write ntowrite bytes of generated body text
*/

static int
gen_write_body(ptcx, ntowrite, cs)
	ptcx_t ptcx;
	int ntowrite;
	struct checksum_t *cs;
{
	int total = ntowrite;
	int nwritten;
	while(ntowrite > 0)
	{
		int sz = (ntowrite>gen_bodylen)?gen_bodylen:ntowrite;
		if((nwritten = GEN_WRITE(gen_body, sz)) < 0)
			break;
		ntowrite -= nwritten;
		if (cs != NULL)
			cs_data(cs, gen_body, nwritten);
	}
	return total - ntowrite;
}

/*
**  GEN_WRITE_HDRS -- write nhdrs generated headers
*/

static int
gen_write_hdrs(ptcx, nhdrs)
	ptcx_t ptcx;
	int nhdrs;
{
	int i;
	int nwritten;
	int total = 0;
	/* send extra headers */
	for(i=0 ; i < nhdrs; i++)
	{
#ifdef __AIX__
		char buf[128];
#else
		char buf[strlen(GEN_HDRFORMAT) + 5];
#endif /* AIX */
		int len = snprintf(buf, sizeof(buf), GEN_HDRFORMAT, i);
		
		if((nwritten = GEN_WRITE(buf, len)) < 0)
			break;
		total += nwritten;
	}
	return total;
}

/*
**  MIME_SEPARATOR -- write a mime separator.
*/

static int
mime_separator(ptcx, n, term, cs)
	ptcx_t ptcx;
	int n;
	int term;
	struct checksum_t *cs;
{
#ifdef __AIX__
	char buf[256];
#else
	char buf[strlen(MIME_TERMINATOR) + 8];
#endif
	int len;

	if (term)
		len = snprintf(buf, sizeof(buf), MIME_TERMINATOR, n);
	else
		len = snprintf(buf, sizeof(buf), MIME_SEPARATOR, n);

	DIE_UNLESS(GEN_WRITE(buf, len));
	if (cs != NULL)
		cs_data(cs, buf, len);
	return len;
}

/*
**  MIME_MULTIPART_HEADERS(ptcx, n, cs) -- write multipart header for depth n
**
**	Note: cs may be NULL if we're in the main rfc822 headers.
*/

static int
mime_multipart_headers(ptcx, n, cs)
	ptcx_t ptcx;
	int n;
	struct checksum_t *cs;
{
#ifdef __AIX__
	char buf[512];
#else
	char buf[strlen(MIME_MULTIPART_HDR) + 4];
#endif
	int len;
	len = snprintf(buf, sizeof(buf), MIME_MULTIPART_HDR, n);
	DIE_UNLESS(GEN_WRITE(buf, len));
	DIE_UNLESS(GEN_CONSTWRITE(CRLF));
	if (cs != NULL)
	{
		cs_data(cs, buf, len);
		cs_data(cs, CRLF, strlen(CRLF));
	}
	return len + strlen(CRLF);
}

/*
**  MIME_PART_HEADERS(ptcx, type, name) -- write part headers.
*/

static int
mime_part_headers(ptcx, type, name, cs)
	ptcx_t ptcx;
	char *type;
	char *name;
	struct checksum_t *cs;
{
#ifdef __AIX__
	char buf[512];
#else
	char buf[strlen(MIME_PART_HDR) + 128];
#endif
	int len;
	len = snprintf(buf, sizeof(buf), MIME_PART_HDR, type, name);
	DIE_UNLESS(GEN_WRITE(buf, len));
	DIE_UNLESS(GEN_CONSTWRITE(CRLF));

	if (cs != NULL)
	{
		cs_data(cs, buf, len);
		cs_data(cs, CRLF, strlen(CRLF));
	}
	return len + strlen(CRLF);
}

/*
**  GENERATE_MESSAGE -- generate a message according to pish
*/

int
generateMessage(ptcx_t ptcx, pish_command_t *pish)
{
	static int mime_multi_overhead = -1,
		mime_single_overhead = -1;
	int nwritten;
	int msgsize;
	int nhdrs;
	int mime;
	int ntowrite;
	int i;
	struct checksum_t *cs = NULL;
	char *cs_rep = NULL;

	assert(ptcx != NULL);
	assert(pish != NULL);

	/* Calculate message size: */
	msgsize = sample(pish->genSize);

	/* calculate other message parameters: */
	nhdrs = sample(pish->genHdrs);
	mime = sample(pish->genMime);
	
	ntowrite = msgsize;

	assert(msgsize >= 0);
	/* send default headers */
#ifdef GEN_MSGIDS
	{
		/* XXX: not threadsafe, but shouldn't matter */
		static long msgnum = 0;
		int n;
# ifdef __AIX__
		/*
		**  Stop for a moment to loathe xlc_r, which cannot
		**  figure out static string lengths at compile time.
		*/
		char buf[512];
# else  /* __AIX__ */
		char buf[strlen(GEN_HDRS)+128];
# endif /* __AIX__ */
		struct tm now;
		time_t tnow;
		char timebuf[64];
		tnow = time(0);
		(void) gmtime_r(&tnow, &now);
		strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T -0000",
			 &now);
		n = snprintf(buf, sizeof(buf), GEN_HDRS,
			     ptcx->processnum,
			     ptcx->threadnum,
			     msgnum++,
			     timebuf);
		DIE_UNLESS(nwritten = GEN_WRITE(buf, n));
	}
#else  /* GEN_MSGIDS */
	DIE_UNLESS(nwritten = GEN_CONSTWRITE(GEN_HDRS));
#endif /* GEN_MSGIDS */
	
#ifdef GEN_HDR_SIZE_COUNTS
	ntowrite -= nwritten;
#endif /* GEN_HDR_SIZE_COUNTS */

	nhdrs -= N_GEN_HDRS;

	/* send additional headers, if necessary */
	if(nhdrs > 0)
	{
		DIE_UNLESS(nwritten = gen_write_hdrs(ptcx, nhdrs));
#ifdef GEN_HDR_SIZE_COUNTS
		ntowrite -= nwritten;
#endif
	}

	/* figure out how many mime parts we can create */
	/* (our message might be too small to have any mime parts) */
	if (mime > 0)
	{
		/* approximate overhead */
		if (mime_multi_overhead < 0)
		{
			mime_multi_overhead = strlen(MIME_SEPARATOR)
				+ strlen(MIME_MULTIPART_HDR)
				+ strlen(MIME_WARNING)
				+ strlen(MIME_TERMINATOR);
			
			mime_single_overhead = strlen(MIME_SEPARATOR)
				+ strlen(MIME_PART_HDR);
		}

		if ((ntowrite - mime_multi_overhead * mime
		     < mime_single_overhead))
		{
			mime = (ntowrite - mime_single_overhead)
				/ mime_multi_overhead;
			assert(mime >= 0);
		}
	}
	
	/*
	**  Checksums are taken over the body, from after the blank
	**  line following the headers to just before the CRLF.CRLF.
	*/

	if (mime > 0)
	{
		int bodylen = ntowrite
			- (mime - 1) * mime_multi_overhead
			- mime_single_overhead
			- strlen(MIME_TERMINATOR);

		for (i = 0; i < mime - 1; i++)
		{
			/* write multipart header for i */
			DIE_UNLESS(nwritten = \
				   mime_multipart_headers(ptcx, i, cs));
			ntowrite -= nwritten;

#ifdef GEN_CHECKSUM
			if (i == 0
			    && pish->genChecksum != CS_NONE
			    && (cs = cs_begin(CS_MD5)) == NULL)
				fprintf(stderr, "generateMessage: can't initialize checksum\n");
#endif /* GEN_CHECKSUM */
			DIE_UNLESS(nwritten = GEN_CONSTWRITE(MIME_WARNING));
			ntowrite -= nwritten;
			if (cs != NULL)
				cs_data(cs, MIME_WARNING,
					strlen(MIME_WARNING));

			/* write boundary for i */
			DIE_UNLESS(nwritten = \
				   mime_separator(ptcx, i, 0, cs));
			ntowrite -= nwritten;
		}
		
		/* write single-part body */
		
		if (mime > 1)
			DIE_UNLESS(nwritten = \
				   mime_part_headers(ptcx, \
						     MIME_TYPE, \
						     MIME_FILE, cs));
		else
			DIE_UNLESS(nwritten = \
				   mime_part_headers(ptcx, \
						     MIME_TYPE, \
						     MIME_FILE, NULL));

		ntowrite -= nwritten;
		
		DIE_UNLESS(nwritten = \
			   gen_write_body(ptcx, bodylen, cs));
		ntowrite -= nwritten;

		/* write mime terminators for all sections */
		for (i = mime - 1; i >= 0; i--)
		{
			DIE_UNLESS(nwritten = mime_separator(ptcx, i, 1, cs));
			ntowrite -= nwritten;
		}
	}
	else
	{
		/* write end of headers */
		DIE_UNLESS(nwritten = GEN_CONSTWRITE(CRLF));
#ifdef GEN_HDR_SIZE_COUNTS
		ntowrite -= nwritten;
#endif
#ifdef GEN_CHECKSUM
		if (pish->genChecksum != CS_NONE
		    && (cs = cs_begin(CS_MD5)) == NULL)
			fprintf(stderr,
				"generateMessage: can't initialize checksum\n");
#endif /* GEN_CHECKSUM */
		/* send non-MIME body */		
		if(ntowrite > 0)
		{
			DIE_UNLESS(nwritten = \
				   gen_write_body(ptcx, ntowrite, cs));
			ntowrite -= nwritten;
		}
	}
	/* done sending body.  Checksum then EOM. */
	if (cs != NULL)
	{
		/* add in crlf at start of CS_MARKER */
		cs_data(cs, CRLF, strlen(CRLF));
		cs_rep = cs_end(cs);
		DIE_UNLESS(GEN_WRITE(cs_rep, strlen(cs_rep)));
		xfree(cs_rep);
		xfree(cs);
	}

	DIE_UNLESS(GEN_CONSTWRITE(EOM));
	ntowrite -= strlen(EOM);
	/* update stats */
	if(ntowrite > 0)
		fprintf(stderr,
			"generateMessage: notice: generated %d/%d bytes\n",
			msgsize-ntowrite, msgsize);
	msgsize -= ntowrite;
	
	if(msgsize <= 0)
		fprintf(stderr,
			"generateMessage: notice: msgsize = %d\n", msgsize);
		
	ptcx->byteswritten += msgsize;
	return msgsize;
}

#endif /* AUTOGEN */
