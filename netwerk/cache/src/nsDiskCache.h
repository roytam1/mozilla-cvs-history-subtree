/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is nsCacheDevice.h, released March 9, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Patrick Beard   <beard@netscape.com>
 *    Gordon Sheridan <gordon@netscape.com>
 */


#ifndef _nsDiskCache_h_
#define _nsDiskCache_h_

#include "prtypes.h"
#include "prnetdb.h"
#include "nsDebug.h"

/**
 *   Cache Location Format
 *
 *    0011 0000 0000 0000 0000 0000 0000 0000 : File Selector (0 = separate file)
 *    0000 0011 0000 0000 0000 0000 0000 0000 : number of extra contiguous blocks 1-4
 *    1100 1100 0000 0000 0000 0000 0000 0000 : reserved bits
 *    0000 0000 1111 1111 1111 1111 1111 1111 : block#  0-16777216 (2^24)
 *
 *    0000 0000 1111 1111 1111 1111 0000 0000 : eFileReservedMask
 *    0000 0000 0000 0000 0000 0000 1111 1111 : eFileGenerationMask
 *
 *  File Selector:
 *      0 = separate file on disk
 *      1 = 256 byte block file
 *      2 = 1k block file
 *      3 = 4k block file
 */
 
 
class nsDiskCacheRecord {
    enum {
        eLocationSelectorMask   = 0x30000000,
        eLocationSelectorOffset = 28,
        
        eExtraBlocksMask        = 0x03000000,
        eExtraBlocksOffset      = 24,
        
        eReservedMask           = 0xCC000000,
        
        eBlockNumberMask        = 0x00FFFFFF,

        eFileReservedMask       = 0x00FFFF00,
        eFileGenerationMask     = 0x000000FF
    };

public:
    nsDiskCacheRecord()
        :   mHashNumber(0), mEvictionRank(0), mLocation(0), mMetaLocation(0)
    {
    }
    
    PRUint32   HashNumber() const
    {
        return mHashNumber;
    }
    
    void       SetHashNumber(PRUint32 hashNumber)
    {
        mHashNumber = hashNumber;
    }

    PRUint32   EvictionRank() const
    {
        return mEvictionRank;
    }

    void       SetEvictionRank(PRUint32 rank)
    {
        mEvictionRank = rank;
    }

    PRUint32   LocationSelector() const
    {
        return (PRUint32)(mLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void       SetLocationSelector(PRUint32 selector)
    {
        mLocation &= ~eLocationSelectorMask; // clear location selector bits
        mLocation |= (selector & eLocationSelectorMask) << eLocationSelectorOffset;
    }

    PRUint32   BlockCount() const
    {
        return (PRUint32)((mLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    void       SetBlockCount(PRUint32 count)
    {
        NS_ASSERTION( (count>=1) && (count<=4),"invalid block count");
        count = --count;
        mLocation &= ~eExtraBlocksMask; // clear extra blocks bits
        mLocation |= (count & eExtraBlocksMask) << eExtraBlocksOffset;
    }

    PRUint32   BlockNumber() const
    {
        return (mLocation & eBlockNumberMask);
    }

    void       SetBlockNumber(PRUint32  blockNumber)
    {
        mLocation &= ~eBlockNumberMask;  // clear block number bits
        mLocation |= blockNumber & eBlockNumberMask;
    }

    PRUint16   FileGeneration() const
    {
        return (mLocation & eFileGenerationMask);
    }

    void       SetFileGeneration(PRUint16 generation)
    {
        mLocation &= ~eFileGenerationMask;  // clear file generation bits
        mLocation |= generation & eFileGenerationMask;
    }

    void        Swap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHashNumber   = ::PR_htonl(mHashNumber);
        mEvictionRank = ::PR_htonl(mEvictionRank);
        mLocation     = ::PR_htonl(mLocation);
        mMetaLocation = ::PR_htonl(mMetaLocation);
#endif
    }
    
    void        Unswap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHashNumber   = ::PR_ntohl(mHashNumber);
        mEvictionRank = ::PR_ntohl(mEvictionRank);
        mLocation     = ::PR_ntohl(mLocation);
        mMetaLocation = ::PR_ntohl(mMetaLocation);
#endif
    }

private:
    PRUint32    mHashNumber;
    PRUint32    mEvictionRank;
    PRUint32    mLocation;
    PRUint32    mMetaLocation;
};


/*


*/

#endif // _nsDiskCache_h_
