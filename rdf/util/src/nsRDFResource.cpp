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
#include "prlog.h"

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
    if (mURI) {
        gRDFService->UnregisterResource(mURI, this);
        // N.B. that we need to free the URI *after* we un-cache the resource,
        // due to the way that the resource manager is implemented.
        delete[] mURI;
    }
    else {
        const char* uri;
        nsresult rv = gRDFService->GetURI(this, &uri);
        if (NS_SUCCEEDED(rv))
            gRDFService->UnregisterResource(uri, this);
    }
        
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
    if (iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsISupports*, this);
        NS_ADDREF_THIS();
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////////////
