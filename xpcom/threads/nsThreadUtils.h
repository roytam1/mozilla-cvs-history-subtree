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

#ifndef nsThreadUtils_h__
#define nsThreadUtils_h__

#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "prinrval.h"

#ifdef MOZILLA_INTERNAL_API
# include "nsString.h"
# define NS_THREADUTILS_METHOD_(x) extern NS_COM NS_METHOD_(x)
#else
# include "nsXPCOMCID.h"
# include "nsStringAPI.h"
# include "nsIThreadManager.h"
# include "nsServiceManagerUtils.h"
# define NS_THREADUTILS_METHOD_(x) inline NS_METHOD_(x)
#endif
#define NS_THREADUTILS_METHOD NS_THREADUTILS_METHOD_(nsresult)

class nsIThread;

//-----------------------------------------------------------------------------
// These methods are alternatives to the methods on nsIThreadManager, provided
// for convenience.

/**
 * Create a new thread, and optionally provide an initial event for the thread.
 *
 * @param result
 *   The resulting nsIThread object.
 * @param event
 *   The initial event to run on this thread.  This parameter can be null.
 * @param name
 *   The name of the thread, which must be unique, or the empty string to
 *   create an anonymous thread.
 */
NS_THREADUTILS_METHOD
NS_NewThread(nsIThread **result, nsIRunnable *event = nsnull,
             const nsACString &name = EmptyCString());

/**
 * Get a reference to the current thread.
 *
 * @param result
 *   The resulting nsIThread object.
 */
NS_THREADUTILS_METHOD
NS_GetCurrentThread(nsIThread **result);

/**
 * Get a reference to the main thread.
 *
 * @param result
 *   The resulting nsIThread object.
 */
NS_THREADUTILS_METHOD
NS_GetMainThread(nsIThread **result);

/**
 * Get a reference to the thread with the given name.
 *
 * @param result
 *   The resulting nsIThread object.
 */
NS_THREADUTILS_METHOD
NS_GetThread(nsIThread **result, const nsACString &name);

/**
 * Test to see if the current thread is the main thread.
 *
 * @returns PR_TRUE if the current thread is the main thread, and PR_FALSE
 * otherwise.
 */
NS_THREADUTILS_METHOD_(PRBool)
NS_IsMainThread();

/**
 * Dispatch the given event to the current thread.
 *
 * @param event
 *   The event to dispatch.
 */
NS_THREADUTILS_METHOD
NS_DispatchToCurrentThread(nsIRunnable *event);

/**
 * Dispatch the given event to the main thread.
 *
 * @param event
 *   The event to dispatch.
 * @param dispatchFlags
 *   The flags to pass to the main thread's dispatch method.
 */
NS_THREADUTILS_METHOD
NS_DispatchToMainThread(nsIRunnable *event,
                        PRUint32 dispatchFlags = NS_DISPATCH_NORMAL);

/**
 * Process all pending events for the given thread before returning.  This
 * method simply calls ProcessNextEvent on the thread while HasPendingEvents
 * continues to return true and the time spent in NS_ProcessPendingEvents
 * does not exceed the given timeout value.
 *
 * @param thread
 *   The thread object for which to process pending events.  If null, then
 *   events will be processed for the current thread.
 * @param timeout
 *   The maximum number of milliseconds to spend processing pending events.
 *   Events are not pre-empted to honor this timeout.  Rather, the timeout
 *   value is simply used to determine whether or not to process another event.
 *   Pass PR_INTERVAL_NO_TIMEOUT to specify no timeout.
 */
NS_THREADUTILS_METHOD
NS_ProcessPendingEvents(nsIThread *thread,
                        PRIntervalTime timeout = PR_INTERVAL_NO_TIMEOUT);

//-----------------------------------------------------------------------------

/**
 * Variant on NS_NewThread.
 */
inline NS_METHOD
NS_NewThread(nsIThread **result, nsIRunnable *event, const char *name) {
  NS_ASSERTION(name, "thread name must not be null");
  return NS_NewThread(result, event, nsDependentCString(name));
}

/**
 * Variant on NS_GetThread.
 */
inline NS_METHOD
NS_GetThread(nsIThread **result, const char *name) {
  NS_ASSERTION(name, "thread name must not be null");
  return NS_GetThread(result, nsDependentCString(name));
}

//-----------------------------------------------------------------------------
// Helpers that work with nsCOMPtr:

inline already_AddRefed<nsIThread>
do_GetCurrentThread() {
  nsIThread *thread = nsnull;
  NS_GetCurrentThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetMainThread() {
  nsIThread *thread = nsnull;
  NS_GetMainThread(&thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetThread(const nsACString &name) {
  nsIThread *thread = nsnull;
  NS_GetThread(&thread, name);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetThread(const char *name) {
  nsIThread *thread = nsnull;
  NS_GetThread(&thread, name);
  return already_AddRefed<nsIThread>(thread);
}

//-----------------------------------------------------------------------------

#ifndef MOZILLA_INTERNAL_API

inline NS_METHOD
NS_NewThread(nsIThread **result, nsIRunnable *event, const nsACString &name) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mgr->NewThread(name, result);
  NS_ENSURE_SUCCESS(rv, rv);

  if (event)
    rv = (*result)->Dispatch(event, NS_DISPATCH_NORMAL);
  return rv;
}

inline NS_METHOD
NS_GetCurrentThread(nsIThread **result) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mgr->GetCurrentThread(result);
}

inline NS_METHOD
NS_GetMainThread(nsIThread **result) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mgr->GetMainThread(result);
}

inline NS_METHOD
NS_GetThread(nsIThread **result, const nsACString &name) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mgr->GetThread(name, result);
}

inline NS_METHOD_(PRBool)
NS_IsMainThread() {
  PRBool result = PR_FALSE;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID);
  if (mgr)
    mgr->GetIsMainThread(&result);
  return result;
}

inline NS_METHOD
NS_DispatchToCurrentThread(nsIRunnable *event)
{
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_GetCurrentThread(getter_AddRefs(thread));
  if (NS_FAILED(rv))
    return rv;
  return thread->Dispatch(event, NS_DISPATCH_NORMAL);
}

inline NS_METHOD
NS_DispatchToMainThread(nsIRunnable *event, PRUint32 dispatchFlags)
{
  nsCOMPtr<nsIThread> thread;
  nsresult rv = NS_GetMainThread(getter_AddRefs(thread));
  if (NS_FAILED(rv))
    return rv;
  return thread->Dispatch(event, dispatchFlags);
}

inline NS_METHOD
NS_ProcessPendingEvents(nsIThread *thread, PRIntervalTime timeout)
{
  PRIntervalTime start = PR_IntervalNow();

  nsCOMPtr<nsIThread> current;
  if (!thread) {
    current = do_GetCurrentThread();
    thread = current.get();
  }

  nsresult rv;
  PRBool val;
  while (NS_SUCCEEDED(thread->HasPendingEvents(&val)) && val) {
    rv = thread->ProcessNextEvent();
    if (NS_FAILED(rv))
      break;
    if (PR_IntervalNow() - start > timeout)
      break;
  }
  return rv;
}

#else  // MOZILLA_INTERNAL_API

//-----------------------------------------------------------------------------
// This class is designed to be subclassed.

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_DEFAULT

class NS_COM nsRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsRunnable() {
  }

protected:
  virtual ~nsRunnable() {
  }
};

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN

#endif  // MOZILLA_INTERNAL_API

#endif  // nsThreadUtils_h__
