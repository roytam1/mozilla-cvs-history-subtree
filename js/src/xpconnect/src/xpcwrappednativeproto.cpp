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

#ifdef DEBUG
PRInt32 XPCWrappedNativeProto::gDEBUG_LiveProtoCount = 0;
#endif

XPCWrappedNativeProto::XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                                             nsIClassInfo* ClassInfo,
                                             XPCNativeSet* Set)
    : mScope(Scope),
      mJSProtoObject(nsnull),
      mClassInfo(ClassInfo),
      mSet(Set),
      mSecurityInfo(nsnull),
      mScriptableInfo(nsnull),
      mRefCnt(0)
{
    // This native object lives as long as its associated JSObject - killed
    // by finalization of the JSObject (or explicitly if Init fails).

#ifdef DEBUG
    PR_AtomicIncrement(&gDEBUG_LiveProtoCount);
#endif
}

XPCWrappedNativeProto::~XPCWrappedNativeProto()
{
    NS_ASSERTION(!mJSProtoObject, "JSProtoObject still alive");
#ifdef DEBUG
    PR_AtomicDecrement(&gDEBUG_LiveProtoCount);
#endif

    delete mScriptableInfo;    
}

JSBool
XPCWrappedNativeProto::Init(XPCCallContext& ccx,
                            const XPCNativeScriptableInfo* scriptableInfo)
{
    if(scriptableInfo && scriptableInfo->GetScriptable())
    {
        mScriptableInfo = scriptableInfo->Clone();
        if(!mScriptableInfo || !mScriptableInfo->BuildJSClass())
            return JS_FALSE;
    }

    JSClass* jsclazz = mScriptableInfo && 
                       mScriptableInfo->AllowPropModsToPrototype() ?
                            &XPC_WN_ModsAllowed_Proto_JSClass :
                            &XPC_WN_NoMods_Proto_JSClass;

    mJSProtoObject = JS_NewObject(ccx, jsclazz,
                                  mScope->GetPrototypeJSObject(),
                                  mScope->GetGlobalJSObject());

    JSBool retval = mJSProtoObject && JS_SetPrivate(ccx, mJSProtoObject, this); 

    if(retval)
        AddRef();
    return retval;
}

void                     
XPCWrappedNativeProto::AddRef()
{
    (void)PR_AtomicIncrement((PRInt32*)&mRefCnt);
}       
 
void                     
XPCWrappedNativeProto::Release()
{
    if(0 == PR_AtomicDecrement((PRInt32*)&mRefCnt))
        delete this;        
}        

void
XPCWrappedNativeProto::JSProtoObjectFinalized(JSContext *cx, JSObject *obj)
{
    NS_ASSERTION(obj == mJSProtoObject, "huh?");
    if(IsShared())
    {
        ClassInfo2WrappedNativeProtoMap* map = mScope->GetWrappedNativeProtoMap();
        {   // scoped lock
            nsAutoLock lock(mScope->GetRuntime()->GetMapLock());  
            map->Remove(mClassInfo);
        }
    }
    mJSProtoObject = nsnull;
    Release();
}

void 
XPCWrappedNativeProto::SystemIsBeingShutDown(XPCCallContext& ccx)
{
    // Note that the instance might receive this call multiple times
    // as we walk to here from various places.
    if(mJSProtoObject)
    {
        // short circuit future finalization
        JS_SetPrivate(ccx, mJSProtoObject, nsnull);
        mJSProtoObject = nsnull;
        // We *must* leak the scriptable because it holds a dynamically 
        // allocated JSClass that the JS engine might try to use.
        mScriptableInfo = nsnull;
        Release();
    }    
}

// static
XPCWrappedNativeProto*
XPCWrappedNativeProto::GetNewOrUsed(XPCCallContext& ccx,
                                    XPCWrappedNativeScope* Scope,
                                    nsIClassInfo* ClassInfo,
                                    const XPCNativeScriptableInfo* scriptableInfo)
{
    XPCWrappedNativeProto* proto;

    ClassInfo2WrappedNativeProtoMap* map = Scope->GetWrappedNativeProtoMap();

    {   // scoped lock
        nsAutoLock lock(Scope->GetRuntime()->GetMapLock());  
        proto = map->Find(ClassInfo);
        if(proto)
        {
            proto->AddRef();
            return proto;
        }
    }

    XPCNativeSet* set = XPCNativeSet::GetNewOrUsed(ccx, ClassInfo);
    if(!set)
        return nsnull;

    proto = new XPCWrappedNativeProto(Scope, ClassInfo, set);

    if(!proto || !proto->Init(ccx, scriptableInfo))
    {
        delete proto;
        return nsnull;
    }

    proto->AddRef();

    {   // scoped lock
        nsAutoLock lock(Scope->GetRuntime()->GetMapLock());  
        map->Add(ClassInfo, proto);
    }

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

    if(!proto || !proto->Init(ccx, nsnull))
    {
        delete proto;
        return nsnull;
    }

    proto->AddRef();

    return proto;
}

void
XPCWrappedNativeProto::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeProto @ %x with mRefCnt = %d", this, mRefCnt));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDEBUG_LiveProtoCount is %d", gDEBUG_LiveProtoCount));
        XPC_LOG_ALWAYS(("mScope @ %x", mScope));
        XPC_LOG_ALWAYS(("mJSProtoObject @ %x", mJSProtoObject));
        XPC_LOG_ALWAYS(("mSet @ %x", mSet));
        XPC_LOG_ALWAYS(("mSecurityInfo of %x", mSecurityInfo));
        XPC_LOG_ALWAYS(("mScriptableInfo @ %x", mScriptableInfo));
        if(depth && mScriptableInfo)
        {
            XPC_LOG_INDENT();
            XPC_LOG_ALWAYS(("mScriptable @ %x", mScriptableInfo->GetScriptable()));
            XPC_LOG_ALWAYS(("mFlags of %x", mScriptableInfo->GetFlags()));
            XPC_LOG_ALWAYS(("mJSClass @ %x", mScriptableInfo->GetJSClass()));
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}        


