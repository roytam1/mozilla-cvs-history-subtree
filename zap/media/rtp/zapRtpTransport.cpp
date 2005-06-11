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

#include "zapRtpTransport.h"
#include "nsComponentManagerUtils.h"
#include "stdio.h"
#include "zapRtpPacket.h"
#include "stdlib.h" // for rand()

////////////////////////////////////////////////////////////////////////
// zapRtpTransport implementation

zapRtpTransport::zapRtpTransport()
    : mRemoteRtpPort(0),
      mRemoteRtcpPort(0)
{
  mLocalSequence = rand();
  mSSRC = rand();
}

zapRtpTransport::~zapRtpTransport()
{
  Close();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRtpTransport)
NS_IMPL_RELEASE(zapRtpTransport)

NS_INTERFACE_MAP_BEGIN(zapRtpTransport)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIRtpTransport)
  NS_INTERFACE_MAP_ENTRY(zapIRtpTransport)
  NS_INTERFACE_MAP_ENTRY(nsIUDPReceiver)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIRtpTransport methods:

/* void open (in unsigned long localRtpPort, in unsigned long localRtcpPort); */
NS_IMETHODIMP
zapRtpTransport::Open(PRUint32 localRtpPort, PRUint32 localRtcpPort)
{
  // open rtp & rtcp sockets:
  mRtpSocket = do_CreateInstance("@mozilla.org/network/udp-socket;1");
  if (!mRtpSocket) return NS_ERROR_FAILURE;
  if (NS_FAILED(mRtpSocket->Init(localRtpPort)))
    return NS_ERROR_FAILURE;
  mRtpSocket->SetReceiver(this);
  
  mRtcpSocket = do_CreateInstance("@mozilla.org/network/udp-socket;1");
  if (!mRtcpSocket) return NS_ERROR_FAILURE;
  if (NS_FAILED(mRtcpSocket->Init(localRtcpPort)))
    return NS_ERROR_FAILURE;
  mRtcpSocket->SetReceiver(this);

  return NS_OK;
}

/* void close (); */
NS_IMETHODIMP
zapRtpTransport::Close()
{
  if (mRtpSocket) {
    mRtpSocket->Close();
    mRtpSocket = nsnull;
  }
  if (mRtcpSocket) {
    mRtcpSocket->Close();
    mRtcpSocket = nsnull;
  }
  return NS_OK;
}

/* void setRtpHandler (in zapIRtpHandler handler); */
NS_IMETHODIMP
zapRtpTransport::SetRtpHandler(zapIRtpHandler *handler)
{
  mHandler = handler;
  return NS_OK;
}

/* void setRemoteEndpoint (in ACString remoteHost, in unsigned long remoteRtpPort, in unsigned long remoteRtcpPort); */
NS_IMETHODIMP
zapRtpTransport::SetRemoteEndpoint(const nsACString& remoteHost,
                                   PRUint32 remoteRtpPort,
                                   PRUint32 remoteRtcpPort)
{
  mRemoteHost = remoteHost;
  mRemoteRtpPort = remoteRtpPort;
  mRemoteRtcpPort = remoteRtcpPort;
  return NS_OK;
}

/* void send (in ACString payload, in unsigned short payloadType, in unsigned long timestamp); */
NS_IMETHODIMP
zapRtpTransport::Send(const nsACString & payload,
                      PRUint16 payloadType, PRUint32 timestamp)
{
  if (!mRtpSocket) {
    NS_WARNING("Trying to send data on unitialized rtp transport.");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<zapIRtpPacket> packet = CreateRtpPacket();
  packet->SetPayloadType(payloadType);
  packet->SetSequenceNumber(mLocalSequence++);
  packet->SetSSRC(mSSRC);
  packet->SetTimestamp(timestamp);
  packet->SetPayload(payload);

#ifdef DEBUG
//      {
//        PRUint16 payloadtype, sequencenumber;
//        PRUint32 timestamp, ssrc;
//        packet->GetPayloadType(&payloadtype);
//        packet->GetSequenceNumber(&sequencenumber);
//        packet->GetTimestamp(&timestamp);
//        packet->GetSSRC(&ssrc);
//        nsCString data;
//        packet->GetPayload(data);
//        printf("send: %d(%d) %d %d %d %d\n", payload.Length(), data.Length(), payloadtype, sequencenumber, timestamp, ssrc);
//      }
#endif
  
  nsCString data;
  packet->GetPacket(data);
  return mRtpSocket->Send(data, mRemoteHost, mRemoteRtpPort);
}


//----------------------------------------------------------------------
// nsIUDPReceiver methods:

/* void handleDatagram (in nsIUDPSocket socket, in nsIDatagram data); */
NS_IMETHODIMP
zapRtpTransport::HandleDatagram(nsIUDPSocket *socket, nsIDatagram *data)
{
  if (socket == mRtpSocket) {
     nsCString octets;
     data->GetData(octets);
     nsCOMPtr<zapIRtpPacket> packet = CreateRtpPacket(octets);
     // XXX preprocess packet
     
#ifdef DEBUG_XXX
     {
       PRUint16 payloadtype, sequencenumber;
       PRUint32 timestamp, ssrc;
       packet->GetPayloadType(&payloadtype);
       packet->GetSequenceNumber(&sequencenumber);
       packet->GetTimestamp(&timestamp);
       packet->GetSSRC(&ssrc);
       nsCString data;
       packet->GetPayload(data);
       printf("receive: %d %d %d %d %d\n", data.Length(), payloadtype, sequencenumber, timestamp, ssrc);
     }
#endif
  

     if (packet && mHandler) {
       mHandler->HandleRtpPacket(packet);
     }
  }
#ifdef DEBUG
  if (socket == mRtcpSocket)
    printf("\n[rtcp datagram]\n");
#endif
  return NS_OK;
}

//----------------------------------------------------------------------
// Create an add-refed rtp transport:

zapIRtpTransport *
CreateRtpTransport()
{
  zapRtpTransport* transport = new zapRtpTransport();
  if (!transport) return nsnull;
  NS_ADDREF(transport);
  
  return transport;
}
