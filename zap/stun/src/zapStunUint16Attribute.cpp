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

#include "zapStunUint16Attribute.h"
#include "prnetdb.h"
#include "nsStringAPI.h"


zapStunUint16Attribute::zapStunUint16Attribute()
  : mType(0), mValueLength(4), mValue(0)
{
}

zapStunUint16Attribute::~zapStunUint16Attribute()
{
}

/* attribute unsigned short value; */
NS_IMETHODIMP 
zapStunUint16Attribute::GetValue(PRUint16 *aValue)
{
  NS_ENSURE_ARG(aValue);
  *aValue = mValue;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunUint16Attribute::SetValue(PRUint16 aValue)
{
  mValue = aValue;
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS2(zapStunUint16Attribute, zapIStunAttribute, zapIStunUint16Attribute)

//-----------------------------------------------------------------------------
// zapIStunAttribute

/* attribute unsigned short type; */
NS_IMETHODIMP 
zapStunUint16Attribute::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG(aType);
  *aType = mType;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunUint16Attribute::SetType(PRUint16 aType)
{
  mType = aType;
  return NS_OK;
}

/* readonly attribute unsigned short length; */
NS_IMETHODIMP 
zapStunUint16Attribute::GetLength(PRUint16 *aLength)
{
  NS_ENSURE_ARG(aLength);
  *aLength = mValueLength;
  return NS_OK;
}

NS_IMETHODIMP 
zapStunUint16Attribute::Serialize(zapIStunMessage2 *aMessage, nsACString & _retval)
{
  nsCString message;
  message.SetLength(4);
  
  ((PRUint16*)message.BeginWriting())[0] = PR_htons(mValue);
  ((PRUint16*)message.BeginWriting())[1] = 0;
    
  _retval = message;
        
  return NS_OK;
}

NS_IMETHODIMP 
zapStunUint16Attribute::Deserialize(PRUint16 aType, PRUint16 aLength, 
                                    const nsACString & aData, 
                                    zapIStunMessage2 *aMessage)
{
  if (aLength != 4) {
#ifdef DEBUG
    printf("zapStunUint16Attribute::Deserialize: invalid length (%d)\n", aLength);
#endif
    return NS_ERROR_FAILURE;    
  }

  mType = aType;
  mValueLength = aLength;
  
  mValue = PR_ntohs(((PRUint16*)aData.BeginReading())[0]);
  
  return NS_OK; 
}
