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

#ifndef npreg_h__
#define npreg_h__

#include "nsISupports.h"
#include "nsIEnumerator.h"
#include "nsIPluginClass.h"

////////////////////////////////////////////////////////////////////////


/**
 * The registry of plugin types.
 */

class nsIPluginRegistry : public nsISupports
{
public:

    /**
     * Add the plugin class to the registry.
     */
    NS_IMETHOD_(nsresult)
    Register(nsIPluginClass* pluginClass) = 0;

    /**
     * Remove the plugin class from the registry.
     */
    NS_IMETHOD_(nsresult)
    Unregister(nsIPluginClass* pluginClass) = 0;

    /** 
     * Create an iterator that will enumerate all of the plugins
     * in the registry.
     */
    NS_IMETHOD_(void)
    CreatePluginEnumerator(nsIEnumerator* *result) = 0;

    /**
     * Find the plugin class with the specified name.
     */
    NS_IMETHOD_(XP_Bool)
    FindByName(const char* name, nsIPluginClass* *pluginClass) = 0;

};


// XXX Remember to get a GUID for this...
#define NS_IPLUGINREGISTRY_IID                       \
{ /* 5d852ef0-a1bc-11d1-85ab-00805f0e4dff */         \
    0x5d852ef0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xab, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xff} \
}



#endif // npreg_h__

