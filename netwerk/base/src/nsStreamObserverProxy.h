/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsStreamObserverProxy_h__
#define nsStreamObserverProxy_h__

#include "nsIStreamObserver.h"
#include "nsIEventQueue.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nslog.h"

NS_DECL_LOG(nsStreamProxyLog)

class nsStreamObserverProxy : public nsIStreamObserverProxy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMOBSERVERPROXY

    nsStreamObserverProxy()
        { NS_INIT_ISUPPORTS(); }
    virtual ~nsStreamObserverProxy() {}

    nsIEventQueue *GetEventQueue() { return mEventQueue.get(); }
    nsIStreamObserver *GetReceiver() { return mReceiver.get(); }

    void ClearReceiver() { mReceiver = nsnull; }

protected:
    nsCOMPtr<nsIEventQueue>     mEventQueue;
    nsCOMPtr<nsIStreamObserver> mReceiver;
};

class nsStreamObserverEvent
{
public:
    nsStreamObserverEvent(nsIStreamObserverProxy *proxy,
                          nsIChannel *channel, nsISupports *context);
    virtual ~nsStreamObserverEvent() {}

    nsresult FireEvent(nsIEventQueue *);
    NS_IMETHOD HandleEvent() = 0;

protected:
    static void PR_CALLBACK HandlePLEvent(PLEvent *);
    static void PR_CALLBACK DestroyPLEvent(PLEvent *);

    PLEvent                          mEvent;
    nsCOMPtr<nsIStreamObserverProxy> mProxy;
    nsCOMPtr<nsIChannel>             mChannel;
    nsCOMPtr<nsISupports>            mContext;
};

#define GET_STREAM_OBSERVER_EVENT(_mEvent_ptr) \
    ((nsStreamObserverEvent *) \
     ((char *)(_mEvent_ptr) - offsetof(nsStreamObserverEvent, mEvent)))

#endif /* !nsStreamObserverProxy_h__ */
