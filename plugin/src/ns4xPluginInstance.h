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

#ifndef ns4xPluginInstance_h__
#define ns4xPluginInstance_h__

#include "nsIPlug.h" // for NPI* interfaces
#include "npupp.h"
#include "jri.h"

////////////////////////////////////////////////////////////////////////

class ns4xPluginInstance : public NPIPluginInstance {
public:

    ////////////////////////////////////////////////////////////////////////
    // NPIPluginInstance methods

    NS_IMETHOD_(NPPluginError)
    Start(void);

    NS_IMETHOD_(NPPluginError)
    Stop(void);

    NS_IMETHOD_(NPPluginError)
    SetWindow(NPPluginWindow* window);

    NS_IMETHOD_(NPPluginError)
    NewStream(NPIPluginStreamPeer* peer, NPIPluginStream* *result);

    NS_IMETHOD_(void)
    Print(NPPluginPrint* platformPrint);

    NS_IMETHOD_(PRInt16)
    HandleEvent(NPPluginEvent* event);

    NS_IMETHOD_(void)
    URLNotify(const char* url, const char* target,
              NPPluginReason reason, void* notifyData);

    NS_IMETHOD_(NPPluginError)
    GetValue(NPPluginVariable variable, void *value);

    NS_IMETHOD_(NPPluginError)
    SetValue(NPPluginManagerVariable variable, void *value);

    NS_IMETHOD_(jobject)
    GetJavaPeer(void);

    ////////////////////////////////////////////////////////////////////////
    // ns4xPluginInstance-specific methods

    /**
     * Construct a new 4.x plugin instance with the specified peer
     * and callbacks.
     */
    ns4xPluginInstance(NPIPluginInstancePeer* peer, NPPluginFuncs* callbacks);

    /**
     * Actually initialize the plugin instance. This calls the 4.x <b>newp</b>
     * callback, and may return an error (which is why it is distinct from the
     * constructor.) If an error is returned, the caller should <i>not</i>
     * continue to use the <b>ns4xPluginInstance</b> object.
     */
    NS_METHOD_(NPPluginError)
    Initialize(void);

    /**
     * Return the 4.x-style interface object.
     */
    NS_METHOD_(NPP)
    GetNPP(void) {
        return &fNPP;
    };

    /**
     * Return the callbacks for the plugin instance.
     */
    NS_METHOD_(const NPPluginFuncs*)
    GetCallbacks(void) {
        return fCallbacks;
    };

    NS_DECL_ISUPPORTS

protected:
    // Use Release() to destroy this
    virtual ~ns4xPluginInstance(void);

    /**
     * The plugin instance peer for this instance.
     */
    NPIPluginInstancePeer* fPeer;

    /**
     * A pointer to the plugin's callback functions. This information
     * is actually stored in the plugin class (<b>nsPluginClass</b>),
     * and is common for all plugins of the class.
     */
    NPPluginFuncs* fCallbacks;

    /**
     * The 4.x-style structure used to communicate between the plugin
     * instance and the browser.
     */
    NPP_t fNPP;
};


#endif // ns4xPluginInstance_h__
