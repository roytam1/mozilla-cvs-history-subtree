/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsSOAPUtils_h__
#define nsSOAPUtils_h__

#include "nsString.h"
#include "nsIDOMElement.h"

class nsSOAPUtils {
public:
  static void GetSpecificChildElement(nsIDOMElement *aParent, 
                                      const nsAReadableString& aNamespace, 
                                      const nsAReadableString& aType, 
                                      nsIDOMElement * *aElement);
  static void GetSpecificSiblingElement(nsIDOMElement *aSibling, 
                                        const nsAReadableString& aNamespace, 
                                        const nsAReadableString& aType, 
                                        nsIDOMElement * *aElement);
  static void GetFirstChildElement(nsIDOMElement* aParent, 
                                   nsIDOMElement** aElement);
  static void GetNextSiblingElement(nsIDOMElement* aStart, 
                                    nsIDOMElement** aElement);
  static nsresult GetElementTextContent(nsIDOMElement* aElement, 
                                    nsAWritableString& aText);
  static PRBool HasChildElements(nsIDOMElement* aElement);

  static void GetNextSibling(nsIDOMNode* aSibling, 
                             nsIDOMNode **aNext);
  static nsresult MakeNamespacePrefix(nsIDOMElement* aElement,
                                     const nsAReadableString & aURI,
                                     nsAWritableString & aPrefix);
  static nsresult MakeNamespacePrefixFixed(nsIDOMElement* aElement,
                                     const nsAReadableString & aURI,
                                     nsAWritableString & aPrefix);
  static nsresult GetNamespaceURI(nsIDOMElement* aElement,
                                  const nsAReadableString & aQName, 
                                  nsAWritableString & aURI);
  static nsresult GetLocalName(const nsAReadableString & aQName, 
                                  nsAWritableString & aLocalName);

// All those missing string functions have to come from somewhere...

  static PRBool StartsWith(nsAReadableString& aSuper,
		           nsAReadableString& aSub);
  static const nsString kSOAPEnvURI;
  static const nsString kSOAPEncodingURI;
  static const nsString kSOAPEnvPrefix;
  static const nsString kSOAPEncodingPrefix;
  static const nsString kXSIURI;
  static const nsString kXSDURI;
  static const nsString kXSIPrefix;
  static const nsString kXSITypeAttribute;
  static const nsString kXSDPrefix;
  static const nsString kEncodingStyleAttribute;
  static const nsString kEnvelopeTagName;
  static const nsString kHeaderTagName;
  static const nsString kBodyTagName;
  static const nsString kFaultTagName;
  static const nsString kFaultCodeTagName;
  static const nsString kFaultStringTagName;
  static const nsString kFaultActorTagName;
  static const nsString kFaultDetailTagName;
  static const nsString kEmpty;
  static const nsString kEncodingSeparator;
  static const nsString kQualifiedSeparator;
  static const nsString kStringType;
  static const nsString kBooleanType;
  static const nsString kDoubleType;
  static const nsString kFloatType;
  static const nsString kLongType;
  static const nsString kIntType;
  static const nsString kShortType;
  static const nsString kByteType;
  static const nsString kArrayType;
  static const nsString kStructType;
  static const nsString kStructTypePrefix;
  static const nsString kLiteralType;
  static const nsString kNullType;
  static const nsString kVoidType;
  static const nsString kUnknownType;
  static const nsString kStringSchemaNamespaceURI;
  static const nsString kBooleanSchemaNamespaceURI;
  static const nsString kDoubleSchemaNamespaceURI;
  static const nsString kFloatSchemaNamespaceURI;
  static const nsString kLongSchemaNamespaceURI;
  static const nsString kIntSchemaNamespaceURI;
  static const nsString kShortSchemaNamespaceURI;
  static const nsString kByteSchemaNamespaceURI;
  static const nsString kArraySchemaNamespaceURI;
  static const nsString kStructSchemaNamespaceURI;
  static const nsString kLiteralSchemaNamespaceURI;
  static const nsString kNullSchemaNamespaceURI;
  static const nsString kVoidSchemaNamespaceURI;
  static const nsString kUnknownSchemaNamespaceURI;
  static const nsString kStringSchemaType;
  static const nsString kBooleanSchemaType;
  static const nsString kDoubleSchemaType;
  static const nsString kFloatSchemaType;
  static const nsString kLongSchemaType;
  static const nsString kIntSchemaType;
  static const nsString kShortSchemaType;
  static const nsString kByteSchemaType;
  static const nsString kArraySchemaType;
  static const nsString kStructSchemaType;
  static const nsString kLiteralSchemaType;
  static const nsString kNullSchemaType;
  static const nsString kVoidSchemaType;
  static const nsString kUnknownSchemaType;
  static const nsString kXMLNamespaceNamespaceURI;
  static const nsString kXMLNamespaceURI;
  static const nsString kXMLNamespacePrefix;
  static const nsString kXMLPrefix;
  static const nsString kTrue;
  static const nsString kFalse;
  static const nsString kTrueA;
  static const nsString kFalseA;
};

#endif
