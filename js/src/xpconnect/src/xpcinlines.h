/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/* private inline methods (#include'd by xpcprivate.h). */

#ifndef xpcinlines_h___
#define xpcinlines_h___

/***************************************************************************/

inline JSBool
XPCCallContext::IsValid() const
{
    return mState != INIT_FAILED;
}

inline nsXPConnect*
XPCCallContext::GetXPConnect() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mXPC;
}

inline XPCJSRuntime*
XPCCallContext::GetRuntime() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mXPCContext->GetRuntime();
}

inline XPCPerThreadData*
XPCCallContext::GetThreadData() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mThreadData;
}

inline XPCContext*
XPCCallContext::GetXPCContext() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mXPCContext;
}

inline JSContext*
XPCCallContext::GetJSContext() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mJSContext;
}

inline JSContext*
XPCCallContext::GetSafeJSContext() const
{
    CHECK_STATE(HAVE_CONTEXT);
    JSContext* cx;
    if(NS_SUCCEEDED(mThreadData->GetJSContextStack()->GetSafeJSContext(&cx)))
        return cx;
    return nsnull;
}

inline JSBool
XPCCallContext::GetContextPopRequired() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mContextPopRequired;
}

inline XPCContext::LangType
XPCCallContext::GetCallerLanguage() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mCallerLanguage;
}

inline XPCContext::LangType
XPCCallContext::GetPrevCallerLanguage() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mPrevCallerLanguage;
}

inline XPCCallContext*
XPCCallContext::GetPrevCallContext() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mPrevCallContext;
}

inline JSObject*
XPCCallContext::GetOperandJSObject() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mOperandJSObject;
}

inline JSObject*
XPCCallContext::GetCurrentJSObject() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mCurrentJSObject;
}

inline JSObject*
XPCCallContext::GetFlattenedJSObject() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mFlattenedJSObject;
}

inline nsISupports*
XPCCallContext::GetIdentityObject() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mWrapper->GetIdentityObject();
}

inline XPCWrappedNative*
XPCCallContext::GetWrapper() const
{
    if(mState == INIT_FAILED)
        return nsnull;

    CHECK_STATE(HAVE_OBJECT);
    return mWrapper;
}

inline XPCWrappedNativeTearOff*
XPCCallContext::GetTearOff() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mTearOff;
}

inline XPCNativeScriptableInfo*
XPCCallContext::GetScriptableInfo() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mScriptableInfo;
}

inline XPCNativeSet*
XPCCallContext::GetSet() const
{
    CHECK_STATE(HAVE_JSID);
    return mSet;
}

inline XPCNativeInterface*
XPCCallContext::GetInterface() const
{
    CHECK_STATE(HAVE_JSID);
    return mInterface;
}

inline XPCNativeMember*
XPCCallContext::GetMember() const
{
    CHECK_STATE(HAVE_JSID);
    return mMember;
}

inline JSBool
XPCCallContext::HasInterfaceAndMember() const
{
    return mState >= HAVE_JSID && mInterface && mMember;
}

inline jsid
XPCCallContext::GetJSID() const
{
    CHECK_STATE(HAVE_JSID);
    return mJSID;
}

inline JSBool
XPCCallContext::GetStaticMemberIsLocal() const
{
    CHECK_STATE(HAVE_JSID);
    return mStaticMemberIsLocal;
}

inline uintN
XPCCallContext::GetArgc() const
{
    CHECK_STATE(READY_TO_CALL);
    return mArgc;
}

inline jsval*
XPCCallContext::GetArgv() const
{
    CHECK_STATE(READY_TO_CALL);
    return mArgv;
}

inline jsval*
XPCCallContext::GetRetVal() const
{
    CHECK_STATE(READY_TO_CALL);
    return mRetVal;
}

inline JSBool
XPCCallContext::GetExceptionWasThrown() const
{
    CHECK_STATE(READY_TO_CALL);
    return mExceptionWasThrown;
}

inline JSBool
XPCCallContext::GetReturnValueWasSet() const
{
    CHECK_STATE(READY_TO_CALL);
    return mReturnValueWasSet;
}

inline void
XPCCallContext::SetRetVal(jsval val)
{
    CHECK_STATE(HAVE_ARGS);
    if(mRetVal)
        *mRetVal = val;
}

inline jsword
XPCCallContext::GetHackyResolveBugID() const
{
    CHECK_STATE(HAVE_CONTEXT);
    return mThreadData->GetHackyResolveBugID();
}

inline jsword
XPCCallContext::SetHackyResolveBugID(jsword id)
{
    CHECK_STATE(HAVE_CONTEXT);
    return mThreadData->SetHackyResolveBugID(id);
}

inline XPCWrappedNative* 
XPCCallContext::GetResolvingWrapper() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mThreadData->GetResolvingWrapper();
}

inline XPCWrappedNative* 
XPCCallContext::SetResolvingWrapper(XPCWrappedNative* w)
{
    CHECK_STATE(HAVE_OBJECT);
    return mThreadData->SetResolvingWrapper(w);
}

inline PRUint16
XPCCallContext::GetMethodIndex() const
{
    CHECK_STATE(HAVE_OBJECT);
    return mMethodIndex;
}

inline void
XPCCallContext::SetMethodIndex(PRUint16 index)
{
    CHECK_STATE(HAVE_OBJECT);
    mMethodIndex = index;
}

/***************************************************************************/

inline const nsIID* XPCNativeInterface::GetIID() const
{
    const nsIID* iid;
    return NS_SUCCEEDED(mInfo->GetIIDShared(&iid)) ? iid : nsnull;
}

inline const char* XPCNativeInterface::GetName() const
{
    const char* name;
    return NS_SUCCEEDED(mInfo->GetNameShared(&name)) ? name : nsnull;
}

inline XPCNativeMember* XPCNativeInterface::FindMember(jsid id) const
{
    int count = (int) mMemberCount;
    for(int i = 0; i < count; i++)
        if(mMembers[i].GetID() == id)
            return (XPCNativeMember*) &mMembers[i];
    return nsnull;
}

/***************************************************************************/

inline JSBool
XPCNativeSet::FindMember(jsid id, XPCNativeMember** pMember,
                         PRUint16* pInterfaceIndex) const
{
    int count = (int) mInterfaceCount;
    int i;

    // look for interface names first

    for(i = 0; i < count; i++)
    {
        if(id == mInterfaces[i]->GetNameID())
        {
            *pMember = nsnull;
            *pInterfaceIndex = (PRUint16) i;
            return JS_TRUE;
        }
    }

    // look for method names
    for(i = 0; i < count; i++)
    {
        XPCNativeMember* member = mInterfaces[i]->FindMember(id);
        if(member)
        {
            *pMember = member;
            *pInterfaceIndex = (PRUint16) i;
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

inline JSBool
XPCNativeSet::FindMember(jsid id, XPCNativeMember** pMember,
                         XPCNativeInterface** pInterface) const
{
    PRUint16 index;
    if(!FindMember(id, pMember, &index))
        return JS_FALSE;
    *pInterface = mInterfaces[index];
    return JS_TRUE;
}

inline JSBool XPCNativeSet::HasInterface(XPCNativeInterface* aInterface) const
{
    int count = (int) mInterfaceCount;
    for(int i = 0; i < count; i++)
        if(aInterface == mInterfaces[i])
            return JS_TRUE;
    return JS_FALSE;
}

/***************************************************************************/

inline XPCWrappedNative*
XPCWrappedNativeTearOff::GetPrivateWrapper() const
{
    return NS_REINTERPRET_CAST(XPCWrappedNative*, mWrapper);
}

inline XPCNativeInterface*
XPCWrappedNativeTearOff::GetPrivateInterface() const
{
    return NS_REINTERPRET_CAST(XPCNativeInterface*, mInterface);
}

inline void
XPCWrappedNativeTearOff::SetWrapper(XPCWrappedNative*  Wrapper)
{
    mWrapper = Wrapper;
}

inline void
XPCWrappedNativeTearOff::SetJSObject(JSObject*  JSObject)
{
    mJSObject = JSObject;
}

inline void
XPCWrappedNativeTearOff::SetNative(nsISupports*  Native)
{
    mNative = Native;
}

inline void
XPCWrappedNativeTearOff::SetInterface(XPCNativeInterface*  Interface)
{
    mInterface = Interface;
}

/***************************************************************************/

inline JSBool 
XPCWrappedNativeTearOffChunk::HasInterfaceNoQI(XPCNativeInterface* aInterface)
{
    // XXX locking !
 
    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = this; chunk; chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            if(to->GetInterface() == aInterface)
                return JS_TRUE;
        }
    }
    return JS_FALSE;
}

inline JSBool 
XPCWrappedNativeTearOffChunk::HasInterfaceNoQI(const nsIID& iid)
{
    // XXX locking !
 
    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = this; chunk; chunk = chunk->mNextChunk)
    {
        const XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            XPCNativeInterface* iface = to->GetPrivateInterface();
            if(iface)
            {
                const nsIID* curIID = iface->GetIID();
                if(curIID && curIID->Equals(iid))
                    return JS_TRUE;
            }
        }
    }
    return JS_FALSE;
}

inline XPCWrappedNativeTearOff*
XPCWrappedNativeTearOffChunk::FindTearOff(XPCCallContext& ccx,
                                          XPCNativeInterface* aInterface,
                                          XPCWrappedNative* aWrappedNative)
{
    // XXX locking !
    XPCWrappedNativeTearOff* firstAvailable = nsnull;

    XPCWrappedNativeTearOffChunk* lastChunk;
    XPCWrappedNativeTearOffChunk* chunk;
    for(lastChunk = chunk = this;
        chunk;
        lastChunk = chunk, chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            if(to->GetInterface() == aInterface)
                return to;
            if(!to->GetInterface() && !firstAvailable)
                firstAvailable = to;
        }
    }

    // We need to determine if the object really does this interface...

    const nsIID* iid = aInterface->GetIID();
    nsISupports* identity = aWrappedNative->GetIdentityObject();
    nsISupports* obj;

    identity->QueryInterface(*iid, (void**)&obj);
    if(!obj)
        return nsnull;

    if(!aWrappedNative->GetSet()->HasInterface(aInterface) &&
       !aWrappedNative->ExtendSet(ccx, aInterface))
        return nsnull;

    if(!firstAvailable)
    {
        XPCWrappedNativeTearOffChunk* newChunk =
            new XPCWrappedNativeTearOffChunk();
        if(!newChunk)
        {
            NS_RELEASE(obj);
            return nsnull;
        }
        lastChunk->mNextChunk = newChunk;
        firstAvailable = chunk->mTearOffs;
    }

    firstAvailable->SetWrapper(aWrappedNative);
    firstAvailable->SetInterface(aInterface);
    firstAvailable->SetNative(obj);

    return firstAvailable;
}

/***************************************************************************/

#endif /* xpcinlines_h___ */
