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

#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

//-----------------------------------------------------------------------------

PR_STATIC_CALLBACK(void)
ReleaseObject(void *data)
{
  NS_STATIC_CAST(nsISupports *, data)->Release();
}

PR_STATIC_CALLBACK(PLDHashOperator)
AppendAndRemoveThread(const void *key, nsCOMPtr<nsIThread> &thread, void *arg)
{
  nsCOMArray<nsIThread> *threads =
      NS_STATIC_CAST(nsCOMArray<nsIThread> *, arg);
  threads->AppendObject(thread);
  return PL_DHASH_REMOVE;
}

//-----------------------------------------------------------------------------

nsThreadManager nsThreadManager::sInstance;

// statically allocated instance
NS_IMETHODIMP_(nsrefcnt) nsThreadManager::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsThreadManager::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1_CI(nsThreadManager, nsIThreadManager)
NS_IMPL_CI_INTERFACE_GETTER1(nsThreadManager, nsIThreadManager)

//-----------------------------------------------------------------------------

nsresult
nsThreadManager::Init()
{
  if (!mThreadsByName.Init() || !mThreadsByPRThread.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  if (PR_NewThreadPrivateIndex(&mCurThreadIndex, ReleaseObject) == PR_FAILURE)
    return NS_ERROR_FAILURE;

  // Setup "main" thread
  nsRefPtr<nsThread> mainThread = new nsThread(NS_LITERAL_CSTRING("main"));
  if (!mainThread)
    return NS_ERROR_OUT_OF_MEMORY;
  mainThread->InitCurrentThread();

  mainThread->GetPRThread(&mMainPRThread);
  return NS_OK;
}

void
nsThreadManager::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "shutdown not called from main thread");

  // We move the threads from the hashtable to a list, so that we avoid
  // holding the hashtable lock while calling nsIThread::Shutdown.

  nsCOMArray<nsIThread> threads;
  mThreadsByPRThread.Enumerate(AppendAndRemoveThread, &threads);
  mThreadsByName.Clear();

  // Shutdown all background threads.
  for (PRInt32 i = 0; i < threads.Count(); ++i) {
    nsIThread *thread = threads.ObjectAt(i);

    PRBool isMainThread = PR_FALSE;
    thread->IsOnCurrentThread(&isMainThread);
    if (!isMainThread)
      thread->Shutdown();
  }

  // Remove the TLS entry for the main thread.
  PR_SetThreadPrivate(mCurThreadIndex, nsnull);
}

nsresult
nsThreadManager::SetupCurrentThread(nsIThread *thread, nsIThread *previous)
{
  nsCString name;
  if (thread) {
    thread->GetName(name);
    if (!name.IsEmpty()) {
      // make sure thread name is unique
      nsCOMPtr<nsIThread> temp;
      GetThread(name, getter_AddRefs(temp));

      if (temp && temp != previous) {
        NS_NOTREACHED("thread name is not unique");
        return NS_ERROR_INVALID_ARG;
      }
      mThreadsByName.Put(name, thread);
    }

    PRThread *prthread = nsnull;
    thread->GetPRThread(&prthread);
    NS_ASSERTION(prthread, "no prthread");

#ifdef DEBUG
    if (previous) {
      PRThread *prthreadPrev = nsnull;
      previous->GetPRThread(&prthreadPrev);
      NS_ASSERTION(prthreadPrev, "no previous prthread");
      NS_ASSERTION(prthread == prthreadPrev, "prthread changed");
    }
#endif

    mThreadsByPRThread.Put(prthread, thread);

    NS_ADDREF(thread);  // for TLS entry
  } else if (previous) {
    previous->GetName(name);
    if (!name.IsEmpty())
      mThreadsByName.Remove(name);

    PRThread *prthread = nsnull;
    previous->GetPRThread(&prthread);
    NS_ASSERTION(prthread, "no prthread");
    mThreadsByPRThread.Remove(prthread);
  }

  // write thread local storage
  PR_SetThreadPrivate(mCurThreadIndex, thread);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::NewThread(const nsACString &name, nsIThread **result)
{
  // make sure name is unique
  if (!name.IsEmpty() && mThreadsByName.Get(name, nsnull)) {
    NS_WARNING("Thread name is not unique");
    return NS_ERROR_INVALID_ARG;
  }

  nsThread *thr = new nsThread(name);
  if (!thr)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(thr);

  nsresult rv = thr->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(thr);
    return rv;
  }

  // At this point, we expect that the thread has been registered in mThread;
  // however, it is possible that it could have also been replaced by now, so
  // we cannot really assert that it was added.

  *result = thr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetThread(const nsACString &name, nsIThread **result)
{
  if (name.IsEmpty()) {
    *result = nsnull;
    return NS_OK;
  }
  mThreadsByName.Get(name, result);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetThreadFromPRThread(PRThread *thread, nsIThread **result)
{
  NS_ENSURE_ARG_POINTER(thread);
  mThreadsByPRThread.Get(thread, result);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetMainThread(nsIThread **result)
{
  mThreadsByPRThread.Get(mMainPRThread, result);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetCurrentThread(nsIThread **result)
{
  // read thread local storage
  void *data = PR_GetThreadPrivate(mCurThreadIndex);
  if (data) {
    NS_ADDREF(*result = NS_STATIC_CAST(nsIThread *, data)); 
  } else {
    // OK, that's fine.  We'll dynamically create one :-)
    nsRefPtr<nsThread> thread = new nsThread(EmptyCString());
    if (!thread)
      return NS_ERROR_OUT_OF_MEMORY;
    thread->InitCurrentThread();
    NS_ADDREF(*result = thread);
  }
  return NS_OK;
}

#if 0
NS_IMETHODIMP
nsThreadManager::SetCurrentThread(nsIThread *thread, nsIThread **result)
{
  nsCOMPtr<nsIThread> prev;
  GetCurrentThread(getter_AddRefs(prev));

  nsresult rv = SetupCurrentThread(thread, prev);
  if (NS_FAILED(rv))
    return rv;

  if (result) {
    *result = nsnull;
    prev.swap(*result);
  }

  return NS_OK;
}
#endif

NS_IMETHODIMP
nsThreadManager::GetIsMainThread(PRBool *result)
{
  *result = (PR_GetCurrentThread() == mMainPRThread);
  return NS_OK;
}
