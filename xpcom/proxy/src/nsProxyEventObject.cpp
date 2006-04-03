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

#include "prprf.h"
#include "prmem.h"

#include "nscore.h"
#include "nsProxyEventPrivate.h"
#include "nsIProxyObjectManager.h"
#include "nsProxyRelease.h"

#include "nsIEventQueueService.h"
#include "nsServiceManagerUtils.h"

#include "nsHashtable.h"

#include "nsIInterfaceInfoManager.h"
#include "xptcall.h"

#include "nsAutoLock.h"

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_xpcom_proxy
static PRMonitor* mon = nsnull;
static PRUint32 totalProxyObjects = 0;
static PRUint32 outstandingProxyObjects = 0;

void
nsProxyEventObject::DebugDump(const char * message, PRUint32 hashKey)
{

    if (mon == nsnull)
    {
        mon = PR_NewMonitor();
    }

    PR_EnterMonitor(mon);

    if (message)
    {
        printf("\n-=-=-=-=-=-=-=-=-=-=-=-=-\n");
        printf("%s\n", message);

        if(strcmp(message, "Create") == 0)
        {
            totalProxyObjects++;
            outstandingProxyObjects++;
        }
        else if(strcmp(message, "Delete") == 0)
        {
            outstandingProxyObjects--;
        }
    }
    printf("nsProxyEventObject @ %x with mRefCnt = %d\n", this, mRefCnt);

    PRBool isRoot = mRoot == nsnull;
    printf("%s wrapper around  @ %x\n", isRoot ? "ROOT":"non-root\n", GetRealObject());

    nsCOMPtr<nsISupports> rootObject = do_QueryInterface(mProxyObject->mRealObject);
    nsCOMPtr<nsISupports> rootQueue = do_QueryInterface(mProxyObject->mDestQueue);
    nsProxyEventKey key(rootObject, rootQueue, mProxyObject->mProxyType);
    printf("Hashkey: %d\n", key.HashCode());
        
    char* name;
    GetClass()->GetInterfaceInfo()->GetName(&name);
    printf("interface name is %s\n", name);
    if(name)
        nsMemory::Free(name);
    char * iid = GetClass()->GetProxiedIID().ToString();
    printf("IID number is %s\n", iid);
    delete iid;
    printf("nsProxyEventClass @ %x\n", mClass);
    
    if(mNext)
    {
        if(isRoot)
        {
            printf("Additional wrappers for this object...\n");
        }
        mNext->DebugDump(nsnull, 0);
    }

    printf("[proxyobjects] %d total used in system, %d outstading\n", totalProxyObjects, outstandingProxyObjects);

    if (message)
        printf("-=-=-=-=-=-=-=-=-=-=-=-=-\n");

    PR_ExitMonitor(mon);
}
#endif

// nsProxyCanonicalObject

NS_IMETHODIMP
nsProxyCanonicalObject::QueryInterface(REFNSIID aIID, void **aResult)
{
    if (aIID.Equals(NS_GET_IID(nsProxyCanonicalObject))) {
        *aResult = this;
        return NS_OK;
    }

    nsISomeInterface* foundInterface = nsnull;
    if (aIID.Equals(NS_GET_IID(nsISupports))) {
        foundInterface = this;
        NS_ADDREF(foundInterface);
        *aResult = foundInterface;
        return NS_OK;
    }

    // protect the mFirst chain
    nsAutoMonitor mon(nsProxyObjectManager::GetInstance()->GetMonitor());

    nsProxyEventObject *poe = mFirst;
    while (poe) {
        if (aIID.Equals(poe->GetIID())) {
            foundInterface = poe->mXPTCStub;
            NS_ADDREF(foundInterface);
            *aResult = foundInterface;
            return NS_OK;
        }

        poe = poe->mNext;
    }

    // If we don't already have a proxy for this interface, ask the original
    // object whether it supports the interface.

    nsCOMPtr<nsISomeInterface> objInterface;
    nsresult rv = mProxiedObject->
        QueryInterface(aIID, getter_AddRefs(objInterface));
    if (NS_FAILED(rv))
        return rv;

    nsProxyEventClass* pec = nsProxyEventClass::GetNewOrUsedClass(aIID);
    if (!pec)
        return NS_ERROR_OUT_OF_MEMORY;

    // Create a nsProxyEventObject for the newly-found interface and add it
    // to our chain.
    poe = new nsProxyEventObject(objInterface, pec, this);
    if (!poe || !poe->mXPTCStub)
        return NS_ERROR_OUT_OF_MEMORY;

    poe->mNext = mFirst;
    mFirst = poe;

    // Answer QI with the stub on the POE
    foundInterface = poe->mXPTCStub;
    NS_ADDREF(foundInterface);
    *aResult = foundInterface;

    return NS_OK;
}

NS_IMPL_THREADSAFE_ADDREF(nsProxyCanonicalObject)

// We implement ::Release manually because we must be holding the manager
// monitor when we call the destructor.
NS_IMETHODIMP_(nsrefcnt)
nsProxyCanonicalObject::Release(void)
{
    // We hold the manager alive, so we don't need to null-check.
    nsProxyObjectManager* manager = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(manager, "No global proxy object manager?");

    nsAutoMonitor mon(manager->GetMonitor());

    nsrefcnt count;
    NS_ASSERTION(mRefCnt, "dup release");

    // Decrement atomically so we don't race with possible addrefs.
    count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
    NS_LOG_RELEASE(this, count, "nsProxyCanonicalObject");
    if (0 == count) {
        // The destructor, called from inside the monitor, removes us
        // from the proxy chain.
        NS_DELETEXPCOM(this);
        return 0;
    }
    return count;
}

nsProxyCanonicalObject::nsProxyCanonicalObject(nsISupports* aObjectToProxy,
                                               nsIEventQueue* aDestQueue,
                                               PRInt32 aProxyType) :
    mProxyType(aProxyType),
    mProxiedObject(aObjectToProxy),
    mDestQueue(aDestQueue)
{
    // Since the proxyobjectmanager is the only thing creating us, it should
    // always exist.
    nsProxyObjectManager* manager = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(manager, "No global proxy object manager?");

    NS_ADDREF(manager);
}
    

nsProxyCanonicalObject::~nsProxyCanonicalObject()
{
    // The destructor is only called from within the global monitor.
    // I wish there was a way to assert that. --bsmedberg

    NS_ASSERTION(!mFirst, "Proxy interface table not clear.");

    nsProxyObjectManager* manager = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(manager, "No global proxy object manager?");

    // Remove ourself from the global proxy object map.
    nsCOMPtr<nsISupports> realQueue = do_QueryInterface(mDestQueue);
    nsProxyEventKey key(mProxiedObject, realQueue, mProxyType);

    nsHashtable *hash = manager->GetRealObjectToProxyObjectMap();
    nsProxyCanonicalObject *hashedThis =
        (nsProxyCanonicalObject*) hash->Remove(&key);
    NS_ASSERTION(hashedThis == this, "Proxy map corrupted");

    // For (mostly bad) reasons, we want to make sure that the last reference
    // we hold to a proxied object is released on a thread the object expects
    // (typically the main thread). If this destructor is being run from a
    // different thread, proxy the final release to the main thread.

    // This should really be handled by the supposedly-threadsafe
    // object's Release() implementation.

    // This is a hacky way to hack around nsCOMPtr's lack of .forget()
    nsISupports* toRelease = nsnull;
    mProxiedObject.swap(toRelease);
    NS_ProxyRelease(mDestQueue, toRelease);

    NS_RELEASE(manager);
}

void
nsProxyCanonicalObject::LockedRemoveProxy(nsProxyEventObject *proxy)
{
    // This method is only called from within the global monitor.
    // I wish there was a way to assert that. --bsmedberg

    nsProxyEventObject **peo = &mFirst;
    while (*peo) {
        if (*peo == proxy) {
            *peo = proxy->mNext;
            proxy->mNext = nsnull;
            return;
        }

        peo = &((*peo)->mNext);
    }
    NS_ERROR("Couldn't find interface proxy in list.");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  nsProxyEventObject
//
//////////////////////////////////////////////////////////////////////////////////////////////////

nsProxyEventObject::nsProxyEventObject(nsISomeInterface* aObj,
                                       nsProxyEventClass* aClass,
                                       nsProxyCanonicalObject* root)
    : mClass(aClass),
      mRoot(root),
      mXPTCStub(nsnull),
      mNext(nsnull)
{
#ifdef DEBUG_xpcom_proxy
    DebugDump("Create", 0);
#endif

    NS_GetXPTCallStub(aClass->GetProxiedIID(), this, &mXPTCStub);
}

nsProxyEventObject::~nsProxyEventObject()
{
    // The destructor is only called from within the global monitor.
    // I wish there was a way to assert that. --bsmedberg
#ifdef DEBUG_xpcom_proxy
    DebugDump("Delete", 0);
#endif
    if (mRoot) {
        mRoot->LockedRemoveProxy(this);
    }

    NS_ASSERTION(!mNext, "Not removed from proxy chain!");

    if (mXPTCStub)
        NS_DestroyXPTCallStub(mXPTCStub);
}

//
// nsISupports implementation...
//

NS_IMPL_THREADSAFE_ADDREF(nsProxyEventObject)

NS_IMETHODIMP_(nsrefcnt)
nsProxyEventObject::Release(void)
{
    // nsProxyCanonicalObject holds the manager alive, so we don't need to
    // null-check.
    nsProxyObjectManager* manager = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(manager, "No global proxy object manager?");

    nsAutoMonitor mon(manager->GetMonitor());

    nsrefcnt count;
    NS_ASSERTION(mRefCnt, "dup release");

    // Decrement atomically so that we don't race with possible addrefs.
    count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
    NS_LOG_RELEASE(this, count, "nsProxyEventObject");
    if (0 == count) {
        // The destructor, called from inside the monitor, removes us
        // from the proxy chain.
        NS_DELETEXPCOM(this);
        return 0;
    }
    return count;
}

NS_IMETHODIMP
nsProxyEventObject::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    return mRoot->QueryInterface(aIID, aInstancePtr);
}
