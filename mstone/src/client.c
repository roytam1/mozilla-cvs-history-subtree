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
   client.c handles thread creation
   login, loop, logout
   status report generation
   throttling (not supported yet)
   thread joining
 */

#include "bench.h"
#include "xalloc.h"

static int	StartedThreads = 0;   /* counter semaphore for children */
static int	FinishedThreads = 0;   /* counter semaphore for children */

#ifdef _WIN32
#define DEBUG_FILE	"mstone-debug"
#define LOG_FILE	"mstone-log"
#else
#define DEBUG_FILE	"mstone-debug"
#define LOG_FILE	"mstone-log"
#endif /* _WIN32 */

/*
  This is a sleep that knows about test end.
  Should also do throttling.
  We dont check for signals because the only signal expected is test end
 */
void
MS_idle(ptcx_t ptcx, int idleMillis)
{
    int secsLeft = 0;

    secsLeft = gt_shutdowntime - time(0L);

    D_PRINTF(debugfile, "secsLeft=%d, idleSecs=%d\n",
	     secsLeft, idleMillis/1000);

    if (secsLeft <= 0) {			/* time is up, start exiting */
	if (gf_timeexpired < EXIT_SOON)
	    gf_timeexpired = EXIT_SOON;
	return;
    }

    if (idleMillis/1000 > secsLeft)
	idleMillis = secsLeft*1000;
    
    if (idleMillis <= 0)
	return;

    MS_usleep(idleMillis*1000);
}

/*
**  DELAY -- calculate delay time
**
**	Delay for the rest of the time the block was supposed to take,
**	_or_ for the full delay time if we have run over.
*/

static int
delay(delay, start_time)
	int delay;
	int start_time;
{
	int cmd_time = (time(0) - start_time)*1000;
	if (delay >= cmd_time)
		delay -= cmd_time;
	return delay;
}

/* 
 * Perform the given command block
 *
 * commNum = the number of the comm (offset in loaded_comm_list[])
 *
 * returns 1
 */
static int 
do_command(ptcx_t ptcx,			/* thread state */
	   int commNum)			/* command block number */

{
    mail_command_t *comm = &g_loaded_comm_list[commNum];
    cmd_stats_t *cmd_stats =  &(ptcx->cmd_stats[commNum]);
    int cnt, cntEnd;			/* loop counters */
    void *state = NULL;
    int loop_delay, block_time;		/* Sean O'Rourke: added */
    int block_start;

    cntEnd = comm->numLoops+1; /* transfer count +1 */
    block_start = (int)time(0);
    D_PRINTF(debugfile, "do_command start t=%d commNum=%d cntEnd=%d\n",
	     block_start, commNum, cntEnd);

    /* Start and End are special loop cases to make summations easier */
    for (cnt = 0; cnt <= cntEnd; cnt++) { 

	if (gf_timeexpired >= EXIT_FAST) /* no more calls */
	    break;
	if (gf_timeexpired >= EXIT_SOON) {
	    if (!state) break;		/* no shutdown to do */
	    cnt = cntEnd;		/* go to shutdown */
	}

	D_PRINTF(debugfile, "do_command t=%lu count=%d of %d\n",
		 time(0L), cnt, cntEnd);

	if (0 == cnt) {			/* first time */
	    int idle_time;
	    if ((idle_time = sample(comm->startDelay)) > 0) {
		int real_start = gt_startedtime + idle_time / 1000;
		if (real_start > block_start) {
		    MS_idle(ptcx, (real_start - block_start)*1000);
		    block_start = real_start;
		}
	    }
	    cmd_stats->totalcommands++; /* track command blocks trys */
	    state = (*(comm->proto->cmdStart)) (ptcx, comm, cmd_stats);
	    if (NULL == state) {
		D_PRINTF(debugfile, "do_command Start returned NULL\n");
		break;
	    }
	    idle_time = sample(comm->idleTime);
	    
	    if (idle_time && (gf_timeexpired < EXIT_SOON)) {
		idle_time -= (time(0) - block_start)*1000;
		if(idle_time > 0) {
			D_PRINTF(debugfile,"do_command delay %d after Setup\n",
				 idle_time);
			event_start(ptcx, &cmd_stats->idle);
			MS_idle(ptcx, idle_time);
			event_stop(ptcx, &cmd_stats->idle);
		} else {
			D_PRINTF(debugfile,
				 "Warning: behind %d msec for start\n",
				 -idle_time);
		}
	    }  
	} else if ((cntEnd == cnt) && comm->proto->cmdEnd) { /* last */
	    (*(comm->proto->cmdEnd)) (ptcx, comm, cmd_stats, state);
	    break;			/* done with loop */
	}
	else if (comm->proto->cmdLoop) { /* do transfers */
	    int rc;
	    int loop_start = time(0);
	    rc = (*(comm->proto->cmdLoop)) (ptcx, comm, cmd_stats, state);
	    if (rc < 0) {
		D_PRINTF(debugfile, "do_command Loop returned error/done\n");
		cnt = cntEnd -1;	/* end loop */
	    }
					/* do loopDelay even if error/done */

	    loop_delay = sample(comm->loopDelay);
	    
	    if (loop_delay > 0 && (gf_timeexpired < EXIT_SOON)) {
		    loop_delay = delay(loop_delay, loop_start);
		    if(loop_delay > 0) {
			    D_PRINTF(debugfile,"do_command delay %d in loop\n",
				     loop_delay);
			    event_start(ptcx, &cmd_stats->idle);
			    MS_idle(ptcx, loop_delay);
			    event_stop(ptcx, &cmd_stats->idle);
		    }
	    }
	}
    }

    block_time = sample(comm->blockTime);

    /* XXX: semantic change: blockTime now includes command time. */
    /* do blockTime even if we hit an error connecting */
    if ((block_time > 0) && (gf_timeexpired < EXIT_SOON)) {
	block_time = delay(block_time, block_start);
	if(block_time > 0) {
		D_PRINTF(debugfile,"do_command delay %d after Block\n",
			 block_time);
		event_start(ptcx, &cmd_stats->idle);
		MS_idle(ptcx, block_time);
		event_stop(ptcx, &cmd_stats->idle);
	}
    }
    D_PRINTF(debugfile, "do_command end t=%lu commNum=%d cntEdn=%d\n",
	     time(0L), commNum, cntEnd);

    return 1;				/* return connections */
} /* END do_command() */

/*
  Initialize sub process context
*/
int
clientInit(ptcx_t ptcx)
{
    ptcx->dfile = stderr;
 
    if (gn_debug) {
	/* open a debug log file */
	char debug_file_name[255];
	fflush(stderr);
	if (ptcx->threadnum >= 0) {
	    sprintf(debug_file_name, "%s.%d.%d",
		    DEBUG_FILE, ptcx->processnum, ptcx->threadnum);
	} else {
	    sprintf(debug_file_name, "%s.%d",
		    DEBUG_FILE, ptcx->processnum);
	}
	ptcx->dfile = fopen(debug_file_name, "w+");
	if (ptcx->dfile == NULL) {
	    gn_debug = 0;
	    d_printf (stderr, "Can't open debug file.  Debug mode disabled\n");
	}
	D_PRINTF(debugfile, "Running in debug mode\n");
    }

    if (gn_record_telemetry) {
	/* open a transaction log file. */
	char log_file_name[255];
	sprintf(log_file_name, "%s.%d.%d",
		LOG_FILE, ptcx->processnum, ptcx->threadnum);
	ptcx->logfile = open(log_file_name,
			     O_CREAT | O_TRUNC | O_WRONLY, 0664);
	returnerr(debugfile,"Log file is %s [%d]\n", log_file_name, ptcx->logfile);
    } else {
	ptcx->logfile = -1;
    }

    /* Initialize random number generator */
    SRANDOM(ptcx->random_seed);
    D_PRINTF(debugfile, "Random seed: 0x%08x\n", ptcx->random_seed );

    return 0;
}

/* Continuously handle command blocks until time is up
 */
int
clientLoop(ptcx_t ptcx)
{
    for(ptcx->blockCount = 0; gf_timeexpired < EXIT_SOON; ) {
	int comm_index = 0;

	if (gn_number_of_commands > 1) {
	    int ran_number;
	    /* Handle the weighted distribution of commands */
	    /* HAVE FILELIST */

	    D_PRINTF(debugfile, "Total weight %d\n", gn_total_weight);
	    /* random number between 0 and totalweight-1 */
	    ran_number = (RANDOM() % gn_total_weight);
	    D_PRINTF(debugfile, "random %ld\n", ran_number );
		
	    /* loop through pages, find correct one 
	     * while ran_number is positive, decrement it
	     * by the weight of the current page
	     * example: ran_number is 5, pages have weights of 10 and 10
	     *          first iteration comm_index = 0, ran_number = -5
	     *          iteration halted, comm_index = 0
	     */
	    comm_index = -1;
	    while (ran_number >= 0) {
		comm_index++;
		D_PRINTF(debugfile, "Current command index %d: %ld - %d\n",
			 comm_index, ran_number, 
			 g_loaded_comm_list[comm_index].weight
		    );
		ran_number -= g_loaded_comm_list[comm_index].weight;
	    } 

	    if (comm_index >= gn_number_of_commands) { /* shouldnt happen */
		D_PRINTF(debugfile, "Command weight overrun %d %d\n",
			 ran_number, gn_total_weight);
		comm_index--;
	    }
	}

	/* run the command */
	ptcx->connectCount += do_command(ptcx, comm_index);
	++ptcx->blockCount;

	if (gf_timeexpired >= EXIT_SOON) break;	/* done, dont throttle */

	/* For the single processes/thread case, this should be exact */
	if ((gn_maxBlockCnt)		/* check for max loops */
	    && (ptcx->blockCount >= gn_maxBlockCnt)) {
	    D_PRINTF (debugfile, "Saw enough loops %d, exiting\n",
		      ptcx->blockCount);
	    beginShutdown ();		/* indicate early exit */
	    break;
	}

    } /* END while blockCount */

    return 0;
}

/*
  Thread of execution.
  Also works in un-threaded case.
  Initialize, do some system housekeeping, run test, housekeeping, clean up 
*/
THREAD_RET
clientThread(void *targ)
{
    time_t	currentTime;
    struct tm	*tmptm;
    char	timeStamp[DATESTAMP_LEN];
    int		ret = 0;
    tcx_t	*ptcx = (ptcx_t)targ;

    if (clientInit(ptcx)) {		/* should never fail */
#ifdef USE_PTHREADS
	if (ptcx->threadnum >= 0)
	    pthread_exit((void *)((1 << 16) | ptcx->threadnum));
	return NULL;
#else
	return;
#endif
    }

    if (gn_debug) {
	/* write current time to debug file */
	time(&currentTime);
	tmptm = localtime(&currentTime); 
	strftime(timeStamp, DATESTAMP_LEN, "%Y%m%d%H%M%S", tmptm);
	D_PRINTF(debugfile, "Time Stamp: %s\n", timeStamp);
	D_PRINTF(debugfile, "mailstone run dateStamp=%s\n", gs_dateStamp);
    }

#ifdef _WIN32
#define WAIT_INFINITY 100000000
    /* Tell parent we're ready */
    InterlockedIncrement(&StartedThreads);
#else
    ++StartedThreads;			/* thread safe??? -- no, it's not. */
#endif /* _WIN32 */
    
    D_PRINTF(debugfile, "entering clientLoop\n");

    ret = clientLoop(ptcx);		/* do the work */

    D_PRINTF(debugfile, "Test run complete\n" );

    /* write current time to debug file */
    time(&currentTime);
    tmptm = localtime(&currentTime); 
    strftime(timeStamp, DATESTAMP_LEN, "%Y%m%d%H%M%S", tmptm);
    D_PRINTF(debugfile, "Time Stamp: %s\n", timeStamp);

    if (gn_record_telemetry && (ptcx->logfile > 0)) {
	close(ptcx->logfile);
	ptcx->logfile = -1;
    }

    D_PRINTF(debugfile, "client exiting.\n" );

    if (gn_debug && ptcx->dfile) {
	fflush(ptcx->dfile);
	if (ptcx->dfile != stderr) {
	    fclose(ptcx->dfile);
	    ptcx->dfile = stderr;
	}
    }

#ifdef _WIN32
    /* tell parent we're done */
    InterlockedIncrement(&FinishedThreads);
#else  /* _WIN32 */
    ++FinishedThreads;			/* thread safe??? -- once again, no */
#ifdef USE_PTHREADS
    if (ptcx->threadnum >= 0)
	pthread_exit((void *)((ret << 16) | ptcx->threadnum));
#endif
    return NULL;
#endif /* _WIN32 */
} /* END clientThread() */

/*
  The FORMAT format is:
  FORMAT: client=<NUMBER> <TYPE>:<NAME>\t<VALUE>

  TYPE is one of: TIMER, PROTOCOL, LINE

  Everything breaks down to attribute=value pairs.
  "attribute=" is literal, the listed value names the field

  Attribute names must be unique with each timer.

  The [] and {} indicate timers and protocols respectively.
  everything else is a literal (including whitespace).
  The protocols and timers will be expanded with simple text
  substitutions.

  The literal text is as high in the description as possible.
  The processing splits out the values based on surrounding text.
*/
/*
   Output the format that the summaries will be in.
   This call the protocol specific formats, then outputs
   the complete line formats
 */
void
clientSummaryFormat(int clientnum, int outfd)
{
    char 	buf[SIZEOF_SUMMARYTEXT], *cp;
    char	extra[96];
    protocol_t	*pp;

    sprintf (extra, "client=%d", clientnum); /* extra stuff on each line */

    /* Define the contents of each protocol */
    for (pp=g_protocols; pp->name != NULL; ++pp) {
	if (!pp->cmdCount) continue;	/* not used */

	(pp->statsInit)(NULL, &(pp->stats), 0, 0); /* init stats (HERE?) */

	(pp->statsFormat)(pp, extra, buf); /* output lines of format info */
	sendOutput(outfd, buf);
    }

    /* Define the periodic update summaries */
    /* This is the most common message, so keep is as short as practical */
    cp = buf;
    sprintf(cp,
	    "<FORMAT %s LINE=SUMMARY-TIME><TS %s>",
	    extra,
	    "client=client t=time blocks=blocks");
    for (; *cp; ++cp);	/* skip to the end of the string */
    for (pp=g_protocols; pp->name != NULL; ++pp) {
	if (!pp->cmdCount) continue;	/* not used */
	sprintf(cp, "\t<%s {%s}/>", pp->name, pp->name);
	for (; *cp; ++cp);		/* skip to the end of the string */
    }
    strcat (cp, "</TS></FORMAT>\n");
    sendOutput(outfd, buf);

    /* BLOCK-STATISTICS format */
    for (pp=g_protocols; pp->name != NULL; ++pp) {
	cp = buf;
	sprintf(cp,
		"<FORMAT %s LINE=BLOCK-STATISTICS-%s><BS-%s %s>",
		extra, pp->name, pp->name,
		"client=client thread=thread t=time blockID=blockID");
	for (; *cp; ++cp);		/* skip to the end of the string */

	sprintf(cp, "\t<%s {%s}/>", pp->name, pp->name);
	for (; *cp; ++cp);		/* skip to the end of the string */
	sprintf (cp, "</BS-%s></FORMAT>\n", pp->name);
	sendOutput(outfd, buf);
    }

    /* Notice Format */
    sprintf(buf,
	    "<FORMAT %s LINE=NOTICE-1>%s</FORMAT>\n",
	    extra,
	    "<NOTICE client=client ... ");
    sendOutput(outfd, buf);

    /* Abort Format */
    sprintf(buf,
	    "<FORMAT %s LINE=NOTICE-2>%s</FORMAT>\n",
	    extra,
	    "<ABORT client=client ... ");
    sendOutput(outfd, buf);
}

/*
   Output the block information for each thread
 */
void
clientBlockSummary(ptcx_t ptcxs, int ntcxs, int clientnum, int outfd)
{
    time_t 	curtime;
    protocol_t	*pp;
    int 	jj, kk;
    char 	buf[SIZEOF_SUMMARYTEXT], *bp;

    D_PRINTF(stderr, "clientSummaryBS starting.\n" );
    curtime = time(0L);
    for (kk = 0; kk < gn_number_of_commands; ++kk) { /* all commands */
	for (jj = 0; jj < ntcxs; ++jj) { /* all threads */
	    if (0 == ptcxs[jj].cmd_stats[kk].totalcommands) /* nothing */
		continue;
	    pp = g_loaded_comm_list[kk].proto;
					/* output proto independent part */
	    bp = buf;
	    sprintf (bp,
		     "<BS-%s client=%d thread=%d t=%lu blockID=%d>",
		     pp->name, clientnum, jj, curtime,
		     g_loaded_comm_list[kk].blockID);
	    for (; *bp; ++bp);	/* find end of buffer */

	    sprintf (bp,
		     "\t<%s blocks=%ld ",
		     pp->name,
		     ptcxs[jj].cmd_stats[kk].totalcommands);
	    for (; *bp; ++bp);	/* find end of buffer */

	    (pp->statsOutput)(pp, &(ptcxs[jj].cmd_stats[kk]), bp);
	    for (; *bp; ++bp);	/* find end of buffer */

	    sprintf (bp, "/></BS-%s>\n", pp->name); /* end it */
	    sendOutput(outfd, buf);
	}
    }
    D_PRINTF(stderr, "clientSummaryBS done.\n");
}

/* Output the periodic activity summary */
void
clientTimeSummary(ptcx_t ptcxs, int ntcxs, int clientnum, int outfd)
{
    time_t 	curtime;
    int 	blockCount = 0, connectCount = 0;
    protocol_t	*pp;
    int 	jj, kk;
    char 	buf[SIZEOF_SUMMARYTEXT];
    char 	rqsttextbuf[SIZEOF_RQSTSTATSTEXT+1];
    static int	oldThreadStarts= 0;
    static int	oldThreadEnds= 0;

    D_PRINTF(stderr, "clientTimeSummary starting.\n" );
    curtime = time(0L);

    for (pp=g_protocols; pp->name != NULL; ++pp) { /* zero protocol stats */
	if (!pp->cmdCount) continue;	/* not used */
	cmdStatsInit (&(pp->stats));	/* clear proto independent part */
	(pp->statsInit)(NULL, &(pp->stats), 0, 0); /* clear proto part */
    }

    /* sum by protocol all commands, all threads */
    for (jj = 0; jj < ntcxs; ++jj) {
	blockCount += ptcxs[jj].blockCount;
	connectCount += ptcxs[jj].connectCount;
	for (kk = 0; kk < gn_number_of_commands; ++kk) {
	    pp = g_loaded_comm_list[kk].proto;
	    (pp->statsUpdate)(pp, &pp->stats, &(ptcxs[jj].cmd_stats[kk]));
	}
    }

					/* output proto independent part */
    sprintf(buf, "<TS client=%d t=%lu blocks=%d>",
	    clientnum, curtime, blockCount);

    for (pp=g_protocols; pp->name != NULL; ++pp) { /* output proto parts */
	if (!pp->cmdCount) continue;	/* not used */
	(pp->statsOutput)(pp, &pp->stats, rqsttextbuf);
	/* The \t seperates sections for report parsing */
	sprintf(&buf[strlen(buf)], "\t<%s blocks=%ld %s/>",
		pp->name, pp->stats.totalcommands,
		rqsttextbuf);
    }
    strcat(buf, "</TS>\n");		/* end it */
    sendOutput(outfd, buf);

					/* do additional status updates */
    if (oldThreadStarts != StartedThreads) {
	sprintf(buf, "<NOTICE client=%d threadStarts=%d/>\n",
		clientnum, StartedThreads - oldThreadStarts);
	sendOutput(outfd, buf);
	oldThreadStarts = StartedThreads;
    }

    if (oldThreadEnds != FinishedThreads) {
	sprintf(buf, "<NOTICE client=%d threadFinishes=%d/>\n",
		clientnum, FinishedThreads - oldThreadEnds);
	sendOutput(outfd, buf);
	oldThreadEnds = FinishedThreads;
    }

    if (gn_maxerrorcnt) {		/* check for max error count */
	int	errors = 0;
	for (pp=g_protocols; pp->name != NULL; ++pp) { /* sum total */
	    if (!pp->cmdCount) continue;	/* not used */
	    errors += pp->stats.combined.errs;
	}

	if (errors > gn_maxerrorcnt) {
	    returnerr (stderr,
		       "<ABORT client=%d errorCount=%ld errorLimit=%ld/>\n",
		       clientnum, errors, gn_maxerrorcnt);
	    beginShutdown ();
	}
    }

    if ((gn_maxBlockCnt)
	&& (blockCount >= gn_maxBlockCnt)) { /* check for max loops */
	returnerr (stderr,
		   "<ABORT client=%d blockCount=%ld blockLimit=%ld/>\n",
		   clientnum, blockCount, gn_maxBlockCnt);
	beginShutdown ();
    }

    D_PRINTF(stderr, "clientTimeSummary done.\n");
}

/*
  Thread that calls clientTimeSummary at the right rate
*/
THREAD_RET
summaryThread(void *targ)
{
    ptcx_t	ptcxs = (ptcx_t)targ; /* thread contexts */

    D_PRINTF(stderr, "summaryThread starting (ptcxs = 0x%x)...\n", ptcxs);

    /* client threads running...dump periodic stats */
    while (gn_feedback_secs && (gf_timeexpired < EXIT_FAST)) {
	D_PRINTF(stderr, "client %d: clientTimeSummary\n", ptcxs[0].processnum);
	clientTimeSummary(ptcxs, gn_numthreads, ptcxs[0].processnum, ptcxs[0].ofd);
	D_PRINTF(stderr, "client %d: waiting %d seconds before feedback\n",
		 ptcxs[0].processnum, gn_feedback_secs);
	MS_sleep(gn_feedback_secs);
    }    

    D_PRINTF(stderr, "summaryThread exiting...\n");

#ifdef USE_PTHREADS
    pthread_exit(0);
#endif

#ifndef _WIN32
    return NULL;
#endif
}

/*
  Initialize per thread context
*/
void
initTcx(ptcx_t ptcx, int ofd, int pnum, int tnum)
{
    int	kk;
    ptcx->processnum = pnum;
    ptcx->threadnum = tnum;
    ptcx->random_seed = (tnum << 16) + getpid();
    ptcx->cmd_stats = xcalloc(gn_number_of_commands * sizeof(struct cmd_stats));

					/* do PROTO specific init */
    for (kk = 0; kk < gn_number_of_commands; ++kk) {
	(g_loaded_comm_list[kk].proto->statsInit)
	    (&g_loaded_comm_list[kk], &(ptcx->cmd_stats[kk]), pnum, tnum);
    }

    ptcx->ofd = ofd;
    ptcx->sock = BADSOCKET_VALUE;
}

void
destroyTcx(ptcx_t ptcx)
{
    if (ptcx->cmd_stats) {
	xfree(ptcx->cmd_stats);
	ptcx->cmd_stats = 0;
    }
}

static time_t bailtime;

/* advance directly to shutdown phase.  May be called from signal handlers */
void
beginShutdown (void)
{
    if (gf_timeexpired >= EXIT_SOON) return; /* already shutting down */

    gf_timeexpired = EXIT_SOON;
    gt_shutdowntime = bailtime = time(0); /* advance end time to now */

    /* Changing aborttime probably has no effect (wrong process) */
    gt_aborttime = gt_shutdowntime + gt_stopinterval*2*EXIT_FASTEST;
}

/* This is the guts of each sub process.
   The socket for data output has already be setup.
   init context
   start threads (if possible/needed) with proper rampup delays
   wait for threads to end
 */

#ifdef USE_EVENTS
extern int gf_use_events;
int ev_clientProc(int pnum, int outfd,
		  unsigned int testtime,
		  unsigned int thread_stagger_usec);
#endif

int
clientProc(int pnum, int outfd,
	   unsigned int testtime,
	   unsigned int thread_stagger_usec)
{
    int tnum;
    int ret;
    ptcx_t	ptcxs; /* thread contexts */
    THREAD_ID summary_tid;
    int status;

#ifdef USE_EVENTS
    if (gf_use_events)
	return ev_clientProc(pnum, outfd, testtime, thread_stagger_usec);
#endif

    bailtime = gt_shutdowntime;

    returnerr(stderr, "Child starting\n"); /* get our pid and time printed */
    D_PRINTF(stderr, "clientProc(%d, %d, %d) starting\n",
	     pnum, testtime, thread_stagger_usec);

    if (testtime <= 0) {		/* never happens, checked in main.c */
	D_PRINTF (stderr, "ABORTING testtime=%d\n", testtime);
	return 0;
    }

    setup_signal_handlers ();
#ifndef _WIN32
    alarm(testtime);			/* promptly notice test end */
#endif

    clientSummaryFormat (pnum, outfd);

#if defined(USE_PTHREADS) || defined(_WIN32)
    if (gn_numthreads > 0) {
	ptcxs = (ptcx_t)xcalloc(sizeof(tcx_t) * gn_numthreads);

	for (tnum = 0; tnum < gn_numthreads; ++tnum) {
	    initTcx(&ptcxs[tnum], outfd, pnum, tnum);
	}

	if ((ret = sysdep_thread_create(&summary_tid, summaryThread,
					(void *)ptcxs)) != 0) {
	    returnerr(stderr, "client %d: summary thread create failed ret=%d errno=%d: %s\n",
		      pnum, ret, errno, strerror(errno));
	    ptcxs[tnum].tid = 0;
	}

	for (tnum = 0; tnum < gn_numthreads; ++tnum) {
	    if (gf_timeexpired)
		break;

	    /* sleep between each client thread we try to start */
	    if (tnum && thread_stagger_usec) {
		MS_usleep(thread_stagger_usec);
		if (gf_timeexpired)
		    break;
	    }

	    D_PRINTF(stderr, "client %d: thread %d testtime %d\n",
		     pnum, tnum, testtime);

	    if ((ret=sysdep_thread_create(&(ptcxs[tnum].tid), clientThread,
					  (void *)&ptcxs[tnum])) != 0) {
		returnerr(stderr, "client %d: thread %d create() failed ret=%d errno=%d: %s\n",
			  pnum, tnum, ret, errno, strerror(errno));
		ptcxs[tnum].tid = 0;
	    }
	    D_PRINTF(stderr, "client %d: thread %d created with ID %d\n",
		     pnum, tnum, ptcxs[tnum].tid);
	}

	/* threads are going, but wait for them to get through setup */
	while (StartedThreads < gn_numthreads) {
	    int tm = time(0);
	    if (tm > bailtime) {
		++gf_timeexpired;
		bailtime += gt_stopinterval;
	    }
	    if (gf_timeexpired >= EXIT_SOON) /* failsafe if thread count bad */
		break;
	    MS_sleep(2);
	}
	D_PRINTF(stderr, "client %d: started all threads.\n", pnum);


	/* Wait for all threads to exit or overtime */
	while (FinishedThreads < StartedThreads) {
	    int tm = time(0);
	    if (tm > bailtime) {
		++gf_timeexpired;
		bailtime += gt_stopinterval;
#ifndef _WIN32
		if (gf_timeexpired >= EXIT_FAST) {
		    returnerr (stderr, "Client signaling exit, started=%d finished=%d timeexpired=%d\n",
			       StartedThreads, FinishedThreads, gf_timeexpired);
		    kill (0, SIGALRM);	/* wake children */
		}
#endif
	    }
	    if (gf_timeexpired >= EXIT_FASTEST) {
		returnerr (stderr, "Forcing sockets closed.\n");
		/* close all client sockets, to force calls to exit */
		for (tnum = 0; tnum < gn_numthreads; ++tnum) {
		    if (BADSOCKET(ptcxs[tnum].sock))
			continue;
		    D_PRINTF (stderr, "Closing sock=%d tnum=%d\n",
			      ptcxs[tnum].sock, tnum);
		    set_abortive_close(ptcxs[tnum].sock);
		    NETCLOSE(ptcxs[tnum].sock);
		}
		returnerr (stderr, "Forced socket close complete.\n");
		break;
	    }
  	    MS_sleep(1);
	}	

	D_PRINTF (stderr, "Shutdown timeexpired=%d\n", gf_timeexpired);
	if (gf_timeexpired < EXIT_FAST)	{
	    gf_timeexpired = EXIT_FAST; /* signal summary thread to exit */
	    returnerr (stderr, "Clean child shutdown\n");
	} else if (gf_timeexpired >=  EXIT_FASTEST) {
	    returnerr (stderr, "Forced child shutdown\n");
	} else {
	    returnerr (stderr, "Accellerated child shutdown\n");
	}

	D_PRINTF(stderr, "client %d: joining summary thread\n", pnum);
	if ((ret=sysdep_thread_join(summary_tid, &status)) != 0) {
	    returnerr(stderr,
		      "client %d: summary thread join failed ret=%d errno=%d: %s\n",
		      pnum, ret, errno, strerror(errno));
	}

#ifndef _WIN32
	/* do a summary now in case we hang in joins (needed?) */
	D_PRINTF(stderr, "client %d: pre-join clientTimeSummary\n", pnum);
	clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);

	for (tnum = 0; tnum < gn_numthreads; ++tnum) {
	    D_PRINTF(stderr, "client %d: joining thread %d ID %d\n",
		     pnum, tnum, ptcxs[tnum].tid);
	    if (ptcxs[tnum].tid) {
		sysdep_thread_join(ptcxs[tnum].tid, &status);
		D_PRINTF(stderr, "client %d: thread %d joined ID %d, ret=%d status=%d\n",
			 pnum, tnum, ptcxs[tnum].tid, ret, status);
		ptcxs[tnum].tid = 0;
	    }
	}
#endif /* _WIN32 */
    } else
#endif /* USE_PTHREADS || _WIN32*/
    {				/* thread un-available or 0 */
	gn_numthreads = 1;
	ptcxs = (ptcx_t)xcalloc(sizeof(tcx_t) * gn_numthreads);

	initTcx(&ptcxs[0], outfd, pnum, -1);

	D_PRINTF(stderr, "client %d: testtime: %d\n", pnum);

	/* set initial data point */
	D_PRINTF(stderr, "client %d: initial clientTimeSummary\n", 0);
	clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);

	clientThread(&ptcxs[0]);
    }

    /* final time summary feedback */
    D_PRINTF(stderr, "client %d: final summaries\n", 0);
    clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);
    clientBlockSummary(ptcxs, gn_numthreads, pnum, outfd);

    for (tnum = 0; tnum < gn_numthreads; ++tnum) { /* clean up */
	D_PRINTF(stderr, "client %d: thread %d destroyed\n", pnum, tnum);
	destroyTcx(&ptcxs[tnum]);
    }

    D_PRINTF(stderr, "child: %d done\n", pnum);
    return 0;
}

#ifdef USE_EVENTS
/* version of clientProc which uses event queue */

/*********************************************************************
Things to note:

The idleTime counter is hopefully correct, but it's a bit tricky --
one event will start the timer, and the next is expected to stop it.
The invariant is that either ptcx->ev_stats->idle will be NULL or will
be started at the beginning of each event function.

startDelay is strange in that it is sampled from a block that is never
executed.  However, this block is not counted when figuring
statistics, so it won't mess with the results.
*********************************************************************/

#define get_loop_throttle(X) ((X)->throttle)
#define get_block_throttle(X) ((X)->throttle)

static void
update_dynamic_throttle(ptcx_t ptcx, mail_command_t * cmd,
			struct timeval * expected)
{
    struct timeval now;
    int elapsed;

    if (cmd->loopThrottle == 0 || cmd->throttleFactor == 1)
	return;
    
    assert(cmd->throttleFactor != 0);
    gettimeofday(&now, NULL);
    timersub(&now, expected, &now);
    elapsed = now.tv_usec / 1000 + now.tv_sec * 1000;
    if (elapsed > cmd->loopThrottle) {
	cmd->throttle *= cmd->throttleFactor;
	D_PRINTF(stderr, "Throttling back %s.  New rate = %g\n",
		 cmd->proto->name, cmd->throttle);
    } else if (elapsed < - cmd->loopThrottle) {
	cmd->throttle /= cmd->throttleFactor;
	D_PRINTF(stderr, "Throttling up %s.  New rate = %g\n",
		 cmd->proto->name, cmd->throttle);
    } else {
	D_PRINTF(stderr,
		 "Not throttling %s (elapsed = %d, throttle = %d).  Rate = %g\n",
		 cmd->proto->name, elapsed, cmd->loopThrottle, cmd->throttle);
    }
}

#include "event.h"
ev_queue_t g_eq;

static int ev_clientThread(void * );
static void ev_clientDone(void * );
static void client_init(void *arg);
static void client_loop(void *arg);

int
ev_clientProc(int pnum, int outfd,
	      unsigned int testtime,
	      unsigned int thread_stagger_usec)
{
    int tnum;
    int ret;
    ptcx_t	ptcxs; /* thread contexts */
    THREAD_ID summary_tid;
    int status;


    bailtime = gt_shutdowntime;

    returnerr(stderr, "Child starting\n"); /* get our pid and time printed */
    D_PRINTF(stderr, "ev_clientProc(%d, %d, %d) starting\n",
	     pnum, testtime, thread_stagger_usec);

    if (testtime <= 0) {		/* never happens, checked in main.c */
	D_PRINTF (stderr, "ABORTING testtime=%d\n", testtime);
	return 0;
    }

    setup_signal_handlers ();

    alarm(testtime);			/* promptly notice test end */

    D_PRINTF(stderr, "ev_clientProc: outputting summary fmts\n");
    clientSummaryFormat (pnum, outfd);

    if (gn_numthreads > 0) {
	ptcxs = (ptcx_t)xcalloc(sizeof(tcx_t) * gn_numthreads);

	D_PRINTF(stderr, "ev_client: initializing tcx's at 0x%x\n", ptcxs);
	for (tnum = 0; tnum < gn_numthreads; ++tnum) {
	    initTcx(&ptcxs[tnum], outfd, pnum, tnum);
	}

	/* initialize our event queue */
	g_eq = ev_queue_create(NULL);
	D_PRINTF(stderr, "ev_clientProc: created event queue\n");
	if (g_eq == NULL) {
	    returnerr(stderr, "client %d: can't create event queue.\n", pnum);
	    return -1;
	}
	
	D_PRINTF(stderr, "ev_client: creating summary thread\n");
	if ((ret = sysdep_thread_create(&summary_tid, summaryThread,
					(void *)ptcxs)) != 0) {
	    returnerr(stderr, "client %d: summary thread create failed ret=%d errno=%d: %s\n",
		      pnum, ret, errno, strerror(errno));
	    ptcxs[tnum].tid = 0;
	}

	for (tnum = 0; tnum < gn_numthreads; ++tnum) {
	    if (gf_timeexpired)
		break;

	    /* sleep between each client thread we try to start */
	    if (tnum && thread_stagger_usec) {
		MS_usleep(thread_stagger_usec);
		if (gf_timeexpired)
		    break;
	    }

	    D_PRINTF(stderr, "client %d: pseudo-thread %d testtime %d\n",
		     pnum, tnum, testtime);

	    if (ev_clientThread((void*)&ptcxs[tnum]) < 0) {
		returnerr(stderr,
			  "client %d: pseudo-thread %d create() failed\n");
		ptcxs[tnum].tid = 0;
	    }
	}

	/* Wait for all threads to exit or overtime */
	while (FinishedThreads < StartedThreads) {
	    int tm = time(0);
	    if (tm > bailtime) {
		++gf_timeexpired;
		bailtime += gt_stopinterval;
		if (gf_timeexpired >= EXIT_FAST) {
		    returnerr (stderr, "Client signaling exit, started=%d finished=%d timeexpired=%d\n",
			       StartedThreads, FinishedThreads, gf_timeexpired);
		    kill (0, SIGALRM);	/* wake children */
		}
		if (ev_queue_destroy(g_eq, gt_stopinterval * 1000) == 0)
		    break;
	    }
	    if (gf_timeexpired >= EXIT_FASTEST) {
		returnerr (stderr, "Forcing sockets closed.\n");
		/* close all client sockets, to force calls to exit */
		for (tnum = 0; tnum < gn_numthreads; ++tnum) {
		    if (BADSOCKET(ptcxs[tnum].sock))
			continue;
		    D_PRINTF (stderr, "Closing sock=%d tnum=%d\n",
			      ptcxs[tnum].sock, tnum);
		    set_abortive_close(ptcxs[tnum].sock);
		    NETCLOSE(ptcxs[tnum].sock);
		}
		returnerr (stderr, "Forced socket close complete.\n");
		break;
	    }
  	    MS_sleep(1);
	}

	D_PRINTF (stderr, "Shutdown timeexpired=%d\n", gf_timeexpired);
	if (gf_timeexpired < EXIT_FAST)	{
	    gf_timeexpired = EXIT_FAST; /* signal summary thread to exit */
	    returnerr (stderr, "Clean child shutdown\n");
	} else if (gf_timeexpired >=  EXIT_FASTEST) {
	    returnerr (stderr, "Forced child shutdown\n");
	} else {
	    returnerr (stderr, "Accellerated child shutdown\n");
	}

	D_PRINTF(stderr, "client %d: joining summary thread\n", pnum);
	if ((ret=sysdep_thread_join(summary_tid, &status)) != 0) {
	    returnerr(stderr,
		      "client %d: summary thread join failed ret=%d errno=%d: %s\n",
		      pnum, ret, errno, strerror(errno));
	}
	/* do a summary now in case we hang in joins (needed?) */
	D_PRINTF(stderr, "client %d: pre-join clientTimeSummary\n", pnum);
	clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);

    } else {				/* thread un-available or 0 */
	gn_numthreads = 1;
	ptcxs = (ptcx_t)xcalloc(sizeof(tcx_t) * gn_numthreads);

	initTcx(&ptcxs[0], outfd, pnum, -1);

	D_PRINTF(stderr, "client %d: testtime: %d\n", pnum);

	/* set initial data point */
	D_PRINTF(stderr, "client %d: initial clientTimeSummary\n", 0);
	clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);

	clientThread(&ptcxs[0]);
    }

    /* final time summary feedback */
    D_PRINTF(stderr, "client %d: final summaries\n", 0);
    clientTimeSummary(ptcxs, gn_numthreads, pnum, outfd);
    clientBlockSummary(ptcxs, gn_numthreads, pnum, outfd);

    for (tnum = 0; tnum < gn_numthreads; ++tnum) { /* clean up */
	D_PRINTF(stderr, "client %d: thread %d destroyed\n", pnum, tnum);
	destroyTcx(&ptcxs[tnum]);
    }

    D_PRINTF(stderr, "child: %d done\n", pnum);
    return 0;
}

static int
choose_cmd()
{
    int comm_index = 0;
    if (gn_number_of_commands > 1) {
	int ran_number;
	/* Handle the weighted distribution of commands */
	ran_number = (RANDOM() % gn_total_weight);
	
	/* loop through pages, find correct one 
	 * while ran_number is positive, decrement it
	 * by the weight of the current page
	 * example: ran_number is 5, pages have weights of 10 and 10
	 *          first iteration comm_index = 0, ran_number = -5
	 *          iteration halted, comm_index = 0
	 */
	comm_index = -1;
	while (ran_number >= 0) {
	    comm_index++;
	    ran_number -= g_loaded_comm_list[comm_index].weight;
	} 
    }
    return comm_index;
}

static int
ev_clientThread(void *targ)
{
    time_t	currentTime;
    struct tm	*tmptm;
    char	timeStamp[DATESTAMP_LEN];
    tcx_t	*ptcx = (ptcx_t)targ;

    if (clientInit(ptcx)) {		/* should never fail */
	return -1;
    }

    if (gn_debug) {
	/* write current time to debug file */
	time(&currentTime);
	tmptm = localtime(&currentTime); 
	strftime(timeStamp, DATESTAMP_LEN, "%Y%m%d%H%M%S", tmptm);
	D_PRINTF(debugfile, "Time Stamp: %s\n", timeStamp);
	D_PRINTF(debugfile, "mailstone run dateStamp=%s\n", gs_dateStamp);
    }

    ++StartedThreads;
    
    ptcx->blockCount = 0;

    ptcx->ev_stats = NULL;		/* don't update stats in client_init */
    client_init(ptcx);
    
    return 0;
}

static void
ev_clientDone(void *targ)
{
    time_t	currentTime;
    struct tm	*tmptm;
    char	timeStamp[DATESTAMP_LEN];
    tcx_t	*ptcx = (ptcx_t)targ;
    
    /* count outstanding idle time */
    if (ptcx->ev_stats != NULL)
	event_stop(ptcx, &ptcx->ev_stats->idle);

    D_PRINTF(debugfile, "Test run complete\n" );

    /* write current time to debug file */
    time(&currentTime);
    tmptm = localtime(&currentTime); 
    strftime(timeStamp, DATESTAMP_LEN, "%Y%m%d%H%M%S", tmptm);
    D_PRINTF(debugfile, "Time Stamp: %s\n", timeStamp);

    if (gn_record_telemetry && (ptcx->logfile > 0)) {
	close(ptcx->logfile);
	ptcx->logfile = -1;
    }

    D_PRINTF(debugfile, "client exiting.\n" );

    if (gn_debug && ptcx->dfile) {
	fflush(ptcx->dfile);
	if (ptcx->dfile != stderr) {
	    fclose(ptcx->dfile);
	    ptcx->dfile = stderr;
	}
    }

    ++FinishedThreads;
}

void
add_msec(struct timeval * tv, int ms)
{
    tv->tv_sec += ms / 1000;
    tv->tv_usec += 1000 * (ms % 1000);
    if (tv->tv_usec >= 1000000) {
	tv->tv_sec++;
	tv->tv_usec -= 1000000;
    }
}

/*
**  client_init -- perform an init action.
**
**	Note:
**		Test run completion cannot be signaled by closing the
**		event queue, since a "nice" shutdown still demands
**		various logout actions.
*/

static void client_init2(void * );

static void
client_init(void *arg)
{
    ptcx_t ptcx	 = (ptcx_t)arg;
    cmd_stats_t * old_stats = ptcx->ev_stats;
    int comm_index;

    if (gf_timeexpired >= EXIT_SOON) {
	/* do something here? */
	fprintf(stderr, "client_init(): after test-end.\n");
	++FinishedThreads;
	return;
    }

    comm_index = choose_cmd();
    ptcx->ev_comm = &g_loaded_comm_list[comm_index];
    ptcx->ev_stats =  &(ptcx->cmd_stats[comm_index]);
    
    /* Figure out when to do second half of client_init() */
    if (old_stats == NULL) {
	/* first time => use startDelay */
	int start_delay = sample(ptcx->ev_comm->startDelay);
	if (start_delay < 0) {
	    fprintf(stderr, "eek: start-delay < 0\n");
	    start_delay = 0;
	}
	if (ev_after(g_eq, start_delay, client_init2, (void*)ptcx) < 0)
	    /* error */;
    } else {
	/* do protocol-independent statistics */
	++ptcx->connectCount;
	++ptcx->blockCount;
	event_stop(ptcx, &old_stats->idle);
	client_init2((void*)ptcx);
    }
}

/*
**  client_init2 -- second half of client_init()
*/

static void
client_init2(void * arg)
{
    ptcx_t ptcx	 = (ptcx_t)arg;
    struct timeval next;

    /*
    **  figure out when we should do the next action and next block,
    **  but don't actually schedule them until we finish the current
    **  one.  This guarantees that actions are always performed in
    **  order.
    */

    ptcx->ev_stats->totalcommands++; /* track command blocks trys */
    
    gettimeofday(&next, NULL);
    ptcx->ev_next.tv_sec = next.tv_sec;
    ptcx->ev_next.tv_usec = next.tv_usec;
    ptcx->ev_loop = ptcx->ev_comm->numLoops;
    
    /* time for next block */
    add_msec(&ptcx->ev_next, (get_block_throttle(ptcx->ev_comm)
 			      * sample(ptcx->ev_comm->blockTime)));

    /* time for next unit of work in this block */
    add_msec(&next, sample(ptcx->ev_comm->idleTime));

    /* run the init function, and schedule the first loop (or end) */
    ptcx->ev_state = (*(ptcx->ev_comm->proto->cmdStart)) (ptcx, ptcx->ev_comm,
							  ptcx->ev_stats);

    if (ptcx->ev_state == NULL) {
	D_PRINTF(stderr, "do_command Start failed\n");
	event_start(ptcx, &ptcx->ev_stats->idle);
	ev_at(g_eq, &ptcx->ev_next, client_init, (void*)ptcx);
	return;
    }

    /* check to see if we've scheduled enough blocks */
    if (gn_maxBlockCnt && (ptcx->blockCount >= gn_maxBlockCnt)) {
	D_PRINTF (debugfile, "Saw enough loops %d, exiting\n",
		  ptcx->blockCount);
	beginShutdown ();		/* indicate early exit */
    }

    event_start(ptcx, &ptcx->ev_stats->idle);
    ev_at(g_eq, &next, client_loop, (void*)ptcx);
}

static void
client_loop(void *arg)
{
    ptcx_t ptcx = (ptcx_t)arg;
    
    assert(ptcx);
    assert(ptcx->ev_comm);
    assert(ptcx->ev_comm->proto);

    event_stop(ptcx, &ptcx->ev_stats->idle);

    /* check for rude shutdown: */
    if (gf_timeexpired >= EXIT_FAST) {
	fprintf(stderr, "%s: forced shutdown.\n", ptcx->ev_comm->proto->name);
	ev_clientDone(arg);
	return;
    }
    
    if (ptcx->ev_loop == 0) {
	/* run end function */
	if (ptcx->ev_comm->proto->cmdEnd)
	    (*(ptcx->ev_comm->proto->cmdEnd))(ptcx, ptcx->ev_comm,
					      ptcx->ev_stats, ptcx->ev_state);

	if (gf_timeexpired >= EXIT_SOON) {
	    fprintf(stderr, "%s: finished.\n", ptcx->ev_comm->proto->name);
	    ev_clientDone(arg);
	    return;
	}
	/* schedule next item */
	event_start(ptcx, &ptcx->ev_stats->idle);
	ev_at(g_eq, &ptcx->ev_next, client_init, arg);
    } else {
	/* run loop function */
	struct timeval next;
	int ret = 0;

	assert (ptcx->ev_comm);
	assert (ptcx->ev_stats);
	assert (ptcx->ev_state);

	gettimeofday(&next, NULL);
	add_msec(&next, get_loop_throttle(ptcx->ev_comm) * sample(ptcx->ev_comm->loopDelay));
	if (ptcx->ev_comm->proto->cmdLoop)
	    ret = (*(ptcx->ev_comm->proto->cmdLoop))(ptcx,
						     ptcx->ev_comm,
						     ptcx->ev_stats,
						     ptcx->ev_state);

	if (gf_timeexpired >= EXIT_FAST) {
	    fprintf(stderr, "%s: forced shutdown.",
		    ptcx->ev_comm->proto->name);
	    ev_clientDone(arg);
	    return;
	}
	
	if (gf_timeexpired >= EXIT_SOON || ret != 0) {
	    fprintf(stderr, "%s: finishing (%d remaining loops).\n",
		    ptcx->ev_comm->proto->name, ptcx->ev_loop);
	    ptcx->ev_loop = 0;		/* do end function */
	}
	else {
	    ptcx->ev_loop--;		/* do next loop (or end) */
	    /* only update throttling if loop succeeded */
	    update_dynamic_throttle(ptcx, ptcx->ev_comm, &next);
	}
	event_start(ptcx, &ptcx->ev_stats->idle);
	ev_at(g_eq, &next, client_loop, arg);
    }
}

#endif /* USE_EVENTS */
