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

#include "nsDBEnumerator.h"
#include "nsDiskCacheRecord.h"

nsDBEnumerator::nsDBEnumerator(nsIDBAccessor* aDB) :
  m_DB(aDB) 
{
  NS_INIT_REFCNT();

  tempEntry = nsnull ;

  bReset = PR_TRUE ;

  m_CacheEntry = new nsDiskCacheRecord(m_DB) ;
  if(m_CacheEntry)
    NS_ADDREF(m_CacheEntry) ;

}

nsDBEnumerator::~nsDBEnumerator()
{
//  printf(" ~nsDBEnumerator()\n") ;
  NS_IF_RELEASE(m_CacheEntry) ;
}

//
// Implement nsISupports methods
//
NS_IMPL_ISUPPORTS(nsDBEnumerator, NS_GET_IID(nsIEnumerator))

/////////////////////////////////////////////////////////////////
// nsISimpleEnumerator methods

NS_IMETHODIMP
nsDBEnumerator::HasMoreElements(PRBool *_retval)
{
  *_retval = PR_FALSE ;

  m_DB->EnumEntry(&tempEntry, &tempEntry_length, bReset) ;
  bReset = PR_FALSE ;

  if(tempEntry && tempEntry_length != 0)
    *_retval = PR_TRUE ;

  return NS_OK ;
}

// this routine does not create a new item by itself  
// Rather it reuses the item inside the object. So if you need to use the 
// item later, you have to
// create a new item specifically, using copy constructor or some other dup 
// function. And don't forget to release it after you're done
//
NS_IMETHODIMP
nsDBEnumerator::GetNext(nsISupports **_retval)
{
  if(!_retval)
    return NS_ERROR_NULL_POINTER ;
  *_retval = nsnull ;

  NS_ASSERTION(m_CacheEntry, "cache entry doesn't exist, something's wrong") ;

  // I don't like this cast. But don't know what to do else.
  nsDiskCacheRecord* p_CacheEntry ;
  p_CacheEntry = NS_STATIC_CAST(nsDiskCacheRecord*, m_CacheEntry) ;

  p_CacheEntry->RetrieveInfo(tempEntry, tempEntry_length) ;

  *_retval = NS_STATIC_CAST(nsISupports*, m_CacheEntry) ;
  NS_ADDREF(*_retval) ; // all good getter addref 

  return NS_OK ;
}
