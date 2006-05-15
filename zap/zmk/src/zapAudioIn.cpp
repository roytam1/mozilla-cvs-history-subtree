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

#include "zapAudioIn.h"
#include "stdio.h"
#include "nsString.h"
#include "zapMediaFrame.h"
#include "zapAudioDevice.h"
#include "nsIProxyObjectManager.h"
#include "nsAutoLock.h"

///////////////////////////////////////////////////////////////////////
// zapAudioInEvent

NS_IMETHODIMP
zapAudioInEvent::Run()
{
  if (!mAudioIn) return NS_OK;
  
  mAudioIn->CreateFrame(data, timestamp);
  mAudioIn->AudioInEventDone();
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapAudioIn

zapAudioIn::zapAudioIn()
    : mStream(nsnull),
      mKeepRunning(PR_FALSE)
{
  mCallbackLock = PR_NewLock();
  
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
  
#ifdef DEBUG_afri_zmk
  printf("zapAudioIn::zapAudioIn()\n");
#endif
}

zapAudioIn::~zapAudioIn()
{
  PR_DestroyLock(mCallbackLock);

  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
#ifdef DEBUG_afri_zmk
  printf("zapAudioIn::~zapAudioIn()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioIn)
NS_IMPL_RELEASE(zapAudioIn)

NS_INTERFACE_MAP_BEGIN(zapAudioIn)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIAudioIn)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioIn::AddedToGraph(zapIMediaGraph *graph,
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
  mInputDevice = Pa_GetDefaultInputDeviceID();
  mBuffers = 4;
  // unpack node parameters:
  if (node_pars) {
    nsCOMPtr<zapIPortaudioDevice> device;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(device));
    if (device) {
      device->GetDeviceID(&mInputDevice);
    }
    
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("buffers"),
                                   &mBuffers);
  }
  
  if (mBuffers < 2) mBuffers = 2;
  
#ifdef DEBUG_afri_zmk
  printf("(audioin using device %d)", mInputDevice);
#endif
  if (NS_FAILED(mStreamParameters.InitWithProperties(node_pars)))
    return NS_ERROR_FAILURE;

  // XXX shall we start playing now or later??
  Play();
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioIn::RemovedFromGraph(zapIMediaGraph *graph)
{
#ifdef DEBUG_afri_zmk
  printf("(audioin removed from graph)");
#endif
  Stop();
  mEventTarget = nsnull;
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioIn::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapAudioIn::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("audioin is a source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIn::ConnectSink(zapIMediaSink *sink,
                        const nsACString & connection_id)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIn::DisconnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioIn::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIAudioIn methods:

/* void play (); */
NS_IMETHODIMP
zapAudioIn::Play()
{
  if (!mEventTarget)
    return NS_ERROR_FAILURE;
  
  if (!mStream)
    return StartStream();
  return NS_OK;
}

/* void stop (); */
NS_IMETHODIMP
zapAudioIn::Stop()
{
  if (mStream)
    CloseStream();
  return NS_OK;
}

/* readonly attribute zapIAudioDevice defaultInputDevice; */
NS_IMETHODIMP
zapAudioIn::GetDefaultInputDevice(zapIAudioDevice * *aDefaultInputDevice)
{
  PaDeviceID id = Pa_GetDefaultInputDeviceID();
  if (id == paNoDevice) {
    *aDefaultInputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultInputDevice = CreateAudioDevice(id);
  return NS_OK;
}

//----------------------------------------------------------------------
// portaudio audio source callback:

int AudioInCallback(void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    PaTimestamp outTime, void* userData)
{
#ifdef DEBUG_afri_zmk
//  printf("{");
#endif

  zapAudioIn* audioin = (zapAudioIn*)userData;
  
  nsAutoLock lock(audioin->mCallbackLock);

  if (!audioin->mKeepRunning) {
#ifdef DEBUG_afri_zmk
    printf("(audioin cb stopped)}");
#endif
    return 1; // stop stream
  }
  
  // Post the frame. We need to do this synchronously to pace the callback.

  // XXX We should really enter the event queue's monitor here
  // Unlock the callback lock, so that the stream can be closed while
  // we're waiting for the event. It is important to do this AFTER
  // entering the eventQ monitor but BEFORE posting, to coordinate the
  // request revocation in CloseStream()
  lock.unlock();
  nsRefPtr<zapAudioInEvent> ev = new zapAudioInEvent(audioin);
  PRUint32 bufferLength = audioin->mStreamParameters.GetSamplesPerFrame() * GetPortAudioSampleSize(audioin->mStreamParameters.sample_format);
  ev->data.SetLength(bufferLength);
  memcpy(ev->data.BeginWriting(), inputBuffer, bufferLength);
  ev->timestamp = outTime;
    
  audioin->mEventTarget->Dispatch(ev, NS_DISPATCH_SYNC);
  
#ifdef DEBUG_afri_zmk
//  printf("}");
#endif
  return 0;
}

//----------------------------------------------------------------------
// Implementation helpers:

nsresult zapAudioIn::StartStream()
{
  NS_ASSERTION(!mStream, "stream still running");

  // Create a new streaminfo. We can't just implement nsIPropertyBag2
  // on zapAudioIn, because packets and their associated streaminfo
  // might be buffered elsewhere in the graph while we are already
  // busy on the next stream.
  // No need to lock mCallbackLock; callback isn't running
  mStreamInfo = mStreamParameters.CreateStreamInfo();
    
  mKeepRunning = PR_TRUE;

  PaError err = Pa_OpenStream(&mStream,
                              mInputDevice, mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              paNoDevice, 0, 0, nsnull,
                              mStreamParameters.sample_rate,
                              mStreamParameters.GetSamplesPerFrame()/mStreamParameters.channels,
                              mBuffers,
                              paNoFlag,
                              AudioInCallback, this);

  if (err != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to open portaudio input stream: %s\n",
           Pa_GetErrorText(err));
#endif
    mKeepRunning = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  Pa_StartStream(mStream);
  return NS_OK;
}

void zapAudioIn::CloseStream()
{
  NS_ASSERTION(mStream, "stream not running");

  // make sure that callback will stop the stream if it starts running
  // from now on:
  nsAutoLock lock(mCallbackLock);
  mKeepRunning = PR_FALSE;
  lock.unlock();
  // cancel any outstanding CreateFrame() notifications:
  mEvent.Revoke();

  Pa_CloseStream(mStream);
  mStream = nsnull;
#ifdef DEBUG_afri_zmk
  printf("(audioin stream closed)");
#endif
}

void zapAudioIn::CreateFrame(const nsACString& data, double timestamp)
{
  if (!mOutput) return;
  
  // Create frame:
  nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();

  if (mStreamParameters.sample_format == sf_float32_32768) {
    // convert from int16 to float:
    PRUint32 samplesPerFrame = mStreamParameters.GetSamplesPerFrame();
    frame->mData.SetLength( samplesPerFrame * GetZapAudioSampleSize(mStreamParameters.sample_format));
    float* d = (float*)frame->mData.BeginWriting();
    nsACString::const_iterator p;
    data.BeginReading(p);
    PRInt16* s = (PRInt16*)p.get();
    for (int i=0; i<samplesPerFrame; ++i)
      *d++ = (float)*s++;
  }
  else {
    // portaudio format maps directly onto zmk audio stream format
    frame->mData = data;
  }
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = (PRUint32)timestamp;

  mOutput->ConsumeFrame(frame);
}

void zapAudioIn::AudioInEventDone()
{
  mEvent.Forget();
}
