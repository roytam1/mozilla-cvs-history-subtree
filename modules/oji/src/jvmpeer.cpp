/* -*- Mode: C++; tb-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager Methods to support the JVM Plugin API
////////////////////////////////////////////////////////////////////////////////

#include "lo_ele.h"
#include "layout.h"
#include "shist.h"
#include "jvmmgr.h"
#include "plstr.h"

////////////////////////////////////////////////////////////////////////////////
// Delegated methods:

NS_METHOD_(NPIPlugin*)
JVMInstancePeer::GetClass(void)
{
    return fPluginInstancePeer->GetClass();
}

NS_METHOD_(nsMIMEType)
JVMInstancePeer::GetMIMEType(void)
{
    return fPluginInstancePeer->GetMIMEType();
}

NS_METHOD_(NPPluginType)
JVMInstancePeer::GetMode(void)
{
    return fPluginInstancePeer->GetMode();
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::GetAttributes(PRUint16& n, const char*const*& names, const char*const*& values)
{
    return fPluginInstancePeer->GetAttributes(n, names, values);
}

NS_METHOD_(const char*)
JVMInstancePeer::GetAttribute(const char* name)
{
    return fPluginInstancePeer->GetAttribute( name );
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::GetParameters(PRUint16& n, const char*const*& names, const char*const*& values)
{
    return fPluginInstancePeer->GetParameters(n, names, values);
}

NS_METHOD_(const char*)
JVMInstancePeer::GetParameter(const char* name)
{
    return fPluginInstancePeer->GetParameter( name );
}

NS_METHOD_(NPTagType)
JVMInstancePeer::GetTagType(void)
{
    return fPluginInstancePeer->GetTagType();
}

NS_METHOD_(const char *)
JVMInstancePeer::GetTagText(void)
{
    return fPluginInstancePeer->GetTagText();
}

NS_METHOD_(NPIPluginManager*)
JVMInstancePeer::GetPluginManager(void)
{
    return fPluginInstancePeer->GetPluginManager();
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::GetURL(const char* url, const char* target, void* notifyData,
                        const char* altHost, const char* referrer,
                        PRBool forceJSEnabled)
{
    return fPluginInstancePeer->GetURL(url, target, notifyData, altHost, referrer, forceJSEnabled);
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::PostURL(const char* url, const char* target, PRUint32 bufLen, 
                         const char* buf, PRBool file, void* notifyData,
                         const char* altHost, const char* referrer,
                         PRBool forceJSEnabled,
                         PRUint32 postHeadersLength, const char* postHeaders)
{
    return fPluginInstancePeer->PostURL(url, target,
                                        bufLen, buf, file, notifyData,
                                        altHost, referrer, forceJSEnabled,
                                        postHeadersLength, postHeaders);
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::NewStream(nsMIMEType type, const char* target,
                           NPIPluginManagerStream* *result)
{
    return fPluginInstancePeer->NewStream(type, target, result);
}

NS_METHOD_(void)
JVMInstancePeer::ShowStatus(const char* message)
{
    fPluginInstancePeer->ShowStatus(message);
}

NS_METHOD_(const char*)
JVMInstancePeer::UserAgent(void)
{
    return fPluginInstancePeer->UserAgent();
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::GetValue(NPPluginManagerVariable variable, void *value)
{
    return fPluginInstancePeer->GetValue(variable, value);
}

NS_METHOD_(NPPluginError)
JVMInstancePeer::SetValue(NPPluginVariable variable, void *value)
{
    return fPluginInstancePeer->SetValue(variable, value);
}

NS_METHOD_(void)
JVMInstancePeer::InvalidateRect(nsRect *invalidRect)
{
    fPluginInstancePeer->InvalidateRect(invalidRect);
}

NS_METHOD_(void)
JVMInstancePeer::InvalidateRegion(nsRegion invalidRegion)
{
    fPluginInstancePeer->InvalidateRegion(invalidRegion);
}

NS_METHOD_(void)
JVMInstancePeer::ForceRedraw(void)
{
    fPluginInstancePeer->ForceRedraw();
}

NS_METHOD_(void)
JVMInstancePeer::RegisterWindow(void* window)
{
    fPluginInstancePeer->RegisterWindow(window);
}
    
NS_METHOD_(void)
JVMInstancePeer::UnregisterWindow(void* window)
{
    fPluginInstancePeer->UnregisterWindow(window);
}

NS_METHOD_(PRInt16)
JVMInstancePeer::AllocateMenuID(PRBool isSubmenu)
{
    return fPluginInstancePeer->AllocateMenuID(isSubmenu);
}

////////////////////////////////////////////////////////////////////////////////
// Non-delegated methods:

////////////////////////////////////////////////////////////////////////////////

