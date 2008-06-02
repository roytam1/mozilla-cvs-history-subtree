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

#include "zapClockReducer.h"
#include "math.h"
#include "nsAutoPtr.h"
#include "zapMediaFrame.h"
#include "zapZMKImplUtils.h"

////////////////////////////////////////////////////////////////////////
// zapClockReducer

zapClockReducer::zapClockReducer()
    : mEpsilon(0)
{
}

zapClockReducer::~zapClockReducer()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapClockReducer)
NS_IMPL_THREADSAFE_RELEASE(zapClockReducer)

NS_INTERFACE_MAP_BEGIN(zapClockReducer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapClockReducer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                       nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  mNumerator = ZMK_GetOptionalUint32(node_pars, NS_LITERAL_STRING("numerator"), 1);
  mDenominator = ZMK_GetOptionalUint32(node_pars, NS_LITERAL_STRING("denominator"), 1);

  if (mNumerator > mDenominator) return NS_ERROR_FAILURE;

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapClockReducer::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapClockReducer::GetSource(nsIPropertyBag2 *source_pars,
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
zapClockReducer::GetSink(nsIPropertyBag2 *sink_pars,
                         zapIMediaSink **_retval)
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

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapClockReducer::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapClockReducer::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapClockReducer::ConsumeFrame(zapIMediaFrame * frame)
{
  mEpsilon += mNumerator;
  if (mEpsilon<<1 >= mDenominator) {
    if (mOutput) mOutput->ConsumeFrame(frame);
    mEpsilon -= mDenominator;
  }

  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapClockReducer::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapClockReducer::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapClockReducer::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("not a passive source!");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}
