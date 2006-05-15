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

#include "zapStunServer.h"
#include "nsIServiceManager.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "nsIEventTarget.h"
#include "zapMediaUtils.h"
#include "zapDatagramFrame.h"

////////////////////////////////////////////////////////////////////////
// zapStunServer

zapStunServer::zapStunServer()
{
  mAllowedClients.Init();
}

zapStunServer::~zapStunServer()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStunServer)
NS_IMPL_RELEASE(zapStunServer)

NS_INTERFACE_MAP_BEGIN(zapStunServer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIStunServer)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIStunServer methods:

/* void addAllowedClient (in ACString username, in ACString password); */
NS_IMETHODIMP
zapStunServer::AddAllowedClient(const nsACString & username,
                                const nsACString & password)
{
  nsCString* pw = new nsCString(password);
  if (!mAllowedClients.Put(username, pw)) {
    delete pw;
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* void removeAllowedClient (in ACString username); */
NS_IMETHODIMP
zapStunServer::RemoveAllowedClient(const nsACString & username)
{
  mAllowedClients.Remove(username);
  return NS_OK;
}

/* attribute zapIStunServerListener listener; */
NS_IMETHODIMP
zapStunServer::GetListener(zapIStunServerListener * *aListener)
{
  *aListener = mListener;
  NS_IF_ADDREF(*aListener);
  return NS_OK;
}
NS_IMETHODIMP
zapStunServer::SetListener(zapIStunServerListener * aListener)
{
  mListener = aListener;
  return NS_OK;
}


//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStunServer::AddedToGraph(zapIMediaGraph *graph,
                            const nsACString & id,
                            nsIPropertyBag2* node_pars)
{
  mGraph = graph;

  if (!node_pars) return NS_ERROR_FAILURE;

  if (NS_FAILED(node_pars->GetPropertyAsACString(NS_LITERAL_STRING("source_addr"),
                                                 mSourceAddress)))
    return NS_ERROR_FAILURE;
  if (NS_FAILED(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("source_port"),
                                               &mSourcePort)))
    return NS_ERROR_FAILURE;
  
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "datagram");
  mNetUtils = do_GetService("@mozilla.org/zap/netutils;1");
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapStunServer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStunServer::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapStunServer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapStunServer::ConnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapStunServer::DisconnectSink(zapIMediaSink *sink,
                              const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStunServer::ProduceFrame(zapIMediaFrame ** _retval)
{
  NS_ERROR("Not a passive source");
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapStunServer::ConnectSource(zapIMediaSource *source,
                             const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapStunServer::DisconnectSource(zapIMediaSource *source,
                                const nsACString & connection_id)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStunServer::ConsumeFrame(zapIMediaFrame * frame)
{
  if (!mOutput) {
    return NS_ERROR_FAILURE;
  }
  
  // we accept stun frames ...
  nsCOMPtr<zapIStunMessage> request = do_QueryInterface(frame);

  // ... or any packets whose data can be parsed into stun messages:
  if (!request) {
    // try to parse into STUN packet
    nsCString data;
    frame->GetData(data);
    if (NS_FAILED(mNetUtils->DeserializeStunPacket(data, nsnull, nsnull, getter_AddRefs(request)))) {
      return NS_ERROR_FAILURE;
    }
  }

  // we also need the address that the request came from:
  nsCOMPtr<nsIDatagram> datagram = do_QueryInterface(frame);
  if (!datagram) {
    return NS_ERROR_FAILURE;
  }
  nsCString fromAddress;
  PRInt32 fromPort;
  datagram->GetAddress(fromAddress);
  datagram->GetPort(&fromPort);

  // check if it's binding request 
  PRUint16 type;
  request->GetMessageType(&type);
  if (type != zapIStunMessage::BINDING_REQUEST_MESSAGE) {
    return NS_ERROR_FAILURE;
  }
  
  // we've passed all checks; the request is of the right type and
  // (probably) destined for us. generate a response.
  nsCOMPtr<zapIStunMessage> response;
  mNetUtils->CreateStunMessage(getter_AddRefs(response));

  nsCString transactionID;
  request->GetTransactionID(transactionID);
  response->SetTransactionID(transactionID);

//    draft-ietf-behave-rfc3489bis-02
//    The server SHOULD add a SERVER attribute to any Binding Response or
//    Binding Error Response it generates, and its value SHOULD indicate
//    the manufacturer of the software and a software version or build
//    number.

  response->SetServer(NS_LITERAL_CSTRING("zap"));
  response->SetHasServerAttrib(PR_TRUE);
  
  PRBool hasAttrib;
  nsCString username;
  nsCString *ppassword;
  PRBool match;
  
//    draft-ietf-mmusic-ice-06
//    The agent does not need to provide STUN service on any other IP
//    addresses or ports, unlike the STUN usage described in [1].  The need
//    to run the service on multiple ports is to support receipt of Binding
//    Requests with the CHANGE-REQUEST attribute.  However, that attribute
//    is not used when STUN is used for connectivity checks.  A server
//    SHOULD reject, with a 400 answer, any STUN requests with a CHANGE-
//    REQUEST attribute whose value is non-zero.  

  request->GetHasChangeRequestAttrib(&hasAttrib);
  if (hasAttrib) {
    response->SetMessageType(zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE);
    response->SetErrorCode(NS_LITERAL_CSTRING("400"));
    response->SetErrorCodeReasonPhrase(NS_LITERAL_CSTRING("CHANGE-REQUEST not supported"));
    response->SetHasErrorCodeAttrib(PR_TRUE);
    goto done;
  }
  
//    draft-ietf-behave-rfc3489bis-02
//    It is RECOMMENDED that the server check the Binding Request for
//    a MESSAGE-INTEGRITY attribute. If not present, and the server
//    requires integrity checks on the request, it generates a Binding
//    Error Response with an ERROR-CODE attribute with response code
//    401.
  // We require MESSAGE-INTEGRITY for all requests:
  request->GetHasMessageIntegrityAttrib(&hasAttrib);
  if (!hasAttrib) {
    response->SetMessageType(zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE);
    response->SetErrorCode(NS_LITERAL_CSTRING("401"));
    response->SetErrorCodeReasonPhrase(NS_LITERAL_CSTRING("Unauthorized"));
    response->SetHasErrorCodeAttrib(PR_TRUE);
    goto done;
  }

//    draft-ietf-behave-rfc3489bis-02
//    If the MESSAGE-INTEGRITY attribute was present, the server
//    computes the HMAC over the request as described in Section
//    10.2.8.  The key to use depends on the shared secret mechanism.
//    If the STUN Shared Secret Request was used, the key MUST be the
//    one associated with the USERNAME attribute present in the
//    request.  If the USERNAME attribute was not present, the server
//    MUST generate a Binding Error Response.  The Binding Error
//    Response MUST include an ERROR-CODE attribute with response code
//    432.
  request->GetHasUsernameAttrib(&hasAttrib);
  if (!hasAttrib) {
    response->SetMessageType(zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE);
    response->SetErrorCode(NS_LITERAL_CSTRING("432"));
    response->SetErrorCodeReasonPhrase(NS_LITERAL_CSTRING("Missing Username"));
    response->SetHasErrorCodeAttrib(PR_TRUE);
    goto done;
  }
  
//    draft-ietf-behave-rfc3489bis-02
//    If the USERNAME is present, but the server doesn't
//    remember the shared secret for that USERNAME (because it timed
//    out, for example), the server MUST generate a Binding Error
//    Response.  The Binding Error Response MUST include an ERROR-CODE
//    attribute with response code 430.
  request->GetUsername(username);
  if (!mAllowedClients.Get(username, &ppassword)) {
    response->SetMessageType(zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE);
    response->SetErrorCode(NS_LITERAL_CSTRING("430"));
    response->SetErrorCodeReasonPhrase(NS_LITERAL_CSTRING("Unauthorized client"));
    response->SetHasErrorCodeAttrib(PR_TRUE);
    goto done;
  }

//    draft-ietf-behave-rfc3489bis-02
//    If the server does know the shared secret, but the computed HMAC
//    differs from the one in the request, the server MUST generate a
//    Binding Error Response with an ERROR-CODE attribute with
//    response code 431.  The Binding Error Response is sent to the IP
//    address and port the Binding Request came from, and sent from
//    the IP address and port the Binding Request was sent to.
  request->CheckMessageIntegrity(*ppassword, &match);
  if (!match) {
    response->SetMessageType(zapIStunMessage::BINDING_ERROR_RESPONSE_MESSAGE);
    response->SetErrorCode(NS_LITERAL_CSTRING("431"));
    response->SetErrorCodeReasonPhrase(NS_LITERAL_CSTRING("Integrity Check failure"));
    response->SetHasErrorCodeAttrib(PR_TRUE);
    goto done;
  }
  
//    draft-ietf-behave-rfc3489bis-02
//    The server MUST check for any attributes in the request with values
//    less than or equal to 0x7fff which it does not understand.  If it
//    encounters any, the server MUST generate a Binding Error Response,
//    and it MUST include an ERROR-CODE attribute with a 420 response code.
//    That response MUST contain an UNKNOWN-ATTRIBUTES attribute listing
//    the attributes with values less than or equal to 0x7fff which were
//    not understood.  The Binding Error Response is sent to the IP address
//    and port the Binding Request came from, and sent from the IP address
//    and port the Binding Request was sent to.

// XXX

  // generate a binding response
  response->SetMessageType(zapIStunMessage::BINDING_RESPONSE_MESSAGE);

    
//    draft-ietf-behave-rfc3489bis-02
//    If the XOR-ONLY attribute was not present in the request, the server
//    MUST add a MAPPED-ADDRESS attribute to the Binding Response.  The IP
//    address component of this attribute MUST be set to the source IP
//    address observed in the Binding Request.  The port component of this
//    attribute MUST be set to the source port observed in the Binding
//    Request.  If the XOR-ONLY attribute was present in the request, the
//    server MUST NOT include the MAPPED-ADDRESS attribute in the Binding
//    Response.
//    The server MUST add a XOR-MAPPED-ADDRESS attribute to the Binding
//    Response.

  PRBool xorOnly;
  request->GetHasXOROnlyAttrib(&xorOnly);
  if (!xorOnly) {
    response->SetMappedAddress(fromAddress);
    response->SetMappedAddressPort(fromPort);
    response->SetHasMappedAddressAttrib(PR_TRUE);
  }
  
  response->SetXORMappedAddress(fromAddress);
  response->SetXORMappedAddressPort(fromPort);
  response->SetHasXORMappedAddressAttrib(PR_TRUE);
  
//    draft-ietf-behave-rfc3489bis-02
//    The source address and port of the Binding Response depend on the
//    value of the CHANGE-REQUEST attribute and on the address and port the
//    Binding Request was received on, and are summarized in Table 1.
// XXX we filter out requests with CHANGE-REQUEST, see above.
    
//    draft-ietf-behave-rfc3489bis-02
//    The server MUST add a SOURCE-ADDRESS attribute to the Binding
//    Response, containing the source address and port used to send the
//    Binding Response.

  response->SetSourceAddress(mSourceAddress);
  response->SetSourceAddressPort(mSourcePort);
  response->SetHasSourceAddressAttrib(PR_TRUE);
    
//    draft-ietf-behave-rfc3489bis-02
//    The server MUST add a CHANGED-ADDRESS attribute to the Binding
//    Response.  This contains the source IP address and port that would be
//    used if the client had set the "change IP" and "change port" flags in
//    the Binding Request.  As summarized in Table 1, these are Ca and Cp,
//    respectively, regardless of the value of the CHANGE-REQUEST flags.
//    draft-ietf-mmusic-ice-06
//    The CHANGED-ADDRESS attribute in a BindingAnswer is set to the
//    transport address on which the server is running.

  response->SetChangedAddress(mSourceAddress);
  response->SetChangedAddressPort(mSourcePort);
  response->SetHasChangedAddressAttrib(PR_TRUE);

//    draft-ietf-behave-rfc3489bis-02
//    If the Binding Request contained a RESPONSE-ADDRESS attribute, the
//    server MUST add a REFLECTED-FROM attribute to the response.  If the
//    Binding Request was authenticated using a username obtained from a
//    Shared Secret Request, the REFLECTED-FROM attribute MUST contain the
//    source IP address and port where that Shared Secret Request came
//    from.  If the username present in the request was not allocated using
//    a Shared Secret Request, the REFLECTED-FROM attribute MUST contain
//    the source address and port of the entity which obtained the
//    username, as best can be verified with the mechanism used to allocate
//    the username.  If the username was not present in the request, and
//    the server was willing to process the request, the REFLECTED-FROM
//    attribute SHOULD contain the source IP address and port where the
//    request came from.

  // XXX
  
//    draft-ietf-behave-rfc3489bis-02
//    If the Binding Request contained both the USERNAME and MESSAGE-
//    INTEGRITY attributes, the server MUST add a MESSAGE-INTEGRITY
//    attribute to the Binding Response.  The attribute contains an HMAC
//    [13] over the response, as described in Section 10.2.8.  The key to
//    use depends on the shared secret mechanism.  If the STUN Shared
//    Secret Request was used, the key MUST be the one associated with the
//    USERNAME attribute present in the Binding Request.

  // We always add a MESSAGE-INTEGRITY attrib (we reject requests
  // without MESSAGE-INTEGRITY/USERNAME):
  response->GenerateMessageIntegrityAttrib(*ppassword);

  // inform listener:
  if (mListener)
    mListener->BindingRequestReceived(username, fromAddress, fromPort);

  // common processing for binding & error responses:
  done:
  
  // create datagram:
  zapDatagramFrame* outframe = new zapDatagramFrame();
  outframe->AddRef();
  outframe->mStreamInfo = mStreamInfo;
  response->Serialize(outframe->mData);


//    draft-ietf-behave-rfc3489bis-02
//    If the RESPONSE-ADDRESS attribute was absent from the Binding
//    Request, the destination address and port of the Binding Response
//    MUST be the same as the source address and port of the Binding
//    Request.  Otherwise, the destination address and port of the Binding
//    Response MUST be the value of the IP address and port in the
//    RESPONSE-ADDRESS attribute.

// XXX we always send to the source address atm
  
  outframe->mAddress = fromAddress;
  outframe->mPort = fromPort;
  mOutput->ConsumeFrame(outframe);
  outframe->Release();

  
  return NS_OK;
}

