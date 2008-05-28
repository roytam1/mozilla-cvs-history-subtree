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

#include "zapStunFrame.h"
#include "nsComponentManagerUtils.h"

////////////////////////////////////////////////////////////////////////
// zapStunFrame

PRBool zapStunFrame::Init(nsIDatagram* datagram, nsIPropertyBag2* streamInfo)
{
  mStreamInfo = streamInfo;
  
  // aggregate stun message:
  mStunMessage = do_CreateInstance("@mozilla.org/zap/stun-message;1",
                                   (zapIMediaFrame*)this);
  if (!mStunMessage) {
    NS_ERROR("failed to create aggregated STUN message");
    return PR_FALSE;
  }
  
  nsCString data;
  datagram->GetData(data);
  nsCOMPtr<zapIStunMessage> stunMessageItf = do_QueryInterface((zapIMediaFrame*)this);
  if (NS_FAILED(stunMessageItf->Deserialize(data, nsnull, nsnull)))
    return PR_FALSE;
  
  datagram->GetAddress(mAddress);
  datagram->GetPort(&mPort);

  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapStunFrame)
NS_IMPL_THREADSAFE_RELEASE(zapStunFrame)

NS_INTERFACE_MAP_BEGIN(zapStunFrame)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaFrame)
  NS_INTERFACE_MAP_ENTRY(zapIMediaFrame)
  NS_INTERFACE_MAP_ENTRY(nsIDatagram)
NS_INTERFACE_MAP_END_AGGREGATED(mStunMessage)

//----------------------------------------------------------------------
// zapIMediaFrame methods:

/* attribute nsIPropertyBag2 streamInfo; */
NS_IMETHODIMP
zapStunFrame::GetStreamInfo(nsIPropertyBag2** aStreamInfo)
{
  *aStreamInfo = mStreamInfo.get();
  NS_IF_ADDREF(*aStreamInfo);
  return NS_OK;
}
NS_IMETHODIMP
zapStunFrame::SetStreamInfo(nsIPropertyBag2 * aStreamInfo)
{
  mStreamInfo = aStreamInfo;
  return NS_OK;
}

/* attribute unsigned long long timestamp; */
NS_IMETHODIMP
zapStunFrame::GetTimestamp(PRUint64 *aTimestamp)
{
  *aTimestamp = 0;
  // shouldn't be called for stun frames
  return NS_ERROR_FAILURE;
}
NS_IMETHODIMP
zapStunFrame::SetTimestamp(PRUint64 aTimestamp)
{
  // shouldn't be called for stun frames
  NS_ERROR("Can't set timestamp for STUN frames");
  return NS_ERROR_FAILURE;
}

/* readonly attribute ACString data; */
NS_IMETHODIMP
zapStunFrame::GetData(nsACString & aData)
{
  nsCOMPtr<zapIStunMessage> stunMessageItf = do_QueryInterface((zapIMediaFrame*)this);
  NS_ASSERTION(stunMessageItf, "uh-oh, no stun message");
  return stunMessageItf->Serialize(aData);
}

//----------------------------------------------------------------------
// nsIDatagram methods:

/* readonly attribute ACString data; */
// see zapIMediaFrame

/* readonly attribute ACString address; */
NS_IMETHODIMP
zapStunFrame::GetAddress(nsACString & aAddress)
{
  aAddress = mAddress;
  return NS_OK;
}

/* readonly attribute long port; */
NS_IMETHODIMP
zapStunFrame::GetPort(PRInt32 *aPort)
{
  *aPort = mPort;
  return NS_OK;
}
