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
 *                     Thom O'Connor <thom@sendmail.com>
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

#include <assert.h>
#include <stdio.h>
#include "bench.h"
#include "pish.h"
#include "xalloc.h"
#include "checksum.h"

/* flags definitions */
#define leaveMailOnServer 0x01

/*
**  MPOP user structure
**
**    each context is a NULL-terminated linked list of users.
*/
typedef struct _MPop {
	struct _MPop	*next;
	char		*login;		/* login name (malloc()-allocated) */
	char		*passwd;	/* POP password (malloc()-allocated) */
	int		old_msgs;	/* messages in box at last check */
	int 		leave_on_server; /* 1 == don't DELE messages */
} MPop;

/* POP RFC says responses are all <= 512 bytes including CRLF: */
#define RESPLEN 	512

/*
**  Functions implementing each protocol command
*/

static int mpop_connect(ptcx_t , pish_command_t *, pish_stats_t *, MPop *);
static int mpop_login(ptcx_t , pish_stats_t *, MPop *, pmail_command_t);
static int mpop_uidl(ptcx_t , pish_stats_t *, MPop *);
static int mpop_stat(ptcx_t , pish_stats_t *, MPop *);
static int mpop_retr(ptcx_t , pish_stats_t *, MPop *, int );
static int mpop_dele(ptcx_t , pish_stats_t *, MPop *, int );
static int mpop_quit(ptcx_t , pish_command_t *, pish_stats_t *, MPop *);

/*
**  Internal utility functions
*/

static int mpop_cmd_resp(ptcx_t , char *, char *, int );
static int mpop_basic_cmd(ptcx_t , char *);
static int mpop_multiline_cmd(ptcx_t , char *);
static int mpop_simultaneous_users(pish_command_t *, pish_stats_t *);

static int
mpop_cmd_resp(ptcx, cmd, resp, rlen)
	ptcx_t ptcx;
	char *cmd;
	char *resp;
	int rlen;
{
	int len;
	SOCKET sock = ptcx->sock;

	T_PRINTF(ptcx->logfile, cmd, strlen(cmd), "MPOP command");
	len = doCommandResponse(ptcx, sock, cmd, resp, rlen);
	if (len > 0 && resp[0] != '+') /* not "+OK..." */
		len = -1;	
	
	T_PRINTF(ptcx->logfile, resp, len, "MPOP response");
	return len;
}

static int
mpop_basic_cmd(ptcx, cmd)
	ptcx_t ptcx;
	char *cmd;
{
	char dont_care_buf[RESPLEN];
	return mpop_cmd_resp(ptcx, cmd, dont_care_buf, sizeof(dont_care_buf));
}

static int
mpop_connect(ptcx, pish, stats, mp)
	ptcx_t ptcx;
	pish_command_t *pish;
	pish_stats_t *stats;
	MPop *mp;
{
	char	buf[RESPLEN];
	int	len;
	
	ptcx->net_timeout = pish->net_timeout;
    
	event_start(ptcx, &stats->connect);
	ptcx->sock = connectSocket(ptcx, &pish->hostInfo, "tcp");
	event_stop(ptcx, &stats->connect);
	if (BADSOCKET(ptcx->sock))
	{
		if (gf_timeexpired < EXIT_FAST)
		{
			stats->connect.errs++;
			returnerr(debugfile,
				  "MPOP Couldn't connect to %s: %s\n",
				  pish->hostInfo.hostName, neterrstr());
		}
		return -1;
	}
	if (gf_abortive_close)
	{
		if (set_abortive_close(ptcx->sock) != 0)
		{
			returnerr (debugfile,
				   "MPOP: WARNING: Could not set abortive close\n");
		}
	}

#ifdef SOCK_SSL
	if (pish->sslTunnel)
	{
		SSL_INIT(ptcx->sock, pish);
		D_PRINTF(debugfile, "MPOP: initialized SSL tunneling.");
	}
#endif /* SOCK_SSL */
    
	LS_INIT(ptcx->sock, pish);
	
	/* READ connect response from server */
	event_start(ptcx, &stats->banner);
	len = readResponse(ptcx, ptcx->sock, buf, sizeof(buf));
	event_stop(ptcx, &stats->banner);
	if (len <= 0)
	{
		if (gf_timeexpired < EXIT_FAST)
		{
			stats->banner.errs++;
			returnerr(debugfile,"MPOP Error reading banner: %s\n",
				  neterrstr());
		}
		return -1;
	}
	T_PRINTF(ptcx->logfile, buf, len, "MPOP banner");
	return 0;
}	

static int
mpop_login(ptcx, stats, mp, cmd)
	ptcx_t ptcx;
	pish_stats_t *stats;
	MPop *mp;
	pmail_command_t cmd;
{
	int 	n;
	char	buf[512];	/* big enough for "XXXX username@host\r\n" */
#ifdef SOCK_SSL
	pish_command_t *pish = (pish_command_t*)cmd->data;
	if (!pish->useTLS)
		goto end_tls;
	
	event_start(ptcx, &stats->cmd);
	n = mpop_cmd_resp(ptcx, "CAPA" CRLF, buf, sizeof(buf));
	event_stop(ptcx, &stats->cmd);
	if (n < 0)
	{
		stats->cmd.errs++;
		goto end_tls;
	}
	if (strstr(buf, "STLS") != NULL)
	{
		event_start(ptcx, &stats->cmd);
		n = mpop_basic_cmd(ptcx, "STLS" CRLF);
		event_stop(ptcx, &stats->cmd);
		if (n < 0)
			stats->cmd.errs++;
		else
			SSL_INIT(ptcx->sock, cmd->data);
	}
	
 end_tls:
#endif /* SOCK_SSL */
	
	event_start(ptcx, &stats->login);
	snprintf(buf, sizeof(buf), "USER %s" CRLF, mp->login);
	if ((n = mpop_basic_cmd(ptcx, buf) < 0))
		goto done;
	
	snprintf(buf, sizeof(buf), "PASS %s" CRLF, mp->passwd);
	n = mpop_basic_cmd(ptcx, buf);
 done:
	if (n < 0)
		stats->login.errs++;
	event_stop(ptcx, &stats->login);
	return n;
}

static int
mpop_stat(ptcx, stats, mp)
	ptcx_t ptcx;
	pish_stats_t *stats;
	MPop *mp;
{
	char buf[RESPLEN];
	int len;
	int msgs = -1;
	int size = -1;

	event_start(ptcx, &stats->cmd);
	if ((len = mpop_cmd_resp(ptcx, "STAT" CRLF, buf, sizeof(buf)-1)) < 0)
		goto done;
	buf[len] = '\0';
	sscanf(buf, "+OK %d %d", &msgs, &size);
 done:
	event_stop(ptcx, &stats->cmd);
	if (msgs < 0)
	{
		D_PRINTF(debugfile,
			 "Multipop: Error in STAT response `%s'\n", buf);
		stats->cmd.errs++;
	}
	return msgs;
}

static int
mpop_dele(ptcx, stats, mp, msgnum)
	ptcx_t ptcx;
	pish_stats_t *stats;
	MPop *mp;
	int msgnum;
{
	char buf[64];
	int len;
	snprintf(buf, sizeof(buf), "DELE %d"CRLF, msgnum);
	event_start(ptcx, &stats->cmd);
	len = mpop_basic_cmd(ptcx, buf);
	event_stop(ptcx, &stats->cmd);
	if (len < 0)
		stats->cmd.errs++;
	return len;
}

static int
mpop_quit(ptcx, pish, stats, mp)
	ptcx_t 		ptcx;
	pish_command_t 	*pish;
	pish_stats_t 	*stats;
	MPop 		*mp;
{
	int len = 0;
	if (!sample(pish->dropRate))
	{
		event_start(ptcx, &stats->logout);
		len = mpop_basic_cmd(ptcx, "QUIT"CRLF);
	}
	if (!BADSOCKET(ptcx->sock))
		NETCLOSE(ptcx->sock);
	event_stop(ptcx, &stats->logout);

	if (len < 0)
		stats->logout.errs++;
	return len;
}

static int
mpop_multiline_cmd(ptcx, cmd)
	ptcx_t ptcx;
	char *cmd;
{
	SOCKET sock = ptcx->sock;
	int len;
	int nread = 0;

	char *EOM = CRLF"."CRLF;
	int EOM_LEN = strlen(CRLF"."CRLF);
	
	char buf[4096];

	if ((len = sendCommand(ptcx, ptcx->sock, cmd)) < 0)
		return -1;
	while ((len = retryRead(ptcx, ptcx->sock, buf, sizeof(buf))) <= 0)
	{
		if (len < 0)
			return -1;
	}
	if (buf[0] != '+')
		return -1;
	
	nread += len;
	len -= EOM_LEN;

	/* len is always EOM_LEN less than the amt of data present here */
	while (memcmp(buf + len, EOM, EOM_LEN) != 0)
	{
		memcpy(buf, buf + len, EOM_LEN);
		if ((len = retryRead(ptcx, sock, buf + EOM_LEN,
				    sizeof(buf) - EOM_LEN)) < 0)
			return -1;
		ptcx->bytesread += len;
		nread += len;
	}
	
	return nread;
}

static int
mpop_retr(ptcx, stats, mp, msgnum)
	ptcx_t ptcx;
	pish_stats_t *stats;
	MPop *mp;
	int msgnum;
{
	char cmd[64];
	int len;
	snprintf(cmd, 64, "RETR %d"CRLF, msgnum);
	event_start(ptcx, &stats->msgread);
	len = mpop_multiline_cmd(ptcx, cmd);
	event_stop(ptcx, &stats->msgread);
	if (len < 0)
		stats->msgread.errs++;
	return len;
}

static int
mpop_uidl(ptcx, stats, mp)
	ptcx_t ptcx;
	pish_stats_t *stats;
	MPop *mp;
{
	int len;
	event_start(ptcx, &stats->cmd);
	len = mpop_multiline_cmd(ptcx, "UIDL"CRLF);
	event_stop(ptcx, &stats->cmd);
	if (len < 0)
		stats->cmd.errs++;
	return len;
}

static int
mpop_simultaneous_users(pish, stats)
	pish_command_t	*pish;
	pish_stats_t	*stats;
{
	int per_client = (stats->loginRange.span + 1)
		* (stats->domainRange.span + 1);
	double users = (double)per_client * pish->percentActive;
	return (int)rint(users);
}

/*
** External routines
*/

int
MPopParseStart(cmd, line, defparm)
	pmail_command_t cmd;
	char *line;
	param_list_t *defparm;
{
	int ret = Pop3ParseStart(cmd, line, defparm);
	pish_command_t *pish = (pish_command_t*)cmd->data;
	pish->percentActive = 1.0;
	return ret;
}

int
MPopParseEnd(cmd, section, defparm)
	pmail_command_t cmd;
	string_list_t *section;
	param_list_t *defparm;
{
	pish_command_t *pish;
	if (!Pop3ParseEnd(cmd, section, defparm))
		return returnerr(stderr, "Multipop: POP parse error\n");
	pish = (pish_command_t*)cmd->data;
	if (pish->percentActive == 0)
		return returnerr(stderr,
				 "Multipop: percentActive == 0\n");
	if (pish->userSpacing == NULL)
		return returnerr(stderr,
				 "Multipop: missing userSpacing\n");
	return 1;
}

int
MPopCheck(ptcx, cmd, ptimer, ptr)
	ptcx_t ptcx;
	pmail_command_t cmd;
	cmd_stats_t *ptimer;
	void *ptr;
{
	int 		new_msgs;
	int 		start_time, user_spacing;
	int 		i = 0;
	MPop 		*mp;
	pish_stats_t	*stats = (pish_stats_t *)ptimer->data;
	pish_command_t	*pish = (pish_command_t *)cmd->data;

	for (mp  = (MPop*)ptr; mp != NULL; mp = mp->next)
	{
		int nnew;
		D_PRINTF(debugfile, "Multipop[%d]: begin check for `%s'\n",
			 ptcx->threadnum, mp->login);
		
		start_time = time(0);
		if (mpop_connect(ptcx, pish, stats, mp) < 0)
			continue; /* failed connect => try next user */
	
		if (mpop_login(ptcx, stats, mp, cmd) < 0)
			goto checkdone;
	
		if ((new_msgs = mpop_stat(ptcx, stats, mp)) < 0)
			goto checkdone;

		if (new_msgs > 0 && mp->leave_on_server)
			mpop_uidl(ptcx, stats, mp);

		nnew = new_msgs - mp->old_msgs;
		for (i = mp->old_msgs; i < new_msgs; i++)
		{
			/* NOTE: POP starts counting at 1, it appears */
			int msgnum = i + 1;

			if (pish->genChecksum == CS_NONE)
			{
				if (mpop_retr(ptcx, stats, mp, msgnum) < 0)
					goto checkdone;
			}
			else
			{
				char buf[32];
				int ret;
				snprintf(buf, sizeof(buf), "RETR %d" CRLF, 
					 msgnum);
				ret = cs_retrieve(ptcx, ptcx->sock, CRLF CRLF,
						  CRLF "." CRLF);
				if (ret < 0)
					goto checkdone;
			}
		
			if (mp->leave_on_server == 0)
			{
				if (mpop_dele(ptcx, stats, mp, msgnum) < 0)
					goto checkdone;
			}
			else
				mp->old_msgs++;
		}

		D_PRINTF(debugfile, "Multipop[%d]: %d messages for `%s'\n",
			 ptcx->threadnum, nnew, mp->login);
	checkdone:
		mpop_quit(ptcx, pish, stats, mp);
		user_spacing = sample(pish->userSpacing);
		if (user_spacing > 0 && mp->next != NULL)
		{
			user_spacing -= (time(0) - start_time)*1000;
			if (user_spacing > 0)
				MS_idle(ptcx, user_spacing);
			else
				D_PRINTF(debugfile,
					 "Warning: multipop behind by %d\n",
					 -user_spacing);
		}
	}
	return 0;
}

void *
MPopCheckStart(ptcx, cmd, ptimer)
	ptcx_t ptcx;
	pmail_command_t cmd;
	cmd_stats_t *ptimer;
{
	int 		domain, login;
	int 		len;
	int		i;
	pish_stats_t	*stats = (pish_stats_t *)ptimer->data;
	pish_command_t	*pish = (pish_command_t *)cmd->data;
	int 		users = mpop_simultaneous_users(pish, stats);
	MPop		*mp = NULL;

	D_PRINTF(debugfile, "Multipop[%d]: %d users per block\n",
		 ptcx->threadnum, users);
	
	for (i = 0; i < users; i++)
	{
		/* set up our identity */
		MPop		*temp = XALLOC(struct _MPop);
		temp->next = mp;
		mp = temp;
		
		domain = rangeNext(&stats->domainRange, stats->lastDomain);
		stats->lastDomain = domain;
		login = rangeNext (&stats->loginRange, stats->lastLogin);
		stats->lastLogin = login;
		
		len = strlen(pish->loginFormat) + 10;
		temp->login = xalloc(len);
		snprintf(temp->login, len, pish->loginFormat, login, domain);
		
		len = strlen(pish->passwdFormat) + 10;
		temp->passwd = xalloc(len);
		snprintf(temp->passwd, len, pish->passwdFormat, login);	
		
		temp->old_msgs = 0;
		temp->leave_on_server = pish->flags & leaveMailOnServer;
	}
/* 	assert(mp != NULL); */
	D_PRINTF(debugfile, "Multipop[%d]: first check\n", ptcx->threadnum);
	if (mp == NULL)
		fprintf(stderr, "Multipop[%d]: no users!\n", ptcx->threadnum);
	else
		MPopCheck(ptcx, cmd, ptimer, mp);
	return mp;
}

void
MPopCheckEnd(ptcx, cmd, ptimer, ptr)
	ptcx_t ptcx;
	pmail_command_t cmd;
	cmd_stats_t *ptimer;
	void *ptr;
{
	MPop *mp = (MPop*)ptr;
	D_PRINTF(debugfile, "Multipop[%d]: last check\n", ptcx->threadnum);
	MPopCheck(ptcx, cmd, ptimer, ptr);
	while (mp != NULL)
	{
		MPop *tmp = mp;
		mp = mp->next;
		xfree(tmp->login);
		xfree(tmp->passwd);
		xfree(tmp);
	}
}
