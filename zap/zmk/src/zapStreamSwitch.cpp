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

#include "zapStreamSwitch.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "math.h"
#include "zapIRTPFrame.h"

////////////////////////////////////////////////////////////////////////
// zapStreamSwitchOutput

class zapStreamSwitchOutput : public zapIMediaSource
{
public:
  zapStreamSwitchOutput();
  ~zapStreamSwitchOutput();

  nsresult Init(zapStreamSwitch* streamSwitch, nsIPropertyBag2* source_pars);
  nsresult ConsumeFrame(zapIMediaFrame *frame);
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE

private:
  nsCString mID;
  nsRefPtr<zapStreamSwitch> mStreamSwitch;
  nsCOMPtr<zapIMediaSink> mOutput;
};

//----------------------------------------------------------------------

zapStreamSwitchOutput::zapStreamSwitchOutput()
{
}

zapStreamSwitchOutput::~zapStreamSwitchOutput()
{
  NS_ASSERTION(mStreamSwitch, "Never initialized");
  // clean up references:
  if (mStreamSwitch) {
    mStreamSwitch->mOutputs.Remove(mID);
    if (mStreamSwitch->mSelectedOutput == this)
      mStreamSwitch->mSelectedOutput = nsnull;
    mStreamSwitch = nsnull;
  }
}

nsresult
zapStreamSwitchOutput::Init(zapStreamSwitch* streamSwitch,
                            nsIPropertyBag2* source_pars) {
  // unpack source pars:
  if (!source_pars ||
      NS_FAILED(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("id"),
                                                   mID))) {
    return NS_ERROR_FAILURE;
  }

  // check if an output for this payload type already exists:
  zapStreamSwitchOutput* dummy;
  if (streamSwitch->mOutputs.Get(mID, &dummy))
    return NS_ERROR_FAILURE;
  
  mStreamSwitch = streamSwitch;
  // append ourselves to the StreamSwitch's array of inputs:
  mStreamSwitch->mOutputs.Put(mID, this);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapStreamSwitchOutput)
NS_IMPL_RELEASE(zapStreamSwitchOutput)

NS_INTERFACE_MAP_BEGIN(zapStreamSwitchOutput)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamSwitchOutput::ConnectSink(zapIMediaSink *sink)
{
  NS_ASSERTION(!mOutput, "already connected");
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapStreamSwitchOutput::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapStreamSwitchOutput::ProduceFrame(zapIMediaFrame ** frame)
{
  if (!mStreamSwitch ||
      mStreamSwitch->mSelectedOutput != this ||
      !mStreamSwitch->mInput) {
    *frame = nsnull;
    return NS_ERROR_FAILURE;
  }
  return mStreamSwitch->mInput->ProduceFrame(frame);
}

//----------------------------------------------------------------------

nsresult zapStreamSwitchOutput::ConsumeFrame(zapIMediaFrame *frame)
{
  if (!mOutput) return NS_ERROR_FAILURE;
  return mOutput->ConsumeFrame(frame);  
}


////////////////////////////////////////////////////////////////////////
// zapStreamSwitch

zapStreamSwitch::zapStreamSwitch()
    : mSelectedOutput(nsnull)
{
  mOutputs.Init();
}

zapStreamSwitch::~zapStreamSwitch()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapStreamSwitch)
NS_IMPL_THREADSAFE_RELEASE(zapStreamSwitch)

NS_INTERFACE_MAP_BEGIN(zapStreamSwitch)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIStreamSwitch)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapStreamSwitch::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                       nsIPropertyBag2* node_pars)
{
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapStreamSwitch::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapStreamSwitch::GetSource(nsIPropertyBag2 *source_pars,
                           zapIMediaSource **_retval)
{
  zapStreamSwitchOutput* output = new zapStreamSwitchOutput();
  output->AddRef();
  if (NS_FAILED(output->Init(this, source_pars))) {
    output->Release();
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  *_retval = output;
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapStreamSwitch::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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
zapStreamSwitch::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "sink already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapStreamSwitch::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapStreamSwitch::ConsumeFrame(zapIMediaFrame * frame)
{
  if (!mSelectedOutput) return NS_ERROR_FAILURE;
  return mSelectedOutput->ConsumeFrame(frame);
}

//----------------------------------------------------------------------
// zapIStreamSwitch methods:

/* void selectOutput (in ACString id); */
NS_IMETHODIMP
zapStreamSwitch::SelectOutput(const nsACString & id)
{
  zapStreamSwitchOutput *output = nsnull;
  mOutputs.Get(id, &output);
  mSelectedOutput = output;
  return NS_OK;
}

