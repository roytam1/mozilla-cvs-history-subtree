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

/**
 * nsMemCache is the implementation of an in-memory network-data
 * cache, used to cache the responses to network retrieval commands.
 * Each cache entry may contain both content, e.g. GIF image data, and
 * associated metadata, e.g. HTTP headers.  Each entry is indexed by
 * two different keys: a record id number and an opaque key, which is
 * created by the cache manager by combining the URI with a "secondary
 * key", e.g. HTTP post data.
 */

#ifndef _nsMemCache_h_
#define _nsMemCache_h_

#include "nsINetDataCache.h"

class nsHashtable;
class nsMemCacheRecord;

class nsMemCache : public nsINetDataCache
{
public:
    nsMemCache();
    ~nsMemCache();

    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsINetDataCache methods
    NS_DECL_NSINETDATACACHE

    // Factory
    static NS_METHOD nsMemCacheConstructor(nsISupports *aOuter, REFNSIID aIID,
                                           void **aResult);

protected:

    PRUint32         mNumEntries;
    PRUint32         mCapacity;    // Memory capacity, in bytes
    PRUint32         mOccupancy;   // Memory used, in bytes
    PRBool           mEnabled;     // If false, bypass mem cache

    nsINetDataCache* mNextCache;

    // Mapping from either opaque key or record ID to nsMemCacheRecord
    nsHashtable*     mHashTable;

    // Used to assign record ID's
    static PRInt32   gRecordSerialNumber;

    nsresult Init();
    NS_METHOD Delete(nsMemCacheRecord* aRecord);

    friend class nsMemCacheRecord;
};

#endif // _nsMemCache_h_
