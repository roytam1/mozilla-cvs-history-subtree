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
#include "nsHashPropertyBag.h"

////////////////////////////////////////////////////////////////////////
// zapAudioIn

zapAudioIn::zapAudioIn()
    : mStream(nsnull),
      mState(zapAudioIn_PLAY_IDLE_CLOSED::Instance()),
      mKeepRunning(PR_FALSE)
{
  mCallbackLock = PR_NewLock();
  
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
  
#ifdef DEBUG
  printf("zapAudioIn::zapAudioIn()\n");
#endif
}

zapAudioIn::~zapAudioIn()
{
  PR_DestroyLock(mCallbackLock);

  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
#ifdef DEBUG
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
  graph->GetEventQueue(getter_AddRefs(mEventQ));
    
  // node parameter defaults:
  mInputDevice = Pa_GetDefaultInputDeviceID();
  // unpack node parameters:
  if (node_pars) {
    nsCOMPtr<zapIPortaudioDevice> device;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(device));
    if (device) {
      device->GetDeviceID(&mInputDevice);
    }
  }
#ifdef DEBUG
  printf("(audioin using device %d)", mInputDevice);
#endif
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioIn::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioIn::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mSink) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }

  // source parameter defaults:
  mSampleRate = 8000; // 8000Hz
  mFrameDuration = 0.02; // 20ms
  mNumChannels = 1; // mono
  mSampleFormat = sf_float32_32768;

  // unpack source parameters:
  if (source_pars) {
    source_pars->GetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                     &mSampleRate);
    source_pars->GetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                     &mFrameDuration);
    source_pars->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                     &mNumChannels);
    nsCString sampleformat_string;
    if (NS_SUCCEEDED(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                        sampleformat_string))) {
      mSampleFormat = StrToZapAudioSampleFormat(sampleformat_string);
      if (mSampleFormat == sf_unknown) {
        NS_ERROR("unknown sample format");
        return NS_ERROR_FAILURE;
      }
    }
  }

  mSamplesPerFrame = (PRUint32)(mSampleRate*mFrameDuration*mNumChannels);
  
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
  return mState->ConnectSink(this, sink, connection_id);
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIn::DisconnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  return mState->DisconnectSink(this, sink, connection_id);
}

/* void requestFrame (); */
NS_IMETHODIMP
zapAudioIn::RequestFrame()
{
  return mState->RequestFrame(this);
}

//----------------------------------------------------------------------
// zapIAudioIn methods:

/* void play (); */
NS_IMETHODIMP
zapAudioIn::Play()
{
  return mState->Play(this);
}

/* void stop (); */
NS_IMETHODIMP
zapAudioIn::Stop()
{
  return mState->Stop(this);
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

class zapAudioInSendEvent : public PLEvent
{
public:
  zapAudioInSendEvent(zapAudioIn* audioin)
  {
    PL_InitEvent(this, audioin, EventHandler, EventCleanup);
  }

  PR_STATIC_CALLBACK(void *) EventHandler(PLEvent* ev)
  {
    zapAudioInSendEvent* rev = (zapAudioInSendEvent*) ev;
    zapAudioIn* audioin = (zapAudioIn*) ev->owner;

    audioin->SendFrame(rev->data, rev->timestamp);
    
    return (void*)PR_TRUE;
  }

  PR_STATIC_CALLBACK(void) EventCleanup(PLEvent* ev)
  {
    delete (zapAudioInSendEvent*) ev;
  }

  nsCString data;
  double timestamp;
};


int AudioInCallback(void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    PaTimestamp outTime, void* userData)
{
#ifdef DEBUG
//  printf("{");
#endif

  zapAudioIn* audioin = (zapAudioIn*)userData;
  
  nsAutoLock lock(audioin->mCallbackLock);

  if (!audioin->mKeepRunning) {
#ifdef DEBUG
    printf("(audioin cb stopped)}");
#endif
    return 1; // stop stream
  }
  
  // Post the frame. We need to do this synchronously to pace the callback.
  audioin->mEventQ->EnterMonitor();
  // Unlock the callback lock, so that the stream can be closed while
  // we're waiting for the event. It is important to do this AFTER
  // entering the eventQ monitor but BEFORE posting, to coordinate the
  // request revocation in CloseStream()
  lock.unlock();
  zapAudioInSendEvent* ev = new zapAudioInSendEvent(audioin);
  PRUint32 bufferLength = audioin->mSamplesPerFrame * GetPortAudioSampleSize(audioin->mSampleFormat);
  ev->data.SetLength(bufferLength);
  memcpy(ev->data.BeginWriting(), inputBuffer, bufferLength);
  ev->timestamp = outTime;
    
  void* result;
  audioin->mEventQ->PostSynchronousEvent(ev, &result);
  audioin->mEventQ->ExitMonitor();
  
#ifdef DEBUG
//  printf("}");
#endif
  return 0;
}

//----------------------------------------------------------------------
// Implementation helpers:

void zapAudioIn::ChangeState(zapAudioInState* state)
{
  mState = state;
}

nsresult zapAudioIn::StartStream()
{
  NS_ASSERTION(!mStream, "stream still running");

  // Create a new streaminfo. We can't just implement nsIPropertyBag2
  // on zapAudioIn, because packets and their associated streaminfo
  // might be buffered elsewhere in the graph while we are already
  // busy on the next stream.
  nsCOMPtr<nsIWritablePropertyBag> bag;
  NS_NewHashPropertyBag(getter_AddRefs(bag));
  mStreamInfo = do_QueryInterface(bag);

  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("type"),
                                     NS_LITERAL_CSTRING("audio"));
  mStreamInfo->SetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                   mSampleRate);
  mStreamInfo->SetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                   mFrameDuration);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                   mNumChannels);

  nsCString format_string;
  ZapAudioSampleFormatToStr(mSampleFormat, format_string);
  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                     format_string);
  // No need to lock mCallbackLock; callback isn't running
  mKeepRunning = PR_TRUE;

  PaError err = Pa_OpenStream(&mStream,
                              mInputDevice, mNumChannels,
                              ZapAudioSampleFormatToPaFormat(mSampleFormat), nsnull,
                              paNoDevice, 0, 0, nsnull,
                              mSampleRate, mSamplesPerFrame/mNumChannels, 1,
                              paNoFlag,
                              AudioInCallback, this);

  if (err != paNoError) {
#ifdef DEBUG
    printf("Failed to open portaudio input stream: %s\n", Pa_GetErrorText(err));
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
  // cancel any outstanding SendFrame() notifications:
  mEventQ->EnterMonitor();
  mEventQ->RevokeEvents(this);
  mEventQ->ExitMonitor();

  Pa_CloseStream(mStream);
  mStream = nsnull;
#ifdef DEBUG
  printf("(audioin stream closed)");
#endif
}

void zapAudioIn::SendFrame(const nsACString& data, double timestamp)
{
  // Create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();

  if (mSampleFormat == sf_float32_32768) {
    // convert from int16 to float:
    nsCString fdata;
    fdata.SetLength(mSamplesPerFrame * GetZapAudioSampleSize(mSampleFormat));
    float* d = (float*)fdata.BeginWriting();
    nsACString::const_iterator p;
    data.BeginReading(p);
    PRInt16* s = (PRInt16*)p.get();
    for (int i=0; i<mSamplesPerFrame; ++i)
      *d++ = (float)*s++;
    frame->mData = fdata;
  }
  else {
    // portaudio format maps directly onto zmk audio stream format
    frame->mData = data;
  }
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = (PRUint32)timestamp;
  
  mState->SendFrame(this, frame);
  frame->Release();
}

////////////////////////////////////////////////////////////////////////
// zapAudioInState implementation

nsresult zapAudioInState::ConnectSink(zapAudioIn* audioin, zapIMediaSink* sink,
                                      const nsACString& connection_id)
{
#ifdef DEBUG
  printf("zapAudioInState::ConnectSink: protocol error in state %s\n", GetName());
#endif
  return NS_ERROR_FAILURE;
}

nsresult zapAudioInState::RequestFrame(zapAudioIn* audioin)
{
#ifdef DEBUG
  printf("zapAudioInState::RequestFrame: protocol error in state %s\n", GetName());
#endif
  return NS_ERROR_FAILURE;
}
  
void zapAudioInState::SendFrame(zapAudioIn* audioin, zapIMediaFrame* frame)
{
#ifdef DEBUG
  printf("zapAudioInState::SendFrame: protocol error in state %s\n", GetName());
#endif
}
  
void zapAudioInState::ChangeState(zapAudioIn* audioin, zapAudioInState* state)
{
#ifdef DEBUG
  if (!((this == zapAudioIn_PLAY_IDLE_OPEN::Instance() &&
         state == zapAudioIn_PLAY_WAITING_OPEN::Instance()) ||
        (this == zapAudioIn_PLAY_WAITING_OPEN::Instance() &&
         state == zapAudioIn_PLAY_IDLE_OPEN::Instance())))
    printf("(audioin %s->%s)", GetName(), state->GetName());
#endif
  audioin->ChangeState(state);
}

////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_IDLE_CLOSED implementation

zapAudioInState* zapAudioIn_STOP_IDLE_CLOSED::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_STOP_IDLE_CLOSED;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_STOP_IDLE_CLOSED::mInstance = nsnull;

nsresult zapAudioIn_STOP_IDLE_CLOSED::Play(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLE_CLOSED::Stop(zapAudioIn* audioin)
{
  // already stopped
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLE_CLOSED::ConnectSink(zapAudioIn* audioin,
                                                  zapIMediaSink* sink,
                                                  const nsACString& connection_id)
{
  NS_ASSERTION(!audioin->mSink, "already connected");
  audioin->mSink = sink;
  // stay in state
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLE_CLOSED::DisconnectSink(zapAudioIn* audioin,
                                                     zapIMediaSink *sink,
                                                     const nsACString & connection_id)
{
  audioin->mSink = nsnull;
  // stay in state
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLE_CLOSED::RequestFrame(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_STOP_WAITING_CLOSED::Instance());
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_IDLE_CLOSED implementation

zapAudioInState* zapAudioIn_PLAY_IDLE_CLOSED::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_PLAY_IDLE_CLOSED;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_PLAY_IDLE_CLOSED::mInstance = nsnull;

nsresult zapAudioIn_PLAY_IDLE_CLOSED::Play(zapAudioIn* audioin)
{
  // play already pending
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_CLOSED::Stop(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_STOP_IDLE_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_CLOSED::ConnectSink(zapAudioIn* audioin,
                                                  zapIMediaSink* sink,
                                                  const nsACString& connection_id)
{
  NS_ASSERTION(!audioin->mSink, "already connected");
  audioin->mSink = sink;
  // stay in state
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_CLOSED::DisconnectSink(zapAudioIn* audioin,
                                                     zapIMediaSink *sink,
                                                     const nsACString & connection_id)
{
  audioin->mSink = nsnull;
  // stay in state
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_CLOSED::RequestFrame(zapAudioIn* audioin)
{
  if (NS_FAILED(audioin->StartStream())) {
    ChangeState(audioin, zapAudioIn_STOP_WAITING_CLOSED::Instance());
  }
  else {
    // success. portaudio callback is running.
    // sink is waiting for frame.
    ChangeState(audioin, zapAudioIn_PLAY_WAITING_OPEN::Instance());
  }
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_IDLE_OPEN implementation

zapAudioInState* zapAudioIn_PLAY_IDLE_OPEN::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_PLAY_IDLE_OPEN;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_PLAY_IDLE_OPEN::mInstance = nsnull;

nsresult zapAudioIn_PLAY_IDLE_OPEN::Play(zapAudioIn* audioin)
{
  // already playing
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_OPEN::Stop(zapAudioIn* audioin)
{
  audioin->CloseStream();
  // make sure sink gets eof frame:
  ChangeState(audioin, zapAudioIn_STOP_IDLEEOF_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_OPEN::DisconnectSink(zapAudioIn* audioin,
                                                   zapIMediaSink *sink,
                                                   const nsACString & connection_id)
{
  audioin->CloseStream();
  audioin->mSink = nsnull;
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLE_OPEN::RequestFrame(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_PLAY_WAITING_OPEN::Instance());
  return NS_OK;
}

void zapAudioIn_PLAY_IDLE_OPEN::SendFrame(zapAudioIn* audioin,
                                          zapIMediaFrame* frame)
{
  // Sink is not waiting. We have buffer overflow.
#ifdef DEBUG
  printf("O");
#endif
  // Silently discard frame
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_WAITING_CLOSED implementation

zapAudioInState* zapAudioIn_STOP_WAITING_CLOSED::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_STOP_WAITING_CLOSED;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_STOP_WAITING_CLOSED::mInstance = nsnull;

nsresult zapAudioIn_STOP_WAITING_CLOSED::Play(zapAudioIn* audioin)
{
  if (NS_FAILED(audioin->StartStream())) {
    ChangeState(audioin, zapAudioIn_STOP_WAITING_CLOSED::Instance());
  }
  else {
    // success. portaudio callback is running.
    // sink is waiting for frame.
    ChangeState(audioin, zapAudioIn_PLAY_WAITING_OPEN::Instance());
  }
  return NS_OK;
}

nsresult zapAudioIn_STOP_WAITING_CLOSED::Stop(zapAudioIn* audioin)
{
  // already stopped
  return NS_OK;
}

nsresult zapAudioIn_STOP_WAITING_CLOSED::DisconnectSink(zapAudioIn* audioin,
                                                        zapIMediaSink *sink,
                                                        const nsACString & connection_id)
{
  audioin->mSink = nsnull;
  ChangeState(audioin, zapAudioIn_STOP_IDLE_CLOSED::Instance());
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_IDLEEOF_CLOSED implementation

zapAudioInState* zapAudioIn_STOP_IDLEEOF_CLOSED::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_STOP_IDLEEOF_CLOSED;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_STOP_IDLEEOF_CLOSED::mInstance = nsnull;

nsresult zapAudioIn_STOP_IDLEEOF_CLOSED::Play(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_PLAY_IDLEEOF_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLEEOF_CLOSED::Stop(zapAudioIn* audioin)
{
  // already stopped
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLEEOF_CLOSED::DisconnectSink(zapAudioIn* audioin,
                                                        zapIMediaSink *sink,
                                                        const nsACString & connection_id)
{
  audioin->mSink = nsnull;
  ChangeState(audioin, zapAudioIn_STOP_IDLE_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_STOP_IDLEEOF_CLOSED::RequestFrame(zapAudioIn* audioin)
{
  // send eof frame:
  audioin->mSink->ProcessFrame(nsnull);
  ChangeState(audioin, zapAudioIn_STOP_IDLE_CLOSED::Instance());
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_IDLEEOF_CLOSED implementation

zapAudioInState* zapAudioIn_PLAY_IDLEEOF_CLOSED::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_PLAY_IDLEEOF_CLOSED;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_PLAY_IDLEEOF_CLOSED::mInstance = nsnull;

nsresult zapAudioIn_PLAY_IDLEEOF_CLOSED::Play(zapAudioIn* audioin)
{
  // play already pending
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLEEOF_CLOSED::Stop(zapAudioIn* audioin)
{
  ChangeState(audioin, zapAudioIn_STOP_IDLEEOF_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLEEOF_CLOSED::DisconnectSink(zapAudioIn* audioin,
                                                        zapIMediaSink *sink,
                                                        const nsACString & connection_id)
{
  audioin->mSink = nsnull;
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_CLOSED::Instance());
  return NS_OK;
}

nsresult zapAudioIn_PLAY_IDLEEOF_CLOSED::RequestFrame(zapAudioIn* audioin)
{
  // send eof frame:
  audioin->mSink->ProcessFrame(nsnull);
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_CLOSED::Instance());
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_WAITING_OPEN implementation

zapAudioInState* zapAudioIn_PLAY_WAITING_OPEN::Instance()
{
  if (!mInstance) {
    mInstance = new zapAudioIn_PLAY_WAITING_OPEN;
  }
  return mInstance;
}

zapAudioInState* zapAudioIn_PLAY_WAITING_OPEN::mInstance = nsnull;

nsresult zapAudioIn_PLAY_WAITING_OPEN::Play(zapAudioIn* audioin)
{
  // already playing
  return NS_OK;
}

nsresult zapAudioIn_PLAY_WAITING_OPEN::Stop(zapAudioIn* audioin)
{
  audioin->CloseStream();
  ChangeState(audioin, zapAudioIn_STOP_IDLE_CLOSED::Instance());
  // send eof frame:
  audioin->mSink->ProcessFrame(nsnull);
  return NS_OK;
}

nsresult zapAudioIn_PLAY_WAITING_OPEN::DisconnectSink(zapAudioIn* audioin,
                                                      zapIMediaSink *sink,
                                                      const nsACString & connection_id)
{
  audioin->CloseStream();
  audioin->mSink = nsnull;
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_CLOSED::Instance());
  return NS_OK;
}

void zapAudioIn_PLAY_WAITING_OPEN::SendFrame(zapAudioIn* audioin,
                                             zapIMediaFrame* frame)
{
  ChangeState(audioin, zapAudioIn_PLAY_IDLE_OPEN::Instance());
  audioin->mSink->ProcessFrame(frame);
}


