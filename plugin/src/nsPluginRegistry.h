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

#ifndef nsPluginRegistry_h__
#define nsPluginRegistry_h__

#include "nsIPluginRegistry.h"
#include "nsIPluginClass.h"

#include "prlog.h"
#include "nsAgg.h" // nsPluginRegistry is aggregated

////////////////////////////////////////////////////////////////////////

/**
 * The registry of plugin types.
 */

class nsPluginRegistryPluginEnumerator;

class nsPluginRegistry : public nsIPluginRegistry
{
    friend class nsPluginRegistryPluginEnumerator;

    /**
     * Destroy the registry.
     */
    virtual ~nsPluginRegistry(void);

public:

    NS_IMETHOD_(nsresult)
    Register(nsIPluginClass* pluginClass);

    NS_IMETHOD_(nsresult)
    Unregister(nsIPluginClass* pluginClass);

    NS_IMETHOD_(void)
    CreatePluginEnumerator(nsIEnumerator* *result);

    NS_IMETHOD_(XP_Bool)
    FindByName(const char* name, nsIPluginClass* *pluginClass);

    NS_DECL_AGGREGATED

    nsPluginRegistry(nsISupports* outer);

protected:
    class Entry {
    public:
        nsIPluginClass* fClass;
        Entry* fNext;
    };

    Entry* fHead;
};




#endif // nsPluginRegistry_h__
