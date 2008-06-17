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
 
#include "zapStunAttributeFactory.h"
#include "zapStunAttributes.h"
 
NS_IMPL_ISUPPORTS1(zapStunAttributeFactory, zapIStunAttributeFactory)

zapStunAttributeFactory::zapStunAttributeFactory()
{
}

zapStunAttributeFactory::~zapStunAttributeFactory()
{
}

/* zapIStunAttribute getAttribute (in unsigned short aType); */
NS_IMETHODIMP 
zapStunAttributeFactory::GetAttribute(PRUint16 aType, 
                                      zapIStunAttribute **_retval)
{
  NS_ENSURE_ARG(_retval);
  
  zapIStunAttribute* attr = nsnull;

  switch (aType) {
  /* rfc3489bis */
    case zapIStunAttribute::MAPPED_ADDRESS:
      attr = new zapStunAddressAttribute();
      break;
    case zapIStunAttribute::USERNAME:
      attr = new zapStunStringAttribute();
      break;
    case zapIStunAttribute::MESSAGE_INTEGRITY:
      attr = new zapStunRawAttribute();
      break;
    case zapIStunAttribute::ERROR_CODE:
      attr = new zapStunErrorCodeAttribute();
      break;
    case zapIStunAttribute::UNKNOWN_ATTRIBUTES:
      attr = new zapStunUnknownAttribute();
      break;
    case zapIStunAttribute::REALM:
      attr = new zapStunStringAttribute();
      break;
    case zapIStunAttribute::NONCE:
      attr = new zapStunStringAttribute();
      break;
    case zapIStunAttribute::XOR_MAPPED_ADDRESS:
      attr = new zapStunXorAddressAttribute();
      break;
    case zapIStunAttribute::SERVER:
      attr = new zapStunStringAttribute();
      break;
    case zapIStunAttribute::ALTERNATE_SERVER:
      attr = new zapStunAddressAttribute();
      break;
    case zapIStunAttribute::FINGERPRINT:
      attr = new zapStunUint32Attribute();
      break;
    default:
      *_retval = nsnull;
      return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_ENSURE_TRUE(attr, NS_ERROR_OUT_OF_MEMORY);
  attr->SetType(aType);

  *_retval = attr;
  NS_ADDREF(*_retval);

  return NS_OK;
}
