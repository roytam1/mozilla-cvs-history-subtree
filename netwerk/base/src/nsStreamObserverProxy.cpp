/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsStreamObserverProxy.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsString.h"
#include "nslog.h"

NS_IMPL_LOG(nsStreamProxyLog)
#define PRINTF NS_LOG_PRINTF(nsStreamProxyLog)

static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);

#define GET_OBSERVER_PROXY(p) \
    ((nsStreamObserverProxy *) (nsIStreamObserverProxy *) p)

//
//----------------------------------------------------------------------------
// nsStreamObserverEvent implementation...
//----------------------------------------------------------------------------
//
nsStreamObserverEvent::nsStreamObserverEvent(nsIStreamObserverProxy *aProxy,
                                             nsIChannel *aChannel,
                                             nsISupports *aContext)
    : mProxy(aProxy)
    , mChannel(aChannel)
    , mContext(aContext)
{} 

nsresult
nsStreamObserverEvent::FireEvent(nsIEventQueue *aEventQ)
{
    NS_PRECONDITION(aEventQ, "null event queue");

    PL_InitEvent(&mEvent, nsnull,
        (PLHandleEventProc) nsStreamObserverEvent::HandlePLEvent,
        (PLDestroyEventProc) nsStreamObserverEvent::DestroyPLEvent);

    PRStatus status = aEventQ->PostEvent(&mEvent);
    return status == PR_SUCCESS ? NS_OK : NS_ERROR_FAILURE;
}

void PR_CALLBACK
nsStreamObserverEvent::HandlePLEvent(PLEvent *aEvent)
{
    nsStreamObserverEvent *ev = GET_STREAM_OBSERVER_EVENT(aEvent);
    NS_ASSERTION(ev, "null event");

    // Pass control the real event handler
    ev->HandleEvent();
}

void PR_CALLBACK
nsStreamObserverEvent::DestroyPLEvent(PLEvent *aEvent)
{
    nsStreamObserverEvent *ev = GET_STREAM_OBSERVER_EVENT(aEvent);
    NS_ASSERTION(ev, "null event");
    delete ev;
}

//
//----------------------------------------------------------------------------
// nsOnStartRequestEvent internal class...
//----------------------------------------------------------------------------
//
class nsOnStartRequestEvent : public nsStreamObserverEvent
{
public:
    nsOnStartRequestEvent(nsIStreamObserverProxy *aProxy,
                          nsIChannel *aChannel,
                          nsISupports *aContext)
        : nsStreamObserverEvent(aProxy, aChannel, aContext)
    {
        MOZ_COUNT_CTOR(nsOnStartRequestEvent);
    }

   ~nsOnStartRequestEvent()
    {
        MOZ_COUNT_DTOR(nsOnStartRequestEvent);
    }

    NS_IMETHOD HandleEvent();
};

NS_IMETHODIMP
nsOnStartRequestEvent::HandleEvent()
{
    PRINTF("HandleEvent -- OnStartRequest [event=%x]\n", this);

    nsIStreamObserver *observer = GET_OBSERVER_PROXY(mProxy)->GetReceiver();
    if (!observer) {
        PRINTF("Already called OnStopRequest (observer is NULL)\n");
        return NS_ERROR_FAILURE;
    }

    return observer->OnStartRequest(mChannel, mContext);
}

//
//----------------------------------------------------------------------------
// nsOnStopRequestEvent internal class...
//----------------------------------------------------------------------------
//
class nsOnStopRequestEvent : public nsStreamObserverEvent
{
public:
    nsOnStopRequestEvent(nsIStreamObserverProxy *aProxy,
                         nsIChannel *aChannel, nsISupports *aContext,
                         nsresult aStatus, const PRUnichar *aStatusText)
        : nsStreamObserverEvent(aProxy, aChannel, aContext)
        , mStatus(aStatus)
        , mStatusText(aStatusText)
    {
        MOZ_COUNT_CTOR(nsOnStopRequestEvent);
    }

   ~nsOnStopRequestEvent()
    {
        MOZ_COUNT_DTOR(nsOnStopRequestEvent);
    }
    
    NS_IMETHOD HandleEvent();

protected:
    nsresult mStatus;
    nsString mStatusText;
};

NS_IMETHODIMP
nsOnStopRequestEvent::HandleEvent()
{
    PRINTF("HandleEvent -- OnStopRequest [event=%x]\n", this);

    nsStreamObserverProxy *observerProxy = GET_OBSERVER_PROXY(mProxy);

    nsCOMPtr<nsIStreamObserver> observer = observerProxy->GetReceiver();
    if (!observer) {
        PRINTF("Already called OnStopRequest (observer is NULL)\n");
        return NS_ERROR_FAILURE;
    }

    observerProxy->ClearReceiver();
    return observer->OnStopRequest(mChannel, mContext, mStatus, mStatusText.GetUnicode());
}

//
//----------------------------------------------------------------------------
// nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_THREADSAFE_ISUPPORTS2(nsStreamObserverProxy,
                              nsIStreamObserverProxy,
                              nsIStreamObserver)

//
//----------------------------------------------------------------------------
// nsIStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP 
nsStreamObserverProxy::OnStartRequest(nsIChannel *aChannel,
                                      nsISupports *aContext)
{
    PRINTF("nsStreamObserverProxy::OnStartRequest\n");
    nsOnStartRequestEvent *ev = 
            new nsOnStartRequestEvent(this, aChannel, aContext);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = ev->FireEvent(mEventQueue);
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}

NS_IMETHODIMP 
nsStreamObserverProxy::OnStopRequest(nsIChannel *aChannel,
                                     nsISupports *aContext,
                                     nsresult aStatus,
                                     const PRUnichar *aStatusText)
{
    PRINTF("nsStreamObserverProxy::OnStopRequest [status=%x]\n", aStatus);
    nsOnStopRequestEvent *ev = 
            new nsOnStopRequestEvent(this, aChannel, aContext, aStatus, aStatusText);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = ev->FireEvent(mEventQueue);
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}

//
//----------------------------------------------------------------------------
// nsIStreamObserverProxy implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamObserverProxy::Init(nsIStreamObserver *aObserver,
                            nsIEventQueue *aEventQ)
{
    nsresult rv = NS_OK;
    NS_PRECONDITION(aObserver, "null observer");

    // Realize event queue
    if ((aEventQ == NS_CURRENT_EVENTQ) || (aEventQ == NS_UI_THREAD_EVENTQ)) {
        nsCOMPtr<nsIEventQueueService> serv =
                do_GetService(kEventQueueService, &rv);
        if (NS_FAILED(rv)) 
            return rv;
        rv = serv->GetSpecialEventQueue((PRInt32) aEventQ,
                getter_AddRefs(mEventQueue));
    } else
        mEventQueue = aEventQ;

    mReceiver = 0;
    mReceiver = aObserver;
    NS_POSTCONDITION(mReceiver, "null receiver");
    return rv;
}
