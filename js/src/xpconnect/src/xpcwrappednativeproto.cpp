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
 * Copyright (C) 1998 Netscape Communications Corporation. All
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

/* new flattening stuff. */

#include "xpcprivate.h"

XPCWrappedNativeProto::XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                                             nsIClassInfo* ClassInfo,
                                             XPCNativeSet* Set)
    : mScope(Scope),
      mJSProtoObject(nsnull),
      mClassInfo(ClassInfo),
      mSet(Set),
      mSecurityInfo(nsnull),
      mScriptableInfo(nsnull)
{
    // This native object lives as long as its associated JSObject - killed
    // by finalization of the JSObject (or explicitly if Init fails).
}

XPCWrappedNativeProto::~XPCWrappedNativeProto()
{
    NS_ASSERTION(!mJSProtoObject, "JSProtoObject still alive");
}

JSBool 
XPCWrappedNativeProto::Init(XPCCallContext& ccx)
{
    // Get the class scriptable helper (if present)
    if(mClassInfo)
    {
        nsCOMPtr<nsISupports> possibleHelper;
        nsresult rv = mClassInfo->GetHelperForLanguage(
                                        nsIClassInfo::LANGUAGE_JAVASCRIPT,
                                        getter_AddRefs(possibleHelper));
        if(NS_SUCCEEDED(rv) && possibleHelper)
        {
            nsCOMPtr<nsIXPCScriptable> helper(do_QueryInterface(possibleHelper));
            if(helper)
            {
                JSUint32 flags;
                rv = helper->GetFlags(ccx.GetJSContext(), nsnull, nsnull, 
                                      &flags, ccx.GetArbitraryScriptable());
                if(NS_FAILED(rv))
                    return JS_FALSE;

                mScriptableInfo = new XPCNativeScriptableInfo(helper, flags);
                if(!mScriptableInfo)
                    return JS_FALSE;

                JSClass* clazz = mScriptableInfo->GetJSClass();
                // XXX fill in the JSClass...
                // remember that name must be nsMemory::Alloc'd
                    

            }
        }
    }

    mJSProtoObject = JS_NewObject(ccx.GetJSContext(), 
                                  &XPC_WN_Proto_JSClass,
                                  mScope->GetPrototypeJSObject(),
                                  mScope->GetGlobalJSObject()); 
    return mJSProtoObject && 
           JS_SetPrivate(ccx.GetJSContext(), mJSProtoObject, this);
}

void 
XPCWrappedNativeProto::JSProtoObjectFinalized(JSContext *cx, JSObject *obj)
{
    NS_ASSERTION(obj == mJSProtoObject, "huh?");
    mJSProtoObject = nsnull;
    delete this;
}

// static 
XPCWrappedNativeProto* 
XPCWrappedNativeProto::GetNewOrUsed(XPCCallContext& ccx,
                                    XPCWrappedNativeScope* Scope,
                                    nsIClassInfo* ClassInfo)
{
    ClassInfo2WrappedNativeProtoMap* map = Scope->GetWrappedNativeProtoMap();
    
    // XXX locking

    XPCWrappedNativeProto* proto = map->Find(ClassInfo);
    if(proto)
        return proto;

    XPCNativeSet* set = XPCNativeSet::GetNewOrUsed(ccx, ClassInfo);
    if(!set)
        return nsnull;

    proto = new XPCWrappedNativeProto(Scope, ClassInfo, set);
    
    if(proto && !proto->Init(ccx))
    {
        delete proto;
        proto = nsnull;
    }

    if(proto)
        map->Add(ClassInfo, proto);

    return proto;
}

// static 
XPCWrappedNativeProto* 
XPCWrappedNativeProto::BuildOneOff(XPCCallContext& ccx, 
                                   XPCWrappedNativeScope* Scope,
                                   XPCNativeSet* Set)
{
    XPCWrappedNativeProto* proto = 
        new XPCWrappedNativeProto(Scope, nsnull, Set);
    
    if(proto && !proto->Init(ccx))
    {
        delete proto;
        proto = nsnull;    
    }
    return proto;
}



