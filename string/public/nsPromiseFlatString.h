/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#ifndef nsPromiseFlatString_h___
#define nsPromiseFlatString_h___

#ifndef nsStringTraits_h___
#include "nsStringTraits.h"
#endif

#ifndef nsAPromiseString_h___
#include "nsAPromiseString.h"
#endif

//-------1---------2---------3---------4---------5---------6---------7---------8

#define kDefaultFlatStringSize 64

template <class CharT>
class basic_nsPromiseFlatString
    : public nsStringTraits<CharT>::abstract_flat_type
  {
    typedef typename nsStringTraits<CharT>::abstract_string_type  string_type;
    typedef string_type::const_iterator                           const_iterator;

    public:
      explicit basic_nsPromiseFlatString( const string_type& );

      virtual
     ~basic_nsPromiseFlatString( )
        {
          if (mOwnsBuffer)
            nsMemory::Free((void*)mBuffer);
        }

      virtual PRUint32  Length() const { return mLength; }

      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 = 0 ) const;
      virtual       CharT* GetWritableFragment( nsWritableFragment<CharT>&, nsFragmentRequest, PRUint32 = 0 ) { return 0; }

      virtual const CharT* get() const { return mBuffer; }
        // eliminate this function after we correctly implement |GetBufferHandle|

    protected:
      PRUint32      mLength;
      const CharT*  mBuffer;
      PRBool        mOwnsBuffer;
      CharT         mInlineBuffer[kDefaultFlatStringSize];
  };

template <class CharT>
basic_nsPromiseFlatString<CharT>::basic_nsPromiseFlatString( const string_type& aString )
    : mLength(aString.Length()),
      mOwnsBuffer(PR_FALSE)
  {
    typedef nsReadingIterator<CharT> iterator;

    iterator start;
    iterator end;
    
    aString.BeginReading(start);
    aString.EndReading(end);

    // First count the number of buffers
    PRInt32 buffer_count = 0;
    while ( start != end )
      {
        buffer_count++;
        start.advance(start.size_forward());
      }

    // Now figure out what we want to do with the string
    aString.BeginReading(start);
    // XXX Not guaranteed null-termination in the first case
    // If it's a single buffer, we just use the implementation's buffer
    if ( buffer_count == 1 ) 
      mBuffer = start.get();
    // If it's too big for our inline buffer, we allocate a new one
    else if ( mLength > kDefaultFlatStringSize-1 )
      {
        CharT* result = NS_STATIC_CAST(CharT*, nsMemory::Alloc((mLength+1) * sizeof(CharT)));
    		CharT* toBegin = result;
        *copy_string(start, end, toBegin) = CharT(0);

        mBuffer = result;
        mOwnsBuffer = PR_TRUE;
      }
    // Otherwise copy into our internal buffer
    else
      {
        mBuffer = mInlineBuffer;
        CharT* toBegin = &mInlineBuffer[0];
        copy_string( start, end, toBegin);
        mInlineBuffer[mLength] = 0;
      }
  }


template <class CharT>
const CharT* 
basic_nsPromiseFlatString<CharT>::GetReadableFragment( nsReadableFragment<CharT>& aFragment, 
						 nsFragmentRequest aRequest, 
						 PRUint32 aOffset ) const 
  {
    switch ( aRequest )
      {
        case kFirstFragment:
        case kLastFragment:
        case kFragmentAt:
          aFragment.mEnd = (aFragment.mStart = mBuffer) + mLength;
          return aFragment.mStart + aOffset;
        
        case kPrevFragment:
        case kNextFragment:
        default:
          return 0;
      }
  }

typedef basic_nsPromiseFlatString<PRUnichar>  nsPromiseFlatString;
typedef basic_nsPromiseFlatString<char>       nsPromiseFlatCString;

#endif /* !defined(nsPromiseFlatString_h___) */
