/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsCOMPtr.h"
#include "nsIEnumerator.h"

////////////////////////////////////////////////////////////////////////////////
// Intersection Enumerators
////////////////////////////////////////////////////////////////////////////////

class nsConjoiningEnumerator : public nsIBidirectionalEnumerator
{
public:
  NS_DECL_ISUPPORTS

  // nsIEnumerator methods:
  NS_IMETHOD HasMoreElements(PRBool* aResult);
  NS_IMETHOD GetNext(nsISupports** aResult);

  // nsIBidirectionalEnumerator methods:
  NS_IMETHOD HasPreviousElements(PRBool* aResult);;
  NS_IMETHOD GetPrev(nsISupports** aResult);

  // nsConjoiningEnumerator methods:
  nsConjoiningEnumerator(nsIEnumerator* first, nsIEnumerator* second);
  virtual ~nsConjoiningEnumerator(void);

protected:
  nsIEnumerator* mFirst;
  nsIEnumerator* mSecond;
  nsIEnumerator* mCurrent;
};

nsConjoiningEnumerator::nsConjoiningEnumerator(nsIEnumerator* first, nsIEnumerator* second)
  : mFirst(first), mSecond(second), mCurrent(first)
{
  NS_ADDREF(mFirst);
  NS_ADDREF(mSecond);
}

nsConjoiningEnumerator::~nsConjoiningEnumerator(void)
{
  NS_RELEASE(mFirst);
  NS_RELEASE(mSecond);
}

NS_IMPL_ADDREF(nsConjoiningEnumerator);
NS_IMPL_RELEASE(nsConjoiningEnumerator);

NS_IMETHODIMP
nsConjoiningEnumerator::QueryInterface(REFNSIID aIID, void** aInstancePtr)
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
nsConjoiningEnumerator::HasMoreElements(PRBool* aResult)
{
  nsresult rv;
  rv = mCurrent->HasMoreElements(aResult);
  if (NS_FAILED(rv)) return rv;

  // okay, the current one has more elements. that's all we wanted to know.
  if (*aResult)
    return NS_OK;

  // if the current one has no more elements, but we're still looking
  // at the first enumerator, then switch to the second and try again.
  if ((! *aResult) && (mCurrent == mFirst))
    mCurrent = mSecond;

  return mCurrent->HasMoreElements(aResult);
}

NS_IMETHODIMP 
nsConjoiningEnumerator::GetNext(nsISupports** aResult)
{
  nsresult rv;

  PRBool hasMore;
  rv = HasMoreElements(&hasMore);
  if (NS_FAILED(rv)) return rv;

  if (! hasMore)
    return NS_ERROR_UNEXPECTED;

  return mCurrent->GetNext(aResult);
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP 
nsConjoiningEnumerator::HasPreviousElements(PRBool* aResult)
{
  nsCOMPtr<nsIBidirectionalEnumerator> current;

  // get the bi-directional interface on the current enumerator
  if (! (current = do_QueryInterface(mCurrent)))
    return NS_ERROR_FAILURE;

  nsresult rv;
  rv = current->HasPreviousElements(aResult);
  if (NS_FAILED(rv)) return rv;

  // okay, the current one has previous elements. that's all we wanted to know
  if (*aResult)
    return NS_OK;

  // if the current one has no previous elements, and we're looking at
  // the second enumerator, then switch to the first and try again.
  if ((! *aResult) && (mCurrent == mSecond))
    mCurrent = mFirst;

  if (! (current = do_QueryInterface(mCurrent)))
    return NS_ERROR_FAILURE;

  return current->HasPreviousElements(aResult);
}

NS_IMETHODIMP 
nsConjoiningEnumerator::GetPrev(nsISupports** aResult)
{
  nsresult rv;

  nsCOMPtr<nsIBidirectionalEnumerator> current;

  // get the bi-directional interface on the current enumerator
  if (! (current = do_QueryInterface(mCurrent)))
    return NS_ERROR_FAILURE;

  PRBool hasPrevious;
  rv = current->HasPreviousElements(&hasPrevious);
  if (NS_FAILED(rv)) return rv;

  if (! hasPrevious)
    return NS_ERROR_UNEXPECTED;

  // because mCurrent may have changed in HasPreviousElements()
  if (! (current = do_QueryInterface(mCurrent)))
    return NS_ERROR_FAILURE;
  
  return current->GetPrev(aResult);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" NS_COM nsresult
NS_NewConjoiningEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                           nsIBidirectionalEnumerator* *aInstancePtrResult)
{
  if (aInstancePtrResult == 0)
    return NS_ERROR_NULL_POINTER;
  nsConjoiningEnumerator* e = new nsConjoiningEnumerator(first, second);
  if (e == 0)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(e);
  *aInstancePtrResult = e;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

static nsresult
nsEnumeratorContains(nsIEnumerator* e, nsISupports* item, PRBool* aResult)
{
  *aResult = PR_FALSE;
  while (1) {
    nsresult rv;

    PRBool hasMore;
    rv = e->HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;

    if (! hasMore)
      break;

    nsCOMPtr<nsISupports> other;
    rv = e->GetNext(getter_AddRefs(other));
    if (NS_FAILED(rv)) return rv;

    if (other.get() == item) {
      *aResult = PR_TRUE;
      break;
    }
  }
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Intersection Enumerators
////////////////////////////////////////////////////////////////////////////////

class nsIntersectionEnumerator : public nsIEnumerator
{
public:
  NS_DECL_ISUPPORTS

  // nsIEnumerator methods:
  NS_IMETHOD HasMoreElements(PRBool* aResult);
  NS_IMETHOD GetNext(nsISupports** aResult);

  // nsIntersectionEnumerator methods:
  nsIntersectionEnumerator(nsIEnumerator* first, nsIEnumerator* second);
  virtual ~nsIntersectionEnumerator(void);

protected:
  nsIEnumerator* mFirst;
  nsIEnumerator* mSecond;
};

nsIntersectionEnumerator::nsIntersectionEnumerator(nsIEnumerator* first, nsIEnumerator* second)
  : mFirst(first), mSecond(second)
{
  NS_ADDREF(mFirst);
  NS_ADDREF(mSecond);
}

nsIntersectionEnumerator::~nsIntersectionEnumerator(void)
{
  NS_RELEASE(mFirst);
  NS_RELEASE(mSecond);
}

NS_IMPL_ISUPPORTS(nsIntersectionEnumerator, nsIEnumerator::GetIID());

NS_IMETHODIMP 
nsIntersectionEnumerator::HasMoreElements(PRBool* aResult)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsIntersectionEnumerator::GetNext(nsISupports** aResult)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" NS_COM nsresult
NS_NewIntersectionEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                      nsIEnumerator* *aInstancePtrResult)
{
  if (aInstancePtrResult == 0)
    return NS_ERROR_NULL_POINTER;
  nsIntersectionEnumerator* e = new nsIntersectionEnumerator(first, second);
  if (e == 0)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(e);
  *aInstancePtrResult = e;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Union Enumerators
////////////////////////////////////////////////////////////////////////////////

class nsUnionEnumerator : public nsIEnumerator
{
public:
  NS_DECL_ISUPPORTS

  // nsIEnumerator methods:
  NS_IMETHOD HasMoreElements(PRBool* aResult);
  NS_IMETHOD GetNext(nsISupports** aResult);

  // nsUnionEnumerator methods:
  nsUnionEnumerator(nsIEnumerator* first, nsIEnumerator* second);
  virtual ~nsUnionEnumerator(void);

protected:
  nsIEnumerator* mFirst;
  nsIEnumerator* mSecond;
};

nsUnionEnumerator::nsUnionEnumerator(nsIEnumerator* first, nsIEnumerator* second)
  : mFirst(first), mSecond(second)
{
  NS_ADDREF(mFirst);
  NS_ADDREF(mSecond);
}

nsUnionEnumerator::~nsUnionEnumerator(void)
{
  NS_RELEASE(mFirst);
  NS_RELEASE(mSecond);
}

NS_IMPL_ISUPPORTS(nsUnionEnumerator, nsIEnumerator::GetIID());

NS_IMETHODIMP 
nsUnionEnumerator::HasMoreElements(PRBool* aResult)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsUnionEnumerator::GetNext(nsISupports** aResult)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" NS_COM nsresult
NS_NewUnionEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                      nsIEnumerator* *aInstancePtrResult)
{
  if (aInstancePtrResult == 0)
    return NS_ERROR_NULL_POINTER;
  nsUnionEnumerator* e = new nsUnionEnumerator(first, second);
  if (e == 0)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(e);
  *aInstancePtrResult = e;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
