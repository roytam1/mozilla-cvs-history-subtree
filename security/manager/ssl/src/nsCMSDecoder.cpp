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
#include "nsNSSCertificate.h"
#include "smime.h"
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

nsCMSMessage::nsCMSMessage()
{
  NS_INIT_ISUPPORTS();
  m_cmsMsg = nsnull;
}
nsCMSMessage::nsCMSMessage(NSSCMSMessage *aCMSMsg)
{
  NS_INIT_ISUPPORTS();
  m_cmsMsg = aCMSMsg;
}

nsCMSMessage::~nsCMSMessage()
{
}

NS_IMETHODIMP nsCMSMessage::VerifySignature()
{
  return  NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCMSMessage::GetSignerEmailAddress(char * * aEmail)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCMSMessage::GetSignerCommonName(char ** aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCMSMessage::ContentIsEncrypted(int *)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCMSMessage::ContentIsSigned(int *)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCMSMessage::VerifyDetachedSignature()
{
  if (NSS_CMSMessage_IsSigned(m_cmsMsg) == PR_TRUE) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP nsCMSMessage::CreateEncrypted(nsISupportsArray * aRecipientCerts)
{
  NSSCMSContentInfo *cinfo;
  NSSCMSEnvelopedData *envd;
  NSSCMSRecipientInfo *recipientInfo;
  CERTCertificate **recipientCerts;
  PLArenaPool *tmpPoolp = nsnull;
  SECOidTag bulkAlgTag;
  int keySize, i;
  nsNSSCertificate *nssRecipientCert;

  // Check the recipient certificates //
  PRUint32 recipientCertCount;
  aRecipientCerts->Count(&recipientCertCount);
  PR_ASSERT(recipientCertCount > 0);

  if ((tmpPoolp = PORT_NewArena(1024)) == nsnull) {
    goto loser;
  }

  if ((recipientCerts = (CERTCertificate**)PORT_ArenaZAlloc(tmpPoolp,
                                           (recipientCertCount+1)*sizeof(CERTCertificate*)))
                                           == nsnull) {
    goto loser;
  }

  for (i=0; i<recipientCertCount; i++) {
    nssRecipientCert = NS_STATIC_CAST(nsNSSCertificate*, aRecipientCerts->ElementAt(i));
    recipientCerts[i] = nssRecipientCert->GetCert();
  }
  recipientCerts[i] = nsnull;

  // Find a bulk key algorithm //
  if (NSS_SMIMEUtil_FindBulkAlgForRecipients(recipientCerts, &bulkAlgTag,
                                            &keySize) != SECSuccess) {
    goto loser;
  }

  m_cmsMsg = NSS_CMSMessage_Create(NULL);
  if (m_cmsMsg == nsnull) {
    goto loser;
  }

  if ((envd = NSS_CMSEnvelopedData_Create(m_cmsMsg, bulkAlgTag, keySize)) == nsnull) {
    goto loser;
  }

  cinfo = NSS_CMSMessage_GetContentInfo(m_cmsMsg);
  if (NSS_CMSContentInfo_SetContent_EnvelopedData(m_cmsMsg, cinfo, envd) != SECSuccess) {
    goto loser;
  }

  cinfo = NSS_CMSEnvelopedData_GetContentInfo(envd);
  if (NSS_CMSContentInfo_SetContent_Data(m_cmsMsg, cinfo, nsnull, PR_FALSE) != SECSuccess) {
    goto loser;
  }

  // Create and attach recipient information //
  for (i=0; recipientCerts[i] != nsnull; i++) {
    if ((recipientInfo = NSS_CMSRecipientInfo_Create(m_cmsMsg, recipientCerts[i])) == nsnull) {
      goto loser;
    }
    if (NSS_CMSEnvelopedData_AddRecipient(envd, recipientInfo) != SECSuccess) {
      goto loser;
    }
  }

  if (tmpPoolp) {
    PORT_FreeArena(tmpPoolp, PR_FALSE);
  }

  return NS_OK;
loser:
  if (m_cmsMsg) {
    NSS_CMSMessage_Destroy(m_cmsMsg);
    m_cmsMsg = nsnull;
  }
  if (tmpPoolp) {
    PORT_FreeArena(tmpPoolp, PR_FALSE);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsCMSMessage::CreateSigned(nsIX509Cert* aSigningCert, nsIX509Cert* aEncryptCert)
{
  NSSCMSContentInfo *cinfo;
  NSSCMSSignedData *sigd;
  NSSCMSSignerInfo *signerinfo;
  CERTCertificate *scert, *ecert;

  /* Get the certs */
  scert = NS_STATIC_CAST(nsNSSCertificate*, aSigningCert)->GetCert();
  ecert = NS_STATIC_CAST(nsNSSCertificate*, aEncryptCert)->GetCert();

  /*
   * create the message object
   */
  m_cmsMsg = NSS_CMSMessage_Create(NULL); /* create a message on its own pool */
  if (m_cmsMsg == NULL) {
    goto loser;
  }

  /*
   * build chain of objects: message->signedData->data
   */
  if ((sigd = NSS_CMSSignedData_Create(m_cmsMsg)) == NULL) {
  	goto loser;
  }
  cinfo = NSS_CMSMessage_GetContentInfo(m_cmsMsg);
  if (NSS_CMSContentInfo_SetContent_SignedData(m_cmsMsg, cinfo, sigd) 
          != SECSuccess) {
  	goto loser;
  }

  cinfo = NSS_CMSSignedData_GetContentInfo(sigd);

  /* we're always passing data in and detaching optionally */
  if (NSS_CMSContentInfo_SetContent_Data(m_cmsMsg, cinfo, nsnull, PR_TRUE) 
          != SECSuccess) {
  	goto loser;
  }

  /* 
   * create & attach signer information
   */
  if ((signerinfo = NSS_CMSSignerInfo_Create(m_cmsMsg, scert, SEC_OID_SHA1)) 
          == NULL) {
    goto loser;
  }

  /* we want the cert chain included for this one */
  if (NSS_CMSSignerInfo_IncludeCerts(signerinfo, NSSCMSCM_CertChain, 
                                       certUsageEmailSigner) 
          != SECSuccess) {
    goto loser;
  }

  if (NSS_CMSSignerInfo_AddSigningTime(signerinfo, PR_Now()) 
	      != SECSuccess) {
    goto loser;
  }

	if (NSS_CMSSignerInfo_AddSMIMECaps(signerinfo) != SECSuccess) {
	    goto loser;
	}

	if (NSS_CMSSignerInfo_AddSMIMEEncKeyPrefs(signerinfo, ecert, 
	                                        CERT_GetDefaultCertDB())
	      != SECSuccess) {
	    goto loser;
	}
	if (NSS_CMSSignedData_AddCertificate(sigd, ecert) != SECSuccess) {
	    goto loser;
	}

  if (NSS_CMSSignedData_AddSignerInfo(sigd, signerinfo) != SECSuccess) {
    goto loser;
  }

  return NS_OK;
loser:
  NSS_CMSMessage_Destroy(m_cmsMsg);
  return NS_ERROR_FAILURE;
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

NS_IMPL_ISUPPORTS1(nsCMSEncoder, nsICMSEncoder)

nsCMSEncoder::nsCMSEncoder()
{
  NS_INIT_ISUPPORTS();
}

nsCMSEncoder::~nsCMSEncoder()
{
}

/* void start (); */
NS_IMETHODIMP nsCMSEncoder::Start(nsICMSMessage *aMsg, NSSCMSContentCallback cb, void * arg)
{
  nsCMSMessage *cmsMsg = NS_STATIC_CAST(nsCMSMessage*, aMsg);
  m_ctx = new PipUIContext();

  m_ecx = NSS_CMSEncoder_Start(cmsMsg->getCMS(), cb, arg, 0, 0, 0, m_ctx, 0, 0, 0, 0);
  if (m_ecx == nsnull) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* void update (in string aBuf, in long aLen); */
NS_IMETHODIMP nsCMSEncoder::Update(const char *aBuf, PRInt32 aLen)
{
  if (NSS_CMSEncoder_Update(m_ecx, aBuf, aLen) != SECSuccess) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* void finish (); */
NS_IMETHODIMP nsCMSEncoder::Finish()
{
  if (NSS_CMSEncoder_Finish(m_ecx) != SECSuccess) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* void encode (in nsICMSMessage aMsg); */
NS_IMETHODIMP nsCMSEncoder::Encode(nsICMSMessage *aMsg)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

