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

#ifndef nsMimeTypeRegistry_h__
#define nsMimeTypeRegistry_h__

#include "nsIMimeTypeRegistry.h"
#include "nsIEnumerator.h"
#include "nsIMimeType.h"
#include "nsAgg.h"
#include "prtypes.h"

////////////////////////////////////////////////////////////////////////

/**
 * The registry of mime types for a plugin. Lists all of the mime type
 * classes that are associated with a particular plugin class.
 */

class nsMimeTypeRegistryEnumerator;

class nsMimeTypeRegistry : public nsIMimeTypeRegistry
{
    friend class nsMimeTypeRegistryEnumerator;

protected:
    /**
     * Destroy the registry.
     */
    virtual ~nsMimeTypeRegistry(void);

public:

    NS_IMETHOD_(nsresult)
    Register(nsIMimeType* mimeType);

    NS_IMETHOD_(nsresult)
    CreateMimeTypeEnumerator(nsIEnumerator* *result);

    NS_IMETHOD_(XP_Bool)
    Find(const char* type, nsIMimeType* *result);

    NS_IMETHOD_(PRBool)
    FindClosestMatch(const char* mimetype, nsIMimeType* *result);

    ////////////////////////////////////////////////////////////////////////

    NS_DECL_AGGREGATED

    /**
     * Create a new, empty mime type registry.
     */
    nsMimeTypeRegistry(nsISupports* outer);


protected:

    /**
     * An entry in the registry.
     */
    class Entry {
    public:
        nsIMimeType* fMimeType;
        Entry* fNext;
    };

    /**
     * The head of the linked-list of entries in the registry.
     */
    Entry* fHead;
};



#endif // nsMimeTypeRegistry_h__
