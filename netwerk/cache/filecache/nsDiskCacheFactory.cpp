/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDBAccessor.h"
#include "nsNetDiskCacheCID.h"
#include "nsDBAccessor.h"
#include "nsNetDiskCache.h"

static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_CID (kComponentManagerCID, NS_COMPONENTMANAGER_CID) ;

static NS_DEFINE_CID(kDBAccessorCID, NS_DBACCESSOR_CID) ;
static NS_DEFINE_CID(kNetDiskCacheCID, NS_NETDISKCACHE_CID) ;

class nsDBAccessorFactory : public nsIFactory
{
  public:
  nsDBAccessorFactory() ;
  virtual ~nsDBAccessorFactory() ;

  // nsISupports methods
  NS_DECL_ISUPPORTS

  // nsIFactory methods
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);
  private:

//  static PRInt32 DBLockCnt ;
} ;

nsDBAccessorFactory::nsDBAccessorFactory()
{
//  DBLockCnt = 0 ;
  NS_INIT_REFCNT();
}

nsDBAccessorFactory::~nsDBAccessorFactory()
{
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

NS_IMPL_ISUPPORTS(nsDBAccessorFactory, kIFactoryIID) ;

NS_IMETHODIMP
nsDBAccessorFactory::CreateInstance(nsISupports *aOuter,
                                   const nsIID &aIID,
                                   void **aResult)
{
  if(!aResult)
    return NS_ERROR_NULL_POINTER ;

  *aResult = nsnull ;

  nsISupports *inst = new nsDBAccessor ;

  if(!inst) 
    return NS_ERROR_OUT_OF_MEMORY ;

  nsresult rv = inst -> QueryInterface(aIID, aResult) ;

  if(NS_FAILED(rv)) 
    delete inst ;

  return rv ;
}

NS_IMETHODIMP
nsDBAccessorFactory::LockFactory(PRBool aLock)
{
  /*
  if(aLock) 
    PR_AtomicIncrement (&DBLockCnt) ;
            
  else 
    PR_AtomicDecrement (&DBLockCnt) ;
    */

  return NS_OK ;

}

class nsNetDiskCacheFactory : public nsIFactory
{
  public:
  nsNetDiskCacheFactory() ;
  virtual ~nsNetDiskCacheFactory() ;

  // nsISupports methods
  NS_DECL_ISUPPORTS

  // nsIFactory methods
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);
  private:

//  static PRInt32 DiskCacheLockCnt ;
} ;

nsNetDiskCacheFactory::nsNetDiskCacheFactory()
{
//  DiskCacheLockCnt = 0 ;
  NS_INIT_REFCNT();
}

nsNetDiskCacheFactory::~nsNetDiskCacheFactory()
{
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

NS_IMPL_ISUPPORTS(nsNetDiskCacheFactory, kIFactoryIID) ;

NS_IMETHODIMP
nsNetDiskCacheFactory::CreateInstance(nsISupports *aOuter,
                                   const nsIID &aIID,
                                   void **aResult)
{
  if(!aResult)
    return NS_ERROR_NULL_POINTER ;

  *aResult = nsnull ;

  nsISupports *inst = new nsNetDiskCache ;

  if(!inst) 
    return NS_ERROR_OUT_OF_MEMORY ;

  nsresult rv = inst -> QueryInterface(aIID, aResult) ;

  if(NS_FAILED(rv)) 
    delete inst ;

  return rv ;
}

NS_IMETHODIMP
nsNetDiskCacheFactory::LockFactory(PRBool aLock)
{
  /*
  if(aLock) 
    PR_AtomicIncrement (&DiskCacheLockCnt) ;
            
  else 
    PR_AtomicDecrement (&DiskCacheLockCnt) ;

  */
  return NS_OK ;
}

extern "C" NS_EXPORT nsresult
NSGetFactory(nsISupports *serviceMgr, 
             const nsCID &aCID,
			 const char * aClassName,
			 const char * aProgID,
			 nsIFactory **aResult)
{
  if(!aResult)
	return NS_ERROR_NULL_POINTER ;

  *aResult = nsnull ;

  nsISupports *inst ;

  if(aCID.Equals(kDBAccessorCID)) {
	inst = new nsDBAccessorFactory() ;
  }
  else if(aCID.Equals(kNetDiskCacheCID)) {
    inst = new nsNetDiskCacheFactory() ; 
  }
  else {
	return NS_ERROR_NO_INTERFACE ;
  }

  if(!inst)
	return NS_ERROR_OUT_OF_MEMORY ;

  nsresult rv = inst->QueryInterface(kIFactoryIID, (void **)aResult) ;

  if (NS_FAILED(rv)) {
	delete inst ;
  }

  return rv ;
}

extern "C" NS_EXPORT PRBool
NSCanUnload (nsISupports* serviceMgr)
{
  // not implemented
  return PR_TRUE ;
}

extern "C" NS_EXPORT nsresult
NSRegisterSelf(nsISupports *aServMgr, const char *path)
{
  nsresult rv = NS_OK ;

  nsIServiceManager *sm ;

  rv = aServMgr->QueryInterface(nsIServiceManager::GetIID(), (void**)&sm) ;

  if(NS_FAILED(rv)) {
	return rv;
  }

  nsIComponentManager *cm ;

  rv = sm->GetService(kComponentManagerCID, 
                      nsIComponentManager::GetIID(), 
                      (nsISupports **) &cm) ;
  if (NS_FAILED(rv)) {
	NS_RELEASE(sm) ;
	return rv ;
  }

  rv = cm->RegisterComponent(kDBAccessorCID, "Disk Cache DB Accessor", 
                             "component://netscape/network/cache/db-access",
                             path, PR_TRUE, PR_TRUE) ;

  rv = cm->RegisterComponent(kNetDiskCacheCID, "Disk Cache Module", 
                             "component://netscape/network/cache/db-access",
                             path, PR_TRUE, PR_TRUE) ;

  sm->ReleaseService(kComponentManagerCID, cm) ;

  NS_RELEASE(sm) ;

  return rv ;
}

extern "C" NS_EXPORT nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char *path)
{
  // not implemented
  return NS_OK ;
}

