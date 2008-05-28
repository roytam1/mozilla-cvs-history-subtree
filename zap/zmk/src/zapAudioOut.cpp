/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "zapAudioOut.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "zapMediaFrame.h"
#include "zapMediaUtils.h"
#include "zapAudioDevice.h"
#include "nsIProxyObjectManager.h"
#include "nsThreadUtils.h"

////////////////////////////////////////////////////////////////////////
// zapAudioOutPlayFrameEvent

class zapAudioOutPlayFrameEvent : public nsRunnable
{
public:
  zapAudioOutPlayFrameEvent(zapAudioOut* audioout, void *buf)
      : mAudioOut(audioout),
        mOutputBuffer(buf)
  {
  }
  
  NS_IMETHODIMP Run()
  {
    NS_ASSERTION(mAudioOut, "uh-oh, no audio out!");
    return mAudioOut->PlayFrame(mOutputBuffer);
  }
  
private:
  nsRefPtr<zapAudioOut> mAudioOut;
  void *mOutputBuffer;
};



////////////////////////////////////////////////////////////////////////
// zapAudioOut

zapAudioOut::zapAudioOut()
    : mStream(nsnull),
      mClockMS(0),
      mSampleClockRemainder(0)
{
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
    NS_WARNING("Failed to initialize portaudio");
  }  
}

zapAudioOut::~zapAudioOut()
{
  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioOut)
NS_IMPL_THREADSAFE_RELEASE(zapAudioOut)

NS_INTERFACE_MAP_BEGIN(zapAudioOut)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIAudioOut)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioOut::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                   nsIPropertyBag2* node_pars)
{
  mContainer = container;
  
  container->GetEventTarget(getter_AddRefs(mEventTarget));

  // node parameter defaults:
  mOutputDevice = Pa_GetDefaultOutputDeviceID();
  mBuffers = 4;
  // unpack node parameters:
  if (node_pars) {
    nsCOMPtr<zapIPortaudioDevice> device;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(device));
    if (device)
      device->GetDeviceID(&mOutputDevice);

    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("buffers"),
                                   &mBuffers);
  }

  if (mBuffers < 2) mBuffers = 2;
  
  nsresult rv =  mStreamParameters.InitWithProperties(node_pars);
  if (NS_FAILED(rv)) return rv;

  ZMK_CREATE_STREAM_INFO(mClockStreamInfo, "clock");
  mClockStreamInfo->SetPropertyAsDouble(NS_LITERAL_STRING("clock_cycle"),
                                        mStreamParameters.GetFrameDuration() );
  
  StartStream();
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapAudioOut::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioOut::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mClockOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapAudioOut::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (mInput) {
    NS_ERROR("input end already connected");
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
zapAudioOut::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapAudioOut::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioOut::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapAudioOut::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mClockOutput, "already connected");
  mClockOutput = sink;
  
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapAudioOut::DisconnectSink(zapIMediaSink *sink)
{
  mClockOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioOut::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("not a passive source!");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIAudioOut methods:

/* readonly attribute zapIAudioDevice defaultOutputDevice; */
NS_IMETHODIMP
zapAudioOut::GetDefaultOutputDevice(zapIAudioDevice * *aDefaultOutputDevice)
{
  PaDeviceID id = Pa_GetDefaultOutputDeviceID();
  if (id == paNoDevice) {
    *aDefaultOutputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultOutputDevice = CreateAudioDevice(id);
  return NS_OK;
}

//----------------------------------------------------------------------
// portaudio audio sink callback:

int AudioOutCallback(void* inputBuffer, void* outputBuffer,
                     unsigned long framesPerBuffer,
                     PaTimestamp outTime, void* userData)
{
  zapAudioOut* audioout = (zapAudioOut*)userData;

  if (audioout->mRefCnt == 1) {
    // audioout is only being held alive by the portaudiostream.
    // -> stop stream and release, so that we can shutdown
    audioout->mStream = nsnull;
    audioout->Release();
    return 1; // 1 == stop stream
  }
  
  nsRefPtr<zapAudioOutPlayFrameEvent> ev = new zapAudioOutPlayFrameEvent(audioout,
                                                                         outputBuffer);

  audioout->mEventTarget->Dispatch(ev, NS_DISPATCH_SYNC);
  
  return 0; // 0 == continue stream
}

//----------------------------------------------------------------------
// Implementation helpers:

// called from portaudio callback:
nsresult zapAudioOut::PlayFrame(void* outputBuffer)
{
  if (!mStream)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<zapIMediaFrame> frame;
  if (mInput)
    mInput->ProduceFrame(getter_AddRefs(frame));
    
  if (!frame || !ValidateFrame(frame)) {
    // undeflow or incompatible frame

    // Generate a silence buffer
    // XXX in the case of Float32 samples, this requires a sane
    // floating point representation where zero is 0 0 0 0.
    if (mStreamParameters.sample_format == sf_float32_32768) {
      // output buffer is int16:
      memset(outputBuffer, 0, mStreamParameters.samples*mStreamParameters.channels*2);
    }
    else {
      memset(outputBuffer, 0, mStreamParameters.GetFrameLength());
    }
  }
  else {
    // we've got a frame. Consume it.
    nsCString data;
    frame->GetData(data);
    
    NS_ASSERTION(data.Length() == mStreamParameters.GetFrameLength(), "buffer length mismatch");
    PRUint32 l = PR_MIN(data.Length(), mStreamParameters.GetFrameLength());
    
    if (mStreamParameters.sample_format == sf_float32_32768) {
      // convert to int16
      float* s = (float*)data.BeginReading();
      PRInt16* d = (PRInt16*)outputBuffer;
      for (unsigned int i=0; i<l/4; ++i)
        *d++ = (PRInt16)*s++;
    }
    else {
      // data is already in portaudio compatible format:
      memcpy(outputBuffer, data.BeginReading(), l);
    }
  }

  mClockMS += (mStreamParameters.samples + mSampleClockRemainder) * 1000 / mStreamParameters.sample_rate;
  mSampleClockRemainder = (mStreamParameters.samples + mSampleClockRemainder) * 1000 % mStreamParameters.sample_rate;
  
  if (mClockOutput) {
    nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();
    frame->mStreamInfo = mClockStreamInfo;
    frame->mTimestamp = mClockMS;
    mClockOutput->ConsumeFrame(frame);
  }

  return NS_OK;
}

PRBool zapAudioOut::ValidateFrame(zapIMediaFrame* frame)
{
  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  if (streamInfo == mLastValidStreamInfo)
    return PR_TRUE;

  // we have a new stream info. Check if the stream is compatible with
  // us:
  if (!CheckAudioStream(streamInfo, mStreamParameters)) {
    return PR_FALSE;
  }
  
  mLastValidStreamInfo = streamInfo;
  return  PR_TRUE;
}

nsresult zapAudioOut::StartStream()
{
  if (mStream) return NS_OK; // stream already running

  // try to open stream:
  PaError err = Pa_OpenStream(&mStream,
                              paNoDevice, 0, 0, nsnull,
                              mOutputDevice, mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              mStreamParameters.sample_rate,
                              mStreamParameters.samples,
                              mBuffers,
                              paNoFlag,
                              AudioOutCallback, this);
  
  if (err != paNoError) {
    NS_WARNING("Failed to open portaudio output stream");
    return NS_ERROR_FAILURE;
  }

  // AddRef ourselves. Portaudio will close stream and Release ref
  // when there is only one reference left
  AddRef();
  
  // start playing stream:
  Pa_StartStream(mStream);

  return NS_OK;
}



