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

#ifndef nsDefaultSOAPEncoder_h__
#define nsDefaultSOAPEncoder_h__

#include "nsString.h"
#include "nsISupportsArray.h"
#include "jsapi.h"
#include "nsISOAPMessage.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"

class nsDefaultSOAPEncoder
{
public:
  static nsresult RegisterEncoders(nsISOAPEncodingRegistry* aRegistry);

protected:
  static nsresult MakeNamespacePrefix(nsIDOMElement* aScope,
		                      nsAReadableString & aURI,
				      nsAWritableString & aPrefix);
  static nsresult EncodeSimpleValue(
	                              const nsAReadableString & aValue, 
				      const nsAReadableString & aNamespaceURI, 
				      const nsAReadableString & aName, 
				      const nsAReadableString & aSchemaNamespaceURI, 
				      const nsAReadableString & aSchemaType, 
				      nsIDOMElement* aDestination);
#if 0
  nsresult StartEmptyMessage(nsISOAPMessage* message);
  nsresult EncodeParameter(nsISOAPParameter* parameter,
                           nsIDOMDocument* document,
                           nsIDOMElement** element);
  nsresult SerializeSupportsArray(nsISupportsArray* array,
                                  nsIDOMElement* element, 
                                  nsIDOMDocument* document);
  nsresult SerializeJavaScriptArray(JSObject* arrayobj,
                                    nsIDOMElement* element, 
                                    nsIDOMDocument* document);
  nsresult SerializeJavaScriptObject(JSObject* obj,
                                     nsIDOMElement* element, 
                                     nsIDOMDocument* document);
  nsresult SerializeParameterValue(nsISOAPParameter* parameter, 
                                   nsIDOMElement* element, 
                                   nsIDOMDocument* document);


  nsresult DecodeParameter(nsIDOMElement* element,
                           PRInt32 type,
                           nsISOAPParameter **_retval);
  nsresult DeserializeSupportsArray(nsIDOMElement *element,
                                    nsISupportsArray **_retval);
  nsresult DeserializeJavaScriptObject(nsIDOMElement *element,
                                       JSObject** obj);
  nsresult DeserializeParameter(nsIDOMElement *element,
                                PRInt32 type,
                                nsISOAPParameter **_retval);
#endif
};

#endif
