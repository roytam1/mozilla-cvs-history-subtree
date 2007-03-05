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

#ifndef __ZAP_RTTTLPLAYER_H__
#define __ZAP_RTTTLPLAYER_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaFrame.h"
#include "nsCOMPtr.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"

////////////////////////////////////////////////////////////////////////
// zapRtttlPlayer

// {09DF0E72-152A-4104-9AD9-9A98B0BDBAB6}
#define ZAP_RTTTLPLAYER_CID                             \
  { 0x09df0e72, 0x152a, 0x4104, { 0x9a, 0xd9, 0x9a, 0x98, 0xb0, 0xbd, 0xba, 0xb6 } }

#define ZAP_RTTTLPLAYER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "rtttl-player"

class zapRtttlPlayer : public zapIMediaNode,
                       public zapIMediaSource
{
public:
  zapRtttlPlayer();
  ~zapRtttlPlayer();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE

private:
  PRBool mLoop;
  double mAmplitude;
  zapAudioStreamParameters mStreamParameters;
  
  // parsed array of tones:
  struct Tone {
    float frequency; // Hz
    float duration;  // s
  };  
  Tone* mTones;
  PRUint32 mToneCount;

  nsresult ParseRTTTL(const char* buf);
  bool ParseControlValue(const char** bufp, float* result);
  bool ParseToneCommand(const char **bufp, Tone* tone,
                        float base_octave,
                        float base_duration,
                        float base_bpm);
  
  // pointer to current tone:
  struct { PRUint32 current; float offset; } mTonePointer;
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;

  nsCOMPtr<zapIMediaSink> mOutput;
};

#endif // __ZAP_RTTTLPLAYER_H__
