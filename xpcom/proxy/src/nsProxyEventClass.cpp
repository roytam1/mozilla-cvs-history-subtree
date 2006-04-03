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

#include "nsProxyEventPrivate.h"
#include "nsIProxyObjectManager.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

#include "nsMemory.h"
#include "nsHashtable.h"

#include "nsAutoLock.h"
#include "xptiprivate.h"
#include "xptcall.h"

// LIFETIME_CACHE will cache class for the entire cyle of the application.
#define LIFETIME_CACHE

static uint32 zero_methods_descriptor;

//////////////////////////////////////////////////////////////////////////////////////////////////
//  nsProxyEventClass
//////////////////////////////////////////////////////////////////////////////////////////////////
NS_IMPL_THREADSAFE_ISUPPORTS1(nsProxyEventClass, nsProxyEventClass)

// static
nsProxyEventClass*
nsProxyEventClass::GetNewOrUsedClass(REFNSIID aIID)
{
    /* find in our hash table */
    
    nsProxyObjectManager *manager = nsProxyObjectManager::GetInstance();
    if (manager == nsnull) return nsnull;
	
	// don't need to lock the map, because this is only called
	// by nsProxyEventClass::GetNewOrUsed which locks it for us.

	nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();
    
    if (iidToClassMap == nsnull)
    {
        return nsnull;
    }

    nsProxyEventClass* clazz = nsnull;
    nsIDKey key(aIID);

#ifdef PROXYEVENTCLASS_DEBUG 
	char* iidStr = aIID.ToString();
	printf("GetNewOrUsedClass  %s\n", iidStr);
	nsCRT::free(iidStr);
#endif

    clazz = (nsProxyEventClass*) iidToClassMap->Get(&key);
	if(clazz)
	{
        NS_ADDREF(clazz);
#ifdef PROXYEVENTCLASS_DEBUG
		char* iidStr = aIID.ToString();
		printf("GetNewOrUsedClass  %s hit\n", iidStr);
		nsCRT::free(iidStr);
#endif
    }
    else
    {
        nsIInterfaceInfoManager* iimgr =
            xptiInterfaceInfoManager::GetInterfaceInfoManagerNoAddRef();

        if(iimgr)
        {
            nsCOMPtr<nsIInterfaceInfo> info;
            if(NS_SUCCEEDED(iimgr->GetInfoForIID(&aIID, getter_AddRefs(info))))
            {
                /* 
                   Check to see if isISupportsDescendent 
                */
                nsCOMPtr<nsIInterfaceInfo> oldest = info;
                nsCOMPtr<nsIInterfaceInfo> parent;

                while(NS_SUCCEEDED(oldest->GetParent(getter_AddRefs(parent)))&&
                      parent)
                {
                    oldest = parent;
                }

                PRBool isISupportsDescendent = PR_FALSE;
                nsID* iid;
                if(NS_SUCCEEDED(oldest->GetInterfaceIID(&iid))) 
                {
                    isISupportsDescendent = iid->Equals(NS_GET_IID(nsISupports));
                    nsMemory::Free(iid);
                }
                
                NS_ASSERTION(isISupportsDescendent, "!isISupportsDescendent");

                if (isISupportsDescendent)  
                {
                    clazz = new nsProxyEventClass(aIID, info);
                    if(!clazz->mDescriptors)
                        NS_RELEASE(clazz);  // sets clazz to NULL
                }
            }
        }
    }
    return clazz;
}

nsProxyEventClass::nsProxyEventClass()
{
    NS_WARNING("This constructor should never be called");
}

nsProxyEventClass::nsProxyEventClass(REFNSIID aIID, nsIInterfaceInfo* aInfo)
: mIID(aIID),
  mDescriptors(NULL)
{
    NS_ADDREF_THIS();
    
    mInfo = aInfo;

    /* add use to the used classes */
    nsIDKey key(aIID);

    nsProxyObjectManager *manager = nsProxyObjectManager::GetInstance();
    if (manager == nsnull) return;
	// don't need to lock the map, because this is only called
	// by GetNewOrUsed which locks it for us.

	nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();
    
    if (iidToClassMap != nsnull)
    {
        iidToClassMap->Put(&key, this);
#ifdef LIFETIME_CACHE
		// extra addref to hold it in the cache
        NS_ADDREF_THIS();
#endif
#ifdef PROXYEVENTCLASS_DEBUG
		char* iidStr = aIID.ToString();
		printf("GetNewOrUsedClass  %s put\n", iidStr);
		nsCRT::free(iidStr);
#endif
    }

    uint16 methodCount;
    if(NS_SUCCEEDED(mInfo->GetMethodCount(&methodCount)))
    {
        if(methodCount)
        {
            int wordCount = (methodCount/32)+1;
            if(NULL != (mDescriptors = new uint32[wordCount]))
            {
                memset(mDescriptors, 0, wordCount * sizeof(uint32));
            }
        }
        else
        {
            mDescriptors = &zero_methods_descriptor;
        }
    }
}

nsProxyEventClass::~nsProxyEventClass()
{
    if(mDescriptors && mDescriptors != &zero_methods_descriptor)
        delete [] mDescriptors;

    if (nsProxyObjectManager::IsManagerShutdown())
        return;

#ifndef LIFETIME_CACHE
    nsIDKey key(mIID);
                
    nsCOMPtr<nsProxyObjectManager> manager = getter_AddRefs(nsProxyObjectManager::GetInstance());
    if (manager == nsnull) return;
	nsHashtable *iidToClassMap =  manager->GetIIDToProxyClassMap();
    
    if (iidToClassMap != nsnull)
    {
        iidToClassMap->Remove(&key);
#ifdef PROXYEVENTCLASS_DEBUG
		char* iidStr = mIID.ToString();
		printf("GetNewOrUsedClass  %s remove\n", iidStr);
		nsCRT::free(iidStr);
#endif
    }
#endif
}
