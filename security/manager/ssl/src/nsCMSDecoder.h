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
 *   Ian McGreer <mcgreer@netscape.com>
 */

#ifndef __NS_CMSDECODER_H__
#define __NS_CMSDECODER_H__

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsICMSDecoder.h"
#include "sechash.h"
#include "cms.h"

#define NS_HASH_CLASSNAME "Hash Object"
#define NS_HASH_CID \
  { 0xa31a3028, 0xae28, 0x11d5, { 0xba, 0x4b, 0x00, 0x10, 0x83, 0x03, 0xb1, 0x17 } }

class nsHash : public nsIHash
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHASH

    nsHash();
    virtual ~nsHash();

  private:
    HASHContext * m_ctxt;
};

#define NS_CMSMESSAGE_CLASSNAME "CMS Message Object"
#define NS_CMSMESSAGE_CID \
  { 0xa4557478, 0xae16, 0x11d5, { 0xba,0x4b,0x00,0x10,0x83,0x03,0xb1,0x17 } }

class nsCMSMessage : public nsICMSMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICMSMESSAGE

  nsCMSMessage();
  nsCMSMessage(NSSCMSMessage* aCMSMsg);
  virtual ~nsCMSMessage();
  
  NSSCMSMessage* getCMS() {return m_cmsMsg;};
private:
  NSSCMSMessage * m_cmsMsg;
};


// ===============================================
// nsCMSDecoder - implementation of nsICMSDecoder
// ===============================================

#define NS_CMSDECODER_CLASSNAME "CMS Decoder Object"
#define NS_CMSDECODER_CID \
  { 0x9dcef3a4, 0xa3bc, 0x11d5, { 0xba, 0x47, 0x00, 0x10, 0x83, 0x03, 0xb1, 0x17 } }

class nsCMSDecoder : public nsICMSDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICMSDECODER

  nsCMSDecoder();
  virtual ~nsCMSDecoder();

private:
  nsCOMPtr<nsIInterfaceRequestor> m_ctx;
  NSSCMSDecoderContext *m_dcx;
};

// ===============================================
// nsCMSEncoder - implementation of nsICMSEncoder
// ===============================================

#define NS_CMSENCODER_CLASSNAME "CMS Decoder Object"
#define NS_CMSENCODER_CID \
  { 0xa15789aa, 0x8903, 0x462b, { 0x81, 0xe9, 0x4a, 0xa2, 0xcf, 0xf4, 0xd5, 0xcb } }
class nsCMSEncoder : public nsICMSEncoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICMSENCODER

  nsCMSEncoder();
  virtual ~nsCMSEncoder();

private:
  nsCOMPtr<nsIInterfaceRequestor> m_ctx;
  NSSCMSEncoderContext *m_ecx;
};

#endif