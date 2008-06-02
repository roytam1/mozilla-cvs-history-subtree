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

#include "zapPCMPing.h"
#include "math.h"
#include "nsAutoPtr.h"
#include "zapMediaFrame.h"

////////////////////////////////////////////////////////////////////////
// zapPCMPing

zapPCMPing::zapPCMPing()
{
}

zapPCMPing::~zapPCMPing()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapPCMPing)
NS_IMPL_THREADSAFE_RELEASE(zapPCMPing)

NS_INTERFACE_MAP_BEGIN(zapPCMPing)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapPCMPing::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                  nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  mPingPitch = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("ping_pitch"), 1760.0);

  nsresult rv = mStreamParameters.InitWithProperties(node_pars);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mStreamParameters.channels != 1 ||
      mStreamParameters.sample_format != sf_float32_32768) {
    NS_ERROR("unsupported sample format! write me!");
    return NS_ERROR_FAILURE;
  }

  // create our ping frame data (a sine wave with the given pitch and a
  // triangular envelope spanning the frame duration)
  mPingData.SetLength(mStreamParameters.GetFrameLength());
  float *d = (float*)mPingData.BeginWriting();
  double sample_step = 1.0/mStreamParameters.sample_rate;
  double samples_left = mStreamParameters.samples/2;
  double amplitude_step = 3000.0/samples_left;
  double sample_time = 0.0;
  double amplitude = 0.0;
  double omega = 6.28318530718*mPingPitch;
  // ramp up
  while (samples_left--) {
    *d++ = (float)(amplitude*sin(omega*sample_time));
    sample_time += sample_step;
    amplitude += amplitude_step;
  }
  samples_left = mStreamParameters.samples/2;
  // ramp down
  while (samples_left--) {
    *d++ = (float)(amplitude*sin(omega*sample_time));
    sample_time += sample_step;
    amplitude -= amplitude_step;
  }

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapPCMPing::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapPCMPing::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapPCMPing::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapPCMPing::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");

  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPCMPing::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPCMPing::ConsumeFrame(zapIMediaFrame * frame)
{
  if (mOutput) {
    nsRefPtr<zapMediaFrame> oframe = new zapMediaFrame();
    oframe->mStreamInfo = mStreamInfo;
    oframe->mTimestamp = 0; // XXX
    oframe->mData = mPingData;
    mOutput->ConsumeFrame(oframe);
  }
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapPCMPing::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapPCMPing::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapPCMPing::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("not a passive source!");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}
