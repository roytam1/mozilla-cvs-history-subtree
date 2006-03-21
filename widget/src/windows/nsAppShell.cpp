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

//-------------------------------------------------------------------------

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

//-------------------------------------------------------------------------
// nsIAppShell methods:

NS_IMETHODIMP
nsAppShell::Init(int *argc, char **argv)
{
  mMainThreadId = GetCurrentThreadId();
  mMsgId = RegisterWindowMessage("nsAppShell_Dispatch");

  return nsBaseAppShell::Init(argc, argv);
}

//-------------------------------------------------------------------------
// nsIThreadObserver methods:

NS_IMETHODIMP
nsAppShell::OnDispatchedEvent(nsIThreadInternal *thr)
{
  // post a message to the native event queue...
  PostThreadMessage(mMainThreadId, mMsgId, 0, 0);
  return NS_OK;
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  PRBool gotMessage = PR_FALSE;

  do {
    MSG msg;
    // Give priority to system messages (in particular keyboard, mouse, timer,
    // and paint messages).
    if (PeekKeyAndIMEMessage(&msg, NULL) ||
        nsToolkit::mPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        nsToolkit::mPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      gotMessage = PR_TRUE;
      TranslateMessage(&msg);
      nsToolkit::mDispatchMessage(&msg);
    } else if (mayWait) {
      // Block and wait for any posted application message
      ::WaitMessage();
    }
  } while (!gotMessage && mayWait);

  return gotMessage;
}
