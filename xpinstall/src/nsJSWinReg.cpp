/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "jsapi.h"
//#include "nsJSUtils.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"
#include "nsWinReg.h"
#include "nsJSWinReg.h"

extern JSClass WinRegClass;
// extern JSClass WinProfileClass;

extern void nsCvrtJSValToStr(nsString&  aString,
                             JSContext* aContext,
                             jsval      aValue);

extern void nsCvrtStrToJSVal(const nsString& aProp,
                             JSContext* aContext,
                             jsval* aReturn);

extern PRBool nsCvrtJSValToBool(PRBool* aProp,
                                JSContext* aContext,
                                jsval aValue);

extern PRBool nsCvrtJSValToObj(nsISupports** aSupports,
                               REFNSIID aIID,
                               const nsString& aTypeName,
                               JSContext* aContext,
                               jsval aValue);


/***********************************************************************************/
// Native mothods for WinReg functions

//
// Native method SetRootKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetRootKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis  = (nsWinReg*)JS_GetPrivate(cx, obj);
  JSBool   rBool        = JS_FALSE;
  PRInt32  b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)
  {
    //  public int SetRootKey(PRInt32 key);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter must be a number");
      return JS_FALSE;
    }

    if(NS_OK != nativeThis->SetRootKey(b0))
    {
      return JS_FALSE;
    }

    *rval = JSVAL_VOID;
  }
  else
  {
    JS_ReportError(cx, "Function SetRootKey requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method CreateKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegCreateKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int CreateKey ( String subKey,
    //                         String className);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->CreateKey(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.CreateKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method DeleteKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegDeleteKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 1)                             
  {
    //  public int DeleteKey ( String subKey);

    nsCvrtJSValToStr(b0, cx, argv[0]);

    if(NS_OK != nativeThis->DeleteKey(b0, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.DeleteKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}


//
// Native method DeleteValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegDeleteValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsString b0;
  nsString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int DeleteValue ( String subKey,
    //                           String valueName);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->DeleteValue(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.DeleteValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method SetValueString
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int SetValueString ( String subKey,
    //                              String valueName,
    //                              String value);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);
    nsCvrtJSValToStr(b2, cx, argv[2]);

    if(NS_OK != nativeThis->SetValueString(b0, b1, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.SetValueString() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method GetValueString
//
PR_STATIC_CALLBACK(JSBool)
WinRegGetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsString* nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int GetValueString ( String subKey,
    //                              String valueName);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->GetValueString(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.GetValueString() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method SetValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegSetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
//  nsWinRegItem *b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int SetValue ( String        subKey,
    //                        String        valueName,
    //                        nsWinRegItem  *value);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);

    // fix: this parameter is an object, not a string.
    // A way needs to be figured out to convert the JSVAL to this object type
//    nsCvrtJSValToStr(b2, cx, argv[2]);

//    if(NS_OK != nativeThis->SetValue(b0, b1, b2, &nativeRet))
//    {
//      return JS_FALSE;
//    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.SetValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method GetValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegGetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg      *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsWinRegValue *nativeRet;
  nsAutoString  b0;
  nsAutoString  b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 2)                             
  {
    //  public int GetValue ( String subKey,
    //                        String valueName);

    nsCvrtJSValToStr(b0, cx, argv[0]);
    nsCvrtJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->GetValue(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.GetValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method InstallObject
//
PR_STATIC_CALLBACK(JSBool)
WinRegInstallObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  nsInstall    *nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  //  public int InstallObject ();

  nativeRet = nativeThis->InstallObject();

  *rval = INT_TO_JSVAL(nativeRet);
  return JS_TRUE;
}

//
// Native method FinalCreateKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegFinalCreateKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  PRInt32      b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int FinalCreateKey ( PRInt32 root,
    //                              String  subKey,
    //                              String  className);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter 1 must be a number");
      return JS_FALSE;
    }

    nsCvrtJSValToStr(b1, cx, argv[1]);
    nsCvrtJSValToStr(b2, cx, argv[2]);

    if(NS_OK != nativeThis->FinalCreateKey(b0, b1, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.FinalCreateKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FinalDeleteKey
//
PR_STATIC_CALLBACK(JSBool)
WinRegFinalDeleteKey(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  PRInt32      b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int FinalDeleteKey ( PRInt32 root,
    //                              String  subKey);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter 1 must be a number");
      return JS_FALSE;
    }

    nsCvrtJSValToStr(b1, cx, argv[1]);

    if(NS_OK != nativeThis->FinalDeleteKey(b0, b1, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.FinalDeleteKey() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FinalDeleteValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegFinalDeleteValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  PRInt32      b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int FinalDeleteValue ( PRInt32 root,
    //                                String  subKey,
    //                                String  className);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter 1 must be a number");
      return JS_FALSE;
    }

    nsCvrtJSValToStr(b1, cx, argv[1]);
    nsCvrtJSValToStr(b2, cx, argv[2]);

    if(NS_OK != nativeThis->FinalDeleteValue(b0, b1, b2, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.FinalDeleteValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FinalSetValueString
//
PR_STATIC_CALLBACK(JSBool)
WinRegFinalSetValueString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  PRInt32      b0;
  nsAutoString b1;
  nsAutoString b2;
  nsAutoString b3;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int FinalSetValueString ( PRInt32 root,
    //                                   String  subKey,
    //                                   String  valueName,
    //                                   String  value);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter 1 must be a number");
      return JS_FALSE;
    }

    nsCvrtJSValToStr(b1, cx, argv[1]);
    nsCvrtJSValToStr(b2, cx, argv[2]);
    nsCvrtJSValToStr(b3, cx, argv[3]);

    if(NS_OK != nativeThis->FinalSetValueString(b0, b1, b2, b3, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.FinalSetValueString() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// Native method FinalSetValue
//
PR_STATIC_CALLBACK(JSBool)
WinRegFinalSetValue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinReg     *nativeThis = (nsWinReg*)JS_GetPrivate(cx, obj);
  PRInt32      nativeRet;
  PRInt32      b0;
  nsAutoString b1;
  nsAutoString b2;
//  nsWinRegItem *b3;

  *rval = JSVAL_NULL;

  // If there's no private data, this must be the prototype, so ignore
  if(nsnull == nativeThis)
  {
    return JS_TRUE;
  }

  if(argc >= 3)
  {
    //  public int FinalSetValue ( PRInt32      root,
    //                             String       subKey,
    //                             String       valueName);
    //                             nsWinRegItem *value);

    if(!JS_ValueToInt32(cx, argv[0], (int32 *)&b0))
    {
      JS_ReportError(cx, "Parameter 1 must be a number");
      return JS_FALSE;
    }

    nsCvrtJSValToStr(b1, cx, argv[1]);
    nsCvrtJSValToStr(b2, cx, argv[2]);

    // fix: this parameter is an object, not a string.
    // A way need to be figured out to convert the JSVAL to this object type
//    nsCvrtJSValToStr(b3, cx, argv[3]);

//    if(NS_OK != nativeThis->FinalSetValue(b0, b1, b2, b3, &nativeRet))
//    {
//      return JS_FALSE;
//    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "WinReg.FinalSetValue() parameters error");
    return JS_FALSE;
  }

  return JS_TRUE;
}

//
// WinReg constructor
//
PR_STATIC_CALLBACK(JSBool)
WinReg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}

//
// WinProfile constructor
//
PR_STATIC_CALLBACK(JSBool)
WinProfile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}

