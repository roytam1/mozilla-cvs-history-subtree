/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
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

/**
 * \file XPCCOMTypeInfo.cpp implementation XPCCOMTypeInfo
 * This file contains the implementations for XPCCOMTypeInfo class and
 * associated helper classes and functions
 */

#include "xpcprivate.h"

inline
void FillOutElemDesc(VARTYPE vt, PRUint16 paramFlags, ELEMDESC & elemDesc)
{
    elemDesc.tdesc.vt = vt;
    elemDesc.paramdesc.wParamFlags = paramFlags;
    elemDesc.paramdesc.pparamdescex = 0;
    elemDesc.tdesc.lptdesc = 0;
    elemDesc.tdesc.lpadesc = 0;
    elemDesc.tdesc.hreftype = 0;
}

void XPCJSPropertyInfo::GetReturnType(XPCCallContext& ccx, ELEMDESC & elemDesc)
{
    VARTYPE vt;
    if(IsSetterMode())     // we're working on a property
    {
        vt = VT_EMPTY;
    }
    else if(IsProperty())
    {
        vt = XPCCOMConvert::JSTypeToCOMType(ccx, mProperty);
    }
    else // Function
    {
        vt = VT_VARIANT;
    }
    FillOutElemDesc(vt, PARAMFLAG_FRETVAL, elemDesc);
}

ELEMDESC* XPCJSPropertyInfo::GetParamInfo()
{
    PRUint32 paramCount = GetParamCount();
    ELEMDESC* elemDesc;
    if(paramCount != 0)
    {
        elemDesc = new ELEMDESC[paramCount];
        for(PRUint32 index = 0; index < paramCount; ++index)
        {
            FillOutElemDesc(VT_VARIANT, PARAMFLAG_FIN, elemDesc[index]);
        }
    }
    else
        elemDesc = 0;
    return elemDesc;
}

XPCJSPropertyInfo::XPCJSPropertyInfo(JSContext* cx, PRUint32 memid, 
                                     JSObject* obj, jsval val) : 
    mPropertyType(INVALID), mMemID(memid)
{
    JSString* str = JS_ValueToString(cx, val);
    if(str)
    {
        const char* chars = JS_GetStringBytes(str);
        if(chars)
        {
            mName = chars;
            JSBool found;
            uintN attr;
            // Get the property, and if it's found and enumerable 
            if(JS_GetPropertyAttributes(cx, obj, chars, &attr, &found) && 
                found && (attr & JSPROP_ENUMERATE) != 0)
            {
                if(chars && JS_GetProperty(cx, obj, chars, &mProperty) && 
                    !JSVAL_IS_VOID(mProperty))
                {
                    if(JSVAL_IS_OBJECT(mProperty) && 
                        JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(mProperty)))
                    {
                        mPropertyType = FUNCTION;
                        JSObject * funcObj = JSVAL_TO_OBJECT(mProperty);
                        JSIdArray * funcObjArray = JS_Enumerate(cx, funcObj);
                        if(funcObjArray)
                        {
                            mParamCount = funcObjArray->length;
                        }
                    }
                    else
                    {
                        mParamCount = 0;
                        if((attr & JSPROP_READONLY) != 0)
                        {
                            mPropertyType = READONLY_PROPERTY;
                        }
                        else
                        {
                            mPropertyType = PROPERTY;
                        }
                    }
                }
            }
        }
    }
}

void XPCFuncDescArray::BuildFuncDesc(XPCCallContext& ccx, JSObject* obj, 
                                     XPCJSPropertyInfo & propInfo)
{
    FUNCDESC* funcDesc = new FUNCDESC;
    mArray.AppendElement(funcDesc);
    // zero is reserved
    funcDesc->memid = propInfo.GetMemID();
    funcDesc->lprgscode = 0;
    funcDesc->funckind = FUNC_DISPATCH;
    funcDesc->invkind = propInfo.GetInvokeKind();
    funcDesc->callconv = CC_STDCALL;
    funcDesc->cParams = propInfo.GetParamCount();
    funcDesc->lprgelemdescParam = propInfo.GetParamInfo();
    funcDesc->cParamsOpt = 0;
    // This could be a problem, supposed to be used for functions of
    // type FUNC_VIRTUAL which we aren't, so zero should be ok
    funcDesc->oVft = 0;
    funcDesc->cScodes = 0;
    funcDesc->wFuncFlags = 0;
    propInfo.GetReturnType(ccx, funcDesc->elemdescFunc);
}

XPCFuncDescArray::XPCFuncDescArray(XPCCallContext& ccx, JSObject* obj, const XPCIDArray& array, XPCNameArray & names)
{
    PRUint32 size = array.Length();
    names.SetSize(size);
    PRUint32 memid = 0;
    JSContext* cx = ccx;
    for(PRUint32 index = 0; index < size; ++index)
    {
        XPCJSPropertyInfo propInfo(cx, ++memid, obj, array[index]);
        names.SetName(index + 1, propInfo.GetName());
        BuildFuncDesc(ccx, obj, propInfo);
        if(propInfo.IsProperty() && !propInfo.IsReadOnly())
        {
            propInfo.SetSetterMode();
            BuildFuncDesc(ccx, obj, propInfo);
        }
    }
}

XPCFuncDescArray::~XPCFuncDescArray()
{
    PRUint32 size = mArray.Count();
    for(PRUint32 index = 0; index < size; ++index)
    {
        delete NS_REINTERPRET_CAST(FUNCDESC*,mArray.ElementAt(index));
    }
}

XPCCOMTypeInfo::XPCCOMTypeInfo(XPCCallContext& ccx, JSObject* obj, 
                               XPCIDArray* array) :
    mRefCnt(0), mJSObject(obj), mIDArray(ccx, array), 
    mFuncDescArray(ccx, obj, *array, mNameArray)
{
}

XPCCOMTypeInfo::~XPCCOMTypeInfo()
{
}

XPCCOMTypeInfo * XPCCOMTypeInfo::New(XPCCallContext& ccx, JSObject* obj)
{
    nsresult retval = NS_ERROR_FAILURE;
    // Saved state must be restored, all exits through 'out'...
    JSExceptionState* saved_exception = DoPreScriptEvaluated(ccx);
    JSErrorReporter older = JS_SetErrorReporter(ccx, nsnull);
    XPCCOMTypeInfo * pTypeInfo = 0;
    JSIdArray * jsArray = JS_Enumerate(ccx, obj);
    if(jsArray)
    {
        XPCIDArray* array = new XPCIDArray(ccx, jsArray);
        if(array)
        {
            pTypeInfo = new XPCCOMTypeInfo(ccx, obj, array);
        }
    }
    JS_SetErrorReporter(ccx, older);
    DoPostScriptEvaluated(ccx, saved_exception);

    return pTypeInfo;
}

STDMETHODIMP XPCCOMTypeInfo::QueryInterface(const struct _GUID & IID,void ** pPtr)
{
    if(IID == IID_ITypeInfo)
    {
        *pPtr = NS_STATIC_CAST(ITypeInfo*,this);
    }
    else if(IID == IID_IUnknown)
    {
        *pPtr = NS_STATIC_CAST(ITypeInfo*,this);
    }
    else
    {
        // We don't know this interface
        *pPtr = 0;
        return E_NOINTERFACE;
    }
    return S_OK;
}

STDMETHODIMP XPCCOMTypeInfo::GetTypeAttr( 
	/* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetTypeComp(
	/* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
{
	return NS_OK;
}

STDMETHODIMP XPCCOMTypeInfo::GetFuncDesc(
        /* [in] */ UINT index,
        /* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetVarDesc(
        /* [in] */ UINT index,
        /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetNames(
        /* [in] */ MEMBERID memid,
        /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
        /* [in] */ UINT cMaxNames,
        /* [out] */ UINT __RPC_FAR *pcNames)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetRefTypeOfImplType(
        /* [in] */ UINT index,
        /* [out] */ HREFTYPE __RPC_FAR *pRefType)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetImplTypeFlags(
        /* [in] */ UINT index,
        /* [out] */ INT __RPC_FAR *pImplTypeFlags)
{
	return NS_OK;
}

STDMETHODIMP XPCCOMTypeInfo::GetIDsOfNames(
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId)
{
    _bstr_t name;
    for(UINT index = 0; index < cNames; ++index)
    {
        name = rgszNames[index];
        nsDependentCString buffer(NS_STATIC_CAST(const char *,name));
        pMemId[index] = mNameArray.Find(buffer);

    }
	return NS_OK;
}

// TODO: Factor out with nsXPCWrappedJS::Invoke
STDMETHODIMP XPCCOMTypeInfo::Invoke(
        /* [in] */ PVOID pvInstance,
        /* [in] */ MEMBERID memid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr)
{
    IUnknown* ptr = NS_REINTERPRET_CAST(IUnknown*,pvInstance);
    IDispatch* pDisp;
    HRESULT result = ptr->QueryInterface(IID_IDispatch, 
                                         NS_REINTERPRET_CAST(void**,&pDisp));
    if(SUCCEEDED(result))
    {
        result = pDisp->Invoke(memid, IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags, 
                               pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    return result;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetDocumentation(
        /* [in] */ MEMBERID memid,
        /* [out] */ BSTR __RPC_FAR *pBstrName,
        /* [out] */ BSTR __RPC_FAR *pBstrDocString,
        /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
        /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{
    PRUint32 index = memid;
    if(index < mIDArray->Length())
    {
        XPCCallContext ccx(NATIVE_CALLER);
        JSString* str = JS_ValueToString(ccx, (*mIDArray)[index]);
        if(str)
        {
            jschar* chars = JS_GetStringChars(str);
            _bstr_t name(chars);
            *pBstrName = name.copy();
            pBstrDocString = 0;
            pdwHelpContext = 0;
            pBstrHelpFile = 0;
        }
    }
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetDllEntry(
        /* [in] */ MEMBERID memid,
        /* [in] */ INVOKEKIND invKind,
        /* [out] */ BSTR __RPC_FAR *pBstrDllName,
        /* [out] */ BSTR __RPC_FAR *pBstrName,
        /* [out] */ WORD __RPC_FAR *pwOrdinal)
{
    // We are not supporting this till a need arrises
	return E_FAIL;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetRefTypeInfo(
        /* [in] */ HREFTYPE hRefType,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::AddressOfMember(
        /* [in] */ MEMBERID memid,
        /* [in] */ INVOKEKIND invKind,
        /* [out] */ PVOID __RPC_FAR *ppv)
{
    // We are not supporting this till a need arrises
	return E_FAIL;
}
    
STDMETHODIMP XPCCOMTypeInfo::CreateInstance(
        /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj)
{
    // We are not supporting this till a need arrises
	return E_FAIL;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetMops(
        /* [in] */ MEMBERID memid,
        /* [out] */ BSTR __RPC_FAR *pBstrMops)
{
	return NS_OK;
}
    
STDMETHODIMP XPCCOMTypeInfo::GetContainingTypeLib(
        /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
        /* [out] */ UINT __RPC_FAR *pIndex)
{
    // We are not supporting this till a need arrises
	return E_FAIL;
}
    
void STDMETHODCALLTYPE XPCCOMTypeInfo::ReleaseTypeAttr( 
    /* [in] */ TYPEATTR __RPC_FAR *pTypeAttr)
{
}

void STDMETHODCALLTYPE XPCCOMTypeInfo::ReleaseFuncDesc( 
    /* [in] */ FUNCDESC __RPC_FAR *pFuncDesc)
{
}

void STDMETHODCALLTYPE XPCCOMTypeInfo::ReleaseVarDesc( 
    /* [in] */ VARDESC __RPC_FAR *pVarDesc)
{
}

