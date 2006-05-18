/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set ts=4 sw=4 sts=4 ci et: */
/*
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * ***** END LICENSE BLOCK *****
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 04/20/2000       IBM Corp.      Added PR_CALLBACK for Optlink use in OS2
 */

#include "nsProxyEvent.h"
#include "nsProxyEventPrivate.h"
#include "nsIProxyObjectManager.h"
#include "nsCRT.h"

#include "pratom.h"
#include "prmem.h"
#include "xptcall.h"

#include "nsXPCOMCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIComponentManager.h"
#include "nsIThreadInternal.h"
#include "nsThreadUtils.h"
#include "nsEventQueue.h"
#include "nsMemory.h"

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("xpcomproxy");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

/**
 * Map the nsAUTF8String, nsUTF8String classes to the nsACString and
 * nsCString classes respectively for now.  These defines need to be removed
 * once Jag lands his nsUTF8String implementation.
 */
#define nsAUTF8String nsACString
#define nsUTF8String nsCString

static NS_DEFINE_IID(kProxyObject_Identity_Class_IID, NS_PROXYEVENT_IDENTITY_CLASS_IID);

//-----------------------------------------------------------------------------

#define NS_PROXYEVENT_IID                             \
{ /* 9a24dc5e-2b42-4a5a-aeca-37b8c8fd8ccd */          \
    0x9a24dc5e,                                       \
    0x2b42,                                           \
    0x4a5a,                                           \
    {0xae, 0xca, 0x37, 0xb8, 0xc8, 0xfd, 0x8c, 0xcd}  \
}

class nsProxyEvent : public nsIRunnable
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXYEVENT_IID) 
    NS_DECL_ISUPPORTS

    nsProxyEvent(nsProxyObjectCallInfo *info = nsnull)
        : mInfo(info)
    {}

    void SetInfo(nsProxyObjectCallInfo *info)
    {
        NS_ASSERTION(info, "null info");
        NS_ASSERTION(!mInfo, "unexpected change to mInfo");
        mInfo = info;
    }

    PRBool IsSyncProxyEvent()
    {
        return mInfo->GetProxyObject()->GetProxyType() & NS_PROXY_SYNC;
    }

protected:
    nsProxyObjectCallInfo *mInfo;
};
NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyEvent, NS_PROXYEVENT_IID)
NS_IMPL_THREADSAFE_ISUPPORTS2(nsProxyEvent, nsProxyEvent, nsIRunnable)

//-----------------------------------------------------------------------------

class nsProxyThreadFilter : public nsIThreadEventFilter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITHREADEVENTFILTER
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsProxyThreadFilter, nsIThreadEventFilter)

NS_IMETHODIMP_(PRBool)
nsProxyThreadFilter::AcceptEvent(nsIRunnable *event)
{
    LOG(("PROXY(%p): filter event [%p]\n", this, event));

    // If we encounter one of our proxy events that is for a synchronous method
    // call, then we want to put it in our event queue for processing.  Else,
    // we want to allow the event to be dispatched to the thread's event queue
    // for processing later once we complete the current sync method call.
    
    nsCOMPtr<nsProxyEvent> pe = do_QueryInterface(event);
    return pe && pe->IsSyncProxyEvent();
}

//-----------------------------------------------------------------------------

class nsProxyCallEvent : public nsProxyEvent
{
public:
    nsProxyCallEvent()
        : nsProxyEvent()
    {}

    NS_IMETHOD Run()
    {
        NS_ASSERTION(mInfo, "no info");
        Invoke();
        if (IsSyncProxyEvent()) {
            mInfo->PostCompleted();
        } else {
            delete mInfo;
            mInfo = nsnull;
        }
        return NS_OK;
    }

    void Invoke()
    {
        LOG(("PROXY(%p): Invoke\n", this));

        nsProxyObject *proxyObject = mInfo->GetProxyObject();
        if (proxyObject)
        {
            // invoke the magic of xptc...
            nsresult rv = XPTC_InvokeByIndex(proxyObject->GetRealObject(), 
                                             mInfo->GetMethodIndex(),
                                             mInfo->GetParameterCount(), 
                                             mInfo->GetParameterList());
            mInfo->SetResult(rv);
        }
        else
        {
            mInfo->SetResult(NS_ERROR_OUT_OF_MEMORY);
        }
    }
};

//-----------------------------------------------------------------------------

class nsProxyCallCompletedEvent : public nsProxyEvent
{
public:
    nsProxyCallCompletedEvent(nsProxyObjectCallInfo *info)
        : nsProxyEvent(info)
    {}

    NS_IMETHOD Run()
    {
        NS_ASSERTION(mInfo, "no info");
        mInfo->SetCompleted();
        return NS_OK;
    }
};

//-----------------------------------------------------------------------------

class nsProxyDestructorEvent : public nsRunnable
{
public:
    nsProxyDestructorEvent(nsProxyObject *doomed)
        : mDoomed(doomed)
    {}

    NS_IMETHOD Run()
    {
        delete mDoomed;
        return NS_OK;
    }

private:
    nsProxyObject *mDoomed;
};

//-----------------------------------------------------------------------------

nsProxyObjectCallInfo::nsProxyObjectCallInfo(nsProxyObject* owner,
                                             nsXPTMethodInfo *methodInfo,
                                             PRUint32 methodIndex, 
                                             nsXPTCVariant* parameterList, 
                                             PRUint32 parameterCount,
                                             nsIInterfaceInfo *interfaceInfo,
                                             nsIRunnable *event)
{
    NS_ASSERTION(owner, "No nsProxyObject!");
    NS_ASSERTION(methodInfo, "No nsXPTMethodInfo!");
    NS_ASSERTION(event, "No PLEvent!");
    
    mCompleted        = 0;
    mMethodIndex      = methodIndex;
    mParameterList    = parameterList;
    mParameterCount   = parameterCount;
    mInterfaceInfo    = interfaceInfo;
    mEvent            = event; // XXX strong or weak ref?
    mMethodInfo       = methodInfo;
    mCallersTarget    = nsnull;

    mOwner            = owner;
    
    if (mOwner->GetProxyType() & NS_PROXY_ASYNC)
        CopyStrings(PR_TRUE);
}


nsProxyObjectCallInfo::~nsProxyObjectCallInfo()
{
    if ((mOwner->GetProxyType() & NS_PROXY_ASYNC)) {
        if (!(mOwner->GetProxyType() & NS_PROXY_AUTOPROXIFY))
            RefCountInInterfacePointers(PR_FALSE);
         CopyStrings(PR_FALSE);
    }

    mOwner = nsnull;
    
    if (mParameterList)  
        free((void*) mParameterList);
}

nsresult
nsProxyObjectCallInfo::Init()
{
    if (mOwner->GetProxyType() & NS_PROXY_AUTOPROXIFY) {
        nsresult rv;
        rv = mOwner->AutoproxifyInParameterList(mParameterList, mParameterCount,
                                                mMethodInfo, mMethodIndex, mInterfaceInfo);
        if (NS_FAILED(rv))
            return rv;
    }
    else if (mOwner->GetProxyType() & NS_PROXY_ASYNC)
        RefCountInInterfacePointers(PR_TRUE);

    return NS_OK;
}

void
nsProxyObjectCallInfo::RefCountInInterfacePointers(PRBool addRef)
{
    for (PRUint32 i = 0; i < mParameterCount; i++)
    {
        nsXPTParamInfo paramInfo = mMethodInfo->GetParam(i);

        if (paramInfo.GetType().IsInterfacePointer() )
        {
            nsISupports* anInterface = nsnull;

            if (paramInfo.IsIn())
            {
                anInterface = ((nsISupports*)mParameterList[i].val.p);
                
                if (anInterface)
                {
                    if (addRef)
                        anInterface->AddRef();
                    else
                        anInterface->Release();
            
                }
            }
        }
    }
}

void
nsProxyObjectCallInfo::CopyStrings(PRBool copy)
{
    for (PRUint32 i = 0; i < mParameterCount; i++)
    {
        const nsXPTParamInfo paramInfo = mMethodInfo->GetParam(i);

        if (paramInfo.IsIn())
        {
            const nsXPTType& type = paramInfo.GetType();
            uint8 type_tag = type.TagPart();
            void *ptr = mParameterList[i].val.p;

            if (!ptr)
                continue;

            if (copy)
            {                
                switch (type_tag) 
                {
                    case nsXPTType::T_CHAR_STR:                                
                        mParameterList[i].val.p =
                            PL_strdup((const char *)ptr);
                        break;
                    case nsXPTType::T_WCHAR_STR:
                        mParameterList[i].val.p =
                            nsCRT::strdup((const PRUnichar *)ptr);
                        break;
                    case nsXPTType::T_DOMSTRING:
                    case nsXPTType::T_ASTRING:
                        mParameterList[i].val.p = 
                            new nsString(*((nsAString*) ptr));
                        break;
                    case nsXPTType::T_CSTRING:
                        mParameterList[i].val.p = 
                            new nsCString(*((nsACString*) ptr));
                        break;
                    case nsXPTType::T_UTF8STRING:                        
                        mParameterList[i].val.p = 
                            new nsUTF8String(*((nsAUTF8String*) ptr));
                        break;
                    default:
                        // Other types are ignored
                        break;                    
                }
            }
            else
            {
                switch (type_tag) 
                {
                    case nsXPTType::T_CHAR_STR:
                    case nsXPTType::T_WCHAR_STR:
                        PL_strfree((char*) ptr);
                        break;
                    case nsXPTType::T_DOMSTRING:
                    case nsXPTType::T_ASTRING:
                        delete (nsString*) ptr;
                        break;
                    case nsXPTType::T_CSTRING:
                        delete (nsCString*) ptr;
                        break;
                    case nsXPTType::T_UTF8STRING:
                        delete (nsUTF8String*) ptr;
                        break;
                    default:
                        // Other types are ignored
                        break;
                }
            }
        }
    }
}

PRBool                
nsProxyObjectCallInfo::GetCompleted()
{
    return (PRBool)mCompleted;
}

void
nsProxyObjectCallInfo::SetCompleted()
{
    LOG(("PROXY(%p): SetCompleted\n", this));
    PR_AtomicSet(&mCompleted, 1);
}

void                
nsProxyObjectCallInfo::PostCompleted()
{
    LOG(("PROXY(%p): PostCompleted\n", this));

    // perform autoproxification cleanup and proxification of 'out'
    // params on proxied object's thread:
    if (mOwner->GetProxyType() & NS_PROXY_AUTOPROXIFY) {
        PRBool bAsync = mOwner->GetProxyType() & NS_PROXY_ASYNC;
        mOwner->AutoproxifyOutParameterList(mParameterList, mParameterCount,
                                            mMethodInfo, mMethodIndex,
                                            mInterfaceInfo,
                                            !bAsync && NS_SUCCEEDED(mResult));
    }
    
    if (mCallersTarget) {
        nsCOMPtr<nsIRunnable> event =
                new nsProxyCallCompletedEvent(this);
        if (event &&
            NS_SUCCEEDED(mCallersTarget->Dispatch(event, NS_DISPATCH_NORMAL)))
            return;
    }

    // OOM?  caller does not have a target?  This is an error!
    NS_WARNING("Failed to dispatch nsProxyCallCompletedEvent");
    SetCompleted();
}
  
nsIEventTarget*      
nsProxyObjectCallInfo::GetCallersTarget() 
{ 
    return mCallersTarget;
}

void
nsProxyObjectCallInfo::SetCallersTarget(nsIEventTarget* target)
{
    mCallersTarget = target;
}   

nsProxyObject::nsProxyObject(nsIEventTarget *target, PRInt32 proxyType,
                             nsISupports *realObject)
{
    mRealObject      = realObject;
    mTarget          = target;
    mProxyType       = proxyType;
}

nsProxyObject::~nsProxyObject()
{   
    // I am worried about order of destruction here.  
    // do not remove assignments.
    
    mRealObject = 0;
    mTarget  = 0;
}

void
nsProxyObject::AddRef()
{
  PR_AtomicIncrement((PRInt32 *)&mRefCnt);
  NS_LOG_ADDREF(this, mRefCnt, "nsProxyObject", sizeof(*this));
}

void
nsProxyObject::Release(void)
{
    NS_PRECONDITION(0 != mRefCnt, "dup release");             

    nsrefcnt count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
    NS_LOG_RELEASE(this, count, "nsProxyObject");

    if (count == 0)
    {
        mRefCnt = 1; /* stabilize */

        PRBool callDirectly;
        mTarget->IsOnCurrentThread(&callDirectly);

        if (callDirectly)
        {
            delete this;
            return;
        }

        // need to do something special here so that
        // the real object will always be deleted on
        // the correct thread..

        nsCOMPtr<nsIRunnable> event = new nsProxyDestructorEvent(this);
        if (event == nsnull)
        {
            NS_NOTREACHED("Leaking nsProxyObject!");
            return;  // if this happens we are going to leak.
        }

        if (NS_FAILED(mTarget->Dispatch(event, NS_DISPATCH_NORMAL)))
            NS_WARNING("Failed to dispatch nsProxyDestructorEvent");
    }
}


nsresult
nsProxyObject::PostAndWait(nsProxyObjectCallInfo *proxyInfo)
{
    LOG(("PROXY(%p): PostAndWait enter [%p]\n", this, proxyInfo));

    NS_ENSURE_ARG_POINTER(proxyInfo);

    nsIRunnable* event = proxyInfo->GetEvent();
    if (!event)
        return NS_ERROR_NULL_POINTER;

    nsIThread *thread = NS_GetCurrentThread();
    nsCOMPtr<nsIThreadInternal> threadInt = do_QueryInterface(thread);
    NS_ENSURE_STATE(threadInt);

    // Install thread filter to limit event processing only to subclasses of
    // nsProxyEvent.  XXX Add support for sequencing?
    nsRefPtr<nsProxyThreadFilter> filter = new nsProxyThreadFilter();
    if (!filter)
        return NS_ERROR_OUT_OF_MEMORY;
    threadInt->PushEventQueue(filter);

    proxyInfo->SetCallersTarget(thread);
    
    // Dispatch can fail if the thread is shutting down
    nsresult rv = mTarget->Dispatch(event, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv)) {
        while (!proxyInfo->GetCompleted()) {
            if (!NS_ProcessNextEvent(thread)) {
                rv = NS_ERROR_UNEXPECTED;
                break;
            }
        }
    } else {
        NS_WARNING("Failed to dispatch nsProxyCallEvent");
    }

    threadInt->PopEventQueue();

    LOG(("PROXY(%p): PostAndWait exit [%p %x]\n", this, proxyInfo, rv));
    return rv;
}
        
        
nsresult
nsProxyObject::convertMiniVariantToVariant(nsXPTMethodInfo *methodInfo, 
                                           nsXPTCMiniVariant * params, 
                                           nsXPTCVariant **fullParam, 
                                           uint8 *outParamCount)
{
    uint8 paramCount = methodInfo->GetParamCount();
    *outParamCount = paramCount;
    *fullParam = nsnull;

    if (!paramCount) return NS_OK;
        
    *fullParam = (nsXPTCVariant*)malloc(sizeof(nsXPTCVariant) * paramCount);
    
    if (*fullParam == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    
    for (int i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
        if ((mProxyType & NS_PROXY_ASYNC) && paramInfo.IsDipper())
        {
            NS_WARNING("Async proxying of out parameters is not supported"); 
            free(*fullParam);
            return NS_ERROR_PROXY_INVALID_OUT_PARAMETER;
        }
        uint8 flags = paramInfo.IsOut() ? nsXPTCVariant::PTR_IS_DATA : 0;
        (*fullParam)[i].Init(params[i], paramInfo.GetType(), flags);
    }
    
    return NS_OK;
}

// Helper returning the iid for a given method parameter. 
static PRBool GetIIDForParam(nsXPTCVariant *params, PRUint32 paramIndex, nsXPTMethodInfo *methodInfo,
                             PRUint32 methodIndex, nsIInterfaceInfo* interfaceInfo,
                             nsIID*retval)
{
    nsXPTParamInfo paramInfo = methodInfo->GetParam(paramIndex);
    if (paramInfo.GetType().TagPart() == nsXPTType::T_INTERFACE_IS) {
        uint8 iidIndex;
        if (NS_FAILED(interfaceInfo->GetInterfaceIsArgNumberForParam(methodIndex, &paramInfo, &iidIndex))) {
            NS_ERROR("Could not get index of iid arg");
            return PR_FALSE;
        }
         nsXPTParamInfo iidParam = methodInfo->GetParam(iidIndex);
         NS_ASSERTION(iidParam.GetType().TagPart() == nsXPTType::T_IID, "iid parameter error");
         if (iidParam.IsOut()) {
             nsIID** p = (nsIID**)params[iidIndex].ptr;
             if (!p || !*p) return PR_FALSE;
             *retval = **p;
         }
         else {
             // in parameter
             nsIID* p = (nsIID*)params[iidIndex].val.p;
             if (!p) return PR_FALSE;
             *retval = *p;
         }
    }
    else { // nsXPTType::T_INTERFACE
        if (NS_FAILED(interfaceInfo->GetIIDForParamNoAlloc(methodIndex, &paramInfo, retval)))
            return PR_FALSE;
    }
    return PR_TRUE;
}

nsresult
nsProxyObject::AutoproxifyInParameterList(nsXPTCVariant *params, uint8 paramCount,
                                          nsXPTMethodInfo *methodInfo, PRUint32 methodIndex,
                                          nsIInterfaceInfo* interfaceInfo)
{
    for (uint8 i=0; i<paramCount; ++i)
    {
        if (!params[i].type.IsInterfacePointer())
            continue;

        if (params[i].IsPtrData()) {
            // this is an 'out' or 'inout' parameter
            NS_ASSERTION(!methodInfo->GetParam(i).IsIn(), "inout parameter will not be autoproxied!");

            continue;
        }

        nsISupports* itf = (nsISupports*)params[i].val.p;

        if (itf == nsnull)
            continue;
        
        // check if this parameter is already proxied:
        nsCOMPtr<nsISupports> identObj;
        itf->QueryInterface(kProxyObject_Identity_Class_IID, getter_AddRefs(identObj));

        if (identObj) {
            // this parameter is already proxied; we'll assume that it
            // wants to be accessed via this existing proxy
            continue;
        }
        
        nsIID iid;
      
        if (!GetIIDForParam(params, i, methodInfo, methodIndex, interfaceInfo, &iid)) {
            NS_WARNING("Couldn't get IID for parameter");
            return NS_ERROR_FAILURE;
        }

        // get a proxy for the given object:
        nsISupports* proxy;
        
        NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD, iid, itf,
                             GetProxyType(), (void**)&proxy);

        if (!proxy) {
            NS_ERROR("failed to create proxy");
            return NS_ERROR_FAILURE;
        }

        // stuff proxy into val.p and mark parameter as needing cleanup
        params[i].val.p = proxy;
        params[i].SetValIsProxied();
    }
    
    return NS_OK;
}

nsresult
nsProxyObject::AutoproxifyOutParameterList(nsXPTCVariant *params, uint8 paramCount,
                                           nsXPTMethodInfo *methodInfo, PRUint32 methodIndex, 
                                           nsIInterfaceInfo* interfaceInfo, PRBool proxifyOutPars)
{
    nsresult rv = NS_OK;
    
    for (uint8 i=0; i<paramCount; ++i)
    {
        if (!params[i].type.IsInterfacePointer())
            continue;

        if (params[i].IsPtrData() && proxifyOutPars && params[i].ptr != nsnull) {
            if(methodInfo->GetParam(i).IsIn()) {
                NS_WARNING("inout parameter will not be autoproxied!");
                continue;
            }
            
            // this is an out parameter that needs to be proxied
            nsISupports* itf = *((nsISupports**)params[i].ptr);

            if (itf == nsnull)
                continue;

            // check if this parameter is already proxied:
            nsCOMPtr<nsISupports> identObj;
            itf->QueryInterface(kProxyObject_Identity_Class_IID, getter_AddRefs(identObj));

            if (identObj) {
                // this parameter is already proxied; we'll assume
                // that it want to be accessed via this existing proxy
                continue;
            }

            nsIID iid;
            if (!GetIIDForParam(params, i, methodInfo, methodIndex, interfaceInfo, &iid)) {
                NS_WARNING("couldn't get iid for parameter");
                // we want to continue despite the failure so that
                // cleanup of proxied 'in' pars is still taken care
                // of.
                rv = NS_ERROR_FAILURE;
                continue;
            }

            // get a proxy for the given object:
            nsISupports* proxy;
            
            NS_GetProxyForObject(mTarget, iid, itf,
                                 GetProxyType(), (void**)&proxy);

            if (!proxy) {
                NS_ERROR("failed to create proxy");
                // continue; see comment above
                rv = NS_ERROR_FAILURE;
                continue;
            }

            // stuff proxy into parameter:
            *((nsISupports**)params[i].ptr) = proxy;

            // release original obj so that proxy gets ownership:
            itf->Release();
        }
        else if (params[i].IsValProxied()) {
            // this is a proxied 'in' parameter needing cleanup

            nsProxyEventObject* proxy = (nsProxyEventObject*)params[i].val.p;
            NS_ASSERTION(proxy, "uh-oh. null proxy? this can't happen.");
            
            // replace by real object and release proxy:
            params[i].val.p = proxy->GetRealObject();

            proxy->Release();
        }
    }
    return rv;
}
       
nsresult
nsProxyObject::Post( PRUint32 methodIndex, 
                     nsXPTMethodInfo *methodInfo, 
                     nsXPTCMiniVariant * params, 
                     nsIInterfaceInfo *interfaceInfo)            
{
    nsresult rv = NS_OK; 

    if (! mTarget  || ! mRealObject)
        return NS_ERROR_OUT_OF_MEMORY;

    if (methodInfo->IsNotXPCOM())
        return NS_ERROR_PROXY_INVALID_IN_PARAMETER;
    
    nsXPTCVariant *fullParam;
    uint8 paramCount; 
    rv = convertMiniVariantToVariant(methodInfo, params, &fullParam, &paramCount);
    
    if (NS_FAILED(rv))
        return rv;

    PRBool callDirectly;

    // see if we should call into the method directly. Either it is a QI function call
    // (methodIndex == 0), or it is a sync proxy and this code is running on the same thread
    // as the destination dispatch target.
    if ( (methodIndex == 0) ||
         (mProxyType & NS_PROXY_SYNC && 
          NS_SUCCEEDED(mTarget->IsOnCurrentThread(&callDirectly)) &&
          callDirectly))
    {
        if (mProxyType & NS_PROXY_AUTOPROXIFY)
            AutoproxifyInParameterList(fullParam, paramCount, methodInfo, methodIndex, interfaceInfo);
        
        // invoke the magic of xptc...
        nsresult rv = XPTC_InvokeByIndex( mRealObject, 
                                          methodIndex,
                                          paramCount, 
                                          fullParam);

        if (mProxyType & NS_PROXY_AUTOPROXIFY)
            AutoproxifyOutParameterList(fullParam, paramCount, methodInfo, methodIndex, interfaceInfo);
        
        if (fullParam) 
            free(fullParam);
        return rv;
    }

    nsRefPtr<nsProxyCallEvent> event = new nsProxyCallEvent();
    if (!event) {
        if (fullParam) 
            free(fullParam);
        return NS_ERROR_OUT_OF_MEMORY;   
    }
    
    nsProxyObjectCallInfo *proxyInfo =
            new nsProxyObjectCallInfo(this, 
                                      methodInfo, 
                                      methodIndex, 
                                      fullParam,   // will be deleted by ~()
                                      paramCount,
                                      interfaceInfo,
                                      event);      // will be deleted by ~()
    
    if (!proxyInfo) {
        if (fullParam)
            free(fullParam);
        return NS_ERROR_OUT_OF_MEMORY;  
    }

    if (NS_FAILED(proxyInfo->Init())) {
        if (fullParam)
            free(fullParam);
        delete proxyInfo;
        return NS_ERROR_FAILURE;
    }
    
    event->SetInfo(proxyInfo);

    if (mProxyType & NS_PROXY_SYNC)
    {
        rv = PostAndWait(proxyInfo);
        
        if (NS_SUCCEEDED(rv))
            rv = proxyInfo->GetResult();
        delete proxyInfo;
        return rv;
    }
    
    if (mProxyType & NS_PROXY_ASYNC)
    {
        mTarget->Dispatch(event, NS_DISPATCH_NORMAL);
        return NS_OK;
    }
    return NS_ERROR_UNEXPECTED;
}
