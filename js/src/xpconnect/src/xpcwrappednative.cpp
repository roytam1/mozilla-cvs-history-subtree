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

// static
XPCWrappedNative*
XPCWrappedNative::GetNewOrUsed(XPCCallContext& ccx,
                               nsISupports* Object,
                               XPCWrappedNativeScope* Scope,
                               XPCNativeInterface* Interface)
{
    // XXX should support null Interface IFF the object has an nsIClassInfo?

    nsCOMPtr<nsISupports> identity(do_QueryInterface(Object));
    if(!identity)
        return nsnull;

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

    // XXX add locking

    XPCWrappedNative* wrapper = map->Find(identity);
    if(wrapper)
    {
        if(!wrapper->FindTearOff(ccx, Interface))
            return nsnull;
        NS_ADDREF(wrapper);
        return wrapper;
    }

    XPCWrappedNativeProto* proto;

    nsCOMPtr<nsIClassInfo> info(do_QueryInterface(Object));

    // If we are making a wrapper for the nsIClassInfo interface then
    // We *don't* want to have it use the prototype meant for instances
    // of that class.
    if(info && !Interface->GetIID()->Equals(NS_GET_IID(nsIClassInfo)))
    {
        proto = XPCWrappedNativeProto::GetNewOrUsed(ccx, Scope, info);
    }
    else
    {
        XPCNativeSet* set =
            XPCNativeSet::GetNewOrUsed(ccx, nsnull, Interface, 0);

        if(!set)
            return nsnull;

        proto = XPCWrappedNativeProto::BuildOneOff(ccx, Scope, set);
    }

    if(!proto)
        return nsnull;

    wrapper = new XPCWrappedNative(Object, proto);
    if(!wrapper)
        return nsnull;

    if(!wrapper->Init(ccx))
    {
        wrapper->Release();
        wrapper->Release();
        return nsnull;
    }

    // XXX allow for null Interface?
    if(!wrapper->FindTearOff(ccx, Interface))
    {
        wrapper->Release();
        wrapper->Release();
        return nsnull;
    }

    map->Add(wrapper);
    return wrapper;
}

// static 
XPCWrappedNative*
XPCWrappedNative::GetUsedOnly(XPCCallContext& ccx,
                              nsISupports* Object,
                              XPCWrappedNativeScope* Scope,
                              XPCNativeInterface* Interface)
{
    // XXX should support null Interface IFF the object has an nsIClassInfo?
    
    nsCOMPtr<nsISupports> identity(do_QueryInterface(Object));
    if(!identity)
        return nsnull;

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

    // XXX add locking

    XPCWrappedNative* wrapper = map->Find(identity);
    if(wrapper)
    {
        if(!wrapper->FindTearOff(ccx, Interface))
            return nsnull;
        NS_ADDREF(wrapper);
        return wrapper;
    }

    return nsnull;
}

XPCWrappedNative::XPCWrappedNative(nsISupports* aIdentity,
                                   XPCWrappedNativeProto* aProto)
    : mProto(aProto),
      mSet(aProto->GetSet()),
      mIdentity(aIdentity),
      mFlatJSObject(nsnull),
      mScriptableInfo(nsnull)
{
    NS_INIT_ISUPPORTS();
}

XPCWrappedNative::~XPCWrappedNative()
{
    if(mScriptableInfo && mScriptableInfo != mProto->GetScriptableInfo())
        delete mScriptableInfo;

    // XXX fill me in...
}

JSBool
XPCWrappedNative::Init(XPCCallContext& ccx)
{
    // Do double addref first. So failure here means object can be deleted
    // by double release.

    JSContext* cx = ccx.GetJSContext();

    // Hacky double initial addrefs to get the JSObject rooted w/o creating
    // another call context (as would happen in AddRef)
    mRefCnt++;
    NS_LOG_ADDREF(this, mRefCnt, "XPCWrappedNative", sizeof(*this));
    mRefCnt++;
    NS_LOG_ADDREF(this, mRefCnt, "XPCWrappedNative", sizeof(*this));
    JS_AddNamedRoot(cx, &mFlatJSObject, "XPCWrappedNative::mFlatJSObject");

    // setup our scriptable info...

    nsCOMPtr<nsIXPCScriptable> helper;

    if(HasSharedProto())
    {
        XPCNativeScriptableInfo* si = mProto->GetScriptableInfo();
        if(si && si->GetScriptable())
        {
            if(si->DontAskInstanceForScriptable())
                mScriptableInfo = si;
            else
            {
                helper = do_QueryInterface(mIdentity);
                if(!helper || helper.get() == si->GetScriptable())
                {
                    mScriptableInfo = si;
                }
            }
        }
    }

    if(!mScriptableInfo)
    {
        if(!helper)
            helper = do_QueryInterface(mIdentity);
        if(helper)
        {
            JSUint32 flags;
            nsresult rv = helper->GetFlags(&flags);
            if(NS_FAILED(rv))
                return JS_FALSE;

            mScriptableInfo = XPCNativeScriptableInfo::NewInfo(helper, flags);
            if(!mScriptableInfo)
                return JS_FALSE;
        }
    }

    // If we have a one-off proto, then it should share our scriptable.
    // This allows the prototypes JSClass callbacks to do the right things
    // (like respecting the DONT_ENUM_STATIC_PROPS flag) w/o requiring 
    // scriptable objects to have an nsIClassInfo.
    if(!HasSharedProto())
        mProto->SetScriptableInfo(mScriptableInfo);

    JSObject* parent = mProto->GetScope()->GetGlobalJSObject();

    if(mScriptableInfo && mScriptableInfo->WantPreCreate())
    {
        if(NS_FAILED(mScriptableInfo->GetScriptable()->
            PreCreate(mIdentity, cx, parent, &parent)))
        {
            return JS_FALSE;        
        }
    }

    XPCWrappedNativeScope* scope = mProto->GetScope();

    // create our flatJSObject

    JSClass* jsclazz = mScriptableInfo ?
                            mScriptableInfo->GetJSClass() :
                            &XPC_WN_NoHelper_JSClass;

    NS_ASSERTION(jsclazz &&
                 jsclazz->name &&
                 jsclazz->flags &&
                 jsclazz->addProperty &&
                 jsclazz->delProperty &&
                 jsclazz->getProperty &&
                 jsclazz->setProperty &&
                 jsclazz->enumerate &&
                 jsclazz->resolve &&
                 jsclazz->convert &&
                 jsclazz->finalize, "bad class");

    mFlatJSObject = JS_NewObject(cx, jsclazz,
                                 mProto->GetJSProtoObject(),
                                 parent);
    if(!mFlatJSObject || !JS_SetPrivate(cx, mFlatJSObject, this))
        return JS_FALSE;

    if(mScriptableInfo && mScriptableInfo->WantCreate())
        mScriptableInfo->GetScriptable()->Create(this, cx, mFlatJSObject);

    // XXX and so on....
    return JS_TRUE;
}


NS_INTERFACE_MAP_BEGIN(XPCWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectJSObjectHolder)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPConnectWrappedNative)
NS_INTERFACE_MAP_END_THREADSAFE


/* 
 *  Wrapped Native lifetime management is messy!
 *
 *  - At creation we push the refcount to 2 (only one of which is owned by
 *    the native caller that caused the wrapper creation).
 *  - Whenever the refcount goes from 1 -> 2 we root mFlatJSObject.
 *  - Whenever the refcount goes from 2 -> 1 we unroot mFlatJSObject
 *    (in this state *no* native callers own a reference to the wrapper).
 *  - The *only* thing that can make the wrapper get destroyed is the
 *    finalization of mFlatJSObject.
 *
 *  - The wrapper has a pointer to the nsISupports 'view' of the wrapped native
 *    object i.e... mIdentity. This is held until the wrapper's refcount goes
 *    to zero and the wrapper is released.
 *
 *  - The wrapper also has 'tearoffs'. It has one tearoff for each interface
 *    that is actually used on the native object. 'Used' means we have either
 *    needed to QueryInterface to verify the availability of that interface
 *    of that we've had to QueryInterface in order to actually make a call
 *    into the wrapped object via the pointer for the given interface.
 *
 *  - Each tearoff's 'mNative' member (if non-null) indicates one reference 
 *    held by our wrapper on the wrapped native for the given interface
 *    associated with the tearoff. If we release that reference then we set
 *    the tearoff's 'mNative' to null.  
 *
 *  - We use the occasion of the JavaScript GCCallback for the JSGC_MARK_END
 *    event to scan the tearoffs of all wrappers for non-null mNative members
 *    that represent unused references. We can tell that a given tearoff's 
 *    mNative is unused by noting that no live XPCCallContexts hold a pointer 
 *    to the tearoff.
 *
 *  - As a time/space tradeoff we may decide to not do this scanning on 
 *    *every* JavaScript GC. We *do* want to do this *sometimes* because
 *    we want to allow for wrapped native's to do their own tearoff patterns.
 *    So, we want to avoid holding references to interfaces that we don't need.
 *    At the same time, we don't want to be bracketing every call into a
 *    wrapped native object with a QueryInterface/Release pair. And we *never*
 *    make a call into the object except via the correct interface for which 
 *    we've QI'd.
 *
 *  - Each tearoff *can* have a mJSObject whose lazily resolved properties
 *    represent the methods/attributes/constants of that specific interface.
 *    This is optionally reflected into JavaScript as "foo.nsIFoo" when "foo"
 *    is the name of mFlatJSObject and "nsIFoo" is the name of the given 
 *    interface associated with the tearoff. When we create the tearoff's
 *    mJSObject we set it's parent to be mFlatJSObject. This way we know that
 *    when mFlatJSObject get's collected there are no outstanding reachable
 *    tearoff mJSObjects. Note that we must clear the private of any lingering
 *    mJSObjects at this point because we have no guarentee of the *order* of
 *    finalization within a given gc cycle.
 */

nsrefcnt
XPCWrappedNative::AddRef(void)
{
    nsrefcnt cnt = (nsrefcnt) PR_AtomicIncrement((PRInt32*)&mRefCnt);
    NS_LOG_ADDREF(this, cnt, "XPCWrappedNative", sizeof(*this));
    if(2 == cnt && IsValid())
    {
        XPCCallContext ccx(NATIVE_CALLER);
        if(ccx.IsValid())
            JS_AddNamedRoot(ccx.GetJSContext(), &mFlatJSObject,
                            "XPCWrappedNative::mFlatJSObject");
    }
    return cnt;
}

nsrefcnt
XPCWrappedNative::Release(void)
{
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    nsrefcnt cnt = (nsrefcnt) PR_AtomicDecrement((PRInt32*)&mRefCnt);
    NS_LOG_RELEASE(this, cnt, "XPCWrappedNative");
    if(0 == cnt)
    {
        NS_DELETEXPCOM(this);
        return 0;
    }
    if(1 == cnt && IsValid())
    {
        XPCJSRuntime* rt = GetRuntime();
        if(rt)
            JS_RemoveRootRT(rt->GetJSRuntime(), &mFlatJSObject);
    }
    return cnt;
}

void
XPCWrappedNative::FlatJSObjectFinalized(JSContext *cx, JSObject *obj)
{
    // Iterate the tearoffs and null out each of their JSObject's privates.
    // This will keep them from trying to access their pointers to the
    // dying tearoff object. We can safely assume that those remaining 
    // JSObjects are about to be finalized too.

    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            JSObject* jso = to->GetJSObject();
            if(jso)
            {
                to->JSObjectFinalized();
                JS_SetPrivate(cx, jso, nsnull);
            }
        }
    }

    mFlatJSObject = nsnull;
    Release();
}

/***************************************************************************/

// static
XPCWrappedNative*
XPCWrappedNative::GetWrappedNativeOfJSObject(JSContext* cx,
                                             JSObject* obj,
                                             JSObject** pobj2,
                                             XPCWrappedNativeTearOff** pTearOff)
{
    NS_PRECONDITION(obj, "bad param");
    JSObject* cur = obj;

    while(cur)
    {
        JSClass* clazz = JS_GET_CLASS(cx, cur);

        if(clazz == &XPC_WN_NoHelper_JSClass ||
           clazz->getObjectOps == XPC_WN_GetObjectOpsStub)
        {
            if(pobj2)
                *pobj2 = cur;
            return (XPCWrappedNative*) JS_GetPrivate(cx, cur);
        }

        if(clazz == &XPC_WN_Tearoff_JSClass)
        {
            if(pobj2)
                *pobj2 = cur;

            XPCWrappedNativeTearOff* to =
                (XPCWrappedNativeTearOff*) JS_GetPrivate(cx, cur);
            if(!to)
                return nsnull;
            if(pTearOff)
                *pTearOff = to;
            
            return (XPCWrappedNative*) JS_GetPrivate(cx, JS_GetParent(cx,cur));
        }

        cur = JS_GetPrototype(cx, cur);
    }
    return nsnull;
}

JSBool 
XPCWrappedNative::ExtendSet(XPCCallContext& ccx, XPCNativeInterface* aInterface)
{
    // XXX add locking

    if(!mSet->HasInterface(aInterface))
    {
        XPCNativeSet* newSet = 
            XPCNativeSet::GetNewOrUsed(ccx, mSet, aInterface, 
                                       mSet->GetInterfaceCount());
        if(!newSet)
            return JS_FALSE;
        mSet = newSet;
    }
    return JS_TRUE;        
}

JSBool 
XPCWrappedNative::InitTearOff(XPCCallContext& ccx,
                              XPCWrappedNativeTearOff* aTearOff,
                              XPCNativeInterface* aInterface,
                              JSBool needJSObject)
{
    // Determine if the object really does this interface...

    const nsIID* iid = aInterface->GetIID();
    nsISupports* identity = GetIdentityObject();
    nsISupports* obj;

    if(NS_FAILED(identity->QueryInterface(*iid, (void**)&obj)) || !obj)
        return JS_FALSE;

    // Guard against trying to build a tearoff for a shared nsIClassInfo. 
    if(iid->Equals(NS_GET_IID(nsIClassInfo)))
    {
        nsCOMPtr<nsISupports> alternate_identity(do_QueryInterface(obj));
        if(alternate_identity.get() != identity)
        {
            NS_RELEASE(obj);
            return JS_FALSE;
        }
    }

    // If this is not already in our set we need to extend our set.
    if(!GetSet()->HasInterface(aInterface) && !ExtendSet(ccx, aInterface))
    {
        NS_RELEASE(obj);
        return JS_FALSE;
    }

    aTearOff->SetInterface(aInterface);
    aTearOff->SetNative(obj);

    if(needJSObject && !InitTearOffJSObject(ccx, aTearOff))
        return JS_FALSE;

    return JS_TRUE;
}     

JSBool 
XPCWrappedNative::InitTearOffJSObject(XPCCallContext& ccx, 
                                      XPCWrappedNativeTearOff* to)
{
    JSContext* cx = ccx.GetJSContext();
    JSObject* obj = JS_NewObject(cx, &XPC_WN_Tearoff_JSClass,
                                 GetScope()->GetPrototypeJSObject(),
                                 mFlatJSObject);
    
    if(!obj || !JS_SetPrivate(cx, obj, to))
        return JS_FALSE;
    to->SetJSObject(obj);
    return JS_TRUE;
}

/***************************************************************************/

#ifdef XPC_DETECT_LEADING_UPPERCASE_ACCESS_ERRORS
// static
void
XPCWrappedNative::HandlePossibleNameCaseError(JSContext* cx,
                                              XPCNativeSet* set,
                                              XPCNativeInterface* iface,
                                              jsval name)
{
    XPCCallContext ccx(JS_CALLER, cx);
    HandlePossibleNameCaseError(ccx, set, iface, name);
}

// static
void
XPCWrappedNative::HandlePossibleNameCaseError(XPCCallContext& ccx,
                                              XPCNativeSet* set,
                                              XPCNativeInterface* iface,
                                              jsval name)
{
    if(!ccx.IsValid())
        return;

    JSString* oldJSStr;
    JSString* newJSStr;
    PRUnichar* oldStr;
    PRUnichar* newStr;
    JSContext* cx = ccx.GetJSContext();
    XPCNativeMember* member;
    XPCNativeInterface* interface;

    if(JSVAL_IS_STRING(name) &&
       nsnull != (oldJSStr = JSVAL_TO_STRING(name)) &&
       nsnull != (oldStr = (PRUnichar*) JS_GetStringChars(oldJSStr)) &&
       oldStr[0] != 0 &&
       nsCRT::IsUpper(oldStr[0]) &&
       nsnull != (newStr = nsCRT::strdup(oldStr)))
    {
        newStr[0] = nsCRT::ToLower(newStr[0]);
        newJSStr = JS_NewUCStringCopyZ(cx, (const jschar*)newStr);
        nsCRT::free(newStr);
        if(newJSStr && (set ? 
             set->FindMember(STRING_TO_JSVAL(newJSStr), &member, &interface) :
             (JSBool) iface->FindMember(STRING_TO_JSVAL(newJSStr))))
        {
            // found it!
            const char* ifaceName = interface->GetNameString();
            const char* goodName = JS_GetStringBytes(newJSStr);
            const char* badName = JS_GetStringBytes(oldJSStr);
            char* locationStr = nsnull;

            nsCOMPtr<nsIXPCException> e =
                dont_AddRef(NS_STATIC_CAST(nsIXPCException*,
                    nsXPCException::NewException("", NS_OK, nsnull, nsnull)));

            nsCOMPtr<nsIJSStackFrameLocation> loc = nsnull;
            if(e)
            {
                nsresult rv;
                rv = e->GetLocation(getter_AddRefs(loc));
                if(NS_SUCCEEDED(rv) && loc)
                {
                    rv = loc->ToString(&locationStr);
                    if(NS_FAILED(rv))
                        locationStr = nsnull;
                }
            }

            if(locationStr && ifaceName && goodName && badName )
            {
                printf("**************************************************\n"
                       "ERROR: JS code at [%s]\n"
                       "tried to access nonexistent property called\n"
                       "\'%s\' on interface of type \'%s\'.\n"
                       "That interface does however have a property called\n"
                       "\'%s\'. Did you mean to access that lowercase property?\n"
                       "Please fix the JS code as appropriate.\n"
                       "**************************************************\n",
                        locationStr, badName, ifaceName, goodName);
            }
            if(locationStr)
                nsMemory::Free(locationStr);
        }
    }
}
#endif

/***************************************************************************/

static JSBool Throw(uintN errNum, XPCCallContext& ccx)
{
    XPCThrower::Throw(errNum, ccx);
    return JS_FALSE;
}

static JSBool ThrowBadParam(nsresult rv, uintN paramNum, XPCCallContext& ccx)
{
    XPCThrower::ThrowBadParam(rv, paramNum, ccx);
    return JS_FALSE;
}

static void ThrowBadResult(nsresult result, XPCCallContext& ccx)
{
    XPCThrower::ThrowBadResult(NS_ERROR_XPC_NATIVE_RETURNED_FAILURE,
                               result, ccx);
}

static JSBool ReportOutOfMemory(XPCCallContext& ccx)
{
    JS_ReportOutOfMemory(ccx.GetJSContext());
    return JS_FALSE;
}

enum SizeMode {GET_SIZE, GET_LENGTH};

/***************************************************************************/

static JSBool
GetArraySizeFromParam(XPCCallContext& ccx,
                      nsIInterfaceInfo* ifaceInfo,
                      const nsXPTMethodInfo* methodInfo,
                      const nsXPTParamInfo& paramInfo,
                      uint16 vtblIndex,
                      uint8 paramIndex,
                      SizeMode mode,
                      nsXPTCVariant* dispatchParams,
                      JSUint32* result)
{
    uint8 argnum;
    nsresult rv;

    // XXX fixup the various exceptions that are thrown

    if(mode == GET_SIZE)
        rv = ifaceInfo->GetSizeIsArgNumberForParam(vtblIndex, &paramInfo, 0, &argnum);
    else
        rv = ifaceInfo->GetLengthIsArgNumberForParam(vtblIndex, &paramInfo, 0, &argnum);
    if(NS_FAILED(rv))
        return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

    const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
    const nsXPTType& arg_type = arg_param.GetType();

    // XXX require PRUint32 here - need to require in compiler too!
    if(arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_U32)
        return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

    *result = dispatchParams[argnum].val.u32;

    return JS_TRUE;
}


static JSBool
GetInterfaceTypeFromParam(XPCCallContext& ccx,
                          nsIInterfaceInfo* ifaceInfo,
                          const nsXPTMethodInfo* methodInfo,
                          const nsXPTParamInfo& paramInfo,
                          uint16 vtblIndex,
                          uint8 paramIndex,
                          const nsXPTType& datum_type,
                          nsXPTCVariant* dispatchParams,
                          nsID** result)
{
    uint8 argnum;
    nsresult rv;
    uint8 type_tag = datum_type.TagPart();

    // XXX fixup the various exceptions that are thrown

    if(type_tag == nsXPTType::T_INTERFACE)
    {
        rv = ifaceInfo->GetIIDForParam(vtblIndex, &paramInfo, result);
        if(NS_FAILED(rv))
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, ccx);
    }
    else if(type_tag == nsXPTType::T_INTERFACE_IS)
    {
        rv = ifaceInfo->GetInterfaceIsArgNumberForParam(vtblIndex, &paramInfo, &argnum);
        if(NS_FAILED(rv))
            return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

        const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
        const nsXPTType& arg_type = arg_param.GetType();
        // XXX require iid type here - need to require in compiler too!
        if(!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID)
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, ccx);

        if(!(*result = (nsID*) nsMemory::Clone(dispatchParams[argnum].val.p,
                                               sizeof(nsID))))
            return ReportOutOfMemory(ccx);
    }
    return JS_TRUE;
}

/***************************************************************************/

// static
JSBool
XPCWrappedNative::CallMethod(XPCCallContext& ccx,
                             CallMode mode /*= CALL_METHOD */)
{
    if(!ccx.CanCallNow())
    {
        NS_ASSERTION(0, "hmm? We are finding out about this late!");
        return Throw(NS_ERROR_XPC_UNEXPECTED, ccx);
    }


    // From here on ALL exits are through 'goto done;'


#define PARAM_BUFFER_COUNT     8

    nsXPTCVariant paramBuffer[PARAM_BUFFER_COUNT];
    JSBool retval = JS_FALSE;

    nsXPTCVariant* dispatchParams = nsnull;
    uint8 i;
    const nsXPTMethodInfo* methodInfo;
    uint8 requiredArgs;
    uint8 paramCount;
    jsval src;
    nsresult invokeResult;
    nsID* conditional_iid = nsnull;
    uintN err;
    nsIXPCSecurityManager* sm;
    JSBool foundDependentParam;

    JSContext* cx = ccx.GetJSContext();
    XPCJSRuntime* rt = ccx.GetRuntime();
    nsXPConnect* xpc = ccx.GetXPConnect();
    XPCContext* xpcc = ccx.GetXPCContext();
    nsISupports* callee = ccx.GetTearOff()->GetNative();
    XPCPerThreadData* tls = ccx.GetThreadData();
    uint16 vtblIndex = ccx.GetMethodIndex();
    nsIInterfaceInfo* ifaceInfo = ccx.GetInterface()->GetInterfaceInfo();
    const nsIID& iid = *ccx.GetInterface()->GetIID();
    jsval name = ccx.GetMember()->GetName();
    jsval* argv = ccx.GetArgv();

#ifdef DEBUG_stats_jband
    PRIntervalTime startTime = PR_IntervalNow();
    PRIntervalTime endTime = 0;
    static int totalTime = 0;
    static int count = 0;
    static const int interval = 10;
    if(0 == (++count % interval))
        printf(">>>>>>>> %d calls on XPCWrappedNatives made.  (%d)\n", count, PR_IntervalToMilliseconds(totalTime));
#endif

    ccx.SetRetVal(JSVAL_NULL);

    tls->SetException(nsnull);
    xpcc->SetLastResult(NS_ERROR_UNEXPECTED);

    // set up the method index and do the security check if needed
    switch(mode)
    {
        case CALL_METHOD:
            sm = xpcc->GetAppropriateSecurityManager(
                                nsIXPCSecurityManager::HOOK_CALL_METHOD);
            if(sm && NS_OK != sm->CanCallMethod(cx, iid, callee,
                                                ifaceInfo, vtblIndex, name))
            {
                // the security manager vetoed. It should have set an exception.
                goto done;
            }
            break;

        case CALL_GETTER:
            sm = xpcc->GetAppropriateSecurityManager(
                                nsIXPCSecurityManager::HOOK_GET_PROPERTY);
            if(sm && NS_OK != sm->CanGetProperty(cx, iid, callee,
                                                 ifaceInfo, vtblIndex, name))
            {
                // the security manager vetoed. It should have set an exception.
                goto done;
            }
            break;

        case CALL_SETTER:
            sm = xpcc->GetAppropriateSecurityManager(
                                nsIXPCSecurityManager::HOOK_SET_PROPERTY);
            if(sm && NS_OK != sm->CanSetProperty(cx, iid, callee,
                                                 ifaceInfo, vtblIndex, name))
            {
                // the security manager vetoed. It should have set an exception.
                goto done;
            }
            break;
        default:
            NS_ASSERTION(0,"bad value");
            goto done;
    }

    if(NS_FAILED(ifaceInfo->GetMethodInfo(vtblIndex, &methodInfo)))
    {
        Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, ccx);
        goto done;
    }

    // XXX ASSUMES that retval is last arg.
    paramCount = methodInfo->GetParamCount();
    requiredArgs = paramCount;
    if(paramCount && methodInfo->GetParam(paramCount-1).IsRetval())
        requiredArgs--;
    if(ccx.GetArgc() < requiredArgs)
    {
        Throw(NS_ERROR_XPC_NOT_ENOUGH_ARGS, ccx);
        goto done;
    }

    // setup variant array pointer
    if(paramCount > PARAM_BUFFER_COUNT)
    {
        if(!(dispatchParams = new nsXPTCVariant[paramCount]))
        {
            JS_ReportOutOfMemory(cx);
            goto done;
        }
    }
    else
        dispatchParams = paramBuffer;

    // iterate through the params to clear flags (for safe cleanup later)
    for(i = 0; i < paramCount; i++)
    {
        nsXPTCVariant* dp = &dispatchParams[i];
        dp->ClearFlags();
        dp->val.p = nsnull;
    }

    // Iterate through the params doing conversions of independent params only.
    // When we later convert the dependent params (if any) we will know that
    // the params upon which they depend will have already been converted -
    // regardless of ordering.
    foundDependentParam = JS_FALSE;
    for(i = 0; i < paramCount; i++)
    {
        JSBool useAllocator = JS_FALSE;
        const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
        const nsXPTType& type = paramInfo.GetType();
        uint8 type_tag = type.TagPart();

        if(type.IsDependent())
        {
            foundDependentParam = JS_TRUE;
            continue;
        }

        nsXPTCVariant* dp = &dispatchParams[i];
        dp->type = type;

        if(type_tag == nsXPTType::T_INTERFACE)
        {
            dp->SetValIsInterface();
        }

        // set 'src' to be the object from which we get the value and
        // prepare for out param

        if(paramInfo.IsOut())
        {
            dp->SetPtrIsData();
            dp->ptr = &dp->val;

            if(!paramInfo.IsRetval() &&
               (JSVAL_IS_PRIMITIVE(argv[i]) ||
                !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                                  rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                                  &src)))
            {
                ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, ccx);
                goto done;
            }

            if(type.IsPointer() &&
               type_tag != nsXPTType::T_INTERFACE &&
               !paramInfo.IsShared())
            {
                useAllocator = JS_TRUE;
                dp->SetValIsAllocated();
            }

            if(!paramInfo.IsIn())
                continue;
        }
        else
        {
            if(type.IsPointer())
            {
                switch(type_tag)
                {
                case nsXPTType::T_IID:
                    dp->SetValIsAllocated();
                    useAllocator = JS_TRUE;
                    break;

                case nsXPTType::T_DOMSTRING:
                    dp->SetValIsDOMString();
                    if(paramInfo.IsDipper())
                    {
                        // Is an 'out' DOMString. Make a new nsAWritableString
                        // now and then continue in order to skip the call to
                        // JSData2Native
                        if(!(dp->val.p = new nsString()))
                        {
                            JS_ReportOutOfMemory(cx);
                            goto done;
                        }
                        continue;
                    }
                    // else...

                    // Is an 'in' DOMString. Set 'useAllocator' to indicate
                    // that JSData2Native should allocate a new
                    // nsAReadableString.
                    useAllocator = JS_TRUE;
                    break;
                }
            }

            // Do this *after* the above because in the case where we have a
            // "T_DOMSTRING && IsDipper()" then argv might be null since this
            // is really an 'out' param masquerading as an 'in' param.
            src = argv[i];
        }

        if(type_tag == nsXPTType::T_INTERFACE &&
           NS_FAILED(ifaceInfo->GetIIDForParam(vtblIndex, &paramInfo,
                                               &conditional_iid)))
        {
            ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, i, ccx);
            goto done;
        }

        if(!XPCConvert::JSData2Native(ccx, &dp->val, src, type,
                                      useAllocator, conditional_iid, &err))
        {
            ThrowBadParam(err, i, ccx);
            goto done;
        }

        if(conditional_iid)
        {
            nsMemory::Free((void*)conditional_iid);
            conditional_iid = nsnull;
        }
    }

    // if any params were dependent, then we must iterate again to convert them.
    if(foundDependentParam)
    {
        for(i = 0; i < paramCount; i++)
        {
            const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
            const nsXPTType& type = paramInfo.GetType();

            if(!type.IsDependent())
                continue;

            nsXPTType datum_type;
            JSUint32 array_count;
            JSUint32 array_capacity;
            JSBool useAllocator = JS_FALSE;
            PRBool isArray = type.IsArray();

            PRBool isSizedString = isArray ?
                    JS_FALSE :
                    type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                    type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

            nsXPTCVariant* dp = &dispatchParams[i];
            dp->type = type;

            if(isArray)
            {
                dp->SetValIsArray();

                if(NS_FAILED(ifaceInfo->GetTypeForParam(vtblIndex, &paramInfo, 1,
                                                    &datum_type)))
                {
                    Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);
                    goto done;
                }
            }
            else
                datum_type = type;

            if(datum_type.IsInterfacePointer())
            {
                dp->SetValIsInterface();
            }

            // set 'src' to be the object from which we get the value and
            // prepare for out param

            if(paramInfo.IsOut())
            {
                dp->SetPtrIsData();
                dp->ptr = &dp->val;

                if(!paramInfo.IsRetval() &&
                   (JSVAL_IS_PRIMITIVE(argv[i]) ||
                    !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE), &src)))
                {
                    ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, ccx);
                    goto done;
                }

                if(datum_type.IsPointer() &&
                   !datum_type.IsInterfacePointer() &&
                   (isArray || !paramInfo.IsShared()))
                {
                    useAllocator = JS_TRUE;
                    dp->SetValIsAllocated();
                }

                if(!paramInfo.IsIn())
                    continue;
            }
            else
            {
                src = argv[i];

                if(datum_type.IsPointer() &&
                   datum_type.TagPart() == nsXPTType::T_IID)
                {
                    useAllocator = JS_TRUE;
                    dp->SetValIsAllocated();
                }
            }

            if(datum_type.IsInterfacePointer() &&
               !GetInterfaceTypeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, datum_type,
                                          dispatchParams, &conditional_iid))
                goto done;

            if(isArray || isSizedString)
            {
                if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, GET_SIZE,
                                          dispatchParams, &array_capacity)||
                   !GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, GET_LENGTH,
                                          dispatchParams, &array_count))
                    goto done;

                if(isArray)
                {
                    if(!XPCConvert::JSArray2Native(ccx, (void**)&dp->val, src,
                                                   array_count, array_capacity,
                                                   datum_type,
                                                   useAllocator,
                                                   conditional_iid, &err))
                    {
                        // XXX need exception scheme for arrays to indicate bad element
                        ThrowBadParam(err, i, ccx);
                        goto done;
                    }
                }
                else // if(isSizedString)
                {
                    if(!XPCConvert::JSStringWithSize2Native(ccx,
                                                   (void*)&dp->val,
                                                   src,
                                                   array_count, array_capacity,
                                                   datum_type, useAllocator,
                                                   &err))
                    {
                        ThrowBadParam(err, i, ccx);
                        goto done;
                    }
                }
            }
            else
            {
                if(!XPCConvert::JSData2Native(ccx, &dp->val, src, type,
                                              useAllocator, conditional_iid,
                                              &err))
                {
                    ThrowBadParam(err, i, ccx);
                    goto done;
                }
            }

            if(conditional_iid)
            {
                nsMemory::Free((void*)conditional_iid);
                conditional_iid = nsnull;
            }
        }
    }


    {
        // avoid deadlock in case the native method blocks somehow
        AutoJSSuspendRequest req(ccx);  // scoped suspend of request

        // do the invoke
        invokeResult = XPTC_InvokeByIndex(callee, vtblIndex,
                                          paramCount, dispatchParams);
        // resume non-blocking JS operations now
    }


    xpcc->SetLastResult(invokeResult);

    if(NS_FAILED(invokeResult))
    {
        ThrowBadResult(invokeResult, ccx);
        goto done;
    }
    else if(ccx.GetExceptionWasThrown())
    {
        // the native callee claims to have already set a JSException
        goto done;
    }

    // now we iterate through the native params to gather and convert results
    for(i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
        if(!paramInfo.IsOut() && !paramInfo.IsDipper())
            continue;

        const nsXPTType& type = paramInfo.GetType();
        nsXPTCVariant* dp = &dispatchParams[i];
        jsval v;
        JSUint32 array_count;
        nsXPTType datum_type;
        PRBool isArray = type.IsArray();
        PRBool isSizedString = isArray ?
                JS_FALSE :
                type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

        if(isArray)
        {
            if(NS_FAILED(ifaceInfo->GetTypeForParam(vtblIndex, &paramInfo, 1,
                                                &datum_type)))
            {
                Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);
                goto done;
            }
        }
        else
            datum_type = type;

        if(isArray || isSizedString)
        {
            if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                      vtblIndex, i, GET_LENGTH, dispatchParams,
                                      &array_count))
                goto done;
        }

        if(datum_type.IsInterfacePointer() &&
           !GetInterfaceTypeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                      vtblIndex, i, datum_type, dispatchParams,
                                      &conditional_iid))
            goto done;

        if(isArray)
        {
            if(!XPCConvert::NativeArray2JS(ccx, &v, (const void**)&dp->val,
                                           datum_type, conditional_iid,
                                           array_count, ccx.GetCurrentJSObject(),
                                           &err))
            {
                // XXX need exception scheme for arrays to indicate bad element
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }
        else if (isSizedString)
        {
            if(!XPCConvert::NativeStringWithSize2JS(ccx, &v,
                                           (const void*)&dp->val,
                                           datum_type,
                                           array_count, &err))
            {
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }
        else
        {
            if(!XPCConvert::NativeData2JS(ccx, &v, &dp->val, datum_type,
                                          conditional_iid,
                                          ccx.GetCurrentJSObject(), &err))
            {
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }

        if(paramInfo.IsRetval())
        {
            if(!ccx.GetReturnValueWasSet())
                ccx.SetRetVal(v);
        }
        else
        {
            // we actually assured this before doing the invoke
            NS_ASSERTION(JSVAL_IS_OBJECT(argv[i]), "out var is not object");
            if(!OBJ_SET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE), &v))
            {
                ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, i, ccx);
                goto done;
            }
        }
        if(conditional_iid)
        {
            nsMemory::Free((void*)conditional_iid);
            conditional_iid = nsnull;
        }
    }

    retval = JS_TRUE;
done:
    // iterate through the params (again!) and clean up
    // any alloc'd stuff and release wrappers of params
    if(dispatchParams)
    {
        for(i = 0; i < paramCount; i++)
        {
            nsXPTCVariant* dp = &dispatchParams[i];
            void* p = dp->val.p;
            if(!p)
                continue;

            if(dp->IsValArray())
            {
                // going to have to cleanup the array and perhaps its contents
                if(dp->IsValAllocated() || dp->IsValInterface())
                {
                    // we need to figure out how many elements are present.
                    JSUint32 array_count;

                    const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
                    if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo,
                                              paramInfo, vtblIndex,
                                              i, GET_LENGTH, dispatchParams,
                                              &array_count))
                    {
                        NS_ASSERTION(0,"failed to get array length, we'll leak here");
                        continue;
                    }
                    if(dp->IsValAllocated())
                    {
                        void** a = (void**)p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            void* o = a[k];
                            if(o) nsMemory::Free(o);
                        }
                    }
                    else // if(dp->IsValInterface())
                    {
                        nsISupports** a = (nsISupports**)p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            nsISupports* o = a[k];
                            NS_IF_RELEASE(o);
                        }
                    }
                }
                // always free the array itself
                nsMemory::Free(p);
            }
            else if(dp->IsValAllocated())
                nsMemory::Free(p);
            else if(dp->IsValInterface())
                ((nsISupports*)p)->Release();
            else if(dp->IsValDOMString())
                delete (nsAReadableString*)p;
        }
    }

    if(conditional_iid)
        nsMemory::Free((void*)conditional_iid);

    if(dispatchParams && dispatchParams != paramBuffer)
        delete [] dispatchParams;

#ifdef DEBUG_stats_jband
    endTime = PR_IntervalNow();

    printf("%s::%s %d ( js->c ) \n", GetInterfaceName(), GetMemberName(desc), PR_IntervalToMilliseconds(endTime-startTime));

    totalTime += (endTime-startTime);
#endif

    return retval;
}

/***************************************************************************/
// interface methods

/* readonly attribute JSObjectPtr JSObject; */
NS_IMETHODIMP XPCWrappedNative::GetJSObject(JSObject * *aJSObject)
{
    *aJSObject = mFlatJSObject;
    return NS_OK;
}

/* readonly attribute nsISupports Native; */
NS_IMETHODIMP XPCWrappedNative::GetNative(nsISupports * *aNative)
{
    return mIdentity->QueryInterface(NS_GET_IID(nsISupports), (void**)aNative);
}

/* readonly attribute JSObjectPtr JSObjectPrototype; */
NS_IMETHODIMP XPCWrappedNative::GetJSObjectPrototype(JSObject * *aJSObjectPrototype)
{
    *aJSObjectPrototype = mProto->GetJSProtoObject();
    return NS_OK;
}

/* readonly attribute nsIXPConnect XPConnect; */
NS_IMETHODIMP XPCWrappedNative::GetXPConnect(nsIXPConnect * *aXPConnect)
{
    if(IsValid())
    {
        nsIXPConnect* temp = GetProto()->GetScope()->GetRuntime()->GetXPConnect();
        NS_IF_ADDREF(temp);
        *aXPConnect = temp;
    }
    else
        *aXPConnect = nsnull;
    return NS_OK;
}

/* XPCNativeInterface FindInterfaceWithMember (in JSVal name); */
NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithMember(jsval name, nsIXPCNativeInterface * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* XPCNativeInterface FindInterfaceWithName (in JSVal name); */
NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithName(jsval name, nsIXPCNativeInterface * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void debugDump (in short depth); */
NS_IMETHODIMP XPCWrappedNative::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNative @ %x with mRefCnt = %d", this, mRefCnt));
    XPC_LOG_INDENT();

        if(depth && mProto)
            mProto->DebugDump(depth);
        else
            XPC_LOG_ALWAYS(("mProto @ %x", mProto));
        
        if(depth && mSet)
            mSet->DebugDump(depth);
        else
            XPC_LOG_ALWAYS(("mSet @ %x", mSet));
        
        XPC_LOG_ALWAYS(("mFlatJSObject of %x", mFlatJSObject));
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
    return NS_OK;
}

/***************************************************************************/
/***************************************************************************/
