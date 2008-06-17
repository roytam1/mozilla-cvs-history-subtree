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

 
#ifndef __ZAP_STUNSMODULE_H__
#define __ZAP_STUNSMODULE_H__
 
#define ZAP_STUNMESSAGE2_CID \
{ 0x2827343a, 0x3475, 0x45bb, \
{ 0x96, 0x96, 0xb2, 0x77, 0xd4, 0x4b, 0xf5, 0x9c } }
#define ZAP_STUNMESSAGE2_CONTRACTID "@mozilla.org/zap/stun-message2;1"

#define ZAP_STUNTRANSPORT_CID \
{ 0xbca52e83, 0xa471, 0x41bc, \
{ 0x89, 0x6c, 0x2c, 0xea, 0x03, 0xf5, 0x69, 0x14 } }
#define ZAP_STUNTRANSPORT_CONTRACTID "@mozilla.org/zap/stun-transport;1"

#define ZAP_STUNADDRESSATTRIBUTE_CID \
{ 0xdf9bac34, 0x7acd, 0x4479, \
{ 0x89, 0x98, 0xe2, 0x2e, 0x18, 0x5b, 0xe9, 0xed } }
#define ZAP_STUNADDRESSATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-address-attribute;1"

#define ZAP_STUNXORADDRESSATTRIBUTE_CID \
{ 0x2587609f, 0x222d, 0x4fb4, \
{ 0x94, 0xdd, 0xf1, 0x0d, 0x5c, 0x2b, 0x68, 0x13 } }
#define ZAP_STUNXORADDRESSATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-xor-address-attribute;1"

#define ZAP_STUNSTRINGATTRIBUTE_CID \
{ 0xc3965bcf, 0x4c72, 0x423b, \
{ 0xb1, 0xcf, 0xbb, 0xb1, 0x71, 0x00, 0x2b, 0x80 } }
#define ZAP_STUNSTRINGATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-string-attribute;1"

#define ZAP_STUNRAWATTRIBUTE_CID \
{ 0xe897e30b, 0x4827, 0x4273, \
{ 0xae, 0x25, 0xc0, 0x66, 0x8b, 0xae, 0x6a, 0xed } }
#define ZAP_STUNRAWATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-raw-attribute;1"

#define ZAP_STUNERRORCODEATTRIBUTE_CID \
{ 0x238c8a34, 0xda5d, 0x486c, \
{ 0x8c, 0x2e, 0xde, 0xdd, 0x96, 0xbf, 0xe6, 0x31 } }
#define ZAP_STUNERRORCODEATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-error-code-attribute;1"

#define ZAP_STUNUINT16ATTRIBUTE_CID \
{ 0x568707a7, 0x4b7e, 0x45d2, \
{ 0xa1, 0xb1, 0x05, 0xba, 0x56, 0xf4, 0xaa, 0x6b } }
#define ZAP_STUNUINT16ATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-uint16-attribute;1"

#define ZAP_STUNUINT8ATTRIBUTE_CID \
{ 0xe0bb6d5d, 0x38c4, 0x4d63, \
{ 0xb7, 0x32, 0x14, 0xf0, 0x1b, 0x5a, 0x35, 0x26 } }
#define ZAP_STUNUINT8ATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-uint8-attribute;1"

#define ZAP_STUNUINT32ATTRIBUTE_CID \
{ 0xea2c8dfc, 0x5615, 0x4977, \
{ 0x83, 0x49, 0xbc, 0xb4, 0xd7, 0x30, 0xd7, 0x7e } }
#define ZAP_STUNUINT32ATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-uint32-attribute;1"

#define ZAP_STUNUINT64ATTRIBUTE_CID \
{ 0x03d2157d, 0x2390, 0x4f79, \
{ 0x9f, 0x73, 0xbb, 0xff, 0x8c, 0x34, 0x8b, 0xa1 } }
#define ZAP_STUNUINT64ATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-uint64-attribute;1"

#define ZAP_STUNUNKNOWNATTRIBUTE_CID \
{ 0x20a6d5b3, 0x3e7a, 0x4a61, \
{ 0x90, 0x7f, 0x19, 0x18, 0x34, 0x01, 0x24, 0x88 } }
#define ZAP_STUNUNKNOWNATTRIBUTE_CONTRACTID "@mozilla.org/zap/stun-unknown-attribute;1"

#endif // __ZAP_STUNSMODULE_H__
