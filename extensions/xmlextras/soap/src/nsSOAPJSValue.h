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

#ifndef nsSOAPJSValue_h__
#define nsSOAPJSValue_h__

#include "nsString.h"
#include "jsapi.h"
#include "nsISOAPJSValue.h"
#include "nsISOAPStruct.h"
#include "nsCOMPtr.h"

class nsSOAPJSValue : public nsISOAPJSValue, public nsISOAPStruct
{
public:
  NS_DECL_ISUPPORTS

  // nsISOAPJSValue
  NS_DECL_NSISOAPJSVALUE

  // nsISOAPJSValue
  NS_DECL_NSISOAPSTRUCT

  nsSOAPJSValue();
  virtual ~nsSOAPJSValue();

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
protected:
  JSObject *mObject;
  JSContext *mContext;
};

#endif
