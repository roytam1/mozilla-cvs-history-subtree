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

#include "nsDBAccessor.h"
#include "nscore.h"

#include "prtypes.h"
#include "plhash.h"
#include "nsCRT.h"
#include "nsAutoLock.h"

nsDBAccessor::nsDBAccessor() :
  mDB(0) ,
  mDBFilename(0) 
{

  NS_INIT_REFCNT();
}

nsDBAccessor::~nsDBAccessor()
{
  Shutdown() ;
}

//
// Implement nsISupports methods
//
NS_IMPL_ISUPPORTS(nsDBAccessor, NS_GET_IID(nsIDBAccessor))

//
// Implement nsISupports methods
//
// NS_IMPL_ISUPPORTS(nsDBAccessor, NS_GET_IID(nsIDBAccessor))
/*
NS_IMETHODIMP
nsDBAccessor::QueryInterface(const nsIID &aIID, void **aResult)
{
  if(!aResult)
    return NS_ERROR_NULL_POINTER;

  if(aIID.Equals(NS_GET_IID(nsIDBAccessor)) ||
     aIID.Equals(NS_GET_IID(nsISupports))) {
    *aResult = NS_STATIC_CAST(nsIDBAccessor*, this);
    NS_ADDREF_THIS();
    return NS_OK ;
  } else {
    return NS_ERROR_NO_INTERFACE;
  }
}

// NS_IMPL_ADDREF(nsDBAccessor) ;
NS_IMETHODIMP
nsDBAccessor::AddRef()
{
  mRefCnt++ ;
  printf(" ref counted %d times\n", mRefCnt) ;
}

//NS_IMPL_RELEASE(nsDBAccessor) ;
NS_IMETHODIMP
nsDBAccessor::Release()
{
  mRefCnt-- ;
  printf(" release\n") ;
  if(mRefCnt == 0) 
    delete this ;
}
*/


///////////////////////////////////////////////////////////
// nsIDBAccessor methods 

NS_IMETHODIMP
nsDBAccessor::Init(nsIFileSpec* dbfile)
{
  m_Lock = PR_NewLock() ;

  // this should cover all platforms. 
  dbfile->GetNativePath(&mDBFilename) ;

  HASHINFO hash_info = {
    16*1024 , /* bucket size */
    0 ,       /* fill factor */
    0 ,       /* number of elements */
    0 ,       /* bytes to cache */
    0 ,       /* hash function */
    0} ;      /* byte order */

  nsAutoLock lock(m_Lock) ;

  mDB = dbopen(mDBFilename,
               O_RDWR | O_CREAT ,
               0600 ,
               DB_HASH ,
               & hash_info) ;

  NS_ASSERTION(mDB, "no database") ;

  return NS_OK ;
}

NS_IMETHODIMP
nsDBAccessor::Shutdown(void)
{
  if(mDB) {
    (*mDB->sync)(mDB, 0) ;
    (*mDB->close)(mDB) ;
    mDB = nsnull ;
  }

  if(mDBFilename) nsCRT::free(mDBFilename) ;

  PR_DestroyLock(m_Lock);
  return NS_OK ;
}
  
NS_IMETHODIMP
nsDBAccessor::Get(PRInt32 aID, void** anEntry, PRUint32 *aLength) 
{
  if(!anEntry)
    return NS_ERROR_NULL_POINTER ;

  *anEntry = nsnull ;
  *aLength = 0 ;

  NS_ASSERTION(mDB, "no database") ;

  // Lock the db
  nsAutoLock lock(m_Lock) ;
  DBT db_key, db_data ;

  db_key.data = NS_REINTERPRET_CAST(void*, &aID) ;
  db_key.size = sizeof(PRInt32) ;
  
  int status = 0 ;
  status = (*mDB->get)(mDB, &db_key, &db_data, 0) ;

  if(status == 0) {
    *anEntry = db_data.data ;
    *aLength = db_data.size ;
    return NS_OK ;
  } 
  else if(status == 1)
    return NS_OK ;
  else
    return NS_ERROR_FAILURE ;

}

NS_IMETHODIMP
nsDBAccessor::Put(PRInt32 aID, void* anEntry, PRUint32 aLength)
{
  NS_ASSERTION(mDB, "no database") ;

  // Lock the db
  nsAutoLock lock(m_Lock) ;
  DBT db_key, db_data ;

  db_key.data = NS_REINTERPRET_CAST(void*, &aID) ;
  db_key.size = sizeof(PRInt32) ;

  db_data.data = anEntry ;
  db_data.size = aLength ;

  if(0 == (*mDB->put)(mDB, &db_key, &db_data, 0)) {
    (*mDB->sync)(mDB, 0) ;
    return NS_OK ;
  }
  else {
    NS_ERROR("ERROR: Failed to put anEntry into db.\n") ;
    return NS_ERROR_FAILURE ;
  }
}

NS_IMETHODIMP
nsDBAccessor::Del(PRInt32 aID)
{
  NS_ASSERTION(mDB, "no database") ;

  // Lock the db
  nsAutoLock lock(m_Lock) ;
  DBT db_key ;


  db_key.data = NS_REINTERPRET_CAST(void*, &aID) ;
  db_key.size = sizeof(PRInt32) ;

  PRInt32 status = -1 ;
  status = (*mDB->del)(mDB, &db_key, 0) ;
//  NS_ASSERTION(status == 0, " nsDBAccessor::Del() is wrong, bad key? \n") ;
  if(1==status) 
    printf(" key not in db\n") ;

  if(-1 == status)
    printf(" delete error\n") ;

  if(0 <= status) {
    (*mDB->sync)(mDB, 0) ;
    return NS_OK ;
  }
  else 
    return NS_ERROR_FAILURE ;
}

/* the hash function is taken from PL_HashString, I modified it so
 * it takes length as parameter
 */
NS_IMETHODIMP
nsDBAccessor::GenID(const char* key, PRUint32 length, PRInt32* aID) 
{
//  *aID = (PRInt32) PL_HashString((const void*) key);
  PRInt32 id = 0 ;
  const PRUint8 *s = (const PRUint8*)key ;

  for (PRUint32 i = 0 ; i < length ; i++) {
    id = (id >> 28) ^ (id << 4) ^ *s ;
    s++ ;
  }

  *aID = id ;

  return NS_OK ;
}

NS_IMETHODIMP
nsDBAccessor::EnumEntry(void** anEntry, PRUint32* aLength, PRBool bReset) 
{
  if(!anEntry)
    return NS_ERROR_NULL_POINTER ;

  *anEntry = nsnull ;
  *aLength = 0 ;

  NS_ASSERTION(mDB, "no database") ;

  PRUint32 flag ;

  if(bReset) 
    flag = R_FIRST ;
  else
    flag = R_NEXT ;

  // Lock the db
  nsAutoLock lock(m_Lock) ;
  DBT db_key, db_data ;

  db_key.data = nsnull ;
  db_key.size = 0 ;

  db_data.data = nsnull ;
  db_data.size = 0 ;

  int status = (*mDB->seq)(mDB, &db_key, &db_data, flag) ;

//  if(0 == (*mDB->seq)(mDB, &db_key, &db_data, flag)) {
  if (0 == status) {
    *anEntry = db_data.data ;
    *aLength = db_data.size ;
    return NS_OK ;
  }
  else 
    return NS_ERROR_FAILURE ;
}
