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

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "nsIFactory.h"
#include "nsIStringStream.h"
#include "nsIIOService.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#include "nspr.h"


#define ASYNC_TEST // undefine this if you want to test sycnronous conversion.

/////////////////////////////////
// Event pump setup
/////////////////////////////////
#include "nsIEventQueueService.h"
#ifdef WIN32 
#include <windows.h>
#endif
#ifdef XP_OS2
#include <os2.h>
#endif

static int gKeepRunning = 0;
static nsIEventQueue* gEventQ = nsnull;
/////////////////////////////////
// Event pump END
/////////////////////////////////


/////////////////////////////////
// Test converters include
/////////////////////////////////
#include "Converters.h"

// CID setup
static NS_DEFINE_CID(kEventQueueServiceCID,      NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kComponentManagerCID,       NS_COMPONENTMANAGER_CID);

////////////////////////////////////////////////////////////////////////
// EndListener - This listener is the final one in the chain. It
//   receives the fully converted data, although it doesn't do anything with
//   the data.
////////////////////////////////////////////////////////////////////////
class EndListener : public nsIStreamListener {
public:
    // nsISupports declaration
    NS_DECL_ISUPPORTS

    EndListener() {NS_INIT_ISUPPORTS();};

    // nsIStreamListener method
    NS_IMETHOD OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *inStr, 
                               PRUint32 sourceOffset, PRUint32 count)
    {
        nsresult rv;
        PRUint32 read, len;
        rv = inStr->Available(&len);
        if (NS_FAILED(rv)) return rv;

        char *buffer = (char*)nsMemory::Alloc(len + 1);
        if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

        rv = inStr->Read(buffer, len, &read);
        buffer[len] = '\0';
        if (NS_SUCCEEDED(rv)) {
            printf("CONTEXT %p: Received %u bytes and the following data: \n %s\n\n", ctxt, read, buffer);
        }
        nsMemory::Free(buffer);

        return NS_OK;
    }

    // nsIStreamObserver methods
    NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt) 
    {
        return NS_OK;
    }

    NS_IMETHOD OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus, const PRUnichar* aStatusArg)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(EndListener, nsIStreamListener);
////////////////////////////////////////////////////////////////////////
// EndListener END
////////////////////////////////////////////////////////////////////////


nsresult SendData(const char * aData, nsIStreamListener* aListener, nsIRequest* request) {
    nsString data;
    data.AssignWithConversion(aData);
    nsCOMPtr<nsIInputStream> dataStream;
    nsCOMPtr<nsISupports> sup;
    nsresult rv = NS_NewStringInputStream(getter_AddRefs(sup), data);
    if (NS_FAILED(rv)) return rv;
    dataStream = do_QueryInterface(sup, &rv);
    if (NS_FAILED(rv)) return rv;
    return aListener->OnDataAvailable(request, nsnull, dataStream, 0, -1);
}
#define SEND_DATA(x) SendData(x, converterListener, nsnull)

int
main(int argc, char* argv[])
{
    nsresult rv;

    // Create the Event Queue for this thread...
    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) return rv;

    eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, &gEventQ);

    rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL /* default */);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString previous;



    ///////////////////////////////////////////
    // BEGIN - Stream converter registration
    //   All stream converters must register with the ComponentManager, _and_ make
    //   a registry entry.
    ///////////////////////////////////////////
    TestConverterFactory *convFactory = new TestConverterFactory(kTestConverterCID, "TestConverter", NS_ISTREAMCONVERTER_KEY);
    nsIFactory *convFactSup = nsnull;
    rv = convFactory->QueryInterface(NS_GET_IID(nsIFactory), (void**)&convFactSup);
    if (NS_FAILED(rv)) return rv;

    TestConverterFactory *convFactory1 = new TestConverterFactory(kTestConverter1CID, "TestConverter1", NS_ISTREAMCONVERTER_KEY);
    nsIFactory *convFactSup1 = nsnull;
    rv = convFactory1->QueryInterface(NS_GET_IID(nsIFactory), (void**)&convFactSup1);
    if (NS_FAILED(rv)) return rv;

    // register the TestConverter with the component manager. One contractid registration
    // per conversion pair (from - to pair).
    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=a/foo&to=b/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=a/foo&to=b/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::RegisterFactory(kTestConverter1CID,
                                             "TestConverter1",
                                             NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=c/foo",
                                             convFactSup1,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=c/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=d/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=d/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=c/foo&to=d/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=c/foo&to=d/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;
    
    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=e/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=e/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=f/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=f/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;

    rv = nsComponentManager::RegisterFactory(kTestConverterCID,
                                             "TestConverter",
                                             NS_ISTREAMCONVERTER_KEY "?from=t/foo&to=k/foo",
                                             convFactSup,
                                             PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, "?from=t/foo&to=k/foo", "x",
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
    if (NS_FAILED(rv)) return rv;
    

    NS_WITH_SERVICE(nsIStreamConverterService, StreamConvService, kStreamConverterServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsString fromStr;
    fromStr.AssignWithConversion("a/foo");
    nsString toStr;
    toStr.AssignWithConversion("c/foo");
    

#ifdef ASYNC_TEST
    // ASYNCRONOUS conversion

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // we need a dummy channel for the async calls.
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIURI> dummyURI;
    rv = serv->NewURI("http://aaa", nsnull, getter_AddRefs(dummyURI));
    if (NS_FAILED(rv)) return rv;
    rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                  dummyURI,
                                  nsnull,   // inStr
                                  "multipart/x-mixed-replacE;boundary=thisrandomstring",
                                  -1);      // XXX fix contentLength
    if (NS_FAILED(rv)) return rv;


    // setup a listener to receive the converted data. This guy is the end
    // listener in the chain, he wants the fully converted (toType) data.
    // An example of this listener in mozilla would be the DocLoader.
    nsCOMPtr<nsIStreamListener> dataReceiver = new EndListener();

    // setup a listener to push the data into. This listener sits inbetween the
    // unconverted data of fromType, and the final listener in the chain (in this case
    // the dataReceiver.
    nsCOMPtr<nsIStreamListener> converterListener;
    rv = StreamConvService->AsyncConvertData(fromStr.GetUnicode(), toStr.GetUnicode(), dataReceiver, nsnull, getter_AddRefs(converterListener));
    if (NS_FAILED(rv)) return rv;

    // at this point we have a stream listener to push data to, and the one
    // that will receive the converted data. Let's mimic On*() calls and get the conversion
    // going. Typically these On*() calls would be made inside their respective wrappers On*()
    // methods.
    rv = converterListener->OnStartRequest(nsnull, nsnull);
    if (NS_FAILED(rv)) return rv;


    rv = SEND_DATA("aaa");
    if (NS_FAILED(rv)) return rv;
    
    rv = SEND_DATA("aaa");
    if (NS_FAILED(rv)) return rv;    

    // Finish the request.
    rv = converterListener->OnStopRequest(nsnull, nsnull, rv, nsnull);
    if (NS_FAILED(rv)) return rv;


#else
    // SYNCRONOUS conversion
    nsIInputStream *convertedData = nsnull;
    rv = StreamConvService->Convert(inputData, fromStr.GetUnicode(), toStr.GetUnicode(), nsnull, &convertedData);
#endif

    NS_RELEASE(convFactSup);
    if (NS_FAILED(rv)) return rv;

    // Enter the message pump to allow the URL load to proceed.
    while ( gKeepRunning ) {
#ifdef WIN32
        MSG msg;

        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            gKeepRunning = 0;
    }
#else
#ifdef XP_MAC
    /* Mac stuff is missing here! */
#else
#ifdef XP_OS2
    QMSG qmsg;

    if (WinGetMsg(0, &qmsg, 0, 0, 0))
      WinDispatchMsg(0, &qmsg);
    else
      gKeepRunning = FALSE;
#else
    PLEvent *gEvent;
    rv = gEventQ->GetEvent(&gEvent);
    rv = gEventQ->HandleEvent(gEvent);
#endif /* XP_UNIX */
#endif /* XP_OS2 */
#endif /* !WIN32 */
    }

    return NS_ShutdownXPCOM(NULL);
}
