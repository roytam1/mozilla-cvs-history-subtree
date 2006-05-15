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

#include "zapAudioIO.h"
#include "stdio.h"
#include "nsString.h"
#include "zapMediaFrame.h"
#include "zapAudioDevice.h"
#include "nsIProxyObjectManager.h"

////////////////////////////////////////////////////////////////////////
// zapAudioIOMonitor

class zapAudioIOMonitor : public zapIMediaSource
{
public:
  zapAudioIOMonitor();
  ~zapAudioIOMonitor();

  void Init(zapAudioIO* audioIO);
  void ConsumeFrame(const nsACString& data);

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE

private:
  PRUint32 mSampleClock;
  nsCOMPtr<nsIWritablePropertyBag2> mOutputStreamInfo;
  nsCOMPtr<zapIMediaSink> mOutput;
  nsRefPtr<zapAudioIO> mAudioIO;
};

//----------------------------------------------------------------------

zapAudioIOMonitor::zapAudioIOMonitor()
    : mSampleClock(0)
{
}

zapAudioIOMonitor::~zapAudioIOMonitor()
{
  // clean up reference:
  if (mAudioIO) {
    mAudioIO->mAudioIOMonitor = nsnull;
  }
}

void zapAudioIOMonitor::Init(zapAudioIO* audioIO) {
  mAudioIO = audioIO;
  mAudioIO->mAudioIOMonitor = this;
  mOutputStreamInfo = mAudioIO->mStreamParameters.CreateStreamInfo();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioIOMonitor)
NS_IMPL_RELEASE(zapAudioIOMonitor)

NS_INTERFACE_MAP_BEGIN(zapAudioIOMonitor)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIOMonitor::ConnectSink(zapIMediaSink *sink,
                               const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIOMonitor::DisconnectSink(zapIMediaSink *sink,
                                  const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioIOMonitor::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("not a passive source!");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

void zapAudioIOMonitor::ConsumeFrame(const nsACString& data) {
  mSampleClock += mAudioIO->mStreamParameters.GetSamplesPerFrame();

  if (!mOutput) return;

  // create frame:
  nsRefPtr<zapMediaFrame> oframe = new zapMediaFrame();

  oframe->mStreamInfo = mOutputStreamInfo;
  oframe->mTimestamp = mSampleClock;
  oframe->mData = data;

  mOutput->ConsumeFrame(oframe);
}
  
////////////////////////////////////////////////////////////////////////
// zapAudioIO

zapAudioIO::zapAudioIO()
    : mStream(nsnull),
      mAudioIOMonitor(nsnull),
      mKeepRunning(PR_FALSE)
{
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
  
#ifdef DEBUG_afri_zmk
  printf("zapAudioIO::zapAudioIO()\n");
#endif
}

zapAudioIO::~zapAudioIO()
{
  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
#ifdef DEBUG_afri_zmk
  printf("zapAudioIO::~zapAudioIO()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioIO)
NS_IMPL_RELEASE(zapAudioIO)

NS_INTERFACE_MAP_BEGIN(zapAudioIO)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIAudioIO)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioIO::AddedToGraph(zapIMediaGraph *graph,
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
  mOutputDevice = Pa_GetDefaultOutputDeviceID();
  mBuffers = 4;
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("buffers"),
                                   &mBuffers);
    if (mBuffers < 2) mBuffers = 2;

    nsCOMPtr<zapIPortaudioDevice> idevice;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("input_device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(idevice));
    if (idevice)
      idevice->GetDeviceID(&mInputDevice);
    
    nsCOMPtr<zapIPortaudioDevice> odevice;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("output_device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(odevice));
    if (odevice)
      odevice->GetDeviceID(&mOutputDevice);
  }

  if (NS_FAILED(mStreamParameters.InitWithProperties(node_pars)))
    return NS_ERROR_FAILURE;

  // XXX maybe do this later
  StartStream();

  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioIO::RemovedFromGraph(zapIMediaGraph *graph)
{
#ifdef DEBUG_afri_zmk
  printf("(audioio removed from graph)");
#endif
  CloseStream();
  mEventTarget = nsnull;
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioIO::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  *_retval = nsnull;
  
  if (!source_pars) {
    NS_ERROR("no source pars");
    return NS_ERROR_FAILURE;
  }

  nsCString sourceName;
  NS_ENSURE_SUCCESS(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"),
                                                       sourceName),
                    NS_ERROR_FAILURE);

  if (sourceName == NS_LITERAL_CSTRING("ain")) {
    if (mOutput) {
      NS_ERROR("output end already connected");
      return NS_ERROR_FAILURE;
    }
    *_retval = this;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  else if (sourceName == NS_LITERAL_CSTRING("monitor")) {
    if (mAudioIOMonitor) return NS_ERROR_FAILURE;
    zapAudioIOMonitor* monitor = new zapAudioIOMonitor();
    NS_ADDREF(monitor);
    monitor->Init(this);
    *_retval = monitor;
    return NS_OK;
  }
  // else ...
  return NS_ERROR_FAILURE;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapAudioIO::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapAudioIO::ConnectSource(zapIMediaSource *source,
                          const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIO::DisconnectSource(zapIMediaSource *source,
                             const nsACString & connection_id)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioIO::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioIO::ConnectSink(zapIMediaSink *sink,
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
zapAudioIO::DisconnectSink(zapIMediaSink *sink,
                           const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioIO::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIAudioOut methods:

// -- nothing yet --

//----------------------------------------------------------------------
// portaudio audio sink callback:

class zapAudioIOEvent : public PLEvent
{
public:
  zapAudioIOEvent(zapAudioIO* audioio, double outTime,
                  void *inputBuf, void *outputBuf)
      : inputBuffer(inputBuf),
        outputBuffer(outputBuf),
        timestamp(outTime)
  {
    PL_InitEvent(this, audioio, EventHandler, EventCleanup);
  }

  PR_STATIC_CALLBACK(void *) EventHandler(PLEvent* ev)
  {
    zapAudioIOEvent* rev = (zapAudioIOEvent*) ev;
    zapAudioIO* audioio = (zapAudioIO*) ev->owner;

    if (rev->inputBuffer)
      audioio->CreateOutputFrame(rev->timestamp, rev->inputBuffer);
    if (rev->outputBuffer)
      audioio->ProcessInputFrame(rev->outputBuffer);
    
    return (void*)PR_TRUE;
  }

  PR_STATIC_CALLBACK(void) EventCleanup(PLEvent* ev)
  {
    delete (zapAudioIOEvent*) ev;
  }

  void *outputBuffer;
  void *inputBuffer;
  double timestamp;
};


int AudioIOCallback(void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    PaTimestamp outTime, void* userData)
{
  zapAudioIO* audioio = (zapAudioIO*)userData;
  
  //audioio->mEventQ->EnterMonitor();
  // only post event if we're still supposed to be running:
  // (otherwise we might end in deadlock)
  if (audioio->mKeepRunning) {
    zapAudioIOEvent* ev = new zapAudioIOEvent(audioio, outTime,
                                              inputBuffer, outputBuffer);
    audioio->mEventTarget->Dispatch(ev, NS_DISPATCH_SYNC);
  }
  //audioio->mEventQ->ExitMonitor();

  // stop stream if mKeepRunning is false
  return !mKeepRunning;
}

//----------------------------------------------------------------------
// Implementation helpers:

// called from portaudio callback:
void zapAudioIO::ProcessInputFrame(void* outputBuffer)
{
  nsCOMPtr<zapIMediaFrame> frame;
  if (mInput)
    mInput->ProduceFrame(getter_AddRefs(frame));

  if (!frame || !ValidateInputFrame(frame)) {
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
    if (mAudioIOMonitor) {
      nsCString data;
      data.SetLength( mStreamParameters.GetFrameLength() );
      memset(data.BeginWriting(), 0, mStreamParameters.GetFrameLength());
      mAudioIOMonitor->ConsumeFrame(data);
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
    if (mAudioIOMonitor)
      mAudioIOMonitor->ConsumeFrame(data);
    
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

PRBool zapAudioIO::ValidateInputFrame(zapIMediaFrame* frame)
{
  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  if (streamInfo == mLastValidInputStreamInfo)
    return PR_TRUE;

  // we have a new stream info. Check if the stream is compatible with
  // us:
  if (!CheckAudioStream(streamInfo, mStreamParameters)) {
#ifdef DEBUG_afri_zmk
    printf("(aio incomp. frames!)");
#endif
    return PR_FALSE;
  }
  
  mLastValidInputStreamInfo = streamInfo;
  return  PR_TRUE;
}

void zapAudioIO::CreateOutputFrame(double timestamp, void* inputBuffer)
{
  if (!mOutput) return;

  // Create frame:
  nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();

  PRUint32 samplesPerFrame = mStreamParameters.GetSamplesPerFrame();  
  
  if (mStreamParameters.sample_format == sf_float32_32768) {
    // convert from int16 to float:
    frame->mData.SetLength( samplesPerFrame * GetZapAudioSampleSize(mStreamParameters.sample_format));
    float* d = (float*)frame->mData.BeginWriting();
    PRInt16* s = (PRInt16*)inputBuffer;
    for (int i=0; i<samplesPerFrame; ++i)
      *d++ = (float)*s++;
  }
  else {
    // portaudio format maps directly onto zmk audio stream format
    PRUint32 bufferLength = samplesPerFrame * GetPortAudioSampleSize(mStreamParameters.sample_format);
    frame->mData.SetLength(bufferLength);
    memcpy(frame->mData.BeginWriting(), inputBuffer, bufferLength);
  }
  frame->mStreamInfo = mOutputStreamInfo;
  frame->mTimestamp = (PRUint32)timestamp;
  mOutput->ConsumeFrame(frame);
}


nsresult zapAudioIO::StartStream()
{
  NS_ASSERTION(!mStream, "stream still running");

  // Create a new streaminfo. We can't just implement nsIPropertyBag2
  // on zapAudioIO, because packets and their associated streaminfo
  // might be buffered elsewhere in the graph while we are already
  // busy on the next stream.
  // No need to lock mCallbackLock; callback isn't running
  mOutputStreamInfo = mStreamParameters.CreateStreamInfo();
  
  // try to open stream:
  PaError err = Pa_OpenStream(&mStream,
                              mInputDevice,
                              mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              mOutputDevice,
                              mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              mStreamParameters.sample_rate,
                              mStreamParameters.GetSamplesPerFrame()/mStreamParameters.channels,
                              mBuffers,
                              paNoFlag,
                              AudioIOCallback,
                              this);
  
  if (err != paNoError) {
#ifdef DEBUG_afri_zmk
    printf("Failed to open portaudio io stream: %s\n", Pa_GetErrorText(err));
#endif
    return NS_ERROR_FAILURE;
  }
  
  // start playing stream:
  mKeepRunning = PR_TRUE;
  Pa_StartStream(mStream);

  return NS_OK;
}

void zapAudioIO::CloseStream()
{
  if (!mStream) return; // stream already stopped

  // cancel any outstanding PlayFrame() notifications:
  mEventQ->EnterMonitor();
  mKeepRunning = PR_FALSE;
  mEventQ->RevokeEvents(this);
  mEventQ->ExitMonitor();
  
  Pa_CloseStream(mStream);
  mStream = nsnull;
#ifdef DEBUG_afri_zmk
  printf("(audioio stream closed)");
#endif
}

