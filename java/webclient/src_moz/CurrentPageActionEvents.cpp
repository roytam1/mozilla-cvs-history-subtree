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
 *      Jason Mawdsley <jason@macadamian.com>
 *      Louis-Philippe Gagnon <louisphilippe@macadamian.com>
 */

/*
 * CurrentPageActionEvents.cpp
 */

#include "CurrentPageActionEvents.h"


wsCopySelectionEvent::wsCopySelectionEvent(nsIContentViewerEdit * contentViewerEdit) :
        nsActionEvent(),
        mContentViewerEdit(contentViewerEdit)
{
}

void *
wsCopySelectionEvent::handleEvent ()
{
    void *result = nsnull;
    
    if (mContentViewerEdit) {
        nsresult rv = mContentViewerEdit->CopySelection();
        result = (void *) rv;
    }
    return result;
}

wsFindEvent::wsFindEvent(nsIFindComponent * findcomponent, nsISearchContext * srchcontext) :
        nsActionEvent(),
        mFindComponent(findcomponent),
	mSearchContext(srchcontext)
{
}

void *
wsFindEvent::handleEvent ()
{
    void *result = nsnull;
    
    if (mFindComponent && mSearchContext) {
        PRBool found = PR_TRUE;
        nsresult rv = mFindComponent->FindNext(mSearchContext, &found);
        result = (void *) rv;
    }
    return result;
}

/*
 * wsGetURLEvent
 */

wsGetURLEvent::wsGetURLEvent(nsISHistory * sHistory) :
        nsActionEvent(),
	mHistory(sHistory)
{
}


void *
wsGetURLEvent::handleEvent ()
{
    void *result = nsnull;
    if (mHistory) {
        
        
        PRInt32 currentIndex;
        char *currentURL = nsnull;
        nsresult rv;

        rv = mHistory->GetIndex(&currentIndex);
        
        if (NS_FAILED(rv)) {
            return result;
        }
        
        nsISHEntry * Entry;
        rv = mHistory->GetEntryAtIndex(currentIndex, PR_FALSE, &Entry);

        if (NS_FAILED(rv)) {
            return result;
        }

	nsIURI * URI;
	rv = Entry->GetURI(&URI);

	if (NS_FAILED(rv)) {
            return result;
        }
	
	rv = URI->GetSpec(&currentURL);
	if (NS_FAILED(rv)) {
            return result;
        }
        
        result = (void *) currentURL;
    }
    return result;
} // handleEvent()


wsSelectAllEvent::wsSelectAllEvent(nsIContentViewerEdit * contentViewerEdit) :
        nsActionEvent(),
        mContentViewerEdit(contentViewerEdit)
{
}

void *
wsSelectAllEvent::handleEvent ()
{
    void *result = nsnull;
    
    if (mContentViewerEdit) {
        nsresult rv = mContentViewerEdit->SelectAll();
        result = (void *) rv;
    }
    return result;
}

wsViewSourceEvent::wsViewSourceEvent(nsIDocShell* docShell, PRBool viewMode) :
    nsActionEvent(),
    mDocShell(docShell),
    mViewMode(viewMode)
{
}

void *
wsViewSourceEvent::handleEvent ()
{
    if(mDocShell) {
        if(mViewMode) {
            nsresult rv = mDocShell->SetViewMode(nsIDocShell::viewSource);
            return (void *) rv;
        }
        else
            {
                nsresult rv = mDocShell->SetViewMode(nsIDocShell::viewNormal);
                return (void *) rv;
            }
    }
    return nsnull;
}

