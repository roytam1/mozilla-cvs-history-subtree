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

#ifndef __ZAP_STUNCLIENT_H__
#define __ZAP_STUNCLIENT_H__

#include "zapIStunClient.h"
#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaNodeContainer.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsIWritablePropertyBag2.h"
#include "zapINetUtils.h"
#include "zapIStunMessage.h"
#include "nsDataHashtable.h"

class zapStunBindingRequest;

////////////////////////////////////////////////////////////////////////
// zapStunClient

// {C359F22A-DEF1-4D49-B4C6-6601137FD538}
#define ZAP_STUNCLIENT_CID                             \
  { 0xc359f22a, 0xdef1, 0x4d49, { 0xb4, 0xc6, 0x66, 0x01, 0x13, 0x7f, 0xd5, 0x38 } }

#define ZAP_STUNCLIENT_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "stun-client"

class zapStunClient : public zapIStunClient,
                      public zapIMediaNode,
                      public zapIMediaSource,
                      public zapIMediaSink
{
public:
  zapStunClient();
  ~zapStunClient();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPISTUNCLIENT
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK

  typedef nsDataHashtable<nsCStringHashKey, zapStunBindingRequest*> RequestHash;
  
private:
  friend class zapStunBindingRequest;
  
  void SendStunRequest(zapIStunMessage* message,
                       const nsACString& server,
                       PRInt32 port);

  void CancelPendingRequests();
  
  // pending binding requests (strong refs, managed by objects
  // themselves), indexed by transaction id:
  RequestHash mRequests;
  
  nsCOMPtr<zapIMediaNodeContainer> mContainer;
  nsCOMPtr<zapINetUtils> mNetUtils;

  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  nsCOMPtr<zapIMediaSink> mOutput;
  nsCOMPtr<zapIMediaSource> mInput;
};

#endif // __ZAP_STUNCLIENT_H__
