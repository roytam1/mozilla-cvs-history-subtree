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

#include "nsEnumeratorUtils.h"
#include "nscore.h"

nsArrayEnumerator::nsArrayEnumerator(nsISupportsArray* aValueArray)
    : mValueArray(aValueArray),
      mIndex(0)
{
    NS_INIT_REFCNT();
    NS_IF_ADDREF(mValueArray);
}

nsArrayEnumerator::~nsArrayEnumerator(void)
{
    NS_IF_RELEASE(mValueArray);
}

NS_IMPL_ISUPPORTS(nsArrayEnumerator, nsIRDFEnumerator::GetIID());

NS_IMETHODIMP
nsArrayEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    *aResult = (mIndex < (PRInt32) mValueArray->Count());
    return NS_OK;
}

NS_IMETHODIMP
nsArrayEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mIndex >= (PRInt32) mValueArray->Count())
        return NS_ERROR_UNEXPECTED;

    *aResult = mValueArray->ElementAt(mIndex++);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

nsSingletonEnumerator::nsSingletonEnumerator(nsISupports* aValue)
    : mValue(aValue)
{
    NS_INIT_REFCNT();
    NS_IF_ADDREF(mValue);
    mConsumed = (mValue ? PR_FALSE : PR_TRUE);
}

nsSingletonEnumerator::~nsSingletonEnumerator()
{
    NS_IF_RELEASE(mValue);
}

NS_IMPL_ISUPPORTS(nsSingletonEnumerator, nsIRDFEnumerator::GetIID());

NS_IMETHODIMP
nsSingletonEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    *aResult = !mConsumed;
    return NS_OK;
}


NS_IMETHODIMP
nsSingletonEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mConsumed)
        return NS_ERROR_UNEXPECTED;

    mConsumed = PR_TRUE;

    NS_ADDREF(mValue);
    *aResult = mValue;
    return NS_OK;
}
