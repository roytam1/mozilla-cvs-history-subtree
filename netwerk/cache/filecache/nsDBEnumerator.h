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

#ifndef _NS_DBENUMERATOR_H_
#define _NS_DBENUMERATOR_H_

#include "nsISimpleEnumerator.h"
#include "nsINetDataCacheRecord.h"
#include "nsIDBAccessor.h"
#include "nsCOMPtr.h"
#include "nsNetDiskCache.h"
#include "nsDiskCacheRecord.h"

class nsCachedDiskData ; /* forward decl */

class nsDBEnumerator : public nsISimpleEnumerator {
  public:
  NS_DECL_ISUPPORTS

  /* boolean HasMoreElements (); */
  NS_IMETHOD HasMoreElements(PRBool *_retval) ;

  /* nsISupports GetNext (); */
  NS_IMETHOD GetNext(nsISupports **_retval) ;

  nsDBEnumerator(nsIDBAccessor* aDB, nsNetDiskCache* aCache) ;
  virtual ~nsDBEnumerator() ;

  private:
  nsCOMPtr<nsIDBAccessor>                m_DB ;
  nsCOMPtr<nsNetDiskCache>                        m_DiskCache ;
  void *                                 tempEntry ;
  PRUint32                               tempEntry_length ;
  nsDiskCacheRecord*                    m_CacheEntry ;
  PRBool                                 bReset ;
};

#endif // _NS_DBENUMERATOR_H_
