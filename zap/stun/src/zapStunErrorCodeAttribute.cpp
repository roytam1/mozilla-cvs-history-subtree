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

#include "zapStunErrorCodeAttribute.h"
#include "prnetdb.h"


zapStunErrorCodeAttribute::zapStunErrorCodeAttribute()
  : mType(ERROR_CODE), mValueLength(4), mCode(0), mReserved(0)
{
}

zapStunErrorCodeAttribute::~zapStunErrorCodeAttribute()
{
}

/* attribute unsigned short code; */
NS_IMETHODIMP 
zapStunErrorCodeAttribute::GetCode(PRUint16 *aCode)
{
  NS_ENSURE_ARG(aCode);
  *aCode = mCode;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunErrorCodeAttribute::SetCode(PRUint16 aCode)
{
  mCode = aCode;
  
  if (!(mCode >= 300 && mCode <= 699)) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::SetCode: invalid error code (%d)\n", mCode);
#endif
    return NS_ERROR_INVALID_ARG;
  }   
  
  return NS_OK;
}

/* attribute AUTF8String reason; */
NS_IMETHODIMP 
zapStunErrorCodeAttribute::GetReason(nsACString & aReason)
{
  aReason = mReason;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunErrorCodeAttribute::SetReason(const nsACString & aReason)
{
  mReason = aReason;
  
  if (mReason.Length() > 763) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::SetReason: reason phrase too long\n");
#endif
    return NS_ERROR_INVALID_ARG;      
  }
    
  mValueLength = 4 + mReason.Length();
    
  if (mReason.Length() % 4)
    mPadding = Substring(NS_LITERAL_CSTRING("    "), 0, 4-(mReason.Length() % 4));
    
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS2(zapStunErrorCodeAttribute, zapIStunAttribute, 
                   zapIStunErrorCodeAttribute)

//-----------------------------------------------------------------------------
// zapIStunAttribute

/* attribute unsigned short type; */
NS_IMETHODIMP 
zapStunErrorCodeAttribute::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG(aType);
  *aType = mType;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunErrorCodeAttribute::SetType(PRUint16 aType)
{
  mType = aType;
  return NS_OK;
}

/* readonly attribute unsigned short length; */
NS_IMETHODIMP 
zapStunErrorCodeAttribute::GetLength(PRUint16 *aLength)
{
  NS_ENSURE_ARG(aLength);   
  *aLength = mValueLength;
  return NS_OK;
}

NS_IMETHODIMP 
zapStunErrorCodeAttribute::Serialize(zapIStunMessage2 *aMessage, nsACString & _retval)
{
  nsCString message;
  message.SetLength(4);
    
  // sanity
  if (!(mCode >= 300 && mCode <= 699)) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::Deserialize: invalid error code (%d)\n", mCode);
#endif
    return NS_ERROR_FAILURE;
  }
    
  if (mReason.Length() > 763) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::Deserialize: reason phrase too long\n");
#endif
    return NS_ERROR_FAILURE;      
  }
    
  ((PRUint16*)message.BeginWriting())[0] = PR_htons(mReserved);
    
  message.BeginWriting()[2] = mCode / 100; 
  message.BeginWriting()[3] = mCode % 100;
    
  message.Append(mReason);
  if (mPadding.Length())
    message.Append(mPadding);
    
  _retval = message;
        
  return NS_OK;
}

NS_IMETHODIMP 
zapStunErrorCodeAttribute::Deserialize(PRUint16 aType, PRUint16 aLength, 
                                       const nsACString & aData, 
                                       zapIStunMessage2 *aMessage)
{
  if (aLength < 4) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::Deserialize: invalid attribute length (%d)\n", 
      aData.Length());
#endif
    return NS_ERROR_FAILURE;    
  }
  
  mType = aType;
  mValueLength = aLength;
  
  // sanity 
  if (mType != ERROR_CODE) {
#ifdef DEBUG
    printf("zapStunErrorCodeAttribute::Deserialize: invalid attribute type (%d)\n", 
      mType);
#endif
    return NS_ERROR_FAILURE;      
  }
  
  mReserved = PR_ntohs(((PRUint16*)aData.BeginReading())[0]);
  mCode = (aData.BeginReading()[2] & 0x07) * 100;
  mCode += aData.BeginReading()[3];
  
  if (aLength > 4)
    mReason.Assign(aData.BeginReading()+4, aLength-4);
  if (aData.Length() - aLength)
    mPadding.Assign(aData.BeginReading()+aLength, aData.Length() - aLength);
    
  return NS_OK;
}
