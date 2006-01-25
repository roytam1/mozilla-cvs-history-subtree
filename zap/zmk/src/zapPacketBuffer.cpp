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
      mLifting(PR_TRUE)
{
#ifdef DEBUG_afri_zmk
  printf("zapPacketBuffer::zapPacketBuffer()\n");
#endif
}

zapPacketBuffer::~zapPacketBuffer()
{
  ClearBuffer();
#ifdef DEBUG_afri_zmk
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
  // node parameter defaults:
  mLiftCount = 0;
  mDropCount = 1;
  mMaxSize = 10;
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("lift_count"), &mLiftCount);
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("drop_count"), &mDropCount);
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("max_size"), &mMaxSize);

    if (mLiftCount > mMaxSize) {
      NS_WARNING("adjusted lift count");
      mLiftCount = mMaxSize;
    }
    if (mDropCount > mMaxSize) {
      NS_WARNING("adjusting drop count");
      mDropCount = mMaxSize;
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
zapPacketBuffer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapPacketBuffer::ConnectSource(zapIMediaSource *source,
                               const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::DisconnectSource(zapIMediaSource *source,
                                  const nsACString & connection_id)
{
  mInput = nsnull;

  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPacketBuffer::ConsumeFrame(zapIMediaFrame * frame)
{
  if (mBuffer.GetSize() >= mMaxSize) {
#ifdef DEBUG_afri_zmk
    //printf("O");
#endif
    if (!mDropCount)
      return NS_ERROR_FAILURE;
    // ... else pop mDropCount packets from front:
    PRUint32 c = mDropCount;
    while (c--)
      ((zapIMediaFrame*)mBuffer.PopFront())->Release();
    // and ... proceed with enqueuing the new packet
  }
  // enqueue packet:
  NS_IF_ADDREF(frame);
  mBuffer.Push(frame);

  if (mLifting && mBuffer.GetSize() >= mLiftCount)
    mLifting = PR_FALSE;
  
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapPacketBuffer::ConnectSink(zapIMediaSink *sink,
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
zapPacketBuffer::DisconnectSink(zapIMediaSink *sink,
                                const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapPacketBuffer::ProduceFrame(zapIMediaFrame ** frame)
{
  if (mLifting || !mBuffer.GetSize()) {
#ifdef DEBUG_afri_zmk
//    printf("U");
#endif
    *frame = nsnull;
    return NS_ERROR_FAILURE;
  }
  *frame = (zapIMediaFrame*)mBuffer.PopFront();
  if (!mBuffer.GetSize())
    mLifting = PR_TRUE;
  return NS_OK;
}

//----------------------------------------------------------------------
// Implementation helpers:

void zapPacketBuffer::ClearBuffer()
{
  PacketDeallocator deallocator;
  mBuffer.ForEach(deallocator);
  mBuffer.Empty();
  mLifting = PR_TRUE;
}



