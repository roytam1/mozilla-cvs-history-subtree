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

#ifndef _constants_h
#define _constants_h

/*
**  LIMITS
*/

#define MAX_ACCEPT_SECS	    180		/* maximum time master will wait for listen() */
#define LINE_BUFSIZE	    4096
#define MAX_USERNAME_LEN 32
#define MAX_MAILADDR_LEN 64
#define MAX_COMMAND_LEN 1024
#define MAX_RESPONSE_LEN 1024
#define MAX_ERRORMSG_LEN 256
#define DATESTAMP_LEN 40

#define MAX_IMAP_FOLDERNAME_LEN 256
#define MAX_SEARCH_PATTERN_LEN 256

#define MAX_HTTP_COMMAND_LEN 1024

/* TODO make these dynamic.  For now just use big buffers */
#define SIZEOF_EVENTTEXT    150		/* report text from a single timer */
#define SIZEOF_RQSTSTATSTEXT 2048	/* report text from all timers */
#define SIZEOF_SUMMARYTEXT 8192		/* report text from all protocols */

/*
**  PORTS
*/

#define SMTP_PORT 	25
#define POP3_PORT 	110
#define IMAP4_PORT 	143
#define HTTP_PORT 	80
#define WMAP_PORT 	80
#define WEBMAIL_PORT 	1066

/*
**  time-related
*/
#define USECINSEC	    1000000
#define MSECINSEC	    1000

#define SECS_2_USECS(x)	    ((x) * USECINSEC)
#define USECS_2_SECS(x)	    ((x) / USECINSEC)

/*
**  Miscellany
*/

#define CRLF "\r\n"

#endif /* _constants_h */
