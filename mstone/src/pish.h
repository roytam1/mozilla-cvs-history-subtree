/* -*- Mode: C; c-file-style: "stroustrup"; comment-column: 40 -*- */
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
 * Contributor(s):	Dan Christian <robodan@netscape.com>
 *			Marcel DePaolis <marcel@netcape.com>
 *			Sean O'Rourke <sean@sendmail.com>
 *			Thom O'Connor <thom@sendmail.com>
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

#ifndef _pish_h
#define _pish_h
/* these are protocol dependent timers for Pop, Imap, Smtp, Http*/
typedef struct pish_stats {
    event_timer_t	connect;
    event_timer_t	banner;
    event_timer_t	login;
    event_timer_t	cmd;
    event_timer_t	msgread;
    event_timer_t	msgwrite;
    event_timer_t	logout;

    /* protocol dependent local storage */
					/* should have local ranges too */
    range_t		loginRange;	/* login range for this thread */
    unsigned long	lastLogin;
    range_t		domainRange;	/* domain range for this thread */
    unsigned long	lastDomain;
    range_t		addressRange;	/* address range for this thread */
    unsigned long	lastAddress;
} pish_stats_t;

/* These are common to POP, IMAP, SMTP, HTTP */
typedef struct pish_command {
    resolved_addr_t 	hostInfo;	/* should be a read only cache */

    /* These are common to SMTP, POP, IMAP */
    char *	loginFormat;
    range_t	loginRange;		/* login range for all threads */

    range_t	domainRange;		/* domain range for all threads */

    char *	passwdFormat;

    int         net_timeout;            /* timeout for network ops */

    long	flags;			/* protocol specific flags */

    /* SMTP command attrs */
    char *	addressFormat;
    range_t	addressRange;		/* address range for all threads */

    char *	smtpMailFrom;		/* default from address */
    char *	filePattern;		/* filename pattern */
    int		fileCount;		/* number of files */
    void *	files;			/* array of file info */
    char *      filename;
    dinst_t   *numRecipients; /* recpients per message */
  #ifdef AUTOGEN
    /* Sean O'Rourke: added auto-generation */
    dinst_t   *genSize;       /* message size */
    dinst_t   *genHdrs;       /* total number of headers */
    dinst_t   *genMime;       /* 0 => text/plain, else N-part MIME */

    enum {                    /* what to do with checksums */
      CS_NONE,                /* don't do checksums */
      CS_CHECK,               /* report discrepancies */
      CS_SAVE_BAD             /* save messages which fail */
    }                 genChecksum;
  #endif
  #ifdef SOCK_LINESPEED         /* bandwidth-limitation */
    dinst_t   *latency;
    dinst_t   *bandwidth;
  #endif
  #ifdef SOCK_SSL                       /* SSL/TLS support */
    char      *cert;
    char      *key;
    int       useTLS;
    int       sslTunnel;
  #endif
    int         msgsize;        /* message size without trailing CRLF.CRLF */
    char *      msgdata;        /* cache the file in mem */

    /* POP/IMAP flag to leave mail on server */
    dinst_t     *leaveMailOnServerDist;

    /* time to read each message */
    dinst_t   *msgReadTime;

    /* Multi-POP vars */
    double    percentActive;          /* users concurrently active */
    dinst_t   *userSpacing;           /* time alloted for each connection */
    dinst_t   *dropRate;              /* fraction of connections dropped */

    /* IMAP command attrs */
    char *	imapSearchFolder;
    char *	imapSearchPattern;
    int 	imapSearchRate;

    /* Webmail stuff */
    char              *webBase;          /* base for all web url's */
    string_list_t     *webLoginPages;    /* sequence of pages after login */
    /* login format, given to printf(fmt, username, passwd) */
    char              *webLoginFormat;
    /* format to read message, given to printf(fmt, msgnum) */
    char              *webMsgRead;
    /* delete format, given to printf(fmt, list_of_msgs) */
    char              *webDeleteFormat;
    char              *webExpunge;       /* expunge URL */
    char              *webLogout;        /* logout page */
    char              *webIndex;         /* index page */
} pish_command_t;

					/* TRANSITION functions */
extern int pishParseNameValue (pmail_command_t cmd, char *name, char *tok);

#endif /* _pish_h */
