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
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsThread");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

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
  NS_INTERFACE_MAP_ENTRY(nsIEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIThread)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    foundInterface = NS_STATIC_CAST(nsIClassInfo*, &sThreadClassInfo);
  } else
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER4(nsThread, nsIThread, nsIThreadInternal,
                             nsIEventTarget, nsISupportsPriority)

//-----------------------------------------------------------------------------

class nsThreadShutdownEvent : public nsRunnable {
public:
  nsThreadShutdownEvent(nsThread *thr)
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

// This class is used to convey initialization info to the newly created thread.
class nsThreadStartup : public nsRunnable {
public:
  nsThreadStartup()
    : mMon(nsAutoMonitor::NewMonitor("xpcom.threadstartup"))
    , mInitialized(PR_FALSE) {
  }

  NS_IMETHOD Run() {
    nsAutoMonitor mon(mMon);
    mInitialized = PR_TRUE;
    mon.Notify();
    return NS_OK;
  }

  void Wait() {
    nsAutoMonitor mon(mMon);
    while (!mInitialized)
      mon.Wait();
  }

private:
  virtual ~nsThreadStartup() {
    nsAutoMonitor::DestroyMonitor(mMon);
  }

  PRMonitor *mMon;
  PRBool     mInitialized;
};

//-----------------------------------------------------------------------------

/*static*/ void
nsThread::ThreadFunc(void *arg)
{
  nsThread *self = NS_STATIC_CAST(nsThread *, arg);  // strong reference

  // Inform the ThreadManager
  nsThreadManager::get()->SetCurrentThread(self, nsnull);

  nsCOMPtr<nsIRunnable> event;
  self->mEvents.WaitPendingEvent(getter_AddRefs(event));
  if (!event) {
    NS_NOTREACHED("WaitPendingEvent failed");
    return;
  }
  event->Run();  // unblocks nsThread::Init

  // Now, process incoming events...
  while (self->mActive)
    self->ProcessNextEvent();

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
 
  nsRefPtr<nsThreadStartup> startup = new nsThreadStartup();
  if (!startup)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF_THIS();
 
  mActive = PR_TRUE;
  mThread = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 0);
  if (!mThread) {
    NS_RELEASE_THIS();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // ThreadFunc will wait for this event to be run before it tries to access
  // mThread.  By delaying insertion of this event into the queue, we ensure
  // that mThread is set properly.
  mEvents.PutEvent(startup);

  // Wait for thread to call ThreadManager::SetCurrentThread, which completes
  // initialization of ThreadFunc.
  startup->Wait();

  return NS_OK;
}

nsresult
nsThread::InitCurrentThread()
{
  mActive = PR_TRUE;
  mThread = PR_GetCurrentThread();

  nsThreadManager::get()->SetupCurrentThread(this, nsnull);
  return NS_OK;
}

PRBool
nsThread::PutEvent(nsIRunnable *event, PRUint32 dispatchFlags)
{
  PRBool rv = mEvents.PutEvent(event);
  if (!rv)
    return PR_FALSE;

  nsCOMPtr<nsIThreadObserver> obs;
  {
    nsAutoLock lock(mObserverLock);
    obs = mObserver;
  }
  if (obs)
    obs->OnDispatchEvent(this, dispatchFlags);

  return PR_TRUE;
}

NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *runnable, PRUint32 flags)
{
  LOG(("THRD(%p) Dispatch [%p %x]\n", this, runnable, flags));

  NS_ENSURE_STATE(mThread);

  if (flags == DISPATCH_NORMAL) {
    PutEvent(runnable, flags);
  } else if (flags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_STATE(thread);

    nsRefPtr<nsThreadSyncDispatch> event = new nsThreadSyncDispatch(runnable);
    PutEvent(event, flags);

    while (event->IsPending())
      thread->ProcessNextEvent();
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
nsThread::GetPRThread(PRThread **result)
{
  *result = mThread;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::Shutdown()
{
  NS_ENSURE_STATE(mThread);
  NS_ENSURE_STATE(PR_GetCurrentThread() != mThread);

  // shutdown event queue
  nsCOMPtr<nsIRunnable> event = new nsThreadShutdownEvent(this);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;
  PutEvent(event, NS_DISPATCH_NORMAL);

  // XXX we could still end up with other events being added after the shutdown task

  PR_JoinThread(mThread);
  mThread = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::ProcessNextEvent()
{
  LOG(("THRD(%p) ProcessNextEvent\n", this));

  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  nsCOMPtr<nsIThreadObserver> obs;
  {
    nsAutoLock lock(mObserverLock);
    obs = mObserver;
  }

  if (obs)
    obs->OnEnterProcessNextEvent(this, mActive);

  // If we are shutting down, then do not wait for new events.
  nsCOMPtr<nsIRunnable> event; 
  mEvents.GetEvent(mActive, getter_AddRefs(event));

  nsresult rv = NS_OK;

  if (event) {
    LOG(("THRD(%p) running [%p]\n", this, event.get()));
    event->Run();
  } else {
    NS_ASSERTION(!mActive, "This should only happen when shutting down");
    rv = NS_ERROR_ABORT;
  }

  if (obs)
    obs->OnLeaveProcessNextEvent(this, rv);

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
nsThread::HasPendingEvents(PRBool *result)
{
  *result = mEvents.HasPendingEvent();
  return NS_OK;
}
