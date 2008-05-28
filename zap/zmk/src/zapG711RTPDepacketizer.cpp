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

#include "zapG711RTPDepacketizer.h"
#include "zapRTPFrame.h"
#include "zapIMediaNodeContainer.h"
#include "nsStringAPI.h"
#include "zapMediaUtils.h"
#include "zapMediaFrame.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapG711RTPDepacketizer

zapG711RTPDepacketizer::zapG711RTPDepacketizer()
{
}

zapG711RTPDepacketizer::~zapG711RTPDepacketizer()
{
}

NS_IMETHODIMP
zapG711RTPDepacketizer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                              nsIPropertyBag2 *node_pars)
{
  // default: pcmu
  mType = pcmu;
  
  // extract node parameters:

  if (node_pars) {
    nsCAutoString type;
    if (NS_SUCCEEDED(node_pars->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                    type))) {
      if (type == NS_LITERAL_CSTRING("audio/pcmu")) {
        mType = pcmu;
      }
      else if (type == NS_LITERAL_CSTRING("audio/pcma")) {
        mType = pcma;
      }
      else {
        NS_ERROR("unsupported type");
        return NS_ERROR_FAILURE;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
zapG711RTPDepacketizer::RemovedFromContainer()
{
  return NS_OK;
}

nsresult
zapG711RTPDepacketizer::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  if (!streamInfo) {
    NS_ERROR("can't open stream without info");
    return NS_ERROR_FAILURE;
  }

  nsCString type;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                 type)) ||
      type != NS_LITERAL_CSTRING("rtp")) {
    NS_ERROR("can only convert rtp streams");
    return NS_ERROR_FAILURE;
  }

  // Stream parameters are ok. 
  
  // Create a new streaminfo:
  if (mType == pcmu) {
    ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/pcmu");
  } else {
    ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/pcma");
  }
  
  return NS_OK;
}

nsresult
zapG711RTPDepacketizer::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  nsCOMPtr<zapIRTPFrame> rtpFrame = do_QueryInterface(input);
  if (!rtpFrame) {
    NS_ERROR("stream format error");
    *output = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  // create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  rtpFrame->GetPayload(frame->mData);
  rtpFrame->GetTimestamp(&frame->mTimestamp);
  
  *output = frame;
  return NS_OK;
}
