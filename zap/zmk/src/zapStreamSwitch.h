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

#ifndef __ZAP_STREAMSWITCH_H__
#define __ZAP_STREAMSWITCH_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsDataHashtable.h"
#include "zapIMediaFrame.h"
#include "zapIStreamSwitch.h"

class zapStreamSwitchOutput;

////////////////////////////////////////////////////////////////////////
// zapStreamSwitch

// {D43A998B-587E-42FD-A32A-156C94BB3895}
#define ZAP_STREAMSWITCH_CID                             \
  { 0xd43a998b, 0x587e, 0x42fd, { 0xa3, 0x2a, 0x15, 0x6c, 0x94, 0xbb, 0x38, 0x95 } }

#define ZAP_STREAMSWITCH_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "stream-switch"

class zapStreamSwitch : public zapIMediaNode,
                        public zapIMediaSink,
                        public zapIStreamSwitch
{
public:
  zapStreamSwitch();
  ~zapStreamSwitch();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASINK
  NS_DECL_ZAPISTREAMSWITCH

private:
  friend class zapStreamSwitchOutput;

  void RequestFrame();
  
  // zapStreamSwitchOutput outputs (weak references), indexed by id:
  nsDataHashtable<nsCStringHashKey, zapStreamSwitchOutput*> mOutputs;

  // currently selected output (weak ref):
  zapStreamSwitchOutput *mSelectedOutput;
  
  // StreamSwitch input source:
  nsCOMPtr<zapIMediaSource> mInput;
};

#endif // __ZAP_STREAMSWITCH_H__
