/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "msgCore.h" // for pre-compiled headers
#include "nsCOMPtr.h"
#include "stdio.h"
#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "comi18n.h"
#include "prmem.h"
#include "plstr.h"
#include "nsRepository.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsURLFetcher.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsIIOService.h"
#include "nsIChannel.h"

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

/* 
 * This function will be used by the factory to generate an 
 * mime object class object....
 */
nsresult NS_NewURLFetcher(nsURLFetcher ** aInstancePtrResult)
{
	//nsresult result = NS_OK;
	NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
	if (nsnull != aInstancePtrResult)
	{
		nsURLFetcher *obj = new nsURLFetcher();
		if (obj)
			return obj->QueryInterface(nsIStreamListener::GetIID(), (void**) aInstancePtrResult);
		else
			return NS_ERROR_OUT_OF_MEMORY; // we couldn't allocate the object 
	}
	else
		return NS_ERROR_NULL_POINTER; // aInstancePtrResult was NULL....
}

// The following macros actually implement addref, release and 
// query interface for our component. 
NS_IMPL_ISUPPORTS(nsURLFetcher, nsIStreamListener::GetIID());

/* 
 * Inherited methods for nsMimeConverter
 */
nsURLFetcher::nsURLFetcher()
{
  /* the following macro is used to initialize the ref counting data */
  NS_INIT_REFCNT();

  // Init member variables...
  mOutStream = nsnull;
  mTotalWritten = 0;
  mStillRunning = PR_TRUE;
  mCallback = nsnull;
}

nsURLFetcher::~nsURLFetcher()
{
  mStillRunning = PR_FALSE;
}

nsresult
nsURLFetcher::StillRunning(PRBool *running)
{
  *running = mStillRunning;
  return NS_OK;
}


// Methods for nsIStreamListener...
nsresult
nsURLFetcher::OnDataAvailable(nsISupports * ctxt, nsIInputStream *aIStream, 
                              PRUint32 sourceOffset, PRUint32 aLength)
{
  PRUint32        readLen = aLength;
  PRUint32        wroteIt;

  if (!mOutStream)
    return NS_ERROR_FAILURE;

  char *buf = (char *)PR_Malloc(aLength);
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */

  // read the data from the input stram...
  aIStream->Read(buf, aLength, &readLen);

  // write to the output file...
  wroteIt = mOutStream->write(buf, readLen);
  PR_FREEIF(buf);

  if (wroteIt != readLen)
    return NS_ERROR_FAILURE;
  else
  {
    mTotalWritten += wroteIt;
    return NS_OK;
  }
}


// Methods for nsIStreamObserver 
nsresult
nsURLFetcher::OnStartBinding(nsISupports * /* ctxt */)
{
  return NS_OK;
}

/**
* Notify the observer that the URL has finished loading.  This method is 
* called once when the networking library has finished processing the 
* URL transaction initiatied via the nsINetService::Open(...) call.<BR><BR>
* 
* This method is called regardless of whether the URL loaded successfully.<BR><BR>
* 
* @param status    Status code for the URL load.
* @param msg   A text string describing the error.
* @return The return value is currently ignored.
*/
nsresult
nsURLFetcher::OnStopBinding(nsISupports * /* ctxt */, nsresult aStatus, const PRUnichar* aMsg)
{
#ifdef NS_DEBUG_richie
  printf("nsURLFetcher::OnStopBinding()\n");
#endif

  //
  // Now complete the stream!
  //
  mStillRunning = PR_FALSE;

  // First close the output stream...
  if (mOutStream)
    mOutStream->close();

  // Now if there is a callback, we need to call it...
  if (mCallback)
    mCallback (mURL, aStatus, "", mTotalWritten, aMsg, mTagData);

  // Time to return...
  return NS_OK;
}

nsresult
nsURLFetcher::FireURLRequest(nsIURI *aURL, nsOutputFileStream *fOut, 
                             nsAttachSaveCompletionCallback cb, void *tagData)
{
  nsresult rv;

  if ( (!aURL) || (!fOut) )
  {
    return NS_ERROR_FAILURE;
  }

  if (!fOut->is_open())
  {
    return NS_ERROR_FAILURE;
  }

  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIChannel> channel;
  rv = service->NewChannelFromURI("load", aURL, nsnull, getter_AddRefs(channel));
  if (NS_FAILED(rv)) return rv;

  // Create the Event Queue for this thread...
  NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIEventQueue> eventQ;
  rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), getter_AddRefs(eventQ));
  if (NS_FAILED(rv)) return rv;

  rv = channel->AsyncRead(0, -1, nsnull, eventQ, this);
  if (NS_FAILED(rv)) return rv;

  mURL = dont_QueryInterface(aURL);
  mOutStream = fOut;
  mCallback = cb;
  mTagData = tagData;
  NS_ADDREF(this);
  return NS_OK;
}
