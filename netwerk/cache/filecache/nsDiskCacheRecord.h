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

#ifndef _NET_CACHEDDISKDATA_H_
#define _NET_CACHEDDISKDATA_H_

#include "nsINetDataCacheRecord.h"
#include "nsCOMPtr.h"
#include "nsIDBAccessor.h"
#include "prtypes.h"
#include "nsILoadGroup.h"
#include "nsIFileChannel.h" 
          // don't know why we need it, but nsCOMPtr would compliant otherwise

class nsDiskCacheRecord : public nsINetDataCacheRecord
{
  public:

  nsDiskCacheRecord(nsIDBAccessor* db) ;
  nsDiskCacheRecord(const char* key, PRUint32 length, nsIDBAccessor* db) ;
  virtual ~nsDiskCacheRecord() ;

  NS_DECL_ISUPPORTS
  NS_DECL_NSINETDATACACHERECORD

  NS_IMETHOD RetrieveInfo(void* aInfo, PRUint32 aInfoLength) ;

  protected: 
  nsresult GenInfo(void) ;

  private:

  char*                     mKey ;
  PRUint32                  mKeyLength ;
  PRInt32                   mRecordID ;
  char*                     mMetaData ;
  PRUint32                  mMetaDataLength ;
  nsCOMPtr<nsIFileSpec>     mFile ;
  // according to SCC@netscape, we don't use nsCOMPtr for services (for now) 
  nsIDBAccessor*            mDB ; 
  void*                     mInfo ;
  PRUint32                  mInfoSize ;

  nsCOMPtr<nsIChannel>      mFileTransport ;
} ;

#endif // _NET_CACHEDDISKDATA_H_
