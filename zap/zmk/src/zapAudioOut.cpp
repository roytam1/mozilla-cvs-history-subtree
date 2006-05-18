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
#include "nsString.h"
#include "zapMediaFrame.h"
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
    if (!mAudioOut) return NS_OK;

    mAudioOut->PlayFrame(mOutputBuffer);

    return NS_OK;
  }
  
private:
  nsRefPtr<zapAudioOut> mAudioOut;
  void *mOutputBuffer;
};



////////////////////////////////////////////////////////////////////////
// zapAudioOut

zapAudioOut::zapAudioOut()
    : mStream(nsnull)
{
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
  
#ifdef DEBUG_afri_zmk
  printf("zapAudioOut::zapAudioOut()\n");
#endif
}

zapAudioOut::~zapAudioOut()
{
  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
#ifdef DEBUG_afri_zmk
  printf("zapAudioOut::~zapAudioOut()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioOut)
NS_IMPL_THREADSAFE_RELEASE(zapAudioOut)

NS_INTERFACE_MAP_BEGIN(zapAudioOut)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIAudioOut)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioOut::AddedToGraph(zapIMediaGraph *graph,
                          const nsACString & id,
                          nsIPropertyBag2* node_pars)
{
  // Add a reference to the mediagraph. We must hang onto this
  // reference until the node is destroyed so that any proxies on us
  // have a context to shut down in even after the graph has shut
  // down.
  mGraph = graph;
  
  graph->GetEventTarget(getter_AddRefs(mEventTarget));

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
  
#ifdef DEBUG_afri_zmk
  printf("(audioout using device %d)", mOutputDevice);
#endif
  
  return mStreamParameters.InitWithProperties(node_pars);
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioOut::RemovedFromGraph(zapIMediaGraph *graph)
{
#ifdef DEBUG_afri_zmk
  printf("(audioout removed from graph)");
#endif
  CloseStream();
  mEventTarget = nsnull;
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioOut::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  NS_ERROR("audioout is a sink-only node");
  return NS_ERROR_FAILURE;
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

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioOut::ConnectSource(zapIMediaSource *source,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");
  NS_ASSERTION(!mStream, "protocol error");

  mInput = source;
  StartStream();
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioOut::DisconnectSource(zapIMediaSource *source,
                              const nsACString & connection_id)
{
  mInput = nsnull;
  CloseStream();
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
  
  zapAudioOutPlayFrameEvent* ev = new zapAudioOutPlayFrameEvent(audioout,
                                                                outputBuffer);
  //XXX possible deadlock when stream is closed while we wait for
  //event to be served???
  audioout->mEventTarget->Dispatch(ev, NS_DISPATCH_SYNC);

  return 0;
}

//----------------------------------------------------------------------
// Implementation helpers:

// called from portaudio callback:
void zapAudioOut::PlayFrame(void* outputBuffer)
{
  nsCOMPtr<zapIMediaFrame> frame;
  if (mInput)
    mInput->ProduceFrame(getter_AddRefs(frame));

  if (!frame || !ValidateFrame(frame)) {
    // undeflow or incompatible frame
#ifdef DEBUG_afri_zmk
//    printf("U");
#endif
    // Generate a silence buffer
    // XXX in the case of Float32 samples, this requires a sane
    // floating point representation where zero is 0 0 0 0.
    if (mStreamParameters.sample_format == sf_float32_32768) {
      // output buffer is int16:
      memset(outputBuffer, 0, mStreamParameters.GetSamplesPerFrame()*2);
    }
    else {
      memset(outputBuffer, 0, mStreamParameters.GetSamplesPerFrame() *
                              GetZapAudioSampleSize(mStreamParameters.sample_format));
    }
  }
  else {
    // we've got a frame. Consume it.
#ifdef DEBUG_afri_zmk
//       PRUint32 timestamp;
//       audioout->mFrame->GetTimestamp(&timestamp);
//       printf("<%d>", timestamp);
#endif
    nsCString data;
    frame->GetData(data);
    NS_ASSERTION(data.Length() == mStreamParameters.GetSamplesPerFrame() *
                 GetZapAudioSampleSize(mStreamParameters.sample_format),
                 "buffer length mismatch");
    PRUint32 l = PR_MIN(data.Length(),
                        mStreamParameters.GetSamplesPerFrame() *
                        GetZapAudioSampleSize(mStreamParameters.sample_format));
    if (mStreamParameters.sample_format == sf_float32_32768) {
      // convert to int16
      float* s = (float*)data.BeginReading();
      PRInt16* d = (PRInt16*)outputBuffer;
      for (int i=0; i<l/4; ++i)
        *d++ = (PRInt16)*s++;
    }
    else {
      // data is already in portaudio compatible format:
      memcpy(outputBuffer, data.BeginReading(), l);
    }
  }
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
#ifdef DEBUG_afri_zmk
    printf("(aout incomp. frames!)");
#endif
    return PR_FALSE;
  }
  
  mLastValidStreamInfo = streamInfo;
  return  PR_TRUE;
}

nsresult zapAudioOut::StartStream()
{
  NS_ASSERTION(!mStream, "stream still running");

  // try to open stream:
  PaError err = Pa_OpenStream(&mStream,
                              paNoDevice, 0, 0, nsnull,
                              mOutputDevice, mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              mStreamParameters.sample_rate,
                              mStreamParameters.GetSamplesPerFrame()/mStreamParameters.channels,
                              mBuffers,
                              paNoFlag,
                              AudioOutCallback, this);
  
  if (err != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to open portaudio output stream: %s\n", Pa_GetErrorText(err));
#endif
    return NS_ERROR_FAILURE;
  }
  
  // start playing stream:
  Pa_StartStream(mStream);

  return NS_OK;
}

void zapAudioOut::CloseStream()
{
  if (!mStream) return; // stream already stopped

  Pa_CloseStream(mStream);
  mStream = nsnull;
#ifdef DEBUG_afri_zmk
  printf("(audioout stream closed)");
#endif
}

