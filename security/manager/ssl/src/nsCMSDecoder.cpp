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
 *   David Drinan <ddrinan@netscape.com>
 */
#include "nsISupports.h"
#include "nsCMSDecoder.h"
#include "nsNSSHelper.h"
#include "cms.h"

NS_IMPL_ISUPPORTS1(nsHash, nsIHash)

nsHash::nsHash() : m_ctxt(nsnull)
{
}

nsHash::~nsHash()
{
  if (m_ctxt) {
    HASH_Destroy(m_ctxt);
  }
}

NS_IMETHODIMP nsHash::ResultLen(PRInt16 aAlg, PRUint32 * aLen)
{
  *aLen = HASH_ResultLen((HASH_HashType)aAlg);
  return NS_OK;
}

NS_IMETHODIMP nsHash::Create(PRInt16 aAlg)
{
  m_ctxt = HASH_Create((HASH_HashType)aAlg);
  if (m_ctxt == nsnull) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsHash::Begin()
{
  HASH_Begin(m_ctxt);
  return NS_OK;
}

NS_IMETHODIMP nsHash::Update(unsigned char* aBuf, PRUint32 aLen)
{
  HASH_Update(m_ctxt, (const unsigned char*)aBuf, aLen);
  return NS_OK;
}

NS_IMETHODIMP nsHash::End(unsigned char* aBuf, PRUint32* aResultLen, PRUint32 aMaxResultLen)
{
  HASH_End(m_ctxt, aBuf, aResultLen, aMaxResultLen);
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsCMSMessage, nsICMSMessage)

nsCMSMessage::nsCMSMessage(NSSCMSMessage *aCMSMsg)
{
  NS_INIT_ISUPPORTS();
  m_cmsMsg = aCMSMsg;
}

nsCMSMessage::~nsCMSMessage()
{
}

NS_IMETHODIMP nsCMSMessage::VerifyDetachedSignature()
{
  return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMPL_ISUPPORTS1(nsCMSDecoder, nsICMSDecoder)

nsCMSDecoder::nsCMSDecoder()
{
  NS_INIT_ISUPPORTS();
}

nsCMSDecoder::~nsCMSDecoder()
{
}

/* void start (in NSSCMSContentCallback cb, in voidPtr arg); */
NS_IMETHODIMP nsCMSDecoder::Start(NSSCMSContentCallback cb, void * arg)
{
  m_ctx = new PipUIContext();

  m_dcx = NSS_CMSDecoder_Start(0, cb, arg, 0, m_ctx, 0, 0);
  if (!m_dcx) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* void update (in string bug, in long len); */
NS_IMETHODIMP nsCMSDecoder::Update(const char *buf, PRInt32 len)
{
  NSS_CMSDecoder_Update(m_dcx, (char *)buf, len);
  return NS_OK;
}

/* void finish (); */
NS_IMETHODIMP nsCMSDecoder::Finish(nsICMSMessage ** aCMSMsg)
{
  NSSCMSMessage *cmsMsg;
  cmsMsg = NSS_CMSDecoder_Finish(m_dcx);
  if (cmsMsg) {
    nsCOMPtr<nsICMSMessage> msg = new nsCMSMessage(cmsMsg);
    *aCMSMsg = msg;
    NS_ADDREF(*aCMSMsg);
  }
  return NS_OK;
}
