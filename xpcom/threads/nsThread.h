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

#ifndef nsThread_h__
#define nsThread_h__

#include "nsIThreadInternal.h"
#include "nsISupportsPriority.h"
#include "nsTaskQueue.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsCOMPtr.h"

// A native thread
class nsThread : public nsIThreadInternal, public nsISupportsPriority
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDISPATCHTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITHREADINTERNAL
  NS_DECL_NSISUPPORTSPRIORITY

  nsThread(const nsACString &name);

  // Initialize this as a wrapper for a new PRThread.
  nsresult Init();

  // Initialize this as a wrapper for the current PRThread.
  nsresult InitCurrentThread();

private:
  friend class nsThreadShutdownTask;
  friend class nsThreadManager;

  ~nsThread();

  PR_STATIC_CALLBACK(void) ThreadFunc(void *arg);

  // Wrapper for nsTaskQueue::PutTask
  PRBool PutTask(nsIRunnable *task, PRUint32 dispatchFlags);

  PRLock                     *mObserverLock;
  nsCOMPtr<nsIThreadObserver> mObserver;

  nsTaskQueue mTasks;
  nsCString   mName;
  PRInt32     mPriority;
  PRThread   *mThread;
  PRIntn      mActive;
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

#endif  // nsThread_h__
