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
#ifdef DEBUG
#  define FMT_ADDR " @ 0x%p" 
#  define PARAM_ADDR(w) , w
#else
#  define FMT_ADDR ""
#  define PARAM_ADDR(w)
#endif

    XPCWrappedNative* wrapper = ccx.GetWrapper();
    JSContext* cx = ccx.GetJSContext();

    char* sz = nsnull;

    if(wrapper)
    {
        char* name = nsnull;

        XPCNativeScriptableInfo* si = ccx.GetScriptableInfo();
        if(si)
            name = JS_smprintf("%s", si->GetJSClass()->name);

        XPCWrappedNativeTearOff* to = ccx.GetTearOff();
        if(to)
        {
            const char* fmt = name ? " (%s)" : "%s";
            name = JS_sprintf_append(name, fmt, to->GetInterface()->GetName());
        }    
        else if(!name)
        {
            XPCNativeSet* set = wrapper->GetSet();
            XPCNativeInterface** array = set->GetInterfaceArray();
            PRUint16 count = set->GetInterfaceCount();

            if(count == 1)
                name = JS_sprintf_append(name, "%s", array[0]->GetName());
            else
            {
                for(PRUint16 i = 0; i < count; i++)
                {
                    const char* fmt = (i == 0) ? 
                                        "(%s" : (i == count-1) ? 
                                            ", %s)" : ", %s";
                    name = JS_sprintf_append(name, fmt, array[i]->GetName());
                }       
            }
        }

        if(!name)
        {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }

        sz = JS_smprintf("[xpconnect wrapped %s" FMT_ADDR "]",
                           name PARAM_ADDR(wrapper));

        JS_smprintf_free(name);    
    }
    else
    {
        sz = JS_smprintf("[xpconnect wrapped native prototype]");
    }

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

#undef FMT_ADDR
#undef PARAM_ADDR
}

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Shared_ToString(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    ccx.SetArgsAndResultPtr(argc, argv, vp);
    return ToStringGuts(ccx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Shared_ToSource(JSContext *cx, JSObject *obj,
                       uintN argc, jsval *argv, jsval *vp)
{
    static const char empty[] = "{}";
    *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, empty, sizeof(empty)-1));
    return JS_TRUE;
}

/***************************************************************************/

/*
 * We *never* set the tearoff names (e.g. nsIFoo) as JS_ENUMERATE.
 * We *never* set toString or toSource as JS_ENUMERATE.
 */

static JSBool
DefinePropertyIfFound(XPCCallContext& ccx, 
                      JSObject *obj, jsval idval, 
                      XPCNativeSet* set, 
                      XPCNativeInterface* iface,
                      XPCWrappedNativeScope* scope,
                      JSBool reflectToStringAndToSource,
                      XPCWrappedNative* wrapperToReflectInterfaceNames,
                      uintN propFlags) 
{
    JSContext* cx = ccx.GetJSContext();
    XPCNativeMember* member;
    JSBool found;
    jsid id;

    if(set)
        found = set->FindMember(idval, &member, &iface);
    else
        found = (nsnull != (member = iface->FindMember(idval)));

    if(!found)
    {
        HANDLE_POSSIBLE_NAME_CASE_ERROR(cx, set, iface, idval);
            
        if(reflectToStringAndToSource)
        {
            JSNative call;
            const char* name;
            XPCJSRuntime* rt = ccx.GetRuntime();

            if(idval == rt->GetStringJSVal(XPCJSRuntime::IDX_TO_STRING))
            {
                call = XPC_WN_Shared_ToString;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_STRING);
                id   = rt->GetStringID(XPCJSRuntime::IDX_TO_STRING);
            }
            else if(idval == rt->GetStringJSVal(XPCJSRuntime::IDX_TO_SOURCE))
            {
                call = XPC_WN_Shared_ToSource;
                name = rt->GetStringName(XPCJSRuntime::IDX_TO_SOURCE);
                id   = rt->GetStringID(XPCJSRuntime::IDX_TO_SOURCE);
            }

            else
                call = nsnull;

            if(call)
            {
                JSFunction* fun = JS_NewFunction(cx, call, 0, 0, obj, name);
                if(!fun)
                {
                    JS_ReportOutOfMemory(cx);
                    return JS_FALSE;
                }
                
                AutoResolveName arn(ccx, idval);
                return OBJ_DEFINE_PROPERTY(cx, obj, id, 
                                           OBJECT_TO_JSVAL(JS_GetFunctionObject(fun)),
                                           nsnull, nsnull,
                                           propFlags & ~JSPROP_ENUMERATE, 
                                           nsnull);
            }

            // This *might* be a tearoff name that is not yet part of our
            // set. Let's lookup the name and see if it is the name of an 
            // interface. Then we'll see if the object actually *does* this
            // interface and add a tearoff as necessary.

            if(wrapperToReflectInterfaceNames)
            {
                const char* name;
                XPCWrappedNativeTearOff* to;
                JSObject* jso;
                
                if(JSVAL_IS_STRING(idval) &&
                   nsnull != (name = JS_GetStringBytes(JSVAL_TO_STRING(idval))) &&
                   nsnull != (iface = XPCNativeInterface::GetNewOrUsed(ccx, name)) &&
                   nsnull != (to = wrapperToReflectInterfaceNames->
                                        FindTearOff(ccx, iface, JS_TRUE)) &&
                   nsnull != (jso = to->GetJSObject()))

                {
                    AutoResolveName arn(ccx, idval);
                    return JS_ValueToId(cx, idval, &id) &&
                           OBJ_DEFINE_PROPERTY(cx, obj, id, OBJECT_TO_JSVAL(jso),
                                               nsnull, nsnull, 
                                               propFlags & ~JSPROP_ENUMERATE, 
                                               nsnull);
                }
            }
        }
        return JS_TRUE;    
    }

    if(!member)
    {
        if(wrapperToReflectInterfaceNames)
        {
            XPCWrappedNativeTearOff* to = 
              wrapperToReflectInterfaceNames->FindTearOff(ccx, iface, JS_TRUE);
            
            if(!to)
                return JS_FALSE;
            JSObject* jso = to->GetJSObject();
            if(!jso)
                return JS_FALSE;

            AutoResolveName arn(ccx, idval);
            return JS_ValueToId(cx, idval, &id) &&
                   OBJ_DEFINE_PROPERTY(cx, obj, id, OBJECT_TO_JSVAL(jso),
                                       nsnull, nsnull, 
                                       propFlags & ~JSPROP_ENUMERATE, 
                                       nsnull);
        }        
        return JS_TRUE;    
    }

    if(member->IsConstant())
    {
        jsval val;
        AutoResolveName arn(ccx, idval);
        return member->GetValue(ccx, iface, &val) &&
               JS_ValueToId(cx, idval, &id) &&
               OBJ_DEFINE_PROPERTY(cx, obj, id, val, nsnull, nsnull,
                                   propFlags, nsnull);
    }

    jsval funval;
    if(!member->GetValue(ccx, iface, &funval))
        return JS_FALSE;

    JSObject* funobj =
        JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(funval),
                               scope->GetGlobalJSObject());
    if(!funobj)
        return JS_FALSE;

    if(member->IsMethod())
    {
        AutoResolveName arn(ccx, idval);
        return JS_ValueToId(cx, idval, &id) &&
               OBJ_DEFINE_PROPERTY(cx, obj, id, OBJECT_TO_JSVAL(funobj),
                                   nsnull, nsnull, propFlags, nsnull);
    }

    // else...

    NS_ASSERTION(member->IsAttribute(), "way broken!");

    // Avoid infinite recursion on getter/setter re-lookup.
    if(ccx.GetResolveName() == idval)
        return JS_TRUE;
    
    propFlags |= JSPROP_GETTER | JSPROP_SHARED;
    if(member->IsWritableAttribute())
    {
        propFlags |= JSPROP_SETTER;
        propFlags &= ~JSPROP_READONLY;
    }

    AutoResolveName arn(ccx, idval);
    return JS_ValueToId(cx, idval, &id) &&
           OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                               (JSPropertyOp) funobj,
                               (JSPropertyOp) funobj,
                               propFlags, nsnull);
}

/***************************************************************************/
/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_OnlyIWrite_PropertyStub(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj, idval);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);
    
    if(ccx.GetResolveName() == idval)
        return JS_TRUE;

    if(ccx.GetInterface() && ccx.GetMember() && 
       ccx.GetMember()->IsWritableAttribute())
        return JS_TRUE;
    
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_CannotModifyPropertyStub(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Shared_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
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
            {
                if(!ccx.GetTearOff())
                {
                    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
                    if(si && (si->WantCall() || si->WantConstruct()))
                    {
                        *vp = OBJECT_TO_JSVAL(obj);
                        return JS_TRUE;
                    }
                }
            }
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
            ccx.SetName(ccx.GetRuntime()->GetStringJSVal(XPCJSRuntime::IDX_TO_STRING));
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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Shared_Enumerate(JSContext *cx, JSObject *obj)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    // Since we aren't going to enumerate tearoff names and the prototype
    // handles non-mutated members, we can do this potential short-circuit.
    if(!wrapper->HasMutatedSet())
        return JS_TRUE;

    // Since we might be using this in the helper case, we check to
    // see if this is all avoidable.
    if(wrapper->GetScriptableInfo() &&
       wrapper->GetScriptableInfo()->DontEnumStaticProps())
        return JS_TRUE;

    XPCNativeSet* set = wrapper->GetSet();
    XPCNativeSet* protoSet = wrapper->GetProto()->GetSet();

    JSProperty* prop;
    JSObject* obj2;
    jsid id;

    PRUint16 interface_count = set->GetInterfaceCount();
    XPCNativeInterface** interfaceArray = set->GetInterfaceArray();
    for(PRUint16 i = 0; i < interface_count; i++)
    {
        XPCNativeInterface* interface = interfaceArray[i];
        PRUint16 member_count = interface->GetMemberCount();
        for(PRUint16 k = 0; k < member_count; k++)
        {
            XPCNativeMember* member = interface->GetMemberAt(k);
            jsval name = member->GetName();

            // Skip if this member is going to come from the proto.
            PRUint16 index;
            if(protoSet->FindMember(name, nsnull, &index) && index == i)
                continue;

            // The Lookup will force a Resolve and eager Define of the property
            if(!JS_ValueToId(cx, name, &id) ||
               !OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
            {
                return JS_FALSE;
            }
        }
    }
    return JS_TRUE;
}

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(void)
XPC_WN_NoHelper_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNative* p = (XPCWrappedNative*) JS_GetPrivate(cx, obj);
    if(!p)
        return;
    p->FlatJSObjectFinalized(cx, obj);
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_NoHelper_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    XPCCallContext ccx(JS_CALLER, cx, obj, idval);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCNativeSet* set = ccx.GetSet();
    if(!set)
        return JS_TRUE;

    // Don't resolve properties that are on our prototype.
    if(ccx.GetInterface() && !ccx.GetStaticMemberIsLocal())
        return JS_TRUE;

    return DefinePropertyIfFound(ccx, obj, idval, 
                                 set, nsnull, wrapper->GetScope(),
                                 JS_TRUE, wrapper,
                                 JSPROP_ENUMERATE |
                                 JSPROP_READONLY |
                                 JSPROP_PERMANENT);
}

JSClass XPC_WN_NoHelper_JSClass = {
    "XPCWrappedNative_NoHelper",    // name;
    JSCLASS_HAS_PRIVATE |
    JSCLASS_PRIVATE_IS_NSISUPPORTS, // flags;

    /* Mandatory non-null function pointer members. */
    XPC_WN_OnlyIWrite_PropertyStub, // addProperty;
    XPC_WN_CannotModifyPropertyStub,// delProperty;
    JS_PropertyStub,                // getProperty;
    XPC_WN_OnlyIWrite_PropertyStub, // setProperty;

    XPC_WN_Shared_Enumerate,        // enumerate;
    XPC_WN_NoHelper_Resolve,        // resolve;
    XPC_WN_Shared_Convert,          // convert;
    XPC_WN_NoHelper_Finalize,       // finalize;

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

extern "C" JS_IMPORT_DATA(JSObjectOps) js_ObjectOps;

static JSObjectOps XPC_WN_JSOps;

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_JSOp_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                      jsval *statep, jsid *idp)
{
    // XXX fix this...

/*
    Here are the cases:

    set jsclass enumerate to stub (unless noted otherwise)

    if( helper wants NO enumerate )
        if( DONT_ENUM_STATICS )
            use enumerate stub - don't use this JSOp thing at all
        else
            do shared enumerate - don't use this JSOp thing at all
    else if( helper wants old enumerate )
        use this JSOp
        if( DONT_ENUM_STATICS )
            call scriptable enumerate
            call stub
        else
            if( set not mutated )
                call scriptable enumerate
                call stub
            else
                call shared enumerate
                call scriptable enumerate
                call stub
    else // if( helper wants new enumerate )
        if( DONT_ENUM_STATICS )
            forward to scriptable enumerate
        else
            if( set not mutated )
                forward to scriptable enumerate
            else
                call shared enumerate
                forward to scriptable enumerate
*/

    
    //XPC_WN_NoHelper_NewEnumerate
    
    return js_ObjectOps.enumerate(cx, obj, enum_op, statep, idp);
}

JSObjectOps * JS_DLL_CALLBACK
XPC_WN_GetObjectOpsStub(JSContext *cx, JSClass *clazz)
{
    return &XPC_WN_JSOps;
}

JSBool xpc_InitWrappedNativeJSOps()
{
    if(!XPC_WN_JSOps.newObjectMap)
    {
        memcpy(&XPC_WN_JSOps, &js_ObjectOps, sizeof(JSObjectOps));
        XPC_WN_JSOps.enumerate = XPC_WN_JSOp_Enumerate;
    }
    return JS_TRUE;
}

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_MaybeResolvingPropertyStub(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    if(ccx.GetResolvingWrapper() == wrapper)
        return JS_TRUE;
    return Throw(NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN, cx);
}

// macro fun!
#define PRE_HELPER_STUB                                                      \
    XPCWrappedNative* wrapper =                                              \
        XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);               \
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);                            \
    PRBool retval = JS_TRUE;                                                 \
    nsresult rv = wrapper->GetScriptable()->

#define POST_HELPER_STUB                                                     \
    if(NS_FAILED(rv))                                                        \
        Throw(rv, cx);                                                       \
    return retval;

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_AddProperty(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    PRE_HELPER_STUB
    AddProperty(wrapper, cx, obj, idval, vp, &retval);
    POST_HELPER_STUB
}   

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_DelProperty(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    PRE_HELPER_STUB
    DelProperty(wrapper, cx, obj, idval, vp, &retval);
    POST_HELPER_STUB
}   
     
JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_GetProperty(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    PRE_HELPER_STUB
    GetProperty(wrapper, cx, obj, idval, vp, &retval);
    POST_HELPER_STUB
}   

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_SetProperty(JSContext *cx, JSObject *obj, jsval idval, jsval *vp)
{
    PRE_HELPER_STUB
    SetProperty(wrapper, cx, obj, idval, vp, &retval);
    POST_HELPER_STUB
}   

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    PRE_HELPER_STUB
    Convert(wrapper, cx, obj, type, vp, &retval);
    POST_HELPER_STUB
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_CheckAccess(JSContext *cx, JSObject *obj, jsval idval,
                          JSAccessMode mode, jsval *vp)
{
    PRE_HELPER_STUB
    CheckAccess(wrapper, cx, obj, idval, mode, vp, &retval);
    POST_HELPER_STUB
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
    // this is a hack to get the obj of the actual object not the object
    // that JS thinks is the 'this' (which it passes as 'obj').
    if(!(obj = (JSObject*)argv[-2]))
        return JS_FALSE;

    PRE_HELPER_STUB
    Call(wrapper, cx, obj, argc, argv, rval, &retval);
    POST_HELPER_STUB
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                        jsval *rval)
{
    // this is a hack to get the obj of the actual object not the object
    // that JS thinks is the 'this' (which it passes as 'obj').
    if(!(obj = (JSObject*)argv[-2]))
        return JS_FALSE;

    PRE_HELPER_STUB
    Construct(wrapper, cx, obj, argc, argv, rval, &retval);
    POST_HELPER_STUB
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    PRE_HELPER_STUB
    HasInstance(wrapper, cx, obj, v, bp, &retval);
    POST_HELPER_STUB
}

JS_STATIC_DLL_CALLBACK(void)
XPC_WN_Helper_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNative* wrapper = (XPCWrappedNative*) JS_GetPrivate(cx, obj);
    if(!wrapper)
        return;
    wrapper->GetScriptable()->Finalize(wrapper, cx, obj);
    wrapper->FlatJSObjectFinalized(cx, obj);
}

JS_STATIC_DLL_CALLBACK(uint32)
XPC_WN_Helper_Mark(JSContext *cx, JSObject *obj, void *arg)
{
    PRUint32 ignored;
    XPCWrappedNative* wrapper =
        XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);
    if(wrapper && wrapper->IsValid())
        wrapper->GetScriptable()->Mark(wrapper, cx, obj, arg, &ignored);
    return (uint32) ignored;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Helper_NewEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                           jsval *statep, jsid *idp)
{
    // XXX fix this!

    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    switch(enum_op)
    {
        case JSENUMERATE_INIT:
            // XXX fix this!
            if(1)
            //if(!wrapper->HasMutatedSet())
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
XPC_WN_Helper_NewResolve(JSContext *cx, JSObject *obj, jsval idval, uintN flags,
                         JSObject **objp)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    // XXX This nonsense is here because the JS engine will call my resolve
    // if I try to define a property with a getter or setter. This is a
    // problem since I'm already servicing resolve and we risk infinite
    // recursion.
    if(ccx.GetResolveName() == idval)
        return JS_TRUE;

    jsval old = ccx.SetResolveName(idval);

    // Since I can't always tell if the scriptable did anything on resolve,
    // I'll do my part first and if the scriptable overwrites, then fine.

    if(wrapper->HasMutatedSet())
    {
        // XXX handle this
        // We can use XPCCallContext::SetJSID and if we discover that the id
        // needs to be in a tearoff then we can do a resolve on the tearoff and
        // return.
    }

    nsresult rv = NS_OK;
    JSBool retval = JS_TRUE;
    XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
    if(si && si->WantNewResolve())
    {
        XPCWrappedNative* oldResolvingWrapper;
        
        if(si->AllowPropModsDuringResolve())
            oldResolvingWrapper = ccx.SetResolvingWrapper(wrapper);

        rv = si->GetScriptable()->NewResolve(wrapper, cx, obj, idval, flags,
                                             objp, &retval);
                                               
        if(si->AllowPropModsDuringResolve())
            (void)ccx.SetResolvingWrapper(oldResolvingWrapper);
    }

    old = ccx.SetResolveName(old);
    NS_ASSERTION(old == idval, "bad nest");

    if(NS_FAILED(rv))
        Throw(rv, cx);
    return retval;
}

/***************************************************************************/

XPCNativeScriptableInfo::XPCNativeScriptableInfo(nsIXPCScriptable* scriptable, 
                                                 JSUint32 flags)
    : mScriptable(scriptable), mFlags(flags)
{
    memset(&mJSClass, 0, sizeof(JSClass));
}

XPCNativeScriptableInfo::~XPCNativeScriptableInfo()
{
    if(mJSClass.name) 
        nsMemory::Free((void*)mJSClass.name);
}

// static 
XPCNativeScriptableInfo* 
XPCNativeScriptableInfo::NewInfo(nsIXPCScriptable* scriptable, 
                                 JSUint32 flags)
{
    XPCNativeScriptableInfo* self = 
        new XPCNativeScriptableInfo(scriptable, flags);

    if(!self)
        return nsnull;

    JSClass* clazz = self->GetJSClass();
    
    if(NS_FAILED(scriptable->GetClassName((char**)&clazz->name)) || !clazz->name)
    {
        delete self;
        return nsnull;    
    }
    
    clazz->flags = JSCLASS_HAS_PRIVATE | 
                   JSCLASS_PRIVATE_IS_NSISUPPORTS |
                   JSCLASS_NEW_ENUMERATE |
                   JSCLASS_NEW_RESOLVE;

    if(self->WantAddProperty())
        clazz->addProperty = XPC_WN_Helper_AddProperty;
    else if(self->UseJSStubForAddProperty())
        clazz->addProperty = JS_PropertyStub;
    else if(self->AllowPropModsDuringResolve())
        clazz->addProperty = XPC_WN_MaybeResolvingPropertyStub;
    else
        clazz->addProperty = XPC_WN_CannotModifyPropertyStub;

    if(self->WantDelProperty())
        clazz->delProperty = XPC_WN_Helper_DelProperty;
    else if(self->UseJSStubForDelProperty())
        clazz->delProperty = JS_PropertyStub;
    else if(self->AllowPropModsDuringResolve())
        clazz->delProperty = XPC_WN_MaybeResolvingPropertyStub;
    else
        clazz->delProperty = XPC_WN_CannotModifyPropertyStub;

    if(self->WantGetProperty())
        clazz->getProperty = XPC_WN_Helper_GetProperty;
    else
        clazz->getProperty = JS_PropertyStub;

    if(self->WantSetProperty())
        clazz->setProperty = XPC_WN_Helper_SetProperty;
    else if(self->UseJSStubForSetProperty())
        clazz->setProperty = JS_PropertyStub;
    else if(self->AllowPropModsDuringResolve())
        clazz->setProperty = XPC_WN_MaybeResolvingPropertyStub;
    else
        clazz->setProperty = XPC_WN_CannotModifyPropertyStub;

    // We have to figure out enumeration strategy at call time
    clazz->enumerate = (JSEnumerateOp) XPC_WN_Helper_NewEnumerate;
    clazz->getObjectOps = XPC_WN_GetObjectOpsStub;

    // We have to figure out resolve strategy at call time
    clazz->resolve = (JSResolveOp) XPC_WN_Helper_NewResolve;

    if(self->WantConvert())
        clazz->convert = XPC_WN_Helper_Convert;
    else
        clazz->convert = XPC_WN_Shared_Convert;

    if(self->WantFinalize())
        clazz->finalize = XPC_WN_Helper_Finalize;
    else
        clazz->finalize = XPC_WN_NoHelper_Finalize;

    // We let the rest default to nsnull unless the helper wants them...
    if(self->WantCheckAccess())
        clazz->checkAccess = XPC_WN_Helper_CheckAccess;
    if(self->WantCall())
        clazz->call = XPC_WN_Helper_Call;
    if(self->WantConstruct())
        clazz->construct = XPC_WN_Helper_Construct;
    if(self->WantHasInstance())
        clazz->hasInstance = XPC_WN_Helper_HasInstance;
    if(self->WantMark())
        clazz->mark = XPC_WN_Helper_Mark;
                                                            
    return self;
}

/***************************************************************************/
/***************************************************************************/

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
        JSBool retval = XPCWrappedNative::SetAttribute(ccx);
        if(retval && vp && argc)
            *vp = argv[0];
        return retval;
    }
    // else...

    ccx.SetCallableInfo(ci, JS_FALSE);
    return XPCWrappedNative::GetAttribute(ccx);
}

/***************************************************************************/

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Proto_Enumerate(JSContext *cx, JSObject *obj)
{
    // Could support a lazier lookup here.
    
    NS_ASSERTION(JS_InstanceOf(cx, obj, &XPC_WN_Proto_JSClass, nsnull), "bad proto");
    XPCWrappedNativeProto* self = (XPCWrappedNativeProto*) JS_GetPrivate(cx, obj);
    if(!self)
        return JS_FALSE;

    if(self->GetScriptableInfo() &&
       self->GetScriptableInfo()->DontEnumStaticProps())
        return JS_TRUE;

    XPCNativeSet* set = self->GetSet();
    if(!set)
        return JS_FALSE;

    XPCCallContext ccx(JS_CALLER, cx);
    if(!ccx.IsValid())
        return JS_FALSE;

    JSProperty* prop;
    JSObject* obj2;
    jsid id;

    PRUint16 interface_count = set->GetInterfaceCount();
    XPCNativeInterface** interfaceArray = set->GetInterfaceArray();
    for(PRUint16 i = 0; i < interface_count; i++)
    {
        XPCNativeInterface* interface = interfaceArray[i];
        PRUint16 member_count = interface->GetMemberCount();

        for(PRUint16 k = 0; k < member_count; k++)
        {
            XPCNativeMember* member = interface->GetMemberAt(k);

            // The Lookup will force a Resolve and eager Define of the property
            if(!JS_ValueToId(cx, member->GetName(), &id) ||
               !OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_Proto_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    NS_ASSERTION(JS_InstanceOf(cx, obj, &XPC_WN_Proto_JSClass, nsnull), "bad proto");

    XPCWrappedNativeProto* self = (XPCWrappedNativeProto*) JS_GetPrivate(cx, obj);
    if(!self)
        return JS_FALSE;

    XPCCallContext ccx(JS_CALLER, cx);
    if(!ccx.IsValid())
        return JS_FALSE;

    uintN enumFlag = self->GetScriptableInfo() &&
                     self->GetScriptableInfo()->DontEnumStaticProps() ?
                        0 : JSPROP_ENUMERATE;


    return DefinePropertyIfFound(ccx, obj, idval, 
                                 self->GetSet(), nsnull, self->GetScope(),
                                 JS_TRUE, nsnull,
                                 enumFlag);
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

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_TearOff_Enumerate(JSContext *cx, JSObject *obj)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCWrappedNativeTearOff* to = ccx.GetTearOff();
    XPCNativeInterface* iface;

    if(!to || nsnull == (iface = to->GetInterface()))    
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);
    
    JSProperty* prop;
    JSObject* obj2;
    jsid id;

    PRUint16 member_count = iface->GetMemberCount();
    for(PRUint16 k = 0; k < member_count; k++)
    {
        XPCNativeMember* member = iface->GetMemberAt(k);

        // The Lookup will force a Resolve and eager Define of the property
        if(!JS_ValueToId(cx, member->GetName(), &id) ||
           !OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
        {
            return JS_FALSE;
        }
    }

    return JS_TRUE;
}        

JS_STATIC_DLL_CALLBACK(JSBool)
XPC_WN_TearOff_Resolve(JSContext *cx, JSObject *obj, jsval idval)
{
    XPCCallContext ccx(JS_CALLER, cx, obj);
    XPCWrappedNative* wrapper = ccx.GetWrapper();
    THROW_AND_RETURN_IF_BAD_WRAPPER(cx, wrapper);

    XPCWrappedNativeTearOff* to = ccx.GetTearOff();
    XPCNativeInterface* iface;
    
    if(!to || nsnull == (iface = to->GetInterface()))    
        return Throw(NS_ERROR_XPC_BAD_OP_ON_WN_PROTO, cx);                   \

    return DefinePropertyIfFound(ccx, obj, idval, nsnull, iface, 
                                 wrapper->GetScope(),
                                 JS_TRUE, nsnull,
                                 JSPROP_READONLY |
                                 JSPROP_PERMANENT |
                                 JSPROP_ENUMERATE);
}

JS_STATIC_DLL_CALLBACK(void)
XPC_WN_TearOff_Finalize(JSContext *cx, JSObject *obj)
{
    XPCWrappedNativeTearOff* p = (XPCWrappedNativeTearOff*) 
        JS_GetPrivate(cx, obj);
    if(!p)
        return;
    p->JSObjectFinalized();
}

JSClass XPC_WN_Tearoff_JSClass = {
    "WrappedNative_TearOff",            // name;
    JSCLASS_HAS_PRIVATE,                // flags;

    /* Mandatory non-null function pointer members. */
    XPC_WN_OnlyIWrite_PropertyStub,     // addProperty;
    XPC_WN_CannotModifyPropertyStub,    // delProperty;
    JS_PropertyStub,                    // getProperty;
    XPC_WN_OnlyIWrite_PropertyStub,     // setProperty;
    XPC_WN_TearOff_Enumerate,           // enumerate;
    XPC_WN_TearOff_Resolve,             // resolve;
    XPC_WN_Shared_Convert,              // convert;
    XPC_WN_TearOff_Finalize,            // finalize;

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
/***************************************************************************/
/***************************************************************************/

#if 0

// XXX snipets of code that I may use to restore removed functionality

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

/***************************************************************************/
#endif
