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

#include "zapSpeexDecoder.h"
#include "zapIMediaFrame.h"
#include "nsStringAPI.h"
#include "zapZMKImplUtils.h"
#include "zapIAudioIn.h"
#include "zapMediaFrame.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapSpeexDecoder

zapSpeexDecoder::zapSpeexDecoder()
    : mDecoderState(nsnull)
{
}

zapSpeexDecoder::~zapSpeexDecoder()
{
  NS_ASSERTION(!mDecoderState, "unclean shutdown");
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF_INHERITED(zapSpeexDecoder, zapFilterNode)
NS_IMPL_RELEASE_INHERITED(zapSpeexDecoder, zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapSpeexDecoder)
  NS_INTERFACE_MAP_ENTRY(zapISpeexDecoder)
NS_INTERFACE_MAP_END_INHERITING(zapFilterNode)
  
//----------------------------------------------------------------------
// zapISpeexDecoder methods:

  /* attribute boolean penhance; */
NS_IMETHODIMP
zapSpeexDecoder::GetPenhance(PRBool *aPenhance)
{
  int bPenh;
  speex_decoder_ctl(mDecoderState, SPEEX_GET_ENH, &bPenh);
  *aPenhance = (bPenh==1);
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexDecoder::SetPenhance(PRBool aPenhance)
{
  int bPenh = aPenhance;
  speex_decoder_ctl(mDecoderState, SPEEX_SET_ENH, &bPenh);
  return NS_OK;
}

/* readonly attribute unsigned long maxBitrate; */
NS_IMETHODIMP
zapSpeexDecoder::GetMaxBitrate(PRUint32 *aMaxBitrate)
{
  int br;
  speex_decoder_ctl(mDecoderState, SPEEX_GET_BITRATE, &br);
  *aMaxBitrate = (PRUint32)br;
  return NS_OK;
}


//----------------------------------------------------------------------
// Implementation helpers:

NS_IMETHODIMP
zapSpeexDecoder::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                       nsIPropertyBag2 *node_pars)
{
  // default: nb mode
  const SpeexMode* speexmode = &speex_nb_mode;
  mSampleRate = 8000;
  
  // extract node parameters:

  if (node_pars) {
    if (NS_SUCCEEDED(node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                                                    &mSampleRate))) {
      if (mSampleRate == 8000) {
        speexmode = &speex_nb_mode;
      }
      else if (mSampleRate == 16000) {
        speexmode = &speex_wb_mode;
      }
      else if (mSampleRate == 32000) {
        speexmode = &speex_uwb_mode;
      }
      else {
        NS_ERROR("unsupported sample rate");
        return NS_ERROR_FAILURE;
      }
    }
  }
  
  speex_bits_init(&mDecoderBits);
  mDecoderState = speex_decoder_init(speexmode);

  return NS_OK;
}

NS_IMETHODIMP
zapSpeexDecoder::RemovedFromContainer()
{
  speex_bits_destroy(&mDecoderBits);
  speex_decoder_destroy(mDecoderState);
  mDecoderState = nsnull;
  return NS_OK;
}

nsresult
zapSpeexDecoder::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  if (!streamInfo) {
    NS_ERROR("can't open stream without info");
    return NS_ERROR_FAILURE;
  }

  nsCString type;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                 type)) ||
      type != NS_LITERAL_CSTRING("audio/speex")) {
    NS_ERROR("can only decode audio/speex streams");
    return NS_ERROR_FAILURE;
  }
  
  // Stream parameters are ok. 

  // determine output buffer size:
  int frameSize;
  speex_decoder_ctl(mDecoderState, SPEEX_GET_FRAME_SIZE, &frameSize);
  mOutputBufferLength = 4 * frameSize; // float32 samples
  
  // Create a new streaminfo:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/pcm");
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                                   mSampleRate);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("samples"),
                                   160);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                   1);
  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                     NS_LITERAL_CSTRING("float32_32768"));

  
  return NS_OK;
}

nsresult
zapSpeexDecoder::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  // flush old bits:
  speex_bits_reset(&mDecoderBits);

  // create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  input->GetTimestamp(&frame->mTimestamp);
    
  // decode frame:
  nsCString data;
  input->GetData(data);
  speex_bits_read_from(&mDecoderBits, (char*)data.BeginReading(), data.Length());
  frame->mData.SetLength(mOutputBufferLength);
  speex_decode(mDecoderState, &mDecoderBits, (float*)frame->mData.BeginWriting());
  
  *output = frame;
  return NS_OK;
}
