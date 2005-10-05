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

#include "zapSplitter.h"
#include "zapIMediaGraph.h"
#include "nsHashPropertyBag.h"
#include "nsAutoPtr.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////
// zapSplitterOutput

class zapSplitterOutput : public zapIMediaSource
{
public:
  zapSplitterOutput();
  ~zapSplitterOutput();

  void Init(zapSplitter* splitter);
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE

  void FrameAvailable();
  
private:
  nsRefPtr<zapSplitter> mSplitter;
  nsCOMPtr<zapIMediaSink> mSink;

  PRBool mWaiting;
  PRBool mFrameAvailable;
};

//----------------------------------------------------------------------
zapSplitterOutput::zapSplitterOutput()
    : mWaiting(PR_FALSE),
      mFrameAvailable(PR_FALSE)
{
#ifdef DEBUG
  printf("zapSplitterOutput::zapSplitterOutput()\n");
#endif
}

zapSplitterOutput::~zapSplitterOutput()
{
  NS_ASSERTION(mSplitter, "Never initialized");
#ifdef DEBUG
  printf("zapSplitterOutput::~zapSplitterOutput()\n");
#endif
  // clean up references:
  mSplitter->mOutputs.RemoveElement(this);
  mSplitter = nsnull;
}

void zapSplitterOutput::Init(zapSplitter* splitter) {
  mSplitter = splitter;
  // append ourselves to the splitter's array of inputs:
  mSplitter->mOutputs.AppendElement(this);
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapSplitterOutput)
NS_IMPL_RELEASE(zapSplitterOutput)

NS_INTERFACE_MAP_BEGIN(zapSplitterOutput)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapSplitterOutput::ConnectSink(zapIMediaSink *sink,
                               const nsACString & connection_id)
{
  NS_ASSERTION(!mSink, "sink already connected");
  mSink = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapSplitterOutput::DisconnectSink(zapIMediaSink *sink,
                                  const nsACString & connection_id)
{
  mSink = nsnull;
  return NS_OK;
}

/* void requestFrame (); */
NS_IMETHODIMP
zapSplitterOutput::RequestFrame()
{
  if (mWaiting) return NS_OK; // we're already waiting for data

  if (mFrameAvailable) {
    mFrameAvailable = PR_FALSE;
    mSink->ProcessFrame(mSplitter->mFrame);
  }
  else {
    mWaiting = PR_TRUE;
    // request next frame:
    mSplitter->RequestFrame();
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------

void zapSplitterOutput::FrameAvailable()
{
  if (!mWaiting) {
    // we're not ready to accept a frame
#ifdef DEBUG
    if (mFrameAvailable)
      printf("overflow in splitter output %p\n", this);
#endif
    mFrameAvailable = PR_TRUE;
  }
  else {
    mWaiting = PR_FALSE;
    if (mSink) {
      mFrameAvailable = PR_FALSE;
      mSink->ProcessFrame(mSplitter->mFrame);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// zapSplitter

zapSplitter::zapSplitter()
    : mWaiting(PR_FALSE),
      mProcessing(PR_FALSE)
{
#ifdef DEBUG
  printf("zapSplitter::zapSplitter()\n");
#endif
}

zapSplitter::~zapSplitter()
{
#ifdef DEBUG
  printf("zapSplitter::~zapSplitter()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapSplitter)
NS_IMPL_RELEASE(zapSplitter)

NS_INTERFACE_MAP_BEGIN(zapSplitter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapSplitter::AddedToGraph(zapIMediaGraph *graph,
                          const nsACString & id,
                          nsIPropertyBag2* node_pars)
{
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapSplitter::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapSplitter::GetSource(nsIPropertyBag2 *source_pars,
                       zapIMediaSource **_retval)
{
  zapSplitterOutput* output = new zapSplitterOutput();
  output->AddRef();
  output->Init(this);
  *_retval = output;
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapSplitter::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
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

/* void connectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapSplitter::ConnectSource(zapIMediaSource *source,
                           const nsACString & connection_id)
{
  NS_ASSERTION(!mInput, "sink already connected");
  mInput = source;
  
  if (mWaiting)
    mInput->RequestFrame();
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source, in ACString connection_id); */
NS_IMETHODIMP
zapSplitter::DisconnectSource(zapIMediaSource *source,
                              const nsACString & connection_id)
{
  mInput = nsnull;
  return NS_OK;
}

/* void processFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapSplitter::ProcessFrame(zapIMediaFrame *frame)
{
  NS_ASSERTION(mWaiting, "uh-oh, unexpectatly received a frame");
  mFrame = frame;
  mWaiting = PR_FALSE;

  // make sure we don't synchronously request new frames while
  // updating outputs:
  mProcessing = PR_TRUE;
  // inform outputs:
  for (int i=0, l=mOutputs.Count(); i<l; ++i) {
    ((zapSplitterOutput*)mOutputs[i])->FrameAvailable();
  }
  mProcessing = PR_FALSE;

  // check if any of our outputs requested a new frame while updating:
  if (mWaiting) {
    // yes. request the frame now:
    mInput->RequestFrame();
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------

void zapSplitter::RequestFrame()
{
  if (mWaiting) return; // already waiting for frame
  mWaiting = PR_TRUE;
  if (!mProcessing && mInput) {
    // request new frame now:
    mInput->RequestFrame();
  }
}
