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
 * Portions created by the Initial Developer are Copyright (C) 2006
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

#include "zapStunClient.h"
#include "nsIServiceManager.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "nsIEventTarget.h"
#include "zapMediaUtils.h"
#include "zapDatagramFrame.h"
#include "nsICancelable.h"
#include "nsIDNSListener.h"
#include "nsITimer.h"
#include "zapIStunClientListener.h"
#include "nsAutoPtr.h"

////////////////////////////////////////////////////////////////////////
// zapStunBindingRequest

class zapStunBindingRequest : public nsIDNSListener,
                              public nsITimerCallback,
                              public nsICancelable
{
public:
  zapStunBindingRequest();
  ~zapStunBindingRequest();

  nsresult Init(zapStunClient* stunClient,
                zapIStunClientListener* listener,
                const nsACString & stunServer,
                const nsACString & username,
                const nsACString & password);

  nsresult ConsumeResponse(zapIStunMessage * response);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSICANCELABLE

private:
  nsRefPtr<zapStunClient> mClient;
  nsCOMPtr<zapIStunClientListener> mListener;

  // stun server contacted by current request:
  nsCString mStunServerAddress;
  PRInt32 mStunServerPort;

  // current dns request (only used while resolving stun server):
  nsCOMPtr<nsICancelable> mDNSRequest;

  nsCOMPtr<zapIStunMessage> mStunRequest;
  nsCString mTransactionID;
  nsCString mPassword;
  
  nsCOMPtr<nsITimer> mRetransmissionTimer;
  PRUint32 mRetransmissionCount;  
};

//----------------------------------------------------------------------

zapStunBindingRequest::zapStunBindingRequest()
{
#ifdef DEBUG
  printf("zapStunBindingRequest::zapStunBindingRequest()\n");
#endif
}

zapStunBindingRequest::~zapStunBindingRequest()
{
#ifdef DEBUG
  printf("zapStunBindingRequest::~zapStunBindingRequest()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

// addref/release need to be threadsafe because we will be
// addrefed/released from the timer thread. (Nothing else needs to be
// threadsafe though).
NS_IMPL_THREADSAFE_ADDREF(zapStunBindingRequest)
NS_IMPL_THREADSAFE_RELEASE(zapStunBindingRequest)

NS_INTERFACE_MAP_BEGIN(zapStunBindingRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICancelable)
  NS_INTERFACE_MAP_ENTRY(nsIDNSListener)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsICancelable)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------

nsresult
zapStunBindingRequest::Init(zapStunClient* stunClient,
                            zapIStunClientListener* listener,
                            const nsACString & stunServer,
                            const nsACString & username,
                            const nsACString & password)
{
  mPassword = password;
  
  // XXX parse server address properly
  PRInt32 colon = stunServer.FindChar(':');
  if (colon != -1) {
    mStunServerAddress = Substring(stunServer, 0, colon);
    PRInt32 error;
    nsCString port(Substring(stunServer, colon));
    mStunServerPort = port.ToInteger(&error);
    if (error != NS_OK) {
      NS_ERROR("Error parsing server address");
      return NS_ERROR_FAILURE;
    }
  }
  else {
    mStunServerAddress = stunServer;
    mStunServerPort = 3478;
  }

  // XXX SRV lookup
  nsCOMPtr<nsIEventTarget> eventTarget;
  stunClient->mGraph->GetEventQueue(getter_AddRefs(eventQ));
  nsCOMPtr<nsIDNSService> dnsService = do_GetService("@mozilla.org/network/dns-service;1");
  if (NS_FAILED(dnsService->AsyncResolve(mStunServerAddress, 0, this,
                                         eventQ, getter_AddRefs(mDNSRequest)))) {
    return NS_ERROR_FAILURE;
  }
  
  mListener = listener;
  
  // generate a stun message with new transaction id:
  stunClient->mNetUtils->CreateStunMessage(getter_AddRefs(mStunRequest));
  mStunRequest->SetMessageType(zapIStunMessage::BINDING_REQUEST_MESSAGE);
  mStunRequest->InitTransactionID();
  mStunRequest->GetTransactionID(mTransactionID);
  
  if (!username.IsVoid()) {
    mStunRequest->SetUsername(username);
    mStunRequest->SetHasUsernameAttrib(PR_TRUE);
  }
  if (!mPassword.IsVoid()) {
    mStunRequest->GenerateMessageIntegrityAttrib(mPassword);
  }
  
  // register with our stun client, so that we get responses and
  // cancel events:
  mClient = stunClient;
  this->AddRef();
  mClient->mRequests.Put(mTransactionID, this);
  
  // -> OnLookupComplete() | Cancel()
  return NS_OK;  
}

/* nsIDNSListener::onLookupComplete */
NS_IMETHODIMP
zapStunBindingRequest::OnLookupComplete(nsICancelable *aRequest,
                                        nsIDNSRecord *aRecord,
                                        nsresult aStatus)
{
  mDNSRequest = nsnull;
  if (!aRecord) {
    if (mListener) {
      mListener->BindingRequestComplete(nsnull, this);
      mListener = nsnull;
    }
    mStunRequest = nsnull;
    if (mClient) {
      mClient->mRequests.Remove(mTransactionID);
      mClient = nsnull;
      Release();
    }
  }
  else {
    // XXX We only look at the first record for the time being
    aRecord->GetNextAddrAsString(mStunServerAddress);

    // send request and start retransmission timer:
    mRetransmissionCount = 0;
    mRetransmissionTimer = do_CreateInstance("@mozilla.org/timer;1");
    mRetransmissionTimer->InitWithCallback(this, 100,
                                           nsITimer::TYPE_ONE_SHOT);
    mClient->SendStunRequest(mStunRequest, mStunServerAddress, mStunServerPort);
    // -> Notify() | ConsumeResponse() | Cancel()
  }
  
  return NS_OK;
}

/* nsITimerCallback::notify(in nsITimer timer) */
NS_IMETHODIMP
zapStunBindingRequest::Notify(nsITimer *aTimer)
{
  NS_ASSERTION(mRetransmissionTimer, "no timer!");
  
  if (mRetransmissionCount == 8) {
    // failure to get an answer from the server.
    mRetransmissionTimer = nsnull;
    if (mListener) {
      mListener->BindingRequestComplete(nsnull, this);
      mListener = nsnull;
    }
    mStunRequest = nsnull;
    mRetransmissionTimer = nsnull;
    if (mClient) {
      mClient->mRequests.Remove(mTransactionID);
      mClient = nsnull;
      Release();
    }
  }
  else {
    // resend request & reset timer:
    mClient->SendStunRequest(mStunRequest, mStunServerAddress, mStunServerPort);
    ++mRetransmissionCount;
    PRUint32 oldDelay;
    mRetransmissionTimer->GetDelay(&oldDelay);
    mRetransmissionTimer->InitWithCallback(this, PR_MIN(2*oldDelay, 1600),
                                           nsITimer::TYPE_ONE_SHOT);
    // -> Notify() | ConsumeResponse() | Cancel()
  }
  return NS_OK;
}

/* void cancel (in nsresult aReason) */
NS_IMETHODIMP
zapStunBindingRequest::Cancel(nsresult aReason)
{
  if (mDNSRequest) {
      // this will take care of notifying and clearing our listener,
      // clearing the dns request and removing ourselves from the hash
      // of pending requests:
      mDNSRequest->Cancel(NS_ERROR_FAILURE);
  }
  else if (mRetransmissionTimer) {
    mRetransmissionTimer->Cancel();
    if (mListener) {
      mListener->BindingRequestComplete(nsnull, this);
      mListener = nsnull;
    }
    mStunRequest = nsnull;
    mRetransmissionTimer = nsnull;
    if (mClient) {
      mClient->mRequests.Remove(mTransactionID);
      mClient = nsnull;
      Release();
    }
  }
  
  return NS_OK;
}

nsresult
zapStunBindingRequest::ConsumeResponse(zapIStunMessage * message)
{
  if (!mRetransmissionTimer) {
    // we haven't sent the request yet. this must be a transactionid
    // clash
    return NS_ERROR_FAILURE;
  }

  
  // draft-ietf-behave-rfc3489bis-02.txt: If the response is a Binding
  // Response, the client SHOULD check the response for a
  // MESSAGE-INTEGRITY attribute.  If not present, and the client
  // placed a MESSAGE-INTEGRITY attribute into the request, it MUST
  // discard the response.
  PRUint16 type;
  message->GetMessageType(&type);
  if (type == zapIStunMessage::BINDING_RESPONSE_MESSAGE) {
    PRBool hasMessageIntegrity;
    message->GetHasMessageIntegrityAttrib(&hasMessageIntegrity);
    if (hasMessageIntegrity) {
      // check message integrity
      PRBool match;
      message->CheckMessageIntegrity(mPassword, &match);
      if (!match) {        
        // If we have already tried with XOR-ONLY, we discard this
        // response, so that we don't retry again and again:
        PRBool triedXOROnly;
        mStunRequest->GetHasXOROnlyAttrib(&triedXOROnly);
        if (triedXOROnly) return NS_OK;
      
        // draft-ietf-behave-rfc3489bis-02.txt:
        // If the computed HMAC differs from the one in the response,
        // the client SHOULD determine if the integrity check failed due
        // to a NAT rewriting the MAPPED-ADDRESS.  To perform this
        // check, the client compares the IP address and port in the
        // MAPPED-ADDRESS with the IP address and port extracted from
        // XOR- MAPPED-ADDRESS (extraction involves xor'ing the contents
        // of X-port and X-value with the transaction ID, as described
        // in Section 10).  If the two IP addresses and ports differ,
        // the client MUST discard the response, but then it SHOULD
        // retry the Binding Request with the XOR- ONLY attribute
        // included.  This tells the server not to include a
        // MAPPED-ADDRESS in the Binding Response.  If there is no
        // XOR-MAPPED-ADDRESS, or if there is, but there are no
        // differences between the two IP addresses and ports, the
        // client MUST discard the response and SHOULD alert the user
        // about a possible attack.
        PRBool hasAttrib;
        message->GetHasXORMappedAddressAttrib(&hasAttrib);
        if (!hasAttrib) {
          // silently discard response
          return NS_OK;
        }
        message->GetHasMappedAddressAttrib(&hasAttrib);
        if (!hasAttrib) {
          // silently discard response
          return NS_OK;
        }
        nsCString addr1, addr2;
        PRUint16 port1, port2;
        message->GetMappedAddress(addr1);
        message->GetXORMappedAddress(addr2);
        message->GetMappedAddressPort(&port1);
        message->GetXORMappedAddressPort(&port2);
        if (addr1 != addr2 || port1 != port2) {
          // discard response and retry with XOR-ONLY
          
          // ammend request
          message->SetHasXOROnlyAttrib(PR_TRUE);
          if (!mPassword.IsVoid()) {
            mStunRequest->GenerateMessageIntegrityAttrib(mPassword);
          }
          // reset retransmission state machine:
          mRetransmissionTimer->Cancel();
          mRetransmissionCount = 0;
          mRetransmissionTimer->InitWithCallback(this, 100,
                                                 nsITimer::TYPE_ONE_SHOT);
          mClient->SendStunRequest(mStunRequest, mStunServerAddress, mStunServerPort);
          return NS_OK;        
        }
        else {
          // silently discard response
          return NS_OK;
        }
      }
    }
    else if (!mPassword.IsVoid()) {
      // we placed a message integrity attrib into the request, but the
      // response doesn't have one
      // -> silently discard response
      return NS_OK;
    }
  }

  // the response is acceptable (whether a binding response or binding
  // error response). Stop retransmitting, inform listener, cleanup:  
  mRetransmissionTimer->Cancel();

  if (mListener) {
    mListener->BindingRequestComplete(message, this);
    mListener = nsnull;
  }
  mStunRequest = nsnull;
  mRetransmissionTimer = nsnull;
  if (mClient) {
    mClient->mRequests.Remove(mTransactionID);
    mClient = nsnull;
    Release();
  }

  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapStunClient

zapStunClient::zapStunClient()
{
  mRequests.Init();
}

zapStunClient::~zapStunClient()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

// addref/release need to be threadsafe because we will be
// addrefed/released from the timer thread. (Nothing else needs to be
// threadsafe though).
NS_IMPL_THREADSAFE_ADDREF(zapStunClient)
NS_IMPL_THREADSAFE_RELEASE(zapStunClient)

NS_INTERFACE_MAP_BEGIN(zapStunClient)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIStunClient)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIStunClient methods:

/* nsICancelable sendBindingRequest (in zapIStunClientListener listener, in ACString stunServer, in ACString username, in ACString password); */
NS_IMETHODIMP
zapStunClient::SendBindingRequest(zapIStunClientListener *listener,
                                  const nsACString & stunServer,
                                  const nsACString & username,
                                  const nsACString & password,
                                  nsICancelable **_retval)
{
  if (!mOutput || !mInput) {
    NS_ERROR("Can't send binding request in unconnected state");
    return NS_ERROR_FAILURE;
  }

  zapStunBindingRequest* request = new zapStunBindingRequest();
  request->AddRef();
  if (NS_FAILED(request->Init(this, listener,
                              stunServer, username, password))) {
    request->Release();
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  *_retval = request;
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStunClient::AddedToGraph(zapIMediaGraph *graph,
                            const nsACString & id,
                            nsIPropertyBag2* node_pars)
{
  mGraph = graph;
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "datagram");
  mNetUtils = do_GetService("@mozilla.org/zap/netutils;1");
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapStunClient::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStunClient::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapStunClient::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (mInput) {
    NS_ERROR("input end already connected");
    *_retval = nsnull;
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
zapStunClient::ConnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapStunClient::DisconnectSink(zapIMediaSink *sink,
                              const nsACString & connection_id)
{
  mOutput = nsnull;
  CancelPendingRequests();
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStunClient::ProduceFrame(zapIMediaFrame ** _retval)
{
  NS_ERROR("Not a passive source");
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapStunClient::ConnectSource(zapIMediaSource *source,
                             const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapStunClient::DisconnectSource(zapIMediaSource *source,
                                const nsACString & connection_id)
{
  mInput = nsnull;
  CancelPendingRequests();
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStunClient::ConsumeFrame(zapIMediaFrame * frame)
{
  // we accept stun frames ...
  nsCOMPtr<zapIStunMessage> message = do_QueryInterface(frame);

  // ... or any packets whose data can be parsed into stun messages:
  if (!message) {
    // try to parse into STUN packet
    nsCString data;
    frame->GetData(data);
    if (NS_FAILED(mNetUtils->DeserializeStunPacket(data, nsnull, nsnull, getter_AddRefs(message)))) {
      return NS_ERROR_FAILURE;
    }
  }
  
  // check if it's either a binding response or binding error
  // response:
  PRUint16 type;
  message->GetMessageType(&type);
  if (type != zapIStunMessage::BINDING_RESPONSE_MESSAGE &&
      type != zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE) {
    return NS_ERROR_FAILURE;
  }
  
  // match to request:
  nsCString transactionID;
  message->GetTransactionID(transactionID);
  zapStunBindingRequest* request = nsnull;
  mRequests.Get(transactionID, &request);
  if (!request) {
    return NS_ERROR_FAILURE;
  }

  return request->ConsumeResponse(message);
}

//----------------------------------------------------------------------
// Implementation helpers:

void
zapStunClient::SendStunRequest(zapIStunMessage* message,
                               const nsACString& server,
                               PRInt32 port)
{
  NS_ASSERTION(mOutput, "inconsistent state");
  
  // create datagram frame:
  zapDatagramFrame* frame = new zapDatagramFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  message->Serialize(frame->mData);
  frame->mAddress = server;
  frame->mPort = port;
  mOutput->ConsumeFrame(frame);
  frame->Release();
}

static PLDHashOperator
RemoveRequests(zapStunClient::RequestHash::KeyType key,
               zapStunBindingRequest*&data, void*) {
  // the following line will take care of cleaning up the request. It
  // will attempt and fail to remove itself from the hash. removal
  // will be taken care of by returning PL_DHASH_REMOVE.
  data->Cancel(NS_ERROR_FAILURE);
  return (PLDHashOperator)(PL_DHASH_NEXT | PL_DHASH_REMOVE);
}

void
zapStunClient::CancelPendingRequests()
{
  mRequests.Enumerate(RemoveRequests, nsnull);
  NS_ASSERTION(mRequests.Count() == 0, "failed to remove pending requests");
}
