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

#include "zapStunUnknownAttribute.h"
#include "prnetdb.h"
#include "nsComponentManagerUtils.h"
#include "nsStringAPI.h"
#include "nsMemory.h"


zapStunUnknownAttribute::zapStunUnknownAttribute()
  : mType(UNKNOWN_ATTRIBUTES), mValueLength(0), mPadding(0)
{
}

zapStunUnknownAttribute::~zapStunUnknownAttribute()
{
}

/* void getUnknownAttributes (out unsigned long aCount, [array, size_is (aCount), retval] out unsigned short aAttrs); */
NS_IMETHODIMP 
zapStunUnknownAttribute::GetUnknownAttributes(PRUint32 *aCount, PRUint16 **aAttrs)
{
  NS_ENSURE_ARG(aCount);
  NS_ENSURE_ARG(aAttrs);
  
  *aCount = mUnknownAttributes.Length();
  
  if (*aCount) {
    *aAttrs = (PRUint16*)nsMemory::Alloc(*aCount * sizeof(PRUint16));
    if (!*aAttrs)
      return NS_ERROR_OUT_OF_MEMORY;
  
    for (PRUint32 ii=0; ii<*aCount; ++ii)
      (*aAttrs)[ii] = mUnknownAttributes[ii];
  }
  
  return NS_OK;
}

/* void setUnknownAttributes ([array, size_is (aCount)] in unsigned short aAttrs, in unsigned long aCount); */
NS_IMETHODIMP 
zapStunUnknownAttribute::SetUnknownAttributes(PRUint16 *aAttrs, PRUint32 aCount)
{
  mUnknownAttributes.Clear();
  
  for (PRUint32 ii=0; ii<aCount; ++ii) {
    if (!mUnknownAttributes.AppendElement(aAttrs[ii]))
      return NS_ERROR_OUT_OF_MEMORY; 
  }
  
  mValueLength = aCount * 2;
  
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS2(zapStunUnknownAttribute, zapIStunAttribute, 
                   zapIStunUnknownAttribute)

//-----------------------------------------------------------------------------
// zapIStunAttribute

/* attribute unsigned short type; */
NS_IMETHODIMP 
zapStunUnknownAttribute::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG(aType);   
  *aType = mType;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunUnknownAttribute::SetType(PRUint16 aType)
{
  mType = aType;
  return NS_OK;
}

/* readonly attribute unsigned short length; */
NS_IMETHODIMP 
zapStunUnknownAttribute::GetLength(PRUint16 *aLength)
{
  NS_ENSURE_ARG(aLength);   
  *aLength = mValueLength;
  return NS_OK;
}

NS_IMETHODIMP 
zapStunUnknownAttribute::Serialize(zapIStunMessage2 *aMessage, nsACString & _retval)
{   
  if (mUnknownAttributes.Length() == 0) {
#ifdef DEBUG
    printf("zapStunUnknownAttribute::Serialize: empty attributes\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  nsCString message;
  message.SetLength(mValueLength);
  if (mValueLength % 4)
    message.SetLength(message.Length() + 2);

  PRUint16 index = 0;

  for (PRUint32 ii=0; ii<mUnknownAttributes.Length(); ++ii) {
    ((PRUint16*)message.BeginWriting())[index/2] = PR_htons(mUnknownAttributes[ii]);
    
    index += 2;   
  }
  
  if (mValueLength % 4) {
    // add padding
    ((PRUint16*)message.BeginWriting())[index/2] = PR_htons(mPadding);
  }
  
  _retval = message;
        
  return NS_OK; 
}

NS_IMETHODIMP 
zapStunUnknownAttribute::Deserialize(PRUint16 aType, PRUint16 aLength, 
                                     const nsACString & aData, 
                                     zapIStunMessage2 *aMessage)
{
  if (aLength % 2) {
#ifdef DEBUG
    printf("zapStunUnknownAttribute::Deserialize: framing error\n");
#endif
    return NS_ERROR_FAILURE;          
  }
    
  mType = aType;
  mValueLength = aLength;
      
  PRUint16 index = 0;
    
  while (index < aLength) {
    PRUint16 attrType = PR_ntohs(((PRUint16*)aData.BeginReading())[index/2]);
    if (!mUnknownAttributes.AppendElement(attrType))
      return NS_ERROR_OUT_OF_MEMORY;    
    index += 2;
  }
    
  if (mValueLength % 4 && aData.Length() - aLength == 2) {
    // read padding
    mPadding = PR_ntohs(((PRUint16*)aData.BeginReading())[index/2]);
  }
  
  return NS_OK; 
}
