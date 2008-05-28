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

#include "zapUDPSocketPair.h"
#include "nsXPCOMCIDInternal.h"
#include "zapUDPSocket.h"
#include "nsIUDPSocket.h"
#include "nsNetCID.h"
#include "nsComponentManagerUtils.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapUDPSocketPair

zapUDPSocketPair::zapUDPSocketPair()
{
}

zapUDPSocketPair::~zapUDPSocketPair()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapUDPSocketPair)
NS_IMPL_THREADSAFE_RELEASE(zapUDPSocketPair)

NS_INTERFACE_MAP_BEGIN(zapUDPSocketPair)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNodeContainer)
  NS_INTERFACE_MAP_ENTRY(zapIUDPSocketPair)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapUDPSocketPair::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                        nsIPropertyBag2* node_pars)
{
  mContainer = container;
  
  PRUint32 portbase = 49152;
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("portbase"), &portbase);
  }

  if (portbase == 0 || portbase >= 65535) {
    NS_ERROR("portbase must be between 1 and 65534");
    return NS_ERROR_FAILURE;
  }
  
  // create sockets:
  nsCOMPtr<nsIUDPSocket> socketA = do_CreateInstance(NS_UDPSOCKET_CONTRACTID);
  nsCOMPtr<nsIUDPSocket> socketB = do_CreateInstance(NS_UDPSOCKET_CONTRACTID);

  if (!socketA || !socketB) {
    NS_ERROR("failure creating sockets");
    return NS_ERROR_FAILURE;
  }
  
  // search for free ports port_a, port_b
  // with portbase <= port_a, port_a = 2*n, and port_b = 2*n+1:

  // make sure portbase is next even number:
  portbase = ((portbase+1)>>1)<<1;
  PRUint32 port = portbase;
  for (/**/; port<65535; port+=2) {
    if (NS_SUCCEEDED(socketA->Init(port))) {
      if (NS_SUCCEEDED(socketB->Init(port+1))) {
        break;
      }
      else {
        socketA->Close();
      }
    }
  }
  if (port == 65536) {
    NS_ERROR("no free ports found");
    return NS_ERROR_FAILURE;
  }

  // create a propertybag for initializing our socket nodes:
  nsCOMPtr<nsIWritablePropertyBag2> props = do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID);

  // create socket nodes:
  mSocketA = new zapUDPSocket();
  props->SetPropertyAsInterface(NS_LITERAL_STRING("socket"), socketA);
  if (NS_FAILED(mSocketA->InsertedIntoContainer(this, props))) {
    mSocketA = nsnull;
    return NS_ERROR_FAILURE;
  }

  mSocketB = new zapUDPSocket();
  props->SetPropertyAsInterface(NS_LITERAL_STRING("socket"), socketB);
  if (NS_FAILED(mSocketA->InsertedIntoContainer(this, props))) {
    mSocketA->RemovedFromContainer();
    mSocketA = nsnull;
    mSocketB = nsnull;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapUDPSocketPair::RemovedFromContainer()
{
  if (mSocketA) {
    mSocketA->RemovedFromContainer();
    mSocketA = nsnull;
  }
  if (mSocketB) {
    mSocketB->RemovedFromContainer();
    mSocketB = nsnull;
  }
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapUDPSocketPair::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  NS_ASSERTION(mSocketA, "uh-oh, no socket A");
  NS_ASSERTION(mSocketB, "uh-oh, no socket B");
  
  // dispatch depending on source name given in source parameters
  if (!source_pars) {
    NS_ERROR("no source pars");
    return NS_ERROR_FAILURE;
  }

  nsCString source_name;
  NS_ENSURE_SUCCESS(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), source_name),
                    NS_ERROR_FAILURE);
  
  if (source_name == NS_LITERAL_CSTRING("socket-a")) {
    return mSocketA->GetSource(source_pars, _retval);
  }
  else if (source_name == NS_LITERAL_CSTRING("socket-b")) {
    return mSocketB->GetSource(source_pars, _retval);
  }

  NS_ERROR("unknown source");
  return NS_ERROR_FAILURE;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapUDPSocketPair::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ASSERTION(mSocketA, "uh-oh, no socket A");
  NS_ASSERTION(mSocketB, "uh-oh, no socket B");

  // dispatch depending on source name given in sink parameters
  if (!sink_pars) {
    NS_ERROR("no sink pars");
    return NS_ERROR_FAILURE;
  }

  nsCString sink_name;
  NS_ENSURE_SUCCESS(sink_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), sink_name),
                    NS_ERROR_FAILURE);

  if (sink_name == NS_LITERAL_CSTRING("socket-a")) {
    return mSocketA->GetSink(sink_pars, _retval);
  }
  else if (sink_name == NS_LITERAL_CSTRING("socket-b")) {
    return mSocketB->GetSink(sink_pars, _retval);
  }

  NS_ERROR("unknown sink");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaNodeContainer methods:

/* void lock (in zapIMediaNode node); */
NS_IMETHODIMP
zapUDPSocketPair::Lock(zapIMediaNode *node)
{
  return mContainer->Lock(node);
}

/* void unlock (in zapIMediaNode node); */
NS_IMETHODIMP
zapUDPSocketPair::Unlock(zapIMediaNode *node)
{
  return mContainer->Unlock(node);
}

/* void notify (in zapIMediaNode node, in nsIPropertyBag2 event); */
NS_IMETHODIMP
zapUDPSocketPair::Notify(zapIMediaNode *node, nsIPropertyBag2 *event)
{
  // our children don't generate any notifications
  return NS_OK;
}

/* readonly attribute nsIEventTarget eventTarget; */
NS_IMETHODIMP
zapUDPSocketPair::GetEventTarget(nsIEventTarget * *aEventTarget)
{
  return mContainer->GetEventTarget(aEventTarget);
}

//----------------------------------------------------------------------
// zapIUDPSocketPair methods:

/* readonly attribute unsigned short portA; */
NS_IMETHODIMP
zapUDPSocketPair::GetPortA(PRUint16 *aPortA)
{
  nsCOMPtr<zapIUDPSocket> udpSocket = do_QueryInterface(mSocketA);
  if (!udpSocket) {
    *aPortA = 0;
    return NS_ERROR_FAILURE;
  }
  return udpSocket->GetPort(aPortA);
}

/* readonly attribute unsigned short portB; */
NS_IMETHODIMP
zapUDPSocketPair::GetPortB(PRUint16 *aPortB)
{
  nsCOMPtr<zapIUDPSocket> udpSocket = do_QueryInterface(mSocketB);
  if (!udpSocket) {
    *aPortB = 0;
    return NS_ERROR_FAILURE;
  }
  return udpSocket->GetPort(aPortB);
}
