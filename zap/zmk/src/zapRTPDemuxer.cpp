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

#include "zapRTPDemuxer.h"
#include "zapIMediaGraph.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "math.h"
#include "zapIRTPFrame.h"

////////////////////////////////////////////////////////////////////////
// zapRTPDemuxerOutput

class zapRTPDemuxerOutput : public zapIMediaSource
{
public:
  zapRTPDemuxerOutput();
  ~zapRTPDemuxerOutput();

  nsresult Init(zapRTPDemuxer* RTPDemuxer, nsIPropertyBag2* source_pars);
  nsresult ConsumeFrame(zapIMediaFrame *frame);
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE

private:
  PRUint32 mPayloadType;
  nsRefPtr<zapRTPDemuxer> mRTPDemuxer;
  nsCOMPtr<zapIMediaSink> mOutput;
};

//----------------------------------------------------------------------

zapRTPDemuxerOutput::zapRTPDemuxerOutput()
{
#ifdef DEBUG_afri_zmk
  printf("zapRTPDemuxerOutput::zapRTPDemuxerOutput()\n");
#endif
}

zapRTPDemuxerOutput::~zapRTPDemuxerOutput()
{
  NS_ASSERTION(mRTPDemuxer, "Never initialized");
#ifdef DEBUG_afri_zmk
  printf("zapRTPDemuxerOutput::~zapRTPDemuxerOutput()\n");
#endif
  // clean up references:
  if (mRTPDemuxer) {
    mRTPDemuxer->mOutputs.Remove(mPayloadType);
    mRTPDemuxer = nsnull;
  }
}

nsresult
zapRTPDemuxerOutput::Init(zapRTPDemuxer* RTPDemuxer,
                          nsIPropertyBag2* source_pars) {
  // unpack source pars:
  if (!source_pars ||
      NS_FAILED(source_pars->GetPropertyAsUint32(NS_LITERAL_STRING("payload_type"),
                                                 &mPayloadType))) {
    return NS_ERROR_FAILURE;
  }

  // check if an output for this payload type already exists:
  zapRTPDemuxerOutput* dummy;
  if (RTPDemuxer->mOutputs.Get(mPayloadType, &dummy))
    return NS_ERROR_FAILURE;
  
  mRTPDemuxer = RTPDemuxer;
  // append ourselves to the RTPDemuxer's array of inputs:
  mRTPDemuxer->mOutputs.Put(mPayloadType, this);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRTPDemuxerOutput)
NS_IMPL_RELEASE(zapRTPDemuxerOutput)

NS_INTERFACE_MAP_BEGIN(zapRTPDemuxerOutput)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRTPDemuxerOutput::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRTPDemuxerOutput::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapRTPDemuxerOutput::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

nsresult zapRTPDemuxerOutput::ConsumeFrame(zapIMediaFrame *frame)
{
  if (mOutput)
    return mOutput->ConsumeFrame(frame);
  return NS_ERROR_FAILURE;
}


////////////////////////////////////////////////////////////////////////
// zapRTPDemuxer

zapRTPDemuxer::zapRTPDemuxer()
{
#ifdef DEBUG_afri_zmk
  printf("zapRTPDemuxer::zapRTPDemuxer()\n");
#endif
  mOutputs.Init();
}

zapRTPDemuxer::~zapRTPDemuxer()
{
#ifdef DEBUG_afri_zmk
  printf("zapRTPDemuxer::~zapRTPDemuxer()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRTPDemuxer)
NS_IMPL_RELEASE(zapRTPDemuxer)

NS_INTERFACE_MAP_BEGIN(zapRTPDemuxer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapRTPDemuxer::AddedToGraph(zapIMediaGraph *graph,
                            const nsACString & id,
                            nsIPropertyBag2* node_pars)
{
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapRTPDemuxer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapRTPDemuxer::GetSource(nsIPropertyBag2 *source_pars,
                         zapIMediaSource **_retval)
{
  zapRTPDemuxerOutput* output = new zapRTPDemuxerOutput();
  output->AddRef();
  if (NS_FAILED(output->Init(this, source_pars))) {
    output->Release();
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  *_retval = output;
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapRTPDemuxer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapRTPDemuxer::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "sink already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapRTPDemuxer::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapRTPDemuxer::ConsumeFrame(zapIMediaFrame * frame)
{
  nsCOMPtr<zapIRTPFrame> rtpFrame = do_QueryInterface(frame);
  if (!rtpFrame) {
    NS_WARNING("not an rtp frame");
    return NS_ERROR_FAILURE;
  }

  PRUint16 payloadType;
  rtpFrame->GetPayloadType(&payloadType);
  zapRTPDemuxerOutput *output;
  if (!mOutputs.Get(payloadType, &output)) {
#ifdef DEBUG_afri_zmk
    printf("zapRTPDemuxer: unknown payload %d\n", payloadType);
#endif
    return NS_ERROR_FAILURE;
  }

  return output->ConsumeFrame(frame);
}

