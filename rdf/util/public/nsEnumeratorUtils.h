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

#ifndef nsRDFCursorUtils_h__
#define nsRDFCursorUtils_h__

#include "nsIRDFEnumerator.h"
#include "nsIRDFNode.h"
#include "rdf.h"

class NS_RDF nsArrayEnumerator : public nsIRDFEnumerator
{
public:
    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIRDFEnumerator interface
    NS_IMETHOD HasMoreElements(PRBool* aResult);
    NS_IMETHOD GetNext(nsISupports** aResult);

    // nsRDFArrayEnumerator methods
    nsArrayEnumerator(nsISupportsArray* aValueArray);
    virtual ~nsArrayEnumerator(void);

protected:
    nsISupportsArray* mValueArray;
    PRInt32 mIndex;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsSingletonEnumerator : public nsIRDFEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsIRDFEnumerator methods
    NS_IMETHOD HasMoreElements(PRBool* aResult);
    NS_IMETHOD GetNext(nsISupports** aResult);

    nsSingletonEnumerator(nsISupports* aValue);
    virtual ~nsSingletonEnumerator();

protected:
    nsISupports* mValue;
    PRBool mConsumed;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsAdapterEnumerator : public nsIRDFEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsIRDFEnumerator methods
    NS_IMETHOD HasMoreElements(PRBool* aResult);
    NS_IMETHOD GetNext(nsISupports** aResult);

    nsAdapterEnumerator(nsIEnumerator* aEnum);
    virtual ~nsAdapterEnumerator();

protected:
    nsIEnumerator* mEnum;
    nsISupports*   mCurrent;
    PRBool mStarted;
};

////////////////////////////////////////////////////////////////////////

#endif /* nsRDFCursorUtils_h__ */
