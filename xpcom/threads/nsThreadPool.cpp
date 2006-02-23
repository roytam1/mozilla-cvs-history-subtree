/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsThreadPool.h"
#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "prinrval.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsThreadPool");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

// DESIGN:
//  o  Allocate anonymous threads.
//  o  Use nsThreadPool::Run as the main routine for each thread.
//  o  Each thread waits on the task queue's monitor, checking for
//     pending tasks and rescheduling itself as an idle thread.

#define DEFAULT_THREAD_LIMIT 4
#define DEFAULT_IDLE_THREAD_LIMIT 1
#define DEFAULT_IDLE_THREAD_TIMEOUT PR_SecondsToInterval(60)

NS_IMPL_THREADSAFE_ADDREF(nsThreadPool)
NS_IMPL_THREADSAFE_RELEASE(nsThreadPool)
NS_IMPL_QUERY_INTERFACE3_CI(nsThreadPool, nsIThreadPool, nsIDispatchTarget,
                            nsIRunnable)
NS_IMPL_CI_INTERFACE_GETTER2(nsThreadPool, nsIThreadPool, nsIDispatchTarget)

nsThreadPool::nsThreadPool()
  : mThreadLimit(DEFAULT_THREAD_LIMIT)
  , mIdleThreadLimit(DEFAULT_IDLE_THREAD_LIMIT)
  , mIdleThreadTimeout(DEFAULT_IDLE_THREAD_TIMEOUT)
  , mIdleCount(0)
  , mShutdown(PR_FALSE)
{
}

nsThreadPool::~nsThreadPool()
{
  Shutdown();
}

nsresult
nsThreadPool::PutTask(nsIRunnable *task)
{
  // Avoid spawning a new thread while holding the task queue lock...
 
  PRBool spawnThread = PR_FALSE;
  {
    nsAutoMonitor mon(mTasks.Monitor());

    LOG(("THRD-P(%p) put [%d %d %d]\n", this, mIdleCount, mThreads.Count(),
         mThreadLimit));
    NS_ASSERTION(mIdleCount <= (PRUint32) mThreads.Count(), "oops");

    // Make sure we have a thread to service this task.
    if (mIdleCount == 0 && mThreads.Count() < (PRInt32) mThreadLimit)
      spawnThread = PR_TRUE;

    mTasks.PutTask(task);
  }

  LOG(("THRD-P(%p) put [spawn=%d]\n", this, spawnThread));
  if (!spawnThread)
    return NS_OK;

  nsCOMPtr<nsIThread> thread;
  nsThreadManager::get()->NewThread(EmptyCString(), getter_AddRefs(thread));
  NS_ENSURE_STATE(thread);

  PRBool killThread = PR_FALSE;
  {
    nsAutoMonitor mon(mTasks.Monitor());
    if (mThreads.Count() < (PRInt32) mThreadLimit) {
      mThreads.AppendObject(thread);
    } else {
      killThread = PR_TRUE;  // okay, we don't need this thread anymore
    }
  }
  LOG(("THRD-P(%p) put [%p kill=%d]\n", this, thread.get(), killThread));
  if (killThread) {
    thread->Shutdown();
  } else {
    thread->Dispatch(this, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Run()
{
  LOG(("THRD-P(%p) enter\n", this));

  nsCOMPtr<nsIThread> current;
  nsThreadManager::get()->GetCurrentThread(getter_AddRefs(current));

  PRBool exitThread = PR_FALSE;
  PRBool wasIdle = PR_FALSE;
  PRIntervalTime idleSince;

  do {
    nsCOMPtr<nsIRunnable> task;
    {
      nsAutoMonitor mon(mTasks.Monitor());
      if (!mTasks.GetPendingTask(getter_AddRefs(task))) {
        PRIntervalTime now     = PR_IntervalNow();
        PRIntervalTime timeout = PR_MillisecondsToInterval(mIdleThreadTimeout);

        // If we are shutting down, then don't keep any idle threads
        if (mShutdown) {
          exitThread = PR_TRUE;
        } else {
          if (wasIdle) {
            // if too many idle threads or idle for too long, then bail.
            if (mIdleCount > mIdleThreadLimit || (now - idleSince) > timeout)
              exitThread = PR_TRUE;
          } else {
            // if would be too many idle threads...
            if (mIdleCount == mIdleThreadLimit) {
              exitThread = PR_TRUE;
            } else {
              ++mIdleCount;
              idleSince = now;
              wasIdle = PR_TRUE;
            }
          }
        }

        if (exitThread) {
          if (wasIdle)
            --mIdleCount;
          mThreads.RemoveObject(current);
        } else {
          PRIntervalTime delta = timeout - (now - idleSince);
          LOG(("THRD-P(%p) waiting [%d]\n", this, delta));
          mon.Wait(delta);
        }
      } else if (wasIdle) {
        wasIdle = PR_FALSE;
        --mIdleCount;
      }
    }
    if (task) {
      LOG(("THRD-P(%p) running [%p]\n", this, task.get()));
      task->Run();
    }
  } while (!exitThread);

  LOG(("THRD-P(%p) leave\n", this));
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Dispatch(nsIRunnable *task, PRUint32 flags)
{
  LOG(("THRD-P(%p) dispatch [%p %x]\n", this, task, flags));

  NS_ENSURE_STATE(!mShutdown);

  if (flags == DISPATCH_NORMAL) {
    PutTask(task);
  } else if (flags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_STATE(thread);

    nsRefPtr<nsThreadSyncDispatch> wrapper = new nsThreadSyncDispatch(task);
    PutTask(wrapper);

    while (wrapper->IsPending())
      thread->RunNextTask(nsIThread::RUN_NORMAL);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::IsOnCurrentThread(PRBool *result)
{
  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::Shutdown()
{
  nsCOMArray<nsIThread> threads;
  {
    nsAutoMonitor mon(mTasks.Monitor());
    mShutdown = PR_TRUE;
    mon.NotifyAll();

    threads.AppendObjects(mThreads);
  }

  // It's important that we shutdown the threads while outside the task queue
  // monitor.  Otherwise, we could end up dead-locking.  The threads will take
  // care of removing themselves from mThreads as they exit.

  for (PRInt32 i = 0; i < threads.Count(); ++i)
    threads[i]->Shutdown();

  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetThreadLimit(PRUint32 *value)
{
  *value = mThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetThreadLimit(PRUint32 value)
{
  nsAutoMonitor mon(mTasks.Monitor());
  mThreadLimit = value;
  if (mIdleThreadLimit > mThreadLimit)
    mIdleThreadLimit = mThreadLimit;
  mon.NotifyAll();  // wake up threads so they observe this change
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadLimit(PRUint32 *value)
{
  *value = mIdleThreadLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadLimit(PRUint32 value)
{
  nsAutoMonitor mon(mTasks.Monitor());
  mIdleThreadLimit = value;
  mon.NotifyAll();  // wake up threads so they observe this change
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::GetIdleThreadTimeout(PRUint32 *value)
{
  *value = mIdleThreadTimeout;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadPool::SetIdleThreadTimeout(PRUint32 value)
{
  nsAutoMonitor mon(mTasks.Monitor());
  mIdleThreadTimeout = value;
  mon.NotifyAll();  // wake up threads so they observe this change
  return NS_OK;
}
