/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Terry Hayes <thayes@netscape.com>
 */

#ifndef _NSCMSSECUREMESSAGE_H_
#define _NSCMSSECUREMESSAGE_H_

#include "nsICMSSecureMessage.h"

#include "cms.h"

// ===============================================
// nsCMSManager - implementation of nsICMSManager
// ===============================================

#define NS_CMSSECUREMESSAGE_CLASSNAME "CMS Secure Message"
#define NS_CMSSECUREMESSAGE_CID \
  { 0x5fb907e0, 0x1dd2, 0x11b2, { 0xa7, 0xc0, 0xf1, 0x4c, 0x41, 0x6a, 0x62, 0xa1 } }

class nsCMSSecureMessage
: public nsICMSSecureMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICMSSECUREMESSAGE

  nsCMSSecureMessage();
  virtual ~nsCMSSecureMessage();

private:
  NS_METHOD encode(const unsigned char *data, PRInt32 dataLen, char **_retval);
  NS_METHOD decode(const char *data, unsigned char **result, PRInt32 * _retval);
};


#endif /* _NSCMSMESSAGE_H_ */
