/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
** File:            cttwait.c
** Descritpion:        Implemenation for timed waits on condition variables.
*/

#if defined (_PR_CTHREADS) && defined(_PR_CTHREADS_NO_CONDITION_TIMEDWAIT)

#include "primpl.h"
#include <mach/cthreads.h>
#include <mach/thread_switch.h>

static PRThread *__ct_timed_wait_thread;
static mutex_t __ct_timed_wait_ml;
static condition_t __ct_timed_wait_cv;

typedef struct timed_waiter {
  PRThread *thread;

  condition_t real_cv; /* the cv the thread is waiting on */
  PRIntervalTime timeout;

  /* our internal cv and mutex */
  condition_t _cv;
  mutex_t _ml;

  struct timed_waiter *link;
} timed_waiter;

static timed_waiter *waiters;

/* the number of nanoseconds we pause between wakeups */
#define PAUSE_TIME 10000
#define CT_NANOPERMICRO 1000UL

static void PR_CALLBACK
_timed_wait_thread_fn(void *arg)
{
  while (1)
  {
    mutex_lock(__ct_timed_wait_ml);

    if (waiters)
    {
      PRIntervalTime interval = PAUSE_TIME;

      while (waiters && waiters->timeout <= interval)
      {
        timed_waiter *old_head = waiters;
  
        condition_free(old_head->_cv);
        mutex_free(old_head->_ml);
  
        interval -= old_head->timeout;

        waiters = waiters->link;

        PR_DELETE(old_head);
      }
    }
    else 
    {
      condition_wait(__ct_timed_wait_cv, __ct_timed_wait_ml);
    }

    mutex_unlock(__ct_timed_wait_ml);

    thread_switch(cthread_thread(cthread_self()), SWITCH_OPTION_WAIT, PAUSE_TIME / CT_NANOPERMICRO);

  }
}

void ct_InitTimedWaitThread()
{
  /* this should only be called once. */
  PR_ASSERT(NULL == __ct_timed_wait_thread);

  __ct_timed_wait_cv = condition_alloc();
  condition_init(__ct_timed_wait_cv);
  __ct_timed_wait_ml = mutex_alloc();
  mutex_init(__ct_timed_wait_ml);

  __ct_timed_wait_thread = PR_CreateThread(PR_SYSTEM_THREAD,
					_timed_wait_thread_fn,
					NULL,
					PR_PRIORITY_LOW,
					PR_LOCAL_THREAD,
					PR_UNJOINABLE_THREAD,
					0);
}

PRStatus ct_TimedWait(condition_t cv, mutex_t ml, PRIntervalTime timeout)
{
  /* insert an entry into the timed_waiters list. */
  timed_waiter *new_entry = PR_NEWZAP(timed_waiter);
  timed_waiter *cur, *prev;

  if(new_entry == NULL)
  {
    return PR_FAILURE;
  }

  new_entry->thread = PR_CurrentThread();
  new_entry->real_cv = cv;
  new_entry->timeout = timeout;

  new_entry->_cv = condition_alloc();
  condition_init(new_entry->_cv);
  new_entry->_ml = mutex_alloc();
  mutex_init(new_entry->_ml);

  mutex_lock(__ct_timed_wait_ml);

  cur = waiters;
  prev = 0;

  /* find the proper place to insert the new node. */
  while (cur && new_entry->timeout > cur->timeout)
  {
    prev = cur;
    cur = cur->link;
  }

  /* if we're inserting it into the list somewhere other than the head,
     we need to adjust its timeout accordingly. */
  if (prev) new_entry->timeout -= waiters->timeout;

  /* now we adjust all the timeouts for the entries later than it in the
     list */
  if (cur)
  {
    PRIntervalTime delta = cur->timeout - new_entry->timeout;

    for (; cur != NULL; cur = cur->link)
      cur->timeout += delta;
  }

  if (prev)
  {
    new_entry->link = prev->link;
    prev->link = new_entry;
  }
  else
  {
    new_entry->link = waiters;
    waiters = new_entry;
  }

  condition_broadcast(__ct_timed_wait_cv);
  mutex_unlock(__ct_timed_wait_ml);

  /* race condition here?  we wake up the timed_wait thread, and then it goes
     back to sleep since it has nothing to do, before we wait on our cv.
     something to worry about?  XXX */
  condition_wait(new_entry->_cv, new_entry->_ml);

  return PR_SUCCESS;
}

#endif
