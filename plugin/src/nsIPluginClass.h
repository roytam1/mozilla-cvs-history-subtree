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


#ifndef nsIPluginClass_h__
#define nsIPluginClass_h__

#include "nsIPlug.h" // for NPIPluginInstancePeer
#include "nsISupports.h"
#include "nsIContentHandler.h"
#include "prtypes.h"

////////////////////////////////////////////////////////////////////////

/**
 * A plugin class object, which encapsulates class information about a
 * particular plugin, including it's name, shared library filename, the
 * mime types it handles, etc.
 */

class nsIPluginClass : public nsIContentHandler
{
public:
    /**
     * Return the name of the plugin class
     */
    NS_IMETHOD_(char*)
    GetName(void) = 0;

    /**
     * Return the filename from which the plugin class was loaded.
     */
    NS_IMETHOD_(char*)
    GetDllFilename(void) = 0;

    /**
     * Return the description of the plugin class
     */
    NS_IMETHOD_(char*)
    GetDescription(void) = 0;

    /**
     * Indicate that the plugin class is enabled for business.
     */
    NS_IMETHOD_(void)
    SetEnabled(PRBool enabled) = 0;

    /**
     * Create a new plugin instance for this class.
     */
    NS_IMETHOD_(nsresult)
    NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result) = 0;

    /**
     * Remove the plugin instance from the set of instances
     * associated with the class
     */
    NS_IMETHOD_(nsresult)
    RemoveInstance(const NPIPluginInstance* instance) = 0;
};

// XXX Remember to get a GUID for this...
#define NS_IPLUGINCLASS_IID                          \
{ /* 5d852ef0-a1bc-11d1-85b1-00805f2e4dff */         \
    0x5d852ef0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x2e, 0x4d, 0xff} \
}



#endif // nsIPluginClass_h__
