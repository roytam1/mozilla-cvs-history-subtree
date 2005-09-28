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

#include "zapG711Encoder.h"
#include "zapIMediaFrame.h"
#include "zapIMediaGraph.h"
#include "nsIPropertyBag2.h"
#include "zapMediaFrame.h"
#include "stdio.h"
#include "nsHashPropertyBag.h"
#include "g711.h"

////////////////////////////////////////////////////////////////////////
// zapG711Encoder

zapG711Encoder::zapG711Encoder()
{
#ifdef DEBUG
  printf("zapG711Encoder::zapG711Encoder()");
#endif
}

zapG711Encoder::~zapG711Encoder()
{
#ifdef DEBUG
  printf("zapG711Encoder::~zapG711Encoder()");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF_INHERITED(zapG711Encoder, zapFilterNode)
NS_IMPL_RELEASE_INHERITED(zapG711Encoder, zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapG711Encoder)
NS_INTERFACE_MAP_END_INHERITING(zapFilterNode)
  
//----------------------------------------------------------------------

NS_IMETHODIMP
zapG711Encoder::AddedToGraph(zapIMediaGraph *graph, const nsACString & id, nsIPropertyBag2 *node_pars)
{
  return NS_OK;
}

NS_IMETHODIMP
zapG711Encoder::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

nsresult
zapG711Encoder::OpenStream(nsIPropertyBag2* streamInfo)
{
  if (!streamInfo) {
    NS_ERROR("can't open stream without info");
    return NS_ERROR_FAILURE;
  }

  nsCString type;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                 type)) ||
      type != NS_LITERAL_CSTRING("audio")) {
    NS_ERROR("can only encode audio streams");
    return NS_ERROR_FAILURE;
  }

  double sampleRate;
  if (NS_FAILED(streamInfo->GetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                                &sampleRate)) ||
      sampleRate != 8000.0) {
    NS_ERROR("unsupported sample rate");
#ifdef DEBUG
    printf("%f != 8000\n", sampleRate);
#endif
    return NS_ERROR_FAILURE;
  }

  double frameDuration;
  if (NS_FAILED(streamInfo->GetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                                &frameDuration)) ||
      frameDuration != 0.02) {
    NS_ERROR("unsupported frame duration");
    return NS_ERROR_FAILURE;
  }

  PRUint32 numOutputChannels;
  if (NS_FAILED(streamInfo->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                                &numOutputChannels)) ||
      numOutputChannels != 1) {
    NS_ERROR("unsupported channel count");
    return NS_ERROR_FAILURE;
  }


  nsCString sampleFormat;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                sampleFormat)) ||
      sampleFormat != NS_LITERAL_CSTRING("float32_32768")) {
    NS_ERROR("unsupported sample format");
    return NS_ERROR_FAILURE;
  }
  
  // Stream parameters are ok.

  // Create a new streaminfo:
  nsCOMPtr<nsIWritablePropertyBag> bag;
  NS_NewHashPropertyBag(getter_AddRefs(bag));
  mStreamInfo = do_QueryInterface(bag);
  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("type"),
                                     NS_LITERAL_CSTRING("audio/pcmu"));
  
  return NS_OK;
}

void
zapG711Encoder::CloseStream()
{
}

nsresult
zapG711Encoder::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  // Create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  input->GetTimestamp(&frame->mTimestamp);

  //convert data
  nsCString data;
  input->GetData(data);
  int samples = data.Length()/4;
  frame->mData.SetLength(samples);
  PRUint8* d = (PRUint8*)frame->mData.BeginWriting();
  float* s = (float*)data.BeginReading();
  while (samples--)
    *d++ = linear2ulaw((PRInt32)*s++);

  *output = frame;
  return NS_OK;
}
