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

#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsIWidget.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include <windows.h>

// unknwn.h is needed to build with WIN32_LEAN_AND_MEAN
#include <unknwn.h>

#include "nsWidgetsCID.h"
#include "aimm.h"

NS_IMPL_ISUPPORTS2(nsAppShell, nsIAppShell, nsIThreadObserver)

// XXX why is this not a member variable?
static int gKeepGoing = 1;

#if 0
UINT _pr_PostEventMsgId;
static char *_pr_eventWindowClass = "XPCOM:EventWindow";

static LPCTSTR _md_GetEventQueuePropName() {
    static ATOM atom = 0;
    if (!atom) {
        atom = GlobalAddAtom("XPCOM_EventQueue");
    }
    return MAKEINTATOM(atom);
}

static PRBool   _md_WasInputPending = PR_FALSE;
static PRUint32 _md_InputTime = 0;
static PRBool   _md_WasPaintPending = PR_FALSE;
static PRUint32 _md_PaintTime = 0;
/* last mouse location */
static POINT    _md_LastMousePos;

/*******************************************************************************
 * Timer callback function. Timers are used on WIN32 instead of APP events
 * when there are pending UI events because APP events can cause the GUI to lockup
 * because posted messages are processed before other messages.
 ******************************************************************************/

static void CALLBACK _md_TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    PREventQueue* queue =  (PREventQueue  *) GetProp(hwnd, _md_GetEventQueuePropName());
    PR_ASSERT(queue != NULL);

    KillTimer(hwnd, TIMER_ID);
    queue->timerSet = PR_FALSE;
    queue->removeMsg = PR_FALSE;
    PL_ProcessPendingEvents( queue );
    queue->removeMsg = PR_TRUE;
}

static PRBool _md_IsWIN9X = PR_FALSE;
static PRBool _md_IsOSSet = PR_FALSE;

static void _md_DetermineOSType()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    if (VER_PLATFORM_WIN32_WINDOWS == os.dwPlatformId) {
        _md_IsWIN9X = PR_TRUE;
    }
}

static PRUint32 _md_GetPaintStarvationLimit()
{
    if (! _md_IsOSSet) {
        _md_DetermineOSType();
        _md_IsOSSet = PR_TRUE;
    }

    if (_md_IsWIN9X) {
        return WIN9X_PAINT_STARVATION_LIMIT;
    }

    return PAINT_STARVATION_LIMIT;
}


/*
 * Determine if an event is being starved (i.e the starvation limit has
 * been exceeded.
 * Note: this function uses the current setting and updates the contents
 * of the wasPending and lastTime arguments
 *
 * ispending:       PR_TRUE if the event is currently pending
 * starvationLimit: Threshold defined in milliseconds for determining when
 *                  the event has been held in the queue too long
 * wasPending:      PR_TRUE if the last time _md_EventIsStarved was called
 *                  the event was pending.  This value is updated within
 *                  this function.
 * lastTime:        Holds the last time the event was in the queue.
 *                  This value is updated within this function
 * returns:         PR_TRUE if the event is starved, PR_FALSE otherwise
 */

static PRBool _md_EventIsStarved(PRBool isPending, PRUint32 starvationLimit,
                                 PRBool *wasPending, PRUint32 *lastTime,
                                 PRUint32 currentTime)
{
    if (*wasPending && isPending) {
        /*
         * It was pending previously and the event is still
         * pending so check to see if the elapsed time is
         * over the limit which indicates the event was starved
         */
        if ((currentTime - *lastTime) > starvationLimit) {
            return PR_TRUE; /* pending and over the limit */
        }

        return PR_FALSE; /* pending but within the limit */
    }

    if (isPending) {
        /*
         * was_pending must be false so record the current time
         * so the elapsed time can be computed the next time this
         * function is called
         */
        *lastTime = currentTime;
        *wasPending = PR_TRUE;
        return PR_FALSE;
    }

    /* Event is no longer pending */
    *wasPending = PR_FALSE;
    return PR_FALSE;
}

/* Determines if the there is a pending Mouse or input event */

static PRBool _md_IsInputPending(WORD qstatus)
{
    /* Return immediately there aren't any pending input or paints. */
    if (qstatus == 0) {
        return PR_FALSE;
    }

    /* Is there anything other than a QS_MOUSEMOVE pending? */
    if ((qstatus & QS_MOUSEBUTTON) ||
        (qstatus & QS_KEY) 
#ifndef WINCE
          || (qstatus & QS_HOTKEY)
#endif 
        ) {
      return PR_TRUE;
    }

    /*
     * Mouse moves need extra processing to determine if the mouse
     * pointer actually changed location because Windows automatically
     * generates WM_MOVEMOVE events when a new window is created which
     * we need to filter out.
     */
    if (qstatus & QS_MOUSEMOVE) {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        if ((_md_LastMousePos.x == cursorPos.x) &&
            (_md_LastMousePos.y == cursorPos.y)) {
            return PR_FALSE; /* This is a fake mouse move */
        }

        /* Real mouse move */
        _md_LastMousePos.x = cursorPos.x;
        _md_LastMousePos.y = cursorPos.y;
        return PR_TRUE;
    }

    return PR_FALSE;
}

static PRStatus
_pl_NativeNotify(PLEventQueue* self)
{
#ifdef USE_TIMER
    WORD qstatus;

    PRUint32 now = PR_IntervalToMilliseconds(PR_IntervalNow());

    /* Since calls to set the _md_PerformanceSetting can be nested
     * only performance setting values <= 0 will potentially trigger
     * the use of a timer.
     */
    if ((_md_PerformanceSetting <= 0) &&
        ((now - _md_SwitchTime) > _md_StarvationDelay)) {
        SetTimer(self->eventReceiverWindow, TIMER_ID, 0 ,_md_TimerProc);
        self->timerSet = PR_TRUE;
        _md_WasInputPending = PR_FALSE;
        _md_WasPaintPending = PR_FALSE;
        return PR_SUCCESS;
    }

    qstatus = HIWORD(GetQueueStatus(QS_INPUT | QS_PAINT));

    /* Check for starved input */
    if (_md_EventIsStarved( _md_IsInputPending(qstatus),
                            INPUT_STARVATION_LIMIT,
                            &_md_WasInputPending,
                            &_md_InputTime,
                            now )) {
        /*
         * Use a timer for notification. Timers have the lowest priority.
         * They are not processed until all other events have been processed.
         * This allows any starved paints and input to be processed.
         */
        SetTimer(self->eventReceiverWindow, TIMER_ID, 0 ,_md_TimerProc);
        self->timerSet = PR_TRUE;

        /*
         * Clear any pending paint.  _md_WasInputPending was cleared in
         * _md_EventIsStarved.
         */
        _md_WasPaintPending = PR_FALSE;
        return PR_SUCCESS;
    }

    if (_md_EventIsStarved( (qstatus & QS_PAINT),
                            _md_GetPaintStarvationLimit(),
                            &_md_WasPaintPending,
                            &_md_PaintTime,
                            now) ) {
        /*
         * Use a timer for notification. Timers have the lowest priority.
         * They are not processed until all other events have been processed.
         * This allows any starved paints and input to be processed
         */
        SetTimer(self->eventReceiverWindow, TIMER_ID, 0 ,_md_TimerProc);
        self->timerSet = PR_TRUE;

        /*
         * Clear any pending input.  _md_WasPaintPending was cleared in
         * _md_EventIsStarved.
         */
        _md_WasInputPending = PR_FALSE;
        return PR_SUCCESS;
    }

    /*
     * Nothing is being starved so post a message instead of using a timer.
     * Posted messages are processed before other messages so they have the
     * highest priority.
     */
#endif
    PostMessage( self->eventReceiverWindow, _pr_PostEventMsgId,
                (WPARAM)0, (LPARAM)self );

    return PR_SUCCESS;
}/* --- end _pl_NativeNotify() --- */

static PRStatus
_pl_AcknowledgeNativeNotify(PLEventQueue* self)
{
    MSG aMsg;
    /*
     * only remove msg when we've been called directly by
     * PL_ProcessPendingEvents, not when we've been called by
     * the window proc because the window proc will remove the
     * msg for us.
     */
    if (self->removeMsg) {
        PR_LOG(event_lm, PR_LOG_DEBUG,
               ("_pl_AcknowledgeNativeNotify: self=%p", self));
        PeekMessage(&aMsg, self->eventReceiverWindow,
                    _pr_PostEventMsgId, _pr_PostEventMsgId, PM_REMOVE);
        if (self->timerSet) {
            KillTimer(self->eventReceiverWindow, TIMER_ID);
            self->timerSet = PR_FALSE;
        }
    }
    return PR_SUCCESS;
}

void
PL_FavorPerformanceHint(PRBool favorPerformanceOverEventStarvation,
                        PRUint32 starvationDelay)
{
    _md_StarvationDelay = starvationDelay;

    if (favorPerformanceOverEventStarvation) {
        _md_PerformanceSetting++;
        return;
    }

    _md_PerformanceSetting--;

    if (_md_PerformanceSetting == 0) {
      /* Switched from allowing event starvation to no event starvation so grab
         the current time to determine when to actually switch to using timers
         instead of posted WM_APP messages. */
      _md_SwitchTime = PR_IntervalToMilliseconds(PR_IntervalNow());
    }
}
#endif

/*static*/ void
nsAppShell::FavorPerformanceHint(PRBool favorPerformanceOverEventStarvation,
                                 PRUint32 starvationDelay)
{
}

//-------------------------------------------------------------------------
//
// Create the application shell
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsAppShell::Create(int* argc, char ** argv)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Enter a message handler loop
//
//-------------------------------------------------------------------------

#include "nsITimerManager.h"

static BOOL PeekKeyAndIMEMessage(LPMSG msg, HWND hwnd)
{
  MSG msg1, msg2, *lpMsg;
  BOOL b1, b2;
  b1 = nsToolkit::mPeekMessage(&msg1, NULL, WM_KEYFIRST, WM_IME_KEYLAST, PM_NOREMOVE);
  b2 = nsToolkit::mPeekMessage(&msg2, NULL, WM_IME_SETCONTEXT, WM_IME_KEYUP, PM_NOREMOVE);
  if (b1 || b2) {
    if (b1 && b2) {
      if (msg1.time < msg2.time)
        lpMsg = &msg1;
      else
        lpMsg = &msg2;
    } else if (b1)
      lpMsg = &msg1;
    else
      lpMsg = &msg2;
    return nsToolkit::mPeekMessage(msg, hwnd, lpMsg->message, lpMsg->message, PM_REMOVE);
  }

  return false;
}


NS_IMETHODIMP nsAppShell::Run(void)
{
  NS_ADDREF_THIS();
  MSG  msg;
  int  keepGoing = 1;

  nsresult rv;
  nsCOMPtr<nsITimerManager> timerManager =
      do_GetService("@mozilla.org/timer/manager;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  timerManager->SetUseIdleTimers(PR_TRUE);

  gKeepGoing = 1;
  // Process messages
  do {
    // Give priority to system messages (in particular keyboard, mouse,
    // timer, and paint messages).
     if (PeekKeyAndIMEMessage(&msg, NULL) ||
         nsToolkit::mPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
         nsToolkit::mPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      keepGoing = (msg.message != WM_QUIT);

      if (keepGoing != 0) {
        TranslateMessage(&msg);
        nsToolkit::mDispatchMessage(&msg);
      }
    } else {

      PRBool hasTimers;
      timerManager->HasIdleTimers(&hasTimers);
      if (hasTimers) {
        do {
          timerManager->FireNextIdleTimer();
          timerManager->HasIdleTimers(&hasTimers);
        } while (hasTimers && !nsToolkit::mPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
      } else {

        if (!gKeepGoing) {
          // In this situation, PostQuitMessage() was called, but the WM_QUIT
          // message was removed from the event queue by someone else -
          // (see bug #54725).  So, just exit the loop as if WM_QUIT had been
          // reeceived...
          keepGoing = 0;
        } else {
          // Block and wait for any posted application message
          ::WaitMessage();
        }
      }
    }

  } while (keepGoing != 0);
  Release();
  return msg.wParam;
}

NS_IMETHODIMP nsAppShell::Spinup(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::Spindown(void)
{
  return NS_OK;
}

#if 0
NS_IMETHODIMP nsAppShell::ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen)
{
  return NS_OK;
}

NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
  static MSG msg;

  BOOL gotMessage = false;

  nsresult rv;
  nsCOMPtr<nsITimerManager> timerManager(do_GetService("@mozilla.org/timer/manager;1", &rv));
  if (NS_FAILED(rv)) return rv;

  do {
    // Give priority to system messages (in particular keyboard, mouse,
    // timer, and paint messages).
     if (PeekKeyAndIMEMessage(&msg, NULL) ||
        nsToolkit::mPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        nsToolkit::mPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      gotMessage = true;
    } else {
      PRBool hasTimers;
      timerManager->HasIdleTimers(&hasTimers);
      if (hasTimers) {
        do {
          timerManager->FireNextIdleTimer();
          timerManager->HasIdleTimers(&hasTimers);
        } while (hasTimers && !nsToolkit::mPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
      } else {
        // Block and wait for any posted application message
        ::WaitMessage();
      }
    }

  } while (!gotMessage);

#ifdef DEBUG_danm
  if (msg.message != WM_TIMER)
    printf("-> %d", msg.message);
#endif

  TranslateMessage(&msg);
  aEvent = &msg;
  aRealEvent = PR_TRUE;
  return NS_OK;
}

nsresult nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
  nsToolkit::mDispatchMessage((MSG *)aEvent);
  return NS_OK;
}
#endif

//-------------------------------------------------------------------------
//
// Exit a message handler loop
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsAppShell::Exit(void)
{
  PostQuitMessage(0);
  //
  // Also, set a global flag, just in case someone eats the WM_QUIT message.
  // see bug #54725.
  //
  gKeepGoing = 0;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Thread observer methods
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsAppShell::OnNewTask(nsIThreadInternal *thr, PRUint32 flags)
{
  // post a message to the native event queue...
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnBeforeRunNextTask(nsIThreadInternal *thr,
                                              PRUint32 flags)
{
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnAfterRunNextTask(nsIThreadInternal *thr,
                                             PRUint32 flags,
                                             nsresult status)
{
  // process any pending native events...
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnWaitNextTask(nsIThreadInternal *thr,
                                         PRUint32 flags)
{
  // while (!thr->HasPendingTask())
  //   wait for and process native events
  return NS_OK;
}
