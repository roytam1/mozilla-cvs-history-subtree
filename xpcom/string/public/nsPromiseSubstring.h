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

#ifndef nsPromiseSubstring_h___
#define nsPromiseSubstring_h___

#ifndef nsAString_h___
#include "nsAString.h"
#endif

#ifndef nsAPromiseString_h___
#include "nsAPromiseString.h"
#endif

#ifndef nsStringTraits_h___
#include "nsStringTraits.h"
#endif



  //
  // nsPromiseSubstring
  //

class nsPromiseSubstring
      : public nsAPromiseString
    /*
      NOT FOR USE BY HUMANS (mostly)

      ...not unlike |nsPromiseConcatenation|.  Instances of this class exist only as anonymous
      temporary results from |Substring()|.  Like |nsPromiseConcatenation|, this class only
      holds a pointer, no string data of its own.  It does its magic by overriding and forwarding
      calls to |GetReadableFragment()|.
    */
  {
    typedef nsAString                   string_type;
    typedef string_type::const_iterator const_iterator;

    protected:
      virtual const PRUnichar* GetReadableFragment( nsReadableFragment<PRUnichar>&, nsFragmentRequest, PRUint32 ) const;
      virtual       PRUnichar* GetWritableFragment( nsWritableFragment<PRUnichar>&, nsFragmentRequest, PRUint32 ) { return 0; }

    public:
      nsPromiseSubstring( const string_type& aString, PRUint32 aStartPos, PRUint32 aLength )
          : mString(aString),
            mStartPos( NS_MIN(aStartPos, aString.Length()) ),
            mLength( NS_MIN(aLength, aString.Length()-mStartPos) )
        {
          // nothing else to do here
        }

      nsPromiseSubstring( const const_iterator& aStart, const const_iterator& aEnd )
          : mString(aStart.string())
        {
          const_iterator zeroPoint;
          mString.BeginReading(zeroPoint);
          mStartPos = Distance(zeroPoint, aStart);
          mLength = Distance(aStart, aEnd);
        }

      // nsPromiseSubstring( const nsPromiseSubstring& ); // auto-generated copy-constructor should be OK
      // ~nsPromiseSubstring();                           // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsPromiseSubstring& );        // we're immutable, you can't assign into a substring

    public:
      virtual PRUint32 Length() const;
      virtual PRBool Promises( const string_type& aString ) const { return mString.Promises(aString); }

    private:
      const string_type&  mString;
      PRUint32            mStartPos;
      PRUint32            mLength;
  };

class nsPromiseCSubstring
      : public nsAPromiseCString
    /*
      NOT FOR USE BY HUMANS (mostly)

      ...not unlike |nsPromiseConcatenation|.  Instances of this class exist only as anonymous
      temporary results from |Substring()|.  Like |nsPromiseConcatenation|, this class only
      holds a pointer, no string data of its own.  It does its magic by overriding and forwarding
      calls to |GetReadableFragment()|.
    */
  {
    typedef nsACString                  string_type;
    typedef string_type::const_iterator const_iterator;

    protected:
      virtual const char* GetReadableFragment( nsReadableFragment<char>&, nsFragmentRequest, PRUint32 ) const;
      virtual       char* GetWritableFragment( nsWritableFragment<char>&, nsFragmentRequest, PRUint32 ) { return 0; }

    public:
      nsPromiseCSubstring( const string_type& aString, PRUint32 aStartPos, PRUint32 aLength )
          : mString(aString),
            mStartPos( NS_MIN(aStartPos, aString.Length()) ),
            mLength( NS_MIN(aLength, aString.Length()-mStartPos) )
        {
          // nothing else to do here
        }

      nsPromiseCSubstring( const const_iterator& aStart, const const_iterator& aEnd )
          : mString(aStart.string())
        {
          const_iterator zeroPoint;
          mString.BeginReading(zeroPoint);
          mStartPos = Distance(zeroPoint, aStart);
          mLength = Distance(aStart, aEnd);
        }

      // nsPromiseCSubstring( const nsPromiseCSubstring& ); // auto-generated copy-constructor should be OK
      // ~nsPromiseCSubstring();                            // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsPromiseCSubstring& );         // we're immutable, you can't assign into a substring

    public:
      virtual PRUint32 Length() const;
      virtual PRBool Promises( const string_type& aString ) const { return mString.Promises(aString); }

    private:
      const string_type&  mString;
      PRUint32            mStartPos;
      PRUint32            mLength;
  };




PRUint32
nsPromiseSubstring::Length() const
  {
    return mLength;
  }

const PRUnichar*
nsPromiseSubstring::GetReadableFragment( nsReadableFragment<PRUnichar>& aFragment, nsFragmentRequest aRequest, PRUint32 aPosition ) const
  {
      // Offset any request for a specific position (First, Last, At) by our
      //  substrings startpos within the owning string

    if ( aRequest == kFirstFragment )
      {
        aPosition = mStartPos;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kLastFragment )
      {
        aPosition = mStartPos + mLength;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kFragmentAt )
      aPosition += mStartPos;

    // requests for |kNextFragment| or |kPrevFragment| are just relayed down into the string we're slicing

    const PRUnichar* position_ptr = mString.GetReadableFragment(aFragment, aRequest, aPosition);

    // If |GetReadableFragment| returns |0|, then we are off the string, the contents of the
    //  fragment are garbage.

      // Therefore, only need to fix up the fragment boundaries when |position_ptr| is not null
    if ( position_ptr )
      {
          // if there's more physical data in the returned fragment than I logically have left...
        size_t logical_size_backward = aPosition - mStartPos;
        if ( size_t(position_ptr - aFragment.mStart) > logical_size_backward )
          aFragment.mStart = position_ptr - logical_size_backward;

        size_t logical_size_forward = mLength - logical_size_backward;
        if ( size_t(aFragment.mEnd - position_ptr) > logical_size_forward )
          aFragment.mEnd = position_ptr + logical_size_forward;
      }

    return position_ptr;
  }




PRUint32
nsPromiseCSubstring::Length() const
  {
    return mLength;
  }

const char*
nsPromiseCSubstring::GetReadableFragment( nsReadableFragment<char>& aFragment, nsFragmentRequest aRequest, PRUint32 aPosition ) const
  {
      // Offset any request for a specific position (First, Last, At) by our
      //  substrings startpos within the owning string

    if ( aRequest == kFirstFragment )
      {
        aPosition = mStartPos;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kLastFragment )
      {
        aPosition = mStartPos + mLength;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kFragmentAt )
      aPosition += mStartPos;

    // requests for |kNextFragment| or |kPrevFragment| are just relayed down into the string we're slicing

    const char* position_ptr = mString.GetReadableFragment(aFragment, aRequest, aPosition);

    // If |GetReadableFragment| returns |0|, then we are off the string, the contents of the
    //  fragment are garbage.

      // Therefore, only need to fix up the fragment boundaries when |position_ptr| is not null
    if ( position_ptr )
      {
          // if there's more physical data in the returned fragment than I logically have left...
        size_t logical_size_backward = aPosition - mStartPos;
        if ( size_t(position_ptr - aFragment.mStart) > logical_size_backward )
          aFragment.mStart = position_ptr - logical_size_backward;

        size_t logical_size_forward = mLength - logical_size_backward;
        if ( size_t(aFragment.mEnd - position_ptr) > logical_size_forward )
          aFragment.mEnd = position_ptr + logical_size_forward;
      }

    return position_ptr;
  }



inline
const nsPromiseCSubstring
Substring( const nsACString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsPromiseCSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsPromiseSubstring
Substring( const nsAString& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsPromiseSubstring(aString, aStartPos, aSubstringLength);
  }

inline
const nsPromiseCSubstring
Substring( const nsReadingIterator<char>& aStart, const nsReadingIterator<char>& aEnd )
  {
    return nsPromiseCSubstring(aStart, aEnd);
  }

inline
const nsPromiseSubstring
Substring( const nsReadingIterator<PRUnichar>& aStart, const nsReadingIterator<PRUnichar>& aEnd )
  {
    return nsPromiseSubstring(aStart, aEnd);
  }


#endif /* !defined(nsPromiseSubstring_h___) */
