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
/*
  cmtsdr.c -- Support for the Secret Decoder Ring, which provides
      encryption and decryption using stored keys.

  Created by thayes 18 April 2000
 */
#include "stddef.h"
#include "cmtcmn.h"
#include "cmtutils.h"
#include "messages.h"
#include "rsrcids.h"
#include <string.h>

/* Encryption result - contains the key id and the resulting data */
/* An empty key id indicates that NO encryption was performed */
typedef struct EncryptionResult
{
  CMTItem keyid;
  CMTItem data;
} EncryptionResult;

/* Encrypt request */
typedef struct EncryptRequestMessage
{
  CMTItem keyid;  /* May have length 0 for default */
  CMTItem data;
} EncryptRequestMessage;

static CMTMessageTemplate EncryptRequestTemplate[] =
{
  { CMT_DT_ITEM, offsetof(EncryptRequestMessage, keyid) },
  { CMT_DT_ITEM, offsetof(EncryptRequestMessage, data) },
  { CMT_DT_END }
};

/* Encrypt reply message - SingleItemMessage */
/* Decrypt request message - SingleItemMessage */
/* Decrypt reply message - SingleItemMessage */

/* Constants for testing */
static const char *kPrefix = "Encrypted:";

/* Forward ref */
static void encrypt(CMTItem *data);
static void decrypt(CMTItem *data);

static CMTItem
CMT_CopyDataToItem(const unsigned char *data, CMUint32 len)
{
  CMTItem item;

  item.data = calloc(len, 1);
  item.len = len;
  memcpy(item.data, data, len);

  return item;
}

/* encryption request */
static CMTStatus
tmp_DoEncryptionRequest(CMTItem *message)
{
  CMTStatus rv = CMTSuccess;
  EncryptRequestMessage request;
  SingleItemMessage reply;
  CMUint32 pLen = strlen(kPrefix);

  /* Initialize */
  request.keyid.data = 0;
  request.data.data = 0;

  /* Decode incoming message */
  rv = CMT_DecodeMessage(EncryptRequestTemplate, &request, message);
  if (rv != CMTSuccess) goto loser;  /* Protocol error */

  /* Free incoming message */
  free(message->data);
  message->data = NULL;

  /* "Encrypt" by prefixing the data */
  reply.item.len = request.data.len + pLen;
  reply.item.data = calloc(reply.item.len, 1);
  if (!reply.item.data) {
	rv = CMTFailure;
	goto loser;
  }

  if (pLen) memcpy(reply.item.data, kPrefix, pLen);
  encrypt(&request.data);
  memcpy(&reply.item.data[pLen], request.data.data, request.data.len);
  
  /* Generate response */
  message->type = SSM_SDR_ENCRYPT_REPLY;
  rv = CMT_EncodeMessage(SingleItemMessageTemplate, message, &reply);
  if (rv != CMTSuccess) goto loser;  /* Unknown error */

loser:
  if (request.keyid.data) free(request.keyid.data);
  if (request.data.data) free(request.data.data);

  return rv;
}

/* decryption request */
static CMTStatus
tmp_DoDecryptionRequest(CMTItem *message)
{
  CMTStatus rv = CMTSuccess;
  SingleItemMessage request;
  SingleItemMessage reply;
  CMUint32 pLen = strlen(kPrefix);

  /* Initialize */
  request.item.data = 0;
  reply.item.data = 0;

  /* Decode the message */
  rv = CMT_DecodeMessage(SingleItemMessageTemplate, &request, message);
  if (rv != CMTSuccess) goto loser;

  /* Free incoming message */
  free(message->data);
  message->data = NULL;

  /* "Decrypt" the message by removing the key */
  if (pLen && memcmp(request.item.data, kPrefix, pLen) != 0) {
    rv = CMTFailure;  /* Invalid format */
    goto loser;
  }

  reply.item.len = request.item.len - pLen;
  reply.item.data = calloc(reply.item.len, 1);
  if (!reply.item.data) { rv = CMTFailure;  goto loser; }

  memcpy(reply.item.data, &request.item.data[pLen], reply.item.len);
  decrypt(&reply.item);

  /* Create reply message */
  message->type = SSM_SDR_DECRYPT_REPLY;
  rv = CMT_EncodeMessage(SingleItemMessageTemplate, message, &reply);
  if (rv != CMTSuccess) goto loser;

loser:
  if (request.item.data) free(request.item.data);
  if (reply.item.data) free(reply.item.data);

  return rv;
}

static CMTStatus
tmp_SendMessage(PCMT_CONTROL control, CMTItem *message)
{
  if (message->type == SSM_SDR_ENCRYPT_REQUEST) 
    return tmp_DoEncryptionRequest(message);
  else if (message->type == SSM_SDR_DECRYPT_REQUEST)
    return tmp_DoDecryptionRequest(message);

  return CMTFailure;
}
/* End test code */

CMTStatus
CMT_SDREncrypt(PCMT_CONTROL control, const unsigned char *key, CMUint32 keyLen,
               const unsigned char *data, CMUint32 dataLen,
               unsigned char **result, CMUint32 *resultLen)
{
  CMTItem message;
  EncryptRequestMessage request;
  SingleItemMessage reply;

  /* Fill in the request */
  request.keyid = CMT_CopyDataToItem(key, keyLen);
  request.data = CMT_CopyDataToItem(data, dataLen);

  /* Encode */
  if (CMT_EncodeMessage(EncryptRequestTemplate, &message, &request) != CMTSuccess) {
    goto loser;
  }

  message.type = SSM_SDR_ENCRYPT_REQUEST;

  /* Send */
  /* if (CMT_SendMessage(control, &message) != CMTSuccess) goto loser; */
  if (tmp_SendMessage(control, &message) != CMTSuccess) goto loser;

  if (message.type != SSM_SDR_ENCRYPT_REPLY) goto loser;

  if (CMT_DecodeMessage(SingleItemMessageTemplate, &reply, &message) != CMTSuccess)
    goto loser;

  *result = reply.item.data;
  *resultLen = reply.item.len;

  reply.item.data = 0;

loser:
  if (message.data) free(message.data);
  if (request.keyid.data) free(request.keyid.data);
  if (request.data.data) free(request.data.data);
  if (reply.item.data) free(reply.item.data);

  return CMTSuccess; /* need return value */
}

CMTStatus
CMT_SDRDecrypt(PCMT_CONTROL control, const unsigned char *data, CMUint32 dataLen,
               unsigned char **result, CMUint32 *resultLen)
{
  CMTItem message;
  SingleItemMessage request;
  SingleItemMessage reply;

  /* Fill in the request */
  request.item = CMT_CopyDataToItem(data, dataLen);

  /* Encode */
  if (CMT_EncodeMessage(SingleItemMessageTemplate, &message, &request) != CMTSuccess) {
    goto loser;
  }

  message.type = SSM_SDR_DECRYPT_REQUEST;

  /* Send */
  /* if (CMT_SendMessage(control, &message) != CMTSuccess) goto loser; */
  if (tmp_SendMessage(control, &message) != CMTSuccess) goto loser;

  if (message.type != SSM_SDR_DECRYPT_REPLY) goto loser;

  if (CMT_DecodeMessage(SingleItemMessageTemplate, &reply, &message) != CMTSuccess)
    goto loser;

  *result = reply.item.data;
  *resultLen = reply.item.len;

  reply.item.data = 0;

loser:
  if (message.data) free(message.data);
  if (request.item.data) free(request.item.data);
  if (reply.item.data) free(reply.item.data);

  return CMTSuccess; /* need return value */
}

/* "encrypt" */
static unsigned char mask[64] = {
 0x73, 0x46, 0x1a, 0x05, 0x24, 0x65, 0x43, 0xb4, 0x24, 0xee, 0x79, 0xc1, 0xcc,
 0x49, 0xc7, 0x27, 0x11, 0x91, 0x2e, 0x8f, 0xaa, 0xf7, 0x62, 0x75, 0x41, 0x7e,
 0xb2, 0x42, 0xde, 0x1b, 0x42, 0x7b, 0x1f, 0x33, 0x49, 0xca, 0xd1, 0x6a, 0x85,
 0x05, 0x6c, 0xf9, 0x0e, 0x3e, 0x72, 0x02, 0xf2, 0xd8, 0x9d, 0xa1, 0xb8, 0x6e,
 0x03, 0x18, 0x3e, 0x82, 0x86, 0x34, 0x1a, 0x61, 0xd9, 0x65, 0xb6, 0x7f
};

static void
encrypt(CMTItem *data)
{
  unsigned int i, j;

  j = 0;
  for(i = 0;i < data->len;i++)
  {
    data->data[i] ^= mask[j];

    if (++j >= 64) j = 0;
  }
}

static void
decrypt(CMTItem *data)
{
  encrypt(data);
}
