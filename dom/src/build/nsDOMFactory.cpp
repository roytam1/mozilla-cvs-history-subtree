/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */
#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"

#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsDOMCID.h"
#include "nsIDOMNativeObjectRegistry.h"
#include "nsScriptNameSetRegistry.h"
#include "nsIScriptEventListener.h"
#include "nsIJSEventListener.h"
#include "nsIScriptContext.h"
#include "plhash.h"
#include "nsIPref.h"

static NS_DEFINE_IID(kIDOMNativeObjectRegistry, NS_IDOM_NATIVE_OBJECT_REGISTRY_IID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

class nsDOMNativeObjectRegistry : public nsIDOMNativeObjectRegistry {
public:
  nsDOMNativeObjectRegistry();
  virtual ~nsDOMNativeObjectRegistry();

  NS_DECL_ISUPPORTS

  NS_IMETHOD RegisterFactory(const nsString& aClassName, const nsIID& aCID);
  NS_IMETHOD GetFactoryCID(const nsString& aClassName, nsIID& aCID);

private:
  static PRIntn PR_CALLBACK RemoveStrings(PLHashEntry *he, PRIntn i, void *arg);

  PLHashTable *mFactories;
};

nsDOMNativeObjectRegistry::nsDOMNativeObjectRegistry()
{
  NS_INIT_REFCNT();
  mFactories = nsnull;
}

PRIntn 
nsDOMNativeObjectRegistry::RemoveStrings(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;

  nsCRT::free(str);
  return HT_ENUMERATE_REMOVE;
}

nsDOMNativeObjectRegistry::~nsDOMNativeObjectRegistry()
{
  if (nsnull != mFactories) {
    PL_HashTableEnumerateEntries(mFactories, RemoveStrings, nsnull);
    PL_HashTableDestroy(mFactories);
    mFactories = nsnull;
  }
}

NS_IMPL_ISUPPORTS(nsDOMNativeObjectRegistry, kIDOMNativeObjectRegistry);

NS_IMETHODIMP 
nsDOMNativeObjectRegistry::RegisterFactory(const nsString& aClassName, 
                                           const nsIID& aCID)
{
  if (nsnull == mFactories) {
    mFactories = PL_NewHashTable(4, PL_HashString, PL_CompareStrings,
                                 PL_CompareValues, nsnull, nsnull);
  }

  char *name = aClassName.ToNewCString();
  PL_HashTableAdd(mFactories, name, (void *)&aCID);
  
  return NS_OK;
}
 
NS_IMETHODIMP 
nsDOMNativeObjectRegistry::GetFactoryCID(const nsString& aClassName, 
                                         nsIID& aCID)
{
  if (nsnull == mFactories) {
    return NS_ERROR_FAILURE;
  }
  
  char *name = aClassName.ToNewCString();
  nsIID *classId = (nsIID *)PL_HashTableLookup(mFactories, name);
  nsCRT::free(name);

  aCID = *classId;

  return NS_OK;
}


//////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kCDOMScriptObjectFactory, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kCDOMNativeObjectRegistry, NS_DOM_NATIVE_OBJECT_REGISTRY_CID);
static NS_DEFINE_CID(kCScriptNameSetRegistry, NS_SCRIPT_NAMESET_REGISTRY_CID);

class nsDOMFactory : public nsIFactory
{   
  public:   
    NS_DECL_ISUPPORTS

    NS_DECL_NSIFACTORY

    nsDOMFactory(const nsCID &aClass);   

  protected:
    virtual ~nsDOMFactory();   

  private:   
    nsCID     mClassID;
};   

nsDOMFactory::nsDOMFactory(const nsCID &aClass)   
{   
  NS_INIT_ISUPPORTS();
  mClassID = aClass;
}   

nsDOMFactory::~nsDOMFactory()   
{   
}   

NS_IMPL_ISUPPORTS(nsDOMFactory, NS_GET_IID(nsIFactory))

nsresult nsDOMFactory::CreateInstance(nsISupports *aOuter,  
                                      const nsIID &aIID,  
                                      void **aResult)  
{  
  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  nsISupports *inst = nsnull;

  if (mClassID.Equals(kCDOMNativeObjectRegistry)) {
    inst = (nsISupports *)new nsDOMNativeObjectRegistry();
  }
  else if (mClassID.Equals(kCScriptNameSetRegistry)) {
    inst = (nsISupports *)new nsScriptNameSetRegistry();
  }

  if (inst == NULL) {  
    return NS_ERROR_OUT_OF_MEMORY;  
  }  

  NS_ADDREF(inst);  // Stabilize
  
  nsresult res = inst->QueryInterface(aIID, aResult);

  NS_RELEASE(inst); // Destabilize and avoid leaks. Avoid calling delete <interface pointer>    

  return res;  
}  

nsresult nsDOMFactory::LockFactory(PRBool aLock)  
{  
  // Not implemented in simplest case.  
  return NS_OK;
}  

// return the proper factory to the caller
#if defined(XP_MAC) && defined(MAC_STATIC)
extern "C" NS_DOM nsresult
NSGetFactory_DOM_DLL(nsISupports* servMgr,
                     const nsCID &aClass, 
                     const char *aClassName,
                     const char *aContractID,
                     nsIFactory **aFactory)
#else
extern "C" NS_DOM nsresult
NSGetFactory(nsISupports* servMgr,
             const nsCID &aClass, 
             const char *aClassName,
             const char *aContractID,
             nsIFactory **aFactory)
#endif
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsDOMFactory(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(NS_GET_IID(nsIFactory), (void**)aFactory);
}

void XXXDomNeverCalled();
void XXXDomNeverCalled()
{
  NS_CreateScriptContext(nsnull, nsnull);
  NS_NewJSEventListener(nsnull, nsnull, nsnull);

  //  nsJSContext* jcx = new nsJSContext(0);
  //  if (jcx) {
  NS_NewScriptGlobalObject(0);
  //  }
}


#ifdef DEBUG
/* These are here to be callable from a debugger */
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
JS_BEGIN_EXTERN_C
void DumpJSStack()
{
    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv))
        xpc->DebugDumpJSStack(PR_TRUE, PR_TRUE, PR_FALSE);
    else    
        printf("failed to get XPConnect service!\n");
}

void DumpJSEval(PRUint32 frame, const char* text)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
    if(NS_SUCCEEDED(rv))
        xpc->DebugDumpEvalInJSStackFrame(frame, text);
    else    
        printf("failed to get XPConnect service!\n");
}
JS_END_EXTERN_C
#endif

