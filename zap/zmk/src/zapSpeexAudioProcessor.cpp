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

#include "zapSpeexAudioProcessor.h"
#include "nsIPropertyBag2.h"
#include "stdio.h"
#include "zapMediaFrame.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsStringAPI.h"
#include "nsIComponentManager.h"
#include "float.h"
#include "zapZMKImplUtils.h"

////////////////////////////////////////////////////////////////////////
// zapSpeexAudioProcessorEcho

class zapSpeexAudioProcessorEcho : public zapIMediaSink
{
public:
  zapSpeexAudioProcessorEcho();
  ~zapSpeexAudioProcessorEcho();

  void Init(zapSpeexAudioProcessor* speexAudioProcessor);
  void ProduceEchoFrame(zapIMediaFrame ** frame);

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASINK

private:
  nsRefPtr<zapSpeexAudioProcessor> mSpeexAudioProcessor;
  nsCOMPtr<zapIMediaSource> mInput;
};

//----------------------------------------------------------------------

zapSpeexAudioProcessorEcho::zapSpeexAudioProcessorEcho()
{
}

zapSpeexAudioProcessorEcho::~zapSpeexAudioProcessorEcho()
{
  NS_ASSERTION(mSpeexAudioProcessor, "Never initialized");
  mSpeexAudioProcessor->mEcho = nsnull;
}

void zapSpeexAudioProcessorEcho::Init(zapSpeexAudioProcessor* speexAudioProcessor) {
  mSpeexAudioProcessor = speexAudioProcessor;
  mSpeexAudioProcessor->mEcho = this;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapSpeexAudioProcessorEcho)
NS_IMPL_RELEASE(zapSpeexAudioProcessorEcho)

NS_INTERFACE_MAP_BEGIN(zapSpeexAudioProcessorEcho)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapSpeexAudioProcessorEcho::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapSpeexAudioProcessorEcho::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapSpeexAudioProcessorEcho::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("not a passive sink!");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

void
zapSpeexAudioProcessorEcho::ProduceEchoFrame(zapIMediaFrame ** frame)
{
  *frame = nsnull;
  if (!mInput) return;

  mInput->ProduceFrame(frame);
}


////////////////////////////////////////////////////////////////////////
// zapSpeexAudioProcessor

zapSpeexAudioProcessor::zapSpeexAudioProcessor()
    : mEchoState(nsnull),
      mEcho(nsnull)
{
}

zapSpeexAudioProcessor::~zapSpeexAudioProcessor()
{
  speex_echo_state_destroy(mEchoState);
  mEchoState = nsnull;
  speex_preprocess_state_destroy(mPreprocessState);
  mPreprocessState = nsnull;
  delete[] mResidue;
  mResidue = nsnull;  
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapSpeexAudioProcessor)
NS_IMPL_RELEASE(zapSpeexAudioProcessor)

NS_INTERFACE_MAP_BEGIN(zapSpeexAudioProcessor)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapISpeexAudioProcessor)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapSpeexAudioProcessor::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                              nsIPropertyBag2* node_pars)
{
  // Add a reference to the container. We must hang onto this
  // reference until the node is destroyed so that any proxies on us
  // have a context to shut down in even after the container has shut
  // down.
  mContainer = container;
  
  // unpack node parameters:
  mAEC = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("aec"), PR_FALSE);
  mAEC2Stage = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("aec_2_stage"), PR_FALSE);
  mAECTail = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("aec_tail"), 300.0);
  mDenoise = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("denoise"), PR_FALSE);
  mAGC = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("agc"), PR_FALSE);
  mAGCLevel = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("agc_level"), 8000.0);
  mVAD = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("vad"), PR_FALSE);
  mDereverb = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("dereverb"), PR_FALSE);
  mDereverbLevel = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("dereverb_level"), 0.2);
  mDereverbDecay = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("dereverb_decay"), 0.5);
  
  // XXX 20ms frame size, 8000Hz sampling rate hardcoded atm
  mResidue = new float[161];
  mEchoState = speex_echo_state_init(160, 160/20 * mAECTail);
  mPreprocessState = speex_preprocess_state_init(160, 8000);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DENOISE, &mDenoise);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_AGC, &mAGC);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_AGC_LEVEL,
                       &mAGCLevel);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_VAD, &mVAD);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB,
                       &mDereverb);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL,
                       &mDereverbLevel);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB_DECAY,
                       &mDereverbDecay);
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapSpeexAudioProcessor::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetSource(nsIPropertyBag2 *source_pars,
                                  zapIMediaSource **_retval)
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
zapSpeexAudioProcessor::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (!sink_pars) return NS_ERROR_FAILURE;

  nsCString sinkName;
  NS_ENSURE_SUCCESS(sink_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"),
                                                     sinkName),
                    NS_ERROR_FAILURE);

  if (sinkName == NS_LITERAL_CSTRING("input")) {
    if (mInput) {
      NS_ERROR("input already connected");
      return NS_ERROR_FAILURE;
    }

    *_retval = this;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  else if (sinkName == NS_LITERAL_CSTRING("echo")) {
    if (mEcho) {
      NS_ERROR("echo already connected");
      return NS_ERROR_FAILURE;
    }

    zapSpeexAudioProcessorEcho* echoSink = new zapSpeexAudioProcessorEcho();
    echoSink->AddRef();
    echoSink->Init(this);
    *_retval = echoSink;
    return NS_OK;
  }

  // ... else
  NS_ERROR("unknown sink");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapSpeexAudioProcessor::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapSpeexAudioProcessor::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;

  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapSpeexAudioProcessor::ConsumeFrame(zapIMediaFrame * frame)
{
  if (!mOutput) return NS_ERROR_FAILURE; // nothing to do
  
  nsCOMPtr<zapIMediaFrame> oframe;
  Filter(frame, getter_AddRefs(oframe));
  
  return mOutput->ConsumeFrame(oframe);
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapSpeexAudioProcessor::ConnectSink(zapIMediaSink *sink)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapSpeexAudioProcessor::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapSpeexAudioProcessor::ProduceFrame(zapIMediaFrame ** frame)
{
  if (!mInput) {
    *frame = nsnull;
    return NS_ERROR_FAILURE; // nothing to do
  }
  
  // get input frame:
  nsCOMPtr<zapIMediaFrame> iframe;
  mInput->ProduceFrame(getter_AddRefs(iframe));
  
  Filter(iframe, frame);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapISpeexAudioProcessor methods

/* attribute boolean aec; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetAec(PRBool *aAec)
{
  *aAec = mAEC;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetAec(PRBool aAec)
{
  mAEC = aAec;
  return NS_OK;
}

/* attribute boolean aec2Stage; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetAec2Stage(PRBool *aAec2Stage)
{
  *aAec2Stage = mAEC2Stage;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetAec2Stage(PRBool aAec2Stage)
{
  mAEC2Stage = aAec2Stage;
  return NS_OK;
}

/* attribute float aecTail; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetAecTail(float *aAecTail)
{
  *aAecTail = mAECTail;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetAecTail(float aAecTail)
{
  if (mAECTail != aAecTail) {
    speex_echo_state_destroy(mEchoState);
    // XXX 20ms frame size, 8000Hz sampling rate hardcoded atm
    mEchoState = speex_echo_state_init(160, 160/20 * aAecTail);
  }
  mAECTail = aAecTail;
  return NS_OK;
}

/* attribute boolean denoise; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetDenoise(PRBool *aDenoise)
{
  *aDenoise = mDenoise;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetDenoise(PRBool aDenoise)
{
  if (mDenoise != aDenoise) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_DENOISE, &aDenoise);
  }
  mDenoise = aDenoise;
  return NS_OK;
}

/* attribute boolean agc; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetAgc(PRBool *aAgc)
{
  *aAgc = mAGC;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetAgc(PRBool aAgc)
{
  if (mAGC != aAgc) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_AGC, &aAgc);
  }
  mAGC = aAgc;
  return NS_OK;
}

/* attribute float agcLevel; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetAgcLevel(float *aAgcLevel)
{
  *aAgcLevel = mAGCLevel;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetAgcLevel(float aAgcLevel)
{
  if (mAGCLevel != aAgcLevel) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_AGC_LEVEL, &aAgcLevel);
  }
  mAGCLevel = aAgcLevel;  
  return NS_OK;
}

/* attribute boolean vad; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetVad(PRBool *aVad)
{
  *aVad = mVAD;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetVad(PRBool aVad)
{
  if (mVAD != aVad) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_VAD, &aVad);
  }
  mVAD = aVad;
  return NS_OK;
}

/* attribute boolean dereverb; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetDereverb(PRBool *aDereverb)
{
  *aDereverb = mDereverb;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetDereverb(PRBool aDereverb)
{
  if (mDereverb != aDereverb) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_DEREVERB, &aDereverb);
  }
  mDereverb = aDereverb;
  return NS_OK;
}

/* attribute float dereverbLevel; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetDereverbLevel(float *aDereverbLevel)
{
  *aDereverbLevel = mDereverbLevel;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetDereverbLevel(float aDereverbLevel)
{
  if (mDereverbLevel != aDereverbLevel) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_DEREVERB_LEVEL,
                         &aDereverbLevel);
  }
  mDereverbLevel = aDereverbLevel;
  return NS_OK;
}

/* attribute float dereverbDecay; */
NS_IMETHODIMP
zapSpeexAudioProcessor::GetDereverbDecay(float *aDereverbDecay)
{
  *aDereverbDecay = mDereverbDecay;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexAudioProcessor::SetDereverbDecay(float aDereverbDecay)
{
  if (mDereverbDecay != aDereverbDecay) {
    speex_preprocess_ctl(mPreprocessState,
                         SPEEX_PREPROCESS_SET_DEREVERB_DECAY,
                         &aDereverbDecay);
  }
  mDereverbDecay = aDereverbDecay;
  return NS_OK;
}

/* void reset(); */
NS_IMETHODIMP
zapSpeexAudioProcessor::Reset()
{
  speex_echo_state_reset(mEchoState);
  speex_preprocess_state_destroy(mPreprocessState);
  mPreprocessState = speex_preprocess_state_init(160, 8000);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DENOISE, &mDenoise);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_AGC, &mAGC);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_AGC_LEVEL,
                       &mAGCLevel);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_VAD, &mVAD);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB,
                       &mDereverb);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL,
                       &mDereverbLevel);
  speex_preprocess_ctl(mPreprocessState, SPEEX_PREPROCESS_SET_DEREVERB_DECAY,
                       &mDereverbDecay);

  return NS_OK;
}

//----------------------------------------------------------------------
// Implementation helpers:

void
zapSpeexAudioProcessor::Filter(zapIMediaFrame *frame,
                               zapIMediaFrame **outframe)
{
  // create output frame:
  nsRefPtr<zapMediaFrame> oframe = new zapMediaFrame();
  frame->GetStreamInfo(getter_AddRefs(oframe->mStreamInfo));
  frame->GetTimestamp(&oframe->mTimestamp);

  if (mAEC) {
    // acoustic echo cancellation:
    
    // get echo frame:
    nsCOMPtr<zapIMediaFrame> echoFrame;
    if (mEcho)
      mEcho->ProduceEchoFrame(getter_AddRefs(echoFrame));
    
    // unpack input and echo data:
    nsCString input, echo;
    frame->GetData(input);
    
    if (echoFrame) {
      echoFrame->GetData(echo);
    }
    else {
      // generate a silent frame and pass it to the
      // echo canceller
      echo.SetLength(4*160); //XXX float32 * 0.2ms hardcoded
      float *ep = (float*)echo.BeginWriting();
      for (int i=0;i<160;++i) {
        ep[i] = 0.0;
      }
    }

    oframe->mData.SetLength(4*160); // XXX float32 * 0.2ms hardcoded
    speex_echo_cancel(mEchoState,
                      (float*)input.BeginReading(),
                      (float*)echo.BeginReading(),
                      (float*)oframe->mData.BeginWriting(),
                      mAEC2Stage ? mResidue : nsnull);
  }
  else {
    // copy input data:
    frame->GetData(oframe->mData);
  }
  
  if (mDenoise ||
      mAGC ||
      mVAD ||
      mDereverb ||
      (mAEC && mAEC2Stage)) {
    // run preprocessor:
    speex_preprocess(mPreprocessState,
                     (float*)oframe->mData.BeginWriting(),
                     (mAEC && mAEC2Stage) ? mResidue : nsnull);
  }

  *outframe = oframe;
  NS_ADDREF(*outframe);
}
