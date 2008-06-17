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

#include "zapStunTransport.h"
#include "prnetdb.h"
#include "nsNetCID.h"
#include "nsVoidArray.h"
#include "nsIPropertyBag2.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "zapStunMessage2.h"
#include "zapIStunAttributeFactory.h"
#include "zapStunAttributes.h"
#include "zapITransportAddress.h"
#include "zapICryptoUtils.h"

class zapTransportAddress : public zapITransportAddress
{
public:
  zapTransportAddress();
  ~zapTransportAddress();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPITRANSPORTADDRESS

  nsCString mAddress;
  PRUint32  mPort;
  nsCString mTransport;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(zapTransportAddress, zapITransportAddress)

zapTransportAddress::zapTransportAddress()
  :mPort(0)
{
}

zapTransportAddress::~zapTransportAddress()
{
}

NS_IMETHODIMP 
zapTransportAddress::GetAddress(nsACString & address)
{
  address = mAddress;
  return NS_OK; 
}

NS_IMETHODIMP 
zapTransportAddress::GetPort(PRUint32 * port)
{
  NS_ENSURE_ARG(port);
  *port = mPort;
  return NS_OK; 
}

NS_IMETHODIMP 
zapTransportAddress::GetTransport(nsACString & transport)
{
  transport = mTransport;
  return NS_OK; 
}

static nsresult
computeFingerPrint(zapIStunMessage2 *message, PRUint32 & value)
{
  nsCOMPtr<zapICryptoUtils> crypto = do_GetService(ZAP_CRYPTOUTILS_CONTRACTID);
  if (!crypto) {
    NS_WARNING("Can't find CryptoUtils module");
    return NS_ERROR_FAILURE;
  }
  
  nsCString data;
  nsresult rv = message->Serialize(data);
  NS_ENSURE_SUCCESS(rv, rv);
    
  // compute the crc32 of the serialized stun message
  // excluding the last 8 bytes of the fingerprint attribute
  crypto->ComputeCRC32(StringHead(data, data.Length()-8) , &value);
                             
  value ^= 0x5354554e;
  
  return NS_OK;
}

zapStunTransport::zapStunTransport()
  : mSupportRFC3489(PR_FALSE), mUseFingerPrint(0)
{
}

zapStunTransport::~zapStunTransport()
{
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS1(zapStunTransport, zapIStunTransport)

//-----------------------------------------------------------------------------

NS_IMETHODIMP 
zapStunTransport::Init(nsIPropertyBag2 *aConfiguration)
{ 
  nsresult rv;
  nsCString validMethods;
  
  if (aConfiguration) {
    aConfiguration->GetPropertyAsBool(NS_LITERAL_STRING("supportRFC3489"), 
                                      &mSupportRFC3489);
                                            
    aConfiguration->GetPropertyAsUint32(NS_LITERAL_STRING("useFingerPrint"), 
                                        &mUseFingerPrint);
    
    aConfiguration->GetPropertyAsACString(NS_LITERAL_STRING("validMethods"), 
                                          validMethods);
  
    aConfiguration->GetPropertyAsInterface(NS_LITERAL_STRING("attributeFactory"),
                                           NS_GET_IID(zapIStunAttributeFactory),
                                           getter_AddRefs(mAttrFactory));
  }
  
  nsCStringArray methodsTokens;    
  methodsTokens.ParseString(PromiseFlatCString(validMethods).get(), ":");
  
  for (PRInt32 ii=0; ii<methodsTokens.Count(); ++ii) {
    if (!mValidMethods.AppendElement((PRUint16)methodsTokens[ii]->ToInteger(&rv)))
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  // place holder for transport address
  mTransportAddress = new zapTransportAddress();
  NS_ENSURE_TRUE(mTransportAddress, NS_ERROR_OUT_OF_MEMORY);
  
  return NS_OK;
}

NS_IMETHODIMP 
zapStunTransport::SendMessage(zapIStunMessage2     *aMessage, 
                              zapITransportAddress *aDest)
{
  NS_ENSURE_ARG(aMessage);
  NS_ENSURE_ARG(aDest);
  
  nsCString address, transport;
  PRUint32 port;
  aDest->GetAddress(address);
  aDest->GetPort(&port);
  aDest->GetTransport(transport);
  
#ifdef DEBUG
  printf("zapStunTransport::SendMessage: sending message to '%s':%d over '%s'\n", 
         PromiseFlatCString(address).get(), port, PromiseFlatCString(transport).get());
#endif    
      
  nsresult rv = NS_OK;
  
  if (mUseFingerPrint != 0) {
    nsCOMPtr<zapIStunUint32Attribute> fpAttr;
    
    nsCOMPtr<zapIStunAttribute> attribute;
    aMessage->GetAttribute(zapIStunAttribute::FINGERPRINT, getter_AddRefs(attribute));
          
    if (!attribute) {
      fpAttr = new zapStunUint32Attribute();
      NS_ENSURE_TRUE(fpAttr, NS_ERROR_OUT_OF_MEMORY);
      
      fpAttr->SetType(zapIStunAttribute::FINGERPRINT);
      rv = aMessage->AppendAttribute(fpAttr);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      fpAttr = do_QueryInterface(attribute, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    PRUint32 crc;
    rv = computeFingerPrint(aMessage, crc);
    NS_ENSURE_SUCCESS(rv, rv);
    
    fpAttr->SetValue(crc);
  } 
  
  nsCString data;
  rv = aMessage->Serialize(data);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (data.Length() >= 548) {
#ifdef DEBUG
    printf("zapStunTransport::SendMessage: serialized packet is too big (%d)\n", 
           data.Length());
#endif
    return NS_OK;
  }
  
  if (mSink)
    rv = mSink->SendPacket(data, transport, address, port);
  else {
    rv = NS_ERROR_FAILURE;
#ifdef DEBUG
    printf("zapStunTransport::SendMessage: no transport sink\n");
#endif
  }

  return rv;
}

NS_IMETHODIMP 
zapStunTransport::HandlePacket(const nsACString & aData, 
                               const nsACString & aProtocol, 
                               const nsACString & aAddress, 
                               PRUint16           aPort,
                               PRBool            *_retval)
{
  NS_ENSURE_ARG(_retval);
    
  nsresult rv = NS_OK;
    
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: got packet from '%s':%d over '%s'\n", 
           PromiseFlatCString(aAddress).get(), aPort, 
           PromiseFlatCString(aProtocol).get());
#endif

  mTransportAddress->mAddress = aAddress;
  mTransportAddress->mPort = aPort;
  mTransportAddress->mTransport = aProtocol;
  
  *_retval = PR_FALSE;
  
  // XXX - handle icmp errors
  // it seems that currently these errors are 
  // not propagated up to this layer
  if (aData.IsEmpty()) {
    // this indicate a connection failure
    // call the transport listener so it could abort pending transactions
#ifdef DEBUG
      printf("zapStunTransport::HandlePacket: connection failure from '%s':%d over '%s'\n", 
               PromiseFlatCString(aAddress).get(), aPort, 
               PromiseFlatCString(aProtocol).get());
#endif
    if (mClientListener)
      mClientListener->HandleMessage(nsnull, mTransportAddress, _retval);
  
    if (mServerListener)
      mServerListener->HandleMessage(nsnull, mTransportAddress, _retval);
  
    *_retval = PR_TRUE;
    return NS_OK; 
  } 
    
  // perform basic stun checks
  if (aData.Length() < 20 || aData.Length() >= 548) {
#ifdef DEBUG
      printf("zapStunTransport::HandlePacket: invalid msg length (%d)\n", 
        aData.Length());
#endif    
    return NS_OK;
  }
      
  // check first two bits
  if ((aData.BeginReading()[0] & 0xC0) != 0) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: non-stun packet\n");
#endif      
    return NS_OK;
  }
  
  // check the length field
  PRUint16 msgLength = PR_ntohs(((PRUint16*)aData.BeginReading())[1]);
  if (msgLength % 4) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: msg length is not a multiple of four bytes\n");
#endif
    return NS_OK;
  }
    
  // check magic cookie
  PRUint32 magicCookie = PR_ntohl(((PRUint32*)aData.BeginReading())[1]);
  if (magicCookie != zapIStunMessage2::MAGIC_COOKIE && !mSupportRFC3489) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: wrong magic cookie\n");
#endif
    return NS_OK;
  }
    
  // try to fully parse the message
  nsRefPtr<zapStunMessage2> message = new zapStunMessage2();
  NS_ENSURE_TRUE(message, NS_ERROR_OUT_OF_MEMORY);
  
  rv = message->Deserialize(aData, mAttrFactory);
  if (NS_FAILED(rv)) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: failed to deserialize the message\n");
#endif    
    return NS_OK;
  }
  
  // check method
  PRUint16 msgMethod;
  message->GetMessageMethod(&msgMethod);
  
  if (!this->isValidMethod(msgMethod)) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: method %d is not valid\n", msgMethod);
#endif
    return NS_OK;
  }

  // check finger-print
  if (mUseFingerPrint && !this->isValidFingerprint(message)) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: failed to validate fingerprint\n");
#endif      
    return NS_OK;
  }
  
  // propagate to the right listener
  PRUint16 msgClass;
  message->GetMessageClass(&msgClass);
  
  nsCOMPtr<zapIStunTransportListener> listener;
  
  PRBool ret;
  if (msgClass == zapIStunMessage2::CLASS_REQUEST && mServerListener) {
    rv = mServerListener->HandleMessage(message, mTransportAddress, &ret);
  }
  else if ((msgClass == zapIStunMessage2::CLASS_SUCCESS_RESPONSE ||
            msgClass == zapIStunMessage2::CLASS_ERROR_RESPONSE) && mClientListener) {
    rv = mClientListener->HandleMessage(message, mTransportAddress, &ret);
  }
  else if (msgClass == zapIStunMessage2::CLASS_INDICATION)  {
    // propagate to both
    PRBool indicationRet;
    
    if (mServerListener)
      mServerListener->HandleMessage(message, mTransportAddress, &indicationRet);
    ret |= indicationRet;
    
    if (mClientListener)
      mClientListener->HandleMessage(message, mTransportAddress, &indicationRet);
    ret |= indicationRet;
  }
  else {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: no listener for message class %d\n", msgClass);
#endif
    return NS_OK;
  }   

  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!ret) {
#ifdef DEBUG
    printf("zapStunTransport::HandlePacket: listener failed to handle the message."
           " rv=%d ret=%d\n", rv, ret);
#endif    
    return NS_OK;
  }
  
  *_retval = PR_TRUE;
  
  return NS_OK;
}

NS_IMETHODIMP 
zapStunTransport::SetTransportSink(zapIStunTransportSink *aSink)
{ 
  mSink = aSink;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunTransport::GetTransportSink(zapIStunTransportSink * *aSink)
{
  NS_ENSURE_ARG(aSink);
  
  *aSink = mSink;
  NS_IF_ADDREF(*aSink);
  return NS_OK;
}

NS_IMETHODIMP 
zapStunTransport::GetClientListener(zapIStunTransportListener * *aClientListener)
{
  NS_ENSURE_ARG(aClientListener);
  
  *aClientListener = mClientListener;
  NS_IF_ADDREF(*aClientListener);
  return NS_OK;
}
NS_IMETHODIMP 
zapStunTransport::SetClientListener(zapIStunTransportListener * aClientListener)
{
  mClientListener = aClientListener;
  return NS_OK;   
}

NS_IMETHODIMP 
zapStunTransport::GetServerListener(zapIStunTransportListener * *aServerListener)
{
  NS_ENSURE_ARG(aServerListener);
  
  *aServerListener = mServerListener;
  NS_IF_ADDREF(*aServerListener);
  return NS_OK;
}
NS_IMETHODIMP 
zapStunTransport::SetServerListener(zapIStunTransportListener * aServerListener)
{
  mServerListener = aServerListener;
  return NS_OK; 
}

PRBool 
zapStunTransport::isValidMethod(PRUint16 msgMethod)
{ 
  for (PRUint32 ii=0; ii<mValidMethods.Length(); ++ii)
    if (mValidMethods[ii] == msgMethod)
      return PR_TRUE;
  
  return PR_FALSE;
}

PRBool
zapStunTransport::isValidFingerprint(zapIStunMessage2 *message)
{
  nsCOMPtr<zapIStunAttribute> attribute;
  message->GetAttribute(zapIStunAttribute::FINGERPRINT, getter_AddRefs(attribute));
  
  if (!attribute) {
    if (mUseFingerPrint == 2)
      // fingerprint is a must
      return PR_FALSE;
    
    return PR_TRUE;
  }
  
  nsresult rv;
  nsCOMPtr<zapIStunUint32Attribute> fpAttr = do_QueryInterface(attribute, &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  PRUint32 crcValue, attrValue;
  rv = computeFingerPrint(message, crcValue);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  fpAttr->GetValue(&attrValue);
  
  PRBool res = (crcValue == attrValue);

#ifdef DEBUG
  if (!res) {
    printf("zapStunTransport::isValidFingerprint: fingerprint does not match\n");
  }
#endif  
    
  return res;
}
