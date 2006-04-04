/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __nsProxyEventPrivate_h_
#define __nsProxyEventPrivate_h_

#include "nscore.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsHashtable.h"
#include "nsIInterfaceInfo.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

#include "plevent.h"
#include "xptcall.h"    // defines nsXPTCVariant
#include "nsIEventQueue.h"

#include "nsIProxyObjectManager.h"

class nsProxyEventObject;
class nsProxyEventClass;
class nsProxyObjectCallInfo;

#define NS_XPCOMPROXY_CLASSNAME "XPCom Proxy"

#define NS_PROXYEVENT_MANAGER_CID                \
{ 0xeea90d41, 									 \
  0xb059, 										 \
  0x11d2,						                 \
 {0x91, 0x5e, 0xc1, 0x2b, 0x69, 0x6c, 0x93, 0x33}\
} 

#define NS_PROXYEVENT_CLASS_IID                  \
{ 0xeea90d42,                                    \
  0xb059,                                        \
  0x11d2,                                        \
 {0x91, 0x5e, 0xc1, 0x2b, 0x69, 0x6c, 0x93, 0x33}\
} 

// 4a9cdd77-4025-41f6-aacc-7811d7ae37da
#define NS_PROXY_CANONICAL_OBJECT_IID \
{ 0x4a9cdd77, 0x4025, 0x41f6,    \
{0xaa, 0xcc, 0x78, 0x11, 0xd7, 0xae, 0x37, 0xda} }


class nsProxyEventClass : public nsISupports
{
public:
    NS_DECL_ISUPPORTS
    
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXYEVENT_CLASS_IID)
    static nsProxyEventClass* GetNewOrUsedClass(REFNSIID aIID);

    nsIInterfaceInfo*        GetInterfaceInfo() const {return mInfo;}
    const nsIID&             GetProxiedIID()    const {return mIID; }
protected:
    nsProxyEventClass();
    nsProxyEventClass(REFNSIID aIID, nsIInterfaceInfo* aInfo);
    
private:
    ~nsProxyEventClass();

    nsIID                      mIID;
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    uint32*                    mDescriptors;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyEventClass, NS_PROXYEVENT_CLASS_IID)

class nsProxyCanonicalObject : public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXY_CANONICAL_OBJECT_IID)

    nsProxyCanonicalObject(nsISupports* aObjectToProxy,
                           nsIEventQueue* aDestQueue,
                           PRInt32 aProxyType);

private:
    friend class nsProxyEventObject;
    friend class nsProxyObjectManager;

    // This destructor may only be called while holding the
    // nsProxyObjectManager monitor.
    ~nsProxyCanonicalObject();

    void LockedRemoveProxy(nsProxyEventObject *proxy);

    PRInt32                 mProxyType;
    nsCOMPtr<nsISupports>   mProxiedObject;
    nsCOMPtr<nsIEventQueue> mDestQueue;

    // Threadsafe access to this singly-linked list is protected by
    // the nsProxyObjectManager->GetMonitor monitor.
    nsProxyEventObject    *mFirst;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyCanonicalObject,
                              NS_PROXY_CANONICAL_OBJECT_IID)

class nsProxyEventObject : public nsIXPTCProxy
{
public:

    NS_DECL_ISUPPORTS

    // call this method and return result
    NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const XPTMethodDescriptor *info,
                          nsXPTCMiniVariant* params);

    const nsIID&       GetIID()   const { return mClass->GetIID(); }
    nsProxyEventClass* GetClass() const { return mClass; }
    nsIEventQueue*     GetQueue() const { return mRoot->mDestQueue; }
    PRInt32            GetProxyType() const { return mRoot->mProxyType; }
    nsISomeInterface*  GetRealInterface() const { return mRealInterface; }

    nsProxyEventObject(nsISomeInterface* aObj,
                       nsProxyEventClass* aClass,
                       nsProxyCanonicalObject* root);

#ifdef DEBUG_xpcom_proxy
    void DebugDump(const char * message, PRUint32 hashKey);
#endif

private:
    friend class nsProxyCanonicalObject;

    nsresult            PostAndWait(nsProxyObjectCallInfo *proxyInfo);

    nsresult convertMiniVariantToVariant(const XPTMethodDescriptor * methodInfo, 
                                         nsXPTCMiniVariant * params, 
                                         nsXPTCVariant     **fullParam, 
                                         uint8 *paramCount);
    // This destructor must only be called from within the manager monitor.
    ~nsProxyEventObject();

    nsRefPtr<nsProxyEventClass> mClass;
    nsISomeInterface*           mXPTCStub;

    // the non-proxy interface that this proxy refers to. */
    nsCOMPtr<nsISomeInterface>  mRealInterface;

    // Owning reference...
    nsRefPtr<nsProxyCanonicalObject> mRoot;

    // Weak reference, managed by our canonical root
    nsProxyEventObject *mNext;
};

#define PROXY_SYNC    0x0001  // acts just like a function call.
#define PROXY_ASYNC   0x0002  // fire and forget.  This will return immediately and you will lose all return information.
#define PROXY_ALWAYS  0x0004   // ignore check to see if the eventQ is on the same thread as the caller, and alway return a proxied object.

//#define AUTOPROXIFICATION

// WARNING about PROXY_ASYNC:  
//
// If the calling thread goes away, any function which accesses the calling stack 
// will blow up.
//
//  example:
//
//     myFoo->bar(&x)
//
//     ... thread goes away ...
//
//    bar(PRInt32 *x)
//    {
//         *x = 0;   <-----  You will blow up here.
//
//   
//  So what gets saved?  
//
//  You can safely pass base types by value.  You can also pass interface pointers.
//  I will make sure that the interface pointers are addrefed while they are being 
//  proxied.  You can also pass string and wstring.  These I will copy and free.
//
//  I do **NOT** copy arrays or strings with size.  If you are using these either
//  change your interface, or contact me about this feature request.

class nsProxyObjectCallInfo
{
public:
    
    nsProxyObjectCallInfo(nsProxyEventObject* owner,
                          const XPTMethodDescriptor *methodInfo,
                          PRUint32 methodIndex, 
                          nsXPTCVariant* parameterList, 
                          PRUint32 parameterCount, 
                          PLEvent *event);

    ~nsProxyObjectCallInfo();

    PRUint32            GetMethodIndex() const { return mMethodIndex; }
    nsXPTCVariant*      GetParameterList() const { return mParameterList; }
    PRUint32            GetParameterCount() const { return mParameterCount; }
    PLEvent*            GetPLEvent() const { return mEvent; }
    nsresult            GetResult() const { return mResult; }
    nsProxyEventObject* GetProxyObject() const { return mOwner; }

    PRBool              GetCompleted();
    void                SetCompleted();
    void                PostCompleted();

    void                SetResult(nsresult rv) {mResult = rv; }
    
    nsIEventQueue*      GetCallersQueue();
    void                SetCallersQueue(nsIEventQueue* queue);

private:
    
    nsresult         mResult;                    /* this is the return result of the called function */
    const XPTMethodDescriptor *mMethodInfo;
    PRUint32         mMethodIndex;               /* which method to be called? */
    nsXPTCVariant   *mParameterList;             /* marshalled in parameter buffer */
    PRUint32         mParameterCount;            /* number of params */
    PLEvent         *mEvent;                     /* the current plevent */       
    PRInt32          mCompleted;                 /* is true when the method has been called. */
       
    nsCOMPtr<nsIEventQueue>  mCallersEventQ;     /* this is the eventQ that we must post a message back to 
                                                    when we are done invoking the method (only PROXY_SYNC). 
                                                  */

    /* this is the strong referenced nsProxyObject */
    nsRefPtr<nsProxyEventObject> mOwner;

    void RefCountInInterfacePointers(PRBool addRef);
    void CopyStrings(PRBool copy);
};

////////////////////////////////////////////////////////////////////////////////

class nsProxyEventKey : public nsHashKey
{
public:
    nsProxyEventKey(void* rootObjectKey, void* destQueueKey, PRInt32 proxyType)
        : mRootObjectKey(rootObjectKey), mDestQueueKey(destQueueKey), mProxyType(proxyType) {
    }
  
    PRUint32 HashCode(void) const {
        return NS_PTR_TO_INT32(mRootObjectKey) ^ 
            NS_PTR_TO_INT32(mDestQueueKey) ^ mProxyType;
    }

    PRBool Equals(const nsHashKey *aKey) const {
        const nsProxyEventKey* other = (const nsProxyEventKey*)aKey;
        return mRootObjectKey == other->mRootObjectKey
            && mDestQueueKey == other->mDestQueueKey
            && mProxyType == other->mProxyType;
    }

    nsHashKey *Clone() const {
        return new nsProxyEventKey(mRootObjectKey, mDestQueueKey, mProxyType);
    }

protected:
    void*       mRootObjectKey;
    void*       mDestQueueKey;
    PRInt32     mProxyType;
};

////////////////////////////////////////////////////////////////////////////////
// nsProxyObjectManager
////////////////////////////////////////////////////////////////////////////////

class nsProxyObjectManager: public nsIProxyObjectManager
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROXYOBJECTMANAGER
        
    
    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
    
    nsProxyObjectManager();
    
    static nsProxyObjectManager *GetInstance();
    static PRBool IsManagerShutdown();

    static void Shutdown();
    
    nsHashtable* GetRealObjectToProxyObjectMap() { return &mProxyObjectMap;}   
    nsHashtable* GetIIDToProxyClassMap() { return &mProxyClassMap; }   

    PRMonitor*   GetMonitor() const { return mProxyCreationMonitor; }
    
private:
    ~nsProxyObjectManager();

    static nsProxyEventObject* GetNewOrUsedProxy(nsIEventQueue *destQueue,
                                                 PRInt32 proxyType,
                                                 nsISupports *aObj,
                                                 REFNSIID aIID);
    
    static nsProxyObjectManager* mInstance;
    nsHashtable  mProxyObjectMap;
    nsHashtable  mProxyClassMap;
    PRMonitor   *mProxyCreationMonitor;
};


#endif
