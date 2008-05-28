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

#ifndef __ZAP_AUDIOTONEFRAME_H__
#define __ZAP_AUDIOTONEFRAME_H__

#include "zapIAudioToneFrame.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsIPropertyBag2.h"


// helper struct for holding/accessing the frame's data:
struct zapAudioTone {
  zapAudioTone();
  zapAudioTone(const nsACString& octets);

  PRUint16 GetModulation();
  void SetModulation(PRUint16 modulation);

  PRBool GetT();
  void SetT(PRBool T);

  PRUint16 GetVolume();
  void SetVolume(PRUint16 volume);

  PRUint16 GetDuration();
  void SetDuration(PRUint16 duration);

  PRUint16 GetFrequencyCount();
  PRUint16 GetFrequencyAt(PRUint16 n);
  void AddFrequency(PRUint16 frequency);
  
  nsCString data;
};

class zapAudioToneFrame : public zapIAudioToneFrame
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIAUDIOTONEFRAME
  NS_DECL_ZAPIMEDIAFRAME
  
  zapAudioToneFrame();
  ~zapAudioToneFrame();
  PRBool Init(const nsACString& octets, nsIPropertyBag2* streamInfo);
  PRBool Init(const zapAudioTone& tone, nsIPropertyBag2* streamInfo);
  PRBool Init();
  
  PRBool mMarkerBit;
  zapAudioTone mToneData;
  PRUint64 mTimestamp;
  nsCOMPtr<nsIPropertyBag2> mStreamInfo;
};

//----------------------------------------------------------------------
// helpers to create add-refed audiotone frame objects:

zapIAudioToneFrame *
CreateAudioToneFrame(const nsACString& octets, nsIPropertyBag2* streamInfo);

zapAudioToneFrame *
CreateAudioToneFrame(const zapAudioTone& tone, nsIPropertyBag2* streamInfo);

zapAudioToneFrame * CreateAudioToneFrame();

#endif // __ZAP_AUDIOTONEFRAME_H__
