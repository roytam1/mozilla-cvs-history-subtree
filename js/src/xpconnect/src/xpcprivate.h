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

/* All the XPConnect private declarations - only include locally. */

#ifndef xpcprivate_h___
#define xpcprivate_h___

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "nscore.h"
#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsIClassInfo.h"
#include "nsIComponentManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIGenericFactory.h"
#include "nsMemory.h"
#include "nsIXPConnect.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "nsIXPCSecurityManager.h"
#include "nsIJSRuntimeService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsAutoLock.h"
#include "xptcall.h"
#include "jsapi.h"
#include "jshash.h"
#include "jsprf.h"
#include "prprf.h"
#include "jsinterp.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "xptinfo.h"
#include "xpcforwards.h"
#include "xpclog.h"
#include "xpccomponents.h"
#include "xpcexception.h"
#include "xpcjsid.h"
#include "prlong.h"
#include "prmem.h"
#include "prenv.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"

#include "nsIThread.h"
#include "nsIJSContextStack.h"
#include "prthread.h"
#include "nsDeque.h"
#include "nsVoidArray.h"

#include "nsIConsoleService.h"
#include "nsIScriptError.h"

#ifndef XPCONNECT_STANDALONE
#include "nsIScriptContext.h"
#define XPC_USE_SECURITY_CHECKED_COMPONENT
#endif

#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
#include "nsISecurityCheckedComponent.h"
#endif

// #define XPC_OLD_DOM_SUPPORT

#ifdef XPC_OLD_DOM_SUPPORT
#include "nsIScriptObjectOwner.h"   // for DOM hack in xpcconvert.cpp
#include "nsIScriptGlobalObject.h"
#endif

#ifdef XPC_TOOLS_SUPPORT
#include "nsIXPCToolsProfiler.h"
#endif

/***************************************************************************/
// compile time switches for instrumentation and stuff....

#ifdef DEBUG
#define XPC_DETECT_LEADING_UPPERCASE_ACCESS_ERRORS
#endif

#ifdef DEBUG
#define XPC_REPORT_SHADOWED_WRAPPED_NATIVE_MEMBERS
#endif

#ifdef DEBUG
#define XPC_CHECK_WRAPPER_THREADSAFETY
#endif

#if defined(DEBUG_jst) || defined(DEBUG_jband)
#define XPC_CHECK_CLASSINFO_CLAIMS
#if defined(DEBUG_jst)
#define XPC_ASSERT_CLASSINFO_CLAIMS
#endif
#endif

#if defined(DEBUG_jband) || defined(DEBUG_jst)
#define XPC_DUMP_AT_SHUTDOWN
#define XPC_CHECK_WRAPPERS_AT_SHUTDOWN
//#define DEBUG_stats_jband 1
//#define XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
#endif

/***************************************************************************/
// conditional forward declarations....

#ifdef XPC_REPORT_SHADOWED_WRAPPED_NATIVE_MEMBERS
void DEBUG_ReportShadowedMembers(XPCNativeSet* set,
                                 XPCWrappedNativeProto* proto);
#else
#define DEBUG_ReportShadowedMembers(set, proto) ((void)0)
#endif

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
void DEBUG_ReportWrapperThreadSafetyError(XPCCallContext& ccx,
                                          const char* msg,
                                          const XPCWrappedNative* wrapper); 
void DEBUG_CheckWrapperThreadSafety(const XPCWrappedNative* wrapper);
#else
#define DEBUG_CheckWrapperThreadSafety(w) ((void)0)
#endif

/***************************************************************************/

// Defeat possible Windows macro-mangling of the name
#ifdef GetClassInfo
#undef GetClassInfo
#endif

// To kill #define index(a,b) strchr(a,b) macro in Toolkit types.h
#ifdef XP_OS2_VACPP
#ifdef index
#undef index
#endif
#endif

/***************************************************************************/
// default initial sizes for maps (hashtables)

#define XPC_CONTEXT_MAP_SIZE             16
#define XPC_JS_MAP_SIZE                  64
#define XPC_JS_CLASS_MAP_SIZE           128

#define XPC_NATIVE_MAP_SIZE              64
#define XPC_NATIVE_PROTO_MAP_SIZE        16
#define XPC_NATIVE_INTERFACE_MAP_SIZE   128
#define XPC_NATIVE_SET_MAP_SIZE         128
#define XPC_THIS_TRANSLATOR_MAP_SIZE      8

/***************************************************************************/
// data declarations...
extern const char XPC_ARG_FORMATTER_FORMAT_STR[]; // format string

/***************************************************************************/
// useful macros...

#define XPC_STRING_GETTER_BODY(dest, src) \
    NS_ENSURE_ARG_POINTER(dest); \
    char* result; \
    if(src) \
        result = (char*) nsMemory::Clone(src, \
                                sizeof(char)*(strlen(src)+1)); \
    else \
        result = nsnull; \
    *dest = result; \
    return (result || !src) ? NS_OK : NS_ERROR_OUT_OF_MEMORY

/***************************************************************************/

// We PROMISE to never screw this up.
#ifdef WIN32
#pragma warning(disable : 4355) // OK to pass "this" in member initializer
#endif

typedef PRMonitor XPCLock; 

// This is a cloned subset of nsAutoMonitor. We want the use of a monitor -
// mostly because we need reenterability - but we also want to support passing
// a null monitor in without things blowing up. This is used for wrappers that
// are gaurenteeded to be used only on one thread. We avoid lock overhead by
// using a null monitor. By changing this class we can avoid having multiplte
// code paths or (conditional) manual calls to PR_{Enter,Exit}Monitor.
//
// Note that xpconnect only makes *one* monitor and *mostly* holds it locked 
// only through very small critical sections.

class XPCAutoLock : public nsAutoLockBase {
public:

    static XPCLock* NewLock(const char* name) 
                        {return nsAutoMonitor::NewMonitor(name);}
    static void       DestroyLock(XPCLock* lock)
                        {nsAutoMonitor::DestroyMonitor(lock);}

    XPCAutoLock(XPCLock* lock)
#ifdef DEBUG
        : nsAutoLockBase(lock ? (void*) lock : (void*) this, eAutoMonitor),
#else
        : nsAutoLockBase(lock, eAutoMonitor),
#endif
          mLock(lock)
    {
        if(mLock)
            PR_EnterMonitor(mLock);
    }

    ~XPCAutoLock()
    {
        if(mLock)
        {
#ifdef DEBUG
            PRStatus status = 
#endif
                PR_ExitMonitor(mLock);
            NS_ASSERTION(status == PR_SUCCESS, "PR_ExitMonitor failed");
        }
    }

private:
    XPCLock*  mLock;

    // Not meant to be implemented. This makes it a compiler error to
    // construct or assign an nsAutoLock object incorrectly.
    XPCAutoLock(void) {}
    XPCAutoLock(XPCAutoLock& /*aMon*/) {}
    XPCAutoLock& operator =(XPCAutoLock& /*aMon*/) {
        return *this;
    }

    // Not meant to be implemented. This makes it a compiler error to
    // attempt to create an nsAutoLock object on the heap.
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
    static void* operator new(size_t /*size*/) throw () {
#else
    static void* operator new(size_t /*size*/) {
#endif
        return nsnull;
    }
    static void operator delete(void* /*memory*/) {}
};

/***************************************************************************/

class nsXPConnect : public nsIXPConnect
{
public:
    // all the interface method declarations...
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECT

    // non-interface implementation
public:
    // These get non-addref'd pointers
    static nsXPConnect* GetXPConnect();
    static XPCJSRuntime* GetRuntime(nsXPConnect* xpc = nsnull);
    static XPCContext*  GetContext(JSContext* cx, nsXPConnect* xpc = nsnull);

    // Gets addref'd pointer
    static nsresult GetInterfaceInfoManager(nsIInterfaceInfoManager** iim,
                                            nsXPConnect* xpc = nsnull);
    
    // Gets addref'd pointer
    static nsresult GetContextStack(nsIThreadJSContextStack** stack,
                                    nsXPConnect* xpc = nsnull);

    static JSBool IsISupportsDescendant(nsIInterfaceInfo* info);

    nsIXPCSecurityManager* GetDefaultSecurityManager() const
        {return mDefaultSecurityManager;}
    PRUint16 GetDefaultSecurityManagerFlags() const
        {return mDefaultSecurityManagerFlags;}

    // This returns and AddRef'd pointer. It does not do this with an out param
    // only because this form  is required by generic module macro:
    // NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR 
    static nsXPConnect* GetSingleton();

    // called by module code on dll shutdown
    static void ReleaseXPConnectSingleton();
    virtual ~nsXPConnect();

    JSBool IsShuttingDown() const {return mShutingDown;}

protected:
    nsXPConnect();

private:
    JSBool EnsureRuntime() {return mRuntime ? JS_TRUE : CreateRuntime();}
    JSBool CreateRuntime();

private:
    // Singleton instance
    static nsXPConnect* gSelf;
    static JSBool gOnceAliveNowDead;

    XPCJSRuntime* mRuntime;
    nsIInterfaceInfoManager* mInterfaceInfoManager;
    nsIThreadJSContextStack* mContextStack;
    nsIXPCSecurityManager* mDefaultSecurityManager;
    PRUint16 mDefaultSecurityManagerFlags;
    JSBool mShutingDown;
#ifdef XPC_TOOLS_SUPPORT
    nsCOMPtr<nsIXPCToolsProfiler> mProfiler;
    nsCOMPtr<nsILocalFile> mProfilerOutputFile;
#endif
};

/***************************************************************************/

// In the current xpconnect system there can only be one XPCJSRuntime.
// So, xpconnect can only be used on one JSRuntime within the process.

// no virtuals. no refcounting.
class XPCJSRuntime
{
public:
    static XPCJSRuntime* newXPCJSRuntime(nsXPConnect* aXPConnect,
                                         nsIJSRuntimeService* aJSRuntimeService);

    JSRuntime*     GetJSRuntime() const {return mJSRuntime;}
    nsXPConnect*   GetXPConnect() const {return mXPConnect;}

    JSObject2WrappedJSMap*     GetWrappedJSMap()        const
        {return mWrappedJSMap;}

    IID2WrappedJSClassMap*     GetWrappedJSClassMap()   const
        {return mWrappedJSClassMap;}

    IID2NativeInterfaceMap* GetIID2NativeInterfaceMap() const
        {return mIID2NativeInterfaceMap;}

    ClassInfo2NativeSetMap* GetClassInfo2NativeSetMap() const
        {return mClassInfo2NativeSetMap;}

    NativeSetMap* GetNativeSetMap() const
        {return mNativeSetMap;}

    IID2ThisTranslatorMap* GetThisTraslatorMap() const
        {return mThisTranslatorMap;}

    XPCLock* GetMapLock() const {return mMapLock;}

    XPCContext* GetXPCContext(JSContext* cx);
    XPCContext* SyncXPCContextList(JSContext* cx = nsnull);

    // Mapping of often used strings to jsid atoms that live 'forever'.
    //
    // To add a new string: add to this list and to XPCJSRuntime::mStrings
    // at the top of xpcjsruntime.cpp
    enum {
        IDX_CONSTRUCTOR             = 0 ,
        IDX_TO_STRING               ,
        IDX_TO_SOURCE               ,
        IDX_LAST_RESULT             ,
        IDX_RETURN_CODE             ,
        IDX_VALUE                   ,
        IDX_QUERY_INTERFACE         ,
        IDX_COMPONENTS              ,
        IDX_WRAPPED_JSOBJECT        ,
        IDX_OBJECT                  ,
        IDX_PROTOTYPE               ,
        IDX_CALLABLE_INFO_PROP_NAME ,
        IDX_CREATE_INSTANCE         ,
        IDX_TOTAL_COUNT // just a count of the above
    };

    jsid GetStringID(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrIDs[index];
    }
    jsval GetStringJSVal(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrJSVals[index];
    }
    const char* GetStringName(uintN index) const
    {
        NS_ASSERTION(index < IDX_TOTAL_COUNT, "index out of range");
        return mStrings[index];
    }

    static JSBool JS_DLL_CALLBACK GCCallback(JSContext *cx, JSGCStatus status);

    void DebugDump(PRInt16 depth);

    ~XPCJSRuntime();

#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
   void DEBUG_AddWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         JS_HashTableAdd(DEBUG_WrappedNativeHashtable, wrapper, wrapper);}

   void DEBUG_RemoveWrappedNative(nsIXPConnectWrappedNative* wrapper)
        {XPCAutoLock lock(GetMapLock());
         JS_HashTableRemove(DEBUG_WrappedNativeHashtable, wrapper);}

private:
   JSHashTable *DEBUG_WrappedNativeHashtable;
public:
#endif

private:
    XPCJSRuntime(); // no implementation
    XPCJSRuntime(nsXPConnect* aXPConnect,
                 nsIJSRuntimeService* aJSRuntimeService);

    JSContext2XPCContextMap*  GetContextMap() const {return mContextMap;}
    JSBool GenerateStringIDs(JSContext* cx);
    void PurgeXPCContextList();

private:
    static const char* mStrings[IDX_TOTAL_COUNT];
    jsid mStrIDs[IDX_TOTAL_COUNT];
    jsval mStrJSVals[IDX_TOTAL_COUNT];

    nsXPConnect* mXPConnect;
    JSRuntime*  mJSRuntime;
    nsIJSRuntimeService* mJSRuntimeService; // hold this to hold the JSRuntime
    JSContext2XPCContextMap* mContextMap;
    JSObject2WrappedJSMap*   mWrappedJSMap;
    IID2WrappedJSClassMap*   mWrappedJSClassMap;
    IID2NativeInterfaceMap*  mIID2NativeInterfaceMap;
    ClassInfo2NativeSetMap*  mClassInfo2NativeSetMap;
    NativeSetMap*            mNativeSetMap;
    IID2ThisTranslatorMap*   mThisTranslatorMap;
    XPCLock* mMapLock;
    nsVoidArray mWrappedJSToReleaseArray;

};

/***************************************************************************/
/***************************************************************************/
// XPCContext is mostly a dumb class to hold JSContext specific data and
// maps that let us find wrappers created for the given JSContext.

// no virtuals
class XPCContext
{
public:
    static XPCContext* newXPCContext(XPCJSRuntime* aRuntime,
                                     JSContext* aJSContext);

    XPCJSRuntime* GetRuntime() const {return mRuntime;}
    JSContext* GetJSContext() const {return mJSContext;}

    enum LangType {LANG_UNKNOWN, LANG_JS, LANG_NATIVE};

    LangType GetCallingLangType() const
        {return mCallingLangType;}
    LangType SetCallingLangType(LangType lt)
        {LangType tmp = mCallingLangType; mCallingLangType = lt; return tmp;}
    JSBool CallerTypeIsJavaScript() const {return LANG_JS == mCallingLangType;}
    JSBool CallerTypeIsNative() const {return LANG_NATIVE == mCallingLangType;}
    JSBool CallerTypeIsKnown() const {return LANG_UNKNOWN != mCallingLangType;}

    nsresult GetException(nsIXPCException** e)
        {
            NS_IF_ADDREF(mException);
            *e = mException;
            return NS_OK;
        }
    void SetException(nsIXPCException* e)
        {
            NS_IF_ADDREF(e);
            NS_IF_RELEASE(mException);
            mException = e;
        }

    nsresult GetLastResult() {return mLastResult;}
    void SetLastResult(nsresult rc) {mLastResult = rc;}

    nsresult GetPendingResult() {return mPendingResult;}
    void SetPendingResult(nsresult rc) {mPendingResult = rc;}

    nsIXPCSecurityManager* GetSecurityManager() const
        {return mSecurityManager;}
    void SetSecurityManager(nsIXPCSecurityManager* aSecurityManager)
        {mSecurityManager = aSecurityManager;}

    PRUint16 GetSecurityManagerFlags() const
        {return mSecurityManagerFlags;}
    void SetSecurityManagerFlags(PRUint16 f)
        {mSecurityManagerFlags = f;}

    nsIXPCSecurityManager* GetAppropriateSecurityManager(PRUint16 flags) const
        {
            NS_WARN_IF_FALSE(CallerTypeIsKnown(),"missing caller type set somewhere");
            if(!CallerTypeIsJavaScript())
                return nsnull;
            if(mSecurityManager)
            {
                if(flags & mSecurityManagerFlags)
                    return mSecurityManager;
            }
            else
            {
                nsIXPCSecurityManager* mgr;
                nsXPConnect* xpc = mRuntime->GetXPConnect();
                mgr = xpc->GetDefaultSecurityManager();
                if(mgr && (flags & xpc->GetDefaultSecurityManagerFlags()))
                    return mgr;
            }
            return nsnull;
        }

    void DebugDump(PRInt16 depth);

    ~XPCContext();

private:
    XPCContext();    // no implementation
    XPCContext(XPCJSRuntime* aRuntime, JSContext* aJSContext);

private:
    XPCJSRuntime* mRuntime;
    JSContext*  mJSContext;
    nsresult mLastResult;
    nsresult mPendingResult;
    nsIXPCSecurityManager* mSecurityManager;
    PRUint16 mSecurityManagerFlags;
    nsIXPCException* mException;
    LangType mCallingLangType;
};


/***************************************************************************/

#define NATIVE_CALLER  XPCContext::LANG_NATIVE
#define JS_CALLER      XPCContext::LANG_JS

// No virtuals
// XPCCallContext is ALWAYS declared as a local variables in some function;
// i.e. Instance lifetime is always controled by some C++ function returning.

class XPCCallContext : public nsIXPCNativeCallContext
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCNATIVECALLCONTEXT

    enum {NO_ARGS = (uintN) -1};

    XPCCallContext(XPCContext::LangType callerLanguage,
                   JSContext* cx   = nsnull,
                   JSObject* obj   = nsnull,
                   jsval id        = 0,
                   uintN argc      = NO_ARGS,
                   jsval *argv     = nsnull,
                   jsval *rval     = nsnull);

    virtual ~XPCCallContext();

    inline JSBool                       IsValid() const ;

    inline nsXPConnect*                 GetXPConnect() const ;
    inline XPCJSRuntime*                GetRuntime() const ;
    inline XPCPerThreadData*            GetThreadData() const ;
    inline XPCContext*                  GetXPCContext() const ;
    inline JSContext*                   GetJSContext() const ;
    inline JSContext*                   GetSafeJSContext() const ;
    inline JSBool                       GetContextPopRequired() const ;
    inline XPCContext::LangType         GetCallerLanguage() const ;
    inline XPCContext::LangType         GetPrevCallerLanguage() const ;
    inline XPCCallContext*              GetPrevCallContext() const ;

    inline JSObject*                    GetOperandJSObject() const ;
    inline JSObject*                    GetCurrentJSObject() const ;
    inline JSObject*                    GetFlattenedJSObject() const ;

    inline nsISupports*                 GetIdentityObject() const ;
    inline XPCWrappedNative*            GetWrapper() const ;
    
    inline JSBool                       CanGetTearOff() const ;
    inline XPCWrappedNativeTearOff*     GetTearOff() const ;

    inline XPCNativeScriptableInfo*     GetScriptableInfo() const ;
    inline JSBool                       CanGetSet() const ;
    inline XPCNativeSet*                GetSet() const ;
    inline XPCNativeInterface*          GetInterface() const ;
    inline XPCNativeMember*             GetMember() const ;
    inline JSBool                       HasInterfaceAndMember() const ;
    inline jsval                        GetName() const ;
    inline JSBool                       GetStaticMemberIsLocal() const ;
    inline uintN                        GetArgc() const ;
    inline jsval*                       GetArgv() const ;
    inline jsval*                       GetRetVal() const ;
    inline JSBool                       GetExceptionWasThrown() const ;
    inline JSBool                       GetReturnValueWasSet() const ;

    inline PRUint16                     GetMethodIndex() const ;
    inline void                         SetMethodIndex(PRUint16 index) ;

    inline jsval GetResolveName() const;
    inline jsval SetResolveName(jsval name);

    inline XPCWrappedNative* GetResolvingWrapper() const;
    inline XPCWrappedNative* SetResolvingWrapper(XPCWrappedNative* w);

    inline void SetRetVal(jsval val);

    void SetName(jsval name);
    void SetArgsAndResultPtr(uintN argc, jsval *argv, jsval *rval);
    void SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member, 
                     JSBool isSetter);

    JSBool  CanCallNow();

    void SystemIsBeingShutDown();

    operator JSContext*() const {return GetJSContext();}

private:

    // no copy ctor or assignment allowed
    XPCCallContext(const XPCCallContext& r); // not implemented
    XPCCallContext& operator= (const XPCCallContext& r); // not implemented

private:
    // posible values for mState
    enum State {
        INIT_FAILED,
        SYSTEM_SHUTDOWN,
        HAVE_CONTEXT,
        HAVE_OBJECT,
        HAVE_NAME,
        HAVE_ARGS,
        READY_TO_CALL,
        CALL_DONE
    };

#ifdef DEBUG
inline void CHECK_STATE(int s) const {NS_ASSERTION(mState >= s, "bad state");}
#else
#define CHECK_STATE(s) ((void)0)
#endif

private:
    State                           mState;

    nsCOMPtr<nsXPConnect>           mXPC;

    XPCPerThreadData*               mThreadData;
    XPCContext*                     mXPCContext;
    JSContext*                      mJSContext;
    JSBool                          mContextPopRequired;

    XPCContext::LangType            mCallerLanguage;
    XPCContext::LangType            mPrevCallerLanguage;

    XPCCallContext*                 mPrevCallContext;

    JSObject*                       mOperandJSObject;
    JSObject*                       mCurrentJSObject;
    JSObject*                       mFlattenedJSObject;
    XPCWrappedNative*               mWrapper;
    XPCWrappedNativeTearOff*        mTearOff;

    XPCNativeScriptableInfo*        mScriptableInfo;

    XPCNativeSet*                   mSet;
    XPCNativeInterface*             mInterface;
    XPCNativeMember*                mMember;

    jsval                           mName;
    JSBool                          mStaticMemberIsLocal;

    uintN                           mArgc;
    jsval*                          mArgv;
    jsval*                          mRetVal;

    JSBool                          mExceptionWasThrown;
    JSBool                          mReturnValueWasSet;
    PRUint16                        mMethodIndex;
};

/***************************************************************************/

class XPCWrappedNativeScope
{
public:
    XPCJSRuntime*
    GetRuntime() const {return mRuntime;}

    Native2WrappedNativeMap*
    GetWrappedNativeMap() const {return mWrappedNativeMap;}

    ClassInfo2WrappedNativeProtoMap*
    GetWrappedNativeProtoMap() const {return mWrappedNativeProtoMap;}

    nsXPCComponents*
    GetComponents() const {return mComponents;}

    JSObject*
    GetGlobalJSObject() const {return mGlobalJSObject;}

    JSObject*
    GetPrototypeJSObject() const {return mPrototypeJSObject;}

    static XPCWrappedNativeScope*
    FindInJSObjectScope(XPCCallContext& ccx, JSObject* obj);

    static void 
    SystemIsBeingShutDown(XPCCallContext& ccx);

    static void
    FinishedMarkPhaseOfGC(JSContext* cx, XPCJSRuntime* rt);

    static void
    FinishedFinalizationPhaseOfGC(JSContext* cx);

    static void
    MarkAllInterfaceSets();

#ifdef DEBUG
    static void
    ASSERT_NoInterfaceSetsAreMarked();
#endif

    static void
    SweepAllWrappedNativeTearOffs();

    static void
    DebugDumpAllScopes(PRInt16 depth);

    void
    DebugDump(PRInt16 depth);

    JSBool
    IsValid() const {return mRuntime != nsnull;}

    void SetComponents(nsXPCComponents* aComponents);
    void SetGlobal(XPCCallContext& ccx, JSObject* aGlobal);

    XPCWrappedNativeScope(XPCCallContext& ccx, JSObject* aGlobal);
    virtual ~XPCWrappedNativeScope();
private:

    static void KillDyingScopes();

    XPCWrappedNativeScope(); // not implemented

private:
    static XPCWrappedNativeScope* gScopes;
    static XPCWrappedNativeScope* gDyingScopes;

    XPCJSRuntime*                    mRuntime;
    Native2WrappedNativeMap*         mWrappedNativeMap;
    ClassInfo2WrappedNativeProtoMap* mWrappedNativeProtoMap;
    nsXPCComponents*                 mComponents;
    XPCWrappedNativeScope*           mNext;
    JSObject*                        mGlobalJSObject;
    JSObject*                        mPrototypeJSObject;
};

/***************************************************************************/
// code for throwing exceptions into JS

class XPCThrower
{
public:
    static void Throw(nsresult rv, JSContext* cx);
    static void Throw(nsresult rv, XPCCallContext& ccx);
    static void ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx);
    static void ThrowBadParam(nsresult rv, uintN paramNum, XPCCallContext& ccx);

    static JSBool SetVerbosity(JSBool state)
        {JSBool old = sVerbose; sVerbose = state; return old;}

private:
    static void Verbosify(XPCCallContext& ccx,
                          char** psz, PRBool own);

    static void BuildAndThrowException(JSContext* cx, nsresult rv, const char* sz);
    static JSBool ThrowExceptionObject(JSContext* cx, nsIXPCException* e);

private:
    static JSBool sVerbose;
};


/***************************************************************************/
/***************************************************************************/

// this interfaces exists so we can refcount nsXPCWrappedJSClass
// {2453EBA0-A9B8-11d2-BA64-00805F8A5DD7}
#define NS_IXPCONNECT_WRAPPED_JS_CLASS_IID  \
{ 0x2453eba0, 0xa9b8, 0x11d2,               \
  { 0xba, 0x64, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }

class nsIXPCWrappedJSClass : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXPCONNECT_WRAPPED_JS_CLASS_IID)
    NS_IMETHOD DebugDump(PRInt16 depth) = 0;
};

/*************************/

class nsXPCWrappedJSClass : public nsIXPCWrappedJSClass
{
    // all the interface method declarations...
    NS_DECL_ISUPPORTS
    NS_IMETHOD DebugDump(PRInt16 depth);
public:

    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 REFNSIID aIID,
                 nsXPCWrappedJSClass** clazz);
    
    REFNSIID GetIID() const {return mIID;}
    XPCJSRuntime* GetRuntime() const {return mRuntime;}
    nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo;}
    const char* GetInterfaceName();

    static JSBool InitClasses(XPCCallContext& ccx, JSObject* aGlobalJSObj);
    static JSBool IsWrappedJS(nsISupports* aPtr);

    NS_IMETHOD DelegatedQueryInterface(nsXPCWrappedJS* self, REFNSIID aIID,
                                       void** aInstancePtr);

    JSObject* GetRootJSObject(XPCCallContext& ccx, JSObject* aJSObj);

    NS_IMETHOD CallMethod(nsXPCWrappedJS* wrapper, uint16 methodIndex,
                          const nsXPTMethodInfo* info,
                          nsXPTCMiniVariant* params);

    virtual ~nsXPCWrappedJSClass();
private:
    nsXPCWrappedJSClass();   // not implemented
    nsXPCWrappedJSClass(XPCCallContext& ccx, REFNSIID aIID,
                        nsIInterfaceInfo* aInfo);

    JSObject*  NewOutObject(JSContext* cx);

    JSObject*  CallQueryInterfaceOnJSObject(XPCCallContext& ccx,
                                            JSObject* jsobj, REFNSIID aIID);

    JSBool IsReflectable(uint16 i) const
        {return (JSBool)(mDescriptors[i/32] & (1 << (i%32)));}
    void SetReflectable(uint16 i, JSBool b)
        {if(b) mDescriptors[i/32] |= (1 << (i%32));
         else mDescriptors[i/32] &= ~(1 << (i%32));}

    enum SizeMode {GET_SIZE, GET_LENGTH};

    JSBool GetArraySizeFromParam(JSContext* cx,
                                 const nsXPTMethodInfo* method,
                                 const nsXPTParamInfo& param,
                                 uint16 methodIndex,
                                 uint8 paramIndex,
                                 SizeMode mode,
                                 nsXPTCMiniVariant* params,
                                 JSUint32* result);

    JSBool GetInterfaceTypeFromParam(JSContext* cx,
                                     const nsXPTMethodInfo* method,
                                     const nsXPTParamInfo& param,
                                     uint16 methodIndex,
                                     const nsXPTType& type,
                                     nsXPTCMiniVariant* params,
                                     JSBool* iidIsOwned,
                                     nsID** result);

    void CleanupPointerArray(const nsXPTType& datum_type,
                             JSUint32 array_count,
                             void** arrayp);

    void CleanupPointerTypeObject(const nsXPTType& type,
                                  void** pp);

private:
    XPCJSRuntime* mRuntime;
    nsIInterfaceInfo* mInfo;
    char* mName;
    nsIID mIID;
    uint32* mDescriptors;
};

/*************************/

class nsXPCWrappedJS : public nsXPTCStubBase,
                       public nsIXPConnectWrappedJS,
                       public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDJS

    // Note that both nsXPTCStubBase and nsIXPConnectWrappedJS declare
    // GetInterfaceInfo methods with the same sig. So, the declaration
    // for it here comes from the NS_DECL_NSIXPCONNECTWRAPPEDJS macro

    NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const nsXPTMethodInfo* info,
                          nsXPTCMiniVariant* params);

    /*
    * This is rarely called directly. Instead one usually calls
    * XPCConvert::JSObject2NativeInterface which will handles cases where the
    * JS object is already a wrapped native or a DOM object.
    */

    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 JSObject* aJSObj,
                 REFNSIID aIID,
                 nsISupports* aOuter,
                 nsXPCWrappedJS** wrapper); 

    JSObject* GetJSObject() const {return mJSObj;}
    nsXPCWrappedJSClass*  GetClass() const {return mClass;}
    REFNSIID GetIID() const {return GetClass()->GetIID();}
    nsXPCWrappedJS* GetRootWrapper() const {return mRoot;}
    nsXPCWrappedJS* GetNextWrapper() const {return mNext;}

    nsXPCWrappedJS* Find(REFNSIID aIID);
    nsXPCWrappedJS* FindInherited(REFNSIID aIID);

    JSBool IsValid() const {return mJSObj != nsnull;}
    void SystemIsBeingShutDown(JSRuntime* rt);

    // This is used by XPCJSRuntime::GCCallback to find wrappers that no
    // longer root their JSObject and are only still alive because they
    // were being used via nsSupportsWeakReference at the time when their
    // last (outside) reference was released. Wrappers that fit into that
    // category are only deleted when we see that their cooresponding JSObject
    // is to be finalized.
    JSBool IsSubjectToFinalization() const {return IsValid() && mRefCnt == 1;}

    JSBool IsAggregatedToNative() const {return mRoot->mOuter != nsnull;}
    nsISupports* GetAggregatedNativeObject() const {return mRoot->mOuter;}

    virtual ~nsXPCWrappedJS();
protected:
    nsXPCWrappedJS();   // not implemented
    nsXPCWrappedJS(XPCCallContext& ccx,
                   JSObject* aJSObj,
                   nsXPCWrappedJSClass* aClass,
                   nsXPCWrappedJS* root,
                   nsISupports* aOuter);

private:
    JSObject* mJSObj;
    nsXPCWrappedJSClass* mClass;
    nsXPCWrappedJS* mRoot;
    nsXPCWrappedJS* mNext;
    nsISupports* mOuter;    // only set in root
};

/***************************************************************************/

class XPCJSObjectHolder : public nsIXPConnectJSObjectHolder
{
public:
    // all the interface method declarations...
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER

    // non-interface implementation

public:
    static XPCJSObjectHolder* newHolder(JSContext* cx, JSObject* obj);

    virtual ~XPCJSObjectHolder();

private:
    XPCJSObjectHolder(JSContext* cx, JSObject* obj);
    XPCJSObjectHolder(); // not implemented

    JSRuntime* mRuntime;
    JSObject* mJSObj;
};

/***************************************************************************/
// data conversion

// class here just for static methods
class XPCConvert
{
public:
    static JSBool IsMethodReflectable(const nsXPTMethodInfo& info);

    static JSBool NativeData2JS(XPCCallContext& ccx, jsval* d, const void* s,
                                const nsXPTType& type, const nsID* iid,
                                JSObject* scope, nsresult* pErr);

    static JSBool JSData2Native(XPCCallContext& ccx, void* d, jsval s,
                                const nsXPTType& type,
                                JSBool useAllocator, const nsID* iid,
                                nsresult* pErr);

    static JSBool NativeInterface2JSObject(XPCCallContext& ccx,
                                           nsIXPConnectJSObjectHolder** dest,
                                           nsISupports* src,
                                           const nsID* iid,
                                           JSObject* scope, nsresult* pErr);

    static JSBool JSObject2NativeInterface(XPCCallContext& ccx,
                                           void** dest, JSObject* src,
                                           const nsID* iid,
                                           nsISupports* aOuter,
                                           nsresult* pErr);

    static JSBool NativeArray2JS(XPCCallContext& ccx,
                                 jsval* d, const void** s,
                                 const nsXPTType& type, const nsID* iid,
                                 JSUint32 count, JSObject* scope,
                                 nsresult* pErr);

    static JSBool JSArray2Native(XPCCallContext& ccx, void** d, jsval s,
                                 JSUint32 count, JSUint32 capacity,
                                 const nsXPTType& type,
                                 JSBool useAllocator, const nsID* iid,
                                 uintN* pErr);

    static JSBool NativeStringWithSize2JS(XPCCallContext& ccx,
                                          jsval* d, const void* s,
                                          const nsXPTType& type,
                                          JSUint32 count,
                                          nsresult* pErr);

    static JSBool JSStringWithSize2Native(XPCCallContext& ccx, void* d, jsval s,
                                          JSUint32 count, JSUint32 capacity,
                                          const nsXPTType& type,
                                          JSBool useAllocator,
                                          uintN* pErr);

    static nsresult JSValToXPCException(XPCCallContext& ccx,
                                        jsval s,
                                        const char* ifaceName,
                                        const char* methodName,
                                        nsIXPCException** exception);

    static nsresult JSErrorToXPCException(XPCCallContext& ccx,
                                          const char* message,
                                          const char* ifaceName,
                                          const char* methodName,
                                          const JSErrorReport* report,
                                          nsIXPCException** exception);

    static nsresult ConstructException(nsresult rv, const char* message,
                                       const char* ifaceName, 
                                       const char* methodName,
                                       nsISupports* data,
                                       nsIXPCException** exception);

private:
    XPCConvert(); // not implemented

};

// class to export a JSString as an nsAReadableString, including refcounting
class XPCReadableJSStringWrapper : public nsLiteralString
{
public:
    XPCReadableJSStringWrapper(JSString *str) :
        nsLiteralString(NS_REINTERPRET_CAST(PRUnichar *,
                                            JS_GetStringChars(str)),
                        JS_GetStringLength(str)),
        mStr(str), mBufferHandle(0), mHandleIsShared(JS_FALSE)
    { }

    ~XPCReadableJSStringWrapper();

    // buffer-handle accessors
    const nsBufferHandle<PRUnichar>* GetBufferHandle() const
    {
        return BufferHandle(JS_FALSE);
    }
    
    const nsSharedBufferHandle<PRUnichar>* GetSharedBufferHandle() const
    {
        return BufferHandle(JS_TRUE);
    }

protected:
    struct WrapperBufferHandle :
        public nsSharedBufferHandleWithAllocator<PRUnichar>
    {
        WrapperBufferHandle(XPCReadableJSStringWrapper *outer, JSString *str) :
            nsSharedBufferHandleWithAllocator<PRUnichar>
                (NS_CONST_CAST(PRUnichar *, outer->get()),
                 NS_CONST_CAST(PRUnichar *, outer->get() + outer->Length()),
                 mAllocator),
            mAllocator(str)
        { }

        XPCReadableJSStringWrapper *mOuter;

        struct Allocator : nsStringAllocator<PRUnichar>
        {
            Allocator(JSString *str) :  mStr(OBJECT_TO_JSVAL(str)) { }
            virtual ~Allocator() { }

            virtual void Deallocate(PRUnichar *) const;

            JSBool RootString();
            jsval mStr;
        };

        Allocator mAllocator;
    };
    
    const nsSharedBufferHandle<PRUnichar>* BufferHandle(JSBool shared) const;

    JSString            *mStr;
    WrapperBufferHandle *mBufferHandle;
    JSBool              mHandleIsShared;
};

// readable string conversions, static methods only
class XPCStringConvert
{
public:

    static JSString *ReadableToJSString(JSContext *cx, 
                                        const nsAReadableString &readable);

    static XPCReadableJSStringWrapper *JSStringToReadable(JSString *str);

    static void ShutdownDOMStringFinalizer();

private:
    XPCStringConvert();         // not implemented
};

extern JSBool JS_DLL_CALLBACK
XPC_JSArgumentFormatter(JSContext *cx, const char *format,
                        JSBool fromJS, jsval **vpp, va_list *app);

/***************************************************************************/

class XPCJSStack
{
public:
    static nsresult
    CreateStack(JSContext* cx, nsIJSStackFrameLocation** stack);

    static nsresult
    CreateStackFrameLocation(JSBool isJSFrame,
                             const char* aFilename,
                             const char* aFunctionName,
                             PRInt32 aLineNumber,
                             nsIJSStackFrameLocation* aCaller,
                             nsIJSStackFrameLocation** stack);
private:
    XPCJSStack();   // not implemented
};

/***************************************************************************/

class nsXPCException :
            public nsIXPCException
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
          , public nsISecurityCheckedComponent
#endif
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_XPCEXCEPTION_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCEXCEPTION
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
#endif

    static nsresult NewException(const char *aMessage,
                                 nsresult aResult,
                                 nsIJSStackFrameLocation *aLocation,
                                 nsISupports *aData,
                                 nsIXPCException** exception);

    static JSBool NameAndFormatForNSResult(nsresult rv,
                                           const char** name,
                                           const char** format);

    static void* IterateNSResults(nsresult* rv,
                                  const char** name,
                                  const char** format,
                                  void** iterp);

    static PRUint32 GetNSResultCount();

    nsXPCException();
    virtual ~nsXPCException();

protected:
    void Reset();
private:
    char*                       mMessage;
    nsresult                    mResult;
    char*                       mName;
    nsIJSStackFrameLocation*    mLocation;
    nsISupports*                mData;
    PRBool                      mInitialized;
};

/***************************************************************************/

extern JSClass XPC_WN_NoHelper_JSClass;
extern JSClass XPC_WN_NoMods_Proto_JSClass;
extern JSClass XPC_WN_ModsAllowed_Proto_JSClass;
extern JSClass XPC_WN_Tearoff_JSClass;

extern JSObjectOps * JS_DLL_CALLBACK
XPC_WN_GetObjectOpsNoCall(JSContext *cx, JSClass *clazz);

extern JSObjectOps * JS_DLL_CALLBACK
XPC_WN_GetObjectOpsWithCall(JSContext *cx, JSClass *clazz);

extern JSBool JS_DLL_CALLBACK
XPC_WN_CallMethod(JSContext *cx, JSObject *obj,
                  uintN argc, jsval *argv, jsval *vp);

extern JSBool JS_DLL_CALLBACK
XPC_WN_GetterSetter(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *vp);

extern JSBool 
xpc_InitWrappedNativeJSOps();

/***************************************************************************/
/*
* nsJSID implements nsIJSID. It is also used by nsJSIID and nsJSCID as a
* member (as a hidden implementaion detail) to which they delegate many calls.
*/

class nsJSID : public nsIJSID
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_JS_ID_CID)

    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSID

    PRBool InitWithName(const nsID& id, const char *nameString);
    PRBool SetName(const char* name);
    void   SetNameToNoString()
        {NS_ASSERTION(!mName, "name already set"); mName = gNoString;}
    PRBool NameIsSet() const {return nsnull != mName;}
    const nsID* GetID() const {return &mID;}

    PRBool IsValid() const {return !mID.Equals(GetInvalidIID());}

    static nsJSID* NewID(const char* str);

    nsJSID();
    virtual ~nsJSID();
protected:

    void Reset();
    const nsID& GetInvalidIID() const;

protected:
    static char gNoString[];
    nsID    mID;
    char*   mNumber;
    char*   mName;
};

// nsJSIID

class nsJSIID : public nsIJSIID, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS

    // we manually delagate these to nsJSID
    NS_DECL_NSIJSID

    // we implement the rest...
    NS_DECL_NSIJSIID
    NS_DECL_NSIXPCSCRIPTABLE

    static nsJSIID* NewID(const char* str);

    nsJSIID();
    virtual ~nsJSIID();

private:
    void ResolveName();

private:
    nsJSID mDetails;
};

// nsJSCID

class nsJSCID : public nsIJSCID, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS

    // we manually delagate these to nsJSID
    NS_DECL_NSIJSID

    // we implement the rest...
    NS_DECL_NSIJSCID
    NS_DECL_NSIXPCSCRIPTABLE

    static nsJSCID* NewID(const char* str);

    nsJSCID();
    virtual ~nsJSCID();

private:
    void ResolveName();

private:
    nsJSID mDetails;
};


/***************************************************************************/

class XPCJSContextStack
{
public:
    NS_DECL_NSIJSCONTEXTSTACK
    NS_DECL_NSITHREADJSCONTEXTSTACK

    XPCJSContextStack();
    virtual ~XPCJSContextStack();

private:
    void SyncJSContexts();

private:
    nsDeque     mStack;
    JSContext*  mSafeJSContext;

    // If if non-null, we own it; same as mSafeJSContext if SetSafeJSContext
    // not called.
    JSContext*  mOwnSafeJSContext;
};


/**************************************************************/
// All of our thread local storage.

class XPCPerThreadData
{
public:
    // Get the instance of this object for the current thread
    static XPCPerThreadData* GetData();
    static void CleanupAllThreads();

    ~XPCPerThreadData();

    nsresult GetException(nsIXPCException** aException)
    {
        NS_IF_ADDREF(mException);
        *aException = mException;
        return NS_OK;
    }
    
    void SetException(nsIXPCException* aException)
    {
        NS_IF_ADDREF(aException);
        NS_IF_RELEASE(mException);
        mException = aException;
    }

    XPCJSContextStack* GetJSContextStack() {return mJSContextStack;}

    XPCCallContext*  GetCallContext() const {return mCallContext;}
    XPCCallContext*  SetCallContext(XPCCallContext* ccx)
        {XPCCallContext* old = mCallContext; mCallContext = ccx; return old;}

    jsval GetResolveName() const {return mResolveName;}
    jsval SetResolveName(jsval name)
        {jsval old = mResolveName; mResolveName = name; return old;}

    XPCWrappedNative* GetResolvingWrapper() const {return mResolvingWrapper;}
    XPCWrappedNative* SetResolvingWrapper(XPCWrappedNative* w)
        {XPCWrappedNative* old = mResolvingWrapper;
         mResolvingWrapper = w; return old;}

    void Cleanup();

    PRBool IsValid() const {return mJSContextStack != nsnull;}

    static PRLock* GetLock() {return gLock;}
    // Must be called with the threads locked.
    static XPCPerThreadData* IterateThreads(XPCPerThreadData** iteratorp);

private:
    XPCPerThreadData();

private:
    nsIXPCException*    mException;
    XPCJSContextStack*  mJSContextStack;
    XPCPerThreadData*   mNextThread;
    XPCCallContext*     mCallContext;
    jsval               mResolveName;
    XPCWrappedNative*   mResolvingWrapper;

    static PRLock*           gLock;
    static XPCPerThreadData* gThreads;
    static PRUintn           gTLSIndex;
};

/**************************************************************/

#define NS_XPC_THREAD_JSCONTEXT_STACK_CID  \
{ 0xff8c4d10, 0x3194, 0x11d3, \
    { 0x98, 0x85, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }

class nsXPCThreadJSContextStackImpl : public nsIThreadJSContextStack
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSCONTEXTSTACK
    NS_DECL_NSITHREADJSCONTEXTSTACK

    // This returns and AddRef'd pointer. It does not do this with an out param
    // only because this form  is required by generic module macro:
    // NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR 
    static nsXPCThreadJSContextStackImpl* GetSingleton();
    
    static void FreeSingleton();

    nsXPCThreadJSContextStackImpl();
    virtual ~nsXPCThreadJSContextStackImpl();

private:
    XPCJSContextStack* GetStackForCurrentThread()
        {XPCPerThreadData* data = XPCPerThreadData::GetData();
         return data ? data->GetJSContextStack() : nsnull;}
};

/***************************************************************************/
#define NS_JS_RUNTIME_SERVICE_CID \
{0xb5e65b52, 0x1dd1, 0x11b2, \
    { 0xae, 0x8f, 0xf0, 0x92, 0x8e, 0xd8, 0x84, 0x82 }}

class nsJSRuntimeServiceImpl : public nsIJSRuntimeService
{
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJSRUNTIMESERVICE

    // This returns and AddRef'd pointer. It does not do this with an out param
    // only because this form  is required by generic module macro:
    // NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR 
    static nsJSRuntimeServiceImpl* GetSingleton();

    static void FreeSingleton();

    nsJSRuntimeServiceImpl();
    virtual ~nsJSRuntimeServiceImpl();
 protected:
    JSRuntime *mRuntime;
};

/***************************************************************************/
// 'Components' object

class nsXPCComponents : public nsIXPCComponents,
                        public nsIXPCScriptable
#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
                      , public nsISecurityCheckedComponent
#endif
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS
    NS_DECL_NSIXPCSCRIPTABLE

#ifdef XPC_USE_SECURITY_CHECKED_COMPONENT
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
#endif

public:
    static JSBool
    AttachNewComponentsObject(XPCCallContext& ccx,
                              XPCWrappedNativeScope* aScope,
                              JSObject* aGlobal);

    virtual ~nsXPCComponents();

private:
    nsXPCComponents();

private:
    nsXPCComponents_Interfaces*  mInterfaces;
    nsXPCComponents_Classes*     mClasses;
    nsXPCComponents_ClassesByID* mClassesByID;
    nsXPCComponents_Results*     mResults;
    nsXPCComponents_ID*          mID;
    nsXPCComponents_Exception*   mException;
    nsXPCComponents_Constructor* mConstructor;
};

/***************************************************************************/

extern JSObject*
xpc_NewIDObject(JSContext *cx, JSObject* jsobj, const nsID& aID);

extern nsID*
xpc_JSObjectToID(JSContext *cx, JSObject* obj);

/***************************************************************************/
// in xpcdebug.cpp

extern JSBool
xpc_DumpJSStack(JSContext* cx, JSBool showArgs, JSBool showLocals,
                JSBool showThisProps);

extern JSBool
xpc_DumpEvalInJSStackFrame(JSContext* cx, JSUint32 frameno, const char* text);

extern JSBool
xpc_DumpJSObject(JSObject* obj);

extern JSBool
xpc_InstallJSDebuggerKeywordHandler(JSRuntime* rt);

/***************************************************************************/

// Definition of nsScriptError, defined here because we lack a place to put
// XPCOM objects associated with the JavaScript engine.
class nsScriptError : public nsIScriptError {
public:
    nsScriptError();

    virtual ~nsScriptError();

  // TODO - do something reasonable on getting null from these babies.

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLEMESSAGE
    NS_DECL_NSISCRIPTERROR

private:
    nsString mMessage;
    nsString mSourceName;
    PRUint32 mLineNumber;
    nsString mSourceLine;
    PRUint32 mColumnNumber;
    PRUint32 mFlags;
    nsCString mCategory;
};

/***************************************************************************/
// XXX allowing for future notifications to XPCCallContext

class AutoJSRequest
{
public:
    AutoJSRequest(XPCCallContext& aCCX)
      : mCCX(aCCX), mCX(aCCX.GetJSContext()) {BeginRequest();}
    ~AutoJSRequest() {EndRequest();}

    void EndRequest() {
        if(mCX) {
            JS_EndRequest(mCX);
            mCX = nsnull;
        }
    }
private:
    void BeginRequest() {
        if(JS_GetContextThread(mCX))
            JS_BeginRequest(mCX);
        else
            mCX = nsnull;
    }
private:
    XPCCallContext& mCCX;
    JSContext* mCX;
};

class AutoJSSuspendRequest
{
public:
    AutoJSSuspendRequest(XPCCallContext& aCCX)
      : mCCX(aCCX), mCX(aCCX.GetJSContext()) {SuspendRequest();}
    ~AutoJSSuspendRequest() {ResumeRequest();}

    void ResumeRequest() {
        if(mCX) {
            JS_ResumeRequest(mCX, mDepth);
            mCX = nsnull;
        }
    }
private:
    void SuspendRequest() {
        if(JS_GetContextThread(mCX))
            mDepth = JS_SuspendRequest(mCX);
        else
            mCX = nsnull;
    }
private:
    XPCCallContext& mCCX;
    JSContext* mCX;
    jsrefcount mDepth;
};

/*****************************************/

class AutoJSRequestWithNoCallContext
{
public:
    AutoJSRequestWithNoCallContext(JSContext* aCX) : mCX(aCX) {BeginRequest();}
    ~AutoJSRequestWithNoCallContext() {EndRequest();}

    void EndRequest() {
        if(mCX) {
            JS_EndRequest(mCX);
            mCX = nsnull;
        }
    }
private:
    void BeginRequest() {
        if(JS_GetContextThread(mCX))
            JS_BeginRequest(mCX);
        else
            mCX = nsnull;
    }
private:
    JSContext* mCX;
};

/***************************************************************************/
class AutoJSErrorAndExceptionEater
{
public:
    AutoJSErrorAndExceptionEater(JSContext* aCX)
        : mCX(aCX),
          mOldErrorReporter(JS_SetErrorReporter(mCX, nsnull)),
          mOldExceptionState(JS_SaveExceptionState(mCX)) {}
    ~AutoJSErrorAndExceptionEater()
    {
        JS_SetErrorReporter(mCX, mOldErrorReporter);
        JS_RestoreExceptionState(mCX, mOldExceptionState);
    }
private:
    JSContext*        mCX;
    JSErrorReporter   mOldErrorReporter;
    JSExceptionState* mOldExceptionState;
 };


/***************************************************************************/
class AutoResolveName
{
public:
    AutoResolveName(XPCCallContext& ccx, jsval name)
        : mTLS(ccx.GetThreadData()), 
          mOld(mTLS->SetResolveName(name)),
          mCheck(name) {}
    ~AutoResolveName()
        {jsval old = mTLS->SetResolveName(mOld); 
         NS_ASSERTION(old == mCheck, "Bad Nesting!");}

private:
    XPCPerThreadData* mTLS;
    jsval mOld;
    jsval mCheck;
};

/***************************************************************************/

// Tight. No virtual methods. Can be bitwise copied (until any resolution done).
class XPCNativeMember
{
public:
    static JSBool GetCallInfo(XPCCallContext& ccx, 
                              JSObject* funobj,
                              XPCNativeInterface** pInterface,
                              XPCNativeMember**    pMember);

    jsval   GetName() const {return mName;}

    PRUint16 GetIndex() const {return mIndex;}

    JSBool GetValue(XPCCallContext& ccx, XPCNativeInterface* iface, jsval* pval)
        {if(!IsResolved() && !Resolve(ccx, iface)) return JS_FALSE;
         *pval = mVal; return JS_TRUE;}

    JSBool IsMethod() const
        {return 0 != (mFlags & METHOD);}

    JSBool IsConstant() const
        {return 0 != (mFlags & CONSTANT);}

    JSBool IsAttribute() const
        {return 0 != (mFlags & GETTER);}

    JSBool IsWritableAttribute() const
        {return 0 != (mFlags & SETTER_TOO);}

    JSBool IsReadOnlyAttribute() const
        {return IsAttribute() && !IsWritableAttribute();}


    void SetName(jsval a) {mName = a;}

    void SetMethod(PRUint16 index)
        {mVal = JSVAL_NULL; mFlags = METHOD; mIndex = index;}

    void SetConstant(PRUint16 index)
        {mVal = JSVAL_NULL; mFlags = CONSTANT; mIndex = index;}

    void SetReadOnlyAttribute(PRUint16 index)
        {mVal = JSVAL_NULL; mFlags = GETTER; mIndex = index;}

    void SetWritableAttribute()
        {NS_ASSERTION(mFlags == GETTER,"bad"); mFlags = GETTER | SETTER_TOO;}

    /* default ctor - leave random contents */
    XPCNativeMember() {}
    ~XPCNativeMember() {}

    void Cleanup(JSContext* cx, XPCJSRuntime* rt);

    void DealWithDyingGCThings(JSContext* cx, XPCJSRuntime* rt)
        {if(IsResolved() && JSVAL_IS_GCTHING(mVal) &&
           JS_IsAboutToBeFinalized(cx, JSVAL_TO_GCTHING(mVal)))
           {Cleanup(cx, rt); mVal = JSVAL_NULL; mFlags &= ~RESOLVED;}}

private:
    JSBool IsResolved() const {return mFlags & RESOLVED;}
    JSBool Resolve(XPCCallContext& ccx, XPCNativeInterface* iface);

    void   CleanupCallableInfo(JSContext* cx, XPCJSRuntime* rt, 
                               JSObject* funobj);

    enum {
        RESOLVED    = 0x01,
        METHOD      = 0x02,
        CONSTANT    = 0x04,
        GETTER      = 0x08,
        SETTER_TOO  = 0x10
    };

private:
    // our only data...
    jsval    mName;
    jsval    mVal;
    PRUint16 mIndex;
    PRUint16 mFlags;
};

/***************************************************************************/

// Tight. No virtual methods.
class XPCNativeInterface
{
public:
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            const nsIID* iid);
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            nsIInterfaceInfo* info);
    static XPCNativeInterface* GetNewOrUsed(XPCCallContext& ccx,
                                            const char* name);
    static XPCNativeInterface* GetISupports(XPCCallContext& ccx);

    inline nsIInterfaceInfo* GetInterfaceInfo() const {return mInfo.get();}
    inline jsval             GetName()          const {return mName;}
    
    inline const nsIID* GetIID() const;
    inline const char*  GetNameString() const;
    inline XPCNativeMember* FindMember(jsval name) const;

    inline JSBool HasAncestor(const nsIID* iid) const;

    const char* GetMemberName(XPCCallContext& ccx,
                              const XPCNativeMember* member) const;

    PRUint16 GetMemberCount() const 
        {NS_ASSERTION(!IsMarked(), "bad"); return mMemberCount;}
    XPCNativeMember* GetMemberAt(PRUint16 i)
        {NS_ASSERTION(i < mMemberCount, "bad index"); return &mMembers[i];}

    inline void DealWithDyingGCThings(JSContext* cx, XPCJSRuntime* rt);

    void DebugDump(PRInt16 depth);

    void Mark()       {mMemberCount |= 0x8000;}
    void Unmark()     {mMemberCount &= ~0x8000;}
    JSBool IsMarked() const {return (JSBool)(mMemberCount & 0x8000);}

    static void DestroyInstance(JSContext* cx, XPCJSRuntime* rt,
                                XPCNativeInterface* inst);

private:
    static XPCNativeInterface* NewInstance(XPCCallContext& ccx,
                                           nsIInterfaceInfo* aInfo);

    XPCNativeInterface();   // not implemented
    XPCNativeInterface(nsIInterfaceInfo* aInfo, jsval aName)
        : mInfo(aInfo), mName(aName), mMemberCount(0) {}
    ~XPCNativeInterface() {}
   
    void* operator new(size_t, void* p) {return p;}
   
    XPCNativeInterface(const XPCNativeInterface& r); // not implemented
    XPCNativeInterface& operator= (const XPCNativeInterface& r); // not implemented

private:
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    jsval                      mName;
    PRUint16          mMemberCount;
    XPCNativeMember   mMembers[1]; // always last - object sized for array
};

/***************************************************************************/

class XPCNativeSetKey
{
public:
    XPCNativeSetKey(XPCNativeSet*       BaseSet  = nsnull,
                    XPCNativeInterface* Addition = nsnull,
                    PRUint16            Position = 0)
        : mIsAKey(IS_A_KEY), mBaseSet(BaseSet), mAddition(Addition), 
          mPosition(Position) {}
    ~XPCNativeSetKey() {}

    XPCNativeSet*           GetBaseSet()  const {return mBaseSet;}
    XPCNativeInterface*     GetAddition() const {return mAddition;}
    PRUint16                GetPosition() const {return mPosition;}

    // This is a fun little hack... 
    // We build these keys only on the stack. We use them for lookup in 
    // NativeSetMap. Becasue we don't want to pay the cost of cloning a key and
    // sticking it into the hashtable, when the XPCNativeSet actually
    // gets added to the table the 'key' in the table is a pointer to the 
    // set itself and not this key. Our key compare function expects to get
    // a key and a set. When we do external lookups in the map we pass in one
    // of these keys and our compare function gets passed a key and a set.
    // (see compare_NativeKeyToSet in xpcmaps.cpp). This is all well and good.
    // Except, when the table decides to resize itself. Then it tries to use
    // our compare function with the 'keys' that are in the hashtable (which are
    // really XPCNativeSet objects and not XPCNativeSetKey objects!
    //
    // So, the hack is to have the compare function assume it is getting a 
    // XPCNativeSetKey pointer and call this IsAKey method. If that fails then
    // it realises that it really has a XPCNativeSet pointer and deals with that
    // fact. This is safe because we know that both of these classes have no
    // virtual methods and their first data member is a PRUint16. We are
    // confident that XPCNativeSet->mMemberCount will never be 0xffff.

    JSBool                  IsAKey() const {return mIsAKey == IS_A_KEY;}

    enum {IS_A_KEY = 0xffff};

    // Allow shallow copy

private:
    PRUint16                mIsAKey;
    PRUint16                mPosition;
    XPCNativeSet*           mBaseSet;
    XPCNativeInterface*     mAddition;
};

class XPCNativeSet
{
public:
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid);
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx,
                                      nsIClassInfo* classInfo);
    static XPCNativeSet* GetNewOrUsed(XPCCallContext& ccx,
                                      XPCNativeSet* otherSet,
                                      XPCNativeInterface* newInterface,
                                      PRUint16 position);

    inline JSBool FindMember(jsval name, XPCNativeMember** pMember,
                             PRUint16* pInterfaceIndex) const;

    inline JSBool FindMember(jsval name, XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface) const;

    inline JSBool FindMember(jsval name, 
                             XPCNativeMember** pMember,
                             XPCNativeInterface** pInterface,
                             XPCNativeSet* protoSet,
                             JSBool* pIsLocal) const;

    inline JSBool HasInterface(XPCNativeInterface* aInterface) const;
    inline JSBool HasInterfaceWithAncestor(XPCNativeInterface* aInterface) const;

    inline XPCNativeInterface* FindInterfaceWithIID(const nsIID& iid) const;

    inline XPCNativeInterface* FindNamedInterface(jsval name) const;

    PRUint16 GetMemberCount() const {return mMemberCount;}
    PRUint16 GetInterfaceCount() const 
        {NS_ASSERTION(!IsMarked(), "bad"); return mInterfaceCount;}
    XPCNativeInterface** GetInterfaceArray() {return mInterfaces;}

    XPCNativeInterface* GetInterfaceAt(PRUint16 i)
        {NS_ASSERTION(i < mInterfaceCount, "bad index"); return mInterfaces[i];}

    inline JSBool MatchesSetUpToInterface(const XPCNativeSet* other, 
                                          XPCNativeInterface* iface) const;

    inline void Mark();
private:
    void MarkSelfOnly() {mInterfaceCount |= 0x8000;}
public:
    void Unmark()     {mInterfaceCount &= ~0x8000;}
    JSBool IsMarked() const {return (JSBool)(mInterfaceCount & 0x8000);}

#ifdef DEBUG
    inline void ASSERT_NotMarked();
#endif

    void DebugDump(PRInt16 depth);

    static void DestroyInstance(XPCNativeSet* inst);

private:
    static XPCNativeSet* NewInstance(XPCCallContext& ccx, 
                                     XPCNativeInterface** array, 
                                     PRUint16 count);
    static XPCNativeSet* NewInstanceMutate(XPCNativeSet*       otherSet,
                                           XPCNativeInterface* newInterface,
                                           PRUint16            position);
    XPCNativeSet() {}
    ~XPCNativeSet() {}
    void* operator new(size_t, void* p) {return p;}

private:
    PRUint16                mMemberCount;
    PRUint16                mInterfaceCount;
    XPCNativeInterface*     mInterfaces[1];  // always last - object sized for array
};

/***************************************************************************/

class XPCNativeScriptableInfo
{
public:
    nsIXPCScriptable* GetScriptable() const {return mScriptable;}
    JSUint32          GetFlags() const      {return mFlags;}
    JSClass*          GetJSClass()          {return &mJSClass;}

    JSBool            BuildJSClass();
    
    void              SetScriptable(nsIXPCScriptable* s)
                                {NS_ASSERTION(!ClassBuilt(), "too late!");
                                 mScriptable = s;}

    void              SetFlags(JSUint32 f)
                                {NS_ASSERTION(!ClassBuilt(), "too late!"); 
                                 mFlags = f;}

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return (JSBool)(mFlags & nsIXPCScriptable:: f_ );}

    JSBool WantPreCreate()                GET_IT(WANT_PRECREATE)
    JSBool WantCreate()                   GET_IT(WANT_CREATE)
    JSBool WantAddProperty()              GET_IT(WANT_ADDPROPERTY)
    JSBool WantDelProperty()              GET_IT(WANT_DELPROPERTY)
    JSBool WantGetProperty()              GET_IT(WANT_GETPROPERTY)
    JSBool WantSetProperty()              GET_IT(WANT_SETPROPERTY)
    JSBool WantEnumerate()                GET_IT(WANT_ENUMERATE)
    JSBool WantNewEnumerate()             GET_IT(WANT_NEWENUMERATE)
    JSBool WantNewResolve()               GET_IT(WANT_NEWRESOLVE)
    JSBool WantConvert()                  GET_IT(WANT_CONVERT)
    JSBool WantFinalize()                 GET_IT(WANT_FINALIZE)
    JSBool WantCheckAccess()              GET_IT(WANT_CHECKACCESS)
    JSBool WantCall()                     GET_IT(WANT_CALL)
    JSBool WantConstruct()                GET_IT(WANT_CONSTRUCT)
    JSBool WantHasInstance()              GET_IT(WANT_HASINSTANCE)
    JSBool WantMark()                     GET_IT(WANT_MARK)
    JSBool UseJSStubForAddProperty()      GET_IT(USE_JSSTUB_FOR_ADDPROPERTY)
    JSBool UseJSStubForDelProperty()      GET_IT(USE_JSSTUB_FOR_DELPROPERTY)
    JSBool UseJSStubForSetProperty()      GET_IT(USE_JSSTUB_FOR_SETPROPERTY)
    JSBool DontEnumStaticProps()          GET_IT(DONT_ENUM_STATIC_PROPS)
    JSBool DontEnumQueryInterface()       GET_IT(DONT_ENUM_QUERY_INTERFACE)
    JSBool DontAskInstanceForScriptable() GET_IT(DONT_ASK_INSTANCE_FOR_SCRIPTABLE)
    JSBool ClassInfoInterfacesOnly()      GET_IT(CLASSINFO_INTERFACES_ONLY)
    JSBool AllowPropModsDuringResolve()   GET_IT(ALLOW_PROP_MODS_DURING_RESOLVE)
    JSBool AllowPropModsToPrototype()     GET_IT(ALLOW_PROP_MODS_TO_PROTOTYPE)

#undef GET_IT

    ~XPCNativeScriptableInfo();
    XPCNativeScriptableInfo(nsIXPCScriptable* scriptable = nsnull, 
                            JSUint32 flags = 0);

    XPCNativeScriptableInfo* Clone() const 
        {return new XPCNativeScriptableInfo(mScriptable, mFlags);}

private:
    JSBool ClassBuilt() const {return mJSClass.name != 0;} 

    // disable copy ctor and assignment
    XPCNativeScriptableInfo(const XPCNativeScriptableInfo& r); // not implemented
    XPCNativeScriptableInfo& operator= (const XPCNativeScriptableInfo& r); // not implemented

private:
    nsCOMPtr<nsIXPCScriptable> mScriptable;
    JSUint32                   mFlags;
    JSClass                    mJSClass;
};

/***********************************************/

class XPCWrappedNativeProto
{
public:
    static XPCWrappedNativeProto* 
    GetNewOrUsed(XPCCallContext& ccx,
                 XPCWrappedNativeScope* Scope,
                 nsIClassInfo* ClassInfo,
                 const XPCNativeScriptableInfo* scriptableInfo);

    static XPCWrappedNativeProto* 
    BuildOneOff(XPCCallContext& ccx,
                XPCWrappedNativeScope* Scope,
                XPCNativeSet* Set);

    XPCWrappedNativeScope*   GetScope()   const {return mScope;}
    XPCJSRuntime*            GetRuntime() const {return mScope->GetRuntime();}

    JSObject*                GetJSProtoObject() const {return mJSProtoObject;}
    nsIClassInfo*            GetClassInfo()     const {return mClassInfo;}
    XPCNativeSet*            GetSet()           const {return mSet;}

    XPCNativeScriptableInfo* GetScriptableInfo()   {return mScriptableInfo;}
    void**                   GetSecurityInfoAddr() {return &mSecurityInfo;}

    JSUint32                 GetClassInfoFlags() const {return mClassInfoFlags;}

    JSBool                   IsShared() const {return nsnull != mClassInfo.get();}
    void                     AddRef();
    void                     Release();

#ifdef GET_IT
#undef GET_IT
#endif
#define GET_IT(f_) const {return (JSBool)(mClassInfoFlags & nsIClassInfo:: f_ );}

    JSBool ClassIsSingleton()           GET_IT(SINGLETON)
    JSBool ClassIsThreadSafe()          GET_IT(THREADSAFE)
    JSBool ClassIsMainThreadOnly()      GET_IT(MAIN_THREAD_ONLY)
    JSBool ClassIsDOMObject()           GET_IT(DOM_OBJECT)

#undef GET_IT
    
    XPCLock* GetLock() const 
        {return ClassIsThreadSafe() ? GetRuntime()->GetMapLock() : nsnull;}

    void SetScriptableInfo(XPCNativeScriptableInfo* si)
        {NS_ASSERTION(!mScriptableInfo, "leak here!"); mScriptableInfo = si;}

    void JSProtoObjectFinalized(JSContext *cx, JSObject *obj);

    void SystemIsBeingShutDown(XPCCallContext& ccx);

    void DebugDump(PRInt16 depth);

    void MarkSet() const {mSet->Mark();} 

#ifdef DEBUG
    void ASSERT_SetNotMarked() const {mSet->ASSERT_NotMarked();} 
#endif

private:
    // disable copy ctor and assignment
    XPCWrappedNativeProto(const XPCWrappedNativeProto& r); // not implemented
    XPCWrappedNativeProto& operator= (const XPCWrappedNativeProto& r); // not implemented

    // hide ctor and dtor
    XPCWrappedNativeProto(XPCWrappedNativeScope* Scope,
                          nsIClassInfo* ClassInfo,
                          XPCNativeSet* Set);

    ~XPCWrappedNativeProto();

    JSBool Init(XPCCallContext& ccx,
                const XPCNativeScriptableInfo* scriptableInfo);

private:
#ifdef DEBUG
    static PRInt32 gDEBUG_LiveProtoCount;        
#endif

private:
    XPCWrappedNativeScope*   mScope;
    JSObject*                mJSProtoObject;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    JSUint32                 mClassInfoFlags;
    XPCNativeSet*            mSet;
    void*                    mSecurityInfo;
    XPCNativeScriptableInfo* mScriptableInfo;
    nsrefcnt                 mRefCnt;
};


/***********************************************/

class XPCWrappedNativeTearOff
{
public:
    XPCNativeInterface* GetInterface() const {return mInterface;}
    nsISupports*        GetNative()    const {return mNative;}
    JSObject*           GetJSObject()  const {return mJSObject;}

    void SetInterface(XPCNativeInterface*  Interface) {mInterface = Interface;}
    void SetNative(nsISupports*  Native)              {mNative = Native;}
    void SetJSObject(JSObject*  JSObject)             {mJSObject = JSObject;}

    void JSObjectFinalized() {mJSObject = nsnull;}

    XPCWrappedNativeTearOff()
        : mJSObject(nsnull), mNative(nsnull), mInterface(nsnull) {}
    ~XPCWrappedNativeTearOff() 
        {NS_ASSERTION(!(mInterface||mNative||mJSObject), "tearoff not empty in dtor");}

    void Mark()       {mJSObject = (JSObject*)(((jsword)mJSObject) | 1);}
    void Unmark()     {mJSObject = (JSObject*)(((jsword)mJSObject) & ~1);}
    JSBool IsMarked() const {return (JSBool)(((jsword)mJSObject) & 1);}

private:
    XPCWrappedNativeTearOff(const XPCWrappedNativeTearOff& r); // not implemented
    XPCWrappedNativeTearOff& operator= (const XPCWrappedNativeTearOff& r); // not implemented

private:
    XPCNativeInterface* mInterface;
    nsISupports*        mNative;
    JSObject*           mJSObject;
};

/***********************************************/

#define XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK 3

class XPCWrappedNativeTearOffChunk
{
friend class XPCWrappedNative;
private:
    XPCWrappedNativeTearOffChunk() : mNextChunk(nsnull) {}
    ~XPCWrappedNativeTearOffChunk() {delete mNextChunk;}

private:
    XPCWrappedNativeTearOff mTearOffs[XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK];
    XPCWrappedNativeTearOffChunk* mNextChunk;
};

class XPCWrappedNative : public nsIXPConnectWrappedNative
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCONNECTJSOBJECTHOLDER
    NS_DECL_NSIXPCONNECTWRAPPEDNATIVE

    static nsresult
    GetNewOrUsed(XPCCallContext& ccx,
                 nsISupports* Object,
                 XPCWrappedNativeScope* Scope,
                 XPCNativeInterface* Interface,
                 XPCWrappedNative** wrapper);

    static nsresult
    GetUsedOnly(XPCCallContext& ccx,
                nsISupports* Object,
                XPCWrappedNativeScope* Scope,
                XPCNativeInterface* Interface,
                XPCWrappedNative** wrapper);

    static XPCWrappedNative*
    GetWrappedNativeOfJSObject(JSContext* cx, JSObject* obj,
                               JSObject** pobj2 = nsnull,
                               XPCWrappedNativeTearOff** pTearOff = nsnull);

    XPCWrappedNativeProto* GetProto()          const {return mProto;}
    nsISupports*           GetIdentityObject() const {return mIdentity;}
    JSObject*              GetFlatJSObject()   const {return mFlatJSObject;}

    XPCNativeSet* GetSet() const 
        {XPCAutoLock al(GetLock()); return mSet;}

private:
    void SetSet(XPCNativeSet* set)
        {XPCAutoLock al(GetLock()); mSet = set;}
public:

    void** GetSecurityInfoAddr() const {return mProto->GetSecurityInfoAddr();}
    nsIClassInfo* GetClassInfo() const {return mProto->GetClassInfo();}

    XPCLock* GetLock() const {return mProto->GetLock();}

    // XXX the rules may change here...
    JSBool IsValid() const {return nsnull != mFlatJSObject;}
    JSBool HasSharedProto() const {return GetProto()->IsShared();}
    XPCWrappedNativeScope* GetScope() const {return GetProto()->GetScope();}

    JSBool HasMutatedSet() const {return GetSet() != GetProto()->GetSet();}

    XPCNativeScriptableInfo* GetScriptableInfo() const {return mScriptableInfo;}
    nsIXPCScriptable* GetScriptable() const  {return mScriptableInfo->GetScriptable();}

    void FlatJSObjectFinalized(JSContext *cx, JSObject *obj);

    void SystemIsBeingShutDown(XPCCallContext& ccx);

#ifdef XPC_DETECT_LEADING_UPPERCASE_ACCESS_ERRORS
    // This will try to find a member that is of the form "camelCased"
    // but was accessed from JS using "CamelCased". This is here to catch
    // mistakes caused by the confusion magnet that JS methods are by
    // convention 'foo' while C++ members are by convention 'Foo'.
    static void
    HandlePossibleNameCaseError(XPCCallContext& ccx,
                                XPCNativeSet* set, 
                                XPCNativeInterface* iface,
                                jsval name);
    static void
    HandlePossibleNameCaseError(JSContext* cx,
                                XPCNativeSet* set, 
                                XPCNativeInterface* iface,
                                jsval name);

#define  HANDLE_POSSIBLE_NAME_CASE_ERROR(context, set, iface, name) \
    XPCWrappedNative::HandlePossibleNameCaseError(context, set, iface, name)
#else
#define  HANDLE_POSSIBLE_NAME_CASE_ERROR(context, set, iface, name) ((void)0)
#endif

    enum CallMode {CALL_METHOD, CALL_GETTER, CALL_SETTER};

    static JSBool CallMethod(XPCCallContext& ccx,
                             CallMode mode = CALL_METHOD);

    static JSBool GetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_GETTER);}

    static JSBool SetAttribute(XPCCallContext& ccx)
        {return CallMethod(ccx, CALL_SETTER);}

    inline JSBool HasInterfaceNoQI(XPCNativeInterface* aInterface);
    inline JSBool HasInterfaceNoQI(const nsIID& iid);

    inline XPCWrappedNativeTearOff* FindTearOff(XPCCallContext& ccx, 
                                                XPCNativeInterface* aInterface,
                                                JSBool needJSObject = JS_FALSE);

    void MarkSets() const {mSet->Mark(); GetProto()->MarkSet();}

#ifdef DEBUG
    void ASSERT_SetsNotMarked() const
        {mSet->ASSERT_NotMarked(); GetProto()->ASSERT_SetNotMarked();}
#endif

    inline void SweepTearOffs();

    // Returns a string that shuld be free'd using JS_smprintf_free (or null).
    char* ToString(XPCCallContext& ccx, 
                   XPCWrappedNativeTearOff* to = nsnull) const;

    // Make ctor and dtor protected (rather than private) to placate nsCOMPtr.
protected:
    XPCWrappedNative(nsISupports* aIdentity,
                     XPCWrappedNativeProto* aProto);
    virtual ~XPCWrappedNative();

private:
    JSBool Init(XPCCallContext& ccx, JSObject* parent,
                const XPCNativeScriptableInfo& scriptableInfo);

    JSBool ExtendSet(XPCCallContext& ccx, XPCNativeInterface* aInterface);
    
    JSBool InitTearOff(XPCCallContext& ccx,
                       XPCWrappedNativeTearOff* aTearOff,
                       XPCNativeInterface* aInterface,
                       JSBool needJSObject);
     
    JSBool InitTearOffJSObject(XPCCallContext& ccx, 
                               XPCWrappedNativeTearOff* to);

    static nsresult GatherScriptableInfo(nsISupports* obj,
                                         nsIClassInfo* classInfo,
                                         XPCNativeScriptableInfo* siProto,
                                         XPCNativeScriptableInfo* siWrapper);

    XPCJSRuntime* GetRuntime() const {return mProto->GetScope()->GetRuntime();}

private:
    XPCWrappedNativeProto* mProto;
    XPCNativeSet*          mSet;
    nsISupports*           mIdentity;
    JSObject*              mFlatJSObject;

    XPCNativeScriptableInfo* mScriptableInfo;

    XPCWrappedNativeTearOffChunk mFirstChunk;

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
public:
    PRThread*          mThread; // Don't want to overload _mOwningThread
    static PRThread*   gMainThread;
#endif
};

/***************************************************************************/
// Inlines use the above - include last.

#include "xpcinlines.h"

/***************************************************************************/
// Maps have inlines that use the above - include last.

#include "xpcmaps.h"

/***************************************************************************/

#endif /* xpcprivate_h___ */
