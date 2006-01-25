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

#include "zapDTMFGenerator.h"
#include "stdio.h"
#include "nsString.h"
#include "zapTelephoneEventFrame.h"
#include "zapStreamUtils.h"

////////////////////////////////////////////////////////////////////////
// structure to hold an individual event

struct TEvent {
  PRUint32 duration; // duration (sample units).
  zapTelephoneEvent teventData;
};

////////////////////////////////////////////////////////////////////////
// TEventDeallocator: helper to clean up our tevent buffer

class TEventDeallocator : public nsDequeFunctor
{
public:
  virtual void* operator()(void* obj) {
    TEvent* tevent = (TEvent*)obj;
    delete tevent;
    return 0;
  }
};

////////////////////////////////////////////////////////////////////////
// zapDTMFGenerator

zapDTMFGenerator::zapDTMFGenerator()
    : mTEventSamplesPlayed(0),
      mBuffer(0),
      mSampleClock(0)
{
#ifdef DEBUG_afri_zmk
  printf("zapDTMFGenerator::zapDTMFGenerator()\n");
#endif
}

zapDTMFGenerator::~zapDTMFGenerator()
{
  ClearState();
#ifdef DEBUG_afri_zmk
  printf("zapDTMFGenerator::~zapDTMFGenerator()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapDTMFGenerator)
NS_IMPL_RELEASE(zapDTMFGenerator)

NS_INTERFACE_MAP_BEGIN(zapDTMFGenerator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIDTMFGenerator)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapDTMFGenerator::AddedToGraph(zapIMediaGraph *graph,
                            const nsACString & id,
                            nsIPropertyBag2* node_pars)
{
  // node parameter defaults:
  double tone_duration = 0.09;
  double pause_duration = 0.07;
  double volume = -8.0;
  double max_frame_duration = 0.02;
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("tone_duration"),
                                   &tone_duration);
    node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("pause_duration"),
                                   &pause_duration);
    node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("volume"),
                                   &volume);
    node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("max_frame_duration"),
                                   &max_frame_duration);
  }

  max_frame_duration *= 8000.0;
  if (max_frame_duration < 1.0 || max_frame_duration > 0xFFFF)
    return NS_ERROR_FAILURE;  
  mMaxSamplesPerFrame = (PRUint16)max_frame_duration;

  tone_duration *= 8000.0;
  if (tone_duration > 0xFFFF)
    return NS_ERROR_FAILURE;
  mToneDuration = (PRUint32)tone_duration;

  pause_duration *= 8000.0;
  if (pause_duration > 0xFFFF)
    return NS_ERROR_FAILURE;
  mPauseDuration = (PRUint32)pause_duration;
  
  mVolume = (PRUint16)(-volume);
  
  
  
  // create a new stream info:
  mStreamInfo = CreateStreamInfo(NS_LITERAL_CSTRING("audio/telephone-event"));
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapDTMFGenerator::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapDTMFGenerator::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapDTMFGenerator::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapDTMFGenerator::ConnectSink(zapIMediaSink *sink,
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
zapDTMFGenerator::DisconnectSink(zapIMediaSink *sink,
                                 const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapDTMFGenerator::ProduceFrame(zapIMediaFrame ** _retval)
{
  // XXX We don't handle segmentation yet (events longer than 0xFFFF samples)
  
  TEvent* currentTEvent = (TEvent*)mBuffer.PeekFront();
  if (!currentTEvent) return NS_ERROR_FAILURE;

  // construct telephone-event frame:
  zapTelephoneEventFrame* frame = CreateTelephoneEventFrame(currentTEvent->teventData,
                                                            mStreamInfo);

  // set to timestamp of beginning of tone (see RFC2833):
  frame->mTimestamp = mSampleClock - mTEventSamplesPlayed;

  if (!mTEventSamplesPlayed) {
    // this is the beginning of a new event:
    frame->SetM(PR_TRUE);
  }
  
  if (currentTEvent->duration - mTEventSamplesPlayed <= mMaxSamplesPerFrame) {
    // the tevent (or whatever remainer we are currently playing of it)
    // will fit into one frame
#ifdef DEBUG_afri_zmk
    printf("d<%d,%d,%d>", currentTEvent->teventData.GetEvent(), frame->mTimestamp, currentTEvent->duration);
#endif
    // set duration to cumulative duration (see RFC2833):
    frame->mTEventData.SetDuration((PRUint16)(currentTEvent->duration));
    frame->SetE(PR_TRUE);
    
    mSampleClock += currentTEvent->duration - mTEventSamplesPlayed + mPauseDuration;
    mTEventSamplesPlayed = 0;

    // move on to next tevent:
    mBuffer.PopFront();
    delete currentTEvent;
  }
  else {
    // the tevent needs to be split over several frames:
    mTEventSamplesPlayed += mMaxSamplesPerFrame;

#ifdef DEBUG_afri_zmk
    printf("D<%d,%d,%d>", currentTEvent->teventData.GetEvent(), frame->mTimestamp, mTEventSamplesPlayed);
#endif

// set duration to cumulative duration (see RFC2833):
    frame->mTEventData.SetDuration(mTEventSamplesPlayed);
    mSampleClock += mMaxSamplesPerFrame;
  }

  *_retval = frame;

  return NS_OK;
}

//----------------------------------------------------------------------
// zapIDTMFGenerator methods:

/* void play (in ACString dtmfCodes); */
NS_IMETHODIMP
zapDTMFGenerator::Play(const nsACString & dtmfCodes)
{
  return ParseDTMFData(PromiseFlatCString(dtmfCodes).get());
}

//----------------------------------------------------------------------
// Implementation helpers:

void zapDTMFGenerator::ClearState()
{
  TEventDeallocator deallocator;
  mBuffer.ForEach(deallocator);
  mBuffer.Empty();
  mTEventSamplesPlayed = 0;
}

nsresult zapDTMFGenerator::ParseDTMFData(const char *buf)
{
  while (*buf) {
    switch (*buf) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        QueueTEvent(*buf - '0');
        break;
      case '*':
        QueueTEvent(10);
        break;
      case '#':
        QueueTEvent(11);
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
        QueueTEvent(*buf - 'A');
        break;
      default:
        NS_WARNING("unknown dtmf code");
        // ignore
    }
    ++buf;
  }
  
  return NS_OK;
}

void zapDTMFGenerator::QueueTEvent(PRUint16 event)
{
#ifdef DEBUG_afri_zmk
  printf("D<enqueue %d>", event);
#endif
  TEvent* tevent = new TEvent;
  tevent->duration = mToneDuration;
  tevent->teventData.SetEvent(event);
  tevent->teventData.SetVolume(mVolume);
  mBuffer.Push(tevent);
}
