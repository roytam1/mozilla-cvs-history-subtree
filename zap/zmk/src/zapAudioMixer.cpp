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

#include "zapAudioMixer.h"
#include "zapIMediaGraph.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "zapIMediaFrame.h"
#include "zapMediaFrame.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////
// zapAudioMixerInput

class zapAudioMixerInput : public zapIMediaSink
{
public:
  zapAudioMixerInput();
  ~zapAudioMixerInput();

  void Init(zapAudioMixer* mixer);
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASINK

  void RequestFrame();
  // consume the current frame, return value of mActive:
  PRBool ConsumeFrame(zapIMediaFrame**frame);
  
private:
  nsRefPtr<zapAudioMixer> mMixer;
  nsCOMPtr<zapIMediaSource> mSource;
  PRBool mWaiting; // are we waiting for the next frame?
  nsCOMPtr<zapIMediaFrame> mFrame; // buffers the next frame
  PRBool mActive; // input connected; stream open
};

//----------------------------------------------------------------------
zapAudioMixerInput::zapAudioMixerInput()
    : mWaiting(PR_FALSE),
      mActive(PR_FALSE)
{
#ifdef DEBUG
  printf("zapAudioMixerInput::zapAudioMixerInput()\n");
#endif
}

zapAudioMixerInput::~zapAudioMixerInput()
{
  NS_ASSERTION(mMixer, "Never initialized");
#ifdef DEBUG
  printf("zapAudioMixerInput::~zapAudioMixerInput()\n");
#endif
  // clean up references:
  mMixer->mInputs.RemoveElement(this);

  // make sure mixer gets EOF frame if this was the last input:
  if (!mMixer->mInputs.Count())
    mMixer->FrameAvailable();
  
  mMixer = nsnull;
}

void zapAudioMixerInput::Init(zapAudioMixer* mixer) {
  mMixer = mixer;
  // append ourselves to the mixer's array of inputs:
  mMixer->mInputs.AppendElement(this);
  mWaiting = mMixer->mWaiting;

}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioMixerInput)
NS_IMPL_RELEASE(zapAudioMixerInput)

NS_INTERFACE_MAP_BEGIN(zapAudioMixerInput)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioMixerInput::ConnectSource(zapIMediaSource *source,
                                  const nsACString & connection_id)
{
  NS_ASSERTION(!mSource, "source already connected");
  mSource = source;
  
  if (mWaiting)
    mSource->RequestFrame();
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapAudioMixerInput::DisconnectSource(zapIMediaSource *source,
                                     const nsACString & connection_id)
{
  mSource = nsnull;
  mActive = PR_FALSE;
  return NS_OK;
}

/* void processFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioMixerInput::ProcessFrame(zapIMediaFrame *frame)
{
  NS_ASSERTION(mWaiting, "uh-oh, unexpectatly received a frame");
  mFrame = frame;
  mWaiting = PR_FALSE;
  if (mFrame)
    mActive = PR_TRUE;
  else
    mActive = PR_FALSE;
  
  mMixer->FrameAvailable();
  return NS_OK;
}

//----------------------------------------------------------------------

void zapAudioMixerInput::RequestFrame()
{
  if (mWaiting) return;
  if (mFrame) {
    mMixer->FrameAvailable();
    return;
  }

  mWaiting = PR_TRUE;
  if (mSource)
    mSource->RequestFrame();
}

PRBool zapAudioMixerInput::ConsumeFrame(zapIMediaFrame** frame)
{
#ifdef DEBUG
  if (mActive && mWaiting)
    printf("Underflow in audiomixer input %p\n", this);
#endif
  *frame = mFrame;
  NS_IF_ADDREF(*frame);
  mFrame = nsnull;
  return mActive;
}

////////////////////////////////////////////////////////////////////////
// zapAudioMixer

zapAudioMixer::zapAudioMixer()
    : mFramesAvailable(PR_FALSE),
      mWaiting(PR_FALSE)
{
#ifdef DEBUG
  printf("zapAudioMixer::zapAudioMixer()\n");
#endif
}

zapAudioMixer::~zapAudioMixer()
{
#ifdef DEBUG
  printf("zapAudioMixer::~zapAudioMixer()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioMixer)
NS_IMPL_RELEASE(zapAudioMixer)

NS_INTERFACE_MAP_BEGIN(zapAudioMixer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioMixer::AddedToGraph(zapIMediaGraph *graph,
                            const nsACString & id,
                            nsIPropertyBag2* node_pars)
{
  // node parameter defaults:
  mSampleRate = 8000; // 8000Hz
  mFrameDuration = 0.02; // 20ms
  mNumChannels = 1; // mono
  mSampleFormat = sf_float32_32768;

  // extract node parameters:

   if (node_pars) {
     node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                    &mSampleRate);
     node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                    &mFrameDuration);
     node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                    &mNumChannels);
     nsCString sampleformat_string;
     if (NS_SUCCEEDED(node_pars->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                       sampleformat_string))) {
       mSampleFormat = StrToZapAudioSampleFormat(sampleformat_string);
       if (mSampleFormat != sf_float32_32768) {
         NS_ERROR("unsupported sample format");
         return NS_ERROR_FAILURE;
       }
     }
   }

  mSamplesPerFrame = (PRUint32)(mSampleRate*mFrameDuration*mNumChannels);
   
  // create a new streaminfo:
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
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapAudioMixer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioMixer::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapAudioMixer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  zapAudioMixerInput* input = new zapAudioMixerInput();
  input->AddRef();
  input->Init(this);
  *_retval = input;
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioMixer::ConnectSink(zapIMediaSink *sink, const nsACString & connection_id)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapAudioMixer::DisconnectSink(zapIMediaSink *sink,
                              const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* void requestFrame (); */
NS_IMETHODIMP
zapAudioMixer::RequestFrame()
{
  if (mWaiting) return NS_OK; // we're already waiting for data

  if (!mFramesAvailable) {
    // request frames from inputs:
    for (PRInt32 i=0, l=mInputs.Count(); i<l; ++i) {
      ((zapAudioMixerInput*)mInputs[i])->RequestFrame();
    }
    // don't return; we might get some synchronous frames
  }
  
  if (!mFramesAvailable) {
    // still no frames. set async waiting flag:
    // the first frame that arrives will trigger mixing now.
    mWaiting = PR_TRUE;
  }
  else {
    // We've got frames -> mix synchronously
    Mix();
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------

void zapAudioMixer::FrameAvailable()
{
  mFramesAvailable = PR_TRUE;
  if (mWaiting) {
    Mix();
  }
}

void zapAudioMixer::Mix()
{
  mWaiting = PR_FALSE;
  mFramesAvailable = PR_FALSE;
  
  nsCString outdata;
  outdata.SetLength(GetZapAudioSampleSize(sf_float32_32768) * mSamplesPerFrame);
  float* d = (float*)outdata.BeginWriting();
  memset(d, 0, outdata.Length());

  PRInt32 activeInputs = 0;
  for (PRInt32 i=0, l=mInputs.Count(); i<l; ++i) {
    nsCOMPtr<zapIMediaFrame> frame;
    activeInputs += ((zapAudioMixerInput*)mInputs[i])->ConsumeFrame(getter_AddRefs(frame));
    if (frame) {
      nsCString indata;
      frame->GetData(indata);
      NS_ASSERTION(indata.Length() == GetZapAudioSampleSize(sf_float32_32768) * mSamplesPerFrame,
                   "buffer length mismatch");
      float* s = (float*)indata.BeginReading();
      for (int sp=0; sp < mSamplesPerFrame; ++sp)
        d[sp] += s[sp];
    }
  }

  if (activeInputs == 0) {
    // EOF
    if (mOutput) {
#ifdef DEBUG
      printf("(mixer->EOF)");
#endif
      mOutput->ProcessFrame(nsnull);
    }
    return;
  }
  
  // scale data
  if (activeInputs > 1) {
    float scalefactor = 1.0f / sqrt((float)activeInputs);
    for (int sp=0; sp < mSamplesPerFrame; ++sp) {
      d[sp] *= scalefactor;
      }
  }
  
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mData = outdata;
  //XXX timestamp
  frame->mStreamInfo = mStreamInfo;

  // send frame and clean up:
  if (mOutput)
    mOutput->ProcessFrame(frame);
  frame->Release();
}
