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

#include "bench.h"
#include "pish.h"
#include "http-util.h"
#include "xalloc.h"
#include "sb.h"

/* flags definitions */
#define leaveMailOnServer 0x01

#define WEBMAIL_UPDATE	30 * 1000 /* 30-second default check interval */
#define COOKIE		"Cookie: "

typedef pish_stats_t web_stats_t;
typedef pish_command_t web_command_t;

/*
**  WEB_USER functions
*/

typedef struct web_user_s
{
	char		*cookies;
	int		usernum;
	int		domainnum;
	int		needs_expunge;
} web_user_t;

static void add_cookie(web_user_t *, char *);
static void clear_cookies(web_user_t *);
static web_user_t* new_web_user(web_command_t *, web_stats_t *);
static void delete_web_user(web_user_t *);

static void
add_cookie(user, cookie)
	web_user_t	*user;
	char		*cookie;
{
	if (user->cookies == NULL)
	{
		user->cookies = xalloc(strlen(COOKIE) + strlen(cookie) + 1);
		sprintf(user->cookies, COOKIE "%s", cookie);
	}
	else
	{
		char *tmp = xalloc(strlen(user->cookies)
				   + strlen(cookie)
				   + 3);
		sprintf(tmp, "%s; %s", user->cookies, cookie);
		xfree(user->cookies);
		user->cookies = tmp;
	}
}

static void
clear_cookies(user)
	web_user_t	*user;
{
	xfree(user->cookies);
	user->cookies = NULL;
}

static web_user_t*
new_web_user(cmd, stats)
	web_command_t	*cmd;
	web_stats_t	*stats;
{
	int domain, login;
	web_user_t *ret = XALLOC(web_user_t);

	domain = rangeNext(&stats->domainRange, stats->lastDomain);
	stats->lastDomain = domain;
        login = rangeNext(&stats->loginRange, stats->lastLogin);
	stats->lastLogin = login;

	ret->cookies = NULL;
	ret->usernum = login;
	ret->domainnum = domain;
	ret->needs_expunge = 0;
	return ret;
}

static void
delete_web_user(user)
	web_user_t	*user;
{
	if(user->cookies)
		xfree(user->cookies);
	xfree(user);
}

/*
**  Webmail parsing routines
*/

typedef struct web_param_s
{
	char		*name;
	char		*value;
} web_param_t;

static web_param_t web_defaults[] =
{
	{ "webbase",		"/cgi-bin/mailspinner.cgi"},

	{ "webloginformat", 	"?username=%s&password=%s&login=Login"},
	{ "webloginpages", 	"/LeftControl" },
	{ "webloginpages", 	"/MailFrames" },
	{ "webloginpages", 	"/MailControl" },
	{ "webloginpages", 	"/BodyIndex" },
	
	{ "webdeleteformat",	"/BodyIndex?Delete+Selected+Messages.x=1"
	  			"&Delete+Selected+Messages.y=1&%s" },
	{ "webexpunge",		"/BodyIndex?Expunge.x=1&Expunge.y=1" },
	{ "webindex",		"/Index/1#new"},
	{ "weblogout",		"/Logout"},
	{ "webmsgread",		"/ReadIndex/%d"},
	{ NULL,			NULL }
};

static int
WebmailParseNameValue(mailcmd, name, tok)
	pmail_command_t mailcmd;
	char 		*name;
	char 		*tok;
{
	web_command_t	*cmd = (web_command_t*)mailcmd->data;
	if (strcmp(name, "webloginpages") == 0)
	{
		string_list_t *l = XALLOC(string_list_t);
		l->next = cmd->webLoginPages;
		l->value = xstrdup(tok);
		cmd->webLoginPages = l;
	}

	else if (strcmp(name, "webloginformat") == 0)
		cmd->webLoginFormat = xstrdup(tok);
	else if (strcmp(name, "webbase") == 0)
		cmd->webBase = xstrdup(tok);
	else if (strcmp(name, "webindex") == 0)
		cmd->webIndex = xstrdup(tok);
	else if (strcmp(name, "weblogout") == 0)
		cmd->webLogout = xstrdup(tok);
	else if (strcmp(name, "webmsgread") == 0)
		cmd->webMsgRead = xstrdup(tok);
	else if (strcmp(name, "webdeleteformat") == 0)
		cmd->webDeleteFormat = xstrdup(tok);
	else if (strcmp(name, "webexpunge") == 0)
		cmd->webExpunge = xstrdup(tok);
	else if (strcmp(name, "leavemailonserver") == 0)
		if (atoi(tok) > 0) {
		   cmd->flags |= leaveMailOnServer;
		   cmd->leaveMailOnServerDist =
		      parse_distrib(tok, (value_parser_t)&atof);
		}
		else
		   cmd->flags &= ~leaveMailOnServer;
	else
		return pishParseNameValue(mailcmd, name, tok);
	return 0;
}

int
WebmailParseStart(mailcmd, line, defparm)
	pmail_command_t mailcmd;
	char 		*line;
	param_list_t 	*defparm;
{
	param_list_t	*pp;
	web_param_t	*def;
	web_command_t	*cmd = XALLOC(web_command_t);
	mailcmd->data = cmd;
	
	mailcmd->loopDelay = constant_value(WEBMAIL_UPDATE);
	cmd->hostInfo.portNum = WEBMAIL_PORT;

	/* Fill in the above defaults */
	for (def = web_defaults; def->name; def++)
		(void)WebmailParseNameValue (mailcmd, def->name, def->value);
	
	/* Fill in defaults first, ignore defaults we dont use */
	for (pp = defparm; pp; pp = pp->next)
		(void)WebmailParseNameValue (mailcmd, pp->name, pp->value);

	return 1;
}

#define REQUIRE_ATTR(cmd, attr) do { \
	if (!(cmd)->attr) \
	{ \
		fprintf(stderr,"webmail: missing " #attr ); \
		return returnerr(stderr,"webmail: missing " #attr "\n"); \
	} \
}	while (0)		

int
WebmailParseEnd (mailcmd, section, defparm)
	pmail_command_t mailcmd;
	string_list_t 	*section;
	param_list_t 	*defparm;
{
	string_list_t	*sp;
	web_command_t	*cmd = (web_command_t *)mailcmd->data;

	/* skip first and last */
	for (sp = section->next; sp->next; sp = sp->next)
	{
		char *name = sp->value;
		char *tok = name + strcspn(name, " \t=");
		*tok++ = 0;			/* split name off */
		tok += strspn(tok, " \t=");

		string_tolower(name);
		tok = string_unquote(tok);

		if (WebmailParseNameValue (mailcmd, name, tok) < 0)
		{
			/* not a known attr */
			fprintf(stderr,"unknown attribute '%s' '%s'\n",
				 name, tok);
			returnerr(stderr,"unknown attribute '%s' '%s'\n",
				  name, tok);
		}
	}

	/* check for some of the required command attrs */
	if (!cmd->hostInfo.hostName)
	{
		fprintf(stderr,"missing webmail server");
		return returnerr(stderr,"missing webmail server\n");
	}

	REQUIRE_ATTR(cmd, loginFormat);
	REQUIRE_ATTR(cmd, passwdFormat);
	
	REQUIRE_ATTR(cmd, webIndex);
	REQUIRE_ATTR(cmd, webBase);
	REQUIRE_ATTR(cmd, webMsgRead);
	REQUIRE_ATTR(cmd, webLoginFormat);
	
	/* see if we can resolve the mailserver addr */
	if (resolve_addrs(cmd->hostInfo.hostName, "tcp",
			  &(cmd->hostInfo.host_phe),
			  &(cmd->hostInfo.host_ppe),
			  &(cmd->hostInfo.host_addr),
			  &(cmd->hostInfo.host_type))) {
		return returnerr (stderr, "Error resolving hostname '%s'\n",
				  cmd->hostInfo.hostName);
	}
	else
		cmd->hostInfo.resolved = 1;
	
	rangeSetFirstCount(&cmd->loginRange, cmd->loginRange.first,
			   cmd->loginRange.span, cmd->loginRange.sequential);
	rangeSetFirstCount(&cmd->domainRange, cmd->domainRange.first,
			   cmd->domainRange.span, cmd->domainRange.sequential);

	return 1;
}

#undef REQUIRE_ATTR

static socket_buffer*
web_request(ptcx_t, web_command_t *, char *, web_user_t *);
static int
web_discard(ptcx_t, web_command_t *, char *, web_user_t *);
static int
web_login(ptcx_t, web_command_t	*, web_stats_t *, web_user_t *);
static int
web_list_msgs(ptcx_t, web_command_t *, web_stats_t *, web_user_t *, int *, int);
static int
web_get_msg(ptcx_t, web_command_t *, web_stats_t *, web_user_t *, int);

static int
http_connect (ptcx_t , resolved_addr_t *);

/*
**  This is exactly the same as HttpConnect in http-util.c, except
**  that it doesn't start and stop the event timer.
*/
static int
http_connect(ptcx, hostInfo)
	ptcx_t 		ptcx;
	resolved_addr_t	*hostInfo;
{
	int		rc=0;

	D_PRINTF(stderr, "http_connect()\n");

	ptcx->sock = connectSocket(ptcx, hostInfo, "tcp");
	if (BADSOCKET(ptcx->sock))
	{
		if (gf_timeexpired < EXIT_FAST)
		{
			returnerr(debugfile,
				  "%s<http_connect: connect to %s: %s\n",
				  ptcx->errMsg, hostInfo->hostName,
				  neterrstr());
		}
		return -1;
	}

	if (gf_abortive_close && set_abortive_close(ptcx->sock) != 0)
	{
		returnerr(debugfile,
			  "%s<http_connect: WARNING: Could not set abortive close\n",
			  ptcx->errMsg, neterrstr());
	}
	
	return rc;
}

static socket_buffer*
web_request(ptcx, cmd, url, user)
    ptcx_t 		ptcx;
    web_command_t 	*cmd;
    char		*url;
    web_user_t		*user;
{
	int len;
	int content_length = 0;
	socket_buffer *sb;
	D_PRINTF(debugfile, "web_request: `%s'\n", url);
	if (http_connect(ptcx, &cmd->hostInfo) < 0)
	{
		D_PRINTF(stderr, "web_request: Connect failed: %s\n",
			 strerror(errno));
		return NULL;
	}

	LS_INIT(ptcx->sock, cmd);
	
	ptcx->net_timeout = cmd->net_timeout;
    
	sb = sb_new(ptcx->sock, 1024);
	snprintf(sb->buf, 1024, "GET %s%s HTTP/1.0" CRLF,
		 cmd->webBase, url);
	if (user->cookies)
	{
		D_PRINTF(debugfile, "web_request: cookies = `%s'\n",
			 user->cookies);
		strcat(sb->buf, user->cookies);
		strcat(sb->buf, CRLF);
	}
	strcat (sb->buf, CRLF);

/*  	event_start(ptcx, stats);	 */
	if (sendCommand(ptcx, sb->sock, sb->buf) < 0)
	{
		D_PRINTF(stderr, "web_request: send failed: %s\n",
			strerror(errno));
		goto error_out;
	}

	/* Find return code */
	if (sb_discard(ptcx, sb, " ") < 0)
		goto error_out;

	if (sb->buf[0] != '2')
	{
		D_PRINTF(stderr, "web_request: error code %.3s\n", sb->buf);
		goto error_out;
	}

	for (;;)
	{
		if (sb_discard(ptcx, sb, "\n") < 0
		    || (len = sb_get(ptcx, sb, "\n")) < 0)
			goto error_out;
		
		if (len == 0 || (len == 1 && sb->buf[0] == '\r'))
		{
			sb_discard(ptcx, sb, "\n");
			break;
		}
		if (sb->buf[len-1] == '\r')
			sb->buf[len-1] = '\0';
		else
			sb->buf[len] = '\0';
		
		if (strncasecmp("set-cookie: ", sb->buf,
				strlen("set-cookie: ")) == 0)
			add_cookie(user, sb->buf + strlen("set-cookie: "));
		
#define 	LENGTH "content-length: "
		else if (strncasecmp(LENGTH, sb->buf, strlen(LENGTH)) == 0)
		{
			content_length = atoi(sb->buf + strlen(LENGTH));
			D_PRINTF(debugfile,
				 "web_request: content-length = %d\n",
				 content_length);
		}
#undef		LENGTH
		
		sb_discardn(ptcx, sb, len);
	}
/*  	event_stop(ptcx, stats); */
	return sb;
 error_out:
/*  	event_stop(ptcx, stats); */
	D_PRINTF(stderr, "web_request failed: %s\n", strerror(errno));
	sb_delete(sb);
	return NULL;
}

static int
web_discard(ptcx, cmd, req, user)
    ptcx_t 		ptcx;
    web_command_t 	*cmd;
    char		*req;
    web_user_t		*user;
{
	socket_buffer *sb;
	int nread;
	if ((sb = web_request(ptcx, cmd, req, user)) == NULL)
		return -1;
	nread = sb->data;
	while (sb_read(ptcx, sb) > 0)
		nread += sb->data;
	sb_delete(sb);
	return nread;
}

static int
web_login(ptcx, cmd, stats, user)
	ptcx_t		ptcx;
	web_command_t	*cmd;
	web_stats_t	*stats;
	web_user_t	*user;
{
	char buf[256], username[128], passwd[128];
	int ret = 0;
  	string_list_t *page;
	/* create login URL: */
	snprintf(username, sizeof(username), cmd->loginFormat,
		 user->usernum, user->domainnum);
	snprintf(passwd, sizeof(passwd), cmd->passwdFormat,
		 user->usernum, user->domainnum);		 
	snprintf(buf, sizeof(buf), cmd->webLoginFormat,
		 username, passwd);
	event_start(ptcx, &stats->login);
	if ((ret = web_discard(ptcx, cmd, buf, user)) < 0)
	{
		D_PRINTF(stderr, "web_login: discard failed for user %d: %s\n",
			user->usernum, strerror(errno));
/*    		stats->login.errs++; */
		goto done;
	}
	if (user->cookies == NULL)
	{
		D_PRINTF(stderr, "web_login: No cookies for user %d\n",
			user->usernum);
/*  		stats->login.errs++; */
		ret = -1;
		goto done;
	}
	for (page = cmd->webLoginPages; page ; page = page->next)
	{
		if (web_discard(ptcx, cmd, page->value, user) < 0)
		{
			D_PRINTF(stderr, "webmail: Can't get page `%s'\n",
				buf);
/*    			stats->login.errs++; */
			ret = -1;
			/* keep going */
		}
	}
 done:
	if (ret < 0)
		stats->login.errs++;
	event_stop(ptcx, &stats->login);
	return ret;
}

static int
web_list_msgs(ptcx, cmd, stats, user, msgs, nmsgs)
	ptcx_t		ptcx;
	web_command_t 	*cmd;
	web_stats_t 	*stats;
	web_user_t	*user;
	int		*msgs;
	int		nmsgs;
{
	socket_buffer *sb;
	static char *NEWMSG_PRE1 = "\n<td bgcolor=";
	static char *NEWMSG_PRE2 = "\n<input type=checkbox name=msg value=\"";
	static char *NEWMSG_POST = "\"";
	int msg = 0;
	int len;
	
	assert(cmd->webIndex != NULL);
	if ((sb = web_request(ptcx, cmd, cmd->webIndex, user)) == NULL)
	{
		msg = -1;
		goto done;
	}
	
	while (msg < nmsgs && sb_discard(ptcx, sb, NEWMSG_PRE1) >= 0)
	{
		D_PRINTF(debugfile, "web_list_msgs: looking for %d/%d\n",
			msg, nmsgs);
		
		if (sb_discard(ptcx, sb, NEWMSG_PRE2) < 0)
		{
			D_PRINTF(debugfile,
				"web_list_msgs: message %d pre-delim: %s\n",
				msg, strerror(errno));
			goto done;
		}
		if ((len = sb_get(ptcx, sb, NEWMSG_POST)) < 0)
		{
			D_PRINTF(debugfile,
				"web_list_msgs: message %d post-delim: %s\n",
				msg, strerror(errno));
			goto done;
		}
		msgs[msg++] = atoi(sb->buf);
		sb_discardn(ptcx, sb, len);
		D_PRINTF(debugfile, "web_list_msgs: msg %d = %d\n",
			 msg - 1, msgs[msg-1]);
	}
	D_PRINTF(debugfile,
		 "web_list_msgs: skipping to eof after %d messages\n", msg);
	while (sb_read(ptcx, sb) > 0)
		;
 done:
	if (msg < 0)
		stats->cmd.errs++;
	if (sb)
		sb_delete(sb);
	event_stop(ptcx, &stats->cmd);
	D_PRINTF(debugfile, "web_list_msgs: done\n");
	return msg;
}

static int
web_get_msg(ptcx, cmd, stats, user, msg)
	ptcx_t		ptcx;
	web_command_t	*cmd;
	web_stats_t	*stats;
	web_user_t	*user;
	int		msg;
{
	char url[256];
	int ret;
	sprintf(url, cmd->webMsgRead, msg);
	event_start(ptcx, &stats->msgread);
	if ((ret = web_discard(ptcx, cmd, url, user)) < 0)
		stats->msgread.errs++;
	event_stop(ptcx, &stats->msgread);
	return ret;
}

void *
doWebmailStart(ptcx, mailcmd, ptimer)
	ptcx_t		ptcx;
	mail_command_t	*mailcmd;
	cmd_stats_t	*ptimer;
{
	web_stats_t	*stats = (web_stats_t*)ptimer->data;
	web_command_t	*cmd = (web_command_t*)mailcmd->data;
	web_user_t 	*ret = new_web_user(cmd, stats);

	assert(cmd->webIndex != NULL);
	
	D_PRINTF(debugfile, "doWebmailStart: cmd = %p, user = %p\n",
		cmd, ret);
	/* GET login page */
	if(web_login(ptcx, cmd, stats, ret) < 0)
	{
		D_PRINTF(stderr, "Login failed for user %d: %s\n",
			ret->usernum, strerror(errno));
		delete_web_user(ret);
		return NULL;
	}
	assert(cmd->webIndex != NULL);
	D_PRINTF(debugfile, "Login done for user %d\n", ret->usernum);
	return (void*)ret;
}

#define WEB_NEWMSG_MAX 20

int
doWebmailLoop(ptcx, mailcmd, timer, state)
	ptcx_t 		ptcx;
	mail_command_t	*mailcmd;
	cmd_stats_t 	*timer;
	void 		*state;
{
	web_user_t	*user = (web_user_t*)state;
	web_command_t	*cmd = (web_command_t*)mailcmd->data;
	web_stats_t	*stats = (web_stats_t*)timer->data;
	int		msgs[WEB_NEWMSG_MAX];
	int		nmsgs;
	int		i;
	int 		ndele = 0;

	assert(cmd->webIndex != NULL);
	D_PRINTF(debugfile, "doWebmailLoop: cmd = %p, user = %p\n",
		cmd, user);
	
	if ((nmsgs = web_list_msgs(ptcx, cmd, stats, user, msgs, sizeof(msgs))) < 0)
	{
		D_PRINTF(stderr,
			 "doWebmailLoop: list failed for user %d: %s\n",
			 user->usernum, strerror(errno));
		return -1;
	}

	D_PRINTF(debugfile, "doWebmailLoop: %d msgs for user %d\n",
		nmsgs, user->usernum);
	
	for (i = 0; i < nmsgs; ++i)
	{
		if (web_get_msg(ptcx, cmd, stats, user, msgs[i]) < 0)
		{
			D_PRINTF(stderr, "can't get msg %d for user %d: %s\n",
				 msgs[i], user->usernum, strerror(errno));
/*  			break; */
		}

		D_PRINTF(debugfile, "doWebmailLoop: downloaded message %d\n",
			msgs[i]);
		
#ifdef MSG_READ_TIME
		if (cmd->msgReadTime)
		{
			int read_time = sample(cmd->msgReadTime);
			if (read_time > 0)
			{
				event_start(ptcx, &timer->idle);
				MS_idle(ptcx, read_time);
				event_stop(ptcx, &timer->idle);
			}
		}
#endif /* MSG_READ_TIME */
		
		/* maybe mark message for deletion */
		if (!(cmd->flags & leaveMailOnServer) &&
		   (sample(cmd->leaveMailOnServerDist) == 0))
			msgs[ndele++] = msgs[i];
	}
	/* delete messages (if necessary) */
	if (ndele > 0)
	{
		char buf[8];
		char deles[8 * WEB_NEWMSG_MAX];
		char url[512];
		deles[0] = '\0';
		for (i = 0; i < ndele; i++)
		{
			sprintf(buf, "&msg=%d", msgs[i]);
			strcat(deles, buf);
		}
		D_PRINTF(debugfile, "doWebmailLoop: deleting `%s'\n",
			 deles);
		snprintf(url, sizeof(url),
			 cmd->webDeleteFormat, deles);
		event_start(ptcx, &stats->cmd);
		if (web_discard(ptcx, cmd, url, user) < 0)
		{
			D_PRINTF(stderr, "doWebmailLoop: delete failed\n");
			stats->cmd.errs++;
			event_stop(ptcx, &stats->cmd);
			return -1;
		}
		event_stop(ptcx, &stats->cmd);
		user->needs_expunge = 1;
	}
	return 0;
}

void
doWebmailEnd(ptcx, mailcmd, ptimer, state)
	ptcx_t 		ptcx;
	mail_command_t	*mailcmd;
	cmd_stats_t 	*ptimer;
	void 		*state;
{
	web_command_t	*cmd = (web_command_t*)mailcmd->data;
	web_stats_t	*stats = (web_stats_t*)ptimer->data;
	web_user_t	*user = (web_user_t*)state;

	if (user->needs_expunge)
	{
		D_PRINTF(debugfile, "doWebmailEnd: expunge\n");
		event_start(ptcx, &stats->cmd);
		if (web_discard(ptcx, cmd, cmd->webExpunge, user) < 0)
		{
			D_PRINTF(stderr, "doWebmailEnd: expunge failed\n");
			stats->cmd.errs++;
		}
		event_stop(ptcx, &stats->cmd);
	}
	
	if (sample(cmd->dropRate))
	{
		D_PRINTF(debugfile, "doWebmailEnd: dropped connection\n");
	}
	else
	{
		event_start(ptcx, &stats->logout);
		if (web_discard(ptcx, cmd, cmd->webLogout, user) < 0)
		{
			D_PRINTF(stderr, "doWebmailEnd: logout failed: %s\n",
				 strerror(errno));
			stats->logout.errs++;
		}
		event_stop(ptcx, &stats->logout);
	}
	clear_cookies(user);
	delete_web_user(user);
}

