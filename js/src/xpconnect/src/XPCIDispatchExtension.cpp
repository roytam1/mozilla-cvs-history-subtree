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
#include "XPCIDispatchExtension.h"

static const char* const IDISPATCH_NAME = "IDispatch";

JS_STATIC_DLL_CALLBACK(JSBool)
COMObjectConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, 
                     jsval *rval)
{
    // Make sure we were called with one string parameter
    if(argc != 1 || (argc == 1 && !JSVAL_IS_STRING(argv[0])))
    {
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
    IDispatch* pDispatch = IDispObject::COMCreateInstance(bytes);
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

JSBool nsXPConnect::XPCIDispatchExtension::Initialize(JSContext * aJSContext,
                                  JSObject * aGlobalJSObj)
{
    // TODO: Cleanup error code
    return JS_DefineFunction(aJSContext, aGlobalJSObj, "COMObject", 
                         COMObjectConstructor, 1, 0) ? PR_TRUE : PR_FALSE;
}

nsresult
nsXPConnect::XPCIDispatchExtension::GetInfoForIID(const nsIID & aIID, nsIInterfaceInfo** info)
{
    if(aIID.Equals(NSID_IDISPATCH))
    {
        *info = new IDispatchInfo();
        if (*info)
        {
            NS_ADDREF(*info);
            return NS_OK;
        }
    }
    return NS_ERROR_NO_INTERFACE;
}

nsresult
nsXPConnect::XPCIDispatchExtension::GetInfoForName(const char * name, nsIInterfaceInfo** info)
{
    if(strcmp(name, IDISPATCH_NAME) == 0)
    {
        *info = new IDispatchInfo();
        if (*info)
        {
            NS_ADDREF(*info);
            return NS_OK;
        }
    }
    return NS_ERROR_NO_INTERFACE;
}

JSBool nsXPConnect::XPCIDispatchExtension::DefineProperty(XPCCallContext & ccx, JSObject *obj, jsval idval,
                     XPCWrappedNative* wrapperToReflectInterfaceNames,
					 uintN propFlags, JSBool* resolved)
{
	XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(ccx, "IDispatch");
	if (iface == nsnull)
		return JS_FALSE;
    XPCWrappedNativeTearOff* to = 
		wrapperToReflectInterfaceNames->FindTearOff(ccx, iface, JS_TRUE);
	if (to == nsnull)
		return JS_FALSE;

    JSObject* jso = to->GetJSObject();
	if (jso == nsnull)
		return JS_FALSE;

	IDispatchInterface::Member * IDispatchMember = to->GetJSObject().GetIDispatchInfo()->FindMember(idval);
	if (IDispatchMember == nsnull)
		return JS_FALSE;
	jsval funval;
	if(!IDispatchMember->GetValue(ccx, iface, &funval))
	    return JS_FALSE;
    JSObject* funobj = JS_CloneFunctionObject(ccx, JSVAL_TO_OBJECT(funval), obj);
    if(!funobj)
        return JS_FALSE;
    jsid id;
	if (IDispatchMember->IsFunction())
	{
        AutoResolveName arn(ccx, idval);
        if(resolved)
            *resolved = JS_TRUE;
        return JS_ValueToId(ccx, idval, &id) &&
               OBJ_DEFINE_PROPERTY(ccx, obj, id, OBJECT_TO_JSVAL(funobj),
                                   nsnull, nsnull, propFlags, nsnull);
	}
    NS_ASSERTION(!IDispatchMember || IDispatchMember->IsSetter(), "way broken!");
    propFlags |= JSPROP_GETTER | JSPROP_SHARED;
	if (IDispatchMember->IsSetter())
    {
        propFlags |= JSPROP_SETTER;
        propFlags &= ~JSPROP_READONLY;
    }
    AutoResolveName arn(ccx, idval);
    if(resolved)
        *resolved = JS_TRUE;
    return JS_ValueToId(ccx, idval, &id) &&
           OBJ_DEFINE_PROPERTY(ccx, obj, id, JSVAL_VOID,
                               (JSPropertyOp) funobj,
                               (JSPropertyOp) funobj,
                               propFlags, nsnull);

}

JSBool nsXPConnect::XPCIDispatchExtension::Enumerate(XPCCallContext& ccx, JSObject* obj, XPCWrappedNative * wrapper)
{
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(ccx, &NSID_IDISPATCH);
    if(iface)
    {
        XPCWrappedNativeTearOff* tearoff = wrapper->FindTearOff(ccx, iface);
        if(tearoff)
        {
            IDispatchInterface* pInfo = tearoff->GetJSObject().GetIDispatchInfo();
            PRUint32 members = pInfo->GetMemberCount();
            for(PRUint32 index = 0; index < members; ++index)
            {
                IDispatchInterface::Member & member = pInfo->GetMember(index);
                jsval name = member.GetName();
                if(!xpc_ForcePropertyResolve(ccx, obj, name))
                    return JS_FALSE;
            }
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}