/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nsSupportsArrayEnumerator.h"
#include "nsISupportsArray.h"

nsSupportsArrayEnumerator::nsSupportsArrayEnumerator(nsISupportsArray* array)
  : mArray(array), mCursor(0)
{
  NS_INIT_REFCNT();
  NS_ASSERTION(array, "null array");
  NS_ADDREF(mArray);
}

nsSupportsArrayEnumerator::~nsSupportsArrayEnumerator()
{
  NS_RELEASE(mArray);
}

NS_IMPL_ADDREF(nsSupportsArrayEnumerator);
NS_IMPL_RELEASE(nsSupportsArrayEnumerator);

NS_IMETHODIMP
nsSupportsArrayEnumerator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr)
    return NS_ERROR_NULL_POINTER; 

  if (aIID.Equals(nsIBidirectionalEnumerator::GetIID()) || 
      aIID.Equals(nsIEnumerator::GetIID()) || 
      aIID.Equals(nsISupports::GetIID())) {
    *aInstancePtr = (void*) this; 
    NS_ADDREF_THIS(); 
    return NS_OK; 
  } 
  return NS_NOINTERFACE; 
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::HasMoreElements(PRBool* aResult)
{
  NS_PRECONDITION(aResult, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = mCursor < PRInt32(mArray->Count());
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::GetNext(nsISupports** aResult)
{
  NS_PRECONDITION(aResult, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  if (mCursor >= PRInt32(mArray->Count()))
    return NS_ERROR_UNEXPECTED;

  // nsISupportsArray addref's the return value.
  *aResult = mArray->ElementAt(mCursor++);
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsSupportsArrayEnumerator::HasPreviousElements(PRBool* aResult)
{
  NS_PRECONDITION(aResult, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = (mCursor > 0);
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::GetPrev(nsISupports** aResult)
{
  NS_PRECONDITION(aResult, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  if (mCursor <= 0)
    return NS_ERROR_UNEXPECTED;

  // nsISupportsArray addref's the return value.
  *aResult = mArray->ElementAt(mCursor--);
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_NewISupportsArrayEnumerator(nsISupportsArray* array,
                               nsIBidirectionalEnumerator* *aInstancePtrResult)
{
  if (aInstancePtrResult == 0)
    return NS_ERROR_NULL_POINTER;
  nsSupportsArrayEnumerator* e = new nsSupportsArrayEnumerator(array);
  if (e == 0)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(e);
  *aInstancePtrResult = e;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

