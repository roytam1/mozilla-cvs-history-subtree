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

#include "nsPluginRegistry.h"
#include "plstr.h"
#include "prlog.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginClassIID, NS_IPLUGINCLASS_IID);
static NS_DEFINE_IID(kIPluginRegistryIID, NS_IPLUGINREGISTRY_IID);


////////////////////////////////////////////////////////////////////////

class nsPluginRegistryPluginEnumerator : public nsIEnumerator
{
protected:
    nsPluginRegistry* fTarget;
    nsPluginRegistry::Entry* fCurrent;

public:
    nsPluginRegistryPluginEnumerator(nsPluginRegistry* target);

    virtual ~nsPluginRegistryPluginEnumerator();

    NS_DECL_ISUPPORTS

    NS_IMETHOD_(nsISupports*)
    Next(void);

    NS_IMETHOD_(void)
    Reset(void);
};


nsPluginRegistryPluginEnumerator::nsPluginRegistryPluginEnumerator(nsPluginRegistry* target)
    : fTarget(target)
{
    NS_INIT_REFCNT();
    fTarget->AddRef();
    Reset();
}


nsPluginRegistryPluginEnumerator::~nsPluginRegistryPluginEnumerator()
{
    fTarget->Release();
}


NS_IMPL_ADDREF(nsPluginRegistryPluginEnumerator);
NS_IMPL_RELEASE(nsPluginRegistryPluginEnumerator);
NS_IMPL_QUERY_INTERFACE(nsPluginRegistryPluginEnumerator, NS_IENUMERATOR_IID);


NS_METHOD_(nsISupports*)
nsPluginRegistryPluginEnumerator::Next(void)
{
    if (fCurrent) {
        nsISupports* result = fCurrent->fClass;
        fCurrent = fCurrent->fNext;
        return result;
    } else {
        return NULL;
    }
}


NS_METHOD_(void)
nsPluginRegistryPluginEnumerator::Reset(void)
{
    fCurrent = fTarget->fHead;
}


////////////////////////////////////////////////////////////////////////


nsPluginRegistry::nsPluginRegistry(nsISupports* outer)
    : fHead(NULL)
{
    NS_INIT_AGGREGATED(outer);
}

/*
 * Release all of the entries in the registry.
 */
nsPluginRegistry::~nsPluginRegistry(void)
{
    while (fHead != NULL) {
        Entry *next = fHead->fNext;
        fHead->fClass->Release();
        delete fHead;

        fHead = next;
    }
}


NS_IMPL_AGGREGATED(nsPluginRegistry);


NS_METHOD
nsPluginRegistry::AggregatedQueryInterface(const nsIID& iid, void** instance)
{
    if (iid.Equals(kISupportsIID) ||
        iid.Equals(kIPluginRegistryIID)) {
        *instance = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}




/*
 * Add a new plugin to the registry.
 */
NS_IMETHODIMP_(nsresult)
nsPluginRegistry::Register(nsIPluginClass* pluginClass)
{
    Entry *entry;

#if 0
    /* Ensure uniqueness of pdesc values! */
    for (entry = fHead; entry != NULL; entry = entry->fNext)
        PR_ASSERT(entry->fClass->GetPDesc() != pluginClass->GetPDesc());
#endif

    if ((entry = new Entry()) == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    /* Create local copies of the strings used to describe the plugin. */
    entry->fClass = pluginClass;
    pluginClass->AddRef();

    entry->fNext  = fHead;
    fHead = entry;

    return NS_OK;
}

/*
 * Add a new plugin to the registry.
 */
NS_IMETHODIMP_(nsresult)
nsPluginRegistry::Unregister(nsIPluginClass* pluginClass)
{
    if (fHead == NULL)
        return NS_ERROR_FAILURE; // XXX

    // Special case for the head...
    if (fHead->fClass == pluginClass) {
        fHead->fClass->Release();
        fHead = fHead->fNext;
        return NS_OK;
    }

    // ...else look through the list to find it...
    Entry *entry = fHead;
    while (entry->fNext != NULL) {
        if (entry->fNext->fClass == pluginClass) {
            entry->fNext->fClass->Release();
            entry->fNext = entry->fNext->fNext;
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE; // XXX
}



NS_IMETHODIMP_(void)
nsPluginRegistry::CreatePluginEnumerator(nsIEnumerator* *result)
{
    *result = new nsPluginRegistryPluginEnumerator(this);
    if (*result) (*result)->AddRef();
}


NS_IMETHODIMP_(XP_Bool)
nsPluginRegistry::FindByName(const char* name,
                             nsIPluginClass* *pluginClass)
{
    Entry* entry;
    for (entry = fHead; entry != NULL; entry = entry->fNext) {
        if (PL_strcmp(entry->fClass->GetName(), name) == 0)
            break;
    }

    if (entry == NULL)
        return FALSE;

    *pluginClass = entry->fClass;
    return TRUE;
}


////////////////////////////////////////////////////////////////////////


