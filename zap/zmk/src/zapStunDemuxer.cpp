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

#include "zapStunDemuxer.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "math.h"
#include "nsServiceManagerUtils.h"
#include "zapZMKImplUtils.h"
#include "nsIUDPSocket.h"
#include "zapStunFrame.h"

////////////////////////////////////////////////////////////////////////
// zapStunDemuxer

zapStunDemuxer::zapStunDemuxer()
{
  mStunReqOutput = new zapGenericSource();
  mStunResOutput = new zapGenericSource();
  mOtherOutput = new zapGenericSource();
  mNetUtils = do_GetService("@mozilla.org/zap/netutils;1");
}

zapStunDemuxer::~zapStunDemuxer()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStunDemuxer)
NS_IMPL_RELEASE(zapStunDemuxer)

NS_INTERFACE_MAP_BEGIN(zapStunDemuxer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStunDemuxer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                      nsIPropertyBag2* node_pars)
{
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "stun");
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapStunDemuxer::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStunDemuxer::GetSource(nsIPropertyBag2 *source_pars,
                          zapIMediaSource **_retval)
{
  *_retval = nsnull;

  if (!source_pars) {
    NS_ERROR("no source pars");
    return NS_ERROR_FAILURE;
  }

  nsCString sourceName;
  NS_ENSURE_SUCCESS(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"),
                                                       sourceName),
                    NS_ERROR_FAILURE);
  if (sourceName == NS_LITERAL_CSTRING("stun-req")) {
    *_retval = mStunReqOutput;
  }
  else if (sourceName == NS_LITERAL_CSTRING("stun-res")) {
    *_retval = mStunResOutput;
  }
  else if (sourceName == NS_LITERAL_CSTRING("other")) {
    *_retval = mOtherOutput;
  }
  else {
    NS_ERROR("Unknown source");
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapStunDemuxer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStunDemuxer::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "sink already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStunDemuxer::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStunDemuxer::ConsumeFrame(zapIMediaFrame * frame)
{
  nsCOMPtr<nsIDatagram> datagram = do_QueryInterface(frame);
  if (!datagram) return NS_ERROR_FAILURE;
  
  // dispatch frame based on type:
  nsCString data;
  frame->GetData(data);
  PRInt32 type;
  mNetUtils->SnoopStunPacket(data, &type);
  if (type > 0) {
    // it appears to be a STUN packet
    nsRefPtr<zapStunFrame> stunFrame = new zapStunFrame();
    if (stunFrame &&
        stunFrame->Init(datagram, mStreamInfo)) {
      PRUint16 messageType;
      nsCOMPtr<zapIStunMessage> stunMessage = do_QueryInterface((zapIMediaFrame*)stunFrame);
      NS_ASSERTION(stunMessage, "uh oh! No stun message interface!");
      stunMessage->GetMessageType(&messageType);
      if (messageType > 256) {  
        // a response
        if (mStunResOutput && mStunResOutput->mOutput)
          return mStunResOutput->mOutput->ConsumeFrame(stunFrame);
        else
          return NS_OK;
      }
      else {
        // a request
        if (mStunReqOutput && mStunReqOutput->mOutput)
          return mStunReqOutput->mOutput->ConsumeFrame(stunFrame);
        else
          return NS_OK;
      }
    }
    // else fall through
  }

  if (mOtherOutput && mOtherOutput->mOutput)
    return mOtherOutput->mOutput->ConsumeFrame(frame);

  return NS_OK;
}


