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

#ifndef __ZAP_RTPSESSION_H__
#define __ZAP_RTPSESSION_H__

#include "zapIMediaNode.h"
#include "nsAutoPtr.h"
#include "nsStringAPI.h"
#include "zapIMediaFrame.h"

class zapRTPSessionFilter;

////////////////////////////////////////////////////////////////////////
// zapRTPSession

// {BFB34654-4314-4D82-8867-4DB6F0E732A0}
#define ZAP_RTPSESSION_CID                             \
  { 0xbfb34654, 0x4314, 0x4d82, { 0x88, 0x67, 0x4d, 0xb6, 0xf0, 0xe7, 0x32, 0xa0 } }

#define ZAP_RTPSESSION_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "rtp-session"

class zapRTPSession : public zapIMediaNode
{
public:
  zapRTPSession();
  ~zapRTPSession();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
private:
  nsresult FilterLocal2RemoteRTP(zapIMediaFrame* in, zapIMediaFrame** out);
  nsresult FilterRemote2LocalRTP(zapIMediaFrame* in, zapIMediaFrame** out);
  
  PRUint32 mSSRC;
  PRUint16 mSequenceNumber;
  
  nsRefPtr<zapRTPSessionFilter> mLocal2RemoteRTP;
  nsRefPtr<zapRTPSessionFilter> mRemote2LocalRTP;
  //XXX rtcp
};

#endif // __ZAP_RTPSESSION_H__
