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

#ifndef nsTaskQueue_h__
#define nsTaskQueue_h__

#include <stdlib.h>
#include "prmon.h"
#include "nsIRunnable.h"

// A threadsafe FIFO task queue...
class nsTaskQueue
{
public:
  nsTaskQueue();
  ~nsTaskQueue();

  // This method adds a new task on the pending task queue.
  PRBool PutTask(nsIRunnable *runnable);

  // This method returns true if there is a pending task.
  PRBool HasPendingTask() {
    return GetPendingTask(PR_FALSE, nsnull);
  }

  // This method returns the next pending task or null.
  PRBool GetPendingTask(nsIRunnable **runnable) {
    return GetPendingTask(PR_FALSE, runnable);
  }

  // This method waits for and returns the next pending task.
  PRBool WaitPendingTask(nsIRunnable **runnable) {
    return GetPendingTask(PR_TRUE, runnable);
  }

  // Expose the task queue's monitor for "power users"
  PRMonitor *Monitor() {
    return mMonitor;
  }

private:

  PRBool GetPendingTask(PRBool wait, nsIRunnable **runnable);

  PRBool IsEmpty() {
    return !mHead || (mHead == mTail && mOffsetHead == mOffsetTail);
  }

  enum { TASKS_PER_PAGE = 15 };

  struct Page {
    struct Page *mNext;
    nsIRunnable *mTasks[TASKS_PER_PAGE];

    Page() : mNext(nsnull) {}
  };

  static Page *NewPage() {
    return NS_STATIC_CAST(Page *, calloc(1, sizeof(Page)));
  }

  static void FreePage(Page *p) {
    free(p);
  }

  PRMonitor *mMonitor;

  Page *mHead;
  Page *mTail;

  PRUint16 mOffsetHead;  // offset into mHead where next item is removed
  PRUint16 mOffsetTail;  // offset into mTail where next item is added
};

#endif  // nsTaskQueue_h__
