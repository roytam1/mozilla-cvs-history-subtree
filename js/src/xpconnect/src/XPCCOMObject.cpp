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

#include "xpcprivate.h"

CComModule _Module;

JS_STATIC_DLL_CALLBACK(JSBool)
COMObjectConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, 
                     jsval *rval)
{
    // Make sure we were called with one string parameter
    if(argc != 1 || (argc == 1 && !JSVAL_IS_STRING(argv[0])))
    {
        // TODO: error reporting
        return JS_FALSE;
    }

    JSString * str = JS_ValueToString(cx, argv[0]);
    if(!str)
    {
        // TODO: error reporting
        return JS_FALSE;
    }

    const char * bytes = JS_GetStringBytes(str);
    if(!bytes)
    {
        // TODO: error reporting
        return JS_FALSE;
    }

    // Instantiate the desired COM object
    IDispatch* pDispatch = XPCCOMObject::COMCreateInstance(bytes);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = nsXPConnect::GetXPConnect()->WrapNative(cx, obj, 
                                  NS_REINTERPRET_CAST(nsISupports*, pDispatch),
                                  NSID_IDISPATCH, getter_AddRefs(holder));
    if(FAILED(rv) || !holder)
    {
        // TODO: error reporting
        return JS_FALSE;
    }
    JSObject * jsobj;
    if(NS_FAILED(holder->GetJSObject(&jsobj)))
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(jsobj);
    return JS_TRUE;
}

PRBool
XPCCOMObject::COMCreateFromIDispatch(IDispatch *pDispatch, JSContext *cx, JSObject *obj, jsval *rval)
{
    if (!pDispatch)
    {
        // TODO: error reporting
        return PR_FALSE;
    }

    // TODO: Do we return an existing COM object if we recognize we've already wrapped this IDispatch?

    // Instantiate the desired COM object
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = nsXPConnect::GetXPConnect()->WrapNative(cx, obj, 
                                  NS_REINTERPRET_CAST(nsISupports*, pDispatch),
                                  NSID_IDISPATCH, getter_AddRefs(holder));
    if(FAILED(rv) || !holder)
    {
        // TODO: error reporting
        return PR_FALSE;
    }
    JSObject * jsobj;
    if(NS_FAILED(holder->GetJSObject(&jsobj)))
        return PR_FALSE;
    *rval = OBJECT_TO_JSVAL(jsobj);
    return PR_TRUE;
}

IDispatch * XPCCOMObject::COMCreateInstance(const char * className)
{
    // TODO: This needs to have some error handling. We could probably
    // capture some information from the GetLastError
    // allows us to convert to BSTR for CLSID functions below
    _bstr_t bstrName(className);
    CLSID classID;
    HRESULT hr;
    // If this looks like a class ID
    if(className[0] == '{' && className[strlen(className) -1] == '}')
    {
        hr = CLSIDFromString(bstrName, &classID);
    }
    else // it's probably a prog ID
    {
        hr = CLSIDFromProgID(bstrName, &classID);
    }
    if(SUCCEEDED(hr))
    {
        IDispatch * pDisp;
        hr = CoCreateInstance(
            classID,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IDispatch,
            (LPVOID*)&pDisp);
        if(SUCCEEDED(hr))
        {
            pDisp->AddRef();
            return pDisp;
        }
        else
            return nsnull;
    }
    return nsnull;
}

PRBool XPCCOMObject::IDispatchRegisterCOMObject(JSContext * aContext, 
                                  JSObject* aGlobal)
{
    return JS_DefineFunction(aContext, aGlobal, "COMObject", 
                             COMObjectConstructor, 1, 0) ? PR_TRUE : PR_FALSE;
}

const PRUint32 VARIANT_SIZE = sizeof(VARIANT) - sizeof(VARTYPE);
#define VARIANT_BUFFER_SIZE(count) (VARIANT_SIZE * count)

JSBool XPCCOMObject::Invoke(XPCCallContext & ccx, CallMode mode)
{
    nsresult rv = ccx.CanCallNow();
    if(NS_FAILED(rv))
    {
        // If the security manager is complaining then this is not really an
        // internal error in xpconnect. So, no reason to botch the assertion.
        NS_ASSERTION(rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO, 
                     "hmm? CanCallNow failed in XPCWrappedNative::CallMethod. "
                     "We are finding out about this late!");
        return Throw(rv, ccx);
    }

    JSBool retval = JS_FALSE;
    // TODO: Remove type cast and change GetIDispatchMember to use the correct type
    XPCCOMIDispatchInterface::Member* member = NS_REINTERPRET_CAST(XPCCOMIDispatchInterface::Member*,ccx.GetIDispatchMember());
    XPCJSRuntime* rt = ccx.GetRuntime();
    XPCContext* xpcc = ccx.GetXPCContext();
    XPCPerThreadData* tls = ccx.GetThreadData();
    
    jsval* argv = ccx.GetArgv();
    uintN argc = ccx.GetArgc();

    tls->SetException(nsnull);
    xpcc->SetLastResult(NS_ERROR_UNEXPECTED);

    // set up the method index and do the security check if needed

    PRBool setter;
    PRBool getter;
    PRUint32 secFlag;
    PRUint32 secAction;

    switch(mode)
    {
        case CALL_METHOD:
            secFlag   = nsIXPCSecurityManager::HOOK_CALL_METHOD;
            secAction = nsIXPCSecurityManager::ACCESS_CALL_METHOD;
            getter = setter = PR_FALSE;
            break;
        case CALL_GETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_GET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
            setter = PR_FALSE;
            getter = PR_TRUE;
            break;
        case CALL_SETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_SET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
            setter = PR_TRUE;
            getter = PR_FALSE;
            break;
        default:
            NS_ASSERTION(0,"bad value");
            return JS_FALSE;
    }
    jsval name = member->GetName();
    nsIXPCSecurityManager* sm = xpcc->GetAppropriateSecurityManager(secFlag);
    if(sm && NS_FAILED(sm->CanAccess(secAction, &ccx, ccx,
                                     ccx.GetFlattenedJSObject(),
                                     ccx.GetWrapper()->GetIdentityObject(),
                                     ccx.GetWrapper()->GetClassInfo(), name,
                                     ccx.GetWrapper()->GetSecurityInfoAddr())))
    {
        // the security manager vetoed. It should have set an exception.
        return JS_FALSE;
    }

    XPCWrappedNative* wrapper = ccx.GetWrapper();
    nsISupports * pObj = ccx.GetTearOff()->GetNative();
    PRUint32 args = member->GetParamCount();
    uintN err;
    // TODO: I'm not sure why we need to do this I would have expected COM
    // to report one parameter
    if(setter)
        args = 1;
    if(argc < args)
        args = argc;
    const PRUint32 DEFAULT_ARG_ARRAY_SIZE = 8;
    VARIANT stackArgs[DEFAULT_ARG_ARRAY_SIZE];
    char varStackBuffer[VARIANT_BUFFER_SIZE(DEFAULT_ARG_ARRAY_SIZE)];
    char * varBuffer = args <= DEFAULT_ARG_ARRAY_SIZE ? varStackBuffer : new char[VARIANT_BUFFER_SIZE(args)];
    memset(varBuffer, 0, VARIANT_BUFFER_SIZE(args));
    VARIANT dispResult;
    EXCEPINFO exception;
    DISPPARAMS dispParams;
    dispParams.cArgs = args;
    dispParams.rgvarg = args <= DEFAULT_ARG_ARRAY_SIZE ? stackArgs : new VARIANT[args];
    DISPID propID;
    // If this is a setter, we just need to convert the first parameter
    if(setter)
    {
        propID = DISPID_PROPERTYPUT;
        dispParams.rgdispidNamedArgs = &propID;
        dispParams.cNamedArgs = 1;
        if(!XPCCOMConvert::JSToCOM(ccx, argv[0],dispParams.rgvarg[0], err))
            return ThrowBadParam(err, 0, ccx);
    }
    else if(getter)
    {
        dispParams.rgdispidNamedArgs = 0;
        dispParams.cNamedArgs = 0;
    }
    else
    {
        dispParams.cNamedArgs = 0;
        dispParams.rgdispidNamedArgs = 0;
        VARIANT* pVar = dispParams.rgvarg + args - 1;
        for(PRUint32 index = 0; index < args; ++index, --pVar)
        {
            const XPCCOMIDispatchInterface::Member::ParamInfo & paramInfo = member->GetParamInfo(index);
            if(paramInfo.IsIn())
            {
                jsval val = argv[index];
                if(paramInfo.IsOut())
                {
                    if(JSVAL_IS_PRIMITIVE(argv[index]) ||
                        !OBJ_GET_PROPERTY(ccx, JSVAL_TO_OBJECT(argv[index]),
                                          rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                                          &val))
                    {
                        ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, index, ccx);
                    }
                }
                if(!XPCCOMConvert::JSToCOM(ccx, val,*pVar, err))
                {
                    ThrowBadParam(err, index, ccx);
                }
            }
            else
            {
                paramInfo.InitializeOutputParam(varBuffer + VARIANT_SIZE * index, *pVar);
            }
        }
    }
    HRESULT invokeResult;
    {
        // avoid deadlock in case the native method blocks somehow
        AutoJSSuspendRequest req(ccx);  // scoped suspend of request
        IDispatch * pDisp;
        // TODO: I'm not sure this QI is really needed
        nsresult result = pObj->QueryInterface(NSID_IDISPATCH, (void**)&pDisp);
        if(NS_SUCCEEDED(result))
        {
            WORD dispFlags;
            if(setter)
            {
                dispFlags = DISPATCH_PROPERTYPUT;
            }
            else if(getter)
            {
                dispFlags = DISPATCH_PROPERTYGET;
            }
            else
            {
                dispFlags = DISPATCH_METHOD;
            }
            invokeResult= pDisp->Invoke(member->GetDispID(), IID_NULL, LOCALE_SYSTEM_DEFAULT, dispFlags, &dispParams, &dispResult, &exception, 0);
            if(SUCCEEDED(invokeResult))
            {
                if(!setter && !getter)
                {
                    VARIANT* pVar = dispParams.rgvarg + args - 1;
                    for(PRUint32 index = 0; index < args; ++index, --pVar)
                    {
                        const XPCCOMIDispatchInterface::Member::ParamInfo & paramInfo = member->GetParamInfo(index);
                        if(paramInfo.IsOut())
                        {
                            if(paramInfo.IsRetVal())
                            {
                                if(!ccx.GetReturnValueWasSet())
                                    ccx.SetRetVal(argv[index]);
                            }
                            else
                            {
                                jsval val;
                                if(!XPCCOMConvert::COMToJS(ccx, *pVar, val, err))
                                    ThrowBadParam(err, index, ccx);
                                // we actually assured this before doing the invoke
                                NS_ASSERTION(JSVAL_IS_OBJECT(argv[index]), "out var is not object");
                                if(!OBJ_SET_PROPERTY(ccx, JSVAL_TO_OBJECT(argv[index]),
                                            rt->GetStringID(XPCJSRuntime::IDX_VALUE), &val))
                                    ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, index, ccx);
                            }
                            CleanupVariant(*pVar);
                        }
                    }
                }
                if(!ccx.GetReturnValueWasSet())
                {
                    if(dispResult.vt != VT_EMPTY)
                    {
                        jsval val;
                        if(!XPCCOMConvert::COMToJS(ccx, dispResult, val, err))
                        {
                            ThrowBadParam(err, 0, ccx);
                        }
                        ccx.SetRetVal(val);
                    }
                    else if(setter)
                    {
                        ccx.SetRetVal(argv[0]);
                    }
                }
                retval = JS_TRUE;
            }
        }
    }
    
    xpcc->SetLastResult(invokeResult);

    if(NS_FAILED(invokeResult))
    {
        ThrowBadResult(invokeResult, ccx);
    }

    for(int index = 0; index < args; ++index)
        CleanupVariant(dispParams.rgvarg[index]);
    // Cleanup if we allocated the variant array
    if(dispParams.rgvarg != stackArgs)
        delete [] dispParams.rgvarg;
    if(varBuffer != varStackBuffer)
        delete [] varBuffer;
    return retval;
}

/**
 * throws an error
 */

JSBool XPCCOMObject::Throw(uintN errNum, JSContext* cx)
{
    XPCThrower::Throw(errNum, cx);
    return JS_FALSE;
}

static
JSBool GetMember(XPCCallContext& ccx, JSObject* funobj, XPCNativeInterface*& iface, XPCCOMIDispatchInterface::Member*& member)
{
    // We expect funobj to be a clone, we need the real funobj.

    JSFunction* fun = (JSFunction*) JS_GetPrivate(ccx, funobj);
    if(!fun)
        return JS_FALSE;
    JSObject* realFunObj = JS_GetFunctionObject(fun);
    if(!realFunObj)
        return JS_FALSE;
    jsval val;
    if(!JS_GetReservedSlot(ccx, realFunObj, 1, &val))
        return JS_FALSE;
    if(!JSVAL_IS_INT(val))
        return JS_FALSE;
    iface = NS_REINTERPRET_CAST(XPCNativeInterface*,JSVAL_TO_PRIVATE(val));
    if(!JS_GetReservedSlot(ccx, realFunObj, 0, &val))
        return JS_FALSE;
    if(!JSVAL_IS_INT(val))
        return JS_FALSE;
    member = NS_REINTERPRET_CAST(XPCCOMIDispatchInterface::Member*,JSVAL_TO_PRIVATE(val));
    return JS_TRUE;
}

// Handy macro used in many callback stub below.

#define THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper)                         \
    PR_BEGIN_MACRO                                                           \
    if(!wrapper)                                                             \
        return XPCCOMObject::Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);     \
    if(!wrapper->IsValid())                                                  \
        return XPCCOMObject::Throw(NS_ERROR_XPC_HAS_BEEN_SHUTDOWN, cx);      \
    PR_END_MACRO

/**
 * Callback for functions
 * This callback is called by JS when a function on a JSObject proxying
 * for an IDispatch instance
 */
JSBool JS_DLL_CALLBACK
XPC_IDispatch_CallMethod(JSContext* cx, JSObject* obj,
                  uintN argc, jsval* argv, jsval* vp)
{
    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    JSObject* funobj = JSVAL_TO_OBJECT(argv[-2]);
    XPCCallContext ccx(JS_CALLER, cx, obj, funobj, 0, argc, argv, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);
    ccx.SetArgsAndResultPtr(argc, argv, vp);

    XPCCOMIDispatchInterface::Member* member;
    XPCNativeInterface* iface;
    if(GetMember(ccx, funobj, iface, member))
    {
        ccx.SetIDispatchInfo(iface, member);

        return XPCCOMObject::Invoke(ccx, XPCCOMObject::CALL_METHOD);
    }
    else
        return JS_FALSE;
}

/**
 * Callback for properties
 * This callback is called by JS when a property is set or retrieved on a 
 * JSObject proxying for an IDispatch instance
 */
JSBool JS_DLL_CALLBACK
XPC_IDispatch_GetterSetter(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *vp)
{
    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    JSObject* funobj = JSVAL_TO_OBJECT(argv[-2]);

    XPCCallContext ccx(JS_CALLER, cx, obj, funobj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    ccx.SetArgsAndResultPtr(argc, argv, vp);
    XPCCOMIDispatchInterface::Member* member;
    XPCNativeInterface* iface;
    if(GetMember(ccx, funobj, iface, member))
    {
        ccx.SetIDispatchInfo(iface, member);
        return XPCCOMObject::Invoke(ccx, argc != 0 ? XPCCOMObject::CALL_SETTER : XPCCOMObject::CALL_GETTER);
    }
    else
        return JS_FALSE;
}

/******************************************************************************
 * XPCWrappedNativeTearOff's IDispatch specific members
 */
XPCCOMIDispatchInterface* XPCWrappedNativeTearOff::GetIDispatchInfo() const 
{
    NS_ASSERTION(IsIDispatch(),"We've been asked for IDispatchInfo but it's never been set");
    return NS_REINTERPRET_CAST(XPCCOMIDispatchInterface*,(((jsword)mJSObject) & ~BIT_MASK));
}

void XPCWrappedNativeTearOff::SetIDispatch(JSContext* cx) 
{
    if(nsXPConnect::GetXPConnect()->IsIDispatchSupported())
    {
        mJSObject = (JSObject*)(((jsword)XPCCOMIDispatchInterface::NewInstance(cx, mJSObject, mNative)) | IDISPATCH_BIT);
    }
}

JSObject* XPCWrappedNativeTearOff::GetIDispatchJSObject() const
{
    return GetIDispatchInfo()->GetJSObject();
}

void XPCWrappedNativeTearOff::SetIDispatchJSObject(JSObject* jsobj)
{
    GetIDispatchInfo()->SetJSObject(jsobj);
}

/******************************************************************************
 * nsXPCWrappedJS's IDispatch specific members
 */
STDMETHODIMP nsXPCWrappedJS::QueryInterface(const struct _GUID & IID,void ** pPtr)
{
    return QueryInterface(NS_REINTERPRET_CAST(REFNSIID, IID), pPtr);
}

STDMETHODIMP nsXPCWrappedJS::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

XPCCOMTypeInfo * nsXPCWrappedJS::GetCOMTypeInfo()
{
    if(!mCOMTypeInfo)
    {
        XPCCallContext ccx(NATIVE_CALLER);
        if(ccx.IsValid())
        {
            mCOMTypeInfo = XPCCOMTypeInfo::New(ccx, mJSObj);
            if(mCOMTypeInfo)
            {
                mCOMTypeInfo->AddRef();
            }
        }
    }
    return mCOMTypeInfo;
}

STDMETHODIMP nsXPCWrappedJS::GetTypeInfo(unsigned int, LCID, 
                                         ITypeInfo FAR* FAR* ppTInfo)
{
    *ppTInfo = GetCOMTypeInfo();
    (*ppTInfo)->AddRef();
    return S_OK;
}

STDMETHODIMP nsXPCWrappedJS::GetIDsOfNames(REFIID riid, 
                                           OLECHAR FAR* FAR* rgszNames, 
                                           unsigned int cNames, LCID  lcid,
                                           DISPID FAR* rgDispId)
{
    ITypeInfo * pTypeInfo = GetCOMTypeInfo();
    if(pTypeInfo != nsnull)
    {
        return pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
    }
    return S_OK;
}

void JS_DLL_CALLBACK
xpcWrappedJSErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report);

STDMETHODIMP nsXPCWrappedJS::Invoke(DISPID dispIdMember, REFIID riid, 
                                    LCID lcid, WORD wFlags,
                                    DISPPARAMS FAR* pDispParams, 
                                    VARIANT FAR* pVarResult, 
                                    EXCEPINFO FAR* pExcepInfo, 
                                    unsigned int FAR* puArgErr)
{
    XPCCOMTypeInfo* pTypeInfo = GetCOMTypeInfo();
    if(!pTypeInfo)
    {
        return E_FAIL;
    }
    XPCCallContext ccx(NATIVE_CALLER);
    XPCContext* xpcc;
    JSContext* cx;
    if(ccx.IsValid())
    {
        xpcc = ccx.GetXPCContext();
        cx = ccx.GetJSContext();
    }
    else
    {
        xpcc = nsnull;
        cx = nsnull;
    }
    const nsPromiseFlatCString & name = PromiseFlatCString(pTypeInfo->GetNameForDispID(dispIdMember));
    PRBool getter = (wFlags & DISPATCH_PROPERTYGET) != 0;
    PRBool setter = (wFlags & DISPATCH_PROPERTYPUT) != 0;
    if(getter || setter)
    {
        jsval val;
        uintN err;
        if(getter)
        {
            JS_GetProperty(cx, GetJSObject(), name.get(), &val);
            if(!XPCCOMConvert::JSToCOM(ccx, val, *pVarResult, err))
            {
                return E_FAIL;
            }
        }
        else if(pDispParams->cArgs > 0)
        {
            if(XPCCOMConvert::COMToJS(ccx, pDispParams->rgvarg[0], val, err))
            {
                JS_SetProperty(cx, GetJSObject(), name.get(), &val);
            }
        }
    }
    else // We're invoking a function
    {
        jsval* stackbase;
        jsval* sp = nsnull;
        uint8 i;
        uint8 argc=pDispParams->cArgs;
        uint8 stack_size;
        jsval result;
        uint8 paramCount=0;
        nsresult retval = NS_ERROR_FAILURE;
        nsresult pending_result = NS_OK;
        JSErrorReporter older;
        JSBool success;
        JSBool readyToDoTheCall = JS_FALSE;
        uint8 outConversionFailedIndex;
        JSObject* obj;
        jsval fval;
        nsCOMPtr<nsIException> xpc_exception;
        jsval js_exception;
        void* mark;
        JSBool foundDependentParam;
        JSObject* thisObj;
        JSExceptionState* saved_exception = nsnull;
        XPCJSRuntime* rt = ccx.GetRuntime();

        obj = thisObj = GetJSObject();

        if(cx)
            older = JS_SetErrorReporter(cx, xpcWrappedJSErrorReporter);

        // dispID's for us are 1 based not zero
        if(!cx || !xpcc)
            goto pre_call_clean_up;

        saved_exception = DoPreScriptEvaluated(cx);

        xpcc->SetPendingResult(pending_result);
        xpcc->SetException(nsnull);
        ccx.GetThreadData()->SetException(nsnull);

        // We use js_AllocStack, js_Invoke, and js_FreeStack so that the gcthings
        // we use as args will be rooted by the engine as we do conversions and
        // prepare to do the function call. This adds a fair amount of complexity,
        // but is a good optimization compared to calling JS_AddRoot for each item.

        // setup stack

        // allocate extra space for function and 'this'
        stack_size = argc + 2;


        // In the xpidl [function] case we are making sure now that the 
        // JSObject is callable. If it is *not* callable then we silently 
        // fallback to looking up the named property...
        // (because jst says he thinks this fallback is 'The Right Thing'.)
        //
        // In the normal (non-function) case we just lookup the property by 
        // name and as long as the object has such a named property we go ahead
        // and try to make the call. If it turns out the named property is not
        // a callable object then the JS engine will throw an error and we'll
        // pass this along to the caller as an exception/result code.
        fval = OBJECT_TO_JSVAL(obj);
        if(JS_TypeOfValue(ccx, fval) != JSTYPE_FUNCTION && 
            !JS_GetProperty(cx, obj, name.get(), &fval))
        {
            // XXX We really want to factor out the error reporting better and
            // specifically report the failure to find a function with this name.
            // This is what we do below if the property is found but is not a
            // function. We just need to factor better so we can get to that
            // reporting path from here.
            goto pre_call_clean_up;
        }

        // if stack_size is zero then we won't be needing a stack
        if(stack_size && !(stackbase = sp = js_AllocStack(cx, stack_size, &mark)))
        {
            retval = NS_ERROR_OUT_OF_MEMORY;
            goto pre_call_clean_up;
        }

        // this is a function call, so push function and 'this'
        if(stack_size != argc)
        {
            *sp++ = fval;
            *sp++ = OBJECT_TO_JSVAL(thisObj);
        }

        // make certain we leave no garbage in the stack
        for(i = 0; i < argc; i++)
        {
            sp[i] = JSVAL_VOID;
        }

        uintN err;
        // build the args
        for(i = 0; i < argc; i++)
        {
            jsval val;
            if((pDispParams->rgvarg[i].vt & VT_BYREF) == 0)
            {
                if(!XPCCOMConvert::COMToJS(ccx, pDispParams->rgvarg[i], val, err))
                    goto pre_call_clean_up;
                *sp++ = val;
            }
            else
            {
                // create an 'out' object
                JSObject* out_obj = JS_NewObject(cx, nsnull, nsnull, nsnull);
                if(!out_obj)
                {
                    retval = NS_ERROR_OUT_OF_MEMORY;
                    goto pre_call_clean_up;
                }
                // We'll assume in/out
                // TODO: I'm not sure we tell out vs in/out
                OBJ_SET_PROPERTY(cx, out_obj,
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                        &val);
                *sp++ = OBJECT_TO_JSVAL(out_obj);
            }
        }

        readyToDoTheCall = JS_TRUE;

pre_call_clean_up:

        if(!readyToDoTheCall)
            goto done;

        // do the deed - note exceptions

        JS_ClearPendingException(cx);

        if(!JSVAL_IS_PRIMITIVE(fval))
        {
            // Lift current frame (or make new one) to include the args
            // and do the call.
            JSStackFrame *fp, *oldfp, frame;
            jsval *oldsp;

            fp = oldfp = cx->fp;
            if(!fp)
            {
                memset(&frame, 0, sizeof frame);
                cx->fp = fp = &frame;
            }
            oldsp = fp->sp;
            fp->sp = sp;

            success = js_Invoke(cx, argc, JSINVOKE_INTERNAL);

            result = fp->sp[-1];
            fp->sp = oldsp;
            if(oldfp != fp)
                cx->fp = oldfp;
        }
        else
        {
            // The property was not an object so can't be a function.
            // Let's build and 'throw' an exception.

            static const nsresult code =
                    NS_ERROR_XPC_JSOBJECT_HAS_NO_FUNCTION_NAMED;
            static const char format[] = "%s \"%s\"";
            const char * msg;
            char* sz = nsnull;

            if(nsXPCException::NameAndFormatForNSResult(code, nsnull, &msg) && msg)
                sz = JS_smprintf(format, msg, name);

            nsCOMPtr<nsIException> e;

            XPCConvert::ConstructException(code, sz, "IDispatch", name.get(),
                                           nsnull, getter_AddRefs(e));
            xpcc->SetException(e);
            if(sz)
                JS_smprintf_free(sz);
        }

        /* this one would be set by our error reporter */
// remove after we handle COM exceptions properly
#if 0
        xpcc->GetException(getter_AddRefs(xpc_exception));
        if(xpc_exception)
            xpcc->SetException(nsnull);

        // get this right away in case we do something below to cause JS code
        // to run on this JSContext
        pending_result = xpcc->GetPendingResult();

        /* JS might throw an expection whether the reporter was called or not */
        if(JS_GetPendingException(cx, &js_exception))
        {
            if(!xpc_exception)
                XPCConvert::JSValToXPCException(ccx, js_exception, GetInterfaceName(),
                                                name, getter_AddRefs(xpc_exception));

            /* cleanup and set failed even if we can't build an exception */
            if(!xpc_exception)
            {
                ccx.GetThreadData()->SetException(nsnull); // XXX necessary?
                success = JS_FALSE;
            }
            JS_ClearPendingException(cx);
        }

        if(xpc_exception)
        {
            nsresult e_result;
            if(NS_SUCCEEDED(xpc_exception->GetResult(&e_result)))
            {
                if(IsReportableErrorCode(e_result))
                {

                    // Log the exception to the JS Console, so that users can do
                    // something with it.
                    nsCOMPtr<nsIConsoleService> consoleService
                        (do_GetService(XPC_CONSOLE_CONTRACTID));
                    if(nsnull != consoleService)
                    {
                        nsresult rv;
                        nsCOMPtr<nsIScriptError> scriptError;
                        nsCOMPtr<nsISupports> errorData;
                        rv = xpc_exception->GetData(getter_AddRefs(errorData));
                        if(NS_SUCCEEDED(rv))
                            scriptError = do_QueryInterface(errorData);

                        if(nsnull == scriptError)
                        {
                            // No luck getting one from the exception, so
                            // try to cook one up.
                            scriptError = do_CreateInstance(XPC_SCRIPT_ERROR_CONTRACTID);
                            if(nsnull != scriptError)
                            {
                                char* exn_string;
                                rv = xpc_exception->ToString(&exn_string);
                                if(NS_SUCCEEDED(rv))
                                {
                                    // use toString on the exception as the message
                                    nsAutoString newMessage;
                                    newMessage.AssignWithConversion(exn_string);
                                    nsMemory::Free((void *) exn_string);

                                    // try to get filename, lineno from the first
                                    // stack frame location.
                                    PRUnichar* sourceNameUni = nsnull;
                                    PRInt32 lineNumber = 0;
                                    nsXPIDLCString sourceName;

                                    nsCOMPtr<nsIStackFrame> location;
                                    xpc_exception->
                                        GetLocation(getter_AddRefs(location));
                                    if(location)
                                    {
                                        // Get line number w/o checking; 0 is ok.
                                        location->GetLineNumber(&lineNumber);

                                        // get a filename.
                                        rv = location->GetFilename(getter_Copies(sourceName));
                                    }

                                    rv = scriptError->Init(newMessage.get(),
                                                           NS_ConvertASCIItoUCS2(sourceName).get(),
                                                           nsnull,
                                                           lineNumber, 0, 0,
                                                           "XPConnect JavaScript");
                                    if(NS_FAILED(rv))
                                        scriptError = nsnull;
                                }
                            }
                        }
                        if(nsnull != scriptError)
                            consoleService->LogMessage(scriptError);
                    }
                }
                // Whether or not it passes the 'reportable' test, it might
                // still be an error and we have to do the right thing here...
                if(NS_FAILED(e_result))
                {
                    ccx.GetThreadData()->SetException(xpc_exception);
                    retval = e_result;
                }
            }
            success = JS_FALSE;
        }
        else
        {
            // see if JS code signaled failure result without throwing exception
            if(NS_FAILED(pending_result))
            {
                retval = pending_result;
                success = JS_FALSE;
            }
        }
        if(!success)
            goto done;
#endif

        ccx.GetThreadData()->SetException(nsnull); // XXX necessary?

        // convert out args and result
        // NOTE: this is the total number of native params, not just the args
        // Convert independent params only.
        // When we later convert the dependent params (if any) we will know that
        // the params upon which they depend will have already been converted -
        // regardless of ordering.

        outConversionFailedIndex = paramCount;
        foundDependentParam = JS_FALSE;
        if(JSVAL_IS_VOID(result) || XPCCOMConvert::JSToCOM(ccx, result, *pVarResult, err))
        {
            for(i = 0; i < paramCount; i++)
            {
                jsval val;
                if(JSVAL_IS_PRIMITIVE(stackbase[i+2]) ||
                        !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(stackbase[i+2]),
                            rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                            &val))
                {
                    outConversionFailedIndex = i;
                    break;
                }

            }
        }

        if(outConversionFailedIndex != paramCount)
        {
            // We didn't manage all the result conversions!
            // We have to cleanup any junk that *did* get converted.

            for(PRUint32 index = 0; index < outConversionFailedIndex; index++)
            {
                if((pDispParams->rgvarg[index].vt & VT_BYREF) != 0)
                {
                    XPCCOMObject::CleanupVariant(pDispParams->rgvarg[i]);
                }
            }
        }
        else
        {
            // set to whatever the JS code might have set as the result
            retval = pending_result;
        }

done:
        if(sp)
            js_FreeStack(cx, mark);

        if(cx)
        {
            JS_SetErrorReporter(cx, older);
            DoPostScriptEvaluated(cx, saved_exception);
        }
        // TODO: I think we may need to translate this error, 
        // for now we'll pass through
        return retval;
    }
    return S_OK;
}

