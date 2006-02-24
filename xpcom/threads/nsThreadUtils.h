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
#include "nsStringGlue.h"

class nsIThread;

//-----------------------------------------------------------------------------
// This class is designed to be subclassed.

#ifdef MOZILLA_INTERNAL_API
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

//-----------------------------------------------------------------------------
// These methods are alternatives to the methods on nsIThreadManager, provided
// for convenience.

/**
 * Create a new thread, and optionally provide an initial task for the thread.
 * @param name
 *        The name of the thread (may be empty).
 * @param runnable
 *        The initial task to run on this thread.
 * @param result
 *        The resulting thread.
 */
extern NS_COM NS_METHOD
NS_NewThread(const nsACString &name, nsIRunnable *runnable, nsIThread **result);

/**
 * Equivalent to nsIThreadManager::GetCurrentThread.
 */
extern NS_COM NS_METHOD
NS_GetCurrentThread(nsIThread **result);

/**
 * Equivalent to nsIThreadManager::GetMainThread.
 */
extern NS_COM NS_METHOD
NS_GetMainThread(nsIThread **result);

/**
 * Equivalent to nsIThreadManager::GetThread.
 */
extern NS_COM NS_METHOD
NS_GetThread(const nsACString &name, nsIThread **result);

/**
 * Equivalent to nsIThreadManager::GetIsMainThread.
 */
extern NS_COM PRBool
NS_IsMainThread();

#else   // MOZILLA_INTERNAL_API
#include "nsXPCOMCID.h"
#include "nsIThreadManager.h"
#include "nsServiceManagerUtils.h"

/**
 * XPCOM glue compatible versions
 */

inline NS_METHOD
NS_NewThread(const nsACString &name, nsIRunnable *runnable, nsIThread **result) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mgr->NewThread(name, result);
  NS_ENSURE_SUCCESS(rv, rv);

  if (runnable)
    rv = (*result)->Dispatch(runnable, NS_DISPATCH_NORMAL);
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
NS_GetThread(const nsACString &name, nsIThread **result) {
  nsresult rv;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return mgr->GetThread(name, result);
}

inline PRBool
NS_IsMainThread() {
  PRBool result = PR_FALSE;
  nsCOMPtr<nsIThreadManager> mgr =
      do_GetService(NS_THREADMANAGER_CONTRACTID);
  if (mgr)
    mgr->GetIsMainThread(&result);
  return result;
}


#endif  // MOZILLA_INTERNAL_API

/**
 * Variant on NS_NewThread.
 */
inline NS_METHOD
NS_NewThread(const char *name, nsIRunnable *runnable, nsIThread **result) {
  return NS_NewThread(nsDependentCString(name), runnable, result);
}

/**
 * Variant on NS_GetThread.
 */
inline NS_METHOD
NS_GetThread(const char *name, nsIThread **result) {
  return NS_GetThread(nsDependentCString(name), result);
}

/**
 * Helpers that work with nsCOMPtr.
 */

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
  NS_GetThread(name, &thread);
  return already_AddRefed<nsIThread>(thread);
}

inline already_AddRefed<nsIThread>
do_GetThread(const char *name) {
  nsIThread *thread = nsnull;
  NS_GetThread(name, &thread);
  return already_AddRefed<nsIThread>(thread);
}

/**
 * Run all pending tasks for a given thread before returning.
 */
inline NS_METHOD
NS_RunPendingTasks(nsIThread *thread)
{
  nsresult rv;
  do {
    rv = thread->RunNextTask(nsIThread::RUN_NO_WAIT);  
  } while (NS_SUCCEEDED(rv));

  if (rv == NS_BASE_STREAM_WOULD_BLOCK)
    rv = NS_OK;

  return rv;
}

#endif  // nsThreadUtils_h__
