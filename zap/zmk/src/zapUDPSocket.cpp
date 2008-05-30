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
#include "nsComponentManagerUtils.h"
#include "nsStringAPI.h"
#include "zapDatagramFrame.h"
#include "nsNetCID.h"
#include "nsAutoPtr.h"
#include "zapZMKImplUtils.h"

////////////////////////////////////////////////////////////////////////
// zapUDPSocket

zapUDPSocket::zapUDPSocket()
{
}

zapUDPSocket::~zapUDPSocket()
{
  NS_ASSERTION(!mSocket, "unclean shutdown");
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

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapUDPSocket::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                    nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  if (!node_pars ||
      NS_FAILED(node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("socket"),
                                                  NS_GET_IID(nsIUDPSocket),
                                                  getter_AddRefs(mSocket))) ||
      !mSocket) {
    PRUint32 port = 0;
    PRUint32 max_port = 0;
    if (node_pars &&
        NS_SUCCEEDED(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("port"), &port))) {
      max_port = port;
    }
    else if (node_pars &&
             NS_SUCCEEDED(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("portbase"), &port))) {
      max_port = 65535;
    }
      
    mSocket = do_CreateInstance(NS_UDPSOCKET_CONTRACTID);
    if (!mSocket) {
      NS_ERROR("failure creating socket");
      return NS_ERROR_FAILURE;
    }

    PRBool success = PR_FALSE;
    for (/**/; port <= max_port; ++port) {
      if (NS_SUCCEEDED(mSocket->Init(port))) {
        success = PR_TRUE;
        break;
      }
    }
    if (!success) {
      NS_ERROR("port initialization failed");
      mSocket = nsnull;
      return NS_ERROR_FAILURE;
    }
  }

  mSocket->SetReceiver(this);

  // create a new streaminfo:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "datagram");
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapUDPSocket::RemovedFromContainer()
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
  if (mOutput) {
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
  if (mInput) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}


//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapUDPSocket::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapUDPSocket::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapUDPSocket::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}


//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapUDPSocket::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapUDPSocket::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapUDPSocket::ConsumeFrame(zapIMediaFrame * frame)
{
  nsCOMPtr<nsIDatagram> dg = do_QueryInterface(frame);
  if (!dg) {
    // drop incompatible frame
    return NS_ERROR_FAILURE;
  }

  nsCString data, address;
  PRInt32 port;
  dg->GetData(data);
  dg->GetAddress(address);
  dg->GetPort(&port);
  mSocket->Send(data, address, port);

  return NS_OK;
}

//----------------------------------------------------------------------
// nsIUDPReceiver methods:

/* void handleDatagram (in nsIUDPSocket socket, in nsIDatagram data); */
NS_IMETHODIMP
zapUDPSocket::HandleDatagram(nsIUDPSocket *socket, nsIDatagram *data)
{
  NS_ASSERTION(data, "no datagram");
  if (!mOutput) return NS_OK; // discard silently
    
  // build frame
  nsRefPtr<zapDatagramWrapperFrame> frame = new zapDatagramWrapperFrame();
  frame->mStreamInfo = mStreamInfo;
  frame->mDatagram = data;
  mOutput->ConsumeFrame(frame);
  
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
