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
** File:            ctsynch.c
** Descritpion:        Implemenation for thread synchronization using cthreads
** Exports:            prlock.h, prcvar.h, prmon.h, prcmon.h
*/

#if defined(_PR_CTHREADS)

#include "primpl.h"
#include "obsolete/prsem.h"

#include <string.h>
#include <mach/cthreads.h>
#include <sys/time.h>

/**************************************************************/
/**************************************************************/
/*****************************LOCKS****************************/
/**************************************************************/
/**************************************************************/

void _PR_InitLocks(void)
{
    _PR_MD_INIT_LOCKS();
}

static void pt_PostNotifies(PRLock *lock, PRBool unlock)
{
    PRIntn index, rv;
    _CT_Notified post;
    _CT_Notified *notified, *prev = NULL;
    /*
     * Time to actually notify any conditions that were affected
     * while the lock was held. Get a copy of the list that's in
     * the lock structure and then zero the original. If it's
     * linked to other such structures, we own that storage.
     */
    post = lock->notified;  /* a safe copy; we own the lock */

#if defined(DEBUG)
    memset(&lock->notified, 0, sizeof(_CT_Notified));  /* reset */
#else
    lock->notified.length = 0;  /* these are really sufficient */
    lock->notified.link = NULL;
#endif

    /* should (may) we release lock before notifying? */
    if (unlock)
    {
        rv = mutex_unlock(lock->mutex);
        PR_ASSERT(0 == rv);
    }

    notified = &post;  /* this is where we start */
    do
    {
        for (index = 0; index < notified->length; ++index)
        {
            PR_ASSERT(NULL != notified->cv[index].cv);
            PR_ASSERT(0 != notified->cv[index].times);
            if (-1 == notified->cv[index].times)
            {
                condition_broadcast(notified->cv[index].cv->cv);
            }
            else
            {
                while (notified->cv[index].times-- > 0)
                {
                    condition_signal(notified->cv[index].cv->cv);
                }
            }
        }
        prev = notified;
        notified = notified->link;
        if (&post != prev) PR_DELETE(prev);
    } while (NULL != notified);
}  /* pt_PostNotifies */

PR_IMPLEMENT(PRLock*) PR_NewLock(void)
{
    PRLock *lock;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    lock = PR_NEWZAP(PRLock);
    if (lock != NULL)
    {
	lock->mutex = mutex_alloc();
	if (lock->mutex == NULL)
	{
		PR_DELETE(lock);
		return NULL;
	}
	mutex_init(lock->mutex);
    }
    return lock;
}  /* PR_NewLock */

PR_IMPLEMENT(void) PR_DestroyLock(PRLock *lock)
{
    PR_ASSERT(NULL != lock);
    PR_ASSERT(CTHREAD_THR_HANDLE_IS_ZERO(lock->owner));
    PR_ASSERT(0 == lock->notified.length);
    PR_ASSERT(NULL == lock->notified.link);
    mutex_free(lock->mutex);
#if defined(DEBUG)
    memset(lock, 0xaf, sizeof(PRLock));
#endif
    PR_DELETE(lock);
}  /* PR_DestroyLock */

PR_IMPLEMENT(void) PR_Lock(PRLock *lock)
{
    PR_ASSERT(lock != NULL);
    mutex_lock(lock->mutex);
    PR_ASSERT(0 == lock->notified.length);
    PR_ASSERT(NULL == lock->notified.link);
    PR_ASSERT(CTHREAD_THR_HANDLE_IS_ZERO(lock->owner));
    lock->owner = cthread_self();
}  /* PR_Lock */

PR_IMPLEMENT(PRStatus) PR_Unlock(PRLock *lock)
{
    PR_ASSERT(lock != NULL);
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(lock->mutex));
    PR_ASSERT(lock->owner == cthread_self());

    if (!lock->owner == cthread_self())
        return PR_FAILURE;

    CTHREAD_ZERO_THR_HANDLE(lock->owner);
    if (0 == lock->notified.length)  /* shortcut */
    {
        mutex_unlock(lock->mutex);
    }
    else pt_PostNotifies(lock, PR_TRUE);

    return PR_SUCCESS;
}  /* PR_Unlock */


/**************************************************************/
/**************************************************************/
/***************************CONDITIONS*************************/
/**************************************************************/
/**************************************************************/

#if 0
/*
 * This code is used to compute the absolute time for the wakeup.
 * It's moderately ugly, so it's defined here and called in a
 * couple of places.
 */
#define CT_NANOPERMICRO 1000UL
#define CT_BILLION 1000000000UL

static PRStatus ct_TimedWait(
    condition_t cv, mutex_t ml, PRIntervalTime timeout)
{
    int rv;
    struct timeval now;
    struct timespec tmo;
    PRUint32 ticks = PR_TicksPerSecond();

    tmo.tv_sec = timeout / ticks;
    tmo.tv_nsec = timeout - (tmo.tv_sec * ticks);
    tmo.tv_nsec = PR_IntervalToMicroseconds(CT_NANOPERMICRO * tmo.tv_nsec);

    /* cthreads wants this in absolute time, off we go ... */
#if defined(SOLARIS) && defined(_SVID_GETTOD)
    (void)gettimeofday(&now);
#else
    (void)gettimeofday(&now, NULL);
#endif
    /* that one's usecs, this one's nsecs - grrrr! */
    tmo.tv_sec += now.tv_sec;
    tmo.tv_nsec += (CT_NANOPERMICRO * now.tv_usec);
    tmo.tv_sec += tmo.tv_nsec / CT_BILLION;
    tmo.tv_nsec %= CT_BILLION;

    rv = pthread_cond_timedwait(cv, ml, &tmo);

    /* NSPR doesn't report timeouts */
#ifdef _PR_DCETHREADS
    return (rv == -1 && errno == EAGAIN) ? 0 : rv;
#else
    return (rv == ETIMEDOUT) ? 0 : rv;
#endif
}  /* ct_TimedWait */
#endif


/*
 * Notifies just get posted to the to the protecting mutex. The
 * actual notification is done when the lock is released so that
 * MP systems don't contend for a lock that they can't have.
 */
static void ct_PostNotifyToCvar(PRCondVar *cvar, PRBool broadcast)
{
    PRIntn index = 0;
    _CT_Notified *notified = &cvar->lock->notified;

    PR_ASSERT(cvar->lock->owner == cthread_self());
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(cvar->lock->mutex));

    while (1)
    {
        for (index = 0; index < notified->length; ++index)
        {
            if (notified->cv[index].cv == cvar)
            {
                if (broadcast)
                    notified->cv[index].times = -1;
                else if (-1 != notified->cv[index].times)
                    notified->cv[index].times += 1;
                goto finished;  /* we're finished */
            }
        }
        /* if not full, enter new CV in this array */
        if (notified->length < CT_CV_NOTIFIED_LENGTH) break;

        /* if there's no link, create an empty array and link it */
        if (NULL == notified->link)
            notified->link = PR_NEWZAP(_CT_Notified);
        notified = notified->link;
    }

    /* A brand new entry in the array */
    notified->cv[index].times = (broadcast) ? -1 : 1;
    notified->cv[index].cv = cvar;
    notified->length += 1;

finished:
    PR_ASSERT(cvar->lock->owner == cthread_self());
}  /* ct_PostNotifyToCvar */

PR_IMPLEMENT(PRCondVar*) PR_NewCondVar(PRLock *lock)
{
    PRCondVar *cv = PR_NEW(PRCondVar);
    PR_ASSERT(lock != NULL);
    if (cv != NULL)
    {
	cv->cv = condition_alloc();
	if (cv->cv == NULL)
	{
		PR_DELETE(cv);
		return NULL;
	}
	condition_init(cv->cv);
        cv->lock = lock;
    }
    return cv;
}  /* PR_NewCondVar */

PR_IMPLEMENT(void) PR_DestroyCondVar(PRCondVar *cvar)
{
    condition_free(cvar->cv);
#if defined(DEBUG)
        memset(cvar, 0xaf, sizeof(PRCondVar));
#endif
    PR_DELETE(cvar);
}  /* PR_DestroyCondVar */

PR_IMPLEMENT(PRStatus) PR_WaitCondVar(PRCondVar *cvar, PRIntervalTime timeout)
{
#if 0
    PRStatus rv;
#endif
    PRThread *thred = PR_CurrentThread();

    PR_ASSERT(cvar != NULL);
    /* We'd better be locked */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(cvar->lock->mutex));
    /* and it better be by us */
    PR_ASSERT(cvar->lock->owner == cthread_self());

    /*
     * The thread waiting is used for PR_Interrupt
     */
    thred->waiting = cvar;  /* this is where we're waiting */

    if (thred->state & CT_THREAD_ABORTED)
    {
        thred->waiting = NULL;
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thred->state &= ~CT_THREAD_ABORTED;
        return PR_FAILURE;
    }

    /*
     * If we have pending notifies, post them now.
     *
     * This is not optimal. We're going to post these notifies
     * while we're holding the lock. That means on MP systems
     * that they are going to collide for the lock that we will
     * hold until we actually wait.
     */
    if (0 != cvar->lock->notified.length)
        pt_PostNotifies(cvar->lock, PR_FALSE);

    /*
     * We're surrendering the lock, so clear out the owner field.
     */
    CTHREAD_ZERO_THR_HANDLE(cvar->lock->owner);

#if 0 /* XXX FIX ME */
    if (timeout == PR_INTERVAL_NO_TIMEOUT)
#endif
        condition_wait(cvar->cv, cvar->lock->mutex);
#if 0
    else
        rv = ct_TimedWait(&cvar->cv, &cvar->lock->mutex, timeout);
#endif

    /* We just got the lock back - this better be empty */
    PR_ASSERT(CTHREAD_THR_HANDLE_IS_ZERO(cvar->lock->owner));
    CTHREAD_COPY_THR_HANDLE(cthread_self(), cvar->lock->owner);

    PR_ASSERT(0 == cvar->lock->notified.length);
    thred->waiting = NULL;  /* and now we're not */
    if (thred->state & CT_THREAD_ABORTED)
    {
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thred->state &= ~CT_THREAD_ABORTED;
        return PR_FAILURE;
    }
#if 0
    return (rv == 0) ? PR_SUCCESS : PR_FAILURE;
#else
    return PR_SUCCESS;
#endif
}  /* PR_WaitCondVar */

PR_IMPLEMENT(PRStatus) PR_NotifyCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);   
    ct_PostNotifyToCvar(cvar, PR_FALSE);
    return PR_SUCCESS;
}  /* PR_NotifyCondVar */

PR_IMPLEMENT(PRStatus) PR_NotifyAllCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);
    ct_PostNotifyToCvar(cvar, PR_TRUE);
    return PR_SUCCESS;
}  /* PR_NotifyAllCondVar */

/**************************************************************/
/**************************************************************/
/***************************MONITORS***************************/
/**************************************************************/
/**************************************************************/

PR_IMPLEMENT(PRMonitor*) PR_NewMonitor(void)
{
    PRMonitor *ml;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    ml = PR_NEWZAP(PRMonitor);
    if (ml != NULL)
    {
	ml->lock.mutex = mutex_alloc();
	if (ml->lock.mutex == NULL)
	{
		PR_DELETE(ml);
		return NULL;
	}

	ml->cvar.cv = condition_alloc();
	if (ml->cvar.cv == NULL)
	{
		PR_DELETE(ml);
		mutex_free(ml->lock.mutex);
		return NULL;
	}

	mutex_init(ml->lock.mutex);
	condition_init(ml->cvar.cv);

        ml->entryCount = 0;
        ml->cvar.lock = &ml->lock;
    }
    return ml;
}  /* PR_NewMonitor */

PR_IMPLEMENT(PRMonitor*) PR_NewNamedMonitor(const char* name)
{
    PRMonitor* mon = PR_NewMonitor();
    mon->name = name;
    return mon;
}

PR_IMPLEMENT(void) PR_DestroyMonitor(PRMonitor *mon)
{
    PR_ASSERT(mon != NULL);
    condition_free(mon->cvar.cv);
    mutex_free(mon->lock.mutex);
#if defined(DEBUG)
        memset(mon, 0xaf, sizeof(PRMonitor));
#endif
    PR_DELETE(mon);    
}  /* PR_DestroyMonitor */


/* The GC uses this; it is quite arguably a bad interface.  I'm just 
 * duplicating it for now - XXXMB
 */
PR_IMPLEMENT(PRInt32) PR_GetMonitorEntryCount(PRMonitor *mon)
{
    cthread_t self = cthread_self();
    if (mon->owner == self)
        return mon->entryCount;
    return 0;
}

PR_IMPLEMENT(void) PR_EnterMonitor(PRMonitor *mon)
{
    int rv;
    cthread_t self = cthread_self();

    PR_ASSERT(mon != NULL);
    rv = mutex_try_lock(mon->lock.mutex);
    if (CT_TRYLOCK_SUCCESS == rv)
    {
        /* I now have the lock - I can play in the sandbox */
        /* could/should/would not have gotten lock if entries != 0 */
        PR_ASSERT(0 == mon->entryCount);
        PR_ASSERT(CTHREAD_THR_HANDLE_IS_ZERO(mon->lock.owner));
    }
    else
    {
        PR_ASSERT(CT_TRYLOCK_BUSY == rv);  /* and if it isn't? */
        /* somebody has it locked - is it me? */
        if (!mon->owner == self)
        {
            /* it's not me - this should block */
            PR_Lock(&mon->lock);
            /* and now I have the lock */
            PR_ASSERT(0 == mon->entryCount);
        }
    }

    mon->owner = self;
    mon->lock.owner = self;
    mon->entryCount += 1;

}  /* PR_EnterMonitor */

PR_IMPLEMENT(PRStatus) PR_ExitMonitor(PRMonitor *mon)
{
    cthread_t self = cthread_self();

    PR_ASSERT(mon != NULL);
    /* The lock better be that - locked */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    /* we'd better be the owner */

    PR_ASSERT(mon->owner == self);
    if (!mon->owner == self)
        return PR_FAILURE;

    /* if it's locked and we have it, then the entries should be > 0 */
    PR_ASSERT(mon->entryCount > 0);
    mon->entryCount -= 1;  /* reduce by one */
    PR_ASSERT(mon->entryCount >= 0);  /* doesn't seem like too much to ask */
    if (mon->entryCount == 0)
    {
        /* and if it transitioned to zero - unlock */
        CTHREAD_ZERO_THR_HANDLE(mon->owner);  /* make the owner unknown */
        PR_Unlock(&mon->lock);
    }
    return PR_SUCCESS;
}  /* PR_ExitMonitor */

PR_IMPLEMENT(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime timeout)
{
    PRStatus rv;
    PRInt16 saved_entries;
    cthread_t saved_owner;

    PR_ASSERT(mon != NULL);
    /* we'd better be locked */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    /* and the entries better be positive */
    PR_ASSERT(mon->entryCount > 0);
    /* and it better be by us */
    PR_ASSERT(mon->owner == cthread_self());

    /* tuck these away 'till later */
    saved_entries = mon->entryCount; 
    mon->entryCount = 0;
    CTHREAD_COPY_THR_HANDLE(mon->owner, saved_owner);
    CTHREAD_ZERO_THR_HANDLE(mon->owner);
    
    rv = PR_WaitCondVar(&mon->cvar, timeout);

    /* reinstate the intresting information */
    mon->entryCount = saved_entries;
    CTHREAD_COPY_THR_HANDLE(saved_owner, mon->owner);

    return rv;
}  /* PR_Wait */

PR_IMPLEMENT(PRStatus) PR_Notify(PRMonitor *mon)
{
    PR_ASSERT(NULL != mon);
    /* we'd better be locked */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    /* and the entries better be positive */
    PR_ASSERT(mon->entryCount > 0);
    /* and it better be by us */
    PR_ASSERT(mon->owner == cthread_self());

    ct_PostNotifyToCvar(&mon->cvar, PR_FALSE);

    return PR_SUCCESS;
}  /* PR_Notify */

PR_IMPLEMENT(PRStatus) PR_NotifyAll(PRMonitor *mon)
{
    PR_ASSERT(mon != NULL);
    /* we'd better be locked */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    /* and the entries better be positive */
    PR_ASSERT(mon->entryCount > 0);
    /* and it better be by us */
    PR_ASSERT(mon->owner == cthread_self());

    ct_PostNotifyToCvar(&mon->cvar, PR_TRUE);

    return PR_SUCCESS;
}  /* PR_NotifyAll */

/**************************************************************/
/**************************************************************/
/**************************SEMAPHORES**************************/
/**************************************************************/
/**************************************************************/
PR_IMPLEMENT(void) PR_PostSem(PRSemaphore *semaphore)
{
	PR_Lock(semaphore->cvar->lock);
	PR_NotifyCondVar(semaphore->cvar);
	semaphore->count += 1;
	PR_Unlock(semaphore->cvar->lock);
}  /* PR_PostSem */

PR_IMPLEMENT(PRStatus) PR_WaitSem(PRSemaphore *semaphore)
{
	PRStatus status = PR_SUCCESS;
	PR_Lock(semaphore->cvar->lock);
	while ((semaphore->count == 0) && (PR_SUCCESS == status))
		status = PR_WaitCondVar(semaphore->cvar, PR_INTERVAL_NO_TIMEOUT);
	if (PR_SUCCESS == status) semaphore->count -= 1;
	PR_Unlock(semaphore->cvar->lock);
	return status;
}  /* PR_WaitSem */

PR_IMPLEMENT(void) PR_DestroySem(PRSemaphore *semaphore)
{
    PR_DestroyLock(semaphore->cvar->lock);
    PR_DestroyCondVar(semaphore->cvar);
    PR_DELETE(semaphore);
}  /* PR_DestroySem */

PR_IMPLEMENT(PRSemaphore*) PR_NewSem(PRUintn value)
{
    PRSemaphore *semaphore;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    semaphore = PR_NEWZAP(PRSemaphore);
    if (NULL != semaphore)
    {
        PRLock *lock = PR_NewLock();
        if (NULL != lock)
        {
            semaphore->cvar = PR_NewCondVar(lock);
            if (NULL != semaphore->cvar)
            {
                semaphore->count = value;
                return semaphore;
            }
            PR_DestroyLock(lock);
        }
        PR_DELETE(semaphore);
    }
    return NULL;
}

/**************************************************************/
/**************************************************************/
/******************ROUTINES FOR DCE EMULATION******************/
/**************************************************************/
/**************************************************************/

#include "prpdce.h"

PR_IMPLEMENT(PRStatus) PRP_TryLock(PRLock *lock)
{
    PRIntn rv = mutex_try_lock(lock->mutex);
    if (rv == CT_TRYLOCK_SUCCESS)
    {
        PR_ASSERT(CTHREAD_THR_HANDLE_IS_ZERO(lock->owner));
        CTHREAD_COPY_THR_HANDLE(cthread_self(), lock->owner); 
    }
    else
        PR_ASSERT(!CTHREAD_THR_HANDLE_IS_ZERO(lock->owner));
    /* XXX set error code? */
    return (CT_TRYLOCK_SUCCESS == rv) ? PR_SUCCESS : PR_FAILURE;
}  /* PRP_TryLock */

PR_IMPLEMENT(PRCondVar*) PRP_NewNakedCondVar()
{
    PRCondVar *cv;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    cv = PR_NEW(PRCondVar);
    if (cv != NULL)
    {
	cv->cv = condition_alloc();
	if (cv->cv == NULL)
	{
		PR_DELETE(cv);
		return NULL;
	}

	condition_init(cv->cv);
    }
    return cv;
}  /* PRP_NewNakedCondVar */

PR_IMPLEMENT(void) PRP_DestroyNakedCondVar(PRCondVar *cvar)
{
    condition_free(cvar->cv);
#if defined(DEBUG)
        memset(cvar, 0xaf, sizeof(PRCondVar));
#endif
    PR_DELETE(cvar);
}  /* PRP_DestroyNakedCondVar */

PR_IMPLEMENT(PRStatus) PRP_NakedWait(
    PRCondVar *cvar, PRLock *ml, PRIntervalTime timeout)
{
#if 0
    PRStatus rv;
#endif
    PR_ASSERT(cvar != NULL);
    /* XXX do we really want to assert this in a naked wait? */
    PR_ASSERT(CTHREAD_MUTEX_IS_LOCKED(ml->mutex));
#if 0 /* XXX fix me -- toshok */
    if (timeout == PR_INTERVAL_NO_TIMEOUT)
#endif
        condition_wait(cvar->cv, ml->mutex);
#if 0
    else
        rv = pt_TimedWait(&cvar->cv, &ml->mutex, timeout);
#endif
#if 0
    return (rv == 0) ? PR_SUCCESS : PR_FAILURE;
#else
    return PR_SUCCESS;
#endif
}  /* PRP_NakedWait */

PR_IMPLEMENT(PRStatus) PRP_NakedNotify(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);
    condition_signal(cvar->cv);
    return PR_SUCCESS;
}  /* PRP_NakedNotify */

PR_IMPLEMENT(PRStatus) PRP_NakedBroadcast(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);
    condition_broadcast(cvar->cv);
    return PR_SUCCESS;
}  /* PRP_NakedBroadcast */

#endif  /* defined(_PR_CTHREADS) */

/* ptsynch.c */
