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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   John Bandhauer <jband@netscape.com>
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

/* JavaScript Object Ops for our Wrapped Native JS Objects. */


#include "xpcprivate.h"

/***************************************************************************/

static JSBool Throw(uintN errNum, JSContext* cx)
{
    XPCThrower::Throw(errNum, cx);
    return JS_FALSE;
}

#define THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper)                         \
    PR_BEGIN_MACRO                                                           \
    if(!wrapper)                                                             \
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);                   \
    if(!wrapper->IsValid())                                                  \
        return Throw(NS_ERROR_XPC_HAS_BEEN_SHUTDOWN, cx);                    \
    PR_END_MACRO

/***************************************************************************/

static JSBool
ToStringGuts(XPCCallContext& ccx)
{
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    JSContext* cx = ccx.GetJSContext();
    NS_ASSERTION(wrapper, "bad call");

    // XXX fix this.
    // our proto might have a known classname.
    // we might have a tearoff, then we'd know interface.

#ifdef DEBUG
    char* sz = JS_smprintf("[xpconnect wrapped %s @ 0x%p]",
                           "something", wrapper);
#else
    char* sz = JS_smprintf("[xpconnect wrapped %s]",
                           "something");
#endif

    if(!sz)
    {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    JSString* str = JS_NewString(cx, sz, strlen(sz));
    if(!str)
    {
        JS_smprintf_free(sz);
        // JS_ReportOutOfMemory already reported by failed JS_NewString
        return JS_FALSE;
    }
    
    ccx.SetRetVal(STRING_TO_JSVAL(str));
    return JS_TRUE;
}

/***************************************************************************/

JSBool JS_DLL_CALLBACK
XPC_WN_CannotModifyPropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}        

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_NoHelper_NewEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                             jsval *statep, jsid *idp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    switch(enum_op)
    {
        case JSENUMERATE_INIT:
            if(!wrapper->HasMutatedSet())
            {
                if(idp)
                    *idp = JSVAL_ZERO;
                *statep = JSVAL_NULL;
                return JS_TRUE; 
            }
            // XXX handle this
            // We need to build a list of the ids that are unique to our
            // mutated set. Oh joy!
            return JS_TRUE;

        case JSENUMERATE_NEXT:
            // XXX handle this
            return JS_TRUE;
            
        case JSENUMERATE_DESTROY:
            // XXX handle this
            return JS_TRUE;
        
        default:
            NS_ERROR("bad enum_op");
            return JS_FALSE;   
    }

    NS_NOTREACHED("huh?");
    return JS_FALSE;   
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_NoHelper_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                           JSObject **objp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    if(wrapper->HasMutatedSet())
        return JS_TRUE;        

    // XXX handle this
    // We can use XPCCallContext::SetJSID and if we discover that the id
    // needs to be in a tearoff then we can do a resolve on the tearoff and 
    // return.
    return JS_TRUE;        
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_NoHelper_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    if(type == JSTYPE_OBJECT)
    {
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    switch (type)
    {
        case JSTYPE_FUNCTION:
            return Throw(NS_ERROR_XPC_CANT_CONVERT_WN_TO_FUN, cx);
        case JSTYPE_NUMBER:
            *vp = JSVAL_ONE;
            return JS_TRUE;
        case JSTYPE_BOOLEAN:
            *vp = JSVAL_TRUE;
            return JS_TRUE;
        case JSTYPE_VOID:
        case JSTYPE_STRING:
        {
            ccx.SetJSID(ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_TO_STRING));
            ccx.SetArgsAndResultPtr(0, nsnull, vp);

            XPCNativeMember* member = ccx.GetMember();
            if(member && member->IsMethod())
            {
                if(!XPCWrappedNative::CallMethod(ccx))
                    return JS_FALSE;        
        
                if(JSVAL_IS_PRIMITIVE(*vp))
                    return JS_TRUE;
            }
        
            // else...
            return ToStringGuts(ccx);
        }
        default:
            NS_ERROR("bad type in conversion");
            return JS_FALSE;   
    }
    NS_NOTREACHED("huh?");
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(void)
XPC_WN_NoHelper_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNative* p = (XPCWrappedNative*) JS_GetPrivate(cx, obj);
    if(p)
        p->JSObjectFinalized(cx, obj); 
}

JSClass XPC_WN_NoHelper_JSClass = {
    "XPCWrappedNative_NoHelper",    // name;
    JSCLASS_HAS_PRIVATE |
    JSCLASS_NEW_ENUMERATE |
    JSCLASS_NEW_RESOLVE |
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    XPC_WN_CannotModifyPropertyStub,    // addProperty;
    XPC_WN_CannotModifyPropertyStub,    // delProperty;
    JS_PropertyStub,                    // getProperty;
    XPC_WN_CannotModifyPropertyStub,    // setProperty;

    (JSEnumerateOp) XPC_WN_NoHelper_NewEnumerate, // enumerate;
    (JSResolveOp) XPC_WN_NoHelper_NewResolve,     // resolve;
    XPC_WN_NoHelper_Convert,            // convert;
    XPC_WN_NoHelper_Finalize,           // finalize;

    /* Optionally non-null members start here. */
    nsnull,                         // getObjectOps;
    nsnull,                         // checkAccess;
    nsnull,                         // call;
    nsnull,                         // construct;
    nsnull,                         // xdrObject;
    nsnull,                         // hasInstance;
    nsnull,                         // mark;
    nsnull                          // spare;
};

/***************************************************************************/

// XXX This may be routed differently in the future...
JSBool JS_DLL_CALLBACK
XPC_WN_CallMethod(JSContext *cx, JSObject *obj,
                  uintN argc, jsval *argv, jsval *vp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj, 0, argc, argv, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    XPCCallableInfo* ci = 
        XPCNativeMember::GetCallableInfo(ccx, JSVAL_TO_OBJECT(argv[-2]));
    if(!ci)
        return Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, cx);
    ccx.SetCallableInfo(ci, JS_FALSE);
    return XPCWrappedNative::CallMethod(ccx);
}

JSBool JS_DLL_CALLBACK
XPC_WN_GetterSetter(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *vp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    XPCCallableInfo* ci = 
        XPCNativeMember::GetCallableInfo(ccx, JSVAL_TO_OBJECT(argv[-2]));
    if(!ci)
        return Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, cx);
    
    ccx.SetArgsAndResultPtr(argc, argv, vp);
    if(JS_IsAssigning(cx))
    {
        ccx.SetCallableInfo(ci, JS_TRUE);
        return XPCWrappedNative::SetAttribute(ccx);
    }
    // else...

    ccx.SetCallableInfo(ci, JS_FALSE);
    return XPCWrappedNative::GetAttribute(ccx);
}

/***************************************************************************/

static JSBool
XPC_WN_Proto_ReflectMember(XPCCallContext& ccx, XPCWrappedNativeProto* self,
                           XPCNativeMember* member,
                           XPCNativeInterface* interface,
                           JSContext *cx, JSObject *obj, jsid id)
{
    uintN enumFlag = self->GetScriptableInfo() && 
                     self->GetScriptableInfo()->DontEnumStaticProps() ?
                        0 : JSPROP_ENUMERATE;

    if(member->IsConstant())
    {
        jsval val;
        return member->GetValue(ccx, interface, &val) &&
               OBJ_DEFINE_PROPERTY(cx, obj, id, val, nsnull, nsnull,
                                   enumFlag, nsnull);
    }


    jsval funval;
    if(!member->GetValue(ccx, interface, &funval))
        return JS_FALSE;
    
    JSObject* funobj = 
        JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(funval),
                               self->GetScope()->GetGlobalJSObject());
    if(!funobj)
        return JS_FALSE;

    if(member->IsMethod())
    {
        return OBJ_DEFINE_PROPERTY(cx, obj, id, OBJECT_TO_JSVAL(funobj), 
                                   nsnull, nsnull, enumFlag, nsnull);
    }
    
    // else...

    NS_ASSERTION(member->IsAttribute(), "way broken!");


    // XXX This nonsense is here because the JS engine will call my resolve
    // if I try to define a property with a getter or setter. This is a
    // problem since I'm already servicing resolve and we risk infinite
    // recursion.
    if(ccx.GetHackyResolveBugID() == id)
        return JS_TRUE;
    jsid old = ccx.SetHackyResolveBugID(id);
    NS_ASSERTION(!old, "bad nest");

    JSBool retval = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                                        (JSPropertyOp) funobj, 
                                        (JSPropertyOp) funobj, 
                                        enumFlag | 
                                        JSPROP_GETTER |
                                        (member->IsWritableAttribute() ?
                                         JSPROP_SETTER : 0),
                                        nsnull);
    old = ccx.SetHackyResolveBugID(0);
    NS_ASSERTION(old == id, "bad nest");
    return retval;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Proto_Enumerate(JSContext *cx, JSObject *obj)
{
    XPCWrappedNativeProto* self = (XPCWrappedNativeProto*) JS_GetPrivate(cx, obj);
    if(!self)
        return JS_FALSE;
    
    XPCNativeSet* set = self->GetSet();
    if(!set)
        return JS_FALSE;

    XPCCallContext ccx(JS_CALLER, cx);
    if(!ccx.IsValid())
        return JS_FALSE;

    PRUint16 interface_count = set->Count();
    XPCNativeInterface** interfaceArray = set->GetInterfaceArray();
    for(PRUint16 i = 0; i < interface_count; i++)
    {
        XPCNativeInterface* interface = interfaceArray[i];
        PRUint16 member_count = interface->GetMemberCount();
        
        for(PRUint16 k = 0; k < member_count; k++)
        {
            XPCNativeMember* member = interface->GetMemberAt(k);
            JSProperty* prop;
            JSObject* obj2;

            // The Lookup will force a Resolve and eager Define of the property
            
            if(!OBJ_LOOKUP_PROPERTY(cx, obj, member->GetID(), &obj2, &prop))
                return JS_FALSE;
        }
    }
    return JS_TRUE;        
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Proto_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    XPCWrappedNativeProto* self = (XPCWrappedNativeProto*) JS_GetPrivate(cx, obj);
    if(!self)
        return JS_FALSE;

    jsid id;
    if(!JS_ValueToId(cx, idval, &id))
        return JS_FALSE;

    XPCNativeMember* member;
    XPCNativeInterface* interface;
    
    if(!self->GetSet()->FindMember(id, &member, &interface))
    {
        HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, self->GetSet(), id);
        return JS_TRUE;
    }

    XPCCallContext ccx(JS_CALLER, cx);
    if(!ccx.IsValid())
        return JS_FALSE;
    
    // XXX this doen not really need to be factored out!
    return XPC_WN_Proto_ReflectMember(ccx, self, member, interface, cx, obj, id);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Proto_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    // XXX ?
    return JS_TRUE;        
}

JS_STATIC_DLL_CALLBACK(void)
XPC_WN_Proto_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNativeProto* p = (XPCWrappedNativeProto*) JS_GetPrivate(cx, obj);
    if(p)
        p->JSProtoObjectFinalized(cx, obj); 
}

JSClass XPC_WN_Proto_JSClass = {
    "XPCWrappedNative_Proto",       // name;
    JSCLASS_HAS_PRIVATE | 
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    JS_PropertyStub,                // addProperty;
    JS_PropertyStub,                // delProperty;
    JS_PropertyStub,                // getProperty;
    JS_PropertyStub,                // setProperty;
    XPC_WN_Proto_Enumerate,         // enumerate;
    XPC_WN_Proto_Resolve,           // resolve;
    XPC_WN_Proto_Convert,           // convert;
    XPC_WN_Proto_Finalize,          // finalize;

    /* Optionally non-null members start here. */
    nsnull,                         // getObjectOps;
    nsnull,                         // checkAccess;
    nsnull,                         // call;
    nsnull,                         // construct;
    nsnull,                         // xdrObject;
    nsnull,                         // hasInstance;
    nsnull,                         // mark;
    nsnull                          // spare;
};

/***************************************************************************/
// XXX fix me

JSClass XPC_WN_WithHelper_JSClass = {
    "WrappedNative_WithHelper",    // name;
    JSCLASS_HAS_PRIVATE | 
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    JS_PropertyStub,                // addProperty;
    JS_PropertyStub,                // delProperty;
    JS_PropertyStub,                // getProperty;
    JS_PropertyStub,                // setProperty;
    JS_EnumerateStub,               // enumerate;
    JS_ResolveStub,                 // resolve;
    JS_ConvertStub,                 // convert;
    JS_FinalizeStub,                // finalize;

    /* Optionally non-null members start here. */
    nsnull,                         // getObjectOps;
    nsnull,                         // checkAccess;
    nsnull,                         // call;
    nsnull,                         // construct;
    nsnull,                         // xdrObject;
    nsnull,                         // hasInstance;
    nsnull,                         // mark;
    nsnull                          // spare;
};

JSClass XPC_WN_WithHelperNoCall_JSClass = {
    "WrappedNative_WithHelperNoCall",    // name;
    JSCLASS_HAS_PRIVATE | 
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    JS_PropertyStub,                // addProperty;
    JS_PropertyStub,                // delProperty;
    JS_PropertyStub,                // getProperty;
    JS_PropertyStub,                // setProperty;
    JS_EnumerateStub,               // enumerate;
    JS_ResolveStub,                 // resolve;
    JS_ConvertStub,                 // convert;
    JS_FinalizeStub,                // finalize;

    /* Optionally non-null members start here. */
    nsnull,                         // getObjectOps;
    nsnull,                         // checkAccess;
    nsnull,                         // call;
    nsnull,                         // construct;
    nsnull,                         // xdrObject;
    nsnull,                         // hasInstance;
    nsnull,                         // mark;
    nsnull                          // spare;
};


JSClass XPC_WN_Tearoff_JSClass = {
    "WrappedNative_TearOff",    // name;
    JSCLASS_HAS_PRIVATE | 
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    JS_PropertyStub,                // addProperty;
    JS_PropertyStub,                // delProperty;
    JS_PropertyStub,                // getProperty;
    JS_PropertyStub,                // setProperty;
    JS_EnumerateStub,               // enumerate;
    JS_ResolveStub,                 // resolve;
    JS_ConvertStub,                 // convert;
    JS_FinalizeStub,                // finalize;

    /* Optionally non-null members start here. */
    nsnull,                         // getObjectOps;
    nsnull,                         // checkAccess;
    nsnull,                         // call;
    nsnull,                         // construct;
    nsnull,                         // xdrObject;
    nsnull,                         // hasInstance;
    nsnull,                         // mark;
    nsnull                          // spare;
};

/***************************************************************************/

JSBool xpc_InitWrappedNativeJSOps()
{
    // XXX fix this...
    return JS_TRUE;
}


#if 0


extern "C" JS_IMPORT_DATA(JSObjectOps) js_ObjectOps;

// XXX fix me
JSClass WrappedNative_class;
JSClass WrappedNativeWithCall_class;


static JSObject*
GetDoubleWrappedJSObject(XPCWrappedNativeCallContext& ccx)
{
    JSObject* obj = nsnull;
    nsCOMPtr<nsIXPConnectWrappedJS>
        underware = do_QueryInterface(ccx.GetIdentityObject());
    if(underware)
    {
        JSObject* mainObj = nsnull;
        if(NS_SUCCEEDED(underware->GetJSObject(&mainObj)) && mainObj)
        {
            jsval val;
            if(OBJ_GET_PROPERTY(ccx.GetJSContext(), mainObj, ccx.GetJSID(), 
                                &val) && !JSVAL_IS_PRIMITIVE(val))
            {
                obj = JSVAL_TO_OBJECT(val);
            }
        }
    }
    return obj;
}

/***************************************************************************/


// XXX This may be routed differently in the future...
JSBool JS_DLL_CALLBACK
WrappedNative_CallMethod(JSContext *cx, JSObject *obj,
                         uintN argc, jsval *argv, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, 0, argc, argv, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
        return JS_FALSE;
    
    // XXX fix me!
    return JS_FALSE;
}

/***************************************************************************/

static JSBool
ToStringGuts(XPCWrappedNativeCallContext& ccx)
{
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    JSContext* cx = ccx.GetJSContext();
    NS_ASSERTION(wrapper, "bad call");

    // XXX fix this.
    // our proto might have a known classname.
    // we might have a tearoff, then we'd know interface.

#ifdef DEBUG
    char* sz = JS_smprintf("[xpconnect wrapped %s @ 0x%p]",
                           "something", wrapper);
#else
    char* sz = JS_smprintf("[xpconnect wrapped %s]",
                           "something");
#endif

    if(!sz)
    {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    JSString* str = JS_NewString(cx, sz, strlen(sz));
    if(!str)
    {
        JS_smprintf_free(sz);
        // JS_ReportOutOfMemory already reported by failed JS_NewString
        return JS_FALSE;
    }
    
    ccx.SetRetVal(STRING_TO_JSVAL(str));
    return JS_TRUE;
}

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    if(type == JSTYPE_OBJECT)
    {
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }

    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();

    THROW_AND_RETURN_IF_BAD_WRAPPER(wrapper, cx);

    switch (type)
    {
    case JSTYPE_FUNCTION:
    {
        XPCNativeScriptableInfo* si = ccx.GetScriptableInfo();
        if(!si || si->HideCallAndConstruct())
            return Throw(NS_ERROR_XPC_CANT_CONVERT_WN_TO_FUN, cx);
        
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }
    case JSTYPE_VOID:
    case JSTYPE_STRING:
    {
        ccx.SetJSID(ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_TO_STRING));
        ccx.SetArgsAndResultPtr(0, nsnull, vp);

        XPCNativeMember* member = ccx.GetMember();
        if(member && member->IsMethod())
        {
            if(!XPCWrappedNative::CallMethod(ccx))
                return JS_FALSE;        
        
            if(JSVAL_IS_PRIMITIVE(*vp))
                return JS_TRUE;
        }
        
        // else...

        return ToStringGuts(ccx);
    }

    case JSTYPE_NUMBER:
        *vp = JSVAL_ONE;
        return JS_TRUE;

    case JSTYPE_BOOLEAN:
        *vp = JSVAL_TRUE;
        return JS_TRUE;

    default:
        break;
    }
    NS_ASSERTION(0,"bad type in conversion");
    return JS_FALSE;
}        


JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DefaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();

    THROW_AND_RETURN_IF_BAD_WRAPPER(wrapper, cx);

    XPCNativeScriptableInfo* si = ccx.GetScriptableInfo();
    if(si)
    {
        JSBool retval;
        nsresult rv = si->GetScriptable()->
            DefaultValue(cx, obj, type, vp,
                         ccx.GetTearOffForScriptable(),
                         ccx.GetArbitraryScriptable(), &retval);
        if(NS_SUCCEEDED(rv))
            return retval;
    }
     
    return WrappedNative_Convert(cx, obj, type, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ToString(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, 0, argc, argv, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    
    THROW_AND_RETURN_IF_BAD_WRAPPER(wrapper);
    
    return ToStringGuts(ccx);
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ToSource(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    static const char empty[] = "{}";
    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, empty, sizeof(empty)-1));
    return JS_TRUE;
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id, 0, 0, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();

    if(!wrapper || !wrapper->IsValid())
    {
        // silently fail when looking for constructor property
        XPCJSRuntime* rt = ccx.GetRuntime();
        if(rt && id == rt->GetStringID(XPCJSRuntime::IDX_CONSTRUCTOR))
        {
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }

        // otherwise, do our standard failure dance...
        THROW_AND_RETURN_IF_BAD_WRAPPER(wrapper);
    }

    XPCNativeScriptableInfo* si = ccx.GetScriptableInfo();
    
    if(si && si->CallScriptableFirst())
    {
        JSBool retval;
        nsresult rv = si->GetScriptable()->
            GetProperty(cx, obj, id, vp,
                        ccx.GetTearOffForScriptable(),
                        ccx.GetArbitraryScriptable(), &retval);
        if(XPC_HANDLED(rv))
            return NS_SUCCEEDED(rv) ? retval : JS_FALSE;
    }
     
    
    XPCNativeMember* member = ccx.GetMember();
    if(member)
    {
        if(member->IsConstant())
            return XPCWrappedNative::GetConstant(ccx);
        if(member->IsAttribute())
            return XPCWrappedNative::GetAttribute(ccx);

        // else we are getting a method
        
        // XXX fix this...    
    
        return JS_FALSE;

    }

    XPCJSRuntime* rt = ccx.GetRuntime();

    if(id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT))
    {
        JSObject* realObject = GetDoubleWrappedJSObject(ccx);
        if(realObject)
        {

            // It is a double wrapped object. Figure out if the caller
            // is allowed to see it.

            XPCContext* xpcc = ccx.GetXPCContext();
            nsIXPCSecurityManager* sm;
    
            sm = xpcc->GetAppropriateSecurityManager(
                            nsIXPCSecurityManager::HOOK_GET_PROPERTY);
            if(sm)
            {
                nsCOMPtr<nsIInterfaceInfoManager> iimgr =
                        dont_AddRef(nsXPConnect::GetInterfaceInfoManager());
                if(iimgr)
                {
                    const nsIID& iid = NS_GET_IID(nsIXPCWrappedJSObjectGetter);
                    nsCOMPtr<nsIInterfaceInfo> info;
                    if(NS_SUCCEEDED(iimgr->GetInfoForIID(&iid, 
                                                getter_AddRefs(info))))
                    {
                        if(NS_OK != sm->CanGetProperty(cx, iid,
                                                       ccx.GetIdentityObject(),
                                                       info, 3, id))
                        {
                            // The SecurityManager should have set an exception.
                            return JS_FALSE;
                        }
                    }
                }
            }
            *vp = OBJECT_TO_JSVAL(realObject);
            return JS_TRUE;
        }
    }

    HANDLE_POSSIBLE_NAME_CASE_ERROR(ccx);


    // XXX We really want to be binding these to the proto!

    // deal with possible lookup of toString or toSource
    {
        JSNative call;
        const char* name;

        if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING))
        {
            call = WrappedNative_ToString;
            name = rt->GetStringName(XPCJSRuntime::IDX_TO_STRING);    
        }
        else if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE))
        {
            call = WrappedNative_ToSource;
            name = rt->GetStringName(XPCJSRuntime::IDX_TO_SOURCE);    
        }
        else
            call = nsnull;

        if(call)
        {
            JSFunction* fun = JS_NewFunction(cx, call, 0, 
                                             JSFUN_BOUND_METHOD, 
                                             obj, name);
            if(fun)
            {
                *vp = OBJECT_TO_JSVAL(JS_GetFunctionObject(fun));
                return JS_TRUE;
            }
            else
            {
                JS_ReportOutOfMemory(cx);
                return JS_FALSE;
            }
        }
    }

    if(si && !si->CallScriptableFirst())
    {
        JSBool retval;
        nsresult rv = si->GetScriptable()->
            GetProperty(cx, obj, id, vp,
                        ccx.GetTearOffForScriptable(),
                        ccx.GetArbitraryScriptable(), &retval);
        if(XPC_HANDLED(rv))
            return NS_SUCCEEDED(rv) ? retval : JS_FALSE;
    }

    // XXX up the OperandJSObject's chain, or the wrapper's JSObject's chain?

    // Check up the prototype chain to match JavaScript lookup behavior
    JSObject* proto = JS_GetPrototype(cx, obj); 
    if(proto)
        return OBJ_GET_PROPERTY(cx, proto, id, vp);

    // XXX silently fail when property not found or call fails?
    *vp = JSVAL_VOID;
    return JS_TRUE;
}

#if 0

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id, 0, 0, vp);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
        return JS_FALSE;
    return wrapper->Handle_GetProperty(&ccx);
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id, 1, vp, nsnull);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
        return JS_FALSE;
    return wrapper->Handle_SetProperty(&ccx);
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_LookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp
#if defined JS_THREADSAFE && defined DEBUG
                            , const char *file, uintN line
#endif
                            )
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        // XXX is silent failure good here?
        *objp = nsnull;
        *propp = nsnull;
        return JS_TRUE;
    }
    return wrapper->Handle_LookupProperty(&ccx, objp, propp);
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DefineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        return Throw(NS_ERROR_XPC_CANT_DEFINE_PROP_ON_WN, cx);
    }
    return wrapper->Handle_DefineProperty(&ccx, value, getter, setter, 
                                          attrs, propp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_GetAttributes(JSContext *cx, JSObject *obj, jsid id,
                            JSProperty *prop, uintN *attrsp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        // silent failure.
        *attrsp = 0;
        return JS_TRUE;
    }
    return wrapper->Handle_GetAttributes(&ccx, prop, attrsp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_SetAttributes(JSContext *cx, JSObject *obj, jsid id,
                            JSProperty *prop, uintN *attrsp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        // silent failure.
        return JS_TRUE;
    }
    return wrapper->Handle_SetAttributes(&ccx, prop, attrsp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        // silent failure.
        *vp = JSVAL_FALSE;
        return JS_TRUE;
    }
    return wrapper->Handle_DeleteProperty(&ccx, vp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                        jsval *statep, jsid *idp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        *statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSVAL(0);
        return JS_TRUE;
    }
    return wrapper->Handle_Enumerate(&ccx, enum_op, statep, idp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_CheckAccess(JSContext *cx, JSObject *obj, jsid id,
                          JSAccessMode mode, jsval *vp, uintN *attrsp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, id);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        switch(mode)
        {
        case JSACC_WATCH:
            return ThrowException(NS_ERROR_XPC_CANT_WATCH_WN_STATIC, cx);

        case JSACC_IMPORT:
            return ThrowException(NS_ERROR_XPC_CANT_EXPORT_WN_STATIC, cx);

        default:
            return JS_TRUE;
        }
    }
    return wrapper->Handle_CheckAccess(&ccx, mode, vp, attrsp);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Call(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval)

{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, 0, argc, argv, rval);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        return ThrowException(NS_ERROR_XPC_CANT_CALL_WO_SCRIPTABLE, cx);
    }
    return wrapper->Handle_Call(&ccx);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Construct(JSContext *cx, JSObject *obj,
                        uintN argc, jsval *argv, jsval *rval)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, 0, argc, argv, rval);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        return ThrowException(NS_ERROR_XPC_CANT_CTOR_WO_SCRIPTABLE, cx);
    }
    return wrapper->Handle_Construct(&ccx);
}        

// this is the final resting place of non-handled hasInstance calls
JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ClassHasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    //XXX our default policy is to just say no. Is this right?
    *bp = JS_FALSE;
    return JS_TRUE;
}

// this is in the ObjectOps and is called first
JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        return WrappedNative_ClassHasInstance(cx, obj, v, bp);
    }
    return wrapper->Handle_HasInstance(&ccx, v, bp);
}        

JS_STATIC_DLL_CALLBACK(void)
WrappedNative_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(wrapper)
        wrapper->Handle_Finalize(&ccx);
}        

JS_STATIC_DLL_CALLBACK(void)
WrappedNative_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
    {
        static JSPropertyRefOp drop = js_ObjectOps.dropProperty;
        if(drop)
            drop(cx, obj, prop);
        
    }
    wrapper->Handle_DropProperty(&ccx, prop);
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    XPCWrappedNativeCallContext ccx(cx, JS_CALLER, obj, idval);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    if(!wrapper)
        return JS_TRUE;        
    return wrapper->Handle_Resolve(&ccx);
}        

/***************************************************************************/

/*
* We have two classes - one with and one without call and construct. We use
* the one without for any object without an nsIXPCScriptable so that the
* engine will show a typeof 'object' instead of 'function'
*/

static JSObjectOps WrappedNative_ops = {
    /* Mandatory non-null function pointer members. */
    nsnull,                      /* filled in at runtime! - newObjectMap */
    nsnull,                      /* filled in at runtime! - destroyObjectMap */
    WrappedNative_LookupProperty,
    WrappedNative_DefineProperty,
    WrappedNative_GetProperty,
    WrappedNative_SetProperty,
    WrappedNative_GetAttributes,
    WrappedNative_SetAttributes,
    WrappedNative_DeleteProperty,
    WrappedNative_DefaultValue,
    WrappedNative_Enumerate,
    WrappedNative_CheckAccess,

    /* Optionally non-null members start here. */
    nsnull,                     /* thisObject   */
    WrappedNative_DropProperty, /* dropProperty */
    nsnull,                     /* call         */
    nsnull,                     /* construct    */
    nsnull,                     /* xdrObject    */
    WrappedNative_HasInstance,  /* hasInstance  */
    nsnull,                     /* filled in at runtime! - setProto */
    nsnull,                     /* filled in at runtime! - setParent */
    nsnull,                     /* filled in at runtime! - mark */
    nsnull,                     /* filled in at runtime! - clear */
    0,0                         /* spare */
};

static JSObjectOps WrappedNativeWithCall_ops = {
    /* Mandatory non-null function pointer members. */
    nsnull,                     /* filled in at runtime! - newObjectMap */
    nsnull,                     /* filled in at runtime! - destroyObjectMap */
    WrappedNative_LookupProperty,
    WrappedNative_DefineProperty,
    WrappedNative_GetProperty,
    WrappedNative_SetProperty,
    WrappedNative_GetAttributes,
    WrappedNative_SetAttributes,
    WrappedNative_DeleteProperty,
    WrappedNative_DefaultValue,
    WrappedNative_Enumerate,
    WrappedNative_CheckAccess,

    /* Optionally non-null members start here. */
    nsnull,                     /* thisObject   */
    WrappedNative_DropProperty, /* dropProperty */
    WrappedNative_Call,         /* call         */
    WrappedNative_Construct,    /* construct    */
    nsnull,                     /* xdrObject    */
    WrappedNative_HasInstance,  /* hasInstance  */
    nsnull,                     /* filled in at runtime! - setProto */
    nsnull,                     /* filled in at runtime! - setParent */
    nsnull,                     /* filled in at runtime! - mark */
    nsnull,                     /* filled in at runtime! - clear */
    0,0                         /* spare */
};

JS_STATIC_DLL_CALLBACK(JSObjectOps *)
WrappedNative_getObjectOps(JSContext *cx, JSClass *clazz)
{
    return &WrappedNative_ops;
}

JS_STATIC_DLL_CALLBACK(JSObjectOps *)
WrappedNative_getWithCallObjectOps(JSContext *cx, JSClass *clazz)
{
    return &WrappedNativeWithCall_ops;
}


JSClass WrappedNative_class = {
    "XPCWrappedNative", 
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, 
    WrappedNative_Resolve,
    WrappedNative_Convert,
    WrappedNative_Finalize,
    /* Optionally non-null members start here. */
    WrappedNative_getObjectOps,         /* getObjectOps */
    nsnull,                             /* checkAccess  */
    nsnull,                             /* call         */
    nsnull,                             /* construct    */
    nsnull,                             /* xdrObject    */
    WrappedNative_ClassHasInstance      /* hasInstance  */
};

JSClass WrappedNativeWithCall_class = {
    "XPCWrappedNativeWithCall", 
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, 
    WrappedNative_Resolve,
    WrappedNative_Convert,
    WrappedNative_Finalize,
    /* Optionally non-null members start here. */
    WrappedNative_getWithCallObjectOps, /* getObjectOps */
    nsnull,                             /* checkAccess  */
    nsnull,                             /* call         */
    nsnull,                             /* construct    */
    nsnull,                             /* xdrObject    */
    WrappedNative_ClassHasInstance      /* hasInstance  */
};

JSBool xpc_InitWrappedNativeJSOps()
{
    if(!WrappedNative_ops.newObjectMap)
    {
        WrappedNative_ops.newObjectMap     = js_ObjectOps.newObjectMap;
        WrappedNative_ops.destroyObjectMap = js_ObjectOps.destroyObjectMap;
        WrappedNative_ops.setProto         = js_ObjectOps.setProto;
        WrappedNative_ops.setParent        = js_ObjectOps.setParent;
        WrappedNative_ops.mark             = js_ObjectOps.mark;
        WrappedNative_ops.clear            = js_ObjectOps.clear;

        WrappedNativeWithCall_ops.newObjectMap     = js_ObjectOps.newObjectMap;
        WrappedNativeWithCall_ops.destroyObjectMap = js_ObjectOps.destroyObjectMap;
        WrappedNativeWithCall_ops.setProto         = js_ObjectOps.setProto;
        WrappedNativeWithCall_ops.setParent        = js_ObjectOps.setParent;
        WrappedNativeWithCall_ops.mark             = js_ObjectOps.mark;
        WrappedNativeWithCall_ops.clear            = js_ObjectOps.clear;
    }
    return JS_TRUE;
}

#else
JSBool xpc_InitWrappedNativeJSOps()
{
    // XXX fix this...
    return JS_TRUE;
}

#endif

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
#if 0
// REMOVED STUFF FOLLOWS...

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

/***************************************************************************/

AutoPushJSContext::AutoPushJSContext(JSContext *cx, nsXPConnect* xpc /*= nsnull*/)
{
    NS_ASSERTION(cx, "pushing null cx");
#ifdef DEBUG
    mDebugCX = cx;
#endif
    mContextStack = nsXPConnect::GetContextStack(xpc);
    if(mContextStack)
    {
        JSContext* current;

        if(NS_FAILED(mContextStack->Peek(&current)) ||
           current == cx ||
           NS_FAILED(mContextStack->Push(cx)))
        {
            NS_RELEASE(mContextStack);
        }
    }
}

AutoPushJSContext::~AutoPushJSContext()
{
    if(mContextStack)
    {
#ifdef DEBUG
        JSContext* cx;
        nsresult rv = mContextStack->Pop(&cx);
        NS_ASSERTION(NS_SUCCEEDED(rv) && cx == mDebugCX, "unbalanced stack usage");
#else
        mContextStack->Pop(nsnull);
#endif
        NS_RELEASE(mContextStack);
    }
}

/***************************************************************************/

static void ThrowException(uintN errNum, JSContext* cx,
                           nsXPCWrappedNativeClass* clazz = nsnull,
                           const XPCNativeMemberDescriptor* desc = nsnull)
    {nsXPConnect::GetJSThrower()->ThrowException(errNum, cx, clazz, desc);}

// safer!
#define GET_WRAPPER nsXPCWrappedNativeClass::GetWrappedNativeOfJSObject
// #define GET_WRAPPER (nsXPCWrappedNative*) JS_GetPrivate

/***************************************************************************/

static JSObject*
GetDoubleWrappedJSObject(JSContext* cx, nsXPCWrappedNative* wrapper, jsid id)
{
    JSObject* obj = nsnull;
    
    if(wrapper && wrapper->GetNative())
    {
        nsCOMPtr<nsIXPConnectWrappedJS> 
            underware = do_QueryInterface(wrapper->GetNative());               
        if(underware)
        {
            JSObject* mainObj = nsnull;
            if(NS_SUCCEEDED(underware->GetJSObject(&mainObj)) && mainObj)
            {
                jsval val;
                if(OBJ_GET_PROPERTY(cx, mainObj, id, &val) &&
                   !JSVAL_IS_PRIMITIVE(val))
                    obj = JSVAL_TO_OBJECT(val);
            }
        }
    }
    return obj;
}

/***************************************************************************/

static JSBool
ToStringGuts(JSContext *cx, nsXPCWrappedNative* wrapper, jsval *vp)
{
#ifdef DEBUG
    char* sz = JS_smprintf("[xpconnect wrapped %s @ 0x%p]",
                           wrapper->GetClass()->GetInterfaceName(), wrapper);
#else
    char* sz = JS_smprintf("[xpconnect wrapped %s]",
                           wrapper->GetClass()->GetInterfaceName());
#endif

    if(!sz)
    {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    JSString* str = JS_NewString(cx, sz, strlen(sz));
    if(!str)
    {
        JS_smprintf_free(sz);
        // JS_ReportOutOfMemory already reported by failed JS_NewString
        return JS_FALSE;
    }
    
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    

    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);

    if (!wrapper || !wrapper->IsValid()) {
        if (type == JSTYPE_OBJECT) {
            *vp = OBJECT_TO_JSVAL(obj);
            return JS_TRUE;
        }
        return ThrowException(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);
    }

    switch (type) {
    case JSTYPE_OBJECT:
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;

    case JSTYPE_FUNCTION:
        if(wrapper->GetDynamicScriptable())
        {
            *vp = OBJECT_TO_JSVAL(obj);
            return JS_TRUE;
        }
        return ThrowException(NS_ERROR_XPC_CANT_CONVERT_WN_TO_FUN, cx);

    case JSTYPE_VOID:
    case JSTYPE_STRING:
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");

        XPCJSRuntime* rt;
        const XPCNativeMemberDescriptor* desc;

        if(nsnull != (rt = clazz->GetRuntime()) &&
           nsnull != (desc = clazz->LookupMemberByID(
                           rt->GetStringID(XPCJSRuntime::IDX_TO_STRING))) &&
           desc->IsMethod())
        {
            if(!clazz->CallWrappedMethod(cx, wrapper, desc,
                                         nsXPCWrappedNativeClass::CALL_METHOD,
                                         0, nsnull, vp))
                return JS_FALSE;
            if(JSVAL_IS_PRIMITIVE(*vp))
                return JS_TRUE;
        }

        // else...

        return ToStringGuts(cx, wrapper, vp);
    }

    case JSTYPE_NUMBER:
        *vp = JSVAL_ONE;
        return JS_TRUE;

    case JSTYPE_BOOLEAN:
        *vp = JSVAL_TRUE;
        return JS_TRUE;

    default:
        NS_ASSERTION(0,"bad type in conversion");
        return JS_FALSE;
    }
}


JSBool JS_DLL_CALLBACK
WrappedNative_CallMethod(JSContext *cx, JSObject *obj,
                         uintN argc, jsval *argv, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    JSFunction *fun;
    jsid id;
    jsval idval;

    nsXPCWrappedNative* wrapper;
    wrapper = GET_WRAPPER(cx, obj);
    if(!wrapper)
        return JS_FALSE;

    nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
    NS_ASSERTION(clazz,"wrapper without class");

    NS_ASSERTION(JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION, "bad function");
    fun = (JSFunction*) JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[-2]));
    idval = STRING_TO_JSVAL(JS_InternString(cx, JS_GetFunctionName(fun)));
    JS_ValueToId(cx, idval, &id);

    const XPCNativeMemberDescriptor* desc = clazz->LookupMemberByID(id);
    if(!desc || !desc->IsMethod())
    {
        HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, clazz, id);
        return JS_FALSE;
    }

    return clazz->CallWrappedMethod(cx, wrapper, desc,
                                    nsXPCWrappedNativeClass::CALL_METHOD,
                                    argc, argv, vp);
}


JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ToString(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx,obj);
    if(wrapper && wrapper->IsValid())
        return ToStringGuts(cx, wrapper, vp);
    return JS_FALSE;
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ToSource(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    
    static const char empty[] = "{}";
    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, empty, sizeof(empty)-1));
    return JS_TRUE;
}       

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper;

    wrapper = GET_WRAPPER(cx, obj);
    if(!wrapper || !wrapper->IsValid())
    {
        XPCJSRuntime* rt = nsXPConnect::GetRuntime();
        if(rt && id == rt->GetStringID(XPCJSRuntime::IDX_CONSTRUCTOR))
        {
            // silently fail when looking for constructor property
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }
        return JS_FALSE;
    }

    nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
    NS_ASSERTION(clazz,"wrapper without class");

    const XPCNativeMemberDescriptor* desc = clazz->LookupMemberByID(id);
    if(desc)
    {
        if(desc->IsConstant())
        {
            if(!clazz->GetConstantAsJSVal(cx, wrapper, desc, vp))
                *vp = JSVAL_NULL; //XXX silent failure?
            return JS_TRUE;
        }
        else if(desc->IsMethod())
        {
            JSObject* funobj = clazz->NewFunObj(cx, obj, desc);
            if (!funobj)
                return JS_FALSE;
            *vp = OBJECT_TO_JSVAL(funobj);
            return JS_TRUE;
        }
        else    // attribute
            return clazz->GetAttributeAsJSVal(cx, wrapper, desc, vp);
    }
    else if(wrapper->GetNative() &&
            id == clazz->GetRuntime()->
                            GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT))
    {
        JSObject* realObject = GetDoubleWrappedJSObject(cx, wrapper, id);
        if(realObject)
        {
            // It is a double wrapped object. Figure out if the caller
            // is allowed to see it.

            XPCContext* xpcc = nsXPConnect::GetContext(cx);
            if(xpcc)
            {
                nsIXPCSecurityManager* sm;
    
                sm = xpcc->GetAppropriateSecurityManager(
                                nsIXPCSecurityManager::HOOK_GET_PROPERTY);
                if(sm)
                {
                    nsCOMPtr<nsIInterfaceInfoManager> iimgr =
                            dont_AddRef(nsXPConnect::GetInterfaceInfoManager());
                    if(iimgr)
                    {
                        const nsIID& iid = 
                            NS_GET_IID(nsIXPCWrappedJSObjectGetter);
                        nsCOMPtr<nsIInterfaceInfo> info;
                        if(NS_SUCCEEDED(iimgr->GetInfoForIID(&iid, 
                                                    getter_AddRefs(info))))
                        {
                            if(NS_OK != sm->CanGetProperty(cx, iid,
                                                    wrapper->GetNative(),
                                                    info, 3, id))
                            {
                                // The SecurityManager should have set an exception.
                                return JS_FALSE;
                            }
                        }
                    }
                }
                *vp = OBJECT_TO_JSVAL(realObject);
                return JS_TRUE;
            }
        }
    }
    else
    {
        HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, clazz, id);

        // deal with possible lookup of toString or toSource
        XPCJSRuntime* rt = nsXPConnect::GetRuntime();
        if(rt)
        {
            JSNative call;
            const char* name;

            if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING))
            {
                call = WrappedNative_ToString;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_STRING);    
            }
            else if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE))
            {
                call = WrappedNative_ToSource;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_SOURCE);    
            }
            else
                call = nsnull;

            if(call)
            {
                JSFunction* fun = JS_NewFunction(cx, call, 0, 
                                                 JSFUN_BOUND_METHOD, 
                                                 obj, name);
                if(fun)
                {
                    *vp = OBJECT_TO_JSVAL(JS_GetFunctionObject(fun));
                    return JS_TRUE;
                }
                else
                {
                    JS_ReportOutOfMemory(cx);
                    return JS_FALSE;
                }
            }
        }

        nsIXPCScriptable* ds;
        nsIXPCScriptable* as;
        if(nsnull != (ds = wrapper->GetDynamicScriptable()) &&
           nsnull != (as = wrapper->GetArbitraryScriptable()))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->GetProperty(cx, obj, id, vp,
                                            wrapper, as, &retval)))
                return retval;
        }

        // Check up the prototype chain to match JavaScript lookup behavior
        JSObject* proto = JS_GetPrototype(cx, obj); 
        if(proto)
            return OBJ_GET_PROPERTY(cx, proto, id, vp);
    }
    
    // XXX silently fail when property not found or call fails?
    *vp = JSVAL_VOID;
    return JS_TRUE;
}



JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper;

    wrapper = GET_WRAPPER(cx, obj);
    if(!wrapper || !wrapper->IsValid())
        return JS_FALSE;

    nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
    NS_ASSERTION(clazz,"wrapper without class");

    const XPCNativeMemberDescriptor* desc = clazz->LookupMemberByID(id);
    if(desc)
    {
        if(desc->IsWritableAttribute())
            return clazz->SetAttributeFromJSVal(cx, wrapper, desc, vp);
        else
        {
            // Don't fail silently!
            uintN errNum;

            if(desc->IsConstant())
                errNum = NS_ERROR_XPC_CANT_SET_READ_ONLY_CONSTANT;
            else if(desc->IsMethod())
                errNum = NS_ERROR_XPC_CANT_SET_READ_ONLY_METHOD;
            else
            {
                NS_ASSERTION(desc->IsReadOnlyAttribute(),"bad desc");    
                errNum = NS_ERROR_XPC_CANT_SET_READ_ONLY_ATTRIBUTE;
            }
            return ThrowException(errNum, cx, clazz, desc);
        }
    }
    else
    {
        nsIXPCScriptable* ds;
        nsIXPCScriptable* as;
        if(nsnull != (ds = wrapper->GetDynamicScriptable()) &&
           nsnull != (as = wrapper->GetArbitraryScriptable()))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->SetProperty(cx, obj, id, vp,
                                            wrapper, as, &retval)))
                return retval;
            else
            {
                // Don't fail silently!
                return ThrowException(NS_ERROR_XPC_CALL_TO_SCRIPTABLE_FAILED, cx, clazz);
            }
        }
        else
        {
            HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, clazz, id);
            // Don't fail silently!
            return ThrowException(NS_ERROR_XPC_CANT_ADD_PROP_TO_WRAPPED_NATIVE, cx, clazz);
        }
    }
}

#define XPC_BUILT_IN_PROPERTY ((JSProperty*)1)



JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_LookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp
#if defined JS_THREADSAFE && defined DEBUG
                            , const char *file, uintN line
#endif
                            )
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    XPCJSRuntime* rt = nsXPConnect::GetRuntime();

    if(rt && wrapper && wrapper->IsValid())
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");
        if(clazz->LookupMemberByID(id) ||
           id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING) ||
           id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE))
        {
            *objp = obj;
            *propp = XPC_BUILT_IN_PROPERTY;
            return JS_TRUE;
        }
        else if(wrapper->GetNative() &&
                id == rt->GetStringID(XPCJSRuntime::IDX_WRAPPED_JSOBJECT))
        {
            JSObject* realObject = GetDoubleWrappedJSObject(cx, wrapper, id);
            if(realObject)
            {
                *objp = obj;
                *propp = XPC_BUILT_IN_PROPERTY;
                return JS_TRUE;
            }
        }
        else if(id == rt->GetStringID(XPCJSRuntime::IDX_TO_STRING) ||
                id == rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE))
        {
            *objp = obj;
            *propp = XPC_BUILT_IN_PROPERTY;
            return JS_TRUE;
        }
        else if(nsnull != (ds = wrapper->GetDynamicScriptable()) &&
                nsnull != (as = wrapper->GetArbitraryScriptable()))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->LookupProperty(cx, obj, id, objp, propp,
                                               wrapper, as, &retval)))
                return retval;
        }
        else
        {
            HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, clazz, id);
        }
    }

    *objp = nsnull;
    *propp = nsnull;
    return JS_TRUE;
}


JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DefineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(wrapper && 
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");
        if(!clazz->LookupMemberByID(id))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->DefineProperty(cx, obj, id, value,
                                               getter, setter, attrs, propp,
                                               wrapper, as, &retval)))
                return retval;
        }
    }
    // else fall through
    return ThrowException(NS_ERROR_XPC_CANT_DEFINE_PROP_ON_WN, cx);
}


JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_GetAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(!wrapper || !wrapper->IsValid())
    {
        *attrsp = 0;
        return JS_TRUE;
    }

    nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
    NS_ASSERTION(clazz,"wrapper without class");

    const XPCNativeMemberDescriptor* desc = clazz->LookupMemberByID(id);

    if(desc)
    {
        XPCContext* xpcc = nsXPConnect::GetContext(cx);
        if(!xpcc)
            return JS_FALSE;
        
        if(clazz->AllowedToGetStaticProperty(xpcc, wrapper, desc))
            *attrsp = JSPROP_PERMANENT | JSPROP_ENUMERATE;
        else
            *attrsp = JSPROP_PERMANENT;

        return JS_TRUE;
    }

    if(nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        JSBool retval;
        if(NS_SUCCEEDED(ds->GetAttributes(cx, obj, id, prop, attrsp,
                                          wrapper, as, &retval)))
            return retval;
    }

    // else fall through
    *attrsp = 0;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_SetAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");
        if(!clazz->LookupMemberByID(id))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->SetAttributes(cx, obj, id, prop, attrsp,
                                              wrapper, as, &retval)))
                return retval;
        }
    }
    // else fall through and silently ignore
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");
        if(!clazz->LookupMemberByID(id))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->DeleteProperty(cx, obj, id, vp,
                                               wrapper, as, &retval)))
                return retval;
        }
    }
    // else fall through and silently ignore
    NS_ASSERTION(vp, "hey the engine gave me a null pointer");
    *vp = JSVAL_FALSE;
    return JS_TRUE;
}




JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_DefaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        JSBool retval;
        if(NS_SUCCEEDED(ds->DefaultValue(cx, obj, type, vp,
                                         wrapper, as, &retval)))
            return retval;
    }
    return WrappedNative_Convert(cx, obj, type, vp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                        jsval *statep, jsid *idp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper;
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;

    wrapper = GET_WRAPPER(cx, obj);
    if (!wrapper || !wrapper->IsValid()) {
        *statep = JSVAL_NULL;
        if (idp)
            *idp = INT_TO_JSVAL(0);
        return JS_TRUE;
    }

    nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
    NS_ASSERTION(clazz,"wrapper without class");

    if(nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
        return clazz->DynamicEnumerate(wrapper, ds, as, cx, obj, enum_op,
                                       statep, idp);
    else
        return clazz->StaticEnumerate(wrapper, cx, enum_op, statep, idp);
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_CheckAccess(JSContext *cx, JSObject *obj, jsid id,
                      JSAccessMode mode, jsval *vp, uintN *attrsp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx, obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");
        if(!clazz->LookupMemberByID(id))
        {
            JSBool retval;
            if(NS_SUCCEEDED(ds->CheckAccess(cx, obj, id, mode, vp, attrsp,
                                            wrapper, as, &retval)))
                return retval;
        }
    }
    // else fall through...
    switch (mode) {
    case JSACC_WATCH:
        return ThrowException(NS_ERROR_XPC_CANT_WATCH_WN_STATIC, cx);

    case JSACC_IMPORT:
        return ThrowException(NS_ERROR_XPC_CANT_EXPORT_WN_STATIC, cx);

    default:
        return JS_TRUE;
    }
}


JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Call(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    // this is a hack to get the obj of the actual object not the object
    // that JS thinks is the 'this' (which it passes as 'obj').
    if(!(obj = (JSObject*)argv[-2]))
        return JS_FALSE;

    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx,obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        JSBool retval;
        if(NS_SUCCEEDED(ds->Call(cx, obj, argc, argv, rval,
                        wrapper, as, &retval)))
            return retval;
        return ThrowException(NS_ERROR_XPC_SCRIPTABLE_CALL_FAILED, cx);
    }
    return ThrowException(NS_ERROR_XPC_CANT_CALL_WO_SCRIPTABLE, cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Construct(JSContext *cx, JSObject *obj,
                        uintN argc, jsval *argv, jsval *rval)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    // this is a hack to get the obj of the actual object not the object
    // that JS thinks is the 'this' (which it passes as 'obj').
    if(!(obj = (JSObject*)argv[-2]))
        return JS_FALSE;

    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx,obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        JSBool retval;
        if(NS_SUCCEEDED(ds->Construct(cx, obj, argc, argv, rval,
                                      wrapper, as, &retval)))
            return retval;
        return ThrowException(NS_ERROR_XPC_SCRIPTABLE_CTOR_FAILED, cx);
    }
    return ThrowException(NS_ERROR_XPC_CANT_CTOR_WO_SCRIPTABLE, cx);
}

// this is the final resting place of non-handled hasInstance calls
JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_ClassHasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    //XXX our default policy is to just say no. Is this right?
    *bp = JS_FALSE;
    return JS_TRUE;
}

// this is in the ObjectOps and is called first
JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsIXPCScriptable* ds;
    nsIXPCScriptable* as;
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx,obj);
    if(wrapper &&
       wrapper->IsValid() &&
       nsnull != (ds = wrapper->GetDynamicScriptable()) &&
       nsnull != (as = wrapper->GetArbitraryScriptable()))
    {
        JSBool retval;
        if(NS_SUCCEEDED(ds->HasInstance(cx, obj, v, bp,
                        wrapper, as, &retval)))
            return retval;
    }
    return WrappedNative_ClassHasInstance(cx, obj, v, bp);
}

JS_STATIC_DLL_CALLBACK(void)
WrappedNative_Finalize(JSContext *cx, JSObject *obj)
{
    nsXPCWrappedNative* wrapper = GET_WRAPPER(cx,obj);
    if(!wrapper || !wrapper->IsValid())
        return;

    // Defer this push until we know we have a valid wrapper to work with.
    // This call can *startup* XPConnect after it has been shutdown!
    AUTO_PUSH_JSCONTEXT(cx);
    // XXX we don't want to be setting this in finalization. RIGHT????
    // SET_CALLER_JAVASCRIPT(cx);
    NS_ASSERTION(obj == wrapper->GetJSObject(),"bad obj");
    // wrapper is responsible for calling DynamicScriptable->Finalize
    wrapper->JSObjectFinalized(cx, obj);
}

JS_STATIC_DLL_CALLBACK(void)
WrappedNative_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop)
{
    /* If this is not one of our 'built-in' native properties AND 
    *  the JS engine has a callback to handle dropProperty then call it.
    */
    if(prop != XPC_BUILT_IN_PROPERTY)
    {
        JSPropertyRefOp drop = js_ObjectOps.dropProperty;
        if(drop)
            drop(cx, obj, prop);
    }
}        

JS_STATIC_DLL_CALLBACK(JSBool)
WrappedNative_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    AUTO_PUSH_JSCONTEXT(cx);
    SET_CALLER_JAVASCRIPT(cx);
    nsXPCWrappedNative* wrapper;

    wrapper = GET_WRAPPER(cx, obj);
    if(wrapper && wrapper->IsValid())
    {
        nsXPCWrappedNativeClass* clazz = wrapper->GetClass();
        NS_ASSERTION(clazz,"wrapper without class");

        jsid id; 
        if(JS_ValueToId(cx, idval, &id))
        {
            const XPCNativeMemberDescriptor* desc = clazz->LookupMemberByID(id);
            if(desc)
            {
                jsval val;            
                JSObject* real_obj = wrapper->GetJSObject();
                if(WrappedNative_GetProperty(cx, real_obj, id, &val))
                {
                    return js_ObjectOps.defineProperty(cx, real_obj, 
                                                       id, val,
                                                       nsnull, nsnull, 
                                                       0, nsnull);
                }
            }        
        }
    }
    return JS_TRUE;        
}        



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
#endif
#endif
