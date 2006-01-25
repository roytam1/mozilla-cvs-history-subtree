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

#include "zapFilterNode.h"
#include "zapIMediaFrame.h"
#include "zapIMediaGraph.h"
#include "nsIPropertyBag2.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapFilterNode

zapFilterNode::zapFilterNode()
{
}

zapFilterNode::~zapFilterNode()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapFilterNode)
NS_IMPL_RELEASE(zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapFilterNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// zapIMediaNode methods:

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapFilterNode::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapFilterNode::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapFilterNode::ConnectSink(zapIMediaSink *sink, const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "output end already connected");
  mOutput = sink;
  mCurrentInputStreamInfo = nsnull;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapFilterNode::DisconnectSink(zapIMediaSink *sink, const nsACString & connection_id)
{
  mOutput = nsnull;
  mCurrentInputStreamInfo = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapFilterNode::ProduceFrame(zapIMediaFrame ** frame)
{
  *frame = nsnull;
  
  if (!mInput) return NS_ERROR_FAILURE;

  nsCOMPtr<zapIMediaFrame> input_frame;
  if (NS_FAILED(mInput->ProduceFrame(getter_AddRefs(input_frame))))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPropertyBag2> streamInfo;
  input_frame->GetStreamInfo(getter_AddRefs(streamInfo));
  if (streamInfo != mCurrentInputStreamInfo) {
    if (NS_FAILED(ValidateNewStream(streamInfo)))
      return NS_ERROR_FAILURE;
    mCurrentInputStreamInfo = streamInfo;
  }

  return Filter(input_frame, frame);
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapFilterNode::ConnectSource(zapIMediaSource *source, const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "input end already connected");
  mInput = source;
  mCurrentInputStreamInfo = nsnull;
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapFilterNode::DisconnectSource(zapIMediaSource *source, const nsACString & connection_id)
{
  mInput = nsnull;
  mCurrentInputStreamInfo = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapFilterNode::ConsumeFrame(zapIMediaFrame *frame)
{
  if (!mOutput) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  if (streamInfo != mCurrentInputStreamInfo) {
    if (NS_FAILED(ValidateNewStream(streamInfo)))
      return NS_ERROR_FAILURE;
    mCurrentInputStreamInfo = streamInfo;
  }
  
  nsCOMPtr<zapIMediaFrame> output_frame;
  if (NS_FAILED(Filter(frame, getter_AddRefs(output_frame))))
    return NS_ERROR_FAILURE;

  return mOutput->ConsumeFrame(output_frame);
}

