/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Michael Lowe <michael.lowe@bigfoot.com>
 *   Darin Fisher <darin@meer.net>
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

#include "nsBaseAppShell.h"
#include "nsThreadUtils.h"

// When processing the next thread event, the appshell may process native
// events (if not in performance mode), which can result in suppressing the
// next thread event for at most this many ticks:
#define THREAD_EVENT_STARVATION_LIMIT PR_MillisecondsToInterval(20)

// Must be threadsafe since nsIThreadObserver::OnNewTask may be called from
// any background thread.
NS_IMPL_THREADSAFE_ISUPPORTS2(nsBaseAppShell, nsIAppShell, nsIThreadObserver)

//-------------------------------------------------------------------------
// nsIAppShell methods:

NS_IMETHODIMP
nsBaseAppShell::Init(int *argc, char **argv)
{
  // Configure ourselves as an observer for the current thread:

  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  NS_ENSURE_STATE(thread);

  nsCOMPtr<nsIThreadInternal> threadInt = do_QueryInterface(thread);
  NS_ENSURE_STATE(threadInt);
  threadInt->SetObserver(this);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Run(void)
{
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  NS_ENSURE_STATE(thread);

  ++mKeepGoing;
  while (mKeepGoing)
    thread->ProcessNextEvent();

  NS_ProcessPendingEvents(thread);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::Exit(void)
{
  // Allow more calls to Exit than Run.
  if (--mKeepGoing < 0)
    mKeepGoing = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::FavorPerformanceHint(PRBool favorPerfOverStarvation,
                                     PRUint32 starvationDelay)
{
  mStarvationDelay = PR_MillisecondsToInterval(starvationDelay);
  if (favorPerfOverStarvation) {
    ++mFavorPerf;
  } else {
    --mFavorPerf;
    mSwitchTime = PR_IntervalNow();
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
// nsIThreadObserver methods:

NS_IMETHODIMP
nsBaseAppShell::OnDispatchedEvent(nsIThreadInternal *thr)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBaseAppShell::OnProcessNextEvent(nsIThreadInternal *thr, PRBool mayWait,
                                   PRUint32 recursionDepth)
{
  PRIntervalTime start = PR_IntervalNow();
  PRIntervalTime limit = THREAD_EVENT_STARVATION_LIMIT;

  if (mFavorPerf <= 0 && start > mSwitchTime + mStarvationDelay) {
    // Favor pending native events
    PRIntervalTime now = start;
    PRBool keepGoing;
    do {
      mLastNativeEventTime = now;
      keepGoing = ProcessNextNativeEvent(PR_FALSE);
    } while (keepGoing && ((now = PR_IntervalNow()) - start) < limit);
  } else {
    // Avoid starving native events completely when in performance mode
    if (start - mLastNativeEventTime > limit) {
      mLastNativeEventTime = start;
      ProcessNextNativeEvent(PR_FALSE);
    }
  }

  PRBool val;
  while (NS_SUCCEEDED(thr->HasPendingEvents(&val)) && !val) {
    // If we have been asked to exit from Run, then we should not wait for
    // events to process.  We also want to make sure that the thread event
    // queue does not block on its monitor, as it normally would do if it did
    // not have any pending events.  To avoid that, we simply insert a dummy
    // event into its queue during shutdown.
    if (!mKeepGoing && mayWait) {
      mayWait = PR_FALSE;
      nsCOMPtr<nsIRunnable> dummyEvent = new nsRunnable();
      thr->Dispatch(dummyEvent, NS_DISPATCH_NORMAL);
    }
    mLastNativeEventTime = PR_IntervalNow();
    if (!ProcessNextNativeEvent(mayWait) && !mayWait)
      break;
  }
  return NS_OK;
}
