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

#include "zapFileIn.h"
#include "zapMediaFrame.h"
#include "zapStreamUtils.h"
#include "nsAutoPtr.h"
#include "stdio.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsILocalFile.h"

////////////////////////////////////////////////////////////////////////
// zapFileIn

zapFileIn::zapFileIn()
    : mFile(nsnull)
{
#ifdef DEBUG
  printf("zapFileIn ctor\n");
#endif
}

zapFileIn::~zapFileIn()
{
  NS_ASSERTION(!mFile, "uh oh, cleanup failure");
#ifdef DEBUG
  printf("~zapFileIn\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports implementation:

NS_IMPL_ADDREF(zapFileIn)
NS_IMPL_RELEASE(zapFileIn)

NS_INTERFACE_MAP_BEGIN(zapFileIn)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode implementation:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapFileIn::AddedToGraph(zapIMediaGraph *graph,
                        const nsACString & id,
                        nsIPropertyBag2* node_pars)
{
  // node parameter defaults:
  mBlockSize = 8192;
  mLoop = PR_FALSE;
  nsCString fileSpec;
  
  // unpack node parameters:
  if (!node_pars) return NS_ERROR_FAILURE;
  
  if (NS_FAILED(node_pars->GetPropertyAsAUTF8String(NS_LITERAL_STRING("file_url"),
                                                    fileSpec)))
    return NS_ERROR_FAILURE;
  node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("block_size"), &mBlockSize);
  node_pars->GetPropertyAsBool(NS_LITERAL_STRING("loop"), &mLoop);
  
  // try to open the file:
  nsCOMPtr<nsIIOService> ioService = do_GetService("@mozilla.org/network/io-service;1");
  if (!ioService) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIURI> uri;
  ioService->NewURI(fileSpec, nsnull, nsnull, getter_AddRefs(uri));
  if (!uri) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri);
  if (!fileURL) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIFile> file;
  fileURL->GetFile(getter_AddRefs(file));
  if (!file) return NS_ERROR_FAILURE;
  PRBool exists = PR_FALSE;
  file->Exists(&exists);
  if (!exists) return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file);
  if (!localFile) return NS_ERROR_FAILURE;

  localFile->OpenNSPRFileDesc(PR_RDONLY, 0, &mFile);
  if (!mFile) return NS_ERROR_FAILURE;
  
  // create a new stream info:
  mStreamInfo = CreateStreamInfo(NS_LITERAL_CSTRING("raw"));
  
  return NS_OK;
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapFileIn::RemovedFromGraph(zapIMediaGraph *graph)
{
  if (mFile) {
    PR_Close(mFile);
    mFile = nsnull;
  }
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapFileIn::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapFileIn::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("filein is a source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapFileIn::ConnectSink(zapIMediaSink *sink,
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
zapFileIn::DisconnectSink(zapIMediaSink *sink,
                          const nsACString & connection_id)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapFileIn::ProduceFrame(zapIMediaFrame ** _retval)
{
  *_retval = nsnull;

  if (!mFile) return NS_ERROR_FAILURE;
  
  // construct audio frame:
  nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = 0; // XXX

  if (mLoop && PR_Available(mFile) <= 0)
    PR_Seek(mFile, 0, PR_SEEK_SET); // rewind
  
  // write frame data:
  frame->mData.SetLength(mBlockSize);
  PRUint32 bytesRead = PR_Read(mFile, frame->mData.BeginWriting(), mBlockSize);
  if (bytesRead <= 0) {
    PR_Close(mFile);
    mFile = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  frame->mData.SetLength(bytesRead);
  *_retval = frame;
  NS_ADDREF(*_retval);
  return NS_OK;
}
