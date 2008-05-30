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

#include "zapAudioStreamUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"

//----------------------------------------------------------------------

zapAudioStreamSampleFormat StrToZapAudioSampleFormat(const nsACString& str)
{
  zapAudioStreamSampleFormat rv = sf_unknown;

  if (str == NS_LITERAL_CSTRING("float32_1")) {
    rv = sf_float32_1;
  }
  else if (str == NS_LITERAL_CSTRING("float32_32768")) {
    rv = sf_float32_32768;
  }
  else if (str == NS_LITERAL_CSTRING("int16")) {
    rv = sf_int16;
  }
  else if (str == NS_LITERAL_CSTRING("int32")) {
    rv = sf_int32;
  }

  return rv;
}

void ZapAudioSampleFormatToStr(zapAudioStreamSampleFormat format, nsACString& str)
{
  switch (format) {
    case sf_float32_1:
      str = NS_LITERAL_CSTRING("float32_1");
      break;
    case sf_float32_32768:
      str = NS_LITERAL_CSTRING("float32_32768");
      break;
    case sf_int16:
      str = NS_LITERAL_CSTRING("int16");
      break;
    case sf_int32:
      str = NS_LITERAL_CSTRING("int32");
      break;
    default:
      str.Truncate();
  }
}

PRUint32 GetZapAudioSampleSize(zapAudioStreamSampleFormat format)
{
  switch (format) {
    case sf_float32_1:
    case sf_float32_32768:
    case sf_int32:
      return 4;
      break;
    case sf_int16:
      return 2;
      break;
    default:
      return 0;
  }
}

PRUint32 GetPortAudioSampleSize(zapAudioStreamSampleFormat format)
{
  switch (format) {
    case sf_float32_32768:
      // special case. this format maps onto portaudio int16 and will
      // be converted in audioin/audioout:
      return 2;
      break;
    case sf_float32_1:
    case sf_int32:
      return 4;
      break;
    case sf_int16:
      return 2;
      break;
    default:
      return 0;
  }
}

PaSampleFormat ZapAudioSampleFormatToPaFormat(zapAudioStreamSampleFormat format)
{
  switch (format) {
    case sf_float32_1:
      return paFloat32;
      break;
    case sf_float32_32768:
      // audioin & audioout will map this onto int16
      return paInt16;
      break;
    case sf_int32:
      return paInt32;
      break;
    case sf_int16:
      return paInt16;
      break;
    default:
      NS_ERROR("unknown format");
  }
  return 0;
}

//----------------------------------------------------------------------

void zapAudioStreamParameters::InitWithDefaults()
{
  sample_rate = 8000;
  samples = 160;
  channels = 1;
  sample_format = sf_float32_32768;
}

nsresult
zapAudioStreamParameters::InitWithProperties(nsIPropertyBag2* properties)
{
  InitWithDefaults();
  if (properties) {
    properties->GetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                                    &sample_rate);
    properties->GetPropertyAsUint32(NS_LITERAL_STRING("samples"),
                                    &samples);
    properties->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                    &channels);
    nsCString sampleformat_string;
    if (NS_SUCCEEDED(properties->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                       sampleformat_string))) {
      sample_format = StrToZapAudioSampleFormat(sampleformat_string);
      if (sample_format == sf_unknown) {
        NS_ERROR("unknown sample format");
        return NS_ERROR_FAILURE;
      }
    }
  }
  return NS_OK;
}

already_AddRefed<nsIWritablePropertyBag2>
zapAudioStreamParameters::CreateStreamInfo()
{
  nsIWritablePropertyBag2 *bag;
  CallCreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID, nsnull,
                     NS_GET_IID(nsIWritablePropertyBag2),
                     (void**)&bag);

  bag->SetPropertyAsACString(NS_LITERAL_STRING("type"),
                             NS_LITERAL_CSTRING("audio/pcm"));
  bag->SetPropertyAsUint32(NS_LITERAL_STRING("sample_rate"),
                           sample_rate);
  bag->SetPropertyAsUint32(NS_LITERAL_STRING("samples"),
                           samples);
  bag->SetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                           channels);

  nsCString format_string;
  ZapAudioSampleFormatToStr(sample_format, format_string);
  bag->SetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                             format_string);
  return bag;
}


//----------------------------------------------------------------------

PRBool CheckAudioStream(nsIPropertyBag2* streamInfo,
                        const zapAudioStreamParameters& pars)
{
  if (!streamInfo) {
    NS_ERROR("null stream info");
    return false;
  }
  nsCString sample_format_str;
  ZapAudioSampleFormatToStr(pars.sample_format, sample_format_str);  
  return ZMK_VerifyStreamType(streamInfo, NS_LITERAL_CSTRING("audio/pcm")) &&
         ZMK_VerifyUint32Property(streamInfo,
                                  NS_LITERAL_STRING("sample_rate"), pars.sample_rate) &&
         ZMK_VerifyUint32Property(streamInfo,
                                  NS_LITERAL_STRING("samples"), pars.samples) &&
         ZMK_VerifyUint32Property(streamInfo,
                             NS_LITERAL_STRING("channels"), pars.channels) &&
         ZMK_VerifyCStringProperty(streamInfo,
                                   NS_LITERAL_STRING("sample_format"),
                                   sample_format_str);
}
