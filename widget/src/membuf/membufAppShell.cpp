/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Anya server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
 *    Peter Amstutz <tetron@interreality.org>
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
 */

#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsHashtable.h"
#include "nsVoidArray.h"
#include "membufAppShell.h"

#ifdef WIN32
#include <windows.h>
#endif

#define DEBUG_APPSHELL 1

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

static PLHashTable *sQueueHashTable = nsnull;
static PLHashTable *sCountHashTable = nsnull;
static nsVoidArray *sEventQueueList = nsnull;

NS_IMPL_ISUPPORTS1(membufAppShell, nsIAppShell)

static PRBool gRunning = 0;

membufAppShell::membufAppShell()
{
    NS_INIT_ISUPPORTS();

  if (!sEventQueueList)
    sEventQueueList = new nsVoidArray();

  mEventQueue = nsnull;
}

membufAppShell::~membufAppShell()
{
}

NS_IMETHODIMP
membufAppShell::Create(int *argc, char **argv)
{
    return NS_OK;
}

NS_IMETHODIMP
membufAppShell::Run()
{
    gRunning = 1;

#ifdef WIN32
    // Stay inside native event loop until somebody calls ::Exit()
    MSG msg;
    HANDLE hFakeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    while (gRunning) {
        while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (!::GetMessage(&msg, NULL, 0, 0))
                gRunning = PR_FALSE;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        // Do idle stuff
        ::MsgWaitForMultipleObjects(1, &hFakeEvent, FALSE, 100, QS_ALLEVENTS);
    }

    ::CloseHandle(hFakeEvent);
#else
    while(gRunning) {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(mEventQueue->GetEventQueueSelectFD(), &readset);

        select(mEventQueue->GetEventQueueSelectFD()+1, &readset, 0, 0, 0);
        mEventQueue->ProcessPendingEvents();
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP
membufAppShell::Spinup()
{
  nsresult rv = NS_OK;

#ifdef DEBUG_APPSHELL
  printf("nsAppShell::Spinup()\n");
#endif

  /* Get the event queue service */
  nsCOMPtr<nsIEventQueueService> eventQService = do_GetService(kEventQueueServiceCID, &rv);

  if (NS_FAILED(rv)) {
    NS_WARNING("Could not obtain event queue service");
    return rv;
  }

  /* Get the event queue for the thread.*/
  rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));

  /* If we got an event queue, use it. */
  if (!mEventQueue) {
    /* otherwise create a new event queue for the thread */
    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not create the thread event queue");
      return rv;
    }

    /* Ask again nicely for the event queue now that we have created one. */
    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get the thread event queue");
      return rv;
    }
  }

  ListenToEventQueue(mEventQueue, PR_TRUE);

  return rv;
}

NS_IMETHODIMP
membufAppShell::Spindown()
{
    return NS_OK;
}

#define NUMBER_HASH_KEY(_num) ((PLHashNumber) _num)

static PLHashNumber
IntHashKey(PRInt32 key)
{
  return NUMBER_HASH_KEY(key);
}

// wrapper so we can call a macro
PR_BEGIN_EXTERN_C
static unsigned long getNextRequest (void *aClosure) {
    return true;
}
PR_END_EXTERN_C

NS_IMETHODIMP
membufAppShell::ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen)
{
  if (!mEventQueue) {
    NS_WARNING("nsAppShell::ListenToEventQueue(): No event queue available.");
    return NS_ERROR_NOT_INITIALIZED;
  }

#ifdef DEBUG_APPSHELL
  printf("ListenToEventQueue(%p, %d) this=%p\n", aQueue, aListen, this);
#endif
  if (!sQueueHashTable) {
    sQueueHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }
  if (!sCountHashTable) {
    sCountHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }

  //int   queue_fd = aQueue->GetEventQueueSelectFD();
  void *key      = aQueue;
  if (aListen) {
    /* Add listener -
     * but only if we arn't already in the table... */
    if (!PL_HashTableLookup(sQueueHashTable, key)) {

        PL_HashTableAdd(sQueueHashTable, key, 0);

        PLEventQueue *plqueue;
        aQueue->GetPLEventQueue(&plqueue);
        PL_RegisterEventIDFunc(plqueue, getNextRequest, 0);
        sEventQueueList->AppendElement(plqueue);
    }
  } else {
    /* Remove listener... */
    PLEventQueue *plqueue;
    aQueue->GetPLEventQueue(&plqueue);
    PL_UnregisterEventIDFunc(plqueue);
    sEventQueueList->RemoveElement(plqueue);

    int tag = long(PL_HashTableLookup(sQueueHashTable, key));
    if (tag) {
      PL_HashTableRemove(sQueueHashTable, key);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
membufAppShell::GetNativeEvent(PRBool & aRealEvent, void * & aEvent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufAppShell::DispatchNativeEvent(PRBool aRealEvent, void * aEvent)
{
    mEventQueue->ProcessPendingEvents();
    return NS_OK;
    //return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufAppShell::Exit()
{
    gRunning = 0;
    return NS_OK;
}
