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

#include "nsMimeTypeRegistry.h"
#include "plstr.h"
#include "prlog.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIMimeTypeIID, NS_IMIMETYPE_IID);
static NS_DEFINE_IID(kIMimeTypeRegistryIID, NS_IMIMETYPEREGISTRY_IID);


////////////////////////////////////////////////////////////////////////


nsMimeTypeRegistry::nsMimeTypeRegistry(nsISupports* outer)
    : fHead(NULL)
{
    NS_INIT_AGGREGATED(outer);
}



nsMimeTypeRegistry::~nsMimeTypeRegistry(void)
{
    while (fHead != NULL) {
        Entry *next = fHead->fNext;
        fHead->fMimeType->Release();
        delete fHead;

        fHead = next;
    }
}

////////////////////////////////////////////////////////////////////////

NS_IMPL_AGGREGATED(nsMimeTypeRegistry);


NS_METHOD
nsMimeTypeRegistry::AggregatedQueryInterface(const nsIID& iid, void** instance)
{
    if (iid.Equals(kISupportsIID) ||
        iid.Equals(kIMimeTypeRegistryIID)) {
        *instance = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP_(nsresult)
nsMimeTypeRegistry::Register(nsIMimeType* mimeTypeClass)
{
    Entry *entry;

    for (entry = fHead; entry != NULL; entry = entry->fNext) {
        if (PL_strcasecmp(*entry->fMimeType, *mimeTypeClass) == 0)
            return NS_ERROR_FAILURE; // XXX
    }

    if ((entry = new Entry()) == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    /* Create local copies of the strings used to describe the plugin. */
    entry->fMimeType = mimeTypeClass;
    mimeTypeClass->AddRef();

    entry->fNext = fHead;
    fHead = entry;

    return NS_OK;
}

NS_IMETHODIMP_(nsresult)
nsMimeTypeRegistry::CreateMimeTypeEnumerator(nsIEnumerator* *result)
{
    (*result) = NULL;
    return NS_ERROR_FAILURE; // XXX
}


NS_IMETHODIMP_(XP_Bool)
nsMimeTypeRegistry::Find(const char* mimetype,
                         nsIMimeType* *result)
{
    Entry* entry;
    for (entry = fHead; entry != NULL; entry = entry->fNext) {
        if (PL_strcasecmp(*entry->fMimeType, mimetype) == 0)
            break;
    }

    if (entry == NULL)
        return FALSE;

    if (result) {
        (*result) = entry->fMimeType;
        (*result)->AddRef();
    }

    return TRUE;
}


NS_IMETHODIMP_(XP_Bool)
nsMimeTypeRegistry::FindClosestMatch(const char* pszMimeType,
                                     nsIMimeType* *result)
{
    Entry* entry;
    for (entry = fHead; entry != NULL; entry = entry->fNext) {
        const char* psz = (const char*) *(entry->fMimeType);

        PR_ASSERT(psz != NULL);
        if (psz[0] == '*')
            break; // wildcard handler

        if (PL_strcasecmp(psz, pszMimeType) == 0)
            break; // exact match

    }

    if (entry == NULL)
        return FALSE;

    if (result) {
        (*result) = entry->fMimeType;
        (*result)->AddRef();
    }

    return TRUE;
}


