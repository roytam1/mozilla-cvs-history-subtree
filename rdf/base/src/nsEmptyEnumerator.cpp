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

/*

  A set of "empty cursors" (nsIRDFAssertionCursor,
  nsIRDFArcsOutCursor, nsIRDFArcsInCursor) that can be used to ensure
  that the data source methods which return a cursor always return
  *something*.

 */

#include "nscore.h"
#include "nsIRDFEnumerator.h"
#include "rdf.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

////////////////////////////////////////////////////////////////////////

class EmptyEnumeratorImpl : public nsIRDFEnumerator
{
public:
    EmptyEnumeratorImpl(void) {};
    virtual ~EmptyEnumeratorImpl(void) {};

    // nsISupports interface
    NS_IMETHOD_(nsrefcnt) AddRef(void) {
        return 2;
    }

    NS_IMETHOD_(nsrefcnt) Release(void) {
        return 1;
    }

    NS_IMETHOD QueryInterface(REFNSIID iid, void** result) {
        if (! result)
            return NS_ERROR_NULL_POINTER;

        if (iid.Equals(nsIRDFEnumerator::GetIID()) ||
            iid.Equals(kISupportsIID)) {
            *result = NS_STATIC_CAST(nsIRDFEnumerator*, this);
            NS_ADDREF(this);
            return NS_OK;
        }
        return NS_NOINTERFACE;
    }

    // nsIRDFEnumerator
    NS_IMETHOD HasMoreElements(PRBool* aResult) {
        *aResult = PR_FALSE;
        return NS_OK;
    }

    NS_IMETHOD GetNext(nsISupports** aResult) {
        return NS_ERROR_UNEXPECTED;
    }
};

nsresult
NS_NewEmptyEnumerator(nsIRDFEnumerator** aResult)
{
    static EmptyEnumeratorImpl gEmptyEnumerator;
    *aResult = &gEmptyEnumerator;
    return NS_OK;
}

