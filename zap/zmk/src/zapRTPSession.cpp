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

#include "zapRTPSession.h"
#include <stdlib.h> // for rand
#include "zapIMediaGraph.h"
#include "nsHashPropertyBag.h"

////////////////////////////////////////////////////////////////////////
// zapRTPSession

zapRTPSession::zapRTPSession()
{
#ifdef DEBUG
  printf("zapRTPSession::zapRTPSession()\n");
#endif
}

zapRTPSession::~zapRTPSession()
{
  NS_ASSERTION(!mTransmitter && !mReceiver, "unclean shutdown");
#ifdef DEBUG
  printf("zapRTPSession::~zapRTPSession()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRTPSession)
NS_IMPL_RELEASE(zapRTPSession)

NS_INTERFACE_MAP_BEGIN(zapRTPSession)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapRTPSession::AddedToGraph(zapIMediaGraph *graph,
                         const nsACString & id,
                         nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  if (!node_pars) {
    NS_ERROR("no node parameters");
    return NS_ERROR_FAILURE;
  }

  nsCString address;
  NS_ENSURE_SUCCESS(node_pars->GetPropertyAsACString(NS_LITERAL_STRING("address"),
                                                     address),
                    NS_ERROR_FAILURE);
  
  PRUint16 rtpPort, rtcpPort;
  NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint16(NS_LITERAL_STRING("rtp_port"),
                                                   &rtpPort),
                    NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint16(NS_LITERAL_STRING("rtcp_port"),
                                                   &rtcpPort),
                    NS_ERROR_FAILURE);
  
  // hook up transmitter:
  nsCOMPtr<nsIWritablePropertyBag2> transmitter_pars;
  NS_NewHashPropertyBag2(getter_AddRefs(transmitter_pars));
  transmitter_pars->SetPropertyAsACString(NS_LITERAL_STRING("address"),
                                          address);
  transmitter_pars->SetPropertyAsUint16(NS_LITERAL_STRING("port"), rtpPort);
  
  NS_ENSURE_SUCCESS(graph->AddNode(NS_LITERAL_CSTRING("rtp-transmitter"),
                                   transmitter_pars, mTransmitterID),
                    NS_ERROR_FAILURE);
  graph->GetNode(mTransmitterID, NS_GET_IID(zapIMediaNode), true,
                 getter_AddRefs(mTransmitter));
  if (!mTransmitter) {
    NS_ERROR("transmitter doesn't implement required interface");
    graph->RemoveNode(mTransmitterID);
    return NS_ERROR_FAILURE;
  }

  // hook up receiver:
  if (NS_FAILED(graph->AddNode(NS_LITERAL_CSTRING("rtp-receiver"),
                               nsnull, mReceiverID))) {
    graph->RemoveNode(mTransmitterID);
    return NS_ERROR_FAILURE;
  }
  graph->GetNode(mReceiverID, NS_GET_IID(zapIMediaNode), true,
                 getter_AddRefs(mReceiver));
  if (!mReceiver) {
    NS_ERROR("receiver doesn't implement required interface");
    graph->RemoveNode(mReceiverID);
    graph->RemoveNode(mTransmitterID);
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapRTPSession::RemovedFromGraph(zapIMediaGraph *graph)
{
  if (mTransmitter) {
    graph->RemoveNode(mTransmitterID);
    mTransmitter = nsnull;
  }
  if (mReceiver) {
    graph->RemoveNode(mReceiverID);
    mReceiver = nsnull;
  }
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapRTPSession::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  NS_ASSERTION(mTransmitter, "uh-oh, no transmitter");
  NS_ASSERTION(mReceiver, "uh-oh, no receiver");
  
  // dispatch depending on source name given in source parameters
  if (!source_pars) {
    NS_ERROR("no source pars");
    return NS_ERROR_FAILURE;
  }

  nsCString source_name;
  NS_ENSURE_SUCCESS(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), source_name),
                    NS_ERROR_FAILURE);
  
  if (source_name == NS_LITERAL_CSTRING("remote-rtp")) {
    return mTransmitter->GetSource(source_pars, _retval);
  }
  else if (source_name == NS_LITERAL_CSTRING("local-rtp")) {
    return mReceiver->GetSource(source_pars, _retval);
  }

  NS_ERROR("unknown source");
  return NS_ERROR_FAILURE;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapRTPSession::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ASSERTION(mTransmitter, "uh-oh, no transmitter");
  NS_ASSERTION(mReceiver, "uh-oh, no receiver");

  // dispatch depending on source name given in sink parameters
  if (!sink_pars) {
    NS_ERROR("no sink pars");
    return NS_ERROR_FAILURE;
  }

  nsCString sink_name;
  NS_ENSURE_SUCCESS(sink_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), sink_name),
                    NS_ERROR_FAILURE);

  if (sink_name == NS_LITERAL_CSTRING("local-rtp")) {
    return mTransmitter->GetSink(sink_pars, _retval);
  }
  else if (sink_name == NS_LITERAL_CSTRING("remote-rtp")) {
    return mReceiver->GetSink(sink_pars, _retval);
  }

  NS_ERROR("unknown sink");
  return NS_ERROR_FAILURE;
}


