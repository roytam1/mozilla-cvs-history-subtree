/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2002 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   David Bradley <dbradley@netscape.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

#include "xpcprivate.h"

// TODO: We need to look into consolidating all these instances
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

/**
 * Returns the COM type for a given JS value
 */
VARTYPE XPCCOMConvert::JSTypeToCOMType(XPCCallContext& ccx, jsval val)
{
    if(JSVAL_IS_STRING(val))
    {
        return VT_BSTR;
    }
    else if(JSVAL_IS_INT(val))
    {
        return VT_I4;
    }
    else if(JSVAL_IS_DOUBLE(val))
    {
        return VT_R8;
    }
    else if(JSVAL_IS_OBJECT(val))
    {
        if (JS_IsArrayObject(ccx, JSVAL_TO_OBJECT(val)))
            return VT_ARRAY | VT_VARIANT;
        else
            return VT_DISPATCH;
    }
    else if(JSVAL_IS_BOOLEAN(val))
    {
        return VT_BOOL;
    }
    else if(JSVAL_IS_VOID(val))
    {
        return VT_EMPTY;
    }
    else if(JSVAL_IS_NULL(val))
    {
        return VT_NULL;
    }
    else
    {
        NS_ERROR("XPCCOMConvert::JSTypeToCOMType was unable to identify the type of the jsval");
        return VT_EMPTY;
    }
}

JSBool XPCCOMConvert::JSArrayToCOMArray(XPCCallContext& ccx, JSObject *obj, 
                                        VARIANT & var, uintN& err)
{
    err = NS_OK;
    jsuint len;
    if (!JS_GetArrayLength(ccx, obj, &len))
    {
        // TODO: I think we should create a specific error for this
        err = NS_ERROR_XPC_NOT_ENOUGH_ELEMENTS_IN_ARRAY;
        return PR_FALSE;
    }
    SAFEARRAY * array = SafeArrayCreateVector(VT_VARIANT, 0, len);
    for (long index = 0; index < len; ++index)
    {
        VARIANT arrayVar;
        jsval val;
        if(JS_GetElement(ccx, obj, index, &val) &&
           JSToCOM(ccx, val, arrayVar, err))
        {
            SafeArrayPutElement(array, &index, &arrayVar);
        }
        else
        {
            if (err == NS_OK)
                err = NS_ERROR_FAILURE;
            // This cleans up the elements as well
            SafeArrayDestroyData(array);
            return JS_FALSE;
        }
    }
    var.vt = VT_ARRAY | VT_VARIANT;
    var.parray = array;
    return JS_TRUE;
}

JSBool XPCCOMConvert::JSToCOM(XPCCallContext& ccx,
                              jsval src,
                              VARIANT & dest,
                              uintN& err)
{
    err = NS_ERROR_XPC_BAD_CONVERT_NATIVE;
    if(JSVAL_IS_STRING(src))
    {
        JSString* str = JSVAL_TO_STRING(src);
        if(str)
        {
            jschar * chars = JS_GetStringChars(str);
            if(chars)
            {
                _bstr_t val(chars);
                dest.vt = VT_BSTR;
                dest.bstrVal = val.copy();
            }
            else
            {
                return JS_FALSE;
            }
        }
        else
        {
            return JS_FALSE;
        }
    }
    else if(JSVAL_IS_INT(src))
    {
        dest.vt = VT_I4;
        dest.lVal = JSVAL_TO_INT(src);
    }
    else if(JSVAL_IS_DOUBLE(src))
    {
        dest.vt = VT_R8;
        dest.dblVal = *JSVAL_TO_DOUBLE(src);
    }
    else if(JSVAL_IS_OBJECT(src))
    {
        JSObject * obj = JSVAL_TO_OBJECT(src);
        if (JS_IsArrayObject(ccx, obj))
        {
            return JSArrayToCOMArray(ccx, obj, dest, err);
        }
        else
        {
            // only wrap JSObjects
            nsISupports * pSupport;
            XPCConvert::JSObject2NativeInterface(
                ccx, 
                (void**)&pSupport, 
                obj, 
                &kISupportsIID,
                nsnull, 
                &err);
            dest.vt = VT_DISPATCH;
            pSupport->QueryInterface(NSID_IDISPATCH, NS_REINTERPRET_CAST(void**,&dest.pdispVal));
        }
    }
    else if(JSVAL_IS_BOOLEAN(src))
    {
        dest.vt = VT_BOOL;
        dest.boolVal = JSVAL_TO_BOOLEAN(src);
    }
    else if(JSVAL_IS_VOID(src))
    {
        dest.vt = VT_EMPTY;
    }
    else if(JSVAL_IS_NULL(src))
    {
        dest.vt = VT_NULL;
        return JS_TRUE;
    }
    else
    {
        return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool XPCCOMConvert::COMArrayToJSArray(XPCCallContext& ccx, const VARIANT & src,
                                        jsval & dest,uintN& err)
{
    // We only support one dimensional arrays for now
    if (SafeArrayGetDim(src.parray) != 1)
    {
        err = NS_ERROR_FAILURE;
        return JS_FALSE;
    }
    long size;
    HRESULT hr = SafeArrayGetUBound(src.parray, 1, &size);
    if (FAILED(hr))
    {
        err = NS_ERROR_FAILURE;
        return JS_FALSE;
    }
    JSObject * array = JS_NewArrayObject(ccx, size, nsnull);
    if (!array)
    {
        err = NS_ERROR_OUT_OF_MEMORY;
    }
    // Devine the type of our array
    VARIANT var;
    if ((src.vt & VT_ARRAY) != 0)
    {
        var.vt = src.vt & ~VT_ARRAY;
    }
    else // This was maybe a VT_SAFEARRAY
    {
        SafeArrayGetVartype(src.parray, &var.vt);
    }
    jsval val;
    for (long index = 0; index <= size; ++index)
    {
        hr = SafeArrayGetElement(src.parray, &index, &var.byref);
        if (SUCCEEDED(hr))
        {
            if (COMToJS(ccx, var, val, err))
            {
                JS_SetElement(ccx, array, index, &val);
            }
            else
                return JS_FALSE;
        }
        else
        {
            err = NS_ERROR_FAILURE;
            return JS_FALSE;
        }
    }
    dest = OBJECT_TO_JSVAL(array);
    return JS_TRUE;
}

JSBool XPCCOMConvert::COMToJS(XPCCallContext& ccx, const VARIANT & src,
                              jsval & dest,uintN& err)
{
    err = NS_ERROR_XPC_BAD_CONVERT_JS;
    int x = VT_BSTR;
    int y = VT_BYREF;
    if (src.vt & VT_ARRAY || src.vt == VT_SAFEARRAY)
    {
        return COMArrayToJSArray(ccx, src, dest, err);
    }
    switch (src.vt & ~(VT_BYREF))
    {
        case VT_BSTR:
        {
            JSString * str;
            if(src.vt & VT_BYREF)
                str = JS_NewUCStringCopyZ(ccx, *src.pbstrVal);
            else
                str = JS_NewUCStringCopyZ(ccx, src.bstrVal);
            if(!str)
            {
                err = NS_ERROR_OUT_OF_MEMORY;
                return JS_FALSE;
            }
            dest = STRING_TO_JSVAL(str);
        }
        break;
        case VT_I4:
        {
            dest = INT_TO_JSVAL(src.lVal);
        }
        break;
        case VT_UI1:
        {
            dest = INT_TO_JSVAL(src.bVal);
        }
        break;
        case VT_I2:
        {
            dest = INT_TO_JSVAL(src.iVal);
        }
        break;
        case VT_R4:
        {
            dest = DOUBLE_TO_JSVAL(NS_STATIC_CAST(float,src.fltVal));
        }
        break;
        case VT_R8:
        {
            dest = DOUBLE_TO_JSVAL(src.dblVal);
        }
        break;
        case VT_BOOL:
        {
            dest = BOOLEAN_TO_JSVAL(src.boolVal);
        }
        break;
        case VT_DISPATCH:
        {
            XPCCOMObject::COMCreateFromIDispatch(src.pdispVal, ccx, JS_GetGlobalObject(ccx), &dest);
        }
        break;
        /**
         * Currently unsupported conversion types
         */
        case VT_ERROR:
        case VT_CY:
        case VT_DATE:
        case VT_UNKNOWN:
        case VT_ARRAY:
        case VT_I1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        default:
        {
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}
