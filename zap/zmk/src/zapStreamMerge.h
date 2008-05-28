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

#ifndef __ZAP_AUDIOMIXER_H__
#define __ZAP_AUDIOMIXER_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsVoidArray.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"

////////////////////////////////////////////////////////////////////////
// zapAudioMixer

// {69044C68-74FA-48D0-AF1C-A3A9878DF3B6}
#define ZAP_AUDIOMIXER_CID                             \
  { 0x69044c68, 0x74fa, 0x48d0, { 0xaf, 0x1c, 0xa3, 0xa9, 0x87, 0x8d, 0xf3, 0xb6 } }

#define ZAP_AUDIOMIXER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "audio-mixer"

class zapAudioMixer : public zapIMediaNode,
                      public zapIMediaSource
{
public:
  zapAudioMixer();
  ~zapAudioMixer();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE

private:
  friend class zapAudioMixerInput;

  zapAudioStreamParameters mStreamParameters;

  // zapAudioMixerInput inputs (weak references):
  nsVoidArray mInputs;

  PRUint32 mSampleClock;
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  
  // mixer output sink:
  nsCOMPtr<zapIMediaSink> mOutput;
};

#endif // __ZAP_AUDIOMIXER_H__
