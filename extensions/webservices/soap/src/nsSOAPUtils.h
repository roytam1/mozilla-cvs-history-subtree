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
#include "jsapi.h"

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
  static void GetElementTextContent(nsIDOMElement* aElement, 
                                    nsAWritableString& aText);
  static PRBool HasChildElements(nsIDOMElement* aElement);

  static void GetNextSibling(nsIDOMNode* aSibling, 
                             nsIDOMNode **aNext);
#if 0
  static nsresult GetNamespacePrefix(nsIDOMElement* aElement,
                                     const nsAReadableString & aURI,
                                     nsAWritableString & aPrefix);
  static nsresult GetNamespaceURI(nsIDOMElement* aElement,
                                  const nsAReadableString & aPrefix, 
                                  nsAWritableString & aURI);
#endif
  static void GetInheritedEncodingStyle(nsIDOMElement* aEntry, 
                                        nsAWritableString & aEncodingStyle);
  static JSContext* GetSafeContext();
  static JSContext* GetCurrentContext();
  static nsresult ConvertValueToJSVal(JSContext* aContext, 
                                      nsISupports* aValue, 
                                      const nsAReadableString & aType,
                                      jsval* vp);
  static nsresult ConvertJSValToValue(JSContext* aContext,
                                      jsval val, 
                                      nsISupports** aValue,
                                      nsAWritableString & aType);

  static const nsString kSOAPEnvURI;
  static const nsString kSOAPEncodingURI;
  static const nsString kSOAPEnvPrefix;
  static const nsString kSOAPEncodingPrefix;
  static const nsString kXSIURI;
  static const nsString kXSDURI;
  static const nsString kXSIPrefix;
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
  static const nsString kSOAPCallType;
  static const nsString kEmpty;
  static const nsString kWStringType;
  static const nsString kPRBoolType;
  static const nsString kDoubleType;
  static const nsString kFloatType;
  static const nsString kPRInt64Type;
  static const nsString kPRInt32Type;
  static const nsString kPRInt16Type;
  static const nsString kCharType;
  static const nsString kArrayType;
  static const nsString kTypeSeparator;
  static const nsString kJSObjectTypePrefix;
  static const nsString kIIDObjectTypePrefix;
  static const nsString kNullType;
  static const nsString kVoidType;
  static const nsString kUnknownType;
};

#endif
