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
 * Copyright (C) 1997-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s):	Dan Christian <robodan@netscape.com>
 *			Marcel DePaolis <marcel@netcape.com>
 *			Mike Blakely
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
/* Protocol test for IMAP4 */

#include "bench.h"
#include "pish.h"
#include "idle.h"
#include "socket.h"
#include "xalloc.h"
#include "checksum.h"

typedef struct IMAP_t {
  int	seq_num;	/* IMAP command seq number */
  char	resp_buffer[MAX_RESPONSE_LEN];
  char	selected_folder[MAX_IMAP_FOLDERNAME_LEN];
} IMAP;

/* protos */
#if 0
static int retrImapMsg(ptcx_t ptcx, SOCKET sock, int seqNum, char *buffer, int msgSize, int maxBytes);
#else
static int retrImapMsg(ptcx_t ptcx, SOCKET sock, int seqNum, int msgSize);
#endif /* 0 */
static int readImapResponse(ptcx_t ptcx, SOCKET sock, int seqNum, char *buffer, int buflen);
static int doImapCommandResponse(ptcx_t ptcx, SOCKET sock, int SeqNum, char *cmdand, char *response, int resplen);

static int imapLogin(ptcx_t ptcx, IMAP *, SOCKET, mail_command_t *, cmd_stats_t *ptimer);
static int imapLogout(ptcx_t ptcx, IMAP *, SOCKET);
static int imapCheckINBOX(ptcx_t ptcx, IMAP *, SOCKET, mail_command_t *, cmd_stats_t *);
static int imapSelectFolder(ptcx_t ptcx, IMAP *, SOCKET, char *, cmd_stats_t *);
static int imapCloseFolder(ptcx_t ptcx, IMAP *, SOCKET, cmd_stats_t *);
static int imapParseSelectResponse(ptcx_t ptcx, IMAP *, char *, int *);
static int imapRetrRecentMessages(ptcx_t ptcx, IMAP *, SOCKET, mail_command_t *, cmd_stats_t *, int, int);
static int imapComputeSearchSchedule(ptcx_t ptcx, mail_command_t *, int **);
static int imapSearchFolder(ptcx_t ptcx, IMAP *, SOCKET, mail_command_t *, cmd_stats_t *);
#if 0
static int imapCreateFolder(ptcx_t ptcx, IMAP *pIMAP, const char *folder);
#endif
static int imapDeleteFolder(ptcx_t ptcx, IMAP *pIMAP, const char *folder);

typedef struct _doIMAP4_state {
    IMAP	IMAP_state;
    IMAP	*pIMAP;
    unsigned int timeUntilSearch;
    int		currentSearch;
    int		numSearches;
    int		*searchSchedule;
} doIMAP4_state_t;

/* flags definitions */
#define leaveMailOnServer 0x01
#define leaveMailUnseen   0x02

static void doImap4Exit (ptcx_t ptcx, doIMAP4_state_t *me);


static int
ImapParseNameValue (pmail_command_t cmd,
		    char *name,
		    char *tok)
{
    pish_command_t	*pish = (pish_command_t *)cmd->data;

    /* find a home for the attr/value */
    if (pishParseNameValue(cmd, name, tok) == 0)
        ;                               /* done */
    else if (strcmp(name, "leavemailonserver") == 0) {
	if (atoi(tok) > 0) {
            pish->flags |= leaveMailOnServer;
	    pish->leaveMailOnServerDist = 
	        parse_distrib(tok, (value_parser_t)&atof);
        } else  { /* turn on if < leavemailunseen */
            pish->flags &= ~leaveMailOnServer;
        /* D_PRINTF (stderr, "leaveMailOnServer=%d\n", pish->leaveMailOnServer);*/           
        }
    }   
    else if (strcmp(name, "leavemailunseen") == 0) {
        if (atoi(tok) > 0) {
            /* leaving mail unseen implies leaving on server */
            pish->flags |= leaveMailUnseen | leaveMailOnServer;
        } else  { /* turn on if < leavemailunseen */
            pish->flags &= ~leaveMailUnseen;
        }
        /*D_PRINTF (stderr, "leaveMailOnServer=%d\n", pish->leaveMailOnServer);*/   
    }
    else {
        return -1;
    } 
    return 0;
}

/*
  Set defaults in command structure
*/
int
Imap4ParseStart (pmail_command_t cmd,
		char *line,
		param_list_t *defparm)
{
    param_list_t	*pp;
    pish_command_t	*pish = XCALLOC(pish_command_t);

    cmd->data = pish;

    pish->hostInfo.portNum = IMAP4_PORT; /* get default port */

    D_PRINTF(stderr, "Imap4 Assign defaults\n");
    /* Fill in defaults first, ignore defaults we dont use */
    for (pp = defparm; pp; pp = pp->next) {
	(void)ImapParseNameValue (cmd, pp->name, pp->value);
    }

    return 1;
}

/*
  Fill in structure from a list of lines
 */
int
Imap4ParseEnd (pmail_command_t cmd,
		string_list_t *section,
		param_list_t *defparm)
{
    string_list_t	*sp;
    pish_command_t	*pish = (pish_command_t *)cmd->data;

    /* Now parse section lines */
    D_PRINTF(stderr, "Imap4 Assign section lines\n");
					/* skip first and last */
    for (sp = section->next; sp->next; sp = sp->next) {
	char *name = sp->value;
	char *tok = name + strcspn(name, " \t=");
	*tok++ = 0;			/* split name off */
	tok += strspn(tok, " \t=");

	string_tolower(name);
	tok = string_unquote(tok);

	if (ImapParseNameValue (cmd, name, tok) < 0) {
	    /* not a known attr */
	    D_PRINTF(stderr,"notice: ignoring unknown attribute '%s' '%s'\n",
		     name, tok);
	    returnerr(stderr,"notice: ignoring unknown attribute '%s' '%s'\n",
		      name, tok);
	}	
    }

    /* check for some of the required command attrs */
    if (!pish->hostInfo.hostName) {
	D_PRINTF(stderr,"missing server for command");
	return returnerr(stderr,"missing server for command\n");
    }

    if (!pish->loginFormat) {
	D_PRINTF(stderr,"missing loginFormat for command");
	return returnerr(stderr,"missing loginFormat for command\n");
    }

    if (!pish->passwdFormat) {
	D_PRINTF(stderr,"missing passwdFormat for command");
	return returnerr(stderr,"missing passwdFormat for command\n");
    }

    /* see if we can resolve the mailserver addr */
    if (resolve_addrs(pish->hostInfo.hostName, "tcp",
		      &(pish->hostInfo.host_phe),
		      &(pish->hostInfo.host_ppe),
		      &(pish->hostInfo.host_addr),
		      &(pish->hostInfo.host_type))) {
	return returnerr (stderr, "Error resolving hostname '%s'\n",
			  pish->hostInfo.hostName);
    } else {
	pish->hostInfo.resolved = 1;	/* mark the hostInfo resolved */
    }
    rangeSetFirstCount (&pish->loginRange, pish->loginRange.first,
			  pish->loginRange.span, pish->loginRange.sequential);
    rangeSetFirstCount (&pish->domainRange, pish->domainRange.first,
			  pish->domainRange.span, pish->domainRange.sequential);

    return 1;
}

/*
 * imap4 entry point
 *
 */
void *
doImap4Start(ptcx_t ptcx, mail_command_t *cmd, cmd_stats_t *ptimer)
{
    doIMAP4_state_t	*me = XCALLOC(doIMAP4_state_t);
    pish_command_t	*pish = (pish_command_t *)cmd->data;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;
    int	rc;
    
    if (!me) return NULL;
    me->pIMAP = &me->IMAP_state;

    me->timeUntilSearch = 0;
    me->currentSearch = 0;
    me->numSearches = 0;
    me->searchSchedule = NULL;

    me->pIMAP->seq_num = 1;

    ptcx->net_timeout = pish->net_timeout;
    
    event_start(ptcx, &stats->connect);
    ptcx->sock = connectSocket(ptcx, &pish->hostInfo, "tcp");
    event_stop(ptcx, &stats->connect);
    if (BADSOCKET(ptcx->sock)) {
	if (gf_timeexpired < EXIT_FAST) {
	    stats->connect.errs++;
	    returnerr(debugfile, "IMAP4 Couldn't connect to %s: %s\n",
		      pish->hostInfo.hostName, neterrstr());
	}
	xfree (me);
	return NULL;
    }

    if (gf_abortive_close) {
	if (set_abortive_close(ptcx->sock) != 0) {
	    returnerr (debugfile, "IMAP: WARNING: Could not set abortive close\n");
	}
    }

    /* Sean O'Rourke: add socket "flavors" */
#ifdef SOCK_SSL
    if (pish->sslTunnel) {
	SSL_INIT(ptcx->sock, pish);
	if (BADSOCKET(ptcx->sock)) {
	    returnerr(debugfile, "IMAP ERROR: Could not start SSL tunneling\n");
	    stats->connect.errs++;
	    xfree (me);
	    return NULL;
	}
    }
#endif /* SOCK_SSL */
    
    LS_INIT(ptcx->sock, pish);
    if (BADSOCKET(ptcx->sock)) {
	returnerr(debugfile, "IMAP ERROR: can't initialize socket.\n");
	stats->connect.errs++;		/* (sort of) */
	xfree (me);
	return NULL;
    }
    
    /* READ connect response from server */
    event_start(ptcx, &stats->banner);
    rc = readResponse(ptcx, ptcx->sock, me->pIMAP->resp_buffer, sizeof(me->pIMAP->resp_buffer));
    event_stop(ptcx, &stats->banner);
    if (rc <= 0) {
	if (gf_timeexpired < EXIT_FAST) {
	    stats->banner.errs++;
	    returnerr(debugfile, "IMAP4 Error reading banner: %s\n",
		      neterrstr());
	}
	doImap4Exit (ptcx, me);
	return NULL;
    }

#ifdef SOCK_SSL
    if (!pish->useTLS)
	goto end_tls;
    
    /*
    **  Try to do STARTTLS
    */

    event_start(ptcx, &stats->cmd);
    rc = doImapCommandResponse(ptcx, ptcx->sock, ++me->pIMAP->seq_num,
			       "CAPABILITY" CRLF,
			       me->pIMAP->resp_buffer,
			       sizeof(me->pIMAP->resp_buffer));
    event_stop(ptcx, &stats->cmd);
    if (rc < 0) {
	stats->cmd.errs++;
	goto end_tls;
    }
    if (strstr(me->pIMAP->resp_buffer, "STARTTLS")) {
	SOCKET s = BADSOCKET_VALUE;
	D_PRINTF(debugfile, "Trying STARTTLS\n");
	if (doImapCommandResponse(ptcx, ptcx->sock, ++me->pIMAP->seq_num,
				  "STARTTLS" CRLF,
				  me->pIMAP->resp_buffer,
				  sizeof(me->pIMAP->resp_buffer)) >= 0) {
	    char *p = me->pIMAP->resp_buffer;
	    while (*p && !isspace(*p++))
		;
	    if (strncmp(p, "OK", 2) == 0) {
		s = ptcx->sock;
		SSL_INIT(s, pish);
	    }
	}
	if (BADSOCKET(s)) {
	    D_PRINTF(stderr, "Can't use TLS\n");
	} else {
	    ptcx->sock = s;
	}
    }
 end_tls:
    
#endif /* SOCK_SSL */
    
    rc = imapLogin(ptcx, me->pIMAP, ptcx->sock, cmd, ptimer);
    if (rc != 0) {
	doImap4Exit (ptcx, me);
	return NULL;
    }

    me->pIMAP->selected_folder[0] = '\0';

    /* compute the search intervals  */
    me->searchSchedule = 0;
    me->currentSearch = me->numSearches = 0;
    if (pish->imapSearchRate) {
	me->numSearches = imapComputeSearchSchedule(ptcx, cmd, &me->searchSchedule);
  	me->timeUntilSearch = me->searchSchedule[me->currentSearch];
    }

    D_PRINTF(debugfile,"computed search schedule\n");
    
    return me;
}

int
doImap4Loop(ptcx_t ptcx,
	    mail_command_t *cmd,
	    cmd_stats_t *ptimer,
	    void *mystate)
{
    doIMAP4_state_t	*me = (doIMAP4_state_t *)mystate;

    if (!me) return -1;			/* should never happen */

    /* Enter command loop, we will be here for numLoops */

    /* select INBOX, check for mail, fetching
       and (optionally) deleting RECENT msgs */
    if (imapCheckINBOX(ptcx, me->pIMAP, ptcx->sock, cmd, ptimer)) {
	return -1;			/* signal to logout, clean up */
    }

    if (me->searchSchedule) {		/* check if it's time to search */
	me->timeUntilSearch -= sample(cmd->loopDelay);

	if (me->timeUntilSearch <= 0) {
	    imapSearchFolder(ptcx, me->pIMAP, ptcx->sock, cmd, ptimer);
	    me->currentSearch++;
	    me->timeUntilSearch = me->searchSchedule[me->currentSearch];
	}
    }

    return 0;
}

void
doImap4End(ptcx_t ptcx, mail_command_t *cmd, cmd_stats_t *ptimer, void *mystate)
{
    doIMAP4_state_t	*me = (doIMAP4_state_t *)mystate;
    int rc;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;

    if (!me) return;
    if (BADSOCKET(ptcx->sock)) return;	/* closed by previous error */

    /* Sean O'Rourke: add ramp-down time: */

#ifdef IMAP_RAMPDOWN
    while(gf_timeexpired >= EXIT_SOON) {
	int mystoptime = gt_shutdowntime
	    + ptcx->threadnum*gt_stopinterval*EXIT_FAST / gn_numthreads;
	int secsleft = mystoptime - time(0L);
	if(secsleft > 0)
	{
#ifdef PROB_RAMPDOWN
	    extern value_parser_t d_millitime_atoi;
	    char dist[64];
	    int sleepms;
	    dinst_t* unif;
	    sprintf(dist, "~unif(0, %ds)", secsleft);
	    unif = parse_distrib(dist, d_millitime_atoi);
	    sleepms = sample(unif);
	    MS_usleep(1000*sleepms);
#else /* PROB_RAMPDOWN */
	    /* First threads started are first to terminate: */
	    MS_sleep((int)(secsleft));
#endif /* PROB_RAMPDOWN */	    
	}
	else
	{
	    break;
	}
    }
#endif /* IMAP_RAMPDOWN */
    
    /* close the folder */
    rc = imapCloseFolder(ptcx, me->pIMAP, ptcx->sock, ptimer);
    if (rc != 0) {
	doImap4Exit (ptcx, me);
	return;
    }

    /* if we created a mailstone folder, delete it now */
    if (gf_imapForceUniqueness) {	/* TESTED??? */
	char folder[DATESTAMP_LEN];

	sprintf(folder, "ms%s", gs_dateStamp);
	rc = imapDeleteFolder(ptcx, me->pIMAP, folder);
	if (rc == -1) {
	    doImap4Exit (ptcx, me);
	    return;
	}
    }

    event_start(ptcx, &stats->logout);
    imapLogout(ptcx, me->pIMAP, ptcx->sock);
    event_stop(ptcx, &stats->logout);

    doImap4Exit(ptcx, me);			/* clean up */
}

/* shutdown the connection hard */
/* free memory */
void
doImap4Exit (ptcx_t ptcx, doIMAP4_state_t *me)
{
  if (!BADSOCKET(ptcx->sock))
    NETCLOSE(ptcx->sock);
  ptcx->sock = BADSOCKET_VALUE;

  if (me->searchSchedule)
    xfree(me->searchSchedule);

  xfree (me);
}  

static int
readImapResponse(ptcx_t ptcx, SOCKET sock, int seqNum, char *buffer, int buflen)
{    
    /* read the server response and do nothing with it */
    int totalbytesread = 0;
    int bytesread;
    char markerSingleLine[20];
    char markerMultiLine[20];

    /* marker to tell us when we've recieved the final line from server */
    sprintf(markerSingleLine, "%d ", seqNum);
    sprintf(markerMultiLine, "\n%d ", seqNum);

    while (totalbytesread < buflen)
    {
	if (gf_timeexpired >= EXIT_FAST) {
	    D_PRINTF(debugfile,"Time expired while reading messages - in readIMAPresponse\n");
	    break;
	}
        if ((bytesread = retryRead(ptcx, sock, buffer+totalbytesread,
				   buflen-totalbytesread)) <= 0) {
	    if (gf_timeexpired < EXIT_FAST) {
		returnerr(stderr, "readImapResponse(%d, %d) %s\n",
			  sock, seqNum, neterrstr());
	    }
            return -1;
	}

        totalbytesread += bytesread;
	ptcx->bytesread += bytesread;

	buffer[totalbytesread] = 0;
	/* search for end of response */
	if (strstr(buffer, markerSingleLine)) {
	    break;
	}
	if (strstr(buffer, markerMultiLine)) {
	    break;
	}
    }
    D_PRINTF(debugfile, "Read from server: %s\n", buffer );

    return totalbytesread;
}

/* read from socket until we find <CRLF>.<CRLF> */
static int
#if 0
retrImapMsg(ptcx_t ptcx, SOCKET sock, int seqNum,
	    char *buffer, int msgSize, int maxBytes)
#else
retrImapMsg(ptcx_t ptcx, SOCKET sock, int seqNum, int msgSize)
#endif
{    
    int totalbytesread = 0;
    int bytesread;
    int maxBytes = msgSize + 1024;	/* old semantics */
    char markerSingleLine[20];
    char markerMultiLine[20];
    char buf[4096];
    char *bpos = buf;

    /* marker to tell us when we've recieved the final line from server */
    sprintf(markerSingleLine, "%d ", seqNum);
    sprintf(markerMultiLine, "\n%d ", seqNum);

    while (totalbytesread < maxBytes)
    {
	if (gf_timeexpired >= EXIT_FAST) {
		D_PRINTF(debugfile,"Time expired while reading messages - in retrimap4Msg\n");
		break;
	}

	bytesread = maxBytes - totalbytesread;
	if (bytesread > sizeof(buf) - (1 + bpos - buf))
	    bytesread = sizeof(buf) - (1 + bpos - buf);
        bytesread = retryRead(ptcx, sock, bpos, bytesread);
        if (bytesread <= 0) {
	    if (gf_timeexpired < EXIT_FAST) {
		returnerr(debugfile, "retrImapMsg(%d) %s\n", sock, neterrstr());
	    }
	    return -1;
	}
	
        totalbytesread += bytesread;

	bpos[bytesread] = 0;

	if (totalbytesread > msgSize) {
	    /* search for end of response */
	    if (strstr(buf, markerSingleLine)) {
		break;
	    }
	    if (strstr(buf, markerMultiLine)) {
		break;
	    }
	}
	if (gf_timeexpired >= EXIT_FAST) {
		D_PRINTF(debugfile,"Time expired while reading messages - in retrIMAP\n");
		break;
	}
	/* shift message over in buffer */
	memcpy(buf, bpos + bytesread - sizeof(markerMultiLine),
	       sizeof(markerMultiLine));
	bpos = buf + sizeof(markerMultiLine);
    }

    ptcx->bytesread += totalbytesread;

    return totalbytesread;
}

static int
doImapCommandResponse(ptcx_t ptcx, SOCKET sock, int seqnum,
		      char *command, char *response, int resplen)
{
    int ret;

    if (response == NULL)
	return -1;
    memset(response, 0, resplen);

    /* send the IMAP command already formatted */
    T_PRINTF(ptcx->logfile, command, strlen (command), "IMAP4 SendCommand");
    if ((ret = sendCommand(ptcx, sock, command)) == -1) {
	if (gf_timeexpired < EXIT_FAST) {
	    trimEndWhite (command);
	    returnerr(debugfile, "Error sending [%s] command to server: %s\n",
		      command, neterrstr());
	}
	return -1;
    }

    /* read server response for this sequence number */
    if ((ret = readImapResponse(ptcx, sock, seqnum, response, resplen)) <= 0) {
	if (gf_timeexpired < EXIT_FAST) {
	    trimEndWhite (command);
	    returnerr(debugfile, "Error reading [%s] response: %s\n",
		      command, neterrstr());
	}
	return -1;
    }
    T_PRINTF(ptcx->logfile, response, strlen(response),
	     "IMAP4 ReadResponse");	/* telemetry log. should be lower level */
    return ret;
}

static int
imapLogin(ptcx_t ptcx,
	  IMAP *pIMAP,
	  SOCKET sock,
	  mail_command_t *cmd,
	  cmd_stats_t *ptimer)
{
    unsigned long next_login;
    unsigned long next_domain;
    char mailUser[MAX_MAILADDR_LEN];
    char userPasswd[MAX_USERNAME_LEN];
    char command[MAX_COMMAND_LEN];
    int done = 0;
    int ret;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;
    pish_command_t	*pish = (pish_command_t *)cmd->data;

    gf_imapForceUniqueness = 0;

    while (!done) {
#if defined(IMAP_CAPABILITY) && !defined(SOCK_SSL)
	/*
	**  don't do this if we tried STARTTLS above, since we already
	**  did a CAPABILITY there...
	*/
	sprintf(command, "%d CAPABILITY" CRLF, ++pIMAP->seq_num);
	event_start(ptcx, &stats->cmd);
	ret = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				    command, pIMAP->resp_buffer,
				    sizeof(pIMAP->resp_buffer));
	event_stop(ptcx, &stats->cmd);
	if (ret == -1) {
	    if (gf_timeexpired < EXIT_FAST) {
		stats->cmd.errs++;
	    }
	    return -1;
	}	
#endif /* IMAP_CAPABILITY && ! SOCK_SSL */

	next_domain = rangeNext(&stats->domainRange, stats->lastDomain);
	stats->lastDomain = next_domain;
        next_login = rangeNext(&stats->loginRange, stats->lastLogin);
	stats->lastLogin = next_login;

	sprintf(mailUser, pish->loginFormat, next_login, next_domain);
	sprintf(userPasswd, pish->passwdFormat, next_login);
	D_PRINTF(debugfile,"mailUser=%s, passwd=%s\n", mailUser, userPasswd);

	/* LOGIN */
	sprintf(command, "%d LOGIN %s %s%s", ++pIMAP->seq_num, mailUser, 
		userPasswd, CRLF);
	event_start(ptcx, &stats->login);
	ret = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				  command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	event_stop(ptcx, &stats->login);
	if (ret == -1) {
	    if (gf_timeexpired < EXIT_FAST) {
		stats->login.errs++;
	    }
	    return -1;
	}

	done = 1;

#if 0
	if (gf_imapForceUniqueness) {
	    char *bufPtr = 0;
	    int mailboxInUse = 0;
	    char listResponse[MAX_RESPONSE_LEN];
	    char folderDate[DATESTAMP_LEN];
	    char folder[DATESTAMP_LEN];

	    /* look for a mailstone folder, telling us whether a client
	       has already logged into this mailbox */
	    sprintf(command, "%d LIST \"\" *%s", ++pIMAP->seq_num, CRLF);
	    if (doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				      command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer)) == -1) {
		return -1;
	    }

	    strcpy(listResponse, pIMAP->resp_buffer);
	    bufPtr = listResponse;
	
	    memset(folderDate, 0, DATESTAMP_LEN*sizeof(char));
	
	    mailboxInUse = 0;
	    while (*bufPtr) {
		if (sscanf(bufPtr, "* LIST () \"/\" ms%s", folderDate)) {
		    D_PRINTF(debugfile,"dateStamp: %s\n", gs_dateStamp);
		    D_PRINTF(debugfile,"folderDate: %s\n", folderDate);
		    /* compare the folderDate with dateStamp, if the same,
		       then another mailstone client is logged in, try another
		       mailbox */
		    if (strcmp(folderDate, gs_dateStamp) == 0) {
			D_PRINTF(debugfile,"YES\n");
			mailboxInUse = 1;	
			break;
		    } else {
			D_PRINTF(debugfile,"NO\n");
			/* it's an old mailstone folder, remove it */
			sprintf(folder, "ms%s", folderDate);
			if (deleteFolder(ptcx, pIMAP, folder) == -1) {
			    return -1;
			}
		    }
		} 
	
		bufPtr = strchr(bufPtr, '\n');
		if (bufPtr) bufPtr++;
	    }

	    if (mailboxInUse)
		continue;

	    /* mailbox not in use, let's create a ms<gs_dateStamp> folder */
	    sprintf(folder, "ms%s", gs_dateStamp);
	    if (createFolder(ptcx, pIMAP, folder) == -1) {
		return -1;
	    }

	    done = 1;
	} /* if (gf_imapForceUniqueness) */
#endif
    }
    D_PRINTF(debugfile,"Done with login\n");
    return 0;
}

static int
imapLogout(ptcx_t ptcx, IMAP *pIMAP, SOCKET sock)
{
  char command[MAX_COMMAND_LEN];

  /* LOGOUT (closes the connection) */
  sprintf(command, "%d LOGOUT%s", ++pIMAP->seq_num, CRLF);
  if (doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
			    command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer)) == -1) {
      return -1;
  }
  return 0;
}

static int
imapCheckINBOX(ptcx_t ptcx, IMAP *pIMAP, SOCKET sock, mail_command_t *cmd, cmd_stats_t *timer)
{
  int numExists = 0;
  int numRecent = 0;

  /* SELECT INBOX */ 
  if (imapSelectFolder(ptcx, pIMAP, sock, "INBOX", timer) < 0) {
      return -1;
  }

  /* parse number of existing/recent msgs out of buffer */
  if (imapParseSelectResponse(ptcx, pIMAP, "EXISTS", &numExists) == -1)
      return -1;
  if (imapParseSelectResponse(ptcx, pIMAP, "RECENT", &numRecent) == -1)
      return -1;

  D_PRINTF(debugfile,"SELECT INBOX shows %d recent msgs of %d existing msgs\n",
	   numRecent, numExists);

  /* FETCH messages */
  if (imapRetrRecentMessages(ptcx, pIMAP,
			     sock, cmd, timer, numExists, numRecent))
	return -1;

  return 0;
}


static int
imapSelectFolder(ptcx_t ptcx,
		 IMAP *pIMAP,
		 SOCKET sock,
		 char *folder,
		 cmd_stats_t *ptimer)
{
    char command[MAX_COMMAND_LEN];
    int rc;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;

    if (gf_timeexpired >= EXIT_FAST) return -1;

  /* check if the folder is already selected */
    if (strcmp(pIMAP->selected_folder, folder) == 0) {
	/* it is, so send a NOOP to check for recent msgs */
	sprintf(command, "%d NOOP%s", ++pIMAP->seq_num, CRLF);
	event_start(ptcx, &stats->cmd);
	rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				   command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	event_stop (ptcx, &stats->cmd);
	if (rc == -1) {
	    return -1;
	}
    } else {
	sprintf(command, "%d SELECT %s%s", ++pIMAP->seq_num, folder, CRLF);
	event_start(ptcx, &stats->cmd);
	rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				   command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	event_stop (ptcx, &stats->cmd);
	if (rc == -1) {
	    return -1;
	}
    }

    strcpy(pIMAP->selected_folder, folder);

    return 0;
}

static int
imapCloseFolder(ptcx_t ptcx,
		IMAP *pIMAP,
		SOCKET sock,
		cmd_stats_t *ptimer)
{
    char command[MAX_COMMAND_LEN];
    int rc;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;

    pIMAP->selected_folder[0] = '\0';

    sprintf(command, "%d CLOSE%s", ++pIMAP->seq_num, CRLF);
    event_start(ptcx, &stats->cmd);
    rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
			       command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
    event_stop (ptcx, &stats->cmd);
    if (rc == -1) {
	if (gf_timeexpired < EXIT_FAST) {
	    stats->cmd.errs++;
	}
	return -1;
    }

    return 0;
}

static int
imapParseSelectResponse(ptcx_t ptcx, IMAP *pIMAP, char *attr, int *value)
{
    char *bufPtr = pIMAP->resp_buffer;
    char parsedAttr[32];
    int parsedValue = 0;

    *value = 0;
    /* parse number of existing/recent msgs out of buffer */
    while (bufPtr && *bufPtr) {
	if (sscanf(bufPtr, "* %d %s", &parsedValue, parsedAttr)) {
	    D_PRINTF(debugfile,"found: %d %s\n", parsedValue, parsedAttr);
	    if (strcmp(parsedAttr, attr) == 0)
		*value = parsedValue;
	}

	bufPtr = strchr(bufPtr, '\n');
	if (bufPtr) bufPtr++;
    }
    return 0;
}    

static int
imapRetrRecentMessages(ptcx_t ptcx,
		       IMAP *pIMAP,
		       SOCKET sock,
		       mail_command_t *cmd,
		       cmd_stats_t *ptimer,
		       int numExists, 
		       int numRecent)
{
    int i = 0;
    char command[MAX_COMMAND_LEN];
    int numBytes = 0;
    long msgSize = 0;
#if 0
    char *msgBuffer = NULL;
    int msgBufferSize = 0;
#endif 
    int	rc;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;
    pish_command_t	*pish = (pish_command_t *)cmd->data;
    int leave_on_server;

    char *p, *endp;

    char newmsgs[MAX_RESPONSE_LEN];

    /* if we're told to leave mail on server, do not delete the message */
    leave_on_server = (int)sample(pish->leaveMailOnServerDist);

    if ( numRecent == 0 )
	return 0;

	/* This actually finds the new messages. */
    sprintf(command, "%d SEARCH (NEW)" CRLF, ++pIMAP->seq_num);
    event_start(ptcx, &stats->cmd);
    rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
			       command, pIMAP->resp_buffer, 
			       sizeof(pIMAP->resp_buffer));
    event_stop (ptcx, &stats->cmd);
    if (rc == -1)
	return -1;
    if((p = strstr(pIMAP->resp_buffer, "SEARCH")) == NULL)
    {
	fprintf(stderr, "IMAP: invalid SEARCH response: %s\n",
		 pIMAP->resp_buffer);
	return -1;
    }
    p += strlen("SEARCH");
    
    if((endp = strchr(p, '\n')) != NULL)
	*endp = 0;

    /* response buffer overwritten by new IMAP commands, so save a copy */
    strcpy(newmsgs, p);
    p = newmsgs;
    
    for(i = strtol(p, &p, 0); i != 0; i = strtol(p, &p, 0))
    {
	/* bail if time is up */
	if (gf_timeexpired >= EXIT_SOON) {
	    D_PRINTF(debugfile,"Time expired while reading messages\n");
	    break;
	}

	/* fetch the size of the recent msg */
	sprintf(command, "%d FETCH %d (RFC822.SIZE)%s", 
		++pIMAP->seq_num, i, CRLF);
	event_start(ptcx, &stats->cmd);
	rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				   command, pIMAP->resp_buffer, 
				   sizeof(pIMAP->resp_buffer));
	event_stop (ptcx, &stats->cmd);
	if (rc == -1) {
	    if (gf_timeexpired >= EXIT_FAST) break; /* dont fall into error */
	    return -1;
	}

	/* parse the SIZE out of buffer */
	if (!sscanf(pIMAP->resp_buffer, "* %*d FETCH (RFC822.SIZE %ld)", &msgSize)) {
	    returnerr(debugfile, 
		      "IMAP4 Error parsing size of msg from response, %s: %s\n",
		      pIMAP->resp_buffer, neterrstr());
	    return -1;
	}
	
	/* Sean O'Rourke: we used to allocate a message-size buffer here.  Don't
           do that, since we don't care about message contents,
           anyways. */
#if 0
	msgBufferSize = msgSize+1024;
	msgBuffer = (char *) xcalloc(msgBufferSize);
#endif /* 0 */
	/* FETCH the msg */
	sprintf(command, "%d FETCH %d (RFC822)%s", ++pIMAP->seq_num, i, CRLF);
	event_start(ptcx, &stats->msgread);
	numBytes = sendCommand(ptcx, sock, command);
	if (numBytes == -1) {
	    event_stop(ptcx, &stats->msgread);
#if 0
	    xfree(msgBuffer);
#endif
	    if (gf_timeexpired >= EXIT_FAST) break; /* dont fall into error */
	    stats->msgread.errs++;
	    returnerr(debugfile, "IMAP4 Error sending [%s] command: %s\n",
		      command, neterrstr());
	    return -1;
	}

	/* read msg */
#if 0
	numBytes = retrImapMsg(ptcx, sock, pIMAP->seq_num,
			       msgBuffer, msgSize, msgBufferSize);
#else
	if (pish->genChecksum == CS_NONE)
	    numBytes = retrImapMsg(ptcx, sock, pIMAP->seq_num, msgSize);
	else {
	    char term[20];
	    /* XXX: is this actually what's done? */
	    snprintf(term, sizeof(term), CRLF "%d ", pIMAP->seq_num);
	    numBytes = cs_retrieve(ptcx, sock, CRLF CRLF, term);
	}
#endif
	event_stop(ptcx, &stats->msgread);
	if (numBytes < 0) {
#if 0
	    xfree(msgBuffer);
#endif
	    if (gf_timeexpired >= EXIT_FAST) break; /* dont fall into error */
	    stats->msgread.errs++;
	    returnerr(debugfile, "IMAP4 Error retrieving msg %d: %s\n",
		      i, neterrstr());
	    return -1;
	}

	T_PRINTF(ptcx->logfile, command,
		 strlen (command), "IMAP4 SendCommand");

#if 0	
	xfree(msgBuffer);
#endif

#ifdef MSG_READ_TIME
	/* wait while "user" reads message */
	if (pish->msgReadTime) {
	    int read_time = sample(pish->msgReadTime);
	    if (read_time > 0) {
		event_start(ptcx, &ptimer->idle);
		MS_idle(ptcx, read_time);
		event_stop(ptcx, &ptimer->idle);
	    }
	}
#endif /* MSG_READ_TIME */

	/* send a NOOP */
	sprintf(command, "%d NOOP%s",++pIMAP->seq_num, CRLF);
	event_start(ptcx, &stats->cmd);
	rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				   command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	event_stop (ptcx, &stats->cmd);
	if (rc == -1) {
	    if (gf_timeexpired >= EXIT_FAST) break; /* dont fall into error */
	    stats->cmd.errs++;
	    return -1;
	}

	/* if we're told to leave mail on server, do not delete the message */
        if (!(pish->flags & leaveMailUnseen)) {
	    if (pish->flags & leaveMailOnServer && /* just mark seen */
		(leave_on_server > 0)) {
					/* mark the msg \seen needed??? */
		sprintf(command, "%d STORE %d +FLAGS (\\SEEN)%s",
			++pIMAP->seq_num,i, CRLF);
	    } else {			/* delete message */
					/* mark the msg \deleted and \seen */
		sprintf(command, "%d STORE %d +FLAGS (\\DELETED \\SEEN)%s",
			++pIMAP->seq_num,i, CRLF);
	    }
	    event_start(ptcx, &stats->cmd);
	    rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				       command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	    event_stop (ptcx, &stats->cmd);
	    if (rc == -1) {
		if (gf_timeexpired >= EXIT_FAST) break; /* dont fall into error */
		stats->cmd.errs++;
		return -1;
	    }
	} else {
	    /* We dont mark it seen, but it still isnt new anymore... */
	    D_PRINTF (stderr, "retrRecentMsgs() Leaving messaged %d unseen\n", i);
	}
    }

    if (!(pish->flags & leaveMailOnServer) || /* expunge if we are deleting */
	 (leave_on_server == 0)) {
	/* EXPUNGE \deleted messages */
	sprintf(command, "%d EXPUNGE%s", ++pIMAP->seq_num, CRLF);
	event_start(ptcx, &stats->cmd);
	rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
				   command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
	event_stop (ptcx, &stats->cmd);
	if (rc == -1) {
	    if (gf_timeexpired < EXIT_FAST)	/* dont fall into error */
		return -1;
	    stats->cmd.errs++;
	}
    }

    return 0;
}

/* Given the imapSearchRate, the number of searches to perform in a
   8 hour period, pick a set of random numbers telling us when to search.

   Ex. imapSearchRate = 3 and testime = 1*60*60 (1 hour).
	  pick 3 numbers at random between 1 and 8*60*60

   Ex. imapSearchRate = 5 and testime = 12*60*60 (12 hours), 
	  pick 5*(12/8) numbers a t random between 1 and 
	  12*60*60
	  
   The numbers are ordered and stored in the array searchSchedule.
*/
static int
imapComputeSearchSchedule(ptcx_t ptcx, mail_command_t *cmd, int **searchSchedule)
{
    int i,j,k;
    int numSearches = 0;
    int ran = 0;
    pish_command_t	*pish = (pish_command_t *)cmd->data;

    *searchSchedule = NULL;

    /* make sure we need to be here */
    if (!pish->imapSearchRate)
	return 0;

    /* determine number of searches we'll perform, based on searchRate
       and number of hours in testtime (if testime greater than 8 hours) */
    if (gt_testtime <= 8*60*60)
	numSearches = pish->imapSearchRate;
    else
	numSearches = pish->imapSearchRate * (gt_testtime / (8*60*60));

    D_PRINTF(debugfile,"num searches to perform=%d\n", numSearches);
	
    *searchSchedule = (int *) xcalloc(numSearches*sizeof(int));

    /* fill in the schedule, ordering the array */
    for (i = 0; i < numSearches; i++) {
	ran = (RANDOM() % (gt_testtime <= 8*60*60 ? 8*60*60 : gt_testtime));
		
	j = 0;
	while ((j < i) && ((*searchSchedule)[j] <= ran))
	    j++;

	/* insert time */

	for (k = i; k > j; k--)
	    (*searchSchedule)[k] = (*searchSchedule)[k-1];

	(*searchSchedule)[j] = ran;
    }

    return numSearches;
}

static int
imapSearchFolder(ptcx_t ptcx,
		 IMAP *pIMAP,
		 SOCKET sock,
		 mail_command_t *cmd,
		 cmd_stats_t *ptimer)
{
    struct timeval	beforeSearch;
    struct timeval	afterSearch;
    char command[MAX_COMMAND_LEN];
    int rc;
    pish_command_t	*pish = (pish_command_t *)cmd->data;
    pish_stats_t	*stats = (pish_stats_t *)ptimer->data;

    timeval_stamp(&beforeSearch);

    /* search the folder for messages containing subj pattern,
     * don't do anything with the search results right now.
     */
    sprintf(command, "%d SEARCH subject %s%s",
	    ++pIMAP->seq_num, pish->imapSearchPattern, CRLF);
    event_start(ptcx, &stats->cmd);
    rc = doImapCommandResponse(ptcx, sock, pIMAP->seq_num,
			       command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer));
    event_stop (ptcx, &stats->cmd);
    if (rc == -1) {
	return -1;
    }

    timeval_stamp(&afterSearch);
    
    /* print out a SEARCH (begin_time, end_time)  to graph */
    {					/* needs to be updated */
	char buf[64];
	sprintf(buf, "SEARCH: time=(%lu,%lu)\n",
		beforeSearch.tv_sec, afterSearch.tv_sec); 
	sendOutput(ptcx->ofd, buf);
    }

    return 0;
}

#if 0					/* not currently used */
static int
imapCreateFolder(ptcx_t ptcx, IMAP *pIMAP, const char *folder)
{
    char command[MAX_COMMAND_LEN];

    sprintf(command, "%d CREATE %s%s",
	    ++pIMAP->seq_num, folder, CRLF);
    if (doImapCommandResponse(ptcx, ptcx->sock, pIMAP->seq_num,
			      command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer)) == -1) {
	return -1;
    }
    return 0;
}
#endif

static int
imapDeleteFolder(ptcx_t ptcx, IMAP *pIMAP, const char *folder)
{
    char command[MAX_COMMAND_LEN];

    sprintf(command, "%d DELETE %s%s",
	    ++pIMAP->seq_num, folder, CRLF);
    if (doImapCommandResponse(ptcx, ptcx->sock, pIMAP->seq_num,
			      command, pIMAP->resp_buffer, sizeof(pIMAP->resp_buffer)) == -1) {
	return -1;
    }
    return 0;
}
