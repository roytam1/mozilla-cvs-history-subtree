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


XPCCallContext::XPCCallContext(XPCContext::LangType callerLanguage,
                               JSContext* cx   /* = nsnull  */,
                               JSObject* obj   /* = nsnull  */,
                               jsval name      /* = 0       */,
                               uintN argc      /* = NO_ARGS */,
                               jsval *argv     /* = nsnull  */,
                               jsval *rval     /* = nsnull  */)
    :   mState(INIT_FAILED),
        mXPC(dont_AddRef(nsXPConnect::GetXPConnect())),
        mThreadData(nsnull),
        mXPCContext(nsnull),
        mJSContext(cx),
        mContextPopRequired(JS_FALSE),
        mCallerLanguage(callerLanguage)
{
    NS_INIT_ISUPPORTS();

    if(!mXPC)
        return;

    if(!(mThreadData = XPCPerThreadData::GetData()))
        return;

    XPCJSContextStack* stack = mThreadData->GetJSContextStack();
    JSContext* topJSContext;

    if(!stack || NS_FAILED(stack->Peek(&topJSContext)))
    {
        NS_ASSERTION(0, "bad!");
        return;
    }

    if(!mJSContext)
    {
        if(NS_FAILED(stack->GetSafeJSContext(&mJSContext)) || !mJSContext)
            return;
    }

    // Get into the request as early as we can to avoid problems with scanning
    // callcontexts on other threads from within the gc callbacks.

    if(mCallerLanguage == NATIVE_CALLER && JS_GetContextThread(mJSContext))
        JS_BeginRequest(mJSContext);

    if(topJSContext != mJSContext)
    {
        if(NS_FAILED(stack->Push(mJSContext)))
        {
            NS_ASSERTION(0, "bad!");
            return;
        }
        mContextPopRequired = JS_TRUE;
    }

    if(!(mXPCContext = nsXPConnect::GetContext(mJSContext, mXPC)))
        return;
    // XXX we might hook into a list on the contexts???

    mPrevCallerLanguage = mXPCContext->SetCallingLangType(mCallerLanguage);

    // hook into call context chain for our thread
    mPrevCallContext = mThreadData->SetCallContext(this);

    mState = HAVE_CONTEXT;

    if(!obj)
        return;

    mMethodIndex = 0xDEAD;
    mOperandJSObject = obj;

    mState = HAVE_OBJECT;

    mTearOff = nsnull;
    mWrapper = XPCWrappedNative::GetWrappedNativeOfJSObject(mJSContext, obj,
                                                            &mCurrentJSObject,
                                                            &mTearOff);
    if(!mWrapper)
        return;

    mFlattenedJSObject = mWrapper->GetFlatJSObject();

    if(mTearOff)
        mScriptableInfo = nsnull;
    else
        mScriptableInfo = mWrapper->GetScriptableInfo();

    if(name)
        SetName(name);

    if(argc != NO_ARGS)
        SetArgsAndResultPtr(argc, argv, rval);

    CHECK_STATE(HAVE_OBJECT);
}

void
XPCCallContext::SetName(jsval name)
{
    CHECK_STATE(HAVE_OBJECT);

    mName = name;

    if(mTearOff)
    {
        mSet = nsnull;
        mInterface = mTearOff->GetInterface();
        mMember = mInterface->FindMember(name);
        mStaticMemberIsLocal = JS_TRUE;
        if(mMember && !mMember->IsConstant())
            mMethodIndex = mMember->GetIndex();
    }
    else
    {
        mSet = mWrapper->GetSet();

        if(mSet->FindMember(name, &mMember, &mInterface))
        {
            if(mMember && !mMember->IsConstant())
                mMethodIndex = mMember->GetIndex();
    
            PRUint16 ignored;
            XPCNativeMember* protoMember;
            XPCNativeSet* mProtoSet = mWrapper->GetProto()->GetSet();

            mStaticMemberIsLocal =
                    !mMember ||
                     (mProtoSet != mSet &&
                      (!mProtoSet->FindMember(name, &protoMember, &ignored) ||
                       protoMember != mMember));
        }
        else
        {
            // XXX do we need to lookup tearoff names here?

            mMember = nsnull;
            mInterface = nsnull;
            mStaticMemberIsLocal = JS_FALSE;
        }
    }

    mState = HAVE_NAME;
}

void
XPCCallContext::SetCallableInfo(XPCCallableInfo* ci, JSBool isSetter)
{
    // We are going straight to the method info and need not do a lookup
    // by id.

    // don't be tricked if method is called with wrong 'this'
    if(mTearOff && mTearOff->GetInterface() != ci->GetInterface())
        mTearOff = nsnull;

    mSet = nsnull;
    mInterface = ci->GetInterface();
    mMember = ci->GetMember();
    mMethodIndex = mMember->GetIndex() + (isSetter ? 1 : 0);
    mName = mMember->GetName();

    if(mState < HAVE_NAME)
        mState = HAVE_NAME;
}

void
XPCCallContext::SetArgsAndResultPtr(uintN argc,
                                    jsval *argv,
                                    jsval *rval)
{
    CHECK_STATE(HAVE_OBJECT);

    mArgc   = argc;
    mArgv   = argv;
    mRetVal = rval;

    mExceptionWasThrown = mReturnValueWasSet = JS_FALSE;
    mState = HAVE_ARGS;
}

JSBool
XPCCallContext::CanCallNow()
{
    if(!HasInterfaceAndMember())
        return JS_FALSE;
    if(mState < HAVE_ARGS)
        return JS_FALSE;

    if(!mTearOff)
        mTearOff = mWrapper->FindTearOff(*this, mInterface);

    if(mTearOff)
    {
        // Refresh in case FindTearOff extended the set
        mSet = mWrapper->GetSet();
        
        mState = READY_TO_CALL;
        return JS_TRUE;
    }
    return JS_FALSE;
}

void
XPCCallContext::SystemIsBeingShutDown()
{
    // XXX This is pretty questionable since the per thread cleanup stuff
    // can be making this call on one thread for call contexts on another
    // thread.
    NS_WARNING("XPConnect even through there is a live XPCCallContext");
    mThreadData = nsnull;
    mXPCContext = nsnull;
    mState = SYSTEM_SHUTDOWN;
    if(mPrevCallContext)
        mPrevCallContext->SystemIsBeingShutDown();
}

XPCCallContext::~XPCCallContext()
{
    NS_ASSERTION(mRefCnt == 0, "Someone is holding a bad reference to a XPCCallContext");

    // do cleanup...

    if(mXPCContext)
    {
        mXPCContext->SetCallingLangType(mPrevCallerLanguage);

        if(mContextPopRequired)
        {
            XPCJSContextStack* stack = mThreadData->GetJSContextStack();
            if(stack)
            {
#ifdef DEBUG
                JSContext* poppedCX;
                nsresult rv = stack->Pop(&poppedCX);
                NS_ASSERTION(NS_SUCCEEDED(rv) && poppedCX == mJSContext, "bad pop");
#else
                (void) stack->Pop(nsnull);
#endif
            }
            else
            {
                NS_ASSERTION(0, "bad!");
            }
        }

        if(mCallerLanguage == NATIVE_CALLER && JS_GetContextThread(mJSContext))
            JS_EndRequest(mJSContext);
    }

    if(mThreadData)
    {
#ifdef DEBUG
        XPCCallContext* old = mThreadData->SetCallContext(mPrevCallContext);
        NS_ASSERTION(old == this, "bad pop from per thread data");
#else
        (void) mThreadData->SetCallContext(mPrevCallContext);
#endif
    }
}

NS_IMPL_QUERY_INTERFACE1(XPCCallContext, nsIXPCNativeCallContext)
NS_IMPL_ADDREF(XPCCallContext)

NS_IMETHODIMP_(nsrefcnt)
XPCCallContext::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(_class);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "XPCCallContext");
  // no delete this!
  return mRefCnt;
}

/* readonly attribute nsISupports Callee; */
NS_IMETHODIMP
XPCCallContext::GetCallee(nsISupports * *aCallee)
{
    nsISupports* temp = mWrapper->GetIdentityObject();
    NS_IF_ADDREF(temp);
    *aCallee = temp;
    return NS_OK;
}

/* readonly attribute PRUint16 CalleeMethodIndex; */
NS_IMETHODIMP
XPCCallContext::GetCalleeMethodIndex(PRUint16 *aCalleeMethodIndex)
{
    *aCalleeMethodIndex = mMethodIndex;
    return NS_OK;
}

/* readonly attribute nsIXPConnectWrappedNative CalleeWrapper; */
NS_IMETHODIMP
XPCCallContext::GetCalleeWrapper(nsIXPConnectWrappedNative * *aCalleeWrapper)
{
    nsIXPConnectWrappedNative* temp = mWrapper;
    NS_IF_ADDREF(temp);
    *aCalleeWrapper = temp;
    return NS_OK;
}

/* readonly attribute XPCNativeInterface CalleeInterface; */
NS_IMETHODIMP 
XPCCallContext::GetCalleeInterface(nsIXPCNativeInterface * *aCalleeInterface)
{
    *aCalleeInterface = mInterface;
    return NS_OK;
}

/* readonly attribute JSContextPtr JSContext; */
NS_IMETHODIMP
XPCCallContext::GetJSContext(JSContext * *aJSContext)
{
    *aJSContext = mJSContext;
    return NS_OK;
}

/* readonly attribute PRUint32 Argc; */
NS_IMETHODIMP
XPCCallContext::GetArgc(PRUint32 *aArgc)
{
    *aArgc = (PRUint32) mArgc;
    return NS_OK;
}

/* readonly attribute JSValPtr ArgvPtr; */
NS_IMETHODIMP
XPCCallContext::GetArgvPtr(jsval * *aArgvPtr)
{
    *aArgvPtr = mArgv;
    return NS_OK;
}

/* readonly attribute JSValPtr RetValPtr; */
NS_IMETHODIMP
XPCCallContext::GetRetValPtr(jsval * *aRetValPtr)
{
    *aRetValPtr = mRetVal;
    return NS_OK;
}

/* attribute PRBool ExceptionWasThrown; */
NS_IMETHODIMP
XPCCallContext::GetExceptionWasThrown(PRBool *aExceptionWasThrown)
{
    *aExceptionWasThrown = mExceptionWasThrown;
    return NS_OK;
}
NS_IMETHODIMP
XPCCallContext::SetExceptionWasThrown(PRBool aExceptionWasThrown)
{
    mExceptionWasThrown = aExceptionWasThrown;
    return NS_OK;
}

/* attribute PRBool ReturnValueWasSet; */
NS_IMETHODIMP
XPCCallContext::GetReturnValueWasSet(PRBool *aReturnValueWasSet)
{
    *aReturnValueWasSet = mReturnValueWasSet;
    return NS_OK;
}
NS_IMETHODIMP
XPCCallContext::SetReturnValueWasSet(PRBool aReturnValueWasSet)
{
    mReturnValueWasSet = aReturnValueWasSet;
    return NS_OK;
}

