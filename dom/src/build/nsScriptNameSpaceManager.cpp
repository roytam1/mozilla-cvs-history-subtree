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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsScriptNameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIScriptExternalNameSet.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"


nsScriptNameSpaceManager::nsScriptNameSpaceManager()
{
}

static PRBool PR_CALLBACK
NameStructCleanupCallback(nsHashKey *aKey, void *aData, void* closure)
{
  nsGlobalNameStruct *s = (nsGlobalNameStruct *)aData;

  delete s;

  return PR_TRUE;
}

nsScriptNameSpaceManager::~nsScriptNameSpaceManager()
{
  mGlobalNames.Reset(NameStructCleanupCallback);
}

nsresult
nsScriptNameSpaceManager::FillHash(nsICategoryManager *aCategoryManager,
                                   const char *aCategory,
                                   nsGlobalNameStruct::nametype aType)
{
  nsCOMPtr<nsISimpleEnumerator> e;
  nsresult rv = aCategoryManager->EnumerateCategory(aCategory,
                                                    getter_AddRefs(e));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString categoryEntry;
  nsXPIDLCString contractId;
  nsCOMPtr<nsISupports> entry;

  while (NS_SUCCEEDED(e->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsString!");

      continue;
    }

    rv = category->GetData(getter_Copies(categoryEntry));

    aCategoryManager->GetCategoryEntry(aCategory, categoryEntry,
                                       getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCID cid;

    rv = nsComponentManager::ContractIDToClassID(contractId, &cid);

    if (NS_FAILED(rv)) {
      NS_WARNING("Bad contract id registed with the script namespace manager");

      continue;
    }

    nsGlobalNameStruct *s = new nsGlobalNameStruct;
    NS_ENSURE_TRUE(s, NS_ERROR_OUT_OF_MEMORY);

    s->mType = aType;
    s->mCID = cid;

    nsAutoString name;
    CopyASCIItoUCS2(nsLiteralCString(categoryEntry), name);

    nsStringKey key(name);

    mGlobalNames.Put(&key, s);
  }

  return rv;
}

nsresult
nsScriptNameSpaceManager::Init()
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                nsGlobalNameStruct::eTypeConstructor);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                nsGlobalNameStruct::eTypeProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_STATIC_NAMESET_CATEGORY,
                nsGlobalNameStruct::eTypeStaticNameSet);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillHash(cm, JAVASCRIPT_GLOBAL_DYNAMIC_NAMESET_CATEGORY,
                nsGlobalNameStruct::eTypeDynamicNameSet);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static PRBool PR_CALLBACK
NameSetInitCallback(nsHashKey *aKey, void *aData, void* closure)
{
  nsGlobalNameStruct *s = (nsGlobalNameStruct *)aData;

  if (s->mType != nsGlobalNameStruct::eTypeStaticNameSet) {
    return PR_TRUE;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIScriptExternalNameSet> ns(do_CreateInstance(s->mCID, &rv));
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  rv = ns->InitializeClasses((nsIScriptContext *)closure);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Initing external script classes failed!");

  return PR_TRUE;
}

nsresult
nsScriptNameSpaceManager::InitForContext(nsIScriptContext *aContext)
{
  mGlobalNames.Enumerate(NameSetInitCallback, aContext);

  return NS_OK;
}

nsresult
nsScriptNameSpaceManager::LookupName(const nsAReadableString& aName,
                                     const nsGlobalNameStruct **aNameStruct)
{
  nsStringKey key(aName);

  *aNameStruct = (const nsGlobalNameStruct *)mGlobalNames.Get(&key);

  return NS_OK;
}

