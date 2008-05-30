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

#include "zapSpeexEncoder.h"
#include "zapIMediaFrame.h"
#include "zapIAudioIn.h"
#include "zapMediaFrame.h"
#include "zapZMKImplUtils.h"
#include "stdio.h"
#include "zapAudioStreamUtils.h"

////////////////////////////////////////////////////////////////////////
// zapSpeexEncoder

zapSpeexEncoder::zapSpeexEncoder()
    : mEncoderState(nsnull)
{
}

zapSpeexEncoder::~zapSpeexEncoder()
{
  NS_ASSERTION(!mEncoderState, "unclean shutdown");
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF_INHERITED(zapSpeexEncoder, zapFilterNode)
NS_IMPL_RELEASE_INHERITED(zapSpeexEncoder, zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapSpeexEncoder)
  NS_INTERFACE_MAP_ENTRY(zapISpeexEncoder)
NS_INTERFACE_MAP_END_INHERITING(zapFilterNode)
  
//----------------------------------------------------------------------
// zapISpeexEncoder methods:

/* void SetQuality (in unsigned long quality); */
NS_IMETHODIMP
zapSpeexEncoder::SetQuality(PRUint32 aQuality)
{
  int val = aQuality;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_QUALITY, &val);
  return NS_OK;
}

/* attribute boolean vbr; */
NS_IMETHODIMP
zapSpeexEncoder::GetVbr(PRBool *aVbr)
{
  int bVBR;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_VBR, &bVBR);
  *aVbr = (bVBR==1);
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetVbr(PRBool aVbr)
{
  int bVBR = aVbr;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_VBR, &bVBR);
  return NS_OK;
}

/* attribute float vbrQuality; */
NS_IMETHODIMP
zapSpeexEncoder::GetVbrQuality(float *aVbrQuality)
{
  speex_encoder_ctl(mEncoderState, SPEEX_GET_VBR_QUALITY, aVbrQuality);
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetVbrQuality(float aVbrQuality)
{
  speex_encoder_ctl(mEncoderState, SPEEX_SET_VBR_QUALITY, &aVbrQuality);
  return NS_OK;
}

/* attribute unsigned long complexity; */
NS_IMETHODIMP
zapSpeexEncoder::GetComplexity(PRUint32 *aComplexity)
{
  int complexity;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_COMPLEXITY, &complexity);
  *aComplexity = complexity;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetComplexity(PRUint32 aComplexity)
{
  int complexity = aComplexity;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_COMPLEXITY, &complexity);
  return NS_OK;
}

/* attribute boolean vad; */
NS_IMETHODIMP
zapSpeexEncoder::GetVad(PRBool *aVad)
{
  int bVAD;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_VAD, &bVAD);
  *aVad = (bVAD==1);
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetVad(PRBool aVad)
{
  int bVAD = aVad;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_VAD, &bVAD);
  return NS_OK;
}

/* attribute boolean dtx; */
NS_IMETHODIMP
zapSpeexEncoder::GetDtx(PRBool *aDtx)
{
  int bDTX;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_DTX, &bDTX);
  *aDtx = (bDTX==1);
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetDtx(PRBool aDtx)
{
  int bDTX = aDtx;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_DTX, &bDTX);
  return NS_OK;
}

/* attribute unsigned long averageBitrate; */
NS_IMETHODIMP
zapSpeexEncoder::GetAverageBitrate(PRUint32 *aAverageBitrate)
{
  int abr;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_ABR, &abr);
  *aAverageBitrate = (PRUint32)abr;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetAverageBitrate(PRUint32 aAverageBitrate)
{
  int abr = aAverageBitrate;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_ABR, &abr);
  return NS_OK;
}

/* attribute unsigned long maxBitrate; */
NS_IMETHODIMP
zapSpeexEncoder::GetMaxBitrate(PRUint32 *aMaxBitrate)
{
  int br;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_BITRATE, &br);
  *aMaxBitrate = (PRUint32)br;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetMaxBitrate(PRUint32 aMaxBitrate)
{
  int br = aMaxBitrate;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_BITRATE, &br);
  return NS_OK;
}

/* attribute unsigned long plcTuning; */
NS_IMETHODIMP
zapSpeexEncoder::GetPlcTuning(PRUint32 *aPlcTuning)
{
  int plct;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_PLC_TUNING, &plct);
  *aPlcTuning = (PRUint32)plct;
  return NS_OK;
}
NS_IMETHODIMP
zapSpeexEncoder::SetPlcTuning(PRUint32 aPlcTuning)
{
  int plct = aPlcTuning;
  speex_encoder_ctl(mEncoderState, SPEEX_SET_PLC_TUNING, &plct);
  return NS_OK;
}


//----------------------------------------------------------------------
// Implementation helpers:

NS_IMETHODIMP
zapSpeexEncoder::InsertedIntoContainer(zapIMediaNodeContainer *container,
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
  
  speex_bits_init(&mEncoderBits);
  mEncoderState = speex_encoder_init(speexmode);
  return NS_OK;
}

NS_IMETHODIMP
zapSpeexEncoder::RemovedFromContainer()
{
  speex_bits_destroy(&mEncoderBits);
  speex_encoder_destroy(mEncoderState);
  mEncoderState = nsnull;
  return NS_OK;
}

nsresult
zapSpeexEncoder::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  if (!streamInfo) {
    NS_ERROR("can't open stream without info");
    return NS_ERROR_FAILURE;
  }

  nsCString type;
  if (NS_FAILED(streamInfo->GetPropertyAsACString(NS_LITERAL_STRING("type"),
                                                 type)) ||
      type != NS_LITERAL_CSTRING("audio/pcm")) {
    NS_ERROR("can only encode audio streams");
    return NS_ERROR_FAILURE;
  }
  
  PRUint32 sampleRate;
  if (NS_FAILED(streamInfo->GetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                                                &sampleRate)) ||
      sampleRate != mSampleRate) {
    NS_ERROR("unsupported sample rate");
    return NS_ERROR_FAILURE;
  }

  PRUint32 samples;
  if (NS_FAILED(streamInfo->GetPropertyAsUint32(NS_LITERAL_STRING("samples"),
                                                &samples)) ||
      samples != 160) {
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

  // determine expected buffer size:
  PRUint32 encoderFrameSize;
  speex_encoder_ctl(mEncoderState, SPEEX_GET_FRAME_SIZE, &encoderFrameSize);
  mInputBufferLength = 4 * encoderFrameSize; // float32 samples

  // Create a new streaminfo:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/speex");
  
  return NS_OK;
}

nsresult
zapSpeexEncoder::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  // flush old bits:
  speex_bits_reset(&mEncoderBits);

  // encode frame:
  nsCString data;
  input->GetData(data);
  NS_ASSERTION(mInputBufferLength == data.Length(), "unexpected frame length");
  speex_encode(mEncoderState, (float*)data.BeginReading(), &mEncoderBits);

  // Create frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  input->GetTimestamp(&frame->mTimestamp);
  
  // copy data:
  int length = speex_bits_nbytes(&mEncoderBits);
  frame->mData.SetLength(length);
  speex_bits_write(&mEncoderBits, (char*)frame->mData.BeginWriting(), length);
  *output = frame;
  return NS_OK;
}
