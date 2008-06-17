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

#include "zapStunAddressAttribute.h"
#include "nsStringAPI.h"

zapStunAddressAttribute::zapStunAddressAttribute()
  : mType(0), mValueLength(0)
{
}

zapStunAddressAttribute::~zapStunAddressAttribute()
{
}

/* readonly attribute unsigned short family; */
NS_IMETHODIMP 
zapStunAddressAttribute::GetFamily(PRUint16 *aFamily)
{
  NS_ENSURE_ARG(aFamily);
    
  if (mAddress.raw.family == PR_AF_INET)
    *aFamily = FAMILY_IPV4;
  else if (mAddress.raw.family == PR_AF_INET6)
    *aFamily = FAMILY_IPV6;
  else {
    *aFamily = 0;
  }

  return NS_OK;
}

/* attribute unsigned short port; */
NS_IMETHODIMP 
zapStunAddressAttribute::GetPort(PRUint16 *aPort)
{
  NS_ENSURE_ARG(aPort);   
  *aPort = PR_ntohs(mAddress.inet.port);
  return NS_OK;
}
NS_IMETHODIMP 
zapStunAddressAttribute::SetPort(PRUint16 aPort)
{
  mAddress.inet.port = PR_htons(aPort);
  return NS_OK;
}

/* attribute ACString address; */
NS_IMETHODIMP 
zapStunAddressAttribute::GetAddress(nsACString & aAddress)
{
  char buf[64];
  PRStatus st = PR_NetAddrToString(&mAddress, buf, sizeof(buf));
  if (st != PR_SUCCESS)
    return NS_ERROR_FAILURE; 
    
  aAddress = buf;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunAddressAttribute::SetAddress(const nsACString & aAddress)
{
  PRStatus st = PR_StringToNetAddr(PromiseFlatCString(aAddress).get(), &mAddress);
  if (st != PR_SUCCESS)                            
    return NS_ERROR_FAILURE;
  
  if (mAddress.raw.family == PR_AF_INET)
    mValueLength = 8;
  else
    mValueLength = 20;
                  
  return NS_OK;  
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS2(zapStunAddressAttribute, zapIStunAttribute, 
                   zapIStunAddressAttribute)

//-----------------------------------------------------------------------------
// zapIStunAttribute

/* attribute unsigned short type; */
NS_IMETHODIMP 
zapStunAddressAttribute::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG(aType);   
  *aType = mType;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunAddressAttribute::SetType(PRUint16 aType)
{
  mType = aType;
  return NS_OK;
}

/* readonly attribute unsigned short length; */
NS_IMETHODIMP 
zapStunAddressAttribute::GetLength(PRUint16 *aLength)
{
  NS_ENSURE_ARG(aLength); 
  *aLength = mValueLength;
  return NS_OK;
}

NS_IMETHODIMP 
zapStunAddressAttribute::Serialize(zapIStunMessage2 *aMessage, 
                                   nsACString & _retval)
{
  nsCString message;
  message.SetLength(4); 

  message.BeginWriting()[0] = 0;
  
  if (mAddress.raw.family == PR_AF_INET) {
    message.BeginWriting()[1] = FAMILY_IPV4;

    ((PRUint16*)message.BeginWriting())[1] = mAddress.inet.port;
    message.SetLength(8);
    ((PRUint32*)message.BeginWriting())[1] = mAddress.inet.ip;      
  }
  else if (mAddress.raw.family == PR_AF_INET6) {
    message.BeginWriting()[1] = FAMILY_IPV6;
    
    ((PRUint16*)message.BeginWriting())[1] = mAddress.ipv6.port;
    message.Append((char*)mAddress.ipv6.ip.pr_s6_addr, 16);
  }
  else
    return NS_ERROR_FAILURE;
    
  _retval = message;
        
  return NS_OK;
}

NS_IMETHODIMP 
zapStunAddressAttribute::Deserialize(PRUint16 aType, PRUint16 aLength, 
                                     const nsACString & aData, 
                                     zapIStunMessage2 *aMessage)
{
  if (aLength < 8) {
#ifdef DEBUG
    printf("zapStunAddressAttribute::Deserialize: invalid length (%d)\n", 
      aLength);
#endif
    return NS_ERROR_FAILURE;    
  }

  mType = aType;
  mValueLength = aLength;

  PRUint8 family = aData.BeginReading()[1];

  mAddress.inet.port = ((PRUint16*)aData.BeginReading())[1];
  
  if (family == FAMILY_IPV4) {
    if (aLength != 8) {
#ifdef DEBUG
      printf("zapStunAddressAttribute::Deserialize: invalid ipv4 length (%d)\n", 
        aLength);
#endif
      return NS_ERROR_FAILURE;      
    }

    mAddress.raw.family = PR_AF_INET;
    mAddress.inet.ip = ((PRUint32*)aData.BeginReading())[1];
  }
  else if (family == FAMILY_IPV6) {
    if (aLength != 20) {
#ifdef DEBUG
      printf("zapStunAddressAttribute::Deserialize: invalid ipv6 length (%d)\n",
         aLength);
#endif
      return NS_ERROR_FAILURE;      
    }
    mAddress.raw.family = PR_AF_INET6;
    memcpy(mAddress.ipv6.ip.pr_s6_addr, aData.BeginReading()+4, 16);    
  }
  else {
#ifdef DEBUG
    printf("zapStunAddressAttribute::Deserialize: invalid family (%d)\n", family);
#endif
    return NS_ERROR_FAILURE;    
  }

  return NS_OK;
}
