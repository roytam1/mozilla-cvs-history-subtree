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

#include "zapPacketBuffer.h"
#include "nsIPropertyBag2.h"
#include "stdio.h"
#include "zapIMediaFrame.h"
#include "zapIMediaGraph.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsString.h"

////////////////////////////////////////////////////////////////////////
// PacketDeallocator: helper to clean up packet buffer

class PacketDeallocator : public nsDequeFunctor
{
public:
  virtual void* operator()(void* obj) {
    zapIMediaFrame* frame = (zapIMediaFrame*)obj;
    NS_IF_RELEASE(frame);
    return 0;
  }
};


////////////////////////////////////////////////////////////////////////
// zapPacketBuffer

zapPacketBuffer::zapPacketBuffer()
    : mBuffer(0),
      mSourceState(zapPacketBufferSource_STOP_IDLE::Instance()),
      mSinkState(zapPacketBufferSink_IDLE_PREFILLING::Instance())
{
#ifdef DEBUG
  printf("zapPacketBuffer::zapPacketBuffer()\n");
#endif
}

zapPacketBuffer::~zapPacketBuffer()
{
  ClearBuffer();
#ifdef DEBUG
  printf("zapPacketBuffer::~zapPacketBuffer()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapPacketBuffer)
NS_IMPL_RELEASE(zapPacketBuffer)

NS_INTERFACE_MAP_BEGIN(zapPacketBuffer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapPacketBuffer::AddedToGraph(zapIMediaGraph *graph,
                              const nsACString & id,
                              nsIPropertyBag2* node_pars)
{
  graph->GetEventQueue(getter_AddRefs(mEventQueue));

  // node parameter defaults:
  mPrefillSize = 0;
  mMaxSize = 10;
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("prefill_size"), &mPrefillSize);
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("max_size"), &mMaxSize);

    if (mMaxSize<mPrefillSize) {
      NS_WARNING("adjusted maximum buffer size");
      mMaxSize = mPrefillSize;
    }
  }

  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapPacketBuffer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapPacketBuffer::GetSource(nsIPropertyBag2 *source_pars,
                           zapIMediaSource **_retval)
{
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapPacketBuffer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::ConnectSource(zapIMediaSource *source,
                               const nsACString & connection_id)
{
  return mSourceState->ConnectSource(this, source, connection_id);
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::DisconnectSource(zapIMediaSource *source,
                                  const nsACString & connection_id)
{
  return mSourceState->DisconnectSource(this, source, connection_id);
}

/* void processFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPacketBuffer::ProcessFrame(zapIMediaFrame *frame)
{
  return mSourceState->ProcessFrame(this, frame);
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::ConnectSink(zapIMediaSink *sink,
                             const nsACString & connection_id)
{
  return mSinkState->ConnectSink(this, sink, connection_id);
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::DisconnectSink(zapIMediaSink *sink,
                                const nsACString & connection_id)
{
  return mSinkState->DisconnectSink(this, sink, connection_id);
}

/* void requestFrame (); */
NS_IMETHODIMP
zapPacketBuffer::RequestFrame()
{
  return mSinkState->RequestFrame(this);
}

//----------------------------------------------------------------------
// Implementation helpers:

void
zapPacketBuffer::ChangeSourceState(zapPacketBufferSourceState* state)
{
  mSourceState = state;
}

void
zapPacketBuffer::ChangeSinkState(zapPacketBufferSinkState* state)
{
  mSinkState = state;
}

void zapPacketBuffer::RunQueueing()
{
  mSourceState->RunQueueing(this);
}

void zapPacketBuffer::StopQueueing()
{
  mSourceState->StopQueueing(this);
}

void zapPacketBuffer::PacketQueued()
{
  mSinkState->PacketQueued(this);
}

void zapPacketBuffer::PacketDequeued()
{
  mSourceState->PacketDequeued(this);
}

void zapPacketBuffer::ClearBuffer()
{
  PacketDeallocator deallocator;
  mBuffer.ForEach(deallocator);
  mBuffer.Empty();
}

class zapPacketBufferRequestFrameEvent : public PLEvent
{
public:
  zapPacketBufferRequestFrameEvent(zapPacketBuffer* pb)
      : packetbuffer(pb)
  {
    PL_InitEvent(this, packetbuffer, EventHandler, EventCleanup);
  }

  PR_STATIC_CALLBACK(void *) EventHandler(PLEvent* ev)
  {
    zapPacketBuffer* pb = (zapPacketBuffer*) ev->owner;

    if (pb->mSource)
      pb->mSource->RequestFrame();
    
    return (void*)PR_TRUE;
  }

  PR_STATIC_CALLBACK(void) EventCleanup(PLEvent* ev)
  {
    delete (zapPacketBufferRequestFrameEvent*) ev;
  }

  nsRefPtr<zapPacketBuffer> packetbuffer;
};

void zapPacketBuffer::PostFrameRequest()
{
  mEventQueue->EnterMonitor();
  zapPacketBufferRequestFrameEvent* ev = new zapPacketBufferRequestFrameEvent(this);
  mEventQueue->PostEvent(ev);
  mEventQueue->ExitMonitor();
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSourceState implementation

nsresult
zapPacketBufferSourceState::ConnectSource(zapPacketBuffer* pb,
                                          zapIMediaSource* source,
                                          const nsACString & connection_id)
{
#ifdef DEBUG
  printf("zapPacketBufferSourceState::ConnectSource: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;
}

nsresult
zapPacketBufferSourceState::DisconnectSource(zapPacketBuffer* pb,
                                             zapIMediaSource* source,
                                             const nsACString& connection_id)
{
#ifdef DEBUG
  printf("zapPacketBufferSourceState::DisconnectSource: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;
}

nsresult
zapPacketBufferSourceState::ProcessFrame(zapPacketBuffer* pb,
                                         zapIMediaFrame* frame)
{
#ifdef DEBUG
  printf("zapPacketBufferSourceState::ProcessFrame: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;  
}

void zapPacketBufferSourceState::ChangeState(zapPacketBuffer* pb,
                                             zapPacketBufferSourceState* state)
{
  pb->ChangeSourceState(state);
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_STOP_IDLE implementation

zapPacketBufferSourceState* zapPacketBufferSource_STOP_IDLE::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSource_STOP_IDLE;
  }
  return mInstance;
}

zapPacketBufferSourceState* zapPacketBufferSource_STOP_IDLE::mInstance = nsnull;

void zapPacketBufferSource_STOP_IDLE::PacketDequeued(zapPacketBuffer* pb)
{
  NS_ERROR("not reached");
}

void zapPacketBufferSource_STOP_IDLE::RunQueueing(zapPacketBuffer* pb)
{
  if (pb->mSource) {
    ChangeState(pb, zapPacketBufferSource_RUN_WAITING::Instance());
    pb->mSource->RequestFrame();
  }
  else
    ChangeState(pb, zapPacketBufferSource_RUN_IDLE::Instance());
}

void zapPacketBufferSource_STOP_IDLE::StopQueueing(zapPacketBuffer* pb)
{
  // we're already stopped; do nothing
}

nsresult
zapPacketBufferSource_STOP_IDLE::ConnectSource(zapPacketBuffer* pb,
                                               zapIMediaSource* source,
                                               const nsACString & connection_id)
{
  if (pb->mSource) {
    NS_ERROR("source already connected");
    return NS_ERROR_FAILURE;
  }

  pb->mSource = source;
  return NS_OK;
}

nsresult
zapPacketBufferSource_STOP_IDLE::DisconnectSource(zapPacketBuffer* pb,
                                                  zapIMediaSource* source,
                                                  const nsACString& connection_id)
{
  pb->mSource = nsnull;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_STOP_WAITING implementation

zapPacketBufferSourceState* zapPacketBufferSource_STOP_WAITING::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSource_STOP_WAITING;
  }
  return mInstance;
}

zapPacketBufferSourceState* zapPacketBufferSource_STOP_WAITING::mInstance = nsnull;

void zapPacketBufferSource_STOP_WAITING::PacketDequeued(zapPacketBuffer* pb)
{
  NS_ERROR("not reached");
}

void zapPacketBufferSource_STOP_WAITING::RunQueueing(zapPacketBuffer* pb)
{
  ChangeState(pb, zapPacketBufferSource_RUN_WAITING::Instance());
}

void zapPacketBufferSource_STOP_WAITING::StopQueueing(zapPacketBuffer* pb)
{
  // queuing is already stopped
}

nsresult
zapPacketBufferSource_STOP_WAITING::DisconnectSource(zapPacketBuffer* pb,
                                                     zapIMediaSource* source,
                                                     const nsACString& connection_id)
{
  pb->mSource = nsnull;
  // we won't get a packet from the source anymore:
  ChangeState(pb, zapPacketBufferSource_STOP_IDLE::Instance());
  return NS_OK;
}

nsresult
zapPacketBufferSource_STOP_WAITING::ProcessFrame(zapPacketBuffer* pb,
                                                 zapIMediaFrame* frame)
{
  // silently discard frame; the sink doesn't want it:
#ifdef DEBUG
  printf("(buffer discard)");
#endif
  ChangeState(pb, zapPacketBufferSource_STOP_IDLE::Instance());
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_RUN_WAITING implementation

zapPacketBufferSourceState* zapPacketBufferSource_RUN_WAITING::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSource_RUN_WAITING;
  }
  return mInstance;
}

zapPacketBufferSourceState* zapPacketBufferSource_RUN_WAITING::mInstance = nsnull;

void zapPacketBufferSource_RUN_WAITING::PacketDequeued(zapPacketBuffer* pb)
{
  // nothing to do; we're already waiting for a packet
}

void zapPacketBufferSource_RUN_WAITING::RunQueueing(zapPacketBuffer* pb)
{
  // already running
}

void zapPacketBufferSource_RUN_WAITING::StopQueueing(zapPacketBuffer* pb)
{
  ChangeState(pb, zapPacketBufferSource_STOP_WAITING::Instance());
}

nsresult
zapPacketBufferSource_RUN_WAITING::DisconnectSource(zapPacketBuffer* pb,
                                                    zapIMediaSource* source,
                                                    const nsACString& connection_id)
{
  pb->mSource = nsnull;
  // enqueue an EOF frame:
  pb->mBuffer.Push(nsnull);
  // we won't get any more packet from this source:
  ChangeState(pb, zapPacketBufferSource_RUN_IDLE::Instance());
  pb->PacketQueued();
  return NS_OK;
}

nsresult
zapPacketBufferSource_RUN_WAITING::ProcessFrame(zapPacketBuffer* pb,
                                                zapIMediaFrame* frame)
{
  // enqueue packet:
  NS_IF_ADDREF(frame);
  pb->mBuffer.Push(frame);

  if (pb->mBuffer.GetSize() < pb->mMaxSize) {
    // our buffer is not full; schedule next packet
    pb->PostFrameRequest();
  }
  else {
    ChangeState(pb, zapPacketBufferSource_RUN_IDLE::Instance());
  }
  pb->PacketQueued();

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_RUN_IDLE implementation

zapPacketBufferSourceState* zapPacketBufferSource_RUN_IDLE::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSource_RUN_IDLE;
  }
  return mInstance;
}

zapPacketBufferSourceState* zapPacketBufferSource_RUN_IDLE::mInstance = nsnull;

void zapPacketBufferSource_RUN_IDLE::PacketDequeued(zapPacketBuffer* pb)
{
  if (pb->mSource && pb->mBuffer.GetSize() < pb->mMaxSize) {
    pb->PostFrameRequest();
    ChangeState(pb, zapPacketBufferSource_RUN_WAITING::Instance());
  }
}

void zapPacketBufferSource_RUN_IDLE::RunQueueing(zapPacketBuffer* pb)
{
  // already running
}

void zapPacketBufferSource_RUN_IDLE::StopQueueing(zapPacketBuffer* pb)
{
  ChangeState(pb, zapPacketBufferSource_STOP_IDLE::Instance());
}

nsresult
zapPacketBufferSource_RUN_IDLE::ConnectSource(zapPacketBuffer* pb,
                                              zapIMediaSource* source,
                                              const nsACString & connection_id)
{
  if (pb->mSource) {
    NS_ERROR("source already connected");
    return NS_ERROR_FAILURE;
  }

  pb->mSource = source;

  if (pb->mBuffer.GetSize() < pb->mMaxSize) {
    pb->PostFrameRequest();
    ChangeState(pb, zapPacketBufferSource_RUN_WAITING::Instance());
  }
  
  return NS_OK;
}

nsresult
zapPacketBufferSource_RUN_IDLE::DisconnectSource(zapPacketBuffer* pb,
                                                 zapIMediaSource* source,
                                                 const nsACString& connection_id)
{
  pb->mSource = nsnull;
  // enqueue an EOF frame:
  pb->mBuffer.Push(nsnull);
  pb->PacketQueued();
  return NS_OK;
}



////////////////////////////////////////////////////////////////////////
// zapPacketBufferSinkState implementation

nsresult
zapPacketBufferSinkState::ConnectSink(zapPacketBuffer* pb, zapIMediaSink* sink,
                                      const nsACString& connection_id)
{
#ifdef DEBUG
  printf("zapPacketBufferSinkState::ConnectSink: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;  
}

nsresult
zapPacketBufferSinkState::DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                         const nsACString & connection_id)
{
#ifdef DEBUG
  printf("zapPacketBufferSinkState::DisconnectSink: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;  
}

nsresult
zapPacketBufferSinkState::RequestFrame(zapPacketBuffer* pb)
{
#ifdef DEBUG
  printf("zapPacketBufferSinkState::RequestFrame: protocol error in state %s\n",
         GetName());
#endif
  return NS_ERROR_FAILURE;  
}

void zapPacketBufferSinkState::ChangeState(zapPacketBuffer* pb,
                                           zapPacketBufferSinkState* state)
{
  pb->ChangeSinkState(state);
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_WAITING_PREFILLING implementation

zapPacketBufferSinkState*
zapPacketBufferSink_WAITING_PREFILLING::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSink_WAITING_PREFILLING;
  }
  return mInstance;
}

zapPacketBufferSinkState*
zapPacketBufferSink_WAITING_PREFILLING::mInstance = nsnull;

void zapPacketBufferSink_WAITING_PREFILLING::PacketQueued(zapPacketBuffer* pb)
{
  if (pb->mBuffer.GetSize() >= pb->mPrefillSize ||
      (zapIMediaFrame*)pb->mBuffer.Peek() == nsnull) {
    // we are finished prefilling or the stream has ended.
    // start dequeuing:
    ChangeState(pb, zapPacketBufferSink_IDLE::Instance());
    zapIMediaFrame* frame = (zapIMediaFrame*)pb->mBuffer.PopFront();
    pb->mSink->ProcessFrame(frame);
    NS_IF_RELEASE(frame);
    pb->PacketDequeued();
  }
}

nsresult
zapPacketBufferSink_WAITING_PREFILLING::DisconnectSink(zapPacketBuffer* pb,
                                                       zapIMediaSink *sink,
                                                       const nsACString & connection_id)
{
  pb->mSink = nsnull;
  pb->StopQueueing();
  pb->ClearBuffer();
  ChangeState(pb, zapPacketBufferSink_IDLE_PREFILLING::Instance());
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_WAITING implementation

zapPacketBufferSinkState*
zapPacketBufferSink_WAITING::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSink_WAITING;
  }
  return mInstance;
}

zapPacketBufferSinkState*
zapPacketBufferSink_WAITING::mInstance = nsnull;

void zapPacketBufferSink_WAITING::PacketQueued(zapPacketBuffer* pb)
{
  NS_ASSERTION(pb->mBuffer.GetSize(), "packet queued, but empty queue");
  zapIMediaFrame* frame = (zapIMediaFrame*)pb->mBuffer.PopFront();
  if (!frame) {
    // An EOF frame. Make sure next stream gets prefilled again:
    ChangeState(pb, zapPacketBufferSink_IDLE_PREFILLING::Instance());
  }
  else
    ChangeState(pb, zapPacketBufferSink_IDLE::Instance());
  pb->mSink->ProcessFrame(frame);
  NS_IF_RELEASE(frame);
  pb->PacketDequeued();
}

nsresult
zapPacketBufferSink_WAITING::DisconnectSink(zapPacketBuffer* pb,
                                            zapIMediaSink *sink,
                                            const nsACString & connection_id)
{
  pb->mSink = nsnull;
  pb->StopQueueing();
  pb->ClearBuffer();
  ChangeState(pb, zapPacketBufferSink_IDLE_PREFILLING::Instance());
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_IDLE implementation

zapPacketBufferSinkState*
zapPacketBufferSink_IDLE::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSink_IDLE;
  }
  return mInstance;
}

zapPacketBufferSinkState*
zapPacketBufferSink_IDLE::mInstance = nsnull;

void zapPacketBufferSink_IDLE::PacketQueued(zapPacketBuffer* pb)
{
  // do nothing
}


nsresult
zapPacketBufferSink_IDLE::DisconnectSink(zapPacketBuffer* pb,
                                         zapIMediaSink *sink,
                                         const nsACString & connection_id)
{
  pb->mSink = nsnull;
  pb->StopQueueing();
  pb->ClearBuffer();
  ChangeState(pb, zapPacketBufferSink_IDLE_PREFILLING::Instance());
  return NS_OK;
}

nsresult
zapPacketBufferSink_IDLE::RequestFrame(zapPacketBuffer* pb)
{
  if (pb->mBuffer.GetSize()) {
    zapIMediaFrame* frame = (zapIMediaFrame*)pb->mBuffer.PopFront();
    if (!frame) {
      // An EOF frame. Make sure next stream gets prefilled again:
      ChangeState(pb, zapPacketBufferSink_IDLE_PREFILLING::Instance());
    }
    pb->mSink->ProcessFrame(frame);
    NS_IF_RELEASE(frame);
    pb->PacketDequeued();
  }
  else {
    // nothing in buffer
    ChangeState(pb, zapPacketBufferSink_WAITING::Instance());
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_IDLE_PREFILLING implementation

zapPacketBufferSinkState*
zapPacketBufferSink_IDLE_PREFILLING::Instance()
{
  if (!mInstance) {
    mInstance = new zapPacketBufferSink_IDLE_PREFILLING;
  }
  return mInstance;
}

zapPacketBufferSinkState*
zapPacketBufferSink_IDLE_PREFILLING::mInstance = nsnull;

void zapPacketBufferSink_IDLE_PREFILLING::PacketQueued(zapPacketBuffer* pb)
{
  // do nothing
}


nsresult
zapPacketBufferSink_IDLE_PREFILLING::ConnectSink(zapPacketBuffer* pb,
                                                 zapIMediaSink* sink,
                                                 const nsACString& connection_id)
{
  if (pb->mSink) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }
  pb->mSink = sink;
  
  return NS_OK;
}


nsresult
zapPacketBufferSink_IDLE_PREFILLING::DisconnectSink(zapPacketBuffer* pb,
                                                    zapIMediaSink *sink,
                                                    const nsACString & connection_id)
{
  pb->mSink = nsnull;
  pb->StopQueueing();
  pb->ClearBuffer();
  return NS_OK;
}

nsresult
zapPacketBufferSink_IDLE_PREFILLING::RequestFrame(zapPacketBuffer* pb)
{
  pb->RunQueueing();
  
  if (pb->mBuffer.GetSize() <= pb->mPrefillSize) {
    ChangeState(pb, zapPacketBufferSink_WAITING_PREFILLING::Instance());
  }
  else {
    zapIMediaFrame* frame = (zapIMediaFrame*)pb->mBuffer.PopFront();
    if (frame)
      ChangeState(pb, zapPacketBufferSink_IDLE::Instance());
    // else ... prefill next frame
    pb->mSink->ProcessFrame(frame);
    NS_IF_RELEASE(frame);
    pb->PacketDequeued();
  }
  return NS_OK;
}
