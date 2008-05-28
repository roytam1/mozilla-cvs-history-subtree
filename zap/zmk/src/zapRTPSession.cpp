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
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsHashPropertyBag.h"
#include "zapIRTPFrame.h"

////////////////////////////////////////////////////////////////////////
// zapRTPSessionFilter

class zapRTPSessionFilter : public zapIMediaSource,
                            public zapIMediaSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK

  typedef nsresult (zapRTPSession::*RTPSessionFilterFct)(zapIMediaFrame* in,
                                                         zapIMediaFrame** out);
  
  zapRTPSessionFilter(zapRTPSession* parent, RTPSessionFilterFct fct);
  ~zapRTPSessionFilter();

  nsRefPtr<zapRTPSession> mParent;
  RTPSessionFilterFct mFilterFct;

  nsCOMPtr<zapIMediaSink> mOutput;
  nsCOMPtr<zapIMediaSource> mInput;
};

zapRTPSessionFilter::zapRTPSessionFilter(zapRTPSession* parent,
                                         RTPSessionFilterFct fct)
{
  mParent = parent;
  mFilterFct = fct;
}

zapRTPSessionFilter::~zapRTPSessionFilter()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRTPSessionFilter)
NS_IMPL_RELEASE(zapRTPSessionFilter)

NS_INTERFACE_MAP_BEGIN(zapRTPSessionFilter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRTPSessionFilter::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "output end already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRTPSessionFilter::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapRTPSessionFilter::ProduceFrame(zapIMediaFrame ** frame)
{
  *frame = nsnull;
  
  if (!mInput) return NS_ERROR_FAILURE;

  nsCOMPtr<zapIMediaFrame> input_frame;
  if (NS_FAILED(mInput->ProduceFrame(getter_AddRefs(input_frame))))
    return NS_ERROR_FAILURE;

  return (mParent->*mFilterFct)(input_frame, frame);
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapRTPSessionFilter::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "input end already connected");
  mInput = source;
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapRTPSessionFilter::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapRTPSessionFilter::ConsumeFrame(zapIMediaFrame *frame)
{
  if (!mOutput) return NS_ERROR_FAILURE;

  nsCOMPtr<zapIMediaFrame> output_frame;
  if (NS_FAILED((mParent->*mFilterFct)(frame, getter_AddRefs(output_frame))))
    return NS_ERROR_FAILURE;

  return mOutput->ConsumeFrame(output_frame);
}



////////////////////////////////////////////////////////////////////////
// zapRTPSession

zapRTPSession::zapRTPSession()
{
}

zapRTPSession::~zapRTPSession()
{
  NS_ASSERTION(!mLocal2RemoteRTP && !mRemote2LocalRTP, "unclean shutdown");
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

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapRTPSession::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                     nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
//   if (!node_pars) {
//     NS_ERROR("no node parameters");
//     return NS_ERROR_FAILURE;
//   }

//   NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint32(
//                       NS_LITERAL_STRING("SSRC"), &mSSRC),
//                     NS_ERROR_FAILURE);
    // XXX implement collision resolution (RFC3550)
  mSSRC = rand();

  // RFC3550 5.1:
  // The initial value of the sequence number SHOULD be random
  // (unpredictable) to make known-plaintext attacks on encryption
  // more difficult, even if the source itself does not encrypt
  // according to the method in Section 9.1, because the packets may
  // flow through a translator that does.
  mSequenceNumber = rand();
  
//   nsCString address;
//   NS_ENSURE_SUCCESS(node_pars->GetPropertyAsACString(NS_LITERAL_STRING("address"),
//                                                      address),
//                     NS_ERROR_FAILURE);
  
//   PRUint32 rtpPort, rtcpPort;
//   NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("rtp_port"),
//                                                    &rtpPort),
//                     NS_ERROR_FAILURE);
//   NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("rtcp_port"),
//                                                    &rtcpPort),
//                     NS_ERROR_FAILURE);
  
  // construct our filters:
  mLocal2RemoteRTP = new zapRTPSessionFilter(this, &zapRTPSession::FilterLocal2RemoteRTP);
  mRemote2LocalRTP = new zapRTPSessionFilter(this, &zapRTPSession::FilterRemote2LocalRTP);
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapRTPSession::RemovedFromContainer()
{
  mLocal2RemoteRTP = nsnull;
  mRemote2LocalRTP = nsnull;
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapRTPSession::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  // dispatch depending on source name given in source parameters
  if (!source_pars) {
    NS_ERROR("no source pars");
    return NS_ERROR_FAILURE;
  }

  nsCString source_name;
  NS_ENSURE_SUCCESS(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), source_name),
                    NS_ERROR_FAILURE);
  
  if (source_name == NS_LITERAL_CSTRING("remote2local-rtp")) {
    *_retval = mRemote2LocalRTP;
  }
  else if (source_name == NS_LITERAL_CSTRING("local2remote-rtp")) {
    *_retval = mLocal2RemoteRTP;
  }
  else {
    *_retval = nsnull;
    NS_ERROR("unknown source");
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapRTPSession::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  // dispatch depending on source name given in sink parameters
  if (!sink_pars) {
    NS_ERROR("no sink pars");
    return NS_ERROR_FAILURE;
  }

  nsCString sink_name;
  NS_ENSURE_SUCCESS(sink_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"), sink_name),
                    NS_ERROR_FAILURE);

  if (sink_name == NS_LITERAL_CSTRING("local2remote-rtp")) {
    *_retval = mLocal2RemoteRTP;
  }
  else if (sink_name == NS_LITERAL_CSTRING("remote2local-rtp")) {
    *_retval = mRemote2LocalRTP;
  }
  else {
    *_retval = nsnull;
    NS_ERROR("unknown sink");
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*_retval);
  return NS_OK;
}

//----------------------------------------------------------------------
//

nsresult
zapRTPSession::FilterLocal2RemoteRTP(zapIMediaFrame* in, zapIMediaFrame** out)
{
  nsCOMPtr<zapIRTPFrame> rtpFrame = do_QueryInterface(in);
  if (!rtpFrame) {
    NS_ERROR("unexpected frame");
    *out = nsnull;
    return NS_ERROR_FAILURE;
  }

  // set SSRC and sequence number:
  rtpFrame->SetSSRC(mSSRC);
  rtpFrame->SetSequenceNumber(mSequenceNumber++);
  
  *out = in;
  NS_ADDREF(*out);
  return NS_OK;
}

nsresult
zapRTPSession::FilterRemote2LocalRTP(zapIMediaFrame* in, zapIMediaFrame** out)
{
  *out = in;
  NS_ADDREF(*out);
  return NS_OK;
}
