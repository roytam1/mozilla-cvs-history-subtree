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
 * Contributor(s):      Sean O'Rourke <sean@sendmail.com>
 *                      Sendmail, Inc.
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

#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "event.h"
#include "xalloc.h"

/*
**  EVENT.C -- event queue with dynamic thread-pool
*/

struct ev_queue_params EV_DEFAULTS = { 1000, 4, 8 };

#if 0
# define	EV_DPRINTF(X)	fprintf X /* debugging output */
# define	EV_DEBUG(X)	X
#else
# define	EV_DPRINTF(X)	(void)0
# define	EV_DEBUG(X)	(void)0
#endif

typedef struct Event
{
	ev_func_t	func;
	void *		arg;
	struct timeval	time;
} Event;

#if 0
typedef struct IOEvent
{
	ev_io_func_t	func;
	void *		arg;
	struct pollfd	pfd;
};
#endif /* 0 */

typedef struct ev_qelt
{
	struct ev_qelt *	next;
	Event 			e;
} ev_qelt;

struct ev_Queue
{
	/* tunable parameters */
	struct ev_queue_params p;

	/* state */
	pthread_mutex_t	lock;
	pthread_cond_t	available, finished;
	int		waiting_threads, threads;
	int 		finish;

	ev_qelt	*	head;
};

/* internal functions */
static int ev_queue_create_worker(ev_queue_t );
static void *ev_queue_worker(void*);

ev_queue_t
ev_queue_create(ev_queue_params_t params)
{
	ev_queue_t q = XALLOC(struct ev_Queue);
	int i;
	EV_DEBUG(extern int errno);

	pthread_mutex_init(&(q->lock), NULL);
	pthread_cond_init(&(q->available), NULL);
	pthread_cond_init(&(q->finished), NULL);
	q->waiting_threads = q->threads = 0;
	q->finish = 0;
	q->head = NULL;

	if (params == NULL)
	{
		params = &EV_DEFAULTS;
	}
	else
	{
		if (params->lowat == EV_Q_UNSPEC)
			params->lowat = EV_DEFAULTS.lowat;
		if (params->hiwat == EV_Q_UNSPEC)
			params->hiwat = EV_DEFAULTS.hiwat;
		if (params->min_sleep == EV_Q_UNSPEC)
			params->min_sleep = EV_DEFAULTS.min_sleep;
	}

	memcpy(&q->p, params, sizeof(struct ev_queue_params));
	EV_DPRINTF((stderr, "ev_queue: lowat = %d, hiwat = %d, minsl = %d\n",
		    q->p.lowat, q->p.hiwat, q->p.min_sleep));
	pthread_mutex_lock(&(q->lock));
	for (i = 0; i < q->p.lowat; i++)
		if (ev_queue_create_worker(q) < 0)
		{
			EV_DPRINTF((stderr, "Failed to create worker: %s\n",
				strerror(errno)));
			assert(0);
		}
	pthread_mutex_unlock(&(q->lock));
	EV_DPRINTF((stderr, "Created event queue\n"));
	return q;
}	

int
ev_queue_destroy(q, msec)
	ev_queue_t	q;
	unsigned	msec;
{	
	int ret = 0;
	
	q->finish = 1;
	pthread_mutex_lock(&(q->lock));
	pthread_cond_broadcast(&(q->available));
	EV_DPRINTF((stderr, "Waiting for workers to finish\n"));
	if (msec == 0)
	{
		pthread_cond_wait(&(q->finished), &(q->lock));
	}
	else
	{
		struct timeval tv;
		struct timespec ts;

		gettimeofday(&tv, NULL);
		tv.tv_usec += msec * 1000;
		if (tv.tv_usec > 1000000)
		{
			tv.tv_sec += tv.tv_usec / 1000000;
			tv.tv_usec %= 1000000;
		}
		ts.tv_sec = tv.tv_sec;
		ts.tv_nsec = msec * 1000;
		if (pthread_cond_timedwait(&(q->finished), &(q->lock), &ts) ==
		    ETIMEDOUT)
		{
			/*
			**  some thread(s) took too long.  Do not
			**  destroy the queue yet.
			*/
			return -1;
		}
	}
	pthread_cond_destroy(&(q->available));
	pthread_cond_destroy(&(q->finished));
	pthread_mutex_unlock(&(q->lock));
	
	pthread_mutex_destroy(&(q->lock));
	EV_DPRINTF((stderr, "Queue destroyed\n"));
	return ret;
}

int
ev_after(q, millis, func, arg)
	ev_queue_t	q;
	int 		millis;
	ev_func_t	func;
	void * 		arg;
{
	struct timeval t;
	assert(millis >= 0);
	gettimeofday(&t, NULL);
	t.tv_sec += millis/1000;
	t.tv_usec += (millis % 1000) * 1000;
	if (t.tv_usec >= 1000000)
	{
		t.tv_usec -= 1000000;
		t.tv_sec++;
	}
	return ev_at(q, &t, func, arg);
}

int
ev_at(q, time, func, arg)
	ev_queue_t	q;
	struct timeval *time;
	ev_func_t	func;
	void * 		arg;
{
	int d_place = 0;
	int ret = 0;
	ev_qelt **elt;
	ev_qelt *new_elt;

	struct timeval now;
	gettimeofday(&now, NULL);
	now.tv_usec -= 100000;
	if (now.tv_usec < 0)
	{
		now.tv_sec--;
		now.tv_usec += 1000000;
	}

	if (q->finish)		/* don't schedule after finish. */
		return -1;
	
	new_elt = XALLOC(ev_qelt);
	new_elt->e.time.tv_sec = time->tv_sec;
	new_elt->e.time.tv_usec = time->tv_usec;
	new_elt->e.func = func;
	new_elt->e.arg = arg;

#if 0
	if (timercmp(time, &now, <=))
		fprintf(stderr, "Warning: scheduling event %p > 100ms ago\n",
			new_elt);
#endif /* 0 */
	
	pthread_mutex_lock(&(q->lock));

	/* find new_elt's place in queue */
	elt = &(q->head);
	while (*elt != NULL && timercmp(time, &((*elt)->e.time), >))
	{
		elt = &((*elt)->next);
		d_place++;
	}

 	new_elt->next = *elt;
	*elt = new_elt;
	EV_DPRINTF((stderr, "Scheduled event %p\n", new_elt));

	/*
	**  wake up workers if the queue was empty, or if this next
	**  event might need immediate execution.
	*/
	if(q->head->next == NULL || q->head == new_elt)
		pthread_cond_signal(&(q->available));
	
	pthread_mutex_unlock(&(q->lock));
	return ret;
}

#if 0
int
ev_io(q, fd, events, func, arg)
	ev_queue_t	q;
	int		fd;
	short		events;
	ev_func_t	func;
	void *		arg;
{
	/* NOT IMPLEMENTED */
}
#endif 

static int
ev_queue_create_worker(q)
	ev_queue_t	q;
{
	pthread_t th;
	int ret;
	extern int errno;
	
	if (q->finish)
	{
		EV_DPRINTF((stderr,
			    "Attempt to create worker in finishing queue\n"));
		return -1;
	}
			
	if ((ret = pthread_create(&th, NULL,
				  &ev_queue_worker, (void*)q) != 0))
	{
		int error = errno;
		fprintf(stderr, "Can't create queue worker (%d): %s\n",
			ret, strerror(error));
		return -1;
	}
	if (pthread_detach(th) != 0)
	{
		fprintf(stderr, "Can't detach queue worker: %s\n",
			strerror(errno));
		return -1;
	}
	return 0;
}

static void*
ev_queue_worker(ptr)
	void *		ptr;
{
	ev_queue_t q = (ev_queue_t)ptr;
	EV_DEBUG(pthread_t me = pthread_self());

	EV_DPRINTF((stderr, "%ld: New worker: threads = %d\n", me, q->threads));

	pthread_mutex_lock(&(q->lock));
	q->threads++;

	while (!q->finish)
	{
		ev_qelt *e;
		int sleeptime;
		struct timeval now;

		if (q->head == NULL)
		{
			EV_DPRINTF((stderr, "%ld sleeping -- no events\n",
				    me));
			/* check to see if too many idle threads */
			if (++q->waiting_threads > q->p.hiwat)
			{
				--q->waiting_threads;
				break;
			}
			
			pthread_cond_wait(&(q->available), &(q->lock));

			if (--q->waiting_threads < q->p.lowat)
				ev_queue_create_worker(q);
		}
		else		/* q->head != NULL */
		{
			EV_DPRINTF((stderr, "%ld: Head is %p\n", me, q->head));
			/* some events -- see if they're ready */
			e = q->head;
			gettimeofday(&now, NULL);
			sleeptime = (e->e.time.tv_sec - now.tv_sec) * 1000000
				+ (e->e.time.tv_usec - now.tv_usec);
			
			if (sleeptime < q->p.min_sleep)
			{
				if (sleeptime < 0)
				{
					EV_DPRINTF((stderr,
						    "%ld Warning: %p late by %d usec\n",
						    me, e, -sleeptime));
				}
				/* close enough -- execute it. */
				q->head = q->head->next;
				pthread_mutex_unlock(&(q->lock));
				
				EV_DPRINTF((stderr, "%ld Executing %p\n",
					    me, e));
				(e->e.func)(e->e.arg);
				xfree(e);
		
				/* reacquire lock and keep going */
				pthread_mutex_lock(&(q->lock));
			}
			else
			{
				struct timespec ts;
			
				/* sleep */
				/* check to see if too many idle threads */
				if (++q->waiting_threads > q->p.hiwat)
				{
					--q->waiting_threads;
					break;
				}
				ts.tv_nsec = e->e.time.tv_usec * 1000
					- q->p.min_sleep * 1000;
				ts.tv_sec = e->e.time.tv_sec;
				EV_DPRINTF((stderr, "%ld: sleeping until %ld.%.9ld\n",
					    me, ts.tv_sec, ts.tv_nsec));
				pthread_cond_timedwait(&(q->available),
						       &(q->lock),
						       &ts);
				if (--q->waiting_threads < q->p.lowat)
					ev_queue_create_worker(q);
			}
		}
	}
	/* done */
	if (--q->threads == 0)
	{
		EV_DPRINTF((stderr, "%ld: No more threads -- done", me));
		pthread_cond_signal(&(q->finished));
	}
	
	pthread_mutex_unlock(&(q->lock));
	EV_DPRINTF((stderr, "%ld: Worker finished\n", me));
	return NULL;
}

#ifdef EV_Q_TEST

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void *
xalloc(size)
	size_t size;
{
	void *ret = malloc(size);
	assert(ret != NULL);
	return ret;
}

struct ev_Queue myqueue;

static void
print_time(arg)
	void *	arg;
{
	int nleft = (int)arg;
	time_t t = time(0);
	if (nleft > 0)
		ev_after(&myqueue, 2000, print_time, (void*)nleft);
	fprintf(stderr, "[%d] time is %s\n", nleft, ctime(&t));
}

int
main(argc, argv)
	int 	argc;
	char *	argv[];
{
	int nthings = atoi(argv[1]);
	ev_queue_init(&myqueue);
	while (nthings-- > 0)
	{
		ev_after(&myqueue, 1000*nthings,
			 print_time, (void*)nthings);
	}
	sleep(10);
	ev_queue_destroy(&myqueue);
	exit(0);
}

#endif /* EV_Q_TEST */
