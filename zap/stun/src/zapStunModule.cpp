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

#include "nsIGenericFactory.h"
#include "zapStunCID.h"

#include "zapStunMessage2.h"
#include "zapStunTransport.h"
#include "zapStunAttributes.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunMessage2)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunTransport)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunStringAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunRawAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunAddressAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunXorAddressAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunErrorCodeAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunUnknownAttribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunUint8Attribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunUint16Attribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunUint32Attribute)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunUint64Attribute)

static const nsModuleComponentInfo gComponents[] =
{
  {
    "Mozilla STUN Message2",
    ZAP_STUNMESSAGE2_CID,
    ZAP_STUNMESSAGE2_CONTRACTID,
    zapStunMessage2Constructor
  },
  {
    "Mozilla STUN Transport",
    ZAP_STUNTRANSPORT_CID,
    ZAP_STUNTRANSPORT_CONTRACTID,
    zapStunTransportConstructor
  },
  {
    "Mozilla STUN Address Attribute",
    ZAP_STUNADDRESSATTRIBUTE_CID,
    ZAP_STUNADDRESSATTRIBUTE_CONTRACTID,
    zapStunAddressAttributeConstructor
  },
  {
    "Mozilla STUN Xor Address Attribute",
    ZAP_STUNXORADDRESSATTRIBUTE_CID,
    ZAP_STUNXORADDRESSATTRIBUTE_CONTRACTID,
    zapStunXorAddressAttributeConstructor
  },
  {
    "Mozilla STUN String Attribute",
    ZAP_STUNSTRINGATTRIBUTE_CID,
    ZAP_STUNSTRINGATTRIBUTE_CONTRACTID,
    zapStunStringAttributeConstructor
  },
  {
    "Mozilla STUN Raw Attribute",
    ZAP_STUNRAWATTRIBUTE_CID,
    ZAP_STUNRAWATTRIBUTE_CONTRACTID,
    zapStunRawAttributeConstructor
  },
  {
    "Mozilla STUN ErrorCode Attribute",
    ZAP_STUNERRORCODEATTRIBUTE_CID,
    ZAP_STUNERRORCODEATTRIBUTE_CONTRACTID,
    zapStunErrorCodeAttributeConstructor
  },
  {
    "Mozilla STUN Uint8 Attribute",
    ZAP_STUNUINT8ATTRIBUTE_CID,
    ZAP_STUNUINT8ATTRIBUTE_CONTRACTID,
    zapStunUint8AttributeConstructor
  },
  {
    "Mozilla STUN Uint16 Attribute",
    ZAP_STUNUINT16ATTRIBUTE_CID,
    ZAP_STUNUINT16ATTRIBUTE_CONTRACTID,
    zapStunUint16AttributeConstructor
  },
  {
    "Mozilla STUN Uint32 Attribute",
    ZAP_STUNUINT32ATTRIBUTE_CID,
    ZAP_STUNUINT32ATTRIBUTE_CONTRACTID,
    zapStunUint32AttributeConstructor
  },
  {
    "Mozilla STUN Uint64 Attribute",
    ZAP_STUNUINT64ATTRIBUTE_CID,
    ZAP_STUNUINT64ATTRIBUTE_CONTRACTID,
    zapStunUint64AttributeConstructor
  },
  {
    "Mozilla STUN Unknown Attribute",
    ZAP_STUNUNKNOWNATTRIBUTE_CID,
    ZAP_STUNUNKNOWNATTRIBUTE_CONTRACTID,
    zapStunUnknownAttributeConstructor
  }
};

NS_IMPL_NSGETMODULE(zapStunModule, gComponents)
