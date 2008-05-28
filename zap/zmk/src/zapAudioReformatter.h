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

#ifndef __ZAP_AUDIOREFORMATTER_H__
#define __ZAP_AUDIOREFORMATTER_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsVoidArray.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"
#include "nsAutoPtr.h"
#include "zapMediaFrame.h"
#include "zapIAudioReformatter.h"

////////////////////////////////////////////////////////////////////////
// zapAudioReformatter

// {7D8DF2F9-4BAF-4E37-8EA9-F7F292D47DA0}
#define ZAP_AUDIOREFORMATTER_CID                             \
  { 0x7d8df2f9, 0x4baf, 0x4e37, { 0x8e, 0xa9, 0xf7, 0xf2, 0x92, 0xd4, 0x7d, 0xa0 } }

#define ZAP_AUDIOREFORMATTER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "audio-reformatter"

class zapAudioReformatter : public zapIMediaNode,
                            public zapIMediaSource,
                            public zapIMediaSink,
                            public zapIAudioReformatter
{
public:
  zapAudioReformatter();
  ~zapAudioReformatter();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK
  NS_DECL_ZAPIAUDIOREFORMATTER

private:
  PRBool IsInBufferEmpty();

  enum InputState { ZAP_END_OF_STREAM, ZAP_NEW_DATA, ZAP_NO_DATA };
  InputState GetNextInputFrame();
  PRUint32 ProduceSamples(float *out, PRUint32 sampleCount);
  
  zapAudioStreamParameters mOutStreamPars;

  nsCOMPtr<nsIPropertyBag2> mInStreamInfo;
  zapAudioStreamParameters mInStreamPars;
  nsCString mInBuffer;
  PRUint32 mInBufferPointer;
  PRUint64 mInBufferTimestamp;

  nsRefPtr<zapMediaFrame> mOutFrame;
  float* mOutBufferPointer;
  PRUint32 mOutSamplesLeft;
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  
  nsCOMPtr<zapIMediaSink> mOutput;
  nsCOMPtr<zapIMediaSource> mInput;
};

#endif // __ZAP_AUDIOREFORMATTER_H__
