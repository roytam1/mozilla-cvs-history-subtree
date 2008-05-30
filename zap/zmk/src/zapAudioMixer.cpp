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
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "zapIMediaFrame.h"
#include "zapMediaFrame.h"
#include "math.h"
#include "zapZMKImplUtils.h"
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

  // fetch a frame from input:
  nsresult ProduceFrame(zapIMediaFrame**frame);
  
private:
  nsRefPtr<zapAudioMixer> mMixer;
  nsCOMPtr<zapIMediaSource> mInput;
  nsCOMPtr<nsIPropertyBag2> mLastValidStreamInfo;

  PRBool ValidateFrame(zapIMediaFrame *frame);
};

//----------------------------------------------------------------------
zapAudioMixerInput::zapAudioMixerInput()
{
}

zapAudioMixerInput::~zapAudioMixerInput()
{
  NS_ASSERTION(mMixer, "Never initialized");
  // clean up references:
  mMixer->mInputs.RemoveElement(this);
  mMixer = nsnull;
}

void zapAudioMixerInput::Init(zapAudioMixer* mixer) {
  mMixer = mixer;
  // append ourselves to the mixer's array of inputs:
  mMixer->mInputs.AppendElement(this);
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

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapAudioMixerInput::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapAudioMixerInput::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapAudioMixerInput::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_OK;
}

//----------------------------------------------------------------------

nsresult zapAudioMixerInput::ProduceFrame(zapIMediaFrame** frame)
{
  *frame = nsnull;
  
  if (!mInput || NS_FAILED(mInput->ProduceFrame(frame)))
    return NS_ERROR_FAILURE;
  
  if (!ValidateFrame(*frame)) {
    NS_IF_RELEASE(*frame);
    *frame = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------

PRBool zapAudioMixerInput::ValidateFrame(zapIMediaFrame* frame)
{
  nsCOMPtr<nsIPropertyBag2> streamInfo;
  frame->GetStreamInfo(getter_AddRefs(streamInfo));
  if (streamInfo == mLastValidStreamInfo)
    return PR_TRUE;

  // we have a new stream info. Check if the stream is compatible with
  // us:
  if (!CheckAudioStream(streamInfo, mMixer->mStreamParameters)) {
    return PR_FALSE;
  }
  
  mLastValidStreamInfo = streamInfo;
  return  PR_TRUE;
}


////////////////////////////////////////////////////////////////////////
// zapAudioMixer

zapAudioMixer::zapAudioMixer()
    : mVolumeFactor(1.0),
      mMute(PR_FALSE)
{
}

zapAudioMixer::~zapAudioMixer()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioMixer)
NS_IMPL_THREADSAFE_RELEASE(zapAudioMixer)

NS_INTERFACE_MAP_BEGIN(zapAudioMixer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIAudioMixer)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioMixer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                     nsIPropertyBag2* node_pars)
{
  mContainer = container;
  
  // unpack node parameters:
  nsresult rv;
  rv = mStreamParameters.InitWithProperties(node_pars);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (mStreamParameters.sample_format != sf_float32_32768 && 
      mStreamParameters.sample_format != sf_float32_1) 
  {
    NS_ERROR("unsupported sample format! write me!");
    return NS_ERROR_FAILURE;
  }

  // create a new stream info:
  mStreamInfo = mStreamParameters.CreateStreamInfo();

  mSampleClock = 0;
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapAudioMixer::RemovedFromContainer()
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

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapAudioMixer::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "sink already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapAudioMixer::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioMixer::ProduceFrame(zapIMediaFrame ** _retval)
{
  zapMediaNodeContainerAutoLock lock(mContainer, this);
  
  PRUint32 samplesPerFrame = mStreamParameters.samples * mStreamParameters.channels;
  
  PRInt32 activeInputs = mInputs.Count(); //XXX
  
  // construct audio frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = mSampleClock;
  
  frame->mData.SetLength(mStreamParameters.GetFrameLength());
  float* d = (float*)frame->mData.BeginWriting();
  memset(d, 0, frame->mData.Length());

  for (PRInt32 i=0; i<activeInputs; ++i) {
    nsCOMPtr<zapIMediaFrame> frame;
    ((zapAudioMixerInput*)mInputs[i])->ProduceFrame(getter_AddRefs(frame));
    if (frame && !mMute) {
      nsCString indata;
      frame->GetData(indata);
      NS_ASSERTION(indata.Length() == mStreamParameters.GetFrameLength(),
                   "buffer length mismatch");
      float* s = (float*)indata.BeginReading();
      for (unsigned int sp=0; sp < samplesPerFrame; ++sp)
        d[sp] += s[sp];
    }
  }

  if (!mMute && activeInputs != 0 &&
      (mVolumeFactor != 1.0 || activeInputs > 1)) {
    // scale data
    float scalefactor = mVolumeFactor / sqrt((float)activeInputs);
    for (unsigned int sp=0; sp < samplesPerFrame; ++sp) {
      d[sp] *= scalefactor;
    }
  }

  mSampleClock += mStreamParameters.samples;
  
  *_retval = frame;
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIAudioMixer methods:

/* attribute float masterVolume; */
NS_IMETHODIMP
zapAudioMixer::GetMasterVolume(float *aMasterVolume)
{
  NS_ASSERTION(mVolumeFactor > 0.0, "uh-oh, invalid volume factor");
  *aMasterVolume = 20*log10(mVolumeFactor);
  return NS_OK;
}
NS_IMETHODIMP
zapAudioMixer::SetMasterVolume(float aMasterVolume)
{
  mVolumeFactor = pow(10.0, aMasterVolume/20.0);
  if (mVolumeFactor <= 0.0) mVolumeFactor = 1e-20;
  return NS_OK;
}

/* attribute boolean mute; */
NS_IMETHODIMP
zapAudioMixer::GetMute(PRBool *aMute)
{
  *aMute = mMute;
  return NS_OK;
}
NS_IMETHODIMP
zapAudioMixer::SetMute(PRBool aMute)
{
  mMute = aMute;
  return NS_OK;
}
