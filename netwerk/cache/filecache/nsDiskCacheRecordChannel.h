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

#ifndef _ns_DiskCacheRecordChannel_h_
#define _ns_DiskCacheRecordChannel_h_

#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsDiskCacheRecord.h"

/*
 * This class is plagiarized from nsMemCacheChannel
 */

class nsDiskCacheRecordChannel : public nsIChannel
{
  public:

  nsDiskCacheRecordChannel(nsDiskCacheRecord *aRecord, nsILoadGroup *aLoadGroup);
  virtual ~nsDiskCacheRecordChannel() ;

  // Declare nsISupports methods
  NS_DECL_ISUPPORTS

  // Declare nsIRequest methods
  NS_DECL_NSIREQUEST

  // Declare nsIChannel methods
  NS_DECL_NSICHANNEL

  nsresult Init(void) ;

  private:

  nsresult NotifyStorageInUse(PRInt32 aBytesUsed) ;

  nsDiskCacheRecord*                    mRecord ;
  nsCOMPtr<nsILoadGroup>                mLoadGroup ;
  nsCOMPtr<nsISupports>                 mOwner ;
  nsCOMPtr<nsIChannel>                  mFileTransport ;

  friend class WriteStreamWrapper ;
} ;

#endif // _ns_DiskCacheRecordChannel_h_

