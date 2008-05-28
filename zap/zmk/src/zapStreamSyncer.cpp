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

#include "zapStreamSyncer.h"
#include "nsIPropertyBag2.h"
#include "stdio.h"
#include "zapIMediaFrame.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapStreamSyncerClock

class zapStreamSyncerClock : public zapIMediaSink
{
public:
  zapStreamSyncerClock();
  ~zapStreamSyncerClock();

  void Init(zapStreamSyncer* syncer);

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASINK

  nsRefPtr<zapStreamSyncer> mSyncer;
  nsCOMPtr<zapIMediaSource> mInput;
  
  nsCOMPtr<nsIPropertyBag2> mStreamInfo;
};

//----------------------------------------------------------------------

zapStreamSyncerClock::zapStreamSyncerClock()
{
}

zapStreamSyncerClock::~zapStreamSyncerClock()
{
  NS_ASSERTION(mSyncer, "Never initialized");
  mSyncer->mClock = nsnull;
}

void zapStreamSyncerClock::Init(zapStreamSyncer* syncer) {
  mSyncer = syncer;
  mSyncer->mClock = this;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStreamSyncerClock)
NS_IMPL_RELEASE(zapStreamSyncerClock)

NS_INTERFACE_MAP_BEGIN(zapStreamSyncerClock)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStreamSyncerClock::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStreamSyncerClock::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStreamSyncerClock::ConsumeFrame(zapIMediaFrame * frame)
{
  frame->GetTimestamp(&mSyncer->mCurrentTime);
  mSyncer->Wakeup();
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapStreamSyncerWakeupEvent

NS_IMETHODIMP
zapStreamSyncerWakeupEvent::Run()
{
  if (!mSyncer) return NS_OK;
  mSyncer->mWakeupEvent.Forget();
  mSyncer->Wakeup();
  
  return NS_OK;
}

void
zapStreamSyncerWakeupEvent::Revoke()
{
  mSyncer = nsnull;
}

////////////////////////////////////////////////////////////////////////
// zapStreamSyncer

zapStreamSyncer::zapStreamSyncer()
    : mClock(nsnull),
      mCurrentTime(-1)      
{
}

zapStreamSyncer::~zapStreamSyncer()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStreamSyncer)
NS_IMPL_RELEASE(zapStreamSyncer)

NS_INTERFACE_MAP_BEGIN(zapStreamSyncer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIStreamSyncer)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStreamSyncer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                       nsIPropertyBag2* node_pars)
{
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapStreamSyncer::RemovedFromContainer()
{
  Reset();
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStreamSyncer::GetSource(nsIPropertyBag2 *source_pars,
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
zapStreamSyncer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
  else if (sinkName == NS_LITERAL_CSTRING("clock")) {
    if (mClock) {
      NS_ERROR("clock already connected");
      return NS_ERROR_FAILURE;
    }

    zapStreamSyncerClock* clockSink = new zapStreamSyncerClock();
    clockSink->AddRef();
    clockSink->Init(this);
    *_retval = clockSink;
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
zapStreamSyncer::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStreamSyncer::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;

  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStreamSyncer::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("stream-syncer is an active sink. Maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamSyncer::ConnectSink(zapIMediaSink *sink)
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
zapStreamSyncer::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStreamSyncer::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("stream-syncer:input is an active source. Maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIStreamSyncer:

/* void reset (); */
NS_IMETHODIMP
zapStreamSyncer::Reset()
{
  mWakeupEvent.Revoke();
  mWakeupEvent.Forget();
  mNextFrame = nsnull;
  mCurrentTime = -1;
  
  return NS_OK;
}

/* boolean peekFrame (); */
NS_IMETHODIMP
zapStreamSyncer::PeekFrame(PRBool *_retval)
{
  *_retval = PR_FALSE;
  
  if (!mOutput) return NS_OK;
  
  if (!mNextFrame) {
    if (!mInput ||
        NS_FAILED(mInput->ProduceFrame(getter_AddRefs(mNextFrame))) ||
        !mNextFrame)
      return NS_OK;
  }

  mNextFrame->GetTimestamp(&mCurrentTime);
  
  if (NS_FAILED(mOutput->ConsumeFrame(mNextFrame))) return NS_OK;

  // success
  *_retval = PR_TRUE;
  return NS_OK;
}

/* attribute unsigned long long currentTimestamp; */
NS_IMETHODIMP
zapStreamSyncer::GetCurrentTimestamp(PRUint64 *aCurrentTimestamp)
{
  *aCurrentTimestamp = mCurrentTime;
  return NS_OK;
}
NS_IMETHODIMP
zapStreamSyncer::SetCurrentTimestamp(PRUint64 aCurrentTimestamp)
{
  mCurrentTime = aCurrentTimestamp;
  return NS_OK;
}


//----------------------------------------------------------------------
// Implementation helpers:

void
zapStreamSyncer::Wakeup()
{
  if (!mNextFrame && mInput) {
    mInput->ProduceFrame(getter_AddRefs(mNextFrame));
  }
  
  if (mNextFrame) {
    PRUint64 ts;
    mNextFrame->GetTimestamp(&ts);
#ifdef DEBUG_alex
//     printf("video ts = %d ", ts);
//     printf("-- audio ts = %d \n", mCurrentTime);
#endif
    if (ts <= mCurrentTime) {
      if (mOutput)
        mOutput->ConsumeFrame(mNextFrame);
      mNextFrame = nsnull;
      ScheduleWakeup();
    }
  }
}

void
zapStreamSyncer::ScheduleWakeup()
{
  if (mWakeupEvent.IsPending()) return;
  nsRefPtr<zapStreamSyncerWakeupEvent> ev = new zapStreamSyncerWakeupEvent(this);
  if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev)))
    mWakeupEvent = ev;
}
