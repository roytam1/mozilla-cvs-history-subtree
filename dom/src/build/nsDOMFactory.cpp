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
#include "nsIGenericFactory.h"

#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMNativeObjectRegistry.h"
#include "nsIScriptEventListener.h"
#include "nsIJSEventListener.h"
#include "nsIScriptContext.h"
#include "plhash.h"

extern nsresult NS_CreateScriptContext(nsIScriptGlobalObject *aGlobal,
                                       nsIScriptContext **aContext);

extern nsresult NS_NewJSEventListener(nsIDOMEventListener **aInstancePtrResult,
                                      nsIScriptContext *aContext,
                                      nsISupports *aObject);

extern nsresult NS_NewScriptGlobalObject(nsIScriptGlobalObject **aGlobal);


//////////////////////////////////////////////////////////////////////

class nsDOMSOFactory : public nsIDOMScriptObjectFactory
{
public:
  nsDOMSOFactory();
  virtual ~nsDOMSOFactory();

  NS_DECL_ISUPPORTS

  NS_IMETHOD NewScriptContext(nsIScriptGlobalObject *aGlobal,
                              nsIScriptContext **aContext);

  NS_IMETHOD NewJSEventListener(nsIScriptContext *aContext,
                                nsISupports* aObject,
                                nsIDOMEventListener ** aInstancePtrResult);

  NS_IMETHOD NewScriptGlobalObject(nsIScriptGlobalObject **aGlobal);
};

nsDOMSOFactory::nsDOMSOFactory()
{
  NS_INIT_REFCNT();
}

nsDOMSOFactory::~nsDOMSOFactory()
{
}


NS_INTERFACE_MAP_BEGIN(nsDOMSOFactory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMSOFactory);
NS_IMPL_RELEASE(nsDOMSOFactory);


NS_IMETHODIMP
nsDOMSOFactory::NewScriptContext(nsIScriptGlobalObject *aGlobal,
                                 nsIScriptContext **aContext)
{
  return NS_CreateScriptContext(aGlobal, aContext);
}

NS_IMETHODIMP
nsDOMSOFactory::NewJSEventListener(nsIScriptContext *aContext,
                                   nsISupports *aObject,
                                   nsIDOMEventListener **aInstancePtrResult)
{
  return NS_NewJSEventListener(aInstancePtrResult, aContext, aObject);
}

NS_IMETHODIMP
nsDOMSOFactory::NewScriptGlobalObject(nsIScriptGlobalObject **aGlobal)
{
  return NS_NewScriptGlobalObject(aGlobal);
}

//////////////////////////////////////////////////////////////////////

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDOMSOFactory);

static nsModuleComponentInfo gDOMModuleInfo[] = {
    { "Script Object Factory",
      NS_DOM_SCRIPT_OBJECT_FACTORY_CID,
      nsnull,
      nsDOMSOFactoryConstructor
    }
};

NS_IMPL_NSGETMODULE("DOM components", gDOMModuleInfo)


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

