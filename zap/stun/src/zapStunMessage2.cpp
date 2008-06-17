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
 * Portions created by the Initial Developer are Copyright (C) 2005-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Eilon Yardeni <eyardeni@8x8.com> (original author)
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

#include "zapStunMessage2.h"
#include "prnetdb.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsIRandomGenerator.h"
#include "nsStringAPI.h"
#include "nsMemory.h"
#include "nsComponentManagerUtils.h"
#include "zapIStunAttribute.h"
#include "zapStunRawAttribute.h"
#include "zapStunAttributeFactory.h"


static PRUint16 
EncodeMessageType(PRUint16 aClass, PRUint16 aMethod)
{
  PRUint16 msgType = aClass;
  
  msgType |= (aMethod & 0x000f);
  msgType |= (aMethod & 0x0f00) << 2;
  
  PRUint16 tmp = (aMethod & 0x00f0) << 1;
  msgType |= (tmp & 0x00f0);
  msgType |= (tmp & 0x0f00) << 1;
  
  return msgType;
}

static void
DecodeMessageType(PRUint16 msgType, PRUint16& aClass, PRUint16& aMethod)
{
  aClass  = msgType & 0x0110;
  aMethod = 0;
  
  msgType &= 0xfeef;
    
  aMethod |= (msgType & 0x000f);
  aMethod |= (msgType & 0x00f0) >> 1;
  aMethod |= (msgType & 0x3e00) >> 2;   
}

NS_IMPL_ISUPPORTS1(zapStunMessage2, zapIStunMessage2)

zapStunMessage2::zapStunMessage2()
  : mClass(0),
    mMethod(0),
    mLength(0),
    mMagicCookie(MAGIC_COOKIE)
{
}

zapStunMessage2::~zapStunMessage2()
{
}

/* attribute unsigned short messageClass; */
NS_IMETHODIMP 
zapStunMessage2::GetMessageClass(PRUint16 *aClass)
{
  NS_ENSURE_ARG(aClass);
  *aClass = mClass;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunMessage2::SetMessageClass(PRUint16 aClass)
{
  mClass = aClass;
  return NS_OK;
}

/* attribute unsigned short messageMethod; */
NS_IMETHODIMP 
zapStunMessage2::GetMessageMethod(PRUint16 *aMethod)
{
  NS_ENSURE_ARG(aMethod);
  *aMethod = mMethod;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunMessage2::SetMessageMethod(PRUint16 aMethod)
{
  mMethod = aMethod;
  return NS_OK;
}

/* readonly attribute unsigned short messageLength; */
NS_IMETHODIMP 
zapStunMessage2::GetMessageLength(PRUint16 *aMessageLength)
{
  NS_ENSURE_ARG(aMessageLength);

  // re-compute length as attribute values may change 
  nsresult rv = computeLength();
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aMessageLength = mLength;
  return NS_OK;
}

/* attribute unsigned long magicCookie; */
NS_IMETHODIMP 
zapStunMessage2::GetMagicCookie(PRUint32 *aMagicCookie)
{
  NS_ENSURE_ARG(aMagicCookie);
  *aMagicCookie = mMagicCookie;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunMessage2::SetMagicCookie(PRUint32 aMagicCookie)
{
  mMagicCookie = aMagicCookie;
  return NS_OK;
}

/* attribute ACString transactionID; */
NS_IMETHODIMP 
zapStunMessage2::GetTransactionID(nsACString & aTransactionID)
{
  aTransactionID = Substring((char*)mTransactionID, (char*)mTransactionID+12);
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage2::SetTransactionID(const nsACString & aTransactionID)
{
  if (aTransactionID.Length() != 12)
    return NS_ERROR_INVALID_ARG;

  memcpy(mTransactionID, aTransactionID.BeginReading(), 12);
  return NS_OK;
}

/* ACString serialize (); */
NS_IMETHODIMP 
zapStunMessage2::Serialize(nsACString & _retval)
{
  nsCString message;
  nsresult rv;
    
  // start by allocating and writing the header
  message.SetLength(20);
    
  rv = computeLength();
  NS_ENSURE_SUCCESS(rv, rv);
    
  ((PRUint16*)message.BeginWriting())[0] = PR_htons(EncodeMessageType(mClass, mMethod));
  ((PRUint16*)message.BeginWriting())[1] = PR_htons(mLength);
  ((PRUint32*)message.BeginWriting())[1] = PR_htonl(mMagicCookie);
    
  memcpy(message.BeginWriting()+8, mTransactionID, 12);
    
  // writing index into the packet
  PRUint16 index = 20;
    
  // serialize the attributes
  PRUint32 count = 0;
  if (mAttributes)
    mAttributes->GetLength(&count);
    
  for (PRUint32 ii=0; ii<count; ++ii) {
    nsCOMPtr<zapIStunAttribute> attribute = do_QueryElementAt(mAttributes, ii, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint16 attrType, attrLength;
    attribute->GetType(&attrType);
    attribute->GetLength(&attrLength);
      
    message.SetLength(message.Length() + 4);
      
    ((PRUint16*)message.BeginWriting())[index/2] = PR_htons(attrType);
    index += 2;
    ((PRUint16*)message.BeginWriting())[index/2] = PR_htons(attrLength);
    index += 2;
      
    nsCString attrData;
    rv = attribute->Serialize(this, attrData);
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      printf("zapStunMessage2::Serialize: failed to serialize attribute %d\n", 
        attrType);
#endif      
      return rv;
    }
    
    message.Append(attrData);
    index += attrData.Length();
  }
  
  _retval = message;
  
  return NS_OK;
}

/* void deserialize (in ACString aPacket, in zapIStunAttributeFactory aAttributeFactory); */
NS_IMETHODIMP 
zapStunMessage2::Deserialize(const nsACString & aPacket, 
                             zapIStunAttributeFactory *aAttributeFactory)
{
  PRUint32 packetLength = aPacket.Length();
  
  // sanity checks
  if (packetLength < 20) {
#ifdef DEBUG
    printf("zapStunMessage2::Deserialize: message too short (%d)\n", packetLength);
#endif
    return NS_ERROR_FAILURE;
  }
  
  if (packetLength % 4) {
#ifdef DEBUG
    printf("zapStunMessage2::Deserialize: message length (%d) not a multiple of 4\n", 
      packetLength);
#endif
    return NS_ERROR_FAILURE;
  }
  
  // parse header:
  DecodeMessageType(PR_ntohs(((PRUint16*)aPacket.BeginReading())[0]), 
                    mClass, mMethod);
  mLength = PR_ntohs(((PRUint16*)aPacket.BeginReading())[1]);
  mMagicCookie   = PR_ntohl(((PRUint32*)aPacket.BeginReading())[1]);
  
  if (mLength % 4) {
#ifdef DEBUG
    printf("zapStunMessage2::Deserialize: message length field (%d) not a multiple of 4\n", 
      mLength);
#endif
    return NS_ERROR_FAILURE;
  }
  
  if (packetLength - 20 != mLength) {
#ifdef DEBUG
    printf("zapStunMessage2::Deserialize: message length inconsistent\n");
#endif
    return NS_ERROR_FAILURE;    
  }
  
  memcpy(mTransactionID, aPacket.BeginReading()+8, 12);
 
  // reading index from the packet
  PRUint16 index = 20;
  
  // parse attributes
  
  // Section 15.4: with the exception of the FINGERPRINT
  // attribute, which appears after MESSAGE-INTEGRITY, agents MUST ignore
  // all other attributes that follow MESSAGE-INTEGRITY.
  PRBool hasMessageIntegrity = PR_FALSE;

  nsCOMPtr<zapIStunAttributeFactory> baseFactory = new zapStunAttributeFactory();
  NS_ENSURE_TRUE(baseFactory, NS_ERROR_OUT_OF_MEMORY);
  
  while (index < aPacket.Length()) {
    PRUint16 attrType   = PR_ntohs(((PRUint16*)aPacket.BeginReading())[index/2]);
    index += 2;
    PRUint16 attrLength = PR_ntohs(((PRUint16*)aPacket.BeginReading())[index/2]);
    index += 2;
    
    if (index + attrLength > (PRUint16)aPacket.Length()) {
#ifdef DEBUG
      printf("zapStunMessage2::Deserialize: message framing error\n");
#endif
      return NS_ERROR_FAILURE;
    }
    
    PRUint16 paddedAttrLength = attrLength;
    if (attrLength % 4) {
      // add padding to the length
      paddedAttrLength += (4 - (attrLength % 4)); 
    }
    
    if (index + paddedAttrLength > (PRUint16)aPacket.Length()) {
#ifdef DEBUG
      printf("zapStunMessage2::Deserialize: padded attribute length too big\n");
#endif
      return NS_ERROR_FAILURE;
    }
    
    if (hasMessageIntegrity && attrType != zapIStunAttribute::FINGERPRINT) {
      // ignore the attribute    
      index += paddedAttrLength;
      continue;
    }
 
    // check for MESSAGE-INTEGRITY
    if (attrType == zapIStunAttribute::MESSAGE_INTEGRITY)
      hasMessageIntegrity = PR_TRUE;      
        
    nsCOMPtr<zapIStunAttribute> attribute;
    
    // try the base attribute factory   
    nsresult rv = baseFactory->GetAttribute(attrType, getter_AddRefs(attribute));
    if (aAttributeFactory && (NS_FAILED(rv) || !attribute)) {
      // try the attribute factory provided by the usage     
      rv = aAttributeFactory->GetAttribute(attrType, getter_AddRefs(attribute));
    }
    
    if (NS_FAILED(rv) || !attribute) {
#ifdef DEBUG
      printf("zapStunMessage2::Deserialize: unknown attribute type %d\n", attrType);
#endif
      // check if attribute is comprehension-required
      if (attrType <= 0x7FFF) {
        if (!mUnknownCRAttributes.AppendElement(attrType))
          return NS_ERROR_OUT_OF_MEMORY; 
      }
      
      // add the unknown attribute to the attribute list as a binary value
      // attribute. this is needed for maintaining the binary format
      // of the incoming message for fingerprint/message-integrity computation.
      attribute = new zapStunRawAttribute();
      NS_ENSURE_TRUE(attribute, NS_ERROR_OUT_OF_MEMORY);
    }
    
    nsCString attrData;
    attrData.Assign(Substring(aPacket, index, paddedAttrLength));
    
    rv = attribute->Deserialize(attrType, attrLength, attrData, this);    
    NS_ENSURE_SUCCESS(rv, rv);
          
    rv = this->AppendAttribute(attribute);
    NS_ENSURE_SUCCESS(rv, rv);
      
    index += paddedAttrLength;  
  }
  
  return NS_OK;
}

/* void getUnknownCRAttributes (out unsigned long aCount, [array, size_is (aCount), retval] out unsigned short aAttributes); */
NS_IMETHODIMP 
zapStunMessage2::GetUnknownCRAttributes(PRUint32 *aCount, PRUint16 **aAttributes)
{
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG(aAttributes);
  
  *aCount = mUnknownCRAttributes.Length();
  
  if (*aCount) {
    *aAttributes = (PRUint16*)nsMemory::Alloc(*aCount * sizeof(PRUint16));
    if (!*aAttributes)
      return NS_ERROR_OUT_OF_MEMORY;

    for (PRUint32 ii=0; ii<*aCount; ++ii) {
      (*aAttributes)[ii] = mUnknownCRAttributes[ii];
    }
  }

  return NS_OK;
}

/* void initTransactionID (); */
NS_IMETHODIMP 
zapStunMessage2::InitTransactionID()
{
  nsresult rv;
  nsCOMPtr<nsIRandomGenerator> rg = 
    do_CreateInstance("@mozilla.org/security/random-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint8 *buffer;
  rv = rg->GenerateRandomBytes(12, &buffer);
  NS_ENSURE_SUCCESS(rv, rv);
  
  memcpy(mTransactionID, buffer, 12);
  NS_Free(buffer);
  
  return NS_OK;
}

/* zapIStunAttribute getAttribute (in unsigned short aType); */
NS_IMETHODIMP 
zapStunMessage2::GetAttribute(PRUint16 aType, zapIStunAttribute **_retval)
{
  NS_ENSURE_ARG(_retval);

  *_retval = nsnull;
  
  if (!mAttributes)
    return NS_OK;
  
  PRUint32 count;
  mAttributes->GetLength(&count);
  
  nsresult rv;
  for (PRUint32 ii=0; ii<count; ++ii) {
    nsCOMPtr<zapIStunAttribute> attribute = 
      do_QueryElementAt(mAttributes, ii, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint16 type;
    attribute->GetType(&type);
    
    if (type == aType) {
      *_retval = attribute;
      NS_ADDREF(*_retval);
      break;
    }   
  }
  
  return NS_OK;
}

/* void removeAttribute (in unsigned short aType); */
NS_IMETHODIMP 
zapStunMessage2::RemoveAttribute(PRUint16 aType)
{
  if (!mAttributes)
    return NS_OK;
  
  PRUint32 count;
  mAttributes->GetLength(&count);
    
  nsresult rv;
  for (PRUint32 ii=0; ii<count; ++ii) {
    nsCOMPtr<zapIStunAttribute> attribute = 
      do_QueryElementAt(mAttributes, ii, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint16 type;
    attribute->GetType(&type);
    
    if (type == aType) {
      mAttributes->RemoveElementAt(ii);
      break;
    }   
  }
  
  return NS_OK;
}

/* void appendAttribute (in zapIStunAttribute aAttribute); */
NS_IMETHODIMP 
zapStunMessage2::AppendAttribute(zapIStunAttribute *aAttribute)
{
  NS_ENSURE_ARG(aAttribute);
  nsresult rv;
    
  if (!mAttributes) { 
    mAttributes = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = mAttributes->AppendElement(aAttribute, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/* nsISimpleEnumerator getAttributeEnumerator (); */
NS_IMETHODIMP 
zapStunMessage2::GetAttributeEnumerator(nsISimpleEnumerator **_retval)
{
  NS_ENSURE_ARG(_retval);
    
  if (!mAttributes)
    *_retval = nsnull;
  else
    mAttributes->Enumerate(_retval);
  return NS_OK;
}

nsresult 
zapStunMessage2::computeLength()
{
  mLength = 0;
  
  if (!mAttributes)
    return NS_OK;
  
  PRUint32 count;
  mAttributes->GetLength(&count);

  nsresult rv;  
  for (PRUint32 ii=0; ii<count; ++ii) {
    nsCOMPtr<zapIStunAttribute> attribute = 
      do_QueryElementAt(mAttributes, ii, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint16 attrLength;
    attribute->GetLength(&attrLength);
    
    attrLength += 4; // add attribute header
  
    if (attrLength % 4)
      attrLength += 4 - attrLength % 4; // add padding
  
    mLength += attrLength;  
  }
  
  return NS_OK;
}
