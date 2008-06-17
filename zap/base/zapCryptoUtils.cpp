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

#include "zapCryptoUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsICryptoHash.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"

// crc-32 table from crc32table.cpp 
extern PRUint32 crctable[256];

////////////////////////////////////////////////////////////////////////
// zapCryptoUtils

zapCryptoUtils::zapCryptoUtils()
{
}

zapCryptoUtils::~zapCryptoUtils()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapCryptoUtils)
NS_IMPL_THREADSAFE_RELEASE(zapCryptoUtils)

NS_INTERFACE_MAP_BEGIN(zapCryptoUtils)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapICryptoUtils)
  NS_INTERFACE_MAP_ENTRY(zapICryptoUtils)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapICryptoUtils methods:

/* ACString computeSHA1HMAC(in ACString text, in ACString key); */
NS_IMETHODIMP
zapCryptoUtils::ComputeSHA1HMAC(const nsACString & text,
                                const nsACString & key,
                                nsACString & retval)
{
  nsCOMPtr<nsICryptoHash> hash = do_CreateInstance("@mozilla.org/security/hash;1");

  nsCString K;
  
  // if key is longer than 64, hash it first:
  int key_length = key.Length();
  if (key_length > 64) {
    hash->Init(nsICryptoHash::SHA1);
    hash->Update((PRUint8*)PromiseFlatCString(key).get(), key_length);
    hash->Finish(PR_FALSE, K);
    key_length = 20;
  }
  else {
    K = key;
  }
  
  // pad K with zero bytes to 64 byte length:
  if (key_length < 64) {
    K.SetLength(64);
    memset(K.BeginWriting() + key_length, 0, 64 - key_length);
  }
  
  // inner = bitwise XOR key with 0x36 0x36 ... 0x36 (64 bytes)
  // outer = bitwise XOR key with 0x5C 0x5C ... 0X5C (64 bytes)
  PRUint8 inner[65], outer[65];
  PRUint8 *pK = (PRUint8*)K.BeginWriting();
  for (int i=0; i<64; ++i) {
    inner[i] = pK[i] ^ 0x36;
    outer[i] = pK[i] ^ 0x5c;
  }

  // hash (inner CONCAT text)
  nsCString Hinner;
  hash->Init(nsICryptoHash::SHA1);
  hash->Update(inner, 64);
  hash->Update((PRUint8*)PromiseFlatCString(text).get(), text.Length());
  hash->Finish(PR_FALSE, Hinner);

  // hash (outer CONCAT Hinner)
  hash->Init(nsICryptoHash::SHA1);
  hash->Update(outer, 64);
  hash->Update((PRUint8*)Hinner.get(), 20);
  hash->Finish(PR_FALSE, retval);
  
  return NS_OK;
}

/* unsigned long computeCRC32 (in ACString data); */
NS_IMETHODIMP
zapCryptoUtils::ComputeCRC32(const nsACString & data, PRUint32 *_retval)
{
  // Table-driven CRC-32 implementation as described in Ross Williams'
  // "A Painless Guide to CRC Error Detection Algorithms"

  PRUint32 len = data.Length();
  const PRUint8 *p = (const PRUint8*)data.BeginReading();
  
  PRUint32 crc = 0xFFFFFFFF;
  while (len--)
    crc = crctable[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
  *_retval = crc ^ 0xFFFFFFFF;
  
  return NS_OK;
}
