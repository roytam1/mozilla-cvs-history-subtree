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

#ifndef _event_h
#define _event_h

#include <sys/types.h>
#include <sys/time.h>

typedef struct ev_Queue * ev_queue_t;

typedef struct ev_queue_params
{
	int	min_sleep;	/* smallest time to bother to sleep (usec) */
	int 	lowat;		/* minimum number of free threads */
	int 	hiwat;		/* maximum number of free threads */
} *ev_queue_params_t;

#define EV_Q_UNSPEC -1		/* unspecified parameter value (use defualt) */

typedef void (*ev_func_t)(void*);
typedef void (*ev_io_func_t)(int, short, void *);

/*
**  EV_QUEUE_CREATE -- Allocate and initialize a new event queue.
**
**	Parameters:
**		params -- pointer to queue parameters, or NULL to use
**			defaults.
**
**	Returns:
**		new ev_queue_t or NULL on failure
*/

extern ev_queue_t
ev_queue_create(ev_queue_params_t params);

/*
**  EV_QUEUE_DESTROY -- Flush and destroy an event queue.
**
**	ev_queue_destroy() prevents new events from being scheduled,
**	executes all pending events, then frees all resources
**	associated with the queue.
** 
**	Parameters:
**		q -- the event queue to destroy.
**		msec -- if non-zero, timeout on waiting for worker threads.
**
**	Returns:
**		0 on success, or -1 on failure.
*/

extern int
ev_queue_destroy(ev_queue_t q, unsigned msec);

/*
**  EV_AFTER -- schedule an event relative to now
**
**	Parameters:
**		q -- the queue.
**		time -- how long from now to schedule event (msec.).
**		func -- function to call.
**		arg -- what to pass to function.
**
**	Returns:
**		0 on success, or -1 on failure.
*/

extern int
ev_after(ev_queue_t q, int time, ev_func_t func, void * arg);

/*
**  EV_AT -- schedule an event at an absolute time
**
**	Parameters:
**		q -- the queue.
**		time -- when to schedule event.
**		func -- function to call.
**		arg -- what to pass to function.
**
**	Returns:
**		0 on success, or -1 on failure.
**
**	Note:
**		time can be in the past, in which case the event will
**		be scheduled for immediate execution.
*/

extern int
ev_at(ev_queue_t q, struct timeval * time, ev_func_t func, void * arg);

/*
**  EV_IO -- schedule a handler for asynchronous I/O completion.
**
**	Parameters:
**		q -- the queue.
**		fd -- file descriptor to wait for.
**		events -- events for which to poll.
**		func -- function to call.
**		arg -- what to pass to function.
**
**	Returns:
**		0 on success, or -1 on failure.
**
*/

extern int
ev_io(ev_queue_t q, int fd, short events, ev_io_func_t func, void * arg);

#endif /* _event_h */
