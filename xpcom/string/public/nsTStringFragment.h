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

#ifndef nsTStringFragment_h___
#define nsTStringFragment_h___

#ifndef nsStringFwd_h___
#include "nsStringFwd.h"
#endif

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
#endif

#ifndef nsTStringIterator_h___
#include "nsTStringIterator.h"
#endif

#ifndef nsTAString_h___
#include "nsTAString.h"
#endif

  /**
   * nsTStringFragment
   *
   * Represents a readonly fragment of a string (not necessarily null-
   * terminated).
   */
template <class CharT>
class NS_COM nsTStringFragment : public nsTAString<CharT>
  {
    public:
      typedef CharT                             char_type;
      typedef nsCharTraits<char_type>           char_traits;

      typedef nsTStringFragment<char_type>      self_type;
      typedef nsTStringFragment<char_type>      string_fragment_type;
      typedef nsTAString<char_type>             abstract_string_type;

      typedef nsReadingIterator<char_type>      const_iterator;
      typedef const char_type*                  const_char_iterator;

      typedef nsTStringComparator<char_type>    comparator_type;

      typedef PRUint32                          size_type;
      typedef PRUint32                          index_type;

    public:

      // automatic conversion from nsTStringFragment to nsTAString OK
      /*
      operator const abstract_string_type&() const
        {
          return *NS_REINTERPRET_CAST(const abstract_string_type*, this);
        }
      */

      const_iterator& BeginReading( const_iterator& aResult ) const
        {
          aResult.mPosition = mData;
          return aResult;
        }

      const_iterator& EndReading( const_iterator& aResult ) const
        {
          aResult.mPosition = mData + mLength;
          return aResult;
        }

      const_char_iterator& BeginReading( const_char_iterator& aResult ) const
        {
          return aResult = mData;
        }

      const_char_iterator& EndReading( const_char_iterator& aResult ) const
        {
          return aResult = mData + mLength;
        }

      // returns pointer to string data (not necessarily null-terminated)
      const char_type *Data() const
        {
          return mData;
        }

      size_type Length() const
        {
          return mLength;
        }

      PRBool IsEmpty() const
        {
          return mLength == 0;
        }

      char_type CharAt( PRUint32 i ) const
        {
          NS_ASSERTION(i < mLength, "index exceeds allowable range");
          return mData[i];
        }

      char_type operator[]( PRUint32 i ) const
        {
          return CharAt(i);
        }

      PRBool Equals( const self_type& ) const;
      PRBool Equals( const self_type&, const comparator_type& ) const;

      PRBool Equals( const abstract_string_type& readable ) const
        {
          return Equals(self_type(readable));
        }

      PRBool Equals( const abstract_string_type& readable, const comparator_type& comp ) const
        {
          return Equals(self_type(readable), comp);
        }

      PRBool Equals( const char_type* data ) const
        {
          return Equals(self_type(data));
        }

      PRBool Equals( const char_type* data, const comparator_type& comp ) const
        {
          return Equals(self_type(data), comp);
        }

      PRBool IsVoid() const
        {
          return mFlags & F_VOIDED;
        }

      PRBool IsTerminated() const
        {
          return mFlags & F_TERMINATED;
        }

      char_type First() const
        {
          NS_ASSERTION(mLength > 0, "|First()| called on an empty string fragment");
          return mData[0];
        }

      inline
      char_type Last() const
        {
          NS_ASSERTION(mLength > 0, "|Last()| called on an empty string fragment");
          return mData[mLength - 1];
        }

      size_type CountChar( char_type ) const;
      PRInt32 FindChar( char_type, index_type offset = 0 ) const;


      // ------- begin{ obsolete api support } -------

      // XXX other searching methods
      // PRInt32 FindChar(PRUnichar aChar, PRInt32 anOffset=0, PRInt32 aCount=-1) const;
      PRInt32 Find(const nsCString& aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=0,PRInt32 aCount=-1) const;
      // PRInt32 Find(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=0,PRInt32 aCount=-1) const;
      // PRInt32 Find(const nsAFlatString& aString, PRInt32 anOffset=0, PRInt32 aCount=-1) const; // XXX nsString only
      // PRInt32 Find(const PRUnichar* aString, PRInt32 anOffset=0, PRInt32 aCount=-1) const; // XXX nsString only
      // PRInt32 FindCharInSet(const char* aString,PRInt32 anOffset=0) const;
      // PRInt32 FindCharInSet(const PRUnichar* aString,PRInt32 anOffset=0) const; // XXX nsString only
      // PRInt32 FindCharInSet(const nsCString& aString,PRInt32 anOffset=0) const; // XXX nsCString only
      // PRInt32 RFind(const char* aCString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aCount=-1) const;
      // PRInt32 RFind(const nsAFlatString& aString, PRInt32 anOffset=-1,PRInt32 aCount=-1) const; // XXX nsString only
      // PRInt32 RFind(const nsCString& aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 anOffset=-1,PRInt32 aCount=-1) const; // XXX nsCString only
      // PRInt32 RFind(const PRUnichar* aString,PRInt32 anOffset=-1,PRInt32 aCount=-1) const; // XXX nsString only
      // PRInt32 RFindChar(PRUnichar aChar,PRInt32 anOffset=-1,PRInt32 aCount=-1) const;
      PRInt32 RFindCharInSet(const char_type* aString,PRInt32 anOffset=-1) const;
      // PRInt32 RFindCharInSet(const nsCString& aString,PRInt32 anOffset=-1) const; // XXX nsCString only

      // XXX comparison methods
      // PRInt32 Compare(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const; // XXX nsCString only
      // PRInt32 CompareWithConversion(const char* aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aCount=-1) const; // XXX nsString only
      // PRBool  EqualsWithConversion(const char* aString,PRBool aIgnoreCase=PR_FALSE,PRInt32 aCount=-1) const;
      // PRBool  EqualsIgnoreCase(const char* aString,PRInt32 aCount=-1) const;
      // PRBool IsASCII(const PRUnichar* aBuffer=0); // XXX nsString only
      
      // XXX string conversion functions need to go here
      // char* ToCString(char* aBuf,PRUint32 aBufLength,PRUint32 anOffset=0) const; // XXX nsString only
      // float ToFloat(PRInt32* aErrorCode) const;
      // PRInt32 ToInteger(PRInt32* aErrorCode,PRUint32 aRadix=kRadix10) const;
      
      // XXX substring copy-accessors
      size_type  Left( self_type&, size_type ) const;
      size_type  Mid( self_type&, PRUint32, PRUint32 ) const;
      size_type  Right( self_type&, size_type ) const;

      // ------- end{ obsolete api support } -------

    protected:

      friend class nsTAString<char_type>;
      friend class nsTStringFragmentTuple<char_type>;
 
      // GCC 3.2.2 needs these :-(
      friend class nsTString<char_type>;
      friend class nsTPromiseFlatString<char_type>;

      nsTStringFragment()
        : abstract_string_type()
        , mData(NS_CONST_CAST(char_type *, char_traits::sEmptyBuffer))
        , mLength(0)
        , mFlags(F_TERMINATED) {} 

      nsTStringFragment( const char_type *data )
        : abstract_string_type()
        , mData(NS_CONST_CAST(char_type *, data))
        , mLength(char_traits::length(data))
        , mFlags(0) {} 

      nsTStringFragment( const char_type *data, size_type length, PRUint32 flags = 0 )
        : abstract_string_type()
        , mData(NS_CONST_CAST(char_type *, data))
        , mLength(length)
        , mFlags(flags) {} 

      // this constructor purposefully leaves mData and mLength uninitialized
      explicit
      nsTStringFragment( PRUint32 flags )
        : abstract_string_type()
        , mFlags(flags) {}

      // automatic conversion from nsTAString to nsTStringFragment OK
      nsTStringFragment( const abstract_string_type& readable )
        {
          InitFromAString(readable);
        }

      nsTStringFragment( const self_type &frag )
        : abstract_string_type()
        , mData(frag.mData)
        , mLength(frag.mLength)
        , mFlags(frag.mFlags & (F_TERMINATED | F_VOIDED)) {}

      self_type& operator=( const self_type &frag )
        {
          NS_ASSERTION(mVTable == frag.mVTable, "whoops");
          mData = frag.mData;
          mLength = frag.mLength;
          mFlags = frag.mFlags & (F_TERMINATED | F_VOIDED);
          return *this;
        }

      void InitFromAString( const abstract_string_type& );

        /**
         * required since we do not provide a virtual destructor.  a string
         * fragment should always be subclassed to form a "concrete" class type
         * that can be constructed.
         */
      ~nsTStringFragment()
         {
           // this works hand-in-hand with ~nsTAString to fake a virtual dtor.
           mVTable = nsnull;
         }

        /**
         * returns true if this fragment overlaps with the given fragment.
         */
      PRBool IsDependentOn(const char_type *start, const char_type *end) const
        {
          /**
           * if it _isn't_ the case that one fragment starts after the other ends,
           * or ends before the other starts, then, they conflict:
           * 
           *   !(f2.mStart>=f1.mEnd || f2.mEnd<=f1.mStart)
           * 
           * Simplified, that gives us:
           */
          return ( start < (mData + mLength) && end > mData );
        }

      PRUint32 Flags() const
        {
          return mFlags;
        }

      enum
        {
          F_TERMINATED = 1 << 0,
          F_VOIDED     = 1 << 1
          // additional flags defined by nsTString
        };

      char_type  *mData;
      size_type   mLength;
      PRUint32    mFlags;
  };


#endif // !defined(nsTStringFragment)
