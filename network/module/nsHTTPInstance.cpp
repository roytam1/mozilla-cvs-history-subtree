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

//Note eventually this file moves to plughttp.dll. post DP work.
//
#include "nsHTTPInstance.h"

nsHTTPInstance::nsHTTPInstance(nsICoolURL* i_URL):m_pURL(i_URL)
{
}

nsHTTPInstance::~nsHTTPInstance()
{
}

NS_IMPL_ADDREF(nsHTTPInstance);

nsresult
nsHTTPInstance::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

    if (aIID.Equals(nsIProtocolInstance::GetIID())) {
        *aInstancePtr = (void*) ((nsIProtocolInstance*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(nsIHTTPInstance::GetIID())) {
        *aInstancePtr = (void*) ((nsIHTTPInstance*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}
 
NS_IMPL_RELEASE(nsHTTPInstance);

nsresult
nsHTTPInstance::GetInputStream( nsIInputStream* *o_Stream)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHTTPInstance::SetHeader(const char* i_Header, const char* i_Value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHTTPInstance::GetHeader(const char* i_Header, const char* *o_Value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
