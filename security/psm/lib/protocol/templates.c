/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* 
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "stddef.h"
#include "messages.h"

CMTMessageTemplate SingleNumMessageTemplate[] =
{
  { CMT_DT_INT, offsetof(SingleNumMessage, value), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SingleStringMessageTemplate[] =
{
  { CMT_DT_STRING, offsetof(SingleStringMessage, string), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SingleItemMessageTemplate[] =
{
  { CMT_DT_ITEM, offsetof(SingleItemMessage, item), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate HelloRequestTemplate[] =
{
  { CMT_DT_INT, offsetof(HelloRequest, version), 0, 0 },
  { CMT_DT_INT, offsetof(HelloRequest, policy), 0, 0 },
  { CMT_DT_BOOL, offsetof(HelloRequest, doesUI), 0, 0 },
  { CMT_DT_STRING, offsetof(HelloRequest, profile), 0, 0 },
  { CMT_DT_STRING, offsetof(HelloRequest, profileDir), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate HelloReplyTemplate[] =
{
  { CMT_DT_INT, offsetof(HelloReply, result), 0, 0 },
  { CMT_DT_INT, offsetof(HelloReply, sessionID), 0, 0 },
  { CMT_DT_INT, offsetof(HelloReply, version), 0, 0 },
  { CMT_DT_STRING, offsetof(HelloReply, stringVersion), 0, 0 },
  { CMT_DT_INT, offsetof(HelloReply, httpPort), 0, 0 },
  { CMT_DT_INT, offsetof(HelloReply, policy), 0, 0 },
  { CMT_DT_ITEM, offsetof(HelloReply, nonce), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SSLDataConnectionRequestTemplate[] =
{
  { CMT_DT_INT, offsetof(SSLDataConnectionRequest, flags), 0, 0 },
  { CMT_DT_INT, offsetof(SSLDataConnectionRequest, port), 0, 0 },
  { CMT_DT_STRING, offsetof(SSLDataConnectionRequest, hostIP), 0, 0 },
  { CMT_DT_STRING, offsetof(SSLDataConnectionRequest, hostName), 0, 0 },
  { CMT_DT_BOOL, offsetof(SSLDataConnectionRequest, forceHandshake), 0, 0 },
  { CMT_DT_ITEM, offsetof(SSLDataConnectionRequest, clientContext), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate TLSDataConnectionRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(TLSDataConnectionRequest, port), 0, 0 },
    { CMT_DT_STRING, offsetof(TLSDataConnectionRequest, hostIP), 0, 0 },
    { CMT_DT_STRING, offsetof(TLSDataConnectionRequest, hostName), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate TLSStepUpRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(TLSStepUpRequest, connID), 0, 0 },
    { CMT_DT_ITEM, offsetof(TLSStepUpRequest, clientContext), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate ProxyStepUpRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(ProxyStepUpRequest, connID), 0, 0 },
    { CMT_DT_ITEM, offsetof(ProxyStepUpRequest, clientContext), 0, 0 },
    { CMT_DT_STRING, offsetof(ProxyStepUpRequest, url), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PKCS7DataConnectionRequestTemplate[] =
{
  { CMT_DT_INT, offsetof(PKCS7DataConnectionRequest, resID), 0, 0 },
  { CMT_DT_ITEM, offsetof(PKCS7DataConnectionRequest, clientContext), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate DataConnectionReplyTemplate[] =
{
  { CMT_DT_INT, offsetof(DataConnectionReply, result), 0, 0 },
  { CMT_DT_INT, offsetof(DataConnectionReply, connID), 0, 0 },
  { CMT_DT_INT, offsetof(DataConnectionReply, port), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate UIEventTemplate[] =
{
    { CMT_DT_INT, offsetof(UIEvent, resourceID), 0, 0 },
    { CMT_DT_INT, offsetof(UIEvent, width), 0, 0 },
    { CMT_DT_INT, offsetof(UIEvent, height), 0, 0 },
    { CMT_DT_STRING, offsetof(UIEvent, url), 0, 0 },
    { CMT_DT_ITEM, offsetof(UIEvent, clientContext), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate TaskCompletedEventTemplate[] =
{
    { CMT_DT_INT, offsetof(TaskCompletedEvent, resourceID), 0, 0 },
    { CMT_DT_INT, offsetof(TaskCompletedEvent, numTasks), 0, 0 },
    { CMT_DT_INT, offsetof(TaskCompletedEvent, result), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate VerifyDetachedSigRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(VerifyDetachedSigRequest, pkcs7ContentID), 0, 0 },
    { CMT_DT_INT, offsetof(VerifyDetachedSigRequest, certUsage), 0, 0 },
    { CMT_DT_INT, offsetof(VerifyDetachedSigRequest, hashAlgID), 0, 0 },
    { CMT_DT_BOOL, offsetof(VerifyDetachedSigRequest, keepCert), 0, 0 },
    { CMT_DT_ITEM, offsetof(VerifyDetachedSigRequest, hash), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CreateSignedRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(CreateSignedRequest, scertRID), 0, 0 },
    { CMT_DT_INT, offsetof(CreateSignedRequest, ecertRID), 0, 0 },
    { CMT_DT_INT, offsetof(CreateSignedRequest, dig_alg), 0, 0 },
    { CMT_DT_ITEM, offsetof(CreateSignedRequest, digest), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CreateContentInfoReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(CreateContentInfoReply, ciRID), 0, 0 },
    { CMT_DT_INT, offsetof(CreateContentInfoReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(CreateContentInfoReply, errorCode), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CreateEncryptedRequestTemplate[] =
{
  { CMT_DT_INT, offsetof(CreateEncryptedRequest, scertRID), 0, 0 },
  { CMT_DT_LIST, offsetof(CreateEncryptedRequest, nrcerts), 0, 0 },
  { CMT_DT_INT, offsetof(CreateEncryptedRequest, rcertRIDs), 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CreateResourceRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(CreateResourceRequest, type), 0, 0 },
    { CMT_DT_ITEM, offsetof(CreateResourceRequest, params), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CreateResourceReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(CreateResourceReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(CreateResourceReply, resID), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetAttribRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(GetAttribRequest, resID), 0, 0 },
    { CMT_DT_INT, offsetof(GetAttribRequest, fieldID), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetAttribReplyTemplate[] =
{
  { CMT_DT_INT, offsetof(GetAttribReply, result), 0, 0 },
  { CMT_DT_CHOICE, offsetof(GetAttribReply, value.type), 0, 0 },
  { CMT_DT_RID, offsetof(GetAttribReply, value.u.rid), 0, SSM_RID_ATTRIBUTE },
  { CMT_DT_INT, offsetof(GetAttribReply, value.u.numeric), 0,
    SSM_NUMERIC_ATTRIBUTE },
  { CMT_DT_ITEM, offsetof(GetAttribReply, value.u.string), 0,
    SSM_STRING_ATTRIBUTE},
  { CMT_DT_END_CHOICE, 0, 0, 0 },
  { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SetAttribRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(SetAttribRequest, resID), 0, 0 },
    { CMT_DT_INT, offsetof(SetAttribRequest, fieldID), 0, 0 },
    { CMT_DT_CHOICE, offsetof(SetAttribRequest, value.type), 0, 0 },
    { CMT_DT_RID, offsetof(SetAttribRequest, value.u.rid), 0,
      SSM_RID_ATTRIBUTE },
    { CMT_DT_INT, offsetof(SetAttribRequest, value.u.numeric), 0,
                            SSM_NUMERIC_ATTRIBUTE },
    { CMT_DT_ITEM, offsetof(SetAttribRequest, value.u.string), 0,
                            SSM_STRING_ATTRIBUTE},
    { CMT_DT_END_CHOICE, 0, 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PickleResourceReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(PickleResourceReply, result), 0, 0 },
    { CMT_DT_ITEM, offsetof(PickleResourceReply, blob), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate UnpickleResourceRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(UnpickleResourceRequest, resourceType), 0, 0 },
    { CMT_DT_ITEM, offsetof(UnpickleResourceRequest, resourceData), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate UnpickleResourceReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(UnpickleResourceReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(UnpickleResourceReply, resID), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PickleSecurityStatusReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(PickleSecurityStatusReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(PickleSecurityStatusReply, securityLevel), 0, 0 },
    { CMT_DT_ITEM, offsetof(PickleSecurityStatusReply, blob), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate DupResourceReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(DupResourceReply, result), 0, 0 },
    { CMT_DT_RID, offsetof(DupResourceReply, resID), 0, SSM_RID_ATTRIBUTE },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate DestroyResourceRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(DestroyResourceRequest, resID), 0, 0 },
    { CMT_DT_INT, offsetof(DestroyResourceRequest, resType), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate VerifyCertRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(VerifyCertRequest, resID), 0, 0 },
    { CMT_DT_INT, offsetof(VerifyCertRequest, certUsage), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate AddTempCertToDBRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(AddTempCertToDBRequest, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(AddTempCertToDBRequest, nickname), 0, 0 },
    { CMT_DT_INT, offsetof(AddTempCertToDBRequest, sslFlags), 0, 0 },
    { CMT_DT_INT, offsetof(AddTempCertToDBRequest, emailFlags), 0, 0 },
    { CMT_DT_INT, offsetof(AddTempCertToDBRequest, objSignFlags), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate MatchUserCertRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(MatchUserCertRequest, certType), 0, 0 },
    { CMT_DT_LIST, offsetof(MatchUserCertRequest, numCANames), 0, 0 },
    { CMT_DT_STRING, offsetof(MatchUserCertRequest, caNames), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate MatchUserCertReplyTemplate[] =
{
    { CMT_DT_LIST, offsetof(MatchUserCertReply, numCerts), 0, 0 },
    { CMT_DT_INT, offsetof(MatchUserCertReply, certs), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate EncodeCRMFReqRequestTemplate[] =
{
    { CMT_DT_LIST, offsetof(EncodeCRMFReqRequest, numRequests), 0, 0 },
    { CMT_DT_INT, offsetof(EncodeCRMFReqRequest, reqIDs), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate CMMFCertResponseRequestTemplate[] =
{
    { CMT_DT_STRING, offsetof(CMMFCertResponseRequest, nickname), 0, 0 },
    { CMT_DT_STRING, offsetof(CMMFCertResponseRequest, base64Der), 0, 0 },
    { CMT_DT_INT,    offsetof(CMMFCertResponseRequest, doBackup), 0, 0 },
    { CMT_DT_ITEM,   offsetof(CMMFCertResponseRequest, clientContext), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PasswordRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(PasswordRequest, tokenKey), 0, 0 },
    { CMT_DT_STRING, offsetof(PasswordRequest, prompt), 0, 0 },
    { CMT_DT_ITEM, offsetof(PasswordRequest, clientContext), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PasswordReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(PasswordReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(PasswordReply, tokenID), 0, 0 },
    { CMT_DT_STRING, offsetof(PasswordReply, passwd), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate KeyPairGenRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(KeyPairGenRequest, keyGenCtxtID), 0, 0 },
    { CMT_DT_INT, offsetof(KeyPairGenRequest, genMechanism), 0, 0 },
    { CMT_DT_INT, offsetof(KeyPairGenRequest, keySize), 0, 0 },
    { CMT_DT_ITEM, offsetof(KeyPairGenRequest, params), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate DecodeAndCreateTempCertRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(DecodeAndCreateTempCertRequest, type), 0, 0 },
    { CMT_DT_ITEM, offsetof(DecodeAndCreateTempCertRequest, cert), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GenKeyOldStyleRequestTemplate[] =
{
    { CMT_DT_STRING, offsetof(GenKeyOldStyleRequest, choiceString), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStyleRequest, challenge), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStyleRequest, typeString), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStyleRequest, pqgString), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GenKeyOldStyleTokenRequestTemplate[] = 
{
    { CMT_DT_INT, offsetof(GenKeyOldStyleTokenRequest, rid), 0, 0 },
    { CMT_DT_LIST, offsetof(GenKeyOldStyleTokenRequest, numtokens), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStyleTokenRequest, tokenNames), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GenKeyOldStyleTokenReplyTemplate[] = 
{
    { CMT_DT_INT, offsetof(GenKeyOldStyleTokenReply, rid), 0, 0 },
    { CMT_DT_BOOL, offsetof(GenKeyOldStyleTokenReply, cancel), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStyleTokenReply, tokenName), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GenKeyOldStylePasswordRequestTemplate[] = 
{
    { CMT_DT_INT, offsetof(GenKeyOldStylePasswordRequest, rid), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStylePasswordRequest, tokenName), 0,
      0 },
    { CMT_DT_BOOL, offsetof(GenKeyOldStylePasswordRequest, internal), 0, 0 },
    { CMT_DT_INT, offsetof(GenKeyOldStylePasswordRequest, minpwdlen), 0, 0 },
    { CMT_DT_INT, offsetof(GenKeyOldStylePasswordRequest, maxpwdlen), 0, 0 }, 
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GenKeyOldStylePasswordReplyTemplate[] = 
{
    { CMT_DT_INT, offsetof(GenKeyOldStylePasswordReply, rid), 0, 0 },
    { CMT_DT_BOOL, offsetof(GenKeyOldStylePasswordReply, cancel), 0, 0 },
    { CMT_DT_STRING, offsetof(GenKeyOldStylePasswordReply, password), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};


CMTMessageTemplate GetKeyChoiceListRequestTemplate[] =
{
    { CMT_DT_STRING, offsetof(GetKeyChoiceListRequest, type), 0, 0 },
    { CMT_DT_STRING, offsetof(GetKeyChoiceListRequest, pqgString), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetKeyChoiceListReplyTemplate[] =
{
    { CMT_DT_LIST, offsetof(GetKeyChoiceListReply, nchoices), 0, 0 },
    { CMT_DT_STRING, offsetof(GetKeyChoiceListReply, choices), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate AddNewSecurityModuleRequestTemplate[] =
{
    { CMT_DT_STRING, offsetof(AddNewSecurityModuleRequest, moduleName), 0, 0 },
    { CMT_DT_STRING, offsetof(AddNewSecurityModuleRequest, libraryPath), 0, 0 },
    { CMT_DT_INT, offsetof(AddNewSecurityModuleRequest, pubMechFlags), 0, 0 },
    { CMT_DT_INT, offsetof(AddNewSecurityModuleRequest, pubCipherFlags), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate FilePathRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(FilePathRequest, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(FilePathRequest, prompt), 0, 0 },
    { CMT_DT_BOOL, offsetof(FilePathRequest, getExistingFile), 0, 0 },
    { CMT_DT_STRING, offsetof(FilePathRequest, fileRegEx), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate FilePathReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(FilePathReply, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(FilePathReply, filePath), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PasswordPromptReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(PasswordPromptReply, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(PasswordPromptReply, promptReply), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SignTextRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(SignTextRequest, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(SignTextRequest, stringToSign), 0, 0 },
    { CMT_DT_STRING, offsetof(SignTextRequest, hostName), 0, 0 },
    { CMT_DT_STRING, offsetof(SignTextRequest, caOption), 0, 0 },
    { CMT_DT_LIST, offsetof(SignTextRequest, numCAs), 0, 0 },
    { CMT_DT_STRING, offsetof(SignTextRequest, caNames), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetLocalizedTextReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(GetLocalizedTextReply, whichString), 0, 0 },
    { CMT_DT_STRING, offsetof(GetLocalizedTextReply, localizedString), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate ImportCertReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(ImportCertReply, result), 0, 0 },
    { CMT_DT_INT, offsetof(ImportCertReply, resID), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PromptRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(PromptRequest, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(PromptRequest, prompt), 0, 0 },
    { CMT_DT_ITEM, offsetof(PromptRequest, clientContext), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate PromptReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(PromptReply, resID), 0, 0 },
    { CMT_DT_BOOL, offsetof(PromptReply, cancel), 0, 0 },
    { CMT_DT_STRING, offsetof(PromptReply, promptReply), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate RedirectCompareRequestTemplate[] =
{
    { CMT_DT_ITEM, offsetof(RedirectCompareRequest, socketStatus1Data), 0, 0 },
    { CMT_DT_ITEM, offsetof(RedirectCompareRequest, socketStatus2Data), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate DecodeAndAddCRLRequestTemplate[] = 
{
    { CMT_DT_ITEM,   offsetof(DecodeAndAddCRLRequest, derCrl), 0, 0 },
    { CMT_DT_INT,    offsetof(DecodeAndAddCRLRequest, type), 0, 0 },
    { CMT_DT_STRING, offsetof(DecodeAndAddCRLRequest, url), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SecurityAdvisorRequestTemplate[] =
{
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, infoContext), 0, 0 },
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, resID), 0, 0 },
    { CMT_DT_STRING, offsetof(SecurityAdvisorRequest, hostname), 0, 0 },
    { CMT_DT_STRING, offsetof(SecurityAdvisorRequest, senderAddr), 0, 0 },
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, encryptedP7CInfo), 0, 0 },
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, signedP7CInfo), 0, 0 },
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, decodeError), 0, 0 },
    { CMT_DT_INT,   offsetof(SecurityAdvisorRequest, verifyError), 0, 0 },
    { CMT_DT_BOOL,   offsetof(SecurityAdvisorRequest, encryptthis), 0, 0 },
    { CMT_DT_BOOL,   offsetof(SecurityAdvisorRequest, signthis), 0, 0 },
    { CMT_DT_LIST,	offsetof(SecurityAdvisorRequest, numRecipients), 0, 0 },
    { CMT_DT_STRING, offsetof(SecurityAdvisorRequest, recipients), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SCAddTempCertToPermDBRequestTemplate[] =
{
    { CMT_DT_ITEM, offsetof(SCAddTempCertToPermDBRequest, certKey), 0, 0 },
    { CMT_DT_STRING, offsetof(SCAddTempCertToPermDBRequest, trustStr), 0, 0 },
    { CMT_DT_STRING, offsetof(SCAddTempCertToPermDBRequest, nickname), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SCDeletePermCertsRequestTemplate[] =
{
    { CMT_DT_ITEM, offsetof(SCDeletePermCertsRequest, certKey), 0, 0 },
    { CMT_DT_BOOL, offsetof(SCDeletePermCertsRequest, deleteAll), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate TimeMessageTemplate[] =
{
    { CMT_DT_INT, offsetof(TimeMessage, year), 0, 0 },
    { CMT_DT_INT, offsetof(TimeMessage, month), 0, 0 },
    { CMT_DT_INT, offsetof(TimeMessage, day), 0, 0 },
    { CMT_DT_INT, offsetof(TimeMessage, hour), 0, 0 },
    { CMT_DT_INT, offsetof(TimeMessage, minute), 0, 0 },
    { CMT_DT_INT, offsetof(TimeMessage, second), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate SCCertIndexEnumReplyTemplate[] =
{
    { CMT_DT_INT, offsetof(SCCertIndexEnumReply, length), 0, 0 },
    { CMT_DT_STRUCT_PTR, offsetof(SCCertIndexEnumReply, list), 0, 0 },
    { CMT_DT_STRING, offsetof(CertEnumElement, name), 0, 0 },
    { CMT_DT_ITEM, offsetof(CertEnumElement, certKey), 0, 0 },
    { CMT_DT_END_STRUCT_LIST, 0, 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

/* Test template */

CMTMessageTemplate TestListTemplate[] = 
{
	{ CMT_DT_STRING, offsetof(TestList, listName), 0, 0 },
	{ CMT_DT_STRUCT_LIST, offsetof(TestList, numElements), 0, 0 },
	{ CMT_DT_STRUCT_PTR, offsetof(TestList, elements), 0, 0 },
	{ CMT_DT_STRING, offsetof(TestListElement, name), 0, 0 },
	{ CMT_DT_STRING, offsetof(TestListElement, value), 0, 0 },
	{ CMT_DT_END_STRUCT_LIST, 0, 0, 0 },
	{ CMT_DT_END, 0, 0, 0}
};

CMTMessageTemplate SetPrefListMessageTemplate[] =
{
    { CMT_DT_STRUCT_LIST, offsetof(SetPrefListMessage, length), 0, 0 },
    { CMT_DT_STRUCT_PTR, offsetof(SetPrefListMessage, list), 0, 0 },
    { CMT_DT_STRING, offsetof(SetPrefElement, key), 0, 0 },
    { CMT_DT_STRING, offsetof(SetPrefElement, value), 0, 0 },
    { CMT_DT_INT, offsetof(SetPrefElement, type), 0, 0 },
    { CMT_DT_END_STRUCT_LIST, 0, 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetPrefListRequestTemplate[] =
{
    { CMT_DT_STRUCT_LIST, offsetof(GetPrefListRequest, length), 0, 0 },
    { CMT_DT_STRUCT_PTR, offsetof(GetPrefListRequest, list), 0, 0 },
    { CMT_DT_STRING, offsetof(GetPrefElement, key), 0, 0 },
    { CMT_DT_INT, offsetof(GetPrefElement, type), 0, 0 },
    { CMT_DT_END_STRUCT_LIST, 0, 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate GetCertExtensionTemplate[] =
{
    { CMT_DT_INT, offsetof(GetCertExtension, resID), 0, 0 },
    { CMT_DT_INT, offsetof(GetCertExtension, extension), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};

CMTMessageTemplate HTMLCertInfoRequestTemplate[] =
{
    { CMT_DT_INT, offsetof(HTMLCertInfoRequest, certID), 0, 0 },
    { CMT_DT_INT, offsetof(HTMLCertInfoRequest, showImages), 0, 0 },
    { CMT_DT_INT, offsetof(HTMLCertInfoRequest, showIssuer), 0, 0 },
    { CMT_DT_END, 0, 0, 0 }
};
