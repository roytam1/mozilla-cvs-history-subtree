/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsIEventQueueService.h"
#include "nsIThread.h"

#include "nsIAtom.h"  //hack!  Need a way to define a component as threadsafe (ie. sta).

/**
 * Map the nsAUTF8String, nsUTF8String classes to the nsACString and
 * nsCString classes respectively for now.  These defines need to be removed
 * once Jag lands his nsUTF8String implementation.
 */
#define nsAUTF8String nsACString
#define nsUTF8String nsCString

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kProxyObject_Identity_Class_IID, NS_PROXYEVENT_IDENTITY_CLASS_IID);
        
static void* PR_CALLBACK EventHandler(PLEvent *self);
static void  PR_CALLBACK DestroyHandler(PLEvent *self);
static void* PR_CALLBACK CompletedEventHandler(PLEvent *self);
static void PR_CALLBACK CompletedDestroyHandler(PLEvent *self) ;
static void* PR_CALLBACK ProxyDestructorEventHandler(PLEvent *self);
static void PR_CALLBACK ProxyDestructorDestroyHandler(PLEvent *self) ;

nsProxyObjectCallInfo::nsProxyObjectCallInfo( nsProxyObject* owner,
                                              nsXPTMethodInfo *methodInfo,
                                              PRUint32 methodIndex, 
                                              nsXPTCVariant* parameterList, 
                                              PRUint32 parameterCount, 
                                              nsIInterfaceInfo *interfaceInfo,
                                              PLEvent *event)
{
    NS_ASSERTION(owner, "No nsProxyObject!");
    NS_ASSERTION(methodInfo, "No nsXPTMethodInfo!");
    NS_ASSERTION(event, "No PLEvent!");
    
    mCompleted        = 0;
    mMethodIndex      = methodIndex;
    mParameterList    = parameterList;
    mParameterCount   = parameterCount;
    mInterfaceInfo    = interfaceInfo;
    mEvent            = event;
    mMethodInfo       = methodInfo;
    mCallersEventQ    = nsnull;

    mOwner            = owner;
    
    if (mOwner->GetProxyType() & PROXY_ASYNC)
        CopyStrings(PR_TRUE);
}

nsProxyObjectCallInfo::~nsProxyObjectCallInfo()
{
    if ((mOwner->GetProxyType() & PROXY_ASYNC)) {
        if (!(mOwner->GetProxyType() & PROXY_AUTOPROXIFY))
            RefCountInInterfacePointers(PR_FALSE);
        CopyStrings(PR_FALSE);
    }

    mOwner = nsnull;
    
    PR_FREEIF(mEvent);
    
    if (mParameterList)  
        free( (void*) mParameterList);
}

nsresult
nsProxyObjectCallInfo::Init()
{
    if (mOwner->GetProxyType() & PROXY_AUTOPROXIFY) {
        nsresult rv;
        rv = mOwner->AutoproxifyInParameterList(mParameterList, mParameterCount,
                                                mMethodInfo, mMethodIndex, mInterfaceInfo);
        if (NS_FAILED(rv))
            return rv;
    }
    else if (mOwner->GetProxyType() & PROXY_ASYNC) 
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
                    if(addRef)
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

        if(paramInfo.IsIn())
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
    PR_AtomicSet(&mCompleted, 1);
}

void                
nsProxyObjectCallInfo::PostCompleted()
{
    // perform autoproxification cleanup and proxification of 'out'
    // params on proxied object's thread:
    if (mOwner->GetProxyType() & PROXY_AUTOPROXIFY) {
        PRBool bAsync = mOwner->GetProxyType() & PROXY_ASYNC;
        mOwner->AutoproxifyOutParameterList(mParameterList, mParameterCount,
                                                      mMethodInfo, mMethodIndex,
                                                      mInterfaceInfo,
                                                      !bAsync && NS_SUCCEEDED(mResult));
    }

    if (mCallersEventQ)
    {
        PLEvent *event = PR_NEW(PLEvent);
    
        PL_InitEvent(event, 
                     this,
                     CompletedEventHandler,
                     CompletedDestroyHandler);
   
        mCallersEventQ->PostSynchronousEvent(event, nsnull);
        PR_FREEIF(event);
    }
    else
    {
        // caller does not have an eventQ? This is an error!
        SetCompleted();
    }
}
  
nsIEventQueue*      
nsProxyObjectCallInfo::GetCallersQueue() 
{ 
    return mCallersEventQ;
}   
void
nsProxyObjectCallInfo::SetCallersQueue(nsIEventQueue* queue)
{
    mCallersEventQ = queue;
}   


nsProxyObject::nsProxyObject(nsIEventQueue *destQueue, PRInt32 proxyType, nsISupports *realObject)
{
    mEventQService = do_GetService(kEventQueueServiceCID);

    mRealObject      = realObject;
    mDestQueue       = do_QueryInterface(destQueue);
    mProxyType       = proxyType;
}


nsProxyObject::nsProxyObject(nsIEventQueue *destQueue, PRInt32  proxyType, const nsCID &aClass,  nsISupports *aDelegate,  const nsIID &aIID)
{
    mEventQService = do_GetService(kEventQueueServiceCID);

    nsCOMPtr<nsIComponentManager> compMgr;
    NS_GetComponentManager(getter_AddRefs(compMgr));
    compMgr->CreateInstance(aClass, 
                            aDelegate,
                            aIID,
                            getter_AddRefs(mRealObject));

    mDestQueue       = do_QueryInterface(destQueue);
    mProxyType       = proxyType;
}

nsProxyObject::~nsProxyObject()
{   
    // I am worried about order of destruction here.  
    // do not remove assignments.
    
    mRealObject = 0;
    mDestQueue  = 0;
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
       mDestQueue->IsOnCurrentThread(&callDirectly);

       if (callDirectly)
       {
           delete this;
           return;
       }

      // need to do something special here so that
      // the real object will always be deleted on
      // the correct thread..

       PLEvent *event = PR_NEW(PLEvent);
       if (event == nsnull)
       {
           NS_ASSERTION(0, "Could not create a plevent. Leaking nsProxyObject!");
           return;  // if this happens we are going to leak.
       }
       
       PL_InitEvent(event, 
                    this,
                    ProxyDestructorEventHandler,
                    ProxyDestructorDestroyHandler);  

       mDestQueue->PostEvent(event);
  }                          
}                


nsresult
nsProxyObject::PostAndWait(nsProxyObjectCallInfo *proxyInfo)
{
    if (proxyInfo == nsnull || mEventQService == nsnull) 
        return NS_ERROR_NULL_POINTER;
    
    PRBool eventLoopCreated = PR_FALSE;
    nsresult rv; 

    nsCOMPtr<nsIEventQueue> eventQ;
    rv = mEventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(eventQ));
    if (NS_FAILED(rv))
    {
        rv = mEventQService->CreateMonitoredThreadEventQueue();
        eventLoopCreated = PR_TRUE;
        if (NS_FAILED(rv))
            return rv;
        
        rv = mEventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(eventQ));
    }

    if (NS_FAILED(rv))
        return rv;
    
    proxyInfo->SetCallersQueue(eventQ);

    PLEvent* event = proxyInfo->GetPLEvent();
    if (!event)
        return NS_ERROR_NULL_POINTER;
    
    mDestQueue->PostEvent(event);

    while (! proxyInfo->GetCompleted())
    {
        PLEvent *nextEvent;
        rv = eventQ->WaitForEvent(&nextEvent);
        if (NS_FAILED(rv)) break;
                
        eventQ->HandleEvent(nextEvent);
    }  

    if (eventLoopCreated)
    {
         mEventQService->DestroyThreadEventQueue();
         eventQ = 0;
    }
    
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
        if ((mProxyType & PROXY_ASYNC) && paramInfo.IsDipper())
        {
            NS_WARNING("Async proxying of out parameters is not supported"); 
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
        
        NS_GetProxyForObject(NS_CURRENT_EVENTQ, iid, itf,
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
            
            NS_GetProxyForObject(mDestQueue, iid, itf,
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

    if (! mDestQueue  || ! mRealObject)
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
    // as the destination event queue. 
    if ( (!(mProxyType & PROXY_ISUPPORTS) && methodIndex == 0) ||
         (mProxyType & PROXY_SYNC && 
          NS_SUCCEEDED(mDestQueue->IsOnCurrentThread(&callDirectly)) &&
          callDirectly))
    {
        if (mProxyType & PROXY_AUTOPROXIFY)
            AutoproxifyInParameterList(fullParam, paramCount, methodInfo, methodIndex, interfaceInfo);

        // invoke the magic of xptc...
        nsresult rv = XPTC_InvokeByIndex( mRealObject, 
                                          methodIndex,
                                          paramCount, 
                                          fullParam);
        
        if (mProxyType & PROXY_AUTOPROXIFY)
            AutoproxifyOutParameterList(fullParam, paramCount, methodInfo, methodIndex, interfaceInfo);
        
        if (fullParam) 
            free(fullParam);
        return rv;
    }

    PLEvent *event = PR_NEW(PLEvent);
    
    if (event == nsnull) {
        if (fullParam) 
            free(fullParam);
        return NS_ERROR_OUT_OF_MEMORY;   
    }

    nsProxyObjectCallInfo *proxyInfo = new nsProxyObjectCallInfo(this, 
                                                                 methodInfo, 
                                                                 methodIndex, 
                                                                 fullParam,   // will be deleted by ~()
                                                                 paramCount,
                                                                 interfaceInfo,
                                                                 event);      // will be deleted by ~()
    
    if (proxyInfo == nsnull) {
        PR_DELETE(event);
        if (fullParam)
            free(fullParam);
        return NS_ERROR_OUT_OF_MEMORY;  
    }

    if (NS_FAILED(proxyInfo->Init())) {
        PR_DELETE(event);
        if (fullParam)
            free(fullParam);
        delete proxyInfo;
        return NS_ERROR_FAILURE;
    }

    PL_InitEvent(event, 
                 proxyInfo,
                 EventHandler,
                 DestroyHandler);
   
    if (mProxyType & PROXY_SYNC)
    {
        rv = PostAndWait(proxyInfo);
        
        if (NS_SUCCEEDED(rv))
            rv = proxyInfo->GetResult();
        delete proxyInfo;
        return rv;
    }
    
    if (mProxyType & PROXY_ASYNC)
    {
        mDestQueue->PostEvent(event);
        return NS_OK;
    }
    return NS_ERROR_UNEXPECTED;
}



static void DestroyHandler(PLEvent *self) 
{
    nsProxyObjectCallInfo* owner = (nsProxyObjectCallInfo*)PL_GetEventOwner(self);
    nsProxyObject* proxyObject = owner->GetProxyObject();
    
    if (proxyObject == nsnull)
        return;
 
    if (proxyObject->GetProxyType() & PROXY_ASYNC)
    {        
        delete owner;
    }
    else
    {
        owner->PostCompleted();
    }

}

static void* EventHandler(PLEvent *self) 
{
    nsProxyObjectCallInfo *info = (nsProxyObjectCallInfo*)PL_GetEventOwner(self);
    NS_ASSERTION(info, "No nsProxyObjectCallInfo!");
    
    nsProxyObject *proxyObject = info->GetProxyObject();
        
    if (proxyObject)
    {
        // invoke the magic of xptc...
        nsresult rv = XPTC_InvokeByIndex( proxyObject->GetRealObject(), 
                                          info->GetMethodIndex(),
                                          info->GetParameterCount(), 
                                          info->GetParameterList());
        info->SetResult(rv);
    }
    else
    {
        info->SetResult(NS_ERROR_OUT_OF_MEMORY);
    }
    return NULL;
}

static void CompletedDestroyHandler(PLEvent *self) 
{
}

static void* CompletedEventHandler(PLEvent *self) 
{
    nsProxyObjectCallInfo* owner = (nsProxyObjectCallInfo*)PL_GetEventOwner(self);
    owner->SetCompleted();
    return nsnull;
}

static void* ProxyDestructorEventHandler(PLEvent *self)
{              
    nsProxyObject* owner = (nsProxyObject*) PL_GetEventOwner(self);
    NS_DELETEXPCOM(owner);
    return nsnull;                                            
}

static void ProxyDestructorDestroyHandler(PLEvent *self)
{
    PR_DELETE(self);
}

