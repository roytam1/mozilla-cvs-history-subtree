/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsRDFResource.h"
#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

////////////////////////////////////////////////////////////////////////

PRInt32        nsRDFResource::gRefCnt;
nsIRDFService* nsRDFResource::gRDFService;

////////////////////////////////////////////////////////////////////////////////

nsRDFResource::nsRDFResource(void)
    : mURI(nsnull)
{
    NS_INIT_REFCNT();

    if (gRefCnt++ == 0) {
        nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
                                                   nsIRDFService::GetIID(),
                                                   (nsISupports**) &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
    }
}

nsRDFResource::~nsRDFResource(void)
{
    gRDFService->UnregisterResource(this);

    if (--gRefCnt == 0) {
        nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
        gRDFService = nsnull;
    }
}

NS_IMPL_ADDREF(nsRDFResource)
NS_IMPL_RELEASE(nsRDFResource)

NS_IMETHODIMP
nsRDFResource::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(nsIRDFResource::GetIID()) ||
        iid.Equals(nsIRDFNode::GetIID()) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFResource*, this);
        AddRef();
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRDFNode methods:

NS_IMETHODIMP
nsRDFResource::Init(char* uri)
{
    mURI = uri;
    if (! mURI.get())
        return NS_ERROR_OUT_OF_MEMORY;

    return gRDFService->RegisterResource(this, PR_TRUE);
}

NS_IMETHODIMP
nsRDFResource::EqualsNode(nsIRDFNode* node, PRBool* result)
{
    nsresult rv;
    nsIRDFResource* resource;
    if (NS_SUCCEEDED(node->QueryInterface(nsIRDFResource::GetIID(), (void**)&resource))) {
        rv = EqualsResource(resource, result);
        NS_RELEASE(resource);
    }
    else {
        *result = PR_FALSE;
        rv = NS_OK;
    }
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRDFResource methods:

NS_IMETHODIMP
nsRDFResource::GetValue(char* *uri)
{
    NS_PRECONDITION(uri != nsnull, "null ptr");
    if (!uri)
        return NS_ERROR_NULL_POINTER;

    *uri = mURI.ToNewCString();
    return NS_OK;
}

NS_IMETHODIMP
nsRDFResource::EqualsResource(nsIRDFResource* resource, PRBool* result)
{
    if (!resource || !result)
        return NS_ERROR_NULL_POINTER;

    nsCOMCString uri;
    if (NS_SUCCEEDED(resource->GetValue( getter_Copies(uri) ))) {
        return NS_SUCCEEDED(EqualsString(NS_CONST_CAST(char*, uri.get()), result)) ? NS_OK : NS_ERROR_FAILURE;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsRDFResource::EqualsString(char* uri, PRBool* result)
{
    if (!uri || !result)
        return NS_ERROR_NULL_POINTER;
    *result = nsCRT::strcmp(uri, mURI) == 0;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
