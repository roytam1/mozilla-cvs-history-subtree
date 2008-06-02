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

#include "zapTToneGenerator.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "zapAudioToneFrame.h"
#include "zapZMKImplUtils.h"
#include "stdlib.h"

////////////////////////////////////////////////////////////////////////
// structure to hold an individual tone

struct Tone {
  PRUint32 duration; // duration (sample units). 0==infinite
  zapAudioTone toneData;
};

////////////////////////////////////////////////////////////////////////
// ToneDeallocator: helper to clean up our tone buffer

class ToneDeallocator : public nsDequeFunctor
{
public:
  virtual void* operator()(void* obj) {
    Tone* tone = (Tone*)obj;
    delete tone;
    return 0;
  }
};

////////////////////////////////////////////////////////////////////////
// zapTToneGenerator

zapTToneGenerator::zapTToneGenerator()
    : mLoop(PR_FALSE),
      mToneSamplesPlayed(0),
      mBuffer(0),
      mSampleClock(0)
{
}

zapTToneGenerator::~zapTToneGenerator()
{
  ClearState();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapTToneGenerator)
NS_IMPL_THREADSAFE_RELEASE(zapTToneGenerator)

NS_INTERFACE_MAP_BEGIN(zapTToneGenerator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapITToneGenerator)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapTToneGenerator::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                         nsIPropertyBag2* node_pars)
{
  double max_frame_duration = ZMK_GetOptionalDouble(node_pars,
                                                    NS_LITERAL_STRING("max_frame_duration"),
                                                    0.02);
  max_frame_duration *= 8000.0;
  if (max_frame_duration < 1.0 || max_frame_duration > 0xFFFF)
    return NS_ERROR_FAILURE;
  
  mMaxSamplesPerFrame = (PRUint16)max_frame_duration;
  
  // create a new stream info:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/tone");
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapTToneGenerator::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapTToneGenerator::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapTToneGenerator::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapTToneGenerator::ConnectSink(zapIMediaSink *sink)
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
zapTToneGenerator::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapTToneGenerator::ProduceFrame(zapIMediaFrame ** _retval)
{
  Tone* currentTone = (Tone*)mBuffer.PeekFront();
  if (!currentTone) return NS_ERROR_FAILURE;
  
  // construct audio tone frame:
  zapAudioToneFrame* frame = CreateAudioToneFrame(currentTone->toneData,
                                                  mStreamInfo);
  frame->mTimestamp = mSampleClock;

  if (currentTone->duration == 0) {
    // play continuous tone
    frame->mToneData.SetDuration(mMaxSamplesPerFrame);
    mSampleClock += mMaxSamplesPerFrame;
  }
  else if (currentTone->duration - mToneSamplesPlayed <= mMaxSamplesPerFrame) {
    // the tone (or whatever remainder we are currently playing of it)
    // will fit into one frame

    frame->mToneData.SetDuration((PRUint16)(currentTone->duration - mToneSamplesPlayed));

    mSampleClock += currentTone->duration - mToneSamplesPlayed;
    mToneSamplesPlayed = 0;
    
    // move on to next tone:
    mBuffer.PopFront();
    if (mLoop)
      mBuffer.Push(currentTone);
    else
      delete currentTone;
  }
  else {
    // the tone needs to be split over several frames:
    mToneSamplesPlayed += mMaxSamplesPerFrame;
    frame->mToneData.SetDuration(mMaxSamplesPerFrame);
    mSampleClock += mMaxSamplesPerFrame;
  }
  
  *_retval = frame;
  
  return NS_OK;
}

//----------------------------------------------------------------------
// zapITToneGenerator methods:

/* void play (in ACString tonesData); */
NS_IMETHODIMP
zapTToneGenerator::Play(const nsACString & tonesData)
{
  ClearState();

  // parse tonesData into zapAudioToneFrames:
  return ParseTonesData(PromiseFlatCString(tonesData).get());
}

//----------------------------------------------------------------------
// Implementation helpers:

void zapTToneGenerator::ClearState()
{
  ToneDeallocator deallocator;
  mBuffer.ForEach(deallocator);
  mBuffer.Empty();
  mLoop = PR_FALSE;
  mToneSamplesPlayed = 0;
}

nsresult zapTToneGenerator::ParseTonesData(const char *buf)
{
  while (*buf) {
    switch (*buf) {
      case '@':
        ++buf;
        if (NS_FAILED(ParseTone(&buf)))
          return NS_ERROR_FAILURE;
        break;
      case 'R':
        mLoop = PR_TRUE;
        ++buf;
        break;
      case ' ':
        // skip whitespace
        ++buf;
        break;
      default:
        ClearState();
        return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

nsresult zapTToneGenerator::ParseTone(const char **bufp)
{
  Tone* tone = new Tone;
    char *endp;
  // duration:
  double duration = strtod(*bufp, &endp);
  if (endp != *bufp) {
    tone->duration = (PRUint32)(duration*8000.0);
    *bufp = endp;
    // other (optional) parameters:
    while (1) {
      switch (**bufp) {
        case 'v':
          ++(*bufp);
          tone->toneData.SetVolume((PRUint16)strtol(*bufp, &endp, 10));
          break;
        case '+':
          ++(*bufp);
          tone->toneData.AddFrequency((PRUint16)strtol(*bufp, &endp, 10));
          break;
        case '*':
          ++(*bufp);
          tone->toneData.SetModulation((PRUint16)strtol(*bufp, &endp, 10));
          if (endp[0] == '/' && endp[1] == '3') {
            tone->toneData.SetT(PR_TRUE);
            endp += 2;
          }
          break;
        default:
          mBuffer.Push(tone);
          return NS_OK;
      }
      if (endp == *bufp) break; // parse error
      *bufp = endp;
    }
  }
  
  delete tone;
  return NS_ERROR_FAILURE;
}
