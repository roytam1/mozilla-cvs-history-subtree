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

/* Class used to manage the wrapped native objects within a JS scope. */

#include "xpcprivate.h"

XPCWrappedNativeScope* XPCWrappedNativeScope::gScopes = nsnull;
XPCWrappedNativeScope* XPCWrappedNativeScope::gDyingScopes = nsnull;

// static 
XPCWrappedNativeScope* 
XPCWrappedNativeScope::GetNewOrUsed(XPCCallContext& ccx, JSObject* aGlobal)
{

    XPCWrappedNativeScope* scope = FindInJSObjectScope(ccx, aGlobal, JS_TRUE);
    if(!scope)
        scope = new XPCWrappedNativeScope(ccx, aGlobal);
    return scope;
}

XPCWrappedNativeScope::XPCWrappedNativeScope(XPCCallContext& ccx, 
                                             JSObject* aGlobal)
    :   mRuntime(ccx.GetRuntime()),
        mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mComponents(nsnull),
        mNext(nsnull),
        mGlobalJSObject(nsnull),
        mPrototypeJSObject(nsnull)
{
    // add ourselves to the scopes list
    {   // scoped lock
        XPCAutoLock lock(mRuntime->GetMapLock());  

#ifdef DEBUG
        for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
            NS_ASSERTION(aGlobal != cur->GetGlobalJSObject(), "dup object");
#endif

        mNext = gScopes;
        gScopes = this;
    }

    if(aGlobal)
        SetGlobal(ccx, aGlobal);
}        

void 
XPCWrappedNativeScope::SetComponents(nsXPCComponents* aComponents)
{
    NS_IF_ADDREF(aComponents);
    NS_IF_RELEASE(mComponents);
    mComponents = aComponents;
}

void 
XPCWrappedNativeScope::SetGlobal(XPCCallContext& ccx, JSObject* aGlobal)
{
    // We allow for calling this more than once. This feature is used by
    // nsXPConnect::InitClassesWithNewWrappedGlobal.
    
    mGlobalJSObject = aGlobal;

    // Lookup 'globalObject.Object.prototype' for our wrapper's proto
    {
        AutoJSErrorAndExceptionEater eater(ccx); // scoped error eater

        jsval val;
        jsid idObj = mRuntime->GetStringID(XPCJSRuntime::IDX_OBJECT);
        jsid idProto = mRuntime->GetStringID(XPCJSRuntime::IDX_PROTOTYPE);

        if(OBJ_GET_PROPERTY(ccx, aGlobal, idObj, &val) &&
           !JSVAL_IS_PRIMITIVE(val) &&
           OBJ_GET_PROPERTY(ccx, JSVAL_TO_OBJECT(val), idProto, &val) &&
           !JSVAL_IS_PRIMITIVE(val))
        {
            mPrototypeJSObject = JSVAL_TO_OBJECT(val);
        }
        else
        {
#if defined(DEBUG_jband) || defined(DEBUG_jst)
            NS_ERROR("Can't get globalObject.Object.prototype");
//            NS_WARNING("Can't get globalObject.Object.prototype");
#endif
        }
    }
}        

XPCWrappedNativeScope::~XPCWrappedNativeScope()
{
    // We can do additional cleanup assertions here...

    if(mWrappedNativeMap)
    {
        NS_ASSERTION(0 == mWrappedNativeMap->Count(), "scope has non-empty map");
        delete mWrappedNativeMap;    
    }

    if(mWrappedNativeProtoMap)
    {
        NS_ASSERTION(0 == mWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mWrappedNativeProtoMap;    
    }

    // XXX we should assert that we are dead or that xpconnect has shutdown
    // XXX might not want to do this at xpconnect shutdown time???
    NS_IF_RELEASE(mComponents);
}        


// static 
void 
XPCWrappedNativeScope::FinishedMarkPhaseOfGC(JSContext* cx, XPCJSRuntime* rt)
{
    // Hold the lock until return...
    XPCAutoLock lock(rt->GetMapLock());  

    // Since the JSGC_END call happens outside of a lock,
    // it is possible for us to get called here twice before the FinshedGC 
    // call happens. So, we allow for gDyingScopes not being null.

    XPCWrappedNativeScope* cur = gScopes;
    XPCWrappedNativeScope* prev = nsnull;
    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        if(cur->mGlobalJSObject && 
           JS_IsAboutToBeFinalized(cx, cur->mGlobalJSObject))
        {
            // XXX some wrapper invalidation is in order here...
            

            cur->mGlobalJSObject = nsnull;
            
            // Move this scope from the live list to the dying list.
            if(prev)
                prev->mNext = next;
            else
                gScopes = next;
            cur->mNext = gDyingScopes;
            gDyingScopes = cur;            
            cur = nsnull;
        }
        else if(cur->mPrototypeJSObject && 
                JS_IsAboutToBeFinalized(cx, cur->mPrototypeJSObject))
        {
            // XXX do I want to be warned about this odd case?
            cur->mPrototypeJSObject = nsnull;                
        }
        if(cur)
            prev = cur;
        cur = next;
    }
}        

// static 
void 
XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC(JSContext* cx)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntime();
    if(!rt)
        return;

    // Hold the lock until return...
    XPCAutoLock lock(rt->GetMapLock());  
    KillDyingScopes();
}        

JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeSetMarker(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNative*) he->value)->MarkSets();
    return HT_ENUMERATE_NEXT;
}

// We need to explicitly mark all the protos too because some protos may be 
// alive in the hashtable but not currently in use by any wrapper
JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeProtoSetMarker(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNativeProto*) he->value)->MarkSet();
    return HT_ENUMERATE_NEXT;
}

// static 
void
XPCWrappedNativeScope::MarkAllInterfaceSets()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeSetMarker, nsnull);
        cur->mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoSetMarker, nsnull);
    }
}

#ifdef DEBUG
JS_STATIC_DLL_CALLBACK(intN)
ASSERT_WrappedNativeSetNotMarked(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNative*) he->value)->ASSERT_SetsNotMarked();
    return HT_ENUMERATE_NEXT;
}

JS_STATIC_DLL_CALLBACK(intN)
ASSERT_WrappedNativeProtoSetNotMarked(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNativeProto*) he->value)->ASSERT_SetNotMarked();
    return HT_ENUMERATE_NEXT;
}

// static 
void
XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(
            ASSERT_WrappedNativeSetNotMarked, nsnull);
        cur->mWrappedNativeProtoMap->Enumerate(
            ASSERT_WrappedNativeProtoSetNotMarked, nsnull);
    }
}
#endif

JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeTearoffSweeper(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNative*) he->value)->SweepTearOffs();
    return HT_ENUMERATE_NEXT;
}

// static 
void
XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        cur->mWrappedNativeMap->Enumerate(WrappedNativeTearoffSweeper, nsnull);
}        

// static 
void 
XPCWrappedNativeScope::KillDyingScopes()
{
    // always called inside the lock!
    XPCWrappedNativeScope* cur = gDyingScopes;
    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        delete cur;
        cur = next;
    }
    gDyingScopes = nsnull;
}        

struct ShutdownData
{
    ShutdownData(XPCCallContext& accx)
        : ccx(accx), wrapperCount(0), protoCount(0) {}
    XPCCallContext& ccx;
    int wrapperCount;
    int protoCount;
};

JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeShutdownEnumerator(JSHashEntry *he, intN i, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    XPCWrappedNative* wrapper = (XPCWrappedNative*) he->value;
    
    if(wrapper->IsValid())
    {
        if(!wrapper->HasSharedProto())
            data->protoCount++;
        wrapper->SystemIsBeingShutDown(data->ccx);
        data->wrapperCount++;
    }
    return HT_ENUMERATE_REMOVE;
}

JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeProtoShutdownEnumerator(JSHashEntry *he, intN i, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    XPCWrappedNativeProto* proto = (XPCWrappedNativeProto*) he->value;
    
    proto->SystemIsBeingShutDown(data->ccx);
    data->protoCount++;
    return HT_ENUMERATE_REMOVE;
}

//static
void 
XPCWrappedNativeScope::SystemIsBeingShutDown(XPCCallContext& ccx)
{
    int liveScopeCount = 0;
    
    ShutdownData data(ccx);

    XPCWrappedNativeScope* cur;
    
    // First move all the scopes to the dying list.
    
    cur = gScopes;
    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        cur->mNext = gDyingScopes;
        gDyingScopes = cur;
        cur = next;
        liveScopeCount++;
    }
    gScopes = nsnull;
    
    // Walk the unified dying list and call shutdown on all wrappers and protos

    for(cur = gDyingScopes; cur; cur = cur->mNext)
    {
        // Walk the protos first. Wrapper shutdown can leave dangling
        // proto pointers in the proto map.
        cur->mWrappedNativeProtoMap->
                Enumerate(WrappedNativeProtoShutdownEnumerator,  &data);
        cur->mWrappedNativeMap->
                Enumerate(WrappedNativeShutdownEnumerator,  &data);
    }

    // Now it is safe to kill all the scopes.
    KillDyingScopes();

#ifdef XPC_DUMP_AT_SHUTDOWN
    if(data.wrapperCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNatives\n", data.wrapperCount);
    if(data.protoCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeProtos\n", data.protoCount);
    if(liveScopeCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeScopes\n", liveScopeCount);
#endif
}


/***************************************************************************/

static 
XPCWrappedNativeScope* 
GetScopeOfObject(JSContext* cx, JSObject* obj)
{
    JSClass* clazz;
    nsISupports* supports;

#ifdef JS_THREADSAFE
    clazz = JS_GetClass(cx, obj);
#else
    clazz = JS_GetClass(obj);
#endif

    if(!clazz ||
       !(clazz->flags & JSCLASS_HAS_PRIVATE) ||
       !(clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) ||
       !(supports = (nsISupports*) JS_GetPrivate(cx, obj)))
        return nsnull;

    nsCOMPtr<nsIXPConnectWrappedNative> iface = do_QueryInterface(supports);
    if(iface)
    {
        // We can fairly safely assume that this is really one of our
        // nsXPConnectWrappedNative objects. No other component in our
        // universe should be creating objects that implement the
        // nsIXPConnectWrappedNative interface!
        return ((XPCWrappedNative*)supports)->GetScope();
    }
    return nsnull;
}


#ifdef DEBUG
void DEBUG_CheckForComponentsInScope(XPCCallContext& ccx, JSObject* obj,
                                     JSBool OKIfNotInitialized)
{
    if(OKIfNotInitialized)
        return;

    const char* name = ccx.GetRuntime()->GetStringName(XPCJSRuntime::IDX_COMPONENTS);
    jsval prop;
    if(JS_LookupProperty(ccx, obj, name, &prop) && !JSVAL_IS_PRIMITIVE(prop))
        return;

    static const char msg[] =
    "\n"
    "XPConnect is being called on a scope without a 'Components' property!\n"
    "\n"
    "This is pretty much always bad. It usually means that native code is\n"
    "making a callback to an interface implemented in JavaScript, but the\n"
    "document where the JS object was created has already been cleared and the\n"
    "global properties of that document's window are *gone*. Generally this\n"
    "indicates a problem that should be addressed in the design and use of the\n"
    "callback code."
    "\n";

#ifdef I_FOOLISHLY_WANT_TO_IGNORE_THIS_LIKE_THE_OTHER_CRAP_WE_PRINTF
    NS_WARNING(msg);
#else
    NS_ERROR(msg);
#endif
}        
#else
#define DEBUG_CheckForComponentsInScope(ccx, obj, OKIfNotInitialized) ((void)0)
#endif

// static 
XPCWrappedNativeScope* 
XPCWrappedNativeScope::FindInJSObjectScope(XPCCallContext& ccx, JSObject* obj,
                                           JSBool OKIfNotInitialized)
{
    XPCWrappedNativeScope* scope;

    if(!obj)
        return nsnull;
    
    // If this object is itself a wrapped native then we can get the 
    // scope directly. 
    
    scope = GetScopeOfObject(ccx, obj);
    if(scope)
        return scope;

    // Else we'll have to look up the parent chain to get the scope
    
    JSObject* parent;

    while(nsnull != (parent = JS_GetParent(ccx, obj)))
        obj = parent;

    // XXX We are assuming that the scope count is low enough that traversing
    // the linked list is more reasonable then doing a hashtable lookup.
    {   // scoped lock
        XPCAutoLock lock(ccx.GetRuntime()->GetMapLock());  
        for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        {
            if(obj == cur->GetGlobalJSObject())
            {
                DEBUG_CheckForComponentsInScope(ccx, obj, OKIfNotInitialized);
                return cur;
            }
        }
    }

    // Failure to find the scope is only OK if the caller told us it might fail.
    // This flag would only be set in the call from
    // XPCWrappedNativeScope::GetNewOrUsed 
    NS_ASSERTION(OKIfNotInitialized, "No scope has this global object!");
    return nsnull;
}        


// static 
void
XPCWrappedNativeScope::DebugDumpAllScopes(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;

    // get scope count.
    int count = 0;
    XPCWrappedNativeScope* cur;
    for(cur = gScopes; cur; cur = cur->mNext)
        count++ ;

    XPC_LOG_ALWAYS(("chain of %d XPCWrappedNativeScope(s)", count));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDyingScopes @ %x", gDyingScopes));
        if(depth)
            for(cur = gScopes; cur; cur = cur->mNext)
                cur->DebugDump(depth);
    XPC_LOG_OUTDENT();
#endif
}        

#ifdef DEBUG
JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeMapDumpEnumerator(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNative*)he->value)->DebugDump(*(PRInt16*)arg);
    return HT_ENUMERATE_NEXT;
}
JS_STATIC_DLL_CALLBACK(intN)
WrappedNativeProtoMapDumpEnumerator(JSHashEntry *he, intN i, void *arg)
{
    ((XPCWrappedNativeProto*)he->value)->DebugDump(*(PRInt16*)arg);
    return HT_ENUMERATE_NEXT;
}
#endif

void
XPCWrappedNativeScope::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeScope @ %x", this));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mNext @ %x", mNext));
        XPC_LOG_ALWAYS(("mComponents @ %x", mComponents));
        XPC_LOG_ALWAYS(("mGlobalJSObject @ %x", mGlobalJSObject));
        XPC_LOG_ALWAYS(("mPrototypeJSObject @ %x", mPrototypeJSObject));

        XPC_LOG_ALWAYS(("mWrappedNativeMap @ %x with %d wrappers(s)", \
                         mWrappedNativeMap, \
                         mWrappedNativeMap ? mWrappedNativeMap->Count() : 0));
        // iterate contexts...
        if(depth && mWrappedNativeMap && mWrappedNativeMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedNativeMap->Enumerate(WrappedNativeMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mWrappedNativeProtoMap @ %x with %d protos(s)", \
                         mWrappedNativeProtoMap, \
                         mWrappedNativeProtoMap ? mWrappedNativeProtoMap->Count() : 0));
        // iterate contexts...
        if(depth && mWrappedNativeProtoMap && mWrappedNativeProtoMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}        

