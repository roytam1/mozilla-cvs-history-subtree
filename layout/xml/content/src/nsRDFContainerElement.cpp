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

#include "nsRDFContainerElement.h"

static NS_DEFINE_IID(kIRDFContainerContentIID, NS_IRDFCONTAINERCONTENT_IID);

nsresult
NS_NewRDFContainerElement(nsIRDFContainerContent** result)
{
    NS_PRECONDITION(result, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsIRDFContainerContent* element = new nsRDFContainerElement();
    if (! element)
        return NS_ERROR_OUT_OF_MEMORY;

    return element->QueryInterface(kIRDFContainerContentIID, (void**) result);
}

nsRDFContainerElement::nsRDFContainerElement(void)
{
}
 
nsRDFContainerElement::~nsRDFContainerElement()
{
}

NS_IMETHODIMP 
nsRDFContainerElement::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIRDFContainerContentIID)) {
        *result = static_cast<nsIRDFContainerContent*>(this);
        AddRef();
        return NS_OK;
    }

    return nsRDFElement::QueryInterface(iid, result);
}
////////////////////////////////////////////////////////////////////////
// nsIRDFContainerContent

NS_IMETHODIMP
nsRDFContainerElement::GetElementCount(PRUint32& rCount)
{
    rCount = 0; // XXX
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Implementation methods

