/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "msgCore.h"
#include "nsMsgProtocol.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsISocketTransportService.h"
#include "nsIFileTransportService.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

NS_IMPL_ISUPPORTS(nsMsgProtocol, nsIStreamListener::GetIID())

nsMsgProtocol::nsMsgProtocol()
{
	NS_INIT_REFCNT();
	m_flags = 0;
	m_socketIsOpen = PR_FALSE;
}

nsMsgProtocol::~nsMsgProtocol()
{}

nsresult nsMsgProtocol::OpenNetworkSocket(nsIURI * aURL) // open a connection on this url
{
	nsresult rv = NS_OK;
	char * hostName = nsnull;
	PRInt32 port = 0;

    NS_WITH_SERVICE(nsISocketTransportService, socketService, kSocketTransportServiceCID, &rv);

	if (NS_SUCCEEDED(rv) && aURL)
	{
		aURL->GetPort(&port);
		aURL->GetHost(&hostName);

		rv = socketService->CreateTransport(hostName, port, getter_AddRefs(m_channel));
		nsCRT::free(hostName);
		if (NS_SUCCEEDED(rv) && m_channel)
		{
			m_socketIsOpen = PR_FALSE;
			rv = SetupTransportState();
		}
	}

	return rv;
}

nsresult nsMsgProtocol::OpenFileSocket(nsIURI * aURL, const nsFileSpec * aFileSpec)
{
	// mscott - file needs to be encoded directly into aURL. I should be able to get
	// rid of this method completely.

	nsresult rv = NS_OK;
    NS_WITH_SERVICE(nsIFileTransportService, fileService, kFileTransportServiceCID, &rv);

	if (NS_SUCCEEDED(rv) && aURL)
	{

		rv = fileService->CreateTransport((const char *) *aFileSpec, getter_AddRefs(m_channel));

		if (NS_SUCCEEDED(rv) && m_channel)
		{
			m_socketIsOpen = PR_FALSE;
			rv = SetupTransportState();
		}
	}

	return rv;
}

nsresult nsMsgProtocol::SetupTransportState()
{
	nsresult rv = NS_OK;

	if (!m_socketIsOpen && m_channel)
	{
		rv = m_channel->OpenOutputStream(0 /* start position */, getter_AddRefs(m_outputStream));
		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create an output stream");
		// we want to open the stream 
	} // if m_transport

	return rv;
}

nsresult nsMsgProtocol::CloseSocket()
{
	// release all of our socket state
	m_socketIsOpen = PR_FALSE;
	m_outputStream = null_nsCOMPtr();
	m_channel = null_nsCOMPtr();

	return NS_OK;
}

/*
 * Writes the data contained in dataBuffer into the current output stream. It also informs
 * the transport layer that this data is now available for transmission.
 * Returns a positive number for success, 0 for failure (not all the bytes were written to the
 * stream, etc). We need to make another pass through this file to install an error system (mscott)
 */

PRInt32 nsMsgProtocol::SendData(nsIURI * aURL, const char * dataBuffer)
{
	PRUint32 writeCount = 0; 
	PRInt32 status = 0; 

	NS_PRECONDITION(m_outputStream, "oops....we don't have an output stream...how did that happen?");
	if (dataBuffer && m_outputStream)
	{
		return m_outputStream->Write(dataBuffer, PL_strlen(dataBuffer), &writeCount);
		
	}

	return status;
}

// Whenever data arrives from the connection, core netlib notifices the protocol by calling
// OnDataAvailable. We then read and process the incoming data from the input stream. 
NS_IMETHODIMP nsMsgProtocol::OnDataAvailable(nsISupports *ctxt, nsIBufferInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
	// right now, this really just means turn around and churn through the state machine
	nsCOMPtr<nsIURI> uri = do_QueryInterface(ctxt);
	return ProcessProtocolState(uri, inStr, sourceOffset, count);
}

NS_IMETHODIMP nsMsgProtocol::OnStartBinding(nsISupports *ctxt)
{
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(ctxt, &rv);
	if (NS_SUCCEEDED(rv) && aMsgUrl)
		return aMsgUrl->SetUrlState(PR_TRUE, NS_OK);
	else
		return NS_ERROR_NO_INTERFACE;
}

// stop binding is a "notification" informing us that the stream associated with aURL is going away. 
NS_IMETHODIMP nsMsgProtocol::OnStopBinding(nsISupports *ctxt, nsresult aStatus, const PRUnichar* aMsg)
{
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(ctxt, &rv);
	if (NS_SUCCEEDED(rv) && aMsgUrl)
		return aMsgUrl->SetUrlState(PR_FALSE, aStatus);
	else
		return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP nsMsgProtocol::OnStartRequest(nsISupports *ctxt)
{
	return OnStartBinding(ctxt);
}

NS_IMETHODIMP nsMsgProtocol::OnStopRequest(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg)
{
	return OnStopBinding(ctxt, status, errorMsg);
}

nsresult nsMsgProtocol::LoadUrl(nsIURI * aURL, nsISupports * /* aConsumer */)
{
	// okay now kick us off to the next state...
	// our first state is a process state so drive the state machine...
	nsresult rv = NS_OK;
	nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(aURL);

	if (NS_SUCCEEDED(rv))
	{
		rv = aMsgUrl->SetUrlState(PR_TRUE, NS_OK); // set the url as a url currently being run...
		if (!m_socketIsOpen)
		{

			// in order to run a url, we need to know our current event queue...
			// Create the Event Queue for this thread...
			NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv);
			if (NS_SUCCEEDED(rv))
			{
				nsCOMPtr<nsIEventQueue> queue;
				rv = pEventQService->GetThreadEventQueue(PR_GetCurrentThread(),getter_AddRefs(queue));
				if (NS_SUCCEEDED(rv))
				{
					// put us in a state where we are always notified of incoming data
					m_channel->AsyncRead(0, -1, aURL, queue, this /* stream observer */ );
					m_socketIsOpen = PR_TRUE; // mark the channel as open
				}
			} // if we got an event queue service
		}
		else  // the connection is already open so we should begin processing our new url...
			rv = ProcessProtocolState(aURL, nsnull, 0, 0); 
	}

	return rv;
}

