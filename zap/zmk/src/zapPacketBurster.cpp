/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Joost Technologies B.V. code.
 *
 * The Initial Developer of the Original Code is Joost Technologies B.V.
 * Portions created by the Initial Developer are Copyright (C) 2007
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

#include "zapPacketBurster.h"
#include "zapIMediaFrame.h"
#include "nsAutoPtr.h"

////////////////////////////////////////////////////////////////////////
// zapPacketBursterPumpEvent

NS_IMETHODIMP
zapPacketBursterPumpEvent::Run()
{
  if (!mBurster) return NS_OK; // we've been revoked
  
  nsCOMPtr<zapIMediaFrame> frame;
  if (!mBurster->mInput ||
      NS_FAILED(mBurster->mInput->ProduceFrame(getter_AddRefs(frame)))) {
    // failure to produce a frame. Forget us (if we haven't been
    // revoked already:
    if (mBurster)
      mBurster->mPumpEvent.Forget();
    return NS_OK;    
  }

  // we might have been reentrantly revoked at this point:
  if (!mBurster) return NS_OK;
  
  if (mBurster->mOutput) {
    mBurster->mOutput->ConsumeFrame(frame);
    // again, we might have been revoked:
    if (!mBurster) return NS_OK;
  }

  // keep pumping:
  NS_ASSERTION(mBurster, "unexpected state");
  if (NS_FAILED(NS_DispatchToCurrentThread(this))) {
    NS_ERROR("Failed to post event");
    mBurster->mPumpEvent.Forget();
  }

  return NS_OK;
}

void
zapPacketBursterPumpEvent::Revoke()
{
  mBurster = nsnull;
}

////////////////////////////////////////////////////////////////////////
// zapPacketBurster

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapPacketBurster)
NS_IMPL_RELEASE(zapPacketBurster)

NS_INTERFACE_MAP_BEGIN(zapPacketBurster)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIPacketBurster)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapPacketBurster::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                        nsIPropertyBag2* node_pars)
{
  mContainer = container;

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapPacketBurster::RemovedFromContainer()
{
  mPumpEvent.Revoke();
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapPacketBurster::GetSource(nsIPropertyBag2 *source_pars,
                            zapIMediaSource **_retval)
{
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapPacketBurster::GetSink(nsIPropertyBag2 *sink_pars,
                          zapIMediaSink **_retval)
{
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketBurster::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketBurster::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  mPumpEvent.Revoke();
  
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPacketBurster::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe try without the packet burster?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapPacketBurster::ConnectSink(zapIMediaSink *sink)
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
zapPacketBurster::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapPacketBurster::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIPacketBurster methods:

/* void start (); */
NS_IMETHODIMP
zapPacketBurster::Start()
{
#ifdef DEBUG
  nsCOMPtr<nsIThread> currentThread = do_GetCurrentThread();
  nsCOMPtr<nsIEventTarget> containerET;
  mContainer->GetEventTarget(getter_AddRefs(containerET));
  NS_ASSERTION(SameCOMIdentity(currentThread, containerET),
               "uh-oh, we're being called from a unknown thread");
#endif
  
  if (mPumpEvent.IsPending()) return NS_OK;
  nsRefPtr<zapPacketBursterPumpEvent> ev = new zapPacketBursterPumpEvent(this);
  if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
    NS_ERROR("Failed to dispatch event.");
    return NS_ERROR_FAILURE;
  }

  mPumpEvent = ev;
  return NS_OK;
}

/* void stop (); */
NS_IMETHODIMP
zapPacketBurster::Stop()
{
#ifdef DEBUG
  nsCOMPtr<nsIThread> currentThread = do_GetCurrentThread();
  nsCOMPtr<nsIEventTarget> containerET;
  mContainer->GetEventTarget(getter_AddRefs(containerET));
  NS_ASSERTION(SameCOMIdentity(currentThread, containerET),
               "uh-oh, we're being called from a unknown thread");
#endif
  
  mPumpEvent.Revoke();
  return NS_OK;
}

/* readonly attribute boolean isRunning; */
NS_IMETHODIMP
zapPacketBurster::GetIsRunning(PRBool *aIsRunning)
{
#ifdef DEBUG
  nsCOMPtr<nsIThread> currentThread = do_GetCurrentThread();
  nsCOMPtr<nsIEventTarget> containerET;
  mContainer->GetEventTarget(getter_AddRefs(containerET));
  NS_ASSERTION(SameCOMIdentity(currentThread, containerET),
               "uh-oh, we're being called from a unknown thread");
#endif
  
  *aIsRunning = mPumpEvent.IsPending();
  return NS_OK;
}
