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

#ifndef __gen_nsNetDiskCache_h__
#define __gen_nsNetDiskCache_h__

#include "nsINetDataDiskCache.h"
#include "nsNetDiskCacheCID.h"
#include "nsCOMPtr.h"
#include "nsIPref.h"
#include "nsIDBAccessor.h"

class nsIURI; /* forward decl */
class nsICachedNetData; /* forward decl */
class nsISimpleEnumerator; /* forward decl */
class nsIFileSpec; /* forward decl */

/* starting interface:    nsNetDiskCache */

class nsNetDiskCache : public nsINetDataDiskCache {
  public: 

  NS_DECL_ISUPPORTS
  NS_DECL_NSINETDATACACHE
  NS_DECL_NSINETDATADISKCACHE

  nsNetDiskCache() ;
  virtual ~nsNetDiskCache() ;

  protected:

  NS_IMETHOD Init(void) ;
  NS_IMETHOD InitDB(void) ;
  NS_IMETHOD SearchRUList(PRInt32 id, nsINetDataCacheRecord * *_retval) ;
  NS_IMETHOD AddToRUList(nsINetDataCacheRecord* aRecord) ;
  NS_IMETHOD CountDirSize(PRUint32 * t_size, nsIFileSpec * dir) ;
  NS_IMETHOD CreateDir(nsIFileSpec* dir_spec) ;

  private:

  PRUnichar* 		                m_Description ;
  PRBool 			                m_Enabled ;
  PRBool 			                m_ReadOnly ;
  PRUint32 			                m_NumEntries ;
  nsCOMPtr<nsINetDataCache> 	    m_pNextCache ;
  nsCOMPtr<nsIFileSpec>   	        m_pDiskCacheFolder ;
  char*                             m_DBFileName ;
  nsCOMPtr<nsIFileSpec>             m_DBFile ;

  PRUint32                          m_MaxEntries ;
  PRInt32 			                m_Capacity ;
  // according to SCC@netscape, we don't use nsCOMPtr for services (for now) 
  nsIDBAccessor*                    m_DB ;
  PRUint32                          m_ListCount ;
  PRLock*                           m_Lock ;
};

#endif /* __gen_nsNetDiskCache_h__ */
