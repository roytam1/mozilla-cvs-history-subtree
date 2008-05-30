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

#ifndef __ZAP_AUDIOSTREAMUTILS_H__
#define __ZAP_AUDIOSTREAMUTILS_H__

#include "nsStringAPI.h"
#include "portaudio.h"
#include "zapZMKImplUtils.h"
#include "nsIWritablePropertyBag2.h"
#include "nsCOMPtr.h"

//----------------------------------------------------------------------
// sample format mapping

enum zapAudioStreamSampleFormat {
  sf_float32_1,
  sf_float32_32768,
  sf_int16,
  sf_int32,
  sf_unknown
};

zapAudioStreamSampleFormat StrToZapAudioSampleFormat(const nsACString& str);
void ZapAudioSampleFormatToStr(zapAudioStreamSampleFormat format, nsACString& str);

PRUint32 GetZapAudioSampleSize(zapAudioStreamSampleFormat format);
PRUint32 GetPortAudioSampleSize(zapAudioStreamSampleFormat format);
                                
PaSampleFormat ZapAudioSampleFormatToPaFormat(zapAudioStreamSampleFormat format);

//----------------------------------------------------------------------
//

struct zapAudioStreamParameters {
  PRUint32 sample_rate;    // sample rate in Hz
  PRUint32 samples;        // number of samples/frame *per channel*
  PRUint32 channels;       // number of channels
  zapAudioStreamSampleFormat sample_format; 

  // initialize with default parameters (sample_rate: 8000,
  // samples: 160, channels: 1, sample_format:
  // sf_float32_32768)
  void InitWithDefaults();

  nsresult InitWithProperties(nsIPropertyBag2* properties);

  double GetFrameDuration() const {
    NS_ASSERTION(sample_rate != 0, "invalid sample rate");
    return (double)samples / sample_rate;
  }

  PRUint32 GetFrameLength() const {
    return samples * channels * GetZapAudioSampleSize(sample_format);
  }

  already_AddRefed<nsIWritablePropertyBag2> CreateStreamInfo();
};


//----------------------------------------------------------------------
// CheckAudioStream:
// check that the audio stream defined by 'streamInfo' matches the
// given stream parameters
PRBool CheckAudioStream(nsIPropertyBag2* streamInfo,
                        const zapAudioStreamParameters& pars);

#endif // __ZAP_AUDIOSTREAMUTILS_H__
