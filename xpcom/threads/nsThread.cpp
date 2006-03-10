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
  self->mThread = PR_GetCurrentThread();

  // Inform the ThreadManager
  nsThreadManager::get()->SetupCurrentThread(self, nsnull);

  // Wait for and process startup event
  nsCOMPtr<nsIRunnable> event;
  if (!self->GetEvent(PR_TRUE, getter_AddRefs(event))) {
    NS_WARNING("failed waiting for thread startup event");
    return;
  }
  event->Run();  // unblocks nsThread::Init

  // Now, process incoming events...
  while (self->mActive)
    self->ProcessNextEvent();

  // Inform the threadmanager that this thread is going away
  nsThreadManager::get()->SetupCurrentThread(nsnull, self);

  NS_RELEASE(self);
}

//-----------------------------------------------------------------------------

nsThread::nsThread(const nsACString &name)
  : mLock(PR_NewLock())
  , mEvents(&mEventsRoot)
  , mName(name)
  , mPriority(PRIORITY_NORMAL)
  , mThread(nsnull)
  , mActive(PR_FALSE)
  , mRunningEvent(0)
{
}

nsThread::~nsThread()
{
  PR_DestroyLock(mLock);
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

  // ThreadFunc is responsible for setting mThread
  PRThread *thr = PR_CreateThread(PR_USER_THREAD, ThreadFunc, this,
                                  PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD, 0);
  if (!thr) {
    NS_RELEASE_THIS();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // ThreadFunc will wait for this event to be run before it tries to access
  // mThread.  By delaying insertion of this event into the queue, we ensure
  // that mThread is set properly.
  {
    nsAutoLock lock(mLock);
    mEvents->PutEvent(startup);
  }

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

  return nsThreadManager::get()->SetupCurrentThread(this, nsnull);
}

PRBool
nsThread::PutEvent(nsIRunnable *event)
{
  PRBool rv;
  {
    nsAutoLock lock(mLock);
    rv = mEvents->PutEvent(event);
  }
  if (!rv)
    return PR_FALSE;

  nsCOMPtr<nsIThreadObserver> obs = GetObserver();
  if (obs)
    obs->OnDispatchedEvent(this);

  return PR_TRUE;
}

//-----------------------------------------------------------------------------
// nsIEventTarget

NS_IMETHODIMP
nsThread::Dispatch(nsIRunnable *event, PRUint32 flags)
{
  LOG(("THRD(%p) Dispatch [%p %x]\n", this, event, flags));

  NS_ENSURE_ARG_POINTER(event);
  NS_ENSURE_STATE(mThread);

  if (flags & DISPATCH_SYNC) {
    nsCOMPtr<nsIThread> thread;
    nsThreadManager::get()->GetCurrentThread(getter_AddRefs(thread));
    NS_ENSURE_STATE(thread);

    // XXX we should be able to do something better here... we should
    //     be able to monitor the slot occupied by this event and use
    //     that to tell us when the event has been processed.
 
    nsRefPtr<nsThreadSyncDispatch> wrapper = new nsThreadSyncDispatch(event);
    PutEvent(wrapper);

    while (wrapper->IsPending())
      thread->ProcessNextEvent();
  } else {
    NS_ASSERTION(flags == NS_DISPATCH_NORMAL, "unexpected dispatch flags");
    PutEvent(event);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsThread::IsOnCurrentThread(PRBool *result)
{
  *result = (PR_GetCurrentThread() == mThread);
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsIThread

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
  LOG(("THRD(%p) shutdown\n", this));

  // Maybe we are already shutdown.
  if (!mThread)
    return NS_OK;

  NS_ENSURE_STATE(PR_GetCurrentThread() != mThread);

  // shutdown event queue
  nsCOMPtr<nsIRunnable> event = new nsThreadShutdownEvent(this);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;
  PutEvent(event);

  // XXX we could still end up with other events being added after the shutdown task

  PR_JoinThread(mThread);
  mThread = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::HasPendingEvents(PRBool *result)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  *result = mEvents->GetEvent(PR_FALSE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::ProcessNextEvent()
{
  LOG(("THRD(%p) ProcessNextEvent [%u]\n", this, mRunningEvent));

  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  nsresult rv;

  nsCOMPtr<nsIThreadObserver> obs = mObserver;
  if (obs)
    obs->OnProcessNextEvent(this, mActive, mRunningEvent);

  // If we are shutting down, then do not wait for new events.
  nsCOMPtr<nsIRunnable> event; 
  mEvents->GetEvent(mActive, getter_AddRefs(event));

  if (event) {
    LOG(("THRD(%p) running [%p]\n", this, event.get()));
    ++mRunningEvent;
    event->Run();
    --mRunningEvent;
    rv = NS_OK;
  } else {
    NS_ASSERTION(!mActive, "This should only happen when shutting down");
    rv = NS_ERROR_ABORT;
  }

  return rv;
}

//-----------------------------------------------------------------------------
// nsISupportsPriority

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

//-----------------------------------------------------------------------------
// nsIThreadInternal

NS_IMETHODIMP
nsThread::GetObserver(nsIThreadObserver **obs)
{
  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*obs = mObserver);
  return NS_OK;
}

NS_IMETHODIMP
nsThread::SetObserver(nsIThreadObserver *obs)
{
  NS_ENSURE_STATE(PR_GetCurrentThread() == mThread);

  nsAutoLock lock(mLock);
  mObserver = obs;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PushEventQueue(nsIThreadEventFilter *filter)
{
  nsChainedEventQueue *queue = new nsChainedEventQueue(filter);
  if (!queue)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAutoLock lock(mLock);
  queue->mNext = mEvents;
  mEvents = queue;
  return NS_OK;
}

NS_IMETHODIMP
nsThread::PopEventQueue()
{
  nsAutoLock lock(mLock);

  // Make sure we do not pop too many!
  NS_ENSURE_STATE(mEvents != &mEventsRoot);

  nsChainedEventQueue *queue = mEvents;
  mEvents = mEvents->mNext;

  nsCOMPtr<nsIRunnable> event;
  while (queue->GetEvent(PR_FALSE, getter_AddRefs(event)))
    mEvents->PutEvent(event);
  
  return NS_OK;
}

PRBool
nsThread::nsChainedEventQueue::PutEvent(nsIRunnable *event)
{
  PRBool val;
  if (!mFilter || mFilter->AcceptEvent(event)) {
    val = mQueue.PutEvent(event);
  } else {
    val = mNext->PutEvent(event);
  }
  return val;
}
