/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/*



*/

#include "nsCOMPtr.h"
#include "nsIXULPrototypeDocument.h"
#include "nsIURI.h"

class nsXULPrototypeDocument : public nsIXULPrototypeDocument
{
public:
    static nsresult
    Create(nsIURI* aURI, nsXULPrototypeDocument** aResult);

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIXULPrototypeDocument interface
    NS_IMETHOD GetURI(nsIURI** aResult);

    NS_IMETHOD GetRootElement(nsXULPrototypeElement** aResult);
    NS_IMETHOD SetRootElement(nsXULPrototypeElement* aElement);

    NS_IMETHOD AddStyleSheet(nsIStyleSheet* aStyleSheet);
    NS_IMETHOD GetStyleSheets(nsVoidArray& aResult);

    NS_IMETHOD AddOverlayReference(nsIURI* aURI);
    NS_IMETHOD GetOverlayReferences(nsVoidArray& aResult);

protected:
    nsCOMPtr<nsIURI> mURI;
    nsXULPrototypeElement* mRoot;

    nsXULPrototypeDocument();
    virtual ~nsXULPrototypeDocument();
};


////////////////////////////////////////////////////////////////////////

nsXULPrototypeDocument::nsXULPrototypeDocument()
    : mRoot(nsnull)
{
    NS_INIT_REFCNT();
}


nsXULPrototypeDocument::~nsXULPrototypeDocument()
{
    // destroy XULPrototype elements, etc.
}


NS_IMPL_ISUPPORTS1(nsXULPrototypeDocument, nsIXULPrototypeDocument);


NS_IMETHODIMP
nsXULPrototypeDocument::GetURI(nsIURI** aResult)
{
    *aResult = mURI;
    NS_IF_ADDREF(*aResult);
    return NS_OK;
}


NS_IMETHODIMP
nsXULPrototypeDocument::GetRootElement(nsXULPrototypeElement** aResult)
{
    *aResult = mRoot;
    return NS_OK;
}


NS_IMETHODIMP
nsXULPrototypeDocument::SetRootElement(nsXULPrototypeElement* aElement)
{
    mRoot = aElement;
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

