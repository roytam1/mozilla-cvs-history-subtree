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

#ifndef __ZAP_RTPTRANSMITTER_H__
#define __ZAP_RTPTRANSMITTER_H__

#include "zapFilterNode.h"
#include "nsIWritablePropertyBag2.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapRTPTransmitter

// {C9A54262-F676-420A-90B4-3C92D7E2C456}
#define ZAP_RTPTRANSMITTER_CID \
  { 0xc9a54262, 0xf676, 0x420a, { 0x90, 0xb4, 0x3c, 0x92, 0xd7, 0xe2, 0xc4, 0x56 } }

#define ZAP_RTPTRANSMITTER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "rtp-transmitter"

class zapRTPTransmitter : public zapFilterNode
{
public:
  zapRTPTransmitter();
  ~zapRTPTransmitter();
  
  NS_IMETHOD InsertedIntoContainer(zapIMediaNodeContainer *container,
                                   nsIPropertyBag2 *node_pars);
  NS_IMETHOD RemovedFromContainer();
  virtual nsresult ValidateNewStream(nsIPropertyBag2* streamInfo);
  virtual nsresult Filter(zapIMediaFrame* input, zapIMediaFrame** output);

private:
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  nsCString mAddress;
  PRUint32 mPort;
};

#endif // __ZAP_RTPTRANSMITTER_H__
