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
nsStreamObserverEvent::nsStreamObserverEvent(nsStreamObserverProxyBase *aProxy,
                                             nsIChannel *aChannel,
                                             nsISupports *aContext)
    : mProxy(aProxy)
    , mChannel(aChannel)
    , mContext(aContext)
{
    NS_IF_ADDREF(mProxy);
} 

nsStreamObserverEvent::~nsStreamObserverEvent()
{
    NS_IF_RELEASE(mProxy);
}

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
    nsOnStartRequestEvent(nsStreamObserverProxyBase *aProxy,
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

    nsIStreamObserver *observer = mProxy->GetReceiver();
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
    nsOnStopRequestEvent(nsStreamObserverProxyBase *aProxy,
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

    nsCOMPtr<nsIStreamObserver> observer = mProxy->GetReceiver();
    if (!observer) {
        PRINTF("Already called OnStopRequest (observer is NULL)\n");
        return NS_ERROR_FAILURE;
    }

    //
    // Do not allow any more events to be handled after OnStopRequest
    //
    mProxy->SetReceiver(nsnull);

    return observer->OnStopRequest(mChannel,
                                   mContext,
                                   mStatus,
                                   mStatusText.GetUnicode());
}

//
//----------------------------------------------------------------------------
// nsStreamObserverProxyBase: nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_THREADSAFE_ISUPPORTS1(nsStreamObserverProxyBase,
                              nsIStreamObserver)

//
//----------------------------------------------------------------------------
// nsStreamObserverProxyBase: nsIStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP 
nsStreamObserverProxyBase::OnStartRequest(nsIChannel *aChannel,
                                          nsISupports *aContext)
{
    PRINTF("nsStreamObserverProxyBase::OnStartRequest\n");
    nsOnStartRequestEvent *ev = 
            new nsOnStartRequestEvent(this, aChannel, aContext);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = ev->FireEvent(GetEventQueue());
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}

NS_IMETHODIMP 
nsStreamObserverProxyBase::OnStopRequest(nsIChannel *aChannel,
                                 nsISupports *aContext,
                                 nsresult aStatus,
                                 const PRUnichar *aStatusText)
{
    PRINTF("nsStreamObserverProxyBase::OnStopRequest [status=%x]\n", aStatus);
    nsOnStopRequestEvent *ev = 
            new nsOnStopRequestEvent(this, aChannel, aContext, aStatus, aStatusText);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = ev->FireEvent(GetEventQueue());
    if (NS_FAILED(rv))
        delete ev;
    return rv;
}

//
//----------------------------------------------------------------------------
// nsStreamObserverProxyBase: implementation...
//----------------------------------------------------------------------------
//
nsresult
nsStreamObserverProxyBase::SetEventQueue(nsIEventQueue *aEventQ)
{
    nsresult rv = NS_OK;
    if ((aEventQ == NS_CURRENT_EVENTQ) || (aEventQ == NS_UI_THREAD_EVENTQ)) {
        nsCOMPtr<nsIEventQueueService> serv =
                do_GetService(kEventQueueService, &rv);
        if (NS_FAILED(rv)) 
            return rv;
        rv = serv->GetSpecialEventQueue((PRInt32) aEventQ,
                getter_AddRefs(mEventQ));
    } else
        mEventQ = aEventQ;
    return rv;
}

//
//----------------------------------------------------------------------------
// nsStreamObserverProxy: nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_ISUPPORTS_INHERITED1(nsStreamObserverProxy,
                             nsStreamObserverProxyBase,
                             nsIStreamObserverProxy)

//
//----------------------------------------------------------------------------
// nsStreamObserverProxy: nsIStreamObserverProxy implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamObserverProxy::Init(nsIStreamObserver *aObserver,
                            nsIEventQueue *aEventQ)
{
    NS_PRECONDITION(aObserver, "null observer");
    SetReceiver(aObserver);
    return SetEventQueue(aEventQ);
}
