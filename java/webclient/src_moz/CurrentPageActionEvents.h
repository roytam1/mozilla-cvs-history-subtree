/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 *               Mark Lin <mark.lin@eng.sun.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 */

/*
 * nsActions.h
 */
 
#ifndef CurrentPageActionEvents_h___
#define CurrentPageActionEvents_h___

#include "nsActions.h"

#include "nsIContentViewerEdit.h"
#include "nsIFindComponent.h"
#include "nsISearchContext.h"
#include "nsISHistory.h"


class wsCopySelectionEvent : public nsActionEvent {
public:
    wsCopySelectionEvent(nsIContentViewerEdit * contentViewerEdit);
    void * handleEvent (void);

protected:
    nsIContentViewerEdit * mContentViewerEdit;
};

class wsFindEvent : public nsActionEvent {
public:
    wsFindEvent(nsIFindComponent *findComponent, 
		  nsISearchContext * srchcontext);
    void * handleEvent (void);

protected:
    nsIFindComponent * mFindComponent;
    nsISearchContext * mSearchContext;
};

class wsGetURLEvent : public nsActionEvent {
public:
                       wsGetURLEvent   (nsISHistory * sHistory);
        void    *      handleEvent     (void);
protected:
    nsISHistory * mHistory;
};

class wsSelectAllEvent : public nsActionEvent {
public:
    wsSelectAllEvent(nsIContentViewerEdit * contentViewerEdit);
    void * handleEvent (void);

protected:
    nsIContentViewerEdit * mContentViewerEdit;
};

class wsViewSourceEvent : public nsActionEvent {
public:
    wsViewSourceEvent (nsIDocShell * docShell, PRBool viewMode);
    void * handleEvent (void);

protected:
    nsIDocShell * mDocShell;
    PRBool mViewMode;
};

#endif /* CurrentPageActionEvents_h___ */

      
// EOF
