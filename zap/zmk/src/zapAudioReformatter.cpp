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

#include "zapAudioReformatter.h"
#include "zapIMediaGraph.h"
#include "nsAutoPtr.h"
#include "zapMediaFrame.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////
// zapAudioReformatter

zapAudioReformatter::zapAudioReformatter()
    : mInBufferPointer(0)
{
}

zapAudioReformatter::~zapAudioReformatter()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioReformatter)
NS_IMPL_RELEASE(zapAudioReformatter)

NS_INTERFACE_MAP_BEGIN(zapAudioReformatter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioReformatter::AddedToGraph(zapIMediaGraph *graph,
                                  const nsACString & id,
                                  nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  if (NS_FAILED(mOutStreamPars.InitWithProperties(node_pars)))
    return NS_ERROR_FAILURE;

  if ( (mOutStreamPars.sample_format != sf_float32_32768 &&
        mOutStreamPars.sample_format != sf_float32_1 &&
        mOutStreamPars.sample_format != sf_int16) ||
      mOutStreamPars.channels > 2) {
    NS_ERROR("Unsupported sample format! Write me!");
    return NS_ERROR_FAILURE;
  }
  
  mStreamInfo = mOutStreamPars.CreateStreamInfo();
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioReformatter::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioReformatter::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapAudioReformatter::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapAudioReformatter::ConnectSink(zapIMediaSink *sink,
                                 const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioReformatter::DisconnectSink(zapIMediaSink *sink,
                                    const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioReformatter::ProduceFrame(zapIMediaFrame ** _retval)
{
  PRUint32 samplesLeft = mOutStreamPars.samples * mOutStreamPars.channels;

  nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();
  frame->mData.SetLength(mOutStreamPars.GetFrameLength());
  float *outputBuf = (float*)frame->mData.BeginWriting();

  while (samplesLeft) {
    if (IsInBufferEmpty() && !GetNextInputFrame()) {
      *_retval = nsnull;
      return NS_ERROR_FAILURE;
    }
    
    PRUint32 samplesWritten = ProduceSamples(outputBuf, samplesLeft);
    if (!samplesWritten) {
      *_retval = nsnull;
      return NS_ERROR_FAILURE;
    }
    
    outputBuf += samplesWritten;
    samplesLeft -= samplesWritten;
  }

  frame->mStreamInfo = mStreamInfo;

  *_retval = frame;
  NS_ADDREF(*_retval);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioReformatter::ConnectSource(zapIMediaSource *source,
                                   const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioReformatter::DisconnectSource(zapIMediaSource *source,
                                      const nsACString & connection_id)
{
  mInput = nsnull;
  
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioReformatter::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

PRBool
zapAudioReformatter::IsInBufferEmpty()
{
  return mInBuffer.Length() <= mInBufferPointer;
}

PRBool
zapAudioReformatter::GetNextInputFrame()
{
  if (!mInput) return PR_FALSE;
  
  nsCOMPtr<zapIMediaFrame> frame;
  if (NS_FAILED(mInput->ProduceFrame(getter_AddRefs(frame))))
    return PR_FALSE;
  NS_ASSERTION(frame, "null frame");
  
  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  NS_ASSERTION(streamInfo, "null stream info!");
  
  if (streamInfo != mInStreamInfo) {
    // this is a new stream.
    if (NS_FAILED(mInStreamPars.InitWithProperties(streamInfo)))
      return PR_FALSE;
    if (mInStreamPars.sample_rate != mOutStreamPars.sample_rate) {
      NS_ERROR("node can't resample; only reformat!");
      return NS_ERROR_FAILURE;
    }    
    mInStreamInfo = streamInfo;
  }

  frame->GetData(mInBuffer);
  mInBufferPointer = 0;
  
  return mInBuffer.Length();
}

PRUint32
zapAudioReformatter::ProduceSamples(float *out, PRUint32 sampleCount)
{
  char *inp = (char*)mInBuffer.BeginReading()+mInBufferPointer;
  char *endp = (char*)mInBuffer.BeginReading()+mInBuffer.Length();
  
  if (mInStreamPars.sample_format == mOutStreamPars.sample_format &&
      mInStreamPars.channels == mOutStreamPars.channels) {
    // fast special case. just copy across samples:
    PRUint32 byteCount = PR_MIN(sampleCount*GetZapAudioSampleSize(mInStreamPars.sample_format),
                                endp - inp);
    memcpy(out, inp, byteCount);
    mInBufferPointer += byteCount;
    return byteCount/GetZapAudioSampleSize(mInStreamPars.sample_format);
  }

  // generic case
  
  PRInt32 dChannels = mOutStreamPars.channels - mInStreamPars.channels;
  if (dChannels < 0) {
    NS_ERROR("unsupported conversion! write me!");
    return 0;
  }

  PRUint32 samplesWritten = 0;
  
  while (inp != endp && sampleCount > samplesWritten) {
    float sample;
    switch (mInStreamPars.sample_format) {
      case sf_float32_32768:
      case sf_float32_1:
        sample = *(float*)inp;
        inp += sizeof(float);
        break;
      case sf_int16:
        sample = (float)*(PRInt16*)inp;
        inp += sizeof(PRInt16);
        break;
      default:
        NS_ERROR("unsupported conversion! write me!");
        return 0;
    }
    *out++ = sample;
    ++samplesWritten;
    if (dChannels) {
      *out++ = sample;
      ++samplesWritten;
    }
  }
  mInBufferPointer = inp - (char*)mInBuffer.BeginReading();
  return samplesWritten;
}
