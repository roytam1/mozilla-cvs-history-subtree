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

#ifndef nsPluginClass_h__
#define nsPluginClass_h__

#include "nsIPlug.h" // for NPI* interfaces
#include "nsIPluginClass.h"
#include "nsIMimeType.h"
#include "nsIEnumerator.h"
#include "nsMimeTypeRegistry.h"
#include "prlink.h"

////////////////////////////////////////////////////////////////////////


/**
 * A plugin class object, which encapsulates class information about a
 * particular plugin, including it's name, shared library filename, the
 * mime types it handles, etc.
 */

class nsPluginClass : public nsIPluginClass
{
protected:    
    char*  fName;
    char*  fDllFilename;
    char*  fDescription;
    PRLibrary* fDll;
    NPIPlugin* fFactory;
    PRBool fEnabled;

    class Instance {
    public:
        Instance*          fNext;
        NPIPluginInstance* fInstance;
    };

    Instance* fInstances;

    virtual ~nsPluginClass(void);

public:

    ////////////////////////////////////////////////////////////////////////
    // nsIContentHandler methods

    NS_IMETHOD_(NET_StreamClass*)
    CreateStream(FO_Present_Types format_out,
                 URL_Struct* urls,
                 MWContext* cx);

    ////////////////////////////////////////////////////////////////////////
    // nsIPluginClass methods

    NS_IMETHOD_(char*)
    GetName(void) { return fName; };

    NS_IMETHOD_(char*)
    GetDllFilename(void) { return fDllFilename; };

    NS_IMETHOD_(char*)
    GetDescription(void) { return fDescription; };

    NS_IMETHOD_(void)
    SetEnabled(PRBool enabled);

    NS_IMETHOD_(nsresult)
    NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result);

    NS_IMETHOD_(nsresult)
    RemoveInstance(const NPIPluginInstance* instance);

    ////////////////////////////////////////////////////////////////////////
    // nsPluginClass methods

    nsPluginClass(char* name,
                  char* filename,
                  char* description);

    NS_DECL_ISUPPORTS

private:
    // XXX Not meant to be used.
    nsPluginClass(void) {
        PR_ASSERT(FALSE);
    };

    // XXX Not meant to be used.
    nsPluginClass(nsPluginClass& obj) {
        PR_ASSERT(FALSE);
    };

    // XXX Not meant to be used.
    virtual nsPluginClass& operator
    =(nsPluginClass& obj) {
        PR_ASSERT(FALSE);
        return *(new nsPluginClass());
    };
};


#endif // nsPluginClass_h__
