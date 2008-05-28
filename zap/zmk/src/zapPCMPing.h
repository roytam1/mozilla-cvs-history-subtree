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

#ifndef __ZAP_PCMPING_H__
#define __ZAP_PCMPING_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapAudioStreamUtils.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapPCMPing

// {9E384937-A08B-44F2-9E95-5D437612EA91}
#define ZAP_PCMPING_CID                             \
  { 0x9e384937, 0xa08b, 0x44f2, { 0x9e, 0x95, 0x5d, 0x43, 0x76, 0x12, 0xea, 0x91 } }

#define ZAP_PCMPING_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "pcm-ping"

class zapPCMPing : public zapIMediaNode,
                   public zapIMediaSource,
                   public zapIMediaSink
{
public:
  zapPCMPing();
  ~zapPCMPing();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK

private:
  zapAudioStreamParameters mStreamParameters;
  double mPingPitch;
  
  nsCOMPtr<zapIMediaSource> mInput;
  nsCOMPtr<zapIMediaSink> mOutput;
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;

  nsCString mPingData;
};

#endif // __ZAP_PCMPING_H__
