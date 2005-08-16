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
#include "nsAutoLock.h"

////////////////////////////////////////////////////////////////////////
// zapAudioOut

zapAudioOut::zapAudioOut()
    : mStream(nsnull),
      mWaiting(PR_FALSE)
{
  mLock = PR_NewLock();
  
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
  
#ifdef DEBUG
  printf("zapAudioOut::zapAudioOut()\n");
#endif
}

zapAudioOut::~zapAudioOut()
{
  PR_DestroyLock(mLock);
  
  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
#ifdef DEBUG
  printf("zapAudioOut::~zapAudioOut()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioOut)
NS_IMPL_RELEASE(zapAudioOut)

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
  graph->GetEventQueue(getter_AddRefs(mEventQ));

  // node parameter defaults:
  mOutputDevice = Pa_GetDefaultOutputDeviceID();
  // unpack node parameters:
  if (node_pars) {
    nsCOMPtr<zapIPortaudioDevice> device;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(device));
    if (device)
      device->GetDeviceID(&mOutputDevice);
  }
#ifdef DEBUG
  printf("(audioout using device %d)", mOutputDevice);
#endif
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioOut::RemovedFromGraph(zapIMediaGraph *graph)
{
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
  if (mSource) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

//----------------------------------------------------------------------
// portaudio audio sink callback:

class zapAudioOutRequestEvent : public PLEvent
{
public:
  zapAudioOutRequestEvent(zapAudioOut* audioout)
  {
    PL_InitEvent(this, audioout, EventHandler, EventCleanup);
  }

  PR_STATIC_CALLBACK(void *) EventHandler(PLEvent* ev)
  {
    zapAudioOutRequestEvent* rev = (zapAudioOutRequestEvent*) ev;
    zapAudioOut* audioout = (zapAudioOut*) ev->owner;

    nsAutoLock lock(audioout->mLock);
    NS_ASSERTION(audioout->mSource, "source gone");
    NS_ASSERTION(!audioout->mWaiting, "protocol error");
    audioout->mWaiting = PR_TRUE;
    audioout->mFrame = nsnull;
    lock.unlock();
    audioout->mSource->RequestFrame();
    return (void*)PR_TRUE;
  }

  PR_STATIC_CALLBACK(void) EventCleanup(PLEvent* ev)
  {
    delete (zapAudioOutRequestEvent*) ev;
  }
};

int AudioOutCallback(void* inputBuffer, void* outputBuffer,
                     unsigned long framesPerBuffer,
                     PaTimestamp outTime, void* userData)
{
#ifdef DEBUG  
//  printf("[");
#endif

  PRBool stopStream = PR_FALSE;
  zapAudioOut* audioout = (zapAudioOut*)userData;
  
  nsAutoLock lock(audioout->mLock);

  if (audioout->mWaiting) {
    // underflow: we are still waiting for the next frame.
#ifdef DEBUG
    printf("U");
#endif
    // Generate a silence buffer
    // XXX in the case of Float32 samples, this requires a sane
    // floating point representation where zero is 0 0 0 0.
    if (audioout->mSampleFormat == sf_float32_32768) {
      // output buffer is int16:
      memset(outputBuffer, 0, audioout->mSamplesPerFrame*2);
    }
    else {
      memset(outputBuffer, 0, audioout->mSamplesPerFrame * GetZapAudioSampleSize(audioout->mSampleFormat));
    }
  }
  else {
    if (audioout->mFrame) {
      // We've got a frame. Consume it.
#ifdef DEBUG
//       PRUint32 timestamp;
//       audioout->mFrame->GetTimestamp(&timestamp);
//       printf("<%d>", timestamp);
#endif
      nsCString data;
      audioout->mFrame->GetData(data);
      NS_ASSERTION(data.Length() == audioout->mSamplesPerFrame * GetZapAudioSampleSize(audioout->mSampleFormat),
                   "buffer length mismatch");
      PRUint32 l = PR_MIN(data.Length(), audioout->mSamplesPerFrame * GetZapAudioSampleSize(audioout->mSampleFormat));
      if (audioout->mSampleFormat == sf_float32_32768) {
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
      
      // Request next frame on the media thread. We need to do this
      // synchronously because the portaudio callback is sometimes
      // called in quick succession without giving the media thread a
      // chance to run.
      audioout->mEventQ->EnterMonitor();
      zapAudioOutRequestEvent* ev = new zapAudioOutRequestEvent(audioout);
      void* result = nsnull;

      // Unlock mLock. This must be done after we've entered the
      // eventQ monitor to synchronize ourselves with event revocation
      // in the case of stream closure.
      lock.unlock();
      audioout->mEventQ->PostSynchronousEvent(ev, &result);
      audioout->mEventQ->ExitMonitor();
      if (!result) {
        // the event was cancelled
#ifdef DEBUG
        printf("(audioout stop1)");
#endif
        stopStream = PR_TRUE;
      }
    }
    else {
      // EOF
#ifdef DEBUG
      printf("(audioout stop2)");
#endif
      stopStream = PR_TRUE;
    }
  }

#ifdef DEBUG
//  printf("]");
#endif
  
  return stopStream;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioOut::ConnectSource(zapIMediaSource *source,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mSource, "already connected");
  NS_ASSERTION(!mWaiting, "protocol error");
  NS_ASSERTION(!mStream, "protocol error");

  mSource = source;

  // request first buffer frame:
  mWaiting = PR_TRUE;
  mSource->RequestFrame();

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioOut::DisconnectSource(zapIMediaSource *source,
                              const nsACString & connection_id)
{
  mSource = nsnull;
  if (mStream) {
    CloseStream();
  }
  mWaiting = PR_FALSE;
  return NS_OK;
}

class zapAudioOutCloseEvent : public PLEvent
{
public:
  zapAudioOutCloseEvent(zapAudioOut* _audioout)
      : audioout(_audioout)
  {
    PL_InitEvent(this, audioout, EventHandler, EventCleanup);
  }

  PR_STATIC_CALLBACK(void *) EventHandler(PLEvent* ev)
  {
    zapAudioOutCloseEvent* rev = (zapAudioOutCloseEvent*) ev;

    rev->audioout->CloseStream();
    return (void*)PR_TRUE;
  }

  PR_STATIC_CALLBACK(void) EventCleanup(PLEvent* ev)
  {
    delete (zapAudioOutCloseEvent*) ev;
  }

  // reference to keep alive the audioout object while the stream is
  // being closed:
  nsRefPtr<zapAudioOut> audioout;
};


/* void processFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioOut::ProcessFrame(zapIMediaFrame *frame)
{
  nsAutoLock lock(mLock);
  NS_ASSERTION(mWaiting, "protocol error");
  mWaiting = PR_FALSE;
  mFrame = frame;

  if (mStream && !frame) {
    // Schedule stream closure. The call needs to be asynchronous
    // since this ProcessFrame call might be nested in the callback's
    // RequestFrame call:
    mEventQ->EnterMonitor();
    zapAudioOutCloseEvent* ev = new zapAudioOutCloseEvent(this);
    mEventQ->PostEvent(ev);
    mEventQ->ExitMonitor();
  }
  else if (!mStream) {
    if (frame) {
      nsCOMPtr<nsIPropertyBag2> streamInfo;
      frame->GetStreamInfo(getter_AddRefs(streamInfo));
      if (NS_FAILED(StartStream(streamInfo))) {
#ifdef DEBUG
        NS_ERROR("Error opening stream");
#endif
      }
    }
    else {
      // an EOF frame. request next stream:
      mWaiting = PR_TRUE;
			lock.unlock();
      mSource->RequestFrame();
    }
  }
  
  return NS_OK;
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
// Implementation helpers:

nsresult zapAudioOut::StartStream(nsIPropertyBag2* streamInfo)
{
  NS_ASSERTION(!mStream, "stream still running");

  if (!streamInfo) {
    NS_ERROR("missing stream info");
    return NS_ERROR_FAILURE;
  }
  
  nsCString type;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                 type)) ||
      type != NS_LITERAL_CSTRING("audio")) {
    NS_ERROR("unsupported stream type");
    return NS_ERROR_FAILURE;
  }
  
  double sampleRate;
  if (NS_FAILED(streamInfo->GetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                                &sampleRate))) {
    NS_ERROR("unknown sample rate");
    return NS_ERROR_FAILURE;
  }
  
  double frameDuration;
  if (NS_FAILED(streamInfo->GetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                                &frameDuration))) {
    NS_ERROR("unknown frame duration");
    return NS_ERROR_FAILURE;
  }
  
  PRUint32 numOutputChannels;
  if (NS_FAILED(streamInfo->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                                &numOutputChannels))) {
    NS_ERROR("unknown output channel count");
    return NS_ERROR_FAILURE;
  }

  nsCString format_string;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                  format_string))) {
    NS_ERROR("unknown sample format");
    return NS_ERROR_FAILURE;
  }
  mSampleFormat = StrToZapAudioSampleFormat(format_string);
  if (mSampleFormat == sf_unknown) {
    NS_ERROR("unknown sample format");
    return NS_ERROR_FAILURE;
  }
    
  mSamplesPerFrame = (PRUint32)(sampleRate*frameDuration*numOutputChannels);

  // try to open stream:
  PaError err = Pa_OpenStream(&mStream,
                              paNoDevice, 0, 0, nsnull,
                              mOutputDevice, numOutputChannels,
                              ZapAudioSampleFormatToPaFormat(mSampleFormat), nsnull,
                              sampleRate, mSamplesPerFrame/numOutputChannels, 0,
                              paNoFlag,
                              AudioOutCallback, this);
  
  if (err != paNoError) {
#ifdef DEBUG
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

  // XXX Note: If the stream is not stopped before closing,
  // Pa_CloseStream() will block the media thread until the next
  // callback. More gravely, if the stream is closed while in the
  // callback, portaudio will crash.  The first issue we can't prevent
  // easily. To prevent the second issue we must revoke any pending
  // events from the callback that prevent it from completing while
  // the media thread is not pumping events:

  // Ensure that if the portaudio callback is called now, it will see
  // an EOF frame and stop the stream rather than posting a new
  // request:
  nsAutoLock lock(mLock);
  mFrame = nsnull;
  mWaiting = PR_FALSE;
  lock.unlock();
  
  // Depending on when the callback ran we might still have an old
  // callback waiting for a pending frame request. By cancelling all
  // requests we ensure that it can run to completion (also stopping
  // the stream):
  mEventQ->EnterMonitor();
  mEventQ->RevokeEvents(this);
  mEventQ->ExitMonitor();
  
  // At this point we have made sure that the portaudio callback can
  // run to completion and it is safe to close the
  // stream. Pa_CloseStream() will wait for the callback to complete,
  // blocking the media thread.
    
  Pa_CloseStream(mStream);
  mStream = nsnull;
#ifdef DEBUG
  printf("(audioout stream closed)");
#endif

  if (mSource) {
    // we still have a source. request first frame of next stream:
    mWaiting = PR_TRUE;
    mSource->RequestFrame();
  }
    
}

