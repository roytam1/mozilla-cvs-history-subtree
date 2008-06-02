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

#include "zapAudioToneFrame.h"
#include "zapTToneToPCMConverter.h"
#include "nsAutoPtr.h"
#include "zapMediaFrame.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////
// zapTToneToPCMConverter

zapTToneToPCMConverter::zapTToneToPCMConverter()
{
}

zapTToneToPCMConverter::~zapTToneToPCMConverter()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapTToneToPCMConverter)
NS_IMPL_RELEASE(zapTToneToPCMConverter)

NS_INTERFACE_MAP_BEGIN(zapTToneToPCMConverter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapTToneToPCMConverter::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                              nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  mZtlp = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("ztlp"), 1.0);
  
  if (NS_FAILED(mOutStreamParameters.InitWithProperties(node_pars)))
    return NS_ERROR_FAILURE;

  if (mOutStreamParameters.sample_rate != 8000 ||
      mOutStreamParameters.samples != 160 ||
      mOutStreamParameters.channels != 1 ||
      mOutStreamParameters.sample_format != sf_float32_32768) {
    NS_ERROR("Unsupported sample format! Write me!");
    return NS_ERROR_FAILURE;
  }
  
  mStreamInfo = mOutStreamParameters.CreateStreamInfo();
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapTToneToPCMConverter::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapTToneToPCMConverter::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapTToneToPCMConverter::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapTToneToPCMConverter::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapTToneToPCMConverter::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapTToneToPCMConverter::ProduceFrame(zapIMediaFrame ** _retval)
{
  PRUint32 samplesPerFrame = mOutStreamParameters.samples;
  
  // advance sample clock:
  mSampleClock += samplesPerFrame;

  PRUint32 samplesToWrite = samplesPerFrame;
  double samplePeriod = 1.0/mOutStreamParameters.sample_rate;
  
  zapMediaFrame* frame = nsnull;
  float* d = nsnull;
  while (samplesToWrite) {
    if (!mCurrentInputFrame && !GetNextInputFrame()) {
      // no (further) frame. -> bail
      break;
    }

    // GetNextInputFrame(), above, might have changed the sample clock
    // to resynchronize with a new input stream. Hence we need to
    // recalculate the sampling window here:
    PRUint64 windowStart = mSampleClock + samplesPerFrame - samplesToWrite;
    PRUint64 windowEnd = mSampleClock + samplesPerFrame;
    
    PRUint64 frameStart;
    PRUint16 frameDuration;
    mCurrentInputFrame->GetTimestamp(&frameStart);
    mCurrentInputFrame->GetDuration(&frameDuration);
    PRUint64 frameEnd = frameStart + frameDuration;
    if (frameDuration == 0) {
      // ignore zero length frame
      mCurrentInputFrame = nsnull;
      continue;
    }
    else if ((PRInt64)(frameEnd - windowStart) <= 0) {
      // frame ends in the past.
      // resynchronize clock:
      // XXX is this the right strategy? should we instead expect that
      // sources are synchronized and move on to the next frame?
      mSampleClock = frameStart;
      continue;
    }
    else if ((PRInt64)(windowEnd - frameStart) <= 0) {
      // frame begins in the future. -> bail
      break;
    }
    else {
      // we have a frame and it has samples in the current
      // sampling window
      
      if (!frame) {
        // construct new output frame:
        frame = new zapMediaFrame();
        frame->AddRef();
        frame->mStreamInfo = mStreamInfo;
        frame->mTimestamp = mSampleClock;
        frame->mData.SetLength(mOutStreamParameters.GetFrameLength());
        d = (float*)frame->mData.BeginWriting();
      }
      
      PRUint16 volume, frequencyCount, modulation;
      PRBool T;
      mCurrentInputFrame->GetVolume(&volume);
      mCurrentInputFrame->GetFrequencyCount(&frequencyCount);
      mCurrentInputFrame->GetModulation(&modulation);
      mCurrentInputFrame->GetT(&T);
      double modulationFrequency = T ? modulation : modulation/3.0;
      double amplitude = frequencyCount ? pow(10.0, volume/-10.0) * mZtlp * 32768.0 / frequencyCount : 0.0;

      // construct array of 2*pi*f for all frequencies in the tone:
      double* frequencies = new double[frequencyCount];
      for (int i=0; i<frequencyCount; ++i) {
        PRUint16 temp;
        mCurrentInputFrame->GetFrequencyAt(i, &temp);
        frequencies[i] = 6.28318530718 * temp;
      }

      // write any silence at beginning of frame:
      while ((PRInt64)(windowStart - frameStart) < 0) {
        *d++ = 0.0f;
        --samplesToWrite;
        ++windowStart;
        NS_ASSERTION(samplesToWrite,
                     "ran out of samples to write. this can't happen!");
      }

      NS_ASSERTION((PRInt64)(windowStart - frameStart) >= 0,
                   "this can't happen!");
      
      // write frame samples:
      PRInt64 frameSamplesLeft = (PRInt64)(frameEnd - windowStart);
      NS_ASSERTION(frameSamplesLeft > 0,
                   "no samples left in frame. this can't happen!");
      PRUint32 c = PR_MIN(samplesToWrite, (PRUint32)frameSamplesLeft);
      samplesToWrite -= c;
      if ((PRUint32)frameSamplesLeft == c)
        mCurrentInputFrame = nsnull; // we're done with this frame
        
      double sampleTime = windowStart * samplePeriod;
      while (c) {
        double sample = 0.0f;
        for (int i=0; i<frequencyCount; ++i) {
          sample += sin(frequencies[i] * sampleTime);
        }
        if (modulation) {
          // XXX modulation depth currently fixed to 1
          sample *= sin(modulationFrequency * sampleTime);
        }
        *d++ = (float) (sample * amplitude);
        sampleTime += samplePeriod;
        --c;
      }

      // clean up:
      delete [] frequencies;
    }
  }

  if (!frame) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  // fill remainder of frame with silence samples:
  while (samplesToWrite) {
    *d++ = 0.0f;
    --samplesToWrite;
  }
  
  *_retval = frame;
  return NS_OK;  
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapTToneToPCMConverter::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapTToneToPCMConverter::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapTToneToPCMConverter::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

PRBool
zapTToneToPCMConverter::GetNextInputFrame()
{
  mCurrentInputFrame = nsnull;
  
  if (!mInput) return PR_FALSE;
  
  nsCOMPtr<zapIMediaFrame> frame;
  if (NS_FAILED(mInput->ProduceFrame(getter_AddRefs(frame))))
    return PR_FALSE;
  NS_ASSERTION(frame, "null frame");
  
  mCurrentInputFrame = do_QueryInterface(frame);
  if (!mCurrentInputFrame) {
    NS_ERROR("incompatible input frame type");
    return PR_FALSE;
  }

  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  NS_ASSERTION(streamInfo, "null stream info!");
  
  if (streamInfo != mCurrentInputStreamInfo) {
    // this is a new stream.
    if (!ZMK_VerifyStreamType(streamInfo, NS_LITERAL_CSTRING("audio/tone")))
      return PR_FALSE;
    
    mCurrentInputStreamInfo = streamInfo;
    // resynchronize our sample clock:
    mCurrentInputFrame->GetTimestamp(&mSampleClock);
  }
  
  return PR_TRUE;
}
