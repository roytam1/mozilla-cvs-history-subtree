
/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

// This file exists for code that lost its home during the process of trimming
// down the WinFE for NGLayout.  Hopefully, as layout integration proceeds
// most of this code will become irrelevant or will find a better home elsewhere.

#include "stdafx.h"
#include "cxabstra.h"


extern "C" void FE_RaiseWindow(MWContext *pContext)    {
    //  Make sure this is possible.
    if(pContext && ABSTRACTCX(pContext) && !ABSTRACTCX(pContext)->IsDestroyed()
        && ABSTRACTCX(pContext)->IsFrameContext() && PANECX(pContext)->GetPane()
        && WINCX(pContext)->GetFrame() && WINCX(pContext)->GetFrame()->GetFrameWnd()) {
        CWinCX *pWinCX = WINCX(pContext);
        CFrameWnd *pFrameWnd = pWinCX->GetFrame()->GetFrameWnd();

        //  Bring the frame to the top first.
		if(pFrameWnd->IsIconic())	{
			pFrameWnd->ShowWindow(SW_RESTORE);
		}
        ::SetWindowPos(pFrameWnd->GetSafeHwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#ifdef XP_WIN32
        pFrameWnd->SetForegroundWindow();
#endif

        //  Now, set focus to the view being raised,
        //      possibly a frame cell.
        pWinCX->
            GetFrame()->
            GetFrameWnd()->
            SetActiveView(pWinCX->GetView(), TRUE);
    }
}

extern "C" void FE_Print(const char *pUrl)  {
  XP_ASSERT(0);
}
#if 0
extern "C" BOOL wfe_IsTypePlugin(NPEmbeddedApp* pEmbeddedApp) {
  ASSERT(0);
  return FALSE;
}
#endif


