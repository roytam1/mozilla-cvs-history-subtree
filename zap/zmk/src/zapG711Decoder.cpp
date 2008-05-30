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

#include "zapG711Decoder.h"
#include "zapIMediaFrame.h"
#include "zapIMediaNodeContainer.h"
#include "nsStringAPI.h"
#include "zapZMKImplUtils.h"
#include "zapMediaFrame.h"
#include "stdio.h"
#include "g711.h"

////////////////////////////////////////////////////////////////////////
// zapG711Decoder

zapG711Decoder::zapG711Decoder()
{
}

zapG711Decoder::~zapG711Decoder()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF_INHERITED(zapG711Decoder, zapFilterNode)
NS_IMPL_RELEASE_INHERITED(zapG711Decoder, zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapG711Decoder)
NS_INTERFACE_MAP_END_INHERITING(zapFilterNode)
  
//----------------------------------------------------------------------

NS_IMETHODIMP
zapG711Decoder::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                      nsIPropertyBag2 *node_pars)
{
  return NS_OK;
}

NS_IMETHODIMP
zapG711Decoder::RemovedFromContainer()
{
  return NS_OK;
}

nsresult
zapG711Decoder::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  if (!streamInfo) {
    NS_ERROR("can't open stream without info");
    return NS_ERROR_FAILURE;
  }

  nsCString type;
  streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"), type);
  if (type == NS_LITERAL_CSTRING("audio/pcmu")) {
    mType = pcmu;
  }
  else if (type == NS_LITERAL_CSTRING("audio/pcma")) {
    mType = pcma;
  }
  else {
    NS_ERROR("unsupported stream type");
    return NS_ERROR_FAILURE;
  }
  
  // Stream parameters are ok. 

  // Create a new streaminfo:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/pcm");
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                                   8000);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("samples"),
                                   160);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                   1);
  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                     NS_LITERAL_CSTRING("float32_32768"));

  
  return NS_OK;
}

nsresult
zapG711Decoder::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  // Create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  input->GetTimestamp(&frame->mTimestamp);

  //convert data
  nsCString data;
  input->GetData(data);
  int samples = data.Length();
  frame->mData.SetLength(samples*4);
  float* d = (float*)frame->mData.BeginWriting();
  PRUint8* s = (PRUint8*)data.BeginReading();
  if (mType == pcmu) {
    while (samples--)
      *d++ = (float)ulaw2linear(*s++);
  }
  else {
    while (samples--)
      *d++ = (float)alaw2linear(*s++);
  }
  *output = frame;
  return NS_OK;
}
