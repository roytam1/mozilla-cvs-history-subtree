/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef DEBUG
#define ENABLE_STRING_STATS
#endif

#ifdef ENABLE_STRING_STATS
#include <stdio.h>
#endif

#include <stdlib.h>
#include "nsTStringBase.h"
#include "nsTString.h"
#include "nsTDependentString.h"
#include "nsMemory.h"
#include "pratom.h"

// ---------------------------------------------------------------------------

static PRUnichar gNullChar = 0;

const char*      nsCharTraits<char>     ::sEmptyBuffer = (const char*) &gNullChar;
const PRUnichar* nsCharTraits<PRUnichar>::sEmptyBuffer =               &gNullChar;

// ---------------------------------------------------------------------------

#ifdef ENABLE_STRING_STATS
class nsStringStats
  {
    public:
      nsStringStats()
        : mAllocCount(0), mReallocCount(0), mFreeCount(0), mShareCount(0) {}

      ~nsStringStats()
        {
          printf("nsStringStats\n");
          printf(" => mAllocCount: %d\n", mAllocCount);
          printf(" => mReallocCount: %d\n", mReallocCount);
          printf(" => mFreeCount: %d\n", mFreeCount);
          printf(" => mShareCount: %d\n", mShareCount);
          printf(" => mAdoptCount: %d\n", mAdoptCount);
          printf(" => mAdoptFreeCount: %d\n", mAdoptFreeCount);
        }

      PRInt32 mAllocCount;
      PRInt32 mReallocCount;
      PRInt32 mFreeCount;
      PRInt32 mShareCount;
      PRInt32 mAdoptCount;
      PRInt32 mAdoptFreeCount;
  };
static nsStringStats gStringStats;
#define STRING_STAT_INCREMENT(_s) PR_AtomicIncrement(&gStringStats.m ## _s ## Count)
#else
#define STRING_STAT_INCREMENT(_s)
#endif

// ---------------------------------------------------------------------------

  /**
   * This structure preceeds the string buffers "we" allocate.  It may be the
   * case that nsTStringBase::mData does not point to one of these special
   * buffers.  The mFlags member variable distinguishes the buffer type.
   *
   * When this header is in use, it enables reference counting, and capacity
   * tracking.  NOTE: A string buffer can be modified only if its reference
   * count is 1.
   */
class nsStringHeader
  {
    private:

      PRInt32  mRefCount;
      PRUint32 mStorageSize;

    public:

      void AddRef()
        {
          PR_AtomicIncrement(&mRefCount);
          STRING_STAT_INCREMENT(Share);
        }

      void Release()
        {
          if (PR_AtomicDecrement(&mRefCount) == 0)
            {
              STRING_STAT_INCREMENT(Free);
              free(this); // we were allocated with |malloc|
            }
        }

        /**
         * Alloc returns a pointer to a new string header with set capacity.
         */
      static nsStringHeader* Alloc(size_t size)
        {
          STRING_STAT_INCREMENT(Alloc);

          nsStringHeader *hdr =
              (nsStringHeader *) malloc(sizeof(nsStringHeader) + size);
          if (hdr)
            {
              hdr->mRefCount = 1;
              hdr->mStorageSize = size;
            }
          return hdr;
        }

      static nsStringHeader* Realloc(nsStringHeader* hdr, size_t size)
        {
          STRING_STAT_INCREMENT(Realloc);

          // no point in trying to save ourselves if we hit this assertion
          NS_ASSERTION(!hdr->IsReadonly(), "|Realloc| attempted on readonly string");

          hdr = (nsStringHeader*) realloc(hdr, sizeof(nsStringHeader) + size);
          if (hdr)
            hdr->mStorageSize = size;

          return hdr;
        }

      static nsStringHeader* FromData(void* data)
        {
          return (nsStringHeader*) ( ((char*) data) - sizeof(nsStringHeader) );
        }

      void* Data() const
        {
          return (void*) ( ((char*) this) + sizeof(nsStringHeader) );
        }

      PRUint32 StorageSize() const
        {
          return mStorageSize;
        }

        /**
         * Because nsTStringBase allows only single threaded access, if this
         * method returns FALSE, then the caller can be sure that it has
         * exclusive access to the nsStringHeader and associated data.
         * However, if this function returns TRUE, then there is no telling
         * how many other threads may be accessing this object simultaneously.
         */
      PRBool IsReadonly() const
        {
          return mRefCount > 1;
        }
  };

// ---------------------------------------------------------------------------

inline void
ReleaseData( void* data, PRUint32 flags )
  {
    if (flags & nsTStringBase<char>::F_SHARED)
      {
        nsStringHeader::FromData(data)->Release();
      }
    else if (flags & nsTStringBase<char>::F_OWNED)
      {
        nsMemory::Free(data);
        STRING_STAT_INCREMENT(AdoptFree);
      }
    // otherwise, nothing to do.
  }

// ---------------------------------------------------------------------------

  /**
   * this function is called to prepare mData for writing.  the given capacity
   * indicates the required minimum storage size for mData, in sizeof(char_type)
   * increments.  this function returns true if the operation succeeds.  it also
   * returns the old data and old flags members if mData is newly allocated.
   * the old data must be released by the caller.
   */
template <class CharT>
PRBool
nsTStringBase<CharT>::MutatePrep( size_type capacity, char_type** oldData, PRUint32* oldFlags )
  {
    // initialize to no old data
    *oldData = nsnull;
    *oldFlags = 0;

    size_type curCapacity = Capacity();

    if (capacity <= curCapacity)
      return PR_TRUE;

    if (curCapacity > 0)
      {
        // use doubling algorithm when forced to increase available capacity,
        // but always start out with exactly the requested amount.
        PRUint32 temp = curCapacity;
        while (temp < capacity)
          temp <<= 1;
        capacity = temp;
      }

    //
    // several cases:
    //
    //  (1) we have a shared buffer (mFlags & F_SHARED)
    //  (2) we have an owned buffer (mFlags & F_OWNED)
    //  (3) we have a fixed buffer (mFlags & F_FIXED)
    //  (4) we have a readonly buffer
    //
    // requiring that we in some cases preserve the data before creating
    // a new buffer complicates things just a bit ;-)
    //

    size_type storageSize = (capacity + 1) * sizeof(char_type);

    // case #1
    if (mFlags & F_SHARED)
      {
        nsStringHeader* hdr = nsStringHeader::FromData(mData);
        if (!hdr->IsReadonly())
          {
            hdr = nsStringHeader::Realloc(hdr, storageSize);
            if (hdr)
              {
                mData = (char_type*) hdr->Data();
                return PR_TRUE;
              }
            // out of memory!!  put us in a consistent state at least.
            mData = NS_CONST_CAST(char_type*, char_traits::sEmptyBuffer);
            mLength = 0;
            mFlags = F_TERMINATED;
            return PR_FALSE;
          }
      }

    // if we reach here then, we must allocate a new buffer.  we cannot make
    // use of our F_OWNED or F_FIXED buffers because they are not large enough
    // (based on the initial capacity check).  we need to allocate a new
    // buffer, and possibly copy over the old one, and then we can release the
    // old buffer, and set our member vars.

    nsStringHeader* newHdr = nsStringHeader::Alloc(storageSize);
    if (!newHdr)
      return PR_FALSE; // we are still in a consistent state

    char_type* newData = (char_type*) newHdr->Data();

    // save old data and flags
    *oldData = mData;
    *oldFlags = mFlags;

    mData = newData;
    mFlags = (F_TERMINATED | F_SHARED);

    // mLength does not change

    // though we are not necessarily terminated at the moment, now is probably
    // still the best time to set F_TERMINATED.

    return PR_TRUE;
  }

template <class CharT>
void
nsTStringBase<CharT>::ReleaseData()
  {
    ::ReleaseData(mData, mFlags);
    // mData, mLength, and mFlags are purposefully left dangling
  }

template <class CharT>
void
nsTStringBase<CharT>::ReplacePrep( index_type cutStart, size_type cutLen, size_type fragLen )
  {
    // bound cut length
    cutLen = NS_MIN(cutLen, cutStart + mLength);

    PRUint32 newLen = mLength - cutLen + fragLen;

    char_type* oldData;
    PRUint32 oldFlags;
    if (!MutatePrep(newLen, &oldData, &oldFlags))
      return;

    if (oldData)
      {
        // determine whether or not we need to copy part of the old string
        // over to the new string.

        if (cutStart > 0)
          {
            // copy prefix from old string
            char_traits::copy(mData, oldData, cutStart);
          }

        if (cutStart + cutLen < mLength)
          {
            // copy suffix from old string to new offset
            size_type from = cutStart + cutLen;
            size_type fromLen = mLength - from;
            PRUint32 to = cutStart + fragLen;
            char_traits::copy(mData + to, oldData + from, fromLen);
          }

        ::ReleaseData(oldData, oldFlags);
      }
    else
      {
        // original data remains intact

        // determine whether or not we need to move part of the existing string
        // to make room for the requested hole.
        if (fragLen != cutLen && cutStart + cutLen < mLength)
          {
            PRUint32 from = cutStart + cutLen;
            PRUint32 fromLen = mLength - from;
            PRUint32 to = cutStart + fragLen;
            char_traits::move(mData + to, mData + from, fromLen);
          }
      }

    // add null terminator
    mData[newLen] = char_type(0);
    mLength = newLen;
  }

template <class CharT>
typename
nsTStringBase<CharT>::size_type
nsTStringBase<CharT>::Capacity() const
  {
    size_type capacity;
    if (mFlags & F_SHARED)
      {
        // if the string is readonly, then we pretend that it has no capacity.
        nsStringHeader* hdr = nsStringHeader::FromData(mData);
        if (hdr->IsReadonly())
          capacity = 0;
        else
          capacity = (hdr->StorageSize() / sizeof(char_type)) - 1;
      }
    else if (mFlags & F_FIXED)
      {
        capacity = NS_STATIC_CAST(const nsTAutoString<char_type>*, this)->mFixedCapacity;
      }
    else if (mFlags & F_OWNED)
      {
        // we don't store the capacity of an adopted buffer because that would
        // require an additional member field.  the best we can do is base the
        // capacity on our length.  remains to be seen if this is the right
        // trade-off.
        capacity = mLength;
      }
    else
      {
        capacity = 0;
      }
    return capacity;
  }

#if 0
template <class CharT>
PRBool
nsTStringBase<CharT>::EnsureCapacity( size_type capacity, PRBool preserveData )
  {
    size_type curCapacity = Capacity();

    if (capacity <= curCapacity)
      return PR_TRUE;

    if (curCapacity > 0)
      {
        // use doubling algorithm when forced to increase available capacity,
        // but always start out with exactly the requested amount.
        PRUint32 temp = curCapacity;
        while (temp < capacity)
          temp <<= 1;
        capacity = temp;
      }

    //
    // several cases:
    //
    //  (1) we have a shared buffer (mFlags & F_SHARED)
    //  (2) we have an owned buffer (mFlags & F_OWNED)
    //  (3) we have a fixed buffer (mFlags & F_FIXED)
    //  (4) we have a readonly buffer
    //
    // requiring that we in some cases preserve the data before creating
    // a new buffer complicates things just a bit ;-)
    //

    // case #1
    if (mFlags & F_SHARED)
      {
        nsStringHeader* hdr = nsStringHeader::FromData(mData);
        if (!hdr->IsReadonly())
          {
            hdr = nsStringHeader::Realloc(hdr, capacity * sizeof(char_type));
            if (hdr)
              {
                mData = (char_type*) hdr->Data();
                return PR_TRUE;
              }
            // out of memory!!  put us in a consistent state at least.
            mData = NS_CONST_CAST(char_type*, char_traits::sEmptyBuffer);
            mLength = 0;
            mFlags = F_TERMINATED;
            return PR_FALSE;
          }
      }

    // if we reach here then, we must allocate a new buffer.  we cannot make
    // use of our F_OWNED or F_FIXED buffers because they are not large enough
    // (based on the initial capacity check).  we need to allocate a new
    // buffer, and possibly copy over the old one, and then we can release the
    // old buffer, and set our member vars.

    nsStringHeader* newHdr = nsStringHeader::Alloc(capacity * sizeof(char_type));
    if (!newHdr)
      return PR_FALSE; // we are still in a consistent state

    char_type* newData = (char_type*) newHdr->Data();

    if (preserveData && mLength > 0)
        char_traits::copy(newData, mData, NS_MIN(mLength, capacity));

    ::ReleaseData(mData, mFlags);

    mData = newData;
    mFlags = (F_TERMINATED | F_SHARED);

    // mLength does not change

    // though we are not necessarily terminated at the moment, now is probably
    // still the best time to set F_TERMINATED.

    return PR_TRUE;
  }
#endif

template <class CharT>
void
nsTStringBase<CharT>::EnsureMutable()
  {
    if (mFlags & F_SHARED)
      {
        nsStringHeader* hdr = nsStringHeader::FromData(mData);
        if (hdr->IsReadonly())
          {
            // take advantage of sharing here...
            Assign(string_type(mData, mLength));
          }
      }
  }

// ---------------------------------------------------------------------------

template <class CharT>
void
nsTStringBase<CharT>::Assign( const char_type* data, size_type length )
  {
      // unfortunately, people do pass null sometimes :(
    if (!data)
      {
        Truncate();
        return;
      }

    if (length == size_type(-1))
      length = char_traits::length(data);

    if (IsDependentOn(data, data + length))
      {
        // take advantage of sharing here...
        Assign(string_type(data, length));
        return;
      }

    ReplacePrep(0, mLength, length);
    char_traits::copy(mData, data, length);
  }

template <class CharT>
void
nsTStringBase<CharT>::Assign( const self_type& str )
  {
    // |str| could be sharable.  we need to check its flags to know how to
    // deal with it.

    if (&str == this)
      return;

    if (str.mFlags & F_SHARED)
      {
        // nice! we can avoid a string copy :-)

        // |str| should be null-terminated
        NS_ASSERTION(str.mFlags & F_TERMINATED, "shared, but not terminated");

        ::ReleaseData(mData, mFlags);

        mData = str.mData;
        mLength = str.mLength;
        mFlags = (F_TERMINATED | F_SHARED);

        // get an owning reference to the mData
        nsStringHeader::FromData(mData)->AddRef();
      }
    else
      {
        // else, treat this like an ordinary assignment.
        Assign(str.Data(), str.Length());
      }
  }

template <class CharT>
void
nsTStringBase<CharT>::Assign( const string_tuple_type& tuple )
  {
    if (tuple.IsDependentOn(mData, mData + mLength))
      {
        // take advantage of sharing here...
        Assign(string_type(tuple));
        return;
      }

    size_type length = tuple.Length();

    ReplacePrep(0, mLength, length);
    if (length)
      tuple.WriteTo(mData, length);
  }

  // this is non-inline to reduce codesize at the callsite
template <class CharT>
void
nsTStringBase<CharT>::Assign( const abstract_string_type& readable )
  {
      // promote to string if possible to take advantage of sharing
    if (readable.mVTable == nsTObsoleteAString<char_type>::sCanonicalVTable)
      Assign(*readable.AsString());
    else
      Assign(readable.ToString());
  }


template <class CharT>
void
nsTStringBase<CharT>::Adopt( char_type* data, size_type length )
  {
    ::ReleaseData(mData, mFlags);
    mData = data;
    if (mData)
      {
        if (length == size_type(-1))
          length = char_traits::length(data);
        mLength = length;
        mFlags = (F_TERMINATED | F_OWNED);

        STRING_STAT_INCREMENT(Adopt);
      }
    else
      {
        mLength = 0;
        mFlags = 0;
      }
  }


template <class CharT>
void
nsTStringBase<CharT>::Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length )
  {
    if (length == size_type(-1))
      length = char_traits::length(data);

    if (IsDependentOn(data, data + length))
      {
        nsTAutoString<char_type> temp(data, length);
        Replace(cutStart, cutLength, temp);
        return;
      }

    ReplacePrep(cutStart, cutLength, length);

    if (length > 0)
      char_traits::copy(mData + cutStart, data, length);
  }

template <class CharT>
void
nsTStringBase<CharT>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple )
  {
    if (tuple.IsDependentOn(mData, mData + mLength))
      {
        nsTAutoString<char_type> temp(tuple);
        Replace(cutStart, cutLength, temp);
        return;
      }

    size_type length = tuple.Length();

    ReplacePrep(cutStart, cutLength, length);

    if (length > 0)
      tuple.WriteTo(mData + cutStart, length);
  }

template <class CharT>
void
nsTStringBase<CharT>::Replace( index_type cutStart, size_type cutLength, const abstract_string_type& readable )
  {
    Replace(cutStart, cutLength, readable.ToString());
  }

template <class CharT>
void
nsTStringBase<CharT>::SetCapacity( size_type capacity )
  {
    // capacity does not include room for the terminating null char

    char_type* oldData;
    PRUint32 oldFlags;
    if (!MutatePrep(capacity, &oldData, &oldFlags))
      return;

    if (oldData)
      {
        NS_ASSERTION(capacity > 0, "MutatePrep should have returned null oldData");

        // compute new string length
        size_type newLen = NS_MIN(mLength, capacity);

        // preserve old data
        if (mLength > 0)
          char_traits::copy(mData, oldData, newLen);

        // adjust mLength if our buffer shrunk down in size
        if (newLen < mLength)
          {
            mData[newLen] = char_type(0);
            mLength = newLen;
          }

        ::ReleaseData(oldData, oldFlags);
      }
  }

template <class CharT>
void
nsTStringBase<CharT>::SetLength( size_type length )
  {
    if (length == 0)
      {
        ::ReleaseData(mData, mFlags);
        mData = NS_CONST_CAST(char_type*, char_traits::sEmptyBuffer);
        mLength = 0;
        mFlags = F_TERMINATED;
      }
    else
      {
        // may grow buffer here...
        SetCapacity(length);
        mData[length] = char_type(0);
        mLength = length;
      }
  }

template <class CharT>
void
nsTStringBase<CharT>::SetIsVoid( PRBool val )
  {
    if (val)
      {
        Truncate();
        mFlags |= F_VOIDED;
      }
    else
      {
        mFlags &= ~F_VOIDED;
      }
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const self_type& str ) const
  {
    return mLength == str.mLength && char_traits::compare(mData, str.mData, mLength) == 0;
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const self_type& str, const comparator_type& comp ) const
  {
    return mLength == str.mLength && comp(mData, str.mData, mLength) == 0;
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const abstract_string_type& readable ) const
  {
    const char_type* data;
    size_type length = readable.GetReadableBuffer(&data);

    return mLength == length && char_traits::compare(mData, data, mLength) == 0;
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const abstract_string_type& readable, const comparator_type& comp ) const
  {
    const char_type* data;
    size_type length = readable.GetReadableBuffer(&data);

    return mLength == length && comp(mData, data, mLength) == 0;
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const char_type* data ) const
  {
    return Equals(nsTDependentString<char_type>(data));
  }

template <class CharT>
PRBool
nsTStringBase<CharT>::Equals( const char_type* data, const comparator_type& comp ) const
  {
    return Equals(nsTDependentString<char_type>(data), comp);
  }

template <class CharT>
typename
nsTStringBase<CharT>::size_type
nsTStringBase<CharT>::CountChar( char_type c ) const
  {
    const char_type *start = mData;
    const char_type *end   = mData + mLength;

    return NS_COUNT(start, end, c);
  }

template <class CharT>
PRInt32
nsTStringBase<CharT>::FindChar( char_type c, index_type offset ) const
  {
    if (offset < mLength)
      {
        const char_type* result = char_traits::find(mData + offset, mLength - offset, c);
        if (result)
          return result - mData;
      }
    return -1;
  }


  /**
   * explicit template instantiation
   */

template void     nsTStringBase<char>::ReleaseData();
template PRBool   nsTStringBase<char>::MutatePrep( size_type capacity, char_type** oldData, PRUint32* oldFlags );
template void     nsTStringBase<char>::ReplacePrep( index_type cutStart, size_type cutLen, size_type fragLen );
template PRUint32 nsTStringBase<char>::Capacity() const;
template void     nsTStringBase<char>::EnsureMutable();
template void     nsTStringBase<char>::Assign( const char_type* data, size_type length );
template void     nsTStringBase<char>::Assign( const self_type& str );
template void     nsTStringBase<char>::Assign( const string_tuple_type& tuple );
template void     nsTStringBase<char>::Assign( const abstract_string_type& readable );
template void     nsTStringBase<char>::Adopt( char_type* data, size_type length );
template void     nsTStringBase<char>::Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length );
template void     nsTStringBase<char>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple );
template void     nsTStringBase<char>::Replace( index_type cutStart, size_type cutLength, const abstract_string_type& readable );
template void     nsTStringBase<char>::SetLength( size_type length );
template void     nsTStringBase<char>::SetIsVoid( PRBool val );
template PRBool   nsTStringBase<char>::Equals( const self_type& ) const;
template PRBool   nsTStringBase<char>::Equals( const self_type&, const comparator_type& ) const;
template PRBool   nsTStringBase<char>::Equals( const abstract_string_type& ) const;
template PRBool   nsTStringBase<char>::Equals( const abstract_string_type&, const comparator_type& ) const;
template PRBool   nsTStringBase<char>::Equals( const char_type* ) const;
template PRBool   nsTStringBase<char>::Equals( const char_type*, const comparator_type& ) const;
template PRUint32 nsTStringBase<char>::CountChar( char_type ) const;
template PRInt32  nsTStringBase<char>::FindChar( char_type, index_type ) const;

template void     nsTStringBase<PRUnichar>::ReleaseData();
template PRBool   nsTStringBase<PRUnichar>::MutatePrep( size_type capacity, char_type** oldData, PRUint32* oldFlags );
template void     nsTStringBase<PRUnichar>::ReplacePrep( index_type cutStart, size_type cutLen, size_type fragLen );
template PRUint32 nsTStringBase<PRUnichar>::Capacity() const;
template void     nsTStringBase<PRUnichar>::EnsureMutable();
template void     nsTStringBase<PRUnichar>::Assign( const char_type* data, size_type length );
template void     nsTStringBase<PRUnichar>::Assign( const self_type& str );
template void     nsTStringBase<PRUnichar>::Assign( const string_tuple_type& tuple );
template void     nsTStringBase<PRUnichar>::Assign( const abstract_string_type& readable );
template void     nsTStringBase<PRUnichar>::Adopt( char_type* data, size_type length );
template void     nsTStringBase<PRUnichar>::Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length );
template void     nsTStringBase<PRUnichar>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple );
template void     nsTStringBase<PRUnichar>::Replace( index_type cutStart, size_type cutLength, const abstract_string_type& readable );
template void     nsTStringBase<PRUnichar>::SetLength( size_type length );
template void     nsTStringBase<PRUnichar>::SetIsVoid( PRBool val );
template PRBool   nsTStringBase<PRUnichar>::Equals( const self_type& ) const;
template PRBool   nsTStringBase<PRUnichar>::Equals( const self_type&, const comparator_type& ) const;
template PRBool   nsTStringBase<PRUnichar>::Equals( const abstract_string_type& ) const;
template PRBool   nsTStringBase<PRUnichar>::Equals( const abstract_string_type&, const comparator_type& ) const;
template PRBool   nsTStringBase<PRUnichar>::Equals( const char_type* ) const;
template PRBool   nsTStringBase<PRUnichar>::Equals( const char_type*, const comparator_type& ) const;
template PRUint32 nsTStringBase<PRUnichar>::CountChar( char_type ) const;
template PRInt32  nsTStringBase<PRUnichar>::FindChar( char_type, index_type ) const;
