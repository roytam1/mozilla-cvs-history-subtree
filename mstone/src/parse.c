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
/*
 * parse the workload description file
 *
 */

#include "bench.h"
#include "xalloc.h"

/* global variables */
param_list_t	*g_default_params;	/* list of default values */


/* Find a protocol in the global protocol list */
protocol_t *
protocol_get (char *name)
{
    protocol_t	*pp;

    for (pp=g_protocols; pp->name != NULL; ++pp) {
	/* we only check the length of the registered protocol name */
	/* that way NAME can have additonal stuff after it */
	/* This is much better than the old single letter checking */
	if (0 == strnicmp (name, pp->name, strlen (pp->name)))
	    return pp;
    }
    return NULL;
}

/*
**  SIZE_ATOF(tok) -- convert a string Nx to bytes
**
**  x is 'm' for megabytes, 'k' for kilobytes, or 'b' for bytes
*/
double
size_atof(tok)
    const char *tok;
{
    char *endptr;
    double ret;
    if(*tok == '\0')
    {
	fprintf(stderr, "Empty value in size_atof.\n");
	exit(-1);
    }
    ret = strtod(tok, &endptr);
    while(*endptr && isspace(*endptr))
	endptr++;
    switch(*endptr)
    {
    case 'b': case 'B':
    case '\0':
	break;
    case 'k': case 'K':
	ret *= 1024;
	break;
    case 'M': case 'm':
	ret *= 1024 * 1024;
	break;
    default:
	fprintf(stderr, "Invalid size specifier '%s'\n", endptr);
	exit(-1);
    };
    return ret;
}

/*
  converts a string of the form #x to seconds
  where x can be a unit specifier of
  d or D	days
  h or H	hours
  m or M	minutes
  s or S	seconds
  the default is seconds
*/
int
time_atoi(const char *pstr)
{
    int ret=0;
    int len = strlen(pstr);

    if (!pstr[0]) return 0;
    switch (pstr[len-1]) {
    case 'd': /* days */
    case 'D':
	ret = 24 * 60 * 60 * atoi(pstr);
	break;
    case 'h': /* hours */
    case 'H':
	ret = 60 * 60 * atoi(pstr);
	break;
    case 'm': /* minutes */
    case 'M':
	ret = 60 * atoi(pstr);
	break;
    case 's': /* seconds */
    case 'S':
    default:
	ret = atoi(pstr);
	break;
    }
    return ret;
}

/*
  converts a string of the form #x to milliseconds
  where x can be a unit specifier of
  d or D	days
  h or H	hours
  m or M	minutes
  s or S	seconds
  the default is seconds
*/
int
millitime_atoi(const char *pstr)
{
    int ret=0;
    int len = strlen(pstr);

    if (!pstr[0]) return 0;
    switch (pstr[len-1]) {
    case 'd': /* days */
    case 'D':
	ret = 24 * 60 * 60 * 1000 * atoi(pstr);
	break;
    case 'h': /* hours */
    case 'H':
	ret = 60 * 60 * 1000 * atoi(pstr);
	break;
    case 'm': /* minutes */
    case 'M':
	ret = 60 * 1000 * atoi(pstr);
	break;
    case 's': /* seconds */
    case 'S':
	ret = 1000 * atoi(pstr);
	break;
    default:
	ret = atoi(pstr);
	break;
    }
    return ret;
}

/* 
  Simple keyword indexed string storage
  This kind of thing has been invented many times.  Once more with gusto!
*/
param_list_t *
paramListInit (void)
{
    param_list_t *np = XCALLOC (struct param_list);
    return np;
}

/* paramListAdd returns: 1 update existing value, 0 new, -1 out of memory */
int
paramListAdd (param_list_t *list,
	   const char *name,
	   const char *value)
{
    param_list_t *pl;
    int	found = 0;
    assert (list != NULL);
    assert (name != NULL);
    assert (value != NULL);

    if (list->name == NULL) {		/* first one special case */
	list->name = xstrdup (name);
	list->value = xstrdup (value);
	return 0;
    }
	
    for (pl = list; pl->next; pl = pl->next) {
	if (strcmp (pl->name, name) == 0) {
	    found = 1;
	    break;
	}
    }
    if (found) {
	xfree (pl->value);
	pl->value = xstrdup (value);
	return 1;
    } else {
	param_list_t *np = XCALLOC (struct param_list);
	np->name = xstrdup (name);
	np->value = xstrdup (value);
	pl->next = np;
	return 0;
    }
}

/* paramListGet returns value or NULL */
char *
paramListGet (param_list_t *list,
		const char *name)
{
    param_list_t *pl;
    assert (list != NULL);
    assert (name != NULL);

    for (pl = list; pl->next; pl = pl->next) {
	if (strcmp (pl->name, name) == 0) {
	    return pl->value;
	}
    }
    return NULL;
}

/* 
  Simple keyword indexed string storage
  This kind of thing has been invented many times.  Once more with gusto!
*/
string_list_t *
stringListInit (const char *value)
{
    string_list_t *np = XCALLOC (struct string_list);
    if (value)
	np->value = xstrdup (value);		/* This can be NULL */
    else
	np->value = NULL;
    return np;
}

void
stringListFree (string_list_t *list)
{
    string_list_t *pl, *next;
    assert (list != NULL);

    for (pl = list; pl; pl = next) {
	next = pl->next;
	if (pl->value)
	    xfree (pl->value);
	xfree (pl);
    }
}

/* stringListAdd returns: 0 new, -1 out of memory */
int
stringListAdd (string_list_t *list,
	   const char *value)
{
    string_list_t *pl;
    string_list_t *np;
    assert (list != NULL);
    assert (value != NULL);

    if (list->value == NULL) {		/* first one special case */
	list->value = xstrdup (value);
	return 0;
    }

    for (pl = list; pl->next; pl = pl->next)
	; /* skip to end */
    np = XCALLOC (struct string_list);

    np->value = xstrdup (value);

    pl->next = np;
    return 0;
}

void dump_sl(string_list_t *l)
{
    while (l)
    {
	fprintf(stderr, "`%s', ", l->value);
	l = l->next;
    }
    fprintf(stderr, "NULL\n");
}

/* return non 0 if whitespace */
#define IS_WHITE(c)	((' ' == (c)) || '\t' == (c))
#define IS_TERM(c)	(('\r' == (c)) || '\n' == (c))
#define IS_NUM(c)	(('0' <= (c)) && ('9' >= (c)))

/*
  Get a 'line' from a text buffer (terminated by CR-NL or NL)
  Handle comments.
  Skip empty lines.
  Handle continued lines (backslash as last character)

  Return the pointer to the next line (or NULL at end)
 */
char *
get_line_from_buffer (char *buffer,	/* buffer to sort through */
		      char *line,	/* line buffer to fill in */
		      int *lineCount,	/* count of lines */
		      int continuation) /* part of previous line */
{
    char *end, *start;
    char *next;
    int		len;
    
    assert (buffer != NULL);
    assert (line != NULL);

    next = strchr (buffer, '\n');
    if (next) {
	++next;			/* start point for next line */
    } else if (strlen (buffer) > 0) {
	d_printf (stderr, "Last line missing NEWLINE\n");
    } else {				/* done with buffer */
	*line = 0;
	return NULL;
    }
    if (lineCount) ++(*lineCount);	/* increment line count */

    if (continuation) {
	/* continuation lines keep everything */
	start = buffer;
	end = next-1;
    } else {
	/* find usefull text */
	len = strcspn (buffer, "#\r\n");
	if (!len) {				/* empty line, go to next */
	    return get_line_from_buffer (next, line, lineCount, continuation);
	}
	/* skip trailing white too */
	for (end = buffer + len; end > buffer; --end) {
	    if (!IS_WHITE (end[-1])) break;
	}
	if (end == buffer) {			/* blank line */
	    return get_line_from_buffer (next, line, lineCount, continuation);
	}

	/* find leading whitespace */
	start = buffer + strspn (buffer, " \t");
    }

    assert (end > start);		/* we should have found blank above */

    strncpy(line, start, end - start); /* get a line */
    line[end-start] = '\0';

    if (line[end-start-1] == '\\') { /* continuation line */
	return get_line_from_buffer (next, &line[end-start-1], lineCount, 1);
    }

    D_PRINTF(stderr, "clean line [%s]\n", line);

    return next;
}

/*
  return 1 if for us, 0 if everyone, -1 if not for us
*/
int
check_hostname_attribute (const char *line)
{
    char *hostp; 
    /* Section open, check if for us */
    if ((hostp = strstr(line, "hosts="))
	|| (hostp = strstr(line, "HOSTS="))) {
	hostp = strchr (hostp, '='); /* skip to the equals */
	assert (hostp != NULL);

	/* look for our hostname */
	if (strstr(hostp, gs_parsename) != NULL) {
	    /* should check for complete string match */
	    return 1;
	} else {
	    return -1;
	}
    }

    /* not host specific */
    return 0;
}

double
d_millitime_atoi(str)
    char *str;
{
    return (double)millitime_atoi(str);
}

/*
  Protocol independent parsing
*/
int
cmdParseNameValue (pmail_command_t cmd,
		    char *name,
		    char *tok)
{
    if (strcmp(name, "numloops") == 0)
	cmd->numLoops = atoi(tok);
    else if (strcmp(name, "weight") == 0)
	cmd->weight = atoi(tok);
    /* Sean O'Rourke: added startdelay */
    else if (strcmp(name, "startdelay") == 0)
	cmd->startDelay = parse_distrib(tok, &d_millitime_atoi);
    else if (strcmp(name, "idletime") == 0)
	cmd->idleTime = parse_distrib(tok, &d_millitime_atoi);
    else if (strcmp(name, "blockid") == 0)
	cmd->blockID = time_atoi(tok);
    else if (strcmp(name, "blocktime") == 0)
	cmd->blockTime = parse_distrib(tok, &d_millitime_atoi);
    else if (strcmp(name, "loopdelay") == 0)
	cmd->loopDelay = parse_distrib(tok, &d_millitime_atoi);
    else if (strcmp(name, "checkmailinterval") == 0)
	cmd->loopDelay = parse_distrib(tok, &d_millitime_atoi);
#ifdef DYNAMIC_THROTTLE
    else if (strcmp(name, "loopthrottle") == 0)
	cmd->loopThrottle = millitime_atoi(tok);
    else if (strcmp(name, "throttlefactor") == 0)
	cmd->throttleFactor = atof(tok);
#endif /* DYNAMIC_THROTTLE */
    else
	return 0;			/* no match */

    return 1;				/* matched it */
}

/* 
 * count_num_commands()
 * given a commandsfile string, count the valid command for us.
   TODO Dynamically load the protocols found
 */
static int
count_num_commands(char *commands)
{
    char  *cmdptr = commands;
    int	num_comms = 0;
    char 	line[LINE_BUFSIZE];

    /*  
     * parse through the string line-by-line,strip out comments
     */
    /*D_PRINTF(stderr, "count_num_commands[%s]\n", commands);*/

    while (NULL != (cmdptr = get_line_from_buffer (cmdptr, line, NULL, 0))) {
	/* increment count if we've hit a open tag (e.g. <SMTP...) and
	   it's not the <Default> tag, or <Graph> tag */
	if (line[0] != '<') continue;	/* not an open */
	if (line[1] == '/') continue;	/* a close */
	if (0 == strnicmp (line+1, "default", 7)) continue; /* default */

					/* should already be filtered out */
	if (0 == strnicmp (line+1, "graph", 5)) continue; /* graph */

	/* Section open, check if for us */
	if (check_hostname_attribute (line) < 0) {
	    D_PRINTF (stderr, "count_num_commands: section not for us '%s'\n",
		      line);
	    continue;
	}

					/* find protocol */
	if (NULL == protocol_get (line+1)) {
					/* TODO load handler */

					/* TODO add to protocol list */

	    D_PRINTF (stderr, "count_num_commands: No handler for '%s'\n",
		      line);
	    continue;
	}

	D_PRINTF(stderr, "count_num_commands: Found section '%s'\n", line);
	num_comms++;
    }

    return (num_comms);
} 

char *
string_tolower(char *string)
{
    if (string == NULL)
     return NULL;

    /* convert to lower case */
    for (; *string != '\0'; ++string) {
	*string = tolower (*string);
    }
    return string;
}

char *
string_unquote(char *string)
{
    int len, num;
    char *from, *to;

    if (string == NULL)
	return NULL;

    len = strlen(string);

    if (string[0] == '"' && string[len-1] == '"') {
	/* remove matching double-quotes */
	string[len-1] = '\0';
	++string;
    }

    /* replace quoted characters (and decimal codes) */
    /* assuming line-continuation already happened */
    from = to = string;
    while (*from) {
	if (*from == '\\') {
	    ++from;
	    if (IS_NUM(*from)) {
		num = *from++ - '0';
		if (IS_NUM(*from))
		    num = num*10 + (*from++ - '0');
		if (IS_NUM(*from))
		    num = num*10 + (*from++ - '0');
		*to++ = num;
	    } else {
		switch (*from) {
		case '\0': continue;
		case 'n': *to++ = '\n'; break;
		case 'r': *to++ = '\r'; break;
		case 't': *to++ = '\t'; break;
		default: *to++ = *from; break;
		}
		++from;
	    }
	} else {
	    *to++ = *from++;
	}
    }
    *to = '\0';

    return string;
}

/* 
 * load_commands()
 * Parse the commlist file again, this time getting the commands, filenames
 * and weights and reading the message files into memory.
 */
int
load_commands(char *commands)
{
    char  *cmdptr = commands;

    int	    total_weight = 0;
    char    line[LINE_BUFSIZE];
    int	    lineNumber = 0;
    int	    commIndex = 0;
    int    inCommand = 0;	/* 0 none, -1 ignore, 1 default, 2 other */
    string_list_t	*param_list = NULL;
    int i;

    g_default_params = paramListInit (); /* create default section list */

    /* set built in defaults. always use lower case names */
    paramListAdd (g_default_params, "numloops", "1");
    paramListAdd (g_default_params, "numrecipients", "1");
    paramListAdd (g_default_params, "numlogins", "1");
    paramListAdd (g_default_params, "numaddresses", "1");
    paramListAdd (g_default_params, "weight", "100");
#ifdef SOCK_LINESPEED
    paramListAdd (g_default_params, "latency", "0");
    paramListAdd (g_default_params, "bandwidth", "0");
#endif
#ifdef AUTOGEN
    paramListAdd (g_default_params, "size", "0");
    paramListAdd (g_default_params, "headers", "5");
    paramListAdd (g_default_params, "mime", "0");
#endif
    paramListAdd (g_default_params, "throttlefactor", "1.05");

    paramListAdd (g_default_params, "timeout", "60s");
    
    gn_number_of_commands = count_num_commands(commands);
    D_PRINTF(stderr, "number_of_commands = %d\n", gn_number_of_commands);
    if (gn_number_of_commands <= 0) {	/* no mail msgs - exit */
      return returnerr (stderr, "No commands found\n");
    }

    /* allocate structure to hold command list (command filename weight) */
    g_loaded_comm_list = 
      (mail_command_t *) xcalloc(gn_number_of_commands * sizeof(mail_command_t));
    /* put in "internal" defaults */
    for (i = 0; i < gn_number_of_commands; i++) {
	g_loaded_comm_list[i].throttle = 1; /* dynamic throttle rate */
    }
    
    while (NULL
	   != (cmdptr = get_line_from_buffer (cmdptr, line, &lineNumber, 0))) {

	/* The pre-process step does lots of checking, keep this simple */
	/* check for close tag */
	if ((line[0] == '<') && (line[1] == '/')) {
	    /* default or ignored command */
	    if (inCommand < 2) {
		assert (param_list == NULL);
		inCommand = 0;
		continue;
	    }
	    stringListAdd (param_list, line);	/* store last line */

	    if (g_loaded_comm_list[commIndex].proto->parseEnd) {
		int ret;
		ret = (g_loaded_comm_list[commIndex].proto->parseEnd)
		    (g_loaded_comm_list+commIndex,
		     param_list, g_default_params);

		if (ret < 0) {
		    D_PRINTF (stderr, "Error finalizing section for '%s'\n",
			      line);
		    continue;
		}
	    }
	    g_loaded_comm_list[commIndex].proto->cmdCount++;
	    D_PRINTF (stderr, "Section done: '%s' weight=%d\n\n",
		      line, g_loaded_comm_list[commIndex].weight);
	    /* update total weight */
	    total_weight += g_loaded_comm_list[commIndex].weight;
		    
	    commIndex++;
	    inCommand = 0;
	    if (param_list) {
		stringListFree (param_list);
		param_list = NULL;
	    }
	    continue;
	}

	/* open tag */
	if (line[0] == '<') {
	    protocol_t	*pp;

	    if (check_hostname_attribute (line) < 0) { /* not for us */
		D_PRINTF (stderr, "Section not for us %s\n", line);
		inCommand = -1;
		continue;
	    }

	    /* Check if default special case */
	    if (0 == strnicmp (line+1, "default", 7)) { /* default */
		D_PRINTF (stderr, "DEFAULT section\n");
		inCommand = 1;
		continue;
	    }
	    /* Check if we should ignore it */
	    if (0 == strnicmp (line+1, "graph", 5)) { /* ignore graph */
		D_PRINTF (stderr, "GRAPH section (ignored)\n");
		inCommand = -1;
		continue;
	    }

	    pp = protocol_get (line+1);
	    if (NULL == pp) {		/* protocol not found */
		d_printf (stderr,
			  "Warning: Skipping section with no protocol handler '%s'\n",
			  line);
		continue;
	    }

	    if (pp->parseStart) {
		int		ret;
		ret = (pp->parseStart) (g_loaded_comm_list+commIndex,
					line, g_default_params);
		if (ret < 0) {
		    D_PRINTF (stderr, "Error Initializing section for '%s'\n",
			      line);
		    continue;
		} else if (ret == 0) {
		    D_PRINTF (stderr, "Ignoring section for '%s'\n",
			      line);
		    continue;
		}
	    }

	    /* start a command */
	    D_PRINTF (stderr, "New Section: %s\n", line);
	    g_loaded_comm_list[commIndex].proto = pp;
	    inCommand = 2;
	    param_list = stringListInit (line); /* store first line */
	    /* ignoring rest of line */
	    continue;
	}

	/* If we're not inside a command tag or not for us, ignore the line */
	if (inCommand <= 0) {
	    continue;
	}

	/* attr value */
	if (1 == inCommand) {		/* default, always name value pairs */
	    char *value;
	    value = line + strcspn (line, " \t=");
	    if (value != line) {
		*value++ = 0;		/* terminate name */
		value += strspn(value, " \t=");

		string_tolower(line);
		value = string_unquote(value);

		/*D_PRINTF (stderr, "DEFAULT: name='%s' value='%s'\n",
		  line, value);*/
		paramListAdd (g_default_params, line, value);
	    } else {
		D_PRINTF (stderr, "DEFAULT: Cound not find 'NAME VALUE...', line %d\n",
			  lineNumber);
	    }
	    continue;
	}

	/* store body for protocol parsing */
	stringListAdd (param_list, line);
    }
    return (total_weight);
}

