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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

#include "zapUDPSocket.h"
#include "nsIComponentManager.h"
#include "nsHashPropertyBag.h"
#include "nsString.h"
#include "zapDatagramFrame.h"
#include "nsNetCID.h"

////////////////////////////////////////////////////////////////////////
// zapUDPSocket

zapUDPSocket::zapUDPSocket()
    : mSinkWaiting(PR_FALSE),
      mWaitingOnSource(PR_FALSE)
{
#ifdef DEBUG
  printf("zapUDPSocket::zapUDPSocket()\n");
#endif
}

zapUDPSocket::~zapUDPSocket()
{
  NS_ASSERTION(!mSocket, "unclean shutdown");
#ifdef DEBUG
  printf("zapUDPSocket::~zapUDPSocket()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

// these need to be threadsafe, as the socket thread will access them:
NS_IMPL_THREADSAFE_ADDREF(zapUDPSocket)
NS_IMPL_THREADSAFE_RELEASE(zapUDPSocket)

NS_INTERFACE_MAP_BEGIN(zapUDPSocket)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(nsIUDPReceiver)
  NS_INTERFACE_MAP_ENTRY(zapIUDPSocket)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapUDPSocket::AddedToGraph(zapIMediaGraph *graph,
                         const nsACString & id,
                         nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  if (!node_pars) {
    NS_ERROR("no node parameters");
    return NS_ERROR_FAILURE;
  }
  
  if (NS_FAILED(node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("socket"),
                                                     NS_GET_IID(nsIUDPSocket),
                                                     getter_AddRefs(mSocket)))
      || !mSocket) {
    PRUint16 port = 0;
    node_pars->GetPropertyAsUint16(NS_LITERAL_STRING("port"), &port);
    mSocket = do_CreateInstance(NS_UDPSOCKET_CONTRACTID);
    if (!mSocket) {
      NS_ERROR("failure creating socket");
      return NS_ERROR_FAILURE;
    }
    
    if (NS_FAILED(mSocket->Init(port))) {
      NS_ERROR("port initialization failed");
      mSocket = nsnull;
      return NS_ERROR_FAILURE;
    }
  }

  mSocket->SetReceiver(this);

  // create a new streaminfo:
  nsCOMPtr<nsIWritablePropertyBag> bag;
  NS_NewHashPropertyBag(getter_AddRefs(bag));
  mStreamInfo = do_QueryInterface(bag);

  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("type"),
                                     NS_LITERAL_CSTRING("datagram"));
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapUDPSocket::RemovedFromGraph(zapIMediaGraph *graph)
{
  if (mSocket) {
    mSocket->Close();
    mSocket = nsnull;
  }
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapUDPSocket::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mSink) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapUDPSocket::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (mSource) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}


//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapUDPSocket::ConnectSink(zapIMediaSink *sink,
                        const nsACString & connection_id)
{
  NS_ASSERTION(!mSink, "already connected");
  mSink = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapUDPSocket::DisconnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  mSink = nsnull;
  mSinkWaiting = PR_FALSE;
  return NS_OK;
}

/* void requestFrame (); */
NS_IMETHODIMP
zapUDPSocket::RequestFrame()
{
  NS_ASSERTION(!mSinkWaiting, "protocol error");
  mSinkWaiting = PR_TRUE;
  return NS_OK;
}


//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapUDPSocket::ConnectSource(zapIMediaSource *source,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mSource, "already connected");
  mSource = source;

  // request first buffer frame:
  mWaitingOnSource = PR_TRUE;
  mSource->RequestFrame();
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapUDPSocket::DisconnectSource(zapIMediaSource *source,
                              const nsACString & connection_id)
{
  mSource = nsnull;
  mWaitingOnSource = PR_FALSE;
  return NS_OK;
}

/* void processFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapUDPSocket::ProcessFrame(zapIMediaFrame *frame)
{
  nsCOMPtr<nsIDatagram> dg = do_QueryInterface(frame);
  if (!dg) {
#ifdef DEBUG
    printf("n");
#endif
    // silently drop EOF or incompatible frame
  }
  else {
    nsCString data, address;
    PRInt32 port;
    dg->GetData(data);
    dg->GetAddress(address);
    dg->GetPort(&port);
    mSocket->Send(data, address, port);
  }

  // request next frame
  // XXX we need some pacing callback here
  mWaitingOnSource = PR_FALSE;
  mSource->RequestFrame();
  
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIUDPReceiver methods:

/* void handleDatagram (in nsIUDPSocket socket, in nsIDatagram data); */
NS_IMETHODIMP
zapUDPSocket::HandleDatagram(nsIUDPSocket *socket, nsIDatagram *data)
{
  if (!mSinkWaiting) {
    // silently discard datagram:
#ifdef DEBUG
    printf("d");
#endif
  }
  else {
    NS_ASSERTION(mSink, "uh-oh, no sink!");
    NS_ASSERTION(data, "no datagram");
    // build frame
    zapDatagramWrapperFrame* frame = new zapDatagramWrapperFrame();
    frame->AddRef();
    frame->mStreamInfo = mStreamInfo;
    frame->mDatagram = data;
    mSinkWaiting = PR_FALSE;
    mSink->ProcessFrame(frame);
    frame->Release();
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIUDPSocket methods:

/* readonly attribute unsigned short port; */
NS_IMETHODIMP
zapUDPSocket::GetPort(PRUint16 *aPort)
{
  if (!mSocket) {
    *aPort = 0;
    return NS_ERROR_FAILURE;
  }

  PRInt32 port;
  mSocket->GetPort(&port);
  if (port == -1) {
    *aPort = 0;
    return NS_ERROR_FAILURE;
  }
  *aPort = (PRUint16)port;
  
  return NS_OK;
}
