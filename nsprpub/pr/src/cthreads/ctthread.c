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
** File:            ctthread.c
** Descritpion:        Implemenation for threds using cthreads
** Exports:            ctthread.h
*/

#if defined(_PR_CTHREADS)

#include "prlog.h"
#include "primpl.h"
#include "prpdce.h"

#include <mach/cthreads.h>
#include <mach/kern_return.h>
#include <string.h>
#include <signal.h>

/*
 * Record whether or not we have the privilege to set the scheduling
 * policy and priority of threads.  0 means that privilege is available.
 * EPERM means that privilege is not available.
 */

PRIntn pt_schedpriv;

struct _CT_Bookeeping ct_book = {0};

static void init_cthread_gc_support(void);

static PRIntn ct_PriorityMap(PRThreadPriority pri)
{
    return ct_book.minPrio +
	    pri * (ct_book.maxPrio - ct_book.minPrio) / PR_PRIORITY_LAST;
}

/*
** Initialize a stack for a native cthread thread
*/
static void _PR_InitializeStack(PRThreadStack *ts)
{
    if( ts && (ts->stackTop == 0) ) {
        ts->allocBase = (char *) &ts;
        ts->allocSize = ts->stackSize;

        /*
        ** Setup stackTop and stackBottom values.
        */
#ifdef HAVE_STACK_GROWING_UP
        ts->stackBottom = ts->allocBase + ts->stackSize;
        ts->stackTop = ts->allocBase;
#else
        ts->stackTop    = ts->allocBase;
        ts->stackBottom = ts->allocBase - ts->stackSize;
#endif
    }
}

static void *_ct_root(void *arg)
{
    int rv;
    PRThread *thred = (PRThread*)arg;
    PRBool detached = (thred->state & CT_THREAD_DETACHED);

    /*
     * Both the parent thread and this new thread set thred->id.
     * The new thread must ensure that thred->id is set before
     * it executes its startFunc.  The parent thread must ensure
     * that thred->id is set before PR_CreateThread() returns.
     * Both threads set thred->id without holding a lock.  Since
     * they are writing the same value, this unprotected double
     * write should be safe.
     */
    thred->id = cthread_self();

    /*
    ** CThreads can't detach during creation, so do it late.
    ** I would like to do it only here, but that doesn't seem
    ** to work.
    */
    if (thred->state & CT_THREAD_DETACHED)
    {
        /* cthread_detach() might modify its argument, so we must pass a copy */
        cthread_t self = thred->id;
        cthread_detach(self);
    }

    /* Set up the thread stack information */
    _PR_InitializeStack(thred->stack);

    /*
     * Set within the current thread the pointer to our object.
     * This object will be deleted when the thread termintates,
     * whether in a join or detached (see _PR_InitThreads()).
     */
    rv = cthread_setspecific(ct_book.key, thred);
    PR_ASSERT(0 == rv);

    /* make the thread visible to the rest of the runtime */
    PR_Lock(ct_book.ml);

    /* If this is a GCABLE thread, set its state appropriately */
    if (thred->suspend & CT_THREAD_SETGCABLE)
	    thred->state |= CT_THREAD_GCABLE;
    thred->suspend = 0;

    thred->prev = ct_book.last;
    ct_book.last->next = thred;
    thred->next = NULL;
    ct_book.last = thred;
    PR_Unlock(ct_book.ml);

    thred->startFunc(thred->arg);  /* make visible to the client */

    /* unhook the thread from the runtime */
    PR_Lock(ct_book.ml);
    if (thred->state & CT_THREAD_SYSTEM)
        ct_book.system -= 1;
    else if (--ct_book.user == ct_book.this_many)
        PR_NotifyAllCondVar(ct_book.cv);
    thred->prev->next = thred->next;
    if (NULL == thred->next)
        ct_book.last = thred->prev;
    else
        thred->next->prev = thred->prev;

    /*
     * At this moment, PR_CreateThread() may not have set thred->id yet.
     * It is safe for a detached thread to free thred only after
     * PR_CreateThread() has set thred->id.
     */
    if (detached)
    {
        while (!thred->okToDelete)
            PR_WaitCondVar(ct_book.cv, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(ct_book.ml);

    /* last chance to delete this puppy if the thread is detached */
    if (detached)
    {
    	PR_DELETE(thred->stack);
        memset(thred, 0xaf, sizeof(PRThread));
        PR_DELETE(thred);
    }

    return NULL;
}  /* _ct_root */

static PRThread* _PR_CreateThread(
    PRThreadType type, void (*start)(void *arg),
    void *arg, PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize, PRBool isGCAble)
{
    int rv;
    PRThread *thred;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if ((PRIntn)PR_PRIORITY_FIRST > (PRIntn)priority)
        priority = PR_PRIORITY_FIRST;
    else if ((PRIntn)PR_PRIORITY_LAST < (PRIntn)priority)
        priority = PR_PRIORITY_LAST;
    
#if defined(IRIX)
    if ((16 * 1024) > stackSize) stackSize = (16 * 1024);  /* IRIX minimum */
    else
#endif
    if (0 == stackSize) stackSize = (64 * 1024);  /* default == 64K */
    /*
     * Linux doesn't have pthread_attr_setstacksize.
     */

    thred = PR_NEWZAP(PRThread);
    if (thred != NULL)
    {
        thred->arg = arg;
        thred->startFunc = start;
        thred->priority = priority;
        if (PR_UNJOINABLE_THREAD == state)
            thred->state |= CT_THREAD_DETACHED;
        if (PR_GLOBAL_THREAD == scope)
            thred->state |= CT_THREAD_GLOBAL;
        if (PR_SYSTEM_THREAD == type)
            thred->state |= CT_THREAD_SYSTEM;

        thred->suspend =(isGCAble) ? CT_THREAD_SETGCABLE : 0;

        thred->stack = PR_NEWZAP(PRThreadStack);
        if (thred->stack == NULL) {
            PR_DELETE(thred);  /* all that work ... poof! */
            thred = NULL;  /* and for what? */
            goto done;
        }
        thred->stack->stackSize = stackSize;
        thred->stack->thr = thred;

	    /* make the thread counted to the rest of the runtime */
	    PR_Lock(ct_book.ml);
	    if (thred->state & CT_THREAD_SYSTEM)
	        ct_book.system += 1;
	    else ct_book.user += 1;
	    PR_Unlock(ct_book.ml);

        if ((thred->id = cthread_fork(_ct_root, thred)) == 0)
        {
            PR_Lock(ct_book.ml);
            if (thred->state & CT_THREAD_SYSTEM)
                ct_book.system -= 1;
            else if (--ct_book.user == ct_book.this_many)
                PR_NotifyAllCondVar(ct_book.cv);
            PR_Unlock(ct_book.ml);

            PR_DELETE(thred->stack);
            PR_DELETE(thred);  /* all that work ... poof! */
            thred = NULL;  /* and for what? */
            goto done;
        }

        /*
         * If the new thread is detached, tell it that PR_CreateThread()
         * has set thred->id so it's ok to delete thred.
         */
        if (PR_UNJOINABLE_THREAD == state)
        {
            PR_Lock(ct_book.ml);
            thred->okToDelete = PR_TRUE;
            PR_NotifyAllCondVar(ct_book.cv);
            PR_Unlock(ct_book.ml);
        }
    }

done:
    return thred;
}  /* _PR_CreateThread */

PR_IMPLEMENT(PRThread*) PR_CreateThread(
    PRThreadType type, void (*start)(void *arg), void *arg,
    PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize)
{
    return _PR_CreateThread(
        type, start, arg, priority, scope, state, stackSize, PR_FALSE);
} /* PR_CreateThread */

PR_IMPLEMENT(PRThread*) PR_CreateThreadGCAble(
    PRThreadType type, void (*start)(void *arg), void *arg, 
    PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize)
{
    return _PR_CreateThread(
        type, start, arg, priority, scope, state, stackSize, PR_TRUE);
}  /* PR_CreateThreadGCAble */

PR_IMPLEMENT(void*) GetExecutionEnvironment(PRThread *thred)
{
    return thred->environment;
}  /* GetExecutionEnvironment */
 
PR_IMPLEMENT(void) SetExecutionEnvironment(PRThread *thred, void *env)
{
    thred->environment = env;
}  /* SetExecutionEnvironment */

PR_IMPLEMENT(PRThread*) PR_AttachThread(
    PRThreadType type, PRThreadPriority priority, PRThreadStack *stack)
{
    PRThread *thred = NULL;
    void *privateData = NULL;

    /*
     * NSPR must have been initialized when PR_AttachThread is called.
     * We cannot have PR_AttachThread call implicit initialization
     * because if multiple threads call PR_AttachThread simultaneously,
     * NSPR may be initialized more than once.
     */
    if (!_pr_initialized) {
        /* Since NSPR is not initialized, we cannot call PR_SetError. */
        return NULL;
    }

    cthread_getspecific(ct_book.key, &privateData);
    if (NULL != privateData)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return NULL;
    }
    thred = PR_NEWZAP(PRThread);
    if (NULL == thred) PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    else
    {
        int rv;

        if ((PRIntn)PR_PRIORITY_FIRST > (PRIntn)priority)
            priority = PR_PRIORITY_FIRST;
        else if ((PRIntn)PR_PRIORITY_LAST < (PRIntn)priority)
            priority = PR_PRIORITY_LAST;

        thred->priority = priority;
        thred->id = cthread_self();
        rv = cthread_setspecific(ct_book.key, thred);
        PR_ASSERT(0 == rv);

        PR_Lock(ct_book.ml);
        if (PR_SYSTEM_THREAD == type)
        {
            ct_book.system += 1;
            thred->state |= CT_THREAD_SYSTEM;
        }
        else ct_book.user += 1;

        /* then put it into the list */
        thred->prev = ct_book.last;
	    ct_book.last->next = thred;
        thred->next = NULL;
        ct_book.last = thred;
        PR_Unlock(ct_book.ml);

    }
    return thred;  /* may be NULL */
}  /* PR_AttachThread */

PR_IMPLEMENT(PRStatus) PR_JoinThread(PRThread *thred)
{
    void *result = NULL;
    PR_ASSERT(thred != NULL);

    if ((0xafafafaf == thred->state)
    || (CT_THREAD_DETACHED & thred->state))
    {
        /*
         * This might be a bad address, but if it isn't, the state should
         * either be an unjoinable thread or it's already had the object
         * deleted. However, the client that called join on a detached
         * thread deserves all the rath I can muster....
         */
        PR_SetError(PR_ILLEGAL_ACCESS_ERROR, 0);
        PR_LogPrint(
            "PR_JoinThread: 0x%X not joinable | already smashed\n", thred);

        PR_ASSERT((0xafafafaf == thred->state)
        || (CT_THREAD_DETACHED & thred->state));
    }
    else
    {
        cthread_t id = thred->id;
        result = cthread_join(id);
        if (NULL == result)
            PR_SetError(PR_UNKNOWN_ERROR, errno);
    	PR_DELETE(thred->stack);
        memset(thred, 0xaf, sizeof(PRThread));
        PR_ASSERT(result == NULL);
        PR_DELETE(thred);
    }
    return (NULL != result) ? PR_SUCCESS : PR_FAILURE;
}  /* PR_JoinThread */

PR_IMPLEMENT(void) PR_DetachThread()
{
    PRThread *thred;
    cthread_getspecific(ct_book.key, &thred);
    PR_ASSERT(NULL != thred);

    if (NULL != thred)
    {
	int rv;

        PR_Lock(ct_book.ml);
        if (thred->state & CT_THREAD_SYSTEM)
            ct_book.system -= 1;
        else if (--ct_book.user == ct_book.this_many)
            PR_NotifyAllCondVar(ct_book.cv);
        thred->prev->next = thred->next;
        if (NULL == thred->next)
            ct_book.last = thred->prev;
        else
            thred->next->prev = thred->prev;
        PR_Unlock(ct_book.ml);

        rv = cthread_setspecific(ct_book.key, NULL);
	PR_ASSERT(0 == rv);
    	PR_DELETE(thred->stack);
        memset(thred, 0xaf, sizeof(PRThread));
        PR_DELETE(thred);
    }    
}  /* PR_DetachThread */

PR_IMPLEMENT(PRThread*) PR_GetCurrentThread()
{
    PRThread *thred;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    cthread_getspecific(ct_book.key, &thred);
    PR_ASSERT(NULL != thred);
    return thred;
}  /* PR_GetCurrentThread */

PR_IMPLEMENT(PRThreadScope) PR_GetThreadScope(const PRThread *thred)
{
    return (thred->state & CT_THREAD_GLOBAL) ?
        PR_GLOBAL_THREAD : PR_LOCAL_THREAD;
}  /* PR_GetThreadScope() */

PR_IMPLEMENT(PRThreadType) PR_GetThreadType(const PRThread *thred)
{
    return (thred->state & CT_THREAD_SYSTEM) ?
        PR_SYSTEM_THREAD : PR_USER_THREAD;
}

PR_IMPLEMENT(PRThreadState) PR_GetThreadState(const PRThread *thred)
{
    return (thred->state & CT_THREAD_DETACHED) ?
        PR_UNJOINABLE_THREAD : PR_JOINABLE_THREAD;
}  /* PR_GetThreadState */

PR_IMPLEMENT(PRThreadPriority) PR_GetThreadPriority(const PRThread *thred)
{
    PR_ASSERT(thred != NULL);
    return thred->priority;
}  /* PR_GetThreadPriority */

PR_IMPLEMENT(void) PR_SetThreadPriority(PRThread *thred, PRThreadPriority newPri)
{
    int rv;

    PR_ASSERT(NULL != thred);

    if ((PRIntn)PR_PRIORITY_FIRST > (PRIntn)newPri)
        newPri = PR_PRIORITY_FIRST;
    else if ((PRIntn)PR_PRIORITY_LAST < (PRIntn)newPri)
        newPri = PR_PRIORITY_LAST;

    rv = cthread_priority(thred->id, ct_PriorityMap(newPri), 0);
    PR_ASSERT(KERN_SUCCESS == rv);

    thred->priority = newPri;
}  /* PR_SetThreadPriority */

PR_IMPLEMENT(PRStatus) PR_NewThreadPrivateIndex(
    PRUintn *newIndex, PRThreadPrivateDTOR destructor)
{
    int rv;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    rv = cthread_key_create((cthread_key_t*)newIndex, destructor);

    if (0 == rv)
    {
        PR_Lock(ct_book.ml);
        if (*newIndex >= ct_book.highwater)
            ct_book.highwater = *newIndex + 1;
        PR_Unlock(ct_book.ml);
        return PR_SUCCESS;
    }
    PR_SetError(PR_UNKNOWN_ERROR, rv);
    return PR_FAILURE;
}  /* PR_NewThreadPrivateIndex */

PR_IMPLEMENT(PRStatus) PR_SetThreadPrivate(PRUintn index, void *priv)
{
    PRIntn rv;
    if ((cthread_key_t)index >= ct_book.highwater)
    {
        PR_SetError(PR_TPD_RANGE_ERROR, 0);
        return PR_FAILURE;
    }
    rv = cthread_setspecific((cthread_key_t)index, priv);
    PR_ASSERT(0 == rv);
    if (0 == rv) return PR_SUCCESS;

    PR_SetError(PR_UNKNOWN_ERROR, rv);
    return PR_FAILURE;
}  /* PR_SetThreadPrivate */

PR_IMPLEMENT(void*) PR_GetThreadPrivate(PRUintn index)
{
    void *result = NULL;
    if ((cthread_key_t)index < ct_book.highwater)
        cthread_getspecific((cthread_key_t)index, &result);
    return result;
}  /* PR_GetThreadPrivate */

PR_IMPLEMENT(PRStatus) PR_Interrupt(PRThread *thred)
{
    /*
    ** If the target thread indicates that it's waiting,
    ** find the condition and broadcast to it. Broadcast
    ** since we don't know which thread (if there are more
    ** than one). This sounds risky, but clients must
    ** test their invariants when resumed from a wait and
    ** I don't expect very many threads to be waiting on
    ** a single condition and I don't expect interrupt to
    ** be used very often.
    */
    PRCondVar *victim;
    PR_ASSERT(thred != NULL);
    thred->state |= CT_THREAD_ABORTED;
    victim = thred->waiting;
    if (NULL != victim)
    {
        PRBool haveLock = (victim->lock->owner == cthread_self());
        if (!haveLock) PR_Lock(victim->lock);
        PR_NotifyAllCondVar(victim);
        if (!haveLock) PR_Unlock(victim->lock);
    }
    return PR_SUCCESS;
}  /* PR_Interrupt */

PR_IMPLEMENT(void) PR_ClearInterrupt()
{
    PRThread *me = PR_CurrentThread();
    me->state &= ~CT_THREAD_ABORTED;
}  /* PR_ClearInterrupt */

PR_IMPLEMENT(PRStatus) PR_Sleep(PRIntervalTime ticks)
{
    PRStatus rv;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (PR_INTERVAL_NO_WAIT == ticks)
    {
        cthread_yield();
        rv = PR_SUCCESS;
    }
    else
    {
        PRCondVar *cv = PR_NewCondVar(ct_book.ml);
        PR_ASSERT(cv != NULL);
        PR_Lock(ct_book.ml);
        rv = PR_WaitCondVar(cv, ticks);
        PR_Unlock(ct_book.ml);
        PR_DestroyCondVar(cv);
    }
    return rv;
}  /* PR_Sleep */

PR_IMPLEMENT(void) _PR_InitThreads(
    PRThreadType type, PRThreadPriority priority, PRUintn maxPTDs)
{
    int rv;
    PRThread *thred;

    /*
    ** These might be function evaluations
    */
    ct_book.minPrio = CT_PRIO_MIN;
    ct_book.maxPrio = CT_PRIO_MAX;
    
    PR_ASSERT(NULL == ct_book.ml);
    ct_book.ml = PR_NewLock();
    PR_ASSERT(NULL != ct_book.ml);
    ct_book.cv = PR_NewCondVar(ct_book.ml);
    PR_ASSERT(NULL != ct_book.cv);
    thred = PR_NEWZAP(PRThread);
    PR_ASSERT(NULL != thred);
    thred->arg = NULL;
    thred->startFunc = NULL;
    thred->priority = priority;
    thred->id = cthread_self();

    thred->state |= (CT_THREAD_DETACHED | CT_THREAD_PRIMORD);
    if (PR_SYSTEM_THREAD == type)
    {
        thred->state |= CT_THREAD_SYSTEM;
        ct_book.system += 1;
	    ct_book.this_many = 0;
    }
    else
    {
	    ct_book.user += 1;
	    ct_book.this_many = 1;
    }
    thred->next = thred->prev = NULL;
    ct_book.first = ct_book.last = thred;

    thred->stack = PR_NEWZAP(PRThreadStack);
    PR_ASSERT(thred->stack != NULL);
    thred->stack->stackSize = 0;
    thred->stack->thr = thred;
	_PR_InitializeStack(thred->stack);

    /*
     * Create a key for our use to store a backpointer in the pthread
     * to our PRThread object. This object gets deleted when the thread
     * returns from its root in the case of a detached thread. Other
     * threads delete the objects in Join.
     *
     * NB: The destructor logic seems to have a bug so it isn't used.
     */
    rv = cthread_key_create(&ct_book.key, NULL);
    PR_ASSERT(0 == rv);
    rv = cthread_setspecific(ct_book.key, thred);
    PR_ASSERT(0 == rv);
    
    PR_SetThreadPriority(thred, priority);

    init_cthread_gc_support();
    ct_InitTimedWaitThread();

}  /* _PR_InitThreads */

PR_IMPLEMENT(PRStatus) PR_Cleanup()
{
    PRThread *me = PR_CurrentThread();
    PR_LOG(_pr_thread_lm, PR_LOG_MIN, ("PR_Cleanup: shutting down NSPR"));
    PR_ASSERT(me->state & CT_THREAD_PRIMORD);
    if (me->state & CT_THREAD_PRIMORD)
    {
        PR_Lock(ct_book.ml);
        while (ct_book.user > ct_book.this_many)
            PR_WaitCondVar(ct_book.cv, PR_INTERVAL_NO_TIMEOUT);
        PR_Unlock(ct_book.ml);

        /*
         * I am not sure if it's safe to delete the cv and lock here,
         * since there may still be "system" threads around.
         */
#if 0
        PR_DestroyCondVar(ct_book.cv);
        PR_DestroyLock(ct_book.ml);
#endif
        PR_DELETE(me->stack);
        PR_DELETE(me);
        return PR_SUCCESS;
    }
    return PR_FAILURE;
}  /* PR_Cleanup */

PR_IMPLEMENT(void) PR_ProcessExit(PRIntn status)
{
    _exit(status);
}

PR_IMPLEMENT(void) PR_DestroyThread(PRThread *thread)
{
    /* XXXMB - do we really want this? */
    PR_ASSERT(!"IMPLEMENTED");
    return;
}

/*
 * $$$
 * The following two thread-to-processor affinity functions are not
 * yet implemented for cthreads.  By the way, these functions should return
 * PRStatus rather than PRInt32 to indicate the success/failure status.
 * $$$
 */

PR_IMPLEMENT(PRInt32) PR_GetThreadAffinityMask(PRThread *thread, PRUint32 *mask)
{
    return 0;  /* not implemented */
}

PR_IMPLEMENT(PRInt32) PR_SetThreadAffinityMask(PRThread *thread, PRUint32 mask )
{
    return 0;  /* not implemented */
}

PR_IMPLEMENT(void)
PR_SetThreadDumpProc(PRThread* thread, PRThreadDumpProc dump, void *arg)
{
    thread->dump = dump;
    thread->dumpArg = arg;
}

/* 
 * Garbage collection support follows.
 */

/*
 * statics for Garbage Collection support.  We don't need to protect these
 * signal masks since the garbage collector itself is protected by a lock
 * and multiple threads will not be garbage collecting at the same time.
 */
static sigset_t javagc_vtalarm_sigmask;
static sigset_t javagc_intsoff_sigmask;

static void init_cthread_gc_support()
{
    PRIntn rv;

	rv = sigemptyset(&javagc_vtalarm_sigmask);
    PR_ASSERT(0 == rv);
	rv = sigaddset(&javagc_vtalarm_sigmask, SIGVTALRM);
    PR_ASSERT(0 == rv);
}

PR_IMPLEMENT(void) PR_SetThreadGCAble()
{
    PR_Lock(ct_book.ml);
	PR_CurrentThread()->state |= CT_THREAD_GCABLE;
    PR_Unlock(ct_book.ml);
}

PR_IMPLEMENT(void) PR_ClearThreadGCAble()
{
    PR_Lock(ct_book.ml);
	PR_CurrentThread()->state &= (~CT_THREAD_GCABLE);
    PR_Unlock(ct_book.ml);
}

static PRBool suspendAllOn = PR_FALSE;
static PRBool suspendAllSuspended = PR_FALSE;

/* Are all GCAble threads (except gc'ing thread) suspended? */
PR_IMPLEMENT(PRBool) PR_SuspendAllSuspended()
{
	return suspendAllSuspended;
} /* PR_SuspendAllSuspended */

PR_IMPLEMENT(PRStatus) PR_EnumerateThreads(PREnumerator func, void *arg)
{
    PRIntn count = 0;
    PRStatus rv = PR_SUCCESS;
    PRThread* thred = ct_book.first;
    PRThread *me = PR_CurrentThread();

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_EnumerateThreads\n"));
    /*
     * $$$
     * Need to suspend all threads other than me before doing this.
     * This is really a gross and disgusting thing to do. The only
     * good thing is that since all other threads are suspended, holding
     * the lock during a callback seems like child's play.
     * $$$
     */
    PR_ASSERT(suspendAllOn);

    while (thred != NULL)
    {
        /* Steve Morse, 4-23-97: Note that we can't walk a queue by taking
         * qp->next after applying the function "func".  In particular, "func"
         * might remove the thread from the queue and put it into another one in
         * which case qp->next no longer points to the next entry in the original
         * queue.
         *
         * To get around this problem, we save qp->next in qp_next before applying
         * "func" and use that saved value as the next value after applying "func".
         */
        PRThread* next = thred->next;

        if (thred->state & CT_THREAD_GCABLE)
        {
            PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
                   ("In PR_EnumerateThreads callback thread %X thid = %X\n", 
                    thred, thred->id));

            rv = func(thred, count++, arg);
            if (rv != PR_SUCCESS)
                return rv;
        }
        thred = next;
    }
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("End PR_EnumerateThreads count = %d \n", count));
    return rv;
}  /* PR_EnumerateThreads */

/*
 * PR_SuspendAll and PR_ResumeAll are called during garbage collection.
 */

static void PR_SuspendSet(PRThread *thred)
{
    kern_return_t rv;

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("PR_SuspendSet thred %X thread id = %X\n", thred, thred->id));


    /*
     * Check the thread state and signal the thread to suspend
     */

    PR_ASSERT((thred->suspend & CT_THREAD_SUSPENDED) == 0);

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("doing thread_syspend in PR_SuspendSet thred %X tid = %X\n",
	   thred, thred->id));
    rv = thread_suspend(cthread_thread(thred->id));

    thred->suspend |= CT_THREAD_SUSPENDED;

    PR_ASSERT(KERN_SUCCESS == rv);
}

PR_IMPLEMENT(void) PR_ResumeSet(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("PR_ResumeSet thred %X thread id = %X\n", thred, thred->id));

    /*
     * Clear the global state and set the thread state so that it will
     * continue past yield loop in the suspend signal handler
     */

    PR_ASSERT(thred->suspend & CT_THREAD_SUSPENDED);

    thred->suspend &= ~CT_THREAD_SUSPENDED;

    thread_resume(cthread_thread(thred->id));
}  /* PR_ResumeSet */

PR_IMPLEMENT(void) PR_SuspendAll()
{
#ifdef DEBUG
    PRIntervalTime stime, etime;
#endif
    PRThread* thred = ct_book.first;
    PRThread *me = PR_CurrentThread();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_SuspendAll\n"));
    /*
     * Stop all threads which are marked GC able.
     */
    PR_Lock(ct_book.ml);
    suspendAllOn = 1;
#ifdef DEBUG
    stime = PR_IntervalNow();
#endif
    while (thred != NULL)
    {
	    if ((thred != me) && (thred->state & CT_THREAD_GCABLE))
    		PR_SuspendSet(thred);
        thred = thred->next;
    }

    suspendAllSuspended = PR_TRUE;

#ifdef DEBUG
    etime = PR_IntervalNow();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS,\
        ("End PR_SuspendAll (time %dms)\n", etime - stime));
#endif
}  /* PR_SuspendAll */

PR_IMPLEMENT(void) PR_ResumeAll()
{
#ifdef DEBUG
    PRIntervalTime stime, etime;
#endif
    PRThread* thred = ct_book.first;
    PRThread *me = PR_CurrentThread();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_ResumeAll\n"));
    /*
     * Resume all previously suspended GC able threads.
     */
    suspendAllSuspended = 0;
#ifdef DEBUG
    stime = PR_IntervalNow();
#endif

    while (thred != NULL)
    {
	    if ((thred != me) && (thred->state & CT_THREAD_GCABLE))
    	    PR_ResumeSet(thred);
        thred = thred->next;
    }

    suspendAllOn = 0;
    PR_Unlock(ct_book.ml);
#ifdef DEBUG
    etime = PR_IntervalNow();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS,
        ("End PR_ResumeAll (time %dms)\n", etime - stime));
#endif
}  /* PR_ResumeAll */

/* Return the stack pointer for the given thread- used by the GC */
PR_IMPLEMENT(void *)PR_GetSP(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	    ("in PR_GetSP thred %X thid = %X, sp = %X \n", 
	    thred, thred->id, thred->sp));
    return thred->sp;
}  /* PR_GetSP */

#endif  /* defined(_PR_CTHREADS) */

/* ctthread.c */
