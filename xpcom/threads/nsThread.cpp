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

#include "nsThread.h"
#include "nsThreadManager.h"
#include "nsRunnable.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"

NS_DECL_CI_INTERFACE_GETTER(nsThread)

//-----------------------------------------------------------------------------
// Because we do not have our own nsIFactory, we have to implement nsIClassInfo
// somewhat manually.

class nsThreadClassInfo : public nsIClassInfo {
public:
  NS_DECL_ISUPPORTS_INHERITED  // no mRefCnt
  NS_DECL_NSICLASSINFO

  nsThreadClassInfo() {}
};

static nsThreadClassInfo sThreadClassInfo;

NS_IMETHODIMP_(nsrefcnt) nsThreadClassInfo::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsThreadClassInfo::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1(nsThreadClassInfo, nsIClassInfo)

NS_IMETHODIMP
nsThreadClassInfo::GetInterfaces(PRUint32 *count, nsIID ***array)
{
  return NS_CI_INTERFACE_GETTER_NAME(nsThread)(count, array);
}

NS_IMETHODIMP
nsThreadClassInfo::GetHelperForLanguage(PRUint32 lang, nsISupports **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetContractID(char **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassDescription(char **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassID(nsCID **result)
{
  *result = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetImplementationLanguage(PRUint32 *result)
{
  *result = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetFlags(PRUint32 *result)
{
  *result = THREADSAFE;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadClassInfo::GetClassIDNoAlloc(nsCID *result)
{
  return NS_ERROR_NOT_AVAILABLE;
}

//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(nsThread)
NS_IMPL_THREADSAFE_RELEASE(nsThread)
NS_INTERFACE_MAP_BEGIN(nsThread)
  NS_INTERFACE_MAP_ENTRY(nsIThread)
  NS_INTERFACE_MAP_ENTRY(nsIThreadInternal)
  NS_INTERFACE_MAP_ENTRY(nsIDispatchTarget)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIThread)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    foundInterface = NS_STATIC_CAST(nsIClassInfo*, &sThreadClassInfo);
  } else
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER4(nsThread, nsIThread, nsIThreadInternal,
                             nsIDispatchTarget, nsISupportsPriority)

//-----------------------------------------------------------------------------

class nsThreadShutdownTask : public nsRunnable {
public:
  nsThreadShutdownTask(nsThread *thr)
    : mThread(thr) {
  } 

  NS_IMETHODIMP Run() {
    mThread->mActive = PR_FALSE;
    return NS_OK;
  }

private:
  nsRefPtr<nsThread> mThread;
};

//-----------------------------------------------------------------------------

class nsThreadSyncDispatch : public nsRunnable {
public:
  nsThreadSyncDispatch(nsIRunnable *task)
    : mSyncTask(task) {
  }

  NS_IMETHODIMP Run() {
    mSyncTask->Run();
    mSyncTask = nsnull;
    return NS_OK;
  }

  PRBool IsPending() {
    return mSyncTask != nsnull;
  }

private:
  nsCOMPtr<nsIRunnable> mSyncTask;
};

//-----------------------------------------------------------------------------

// This class is used to convey initialization info to the newly created thread.
class nsThreadStartInfo {
private:
  PRInt32 mRefCnt;

  ~nsThreadStartInfo() {
    nsAutoMonitor::DestroyMonitor(mMon);
  }
public:
  PRMonitor *mMon;
  nsThread  *mThread;
  PRBool     mInitialized;

  nsThreadStartInfo(nsThread *thr)
    : mRefCnt(0)
    , mMon(nsAutoMonitor::NewMonitor("nsThreadStartInfo"))
    , mThread(thr)
    , mInitialized(PR_FALSE) {
  }

  void AddRef() {
    PR_AtomicIncrement(&mRefCnt);
  }

  void Release() {
    if (PR_AtomicDecrement(&mRefCnt) == 0)
      delete this;
  }
};

//-----------------------------------------------------------------------------

/*static*/ void
nsThread::ThreadFunc(void *arg)
{
  nsThreadStartInfo *si = NS_STATIC_CAST(nsThreadStartInfo *, arg);

  nsThread *self = si->mThread;
  NS_ADDREF(self);

  // Inform the ThreadManager
  nsThreadManager::get()->SetCurrentThread(self, nsnull);

  // Unblock nsThread::Init
  {
    nsAutoMonitor mon(si->mMon);
    si->mInitialized = PR_TRUE;
    mon.Notify();
  }
  NS_RELEASE(si);

  // Now, process incoming tasks...
  while (self->mActive)
    self->RunNextTask(nsIThread::RUN_NORMAL);

  NS_RELEASE(self);

  // Inform the threadmanager that this thread is going away
  nsThreadManager::get()->SetCurrentThread(nsnull, nsnull);
}

//-----------------------------------------------------------------------------

nsThread::nsThread(const nsACString &name)
  : mObserverLock(PR_NewLock())
  , mName(name)
{
}

nsThread::~nsThread()
{
  PR_DestroyLock(mObserverLock);
}

nsresult
nsThread::Init()
{
  // spawn thread and wait until it is fully setup
  nsThreadStartInfo *si = new nsThreadStartInfo(this);
  if (!si)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(si);

  mActive = PR_TRUE;
  mThread = PR_CreateThread(PR_USER_THREAD, ThreadFunc, si, PR_PRIORITY_NORMAL,
                            PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
  if (!mThread) {
    NS_RELEASE(si);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Wait for thread to call ThreadManager::SetCurrentThread
  {
    nsAutoMonitor mon(si->mMon);
    while (!si->mInitialized)
      mon.Wait();
  }

  NS_RELEASE(si);
  return NS_OK;
}

nsresult
nsThread::InitCurrentThread()
{
  mActive = PR_TRUE;
  mThread = PR_GetCurrentThread();

  nsThreadManager::get()->SetCurrentThread(this, nsnull);
  return NS_OK;
}

PRBool
nsThread::PutTask(nsIRunnable *task, PRUint32 dispatchFlags)
{
  PRBool rv = mTasks.PutTask(task);
  if (!rv)
    return PR_FALSE;

  nsCOMPtr<nsIThreadObserver> obs;
  {
    nsAutoLock lock(mObserverLock);
    obs = mObserver;
  }
  if (obs)
    obs->OnNewTask(this, dispatchFlags);

  return PR_TRUE;
}

NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *runnable, PRUint32 flags)
{
  NS_ENSURE_STATE(mThread);

  if (flags == DISPATCH_NORMAL) {
    PutTask(runnable, flags);
  } else if (flags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_STATE(thread);

    nsRefPtr<nsThreadSyncDispatch> task = new nsThreadSyncDispatch(runnable);
    PutTask(task, flags);

    while (task->IsPending())
      thread->RunNextTask(nsIThread::RUN_NORMAL);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThread::IsOnCurrentThread(PRBool *result)
{
  *result = (PR_GetCurrentThread() == mThread);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::GetName(nsACString &result)
{
  result = mName;  // no need to lock since this never changes
  return NS_OK;
}

NS_IMETHODIMP
nsThread::Shutdown()
{
  NS_ENSURE_STATE(mThread);
  NS_ENSURE_STATE(PR_GetCurrentThread() != mThread);

  // shutdown task queue
  nsCOMPtr<nsIRunnable> task = new nsThreadShutdownTask(this);
  if (!task)
    return NS_ERROR_OUT_OF_MEMORY;
  PutTask(task, NS_DISPATCH_NORMAL);

  // XXX we could still end up with other tasks being added after the shutdown task

  PR_JoinThread(mThread);
  mThread = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::RunNextTask(PRUint32 flags)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);
  NS_ENSURE_STATE(mActive);
  NS_ENSURE_ARG(flags == RUN_NORMAL || flags == RUN_NO_WAIT);

  nsCOMPtr<nsIThreadObserver> obs;
  {
    nsAutoLock lock(mObserverLock);
    obs = mObserver;
  }

  if (obs)
    obs->OnBeforeRunNextTask(this, flags);

  nsCOMPtr<nsIRunnable> task; 
  if (flags == RUN_NORMAL) {
    if (obs)
      obs->OnWaitNextTask(this, flags);
    mTasks.WaitPendingTask(getter_AddRefs(task));
  } else {
    mTasks.GetPendingTask(getter_AddRefs(task));
  }

  nsresult rv = NS_OK;

  if (task) {
    task->Run();
  } else if (flags & RUN_NO_WAIT) {
    rv = NS_BASE_STREAM_WOULD_BLOCK;
  }

  if (obs)
    obs->OnAfterRunNextTask(this, flags, rv);

  return rv;
}

NS_IMETHODIMP
nsThread::GetPriority(PRInt32 *priority)
{
  *priority = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetPriority(PRInt32 priority)
{
  NS_ENSURE_STATE(mThread);

  // NSPR defines the following four thread priorities:
  //   PR_PRIORITY_LOW
  //   PR_PRIORITY_NORMAL
  //   PR_PRIORITY_HIGH
  //   PR_PRIORITY_URGENT
  // We map the priority values defined on nsISupportsPriority to these values.

  mPriority = priority;

  PRThreadPriority pri;
  if (mPriority <= PRIORITY_HIGHEST) {
    pri = PR_PRIORITY_URGENT;
  } else if (mPriority < PRIORITY_NORMAL) {
    pri = PR_PRIORITY_HIGH;
  } else if (mPriority > PRIORITY_NORMAL) {
    pri = PR_PRIORITY_LOW;
  } else {
    pri = PR_PRIORITY_NORMAL;
  }
  PR_SetThreadPriority(mThread, pri);

  return NS_OK;
}

NS_IMETHODIMP
nsThread::AdjustPriority(PRInt32 delta)
{
  return SetPriority(mPriority + delta);
}

NS_IMETHODIMP
nsThread::GetObserver(nsIThreadObserver **obs)
{
  nsAutoLock lock(mObserverLock);
  NS_ADDREF(*obs = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetObserver(nsIThreadObserver *obs)
{
  nsAutoLock lock(mObserverLock);
  mObserver = obs;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingTask(PRBool *result)
{
  *result = mTasks.HasPendingTask();
  return NS_OK;
}
