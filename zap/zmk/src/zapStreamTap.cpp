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

#include "zapStreamTap.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "math.h"
#include "zapZMKImplUtils.h"

////////////////////////////////////////////////////////////////////////
// zapStreamTapOutput

class zapStreamTapOutput : public zapIMediaSource
{
public:
  zapStreamTapOutput();
  ~zapStreamTapOutput();

  void Init(zapStreamTap* StreamTap);
  void ConsumeFrame(zapIMediaFrame *frame);
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE

private:
  nsRefPtr<zapStreamTap> mStreamTap;
  nsCOMPtr<zapIMediaSink> mOutput;
};

//----------------------------------------------------------------------

zapStreamTapOutput::zapStreamTapOutput()
{
}

zapStreamTapOutput::~zapStreamTapOutput()
{
  NS_ASSERTION(mStreamTap, "Never initialized");
  // clean up references:
  mStreamTap->mOutputs.RemoveElement(this);
  mStreamTap = nsnull;
}

void zapStreamTapOutput::Init(zapStreamTap* streamTap) {
  mStreamTap = streamTap;
  // append ourselves to the StreamTap's array of inputs:
  mStreamTap->mOutputs.AppendElement(this);
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStreamTapOutput)
NS_IMPL_RELEASE(zapStreamTapOutput)

NS_INTERFACE_MAP_BEGIN(zapStreamTapOutput)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamTapOutput::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamTapOutput::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStreamTapOutput::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

void zapStreamTapOutput::ConsumeFrame(zapIMediaFrame *frame)
{
  if (mOutput)
    mOutput->ConsumeFrame(frame);
}


////////////////////////////////////////////////////////////////////////
// zapStreamTap

zapStreamTap::zapStreamTap()
{
}

zapStreamTap::~zapStreamTap()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStreamTap)
NS_IMPL_RELEASE(zapStreamTap)

NS_INTERFACE_MAP_BEGIN(zapStreamTap)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStreamTap::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                    nsIPropertyBag2* node_pars)
{
  mContainer = container;
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapStreamTap::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStreamTap::GetSource(nsIPropertyBag2 *source_pars,
                        zapIMediaSource **_retval)
{
  nsCString sourceType;
  if (!source_pars ||
      NS_FAILED(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                   sourceType)))
    return NS_ERROR_FAILURE;

  if (sourceType == NS_LITERAL_CSTRING("master")) {
    *_retval = this;
    NS_ADDREF(*_retval);
  }
  else if (sourceType == NS_LITERAL_CSTRING("tap")) {
      zapStreamTapOutput* output = new zapStreamTapOutput();
      output->AddRef();
      output->Init(this);
      *_retval = output;
  }
  else
    return NS_ERROR_FAILURE;
  
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapStreamTap::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (mInput) {
    NS_ERROR("input end already connected");
    *_retval = nsnull;
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
zapStreamTap::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "sink already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStreamTap::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStreamTap::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("not a passive sink");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamTap::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mMasterOutput, "already connected");
  mMasterOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamTap::DisconnectSink(zapIMediaSink *sink)
{
  mMasterOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStreamTap::ProduceFrame(zapIMediaFrame ** frame)
{
  if (!mInput || NS_FAILED(mInput->ProduceFrame(frame)))
    return NS_ERROR_FAILURE;

  zapMediaNodeContainerAutoLock lock(mContainer, this);

  // pass output to taps:
  for (int i=0, l=mOutputs.Count(); i<l; ++i) {
    ((zapStreamTapOutput*)mOutputs[i])->ConsumeFrame(*frame);
  }
  
  return NS_OK;
}
