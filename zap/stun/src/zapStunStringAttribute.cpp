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

#include "zapStunStringAttribute.h"
#include "prnetdb.h"


zapStunStringAttribute::zapStunStringAttribute()
  : mType(0), mValueLength(0)
{
}

zapStunStringAttribute::~zapStunStringAttribute()
{
}

/* attribute ACString value; */
NS_IMETHODIMP 
zapStunStringAttribute::GetValue(nsACString & aValue)
{
  aValue = mValue;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunStringAttribute::SetValue(const nsACString & aValue)
{
  if (mType == USERNAME && aValue.Length() > 513) {
#ifdef DEBUG
    printf("zapStunStringAttribute::SetValue: username too long\n");
#endif
    return NS_ERROR_INVALID_ARG;    
  }
  else if (aValue.Length() > 763) {
#ifdef DEBUG
    printf("zapStunStringAttribute::SetValue: value too long\n");
#endif
    return NS_ERROR_INVALID_ARG;      
  }
    
  mValue = aValue;
  mValueLength = mValue.Length();
  
  if (mValue.Length() % 4)
    mPadding = Substring(NS_LITERAL_CSTRING("    "), 0, 4-(mValue.Length() % 4));
    
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsISupports

NS_IMPL_ISUPPORTS2(zapStunStringAttribute, zapIStunAttribute, 
                   zapIStunStringAttribute)

//-----------------------------------------------------------------------------
// zapIStunAttribute

/* attribute unsigned short type; */
NS_IMETHODIMP 
zapStunStringAttribute::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG(aType);
  *aType = mType;
  return NS_OK;
}
NS_IMETHODIMP 
zapStunStringAttribute::SetType(PRUint16 aType)
{
  mType = aType;
  return NS_OK;
}

/* readonly attribute unsigned short length; */
NS_IMETHODIMP 
zapStunStringAttribute::GetLength(PRUint16 *aLength)
{
  NS_ENSURE_ARG(aLength);
  *aLength = mValueLength;
  return NS_OK;
}

NS_IMETHODIMP 
zapStunStringAttribute::Serialize(zapIStunMessage2 *aMessage, nsACString & _retval)
{
  NS_ENSURE_TRUE(!mValue.IsEmpty(), NS_ERROR_FAILURE);
  
  nsCString message;
  message.Append(mValue);
  
  if (!mPadding.IsEmpty())
    message.Append(mPadding);
      
  _retval = message;
        
  return NS_OK; 
}

NS_IMETHODIMP 
zapStunStringAttribute::Deserialize(PRUint16 aType, PRUint16 aLength, 
                                    const nsACString & aData, 
                                    zapIStunMessage2 *aMessage)
{
  mType = aType;
  mValue.Assign(aData.BeginReading(), aLength);
  mValueLength = mValue.Length();
  
  if (mValueLength % 4 && aData.Length() > aLength)
    mPadding.Assign(aData.BeginReading()+aLength, aData.Length() - aLength);
  
  return NS_OK; 
}
