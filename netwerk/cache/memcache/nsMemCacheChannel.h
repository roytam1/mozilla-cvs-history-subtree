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

#ifndef _nsMemCacheChannel_h_
#define _nsMemCacheChannel_h_

#include "nsMemCacheRecord.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsCOMPtr.h"


class nsMemCacheChannel : public nsIChannel
{
public:
    // Constructors and Destructor
    nsMemCacheChannel(nsMemCacheRecord *aRecord, nsILoadGroup *aLoadGroup);
    virtual ~nsMemCacheChannel();

    // Declare nsISupports methods
    NS_DECL_ISUPPORTS

    // Declare nsIRequest methods
    NS_DECL_NSIREQUEST

    // Declare nsIChannel methods
    NS_DECL_NSICHANNEL

protected:

    nsCOMPtr<nsMemCacheRecord>   mRecord;
    nsCOMPtr<nsIInputStream>     mInputStream;
    nsCOMPtr<nsIStreamListener>  mStreamListener;
    nsCOMPtr<nsILoadGroup>       mLoadGroup;
    nsCOMPtr<nsISupports>        mOwner;
};

#endif // _nsMemCacheChannel_h_
