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

#ifndef nsTStringBase_h___
#define nsTStringBase_h___

  // enable support for the obsolete string API if not explicitly disabled
#ifndef MOZ_STRING_WITH_OBSOLETE_API
#define MOZ_STRING_WITH_OBSOLETE_API 1
  // radix values for ToInteger/AppendInt
#define kRadix10        (10)
#define kRadix16        (16)
#define kAutoDetect     (100)
#define kRadixUnknown   (kAutoDetect+1)
#endif

#ifndef nsTAString_h___
#include "nsTAString.h"
#endif

#ifndef nsTStringTuple_h___
#include "nsTStringTuple.h"
#endif


static const PRInt32 kNotFound = -1;


  /**
   * nsTStringBase
   *
   * The base string type.  This type is not instantiated directly.  A sub-
   * class is instantiated instead.  For example, see nsTString.
   */
template <class CharT>
class NS_COM nsTStringBase : public nsTAString<CharT>
  {
    public:

      typedef CharT                                           char_type;
      typedef nsCharTraits<char_type>                         char_traits;

      typedef typename char_traits::incompatible_char_type    incompatible_char_type;

      typedef nsTStringBase<char_type>                        self_type;
      typedef nsTString<char_type>                            string_type;
      typedef nsTStringTuple<char_type>                       string_tuple_type;
      typedef nsTAString<char_type>                           abstract_string_type;

      typedef nsReadingIterator<char_type>                    const_iterator;
      typedef nsWritingIterator<char_type>                    iterator;
      typedef char_type*                                      char_iterator;
      typedef const char_type*                                const_char_iterator;

      typedef nsTStringComparator<char_type>                  comparator_type;

      typedef PRUint32                                        size_type;
      typedef PRUint32                                        index_type;

    public:

        /**
         * reading iterators
         */

      const_iterator& BeginReading( const_iterator& iter ) const
        {
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mStart;
          return iter;
        }

      const_iterator& EndReading( const_iterator& iter ) const
        {
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      const_char_iterator& BeginReading( const_char_iterator& iter ) const
        {
          return iter = mData;
        }

      const_char_iterator& EndReading( const_char_iterator& iter ) const
        {
          return iter = mData + mLength;
        }


        /**
         * writing iterators
         */
      
      iterator& BeginWriting( iterator& iter )
        {
          EnsureMutable();
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mStart;
          return iter;
        }

      iterator& EndWriting( iterator& iter )
        {
          EnsureMutable();
          iter.mStart = mData;
          iter.mEnd = mData + mLength;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      char_iterator& BeginWriting( char_iterator& iter )
        {
          EnsureMutable();
          return iter = mData;
        }

      char_iterator& EndWriting( char_iterator& iter )
        {
          EnsureMutable();
          return iter = mData + mLength;
        }


        /**
         * accessors
         */

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

      PRBool IsVoid() const
        {
          return mFlags & F_VOIDED;
        }

      PRBool IsTerminated() const
        {
          return mFlags & F_TERMINATED;
        }

      char_type CharAt( index_type i ) const
        {
          NS_ASSERTION((mFlags & F_TERMINATED) ? i <= mLength : i < mLength,
              "index exceeds allowable range");
          return mData[i];
        }

      char_type operator[]( index_type i ) const
        {
          return CharAt(i);
        }

      char_type First() const
        {
          NS_ASSERTION(mLength > 0, "|First()| called on an empty string");
          return mData[0];
        }

      inline
      char_type Last() const
        {
          NS_ASSERTION(mLength > 0, "|Last()| called on an empty string");
          return mData[mLength - 1];
        }

      size_type CountChar( char_type ) const;
      PRInt32 FindChar( char_type, index_type offset = 0 ) const;


        /**
         * equality
         */

      PRBool Equals( const self_type& ) const;
      PRBool Equals( const self_type&, const comparator_type& ) const;

      PRBool Equals( const abstract_string_type& readable ) const;
      PRBool Equals( const abstract_string_type& readable, const comparator_type& comp ) const;

      PRBool Equals( const char_type* data ) const;
      PRBool Equals( const char_type* data, const comparator_type& comp ) const;


        /**
         * assignment
         */

      void Assign( char_type c )                                                                { Assign(&c, 1); }
      void Assign( const char_type* data, size_type length = size_type(-1) );
      void Assign( const self_type& );
      void Assign( const string_tuple_type& );
      void Assign( const abstract_string_type& );

      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const string_tuple_type& tuple )                                    { Assign(tuple);    return *this; }
      self_type& operator=( const abstract_string_type& readable )                              { Assign(readable); return *this; }

      void Adopt( char_type* data, size_type length = size_type(-1) );


        /**
         * buffer manipulation
         */

      void Replace( index_type cutStart, size_type cutLength, char_type c )                      { Replace(cutStart, cutLength, &c, 1); }
      void Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length = size_type(-1) );
      void Replace( index_type cutStart, size_type cutLength, const self_type& str )             { Replace(cutStart, cutLength, str.Data(), str.Length()); }
      void Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple );
      void Replace( index_type cutStart, size_type cutLength, const abstract_string_type& readable );

      void Append( char_type c )                                                                 { Replace(mLength, 0, c); }
      void Append( const char_type* data, size_type length = size_type(-1) )                     { Replace(mLength, 0, data, length); }
      void Append( const self_type& str )                                                        { Replace(mLength, 0, str); }
      void Append( const string_tuple_type& tuple )                                              { Replace(mLength, 0, tuple); }
      void Append( const abstract_string_type& readable )                                        { Replace(mLength, 0, readable); }

      self_type& operator+=( char_type c )                                                       { Append(c);        return *this; }
      self_type& operator+=( const char_type* data )                                             { Append(data);     return *this; }
      self_type& operator+=( const self_type& str )                                              { Append(str);      return *this; }
      self_type& operator+=( const string_tuple_type& tuple )                                    { Append(tuple);    return *this; }
      self_type& operator+=( const abstract_string_type& readable )                              { Append(readable); return *this; }

      void Insert( char_type c, index_type pos )                                                 { Replace(pos, 0, c); }
      void Insert( const char_type* data, index_type pos, size_type length = size_type(-1) )     { Replace(pos, 0, data, length); }
      void Insert( const self_type& str, index_type pos )                                        { Replace(pos, 0, str); }
      void Insert( const string_tuple_type& tuple, index_type pos )                              { Replace(pos, 0, tuple); }
      void Insert( const abstract_string_type& readable, index_type pos )                        { Replace(pos, 0, readable); }

      void Cut( index_type cutStart, size_type cutLength )                                       { Replace(cutStart, cutLength, char_traits::sEmptyBuffer, 0); }


        /**
         * buffer sizing
         */

      void SetCapacity( size_type capacity );

      void SetLength( size_type );

      void Truncate( size_type newLength = 0 )
        {
          NS_ASSERTION(newLength <= mLength, "Truncate cannot make string longer");
          SetLength(newLength);
        }


        /**
         * string data is never null, but can be marked void.  if true, the
         * string will be truncated.  @see nsTStringBase::IsVoid
         */

      void SetIsVoid( PRBool );


#if MOZ_STRING_WITH_OBSOLETE_API


        /**
         *  Search for the given substring within this string.
         *  
         *  @param   aString is substring to be sought in this
         *  @param   aIgnoreCase selects case sensitivity
         *  @param   aOffset tells us where in this string to start searching
         *  @param   aCount tells us how far from the offset we are to search. Use
         *           -1 to search the whole string.
         *  @return  offset in string, or kNotFound
         */

      PRInt32 Find( const nsCString& aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aOffset=0, PRInt32 aCount=-1 ) const;
      PRInt32 Find( const char* aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aOffset=0, PRInt32 aCount=-1 ) const;

        // NOTE: these two variants are only implemented for nsTStringBase<PRUnichar>
      PRInt32 Find( const nsAFlatString& aString, PRInt32 aOffset=0, PRInt32 aCount=-1 ) const;
      PRInt32 Find( const PRUnichar* aString, PRInt32 aOffset=0, PRInt32 aCount=-1 ) const;

        
        /**
         * This methods scans the string backwards, looking for the given string
         *
         * @param   aString is substring to be sought in this
         * @param   aIgnoreCase tells us whether or not to do caseless compare
         * @param   aOffset tells us where in this string to start searching.
         *          Use -1 to search from the end of the string.
         * @param   aCount tells us how many iterations to make starting at the
         *          given offset.
         * @return  offset in string, or kNotFound
         */

      PRInt32 RFind( const nsCString& aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aOffset=-1, PRInt32 aCount=-1 ) const;
      PRInt32 RFind( const char* aCString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aOffset=-1, PRInt32 aCount=-1 ) const;

        // NOTE: these two variants are only implemented for nsTStringBase<PRUnichar>
      PRInt32 RFind( const nsAFlatString& aString, PRInt32 aOffset=-1, PRInt32 aCount=-1 ) const;
      PRInt32 RFind( const PRUnichar* aString, PRInt32 aOffset=-1, PRInt32 aCount=-1 ) const;


        /**
         *  Search for given char within this string
         *  
         *  @param   aChar is the character to search for
         *  @param   aOffset tells us where in this strig to start searching
         *  @param   aCount tells us how far from the offset we are to search.
         *           Use -1 to search the whole string.
         *  @return  offset in string, or kNotFound
         */

      // PRInt32 FindChar( PRUnichar aChar, PRInt32 aOffset=0, PRInt32 aCount=-1 ) const;
      PRInt32 RFindChar( PRUnichar aChar, PRInt32 aOffset=-1, PRInt32 aCount=-1 ) const;


        /**
         * This method searches this string for the first character found in
         * the given string.
         *
         * @param aString contains set of chars to be found
         * @param aOffset tells us where in this string to start searching
         *        (counting from left)
         * @return offset in string, or kNotFound
         */

      PRInt32 FindCharInSet( const char* aString, PRInt32 aOffset=0 ) const;
      PRInt32 FindCharInSet( const nsCString& aString, PRInt32 aOffset=0 ) const
        {
          return FindCharInSet(aString.get(), aOffset);
        }

        // NOTE: this variant is only implemented for nsTStringBase<PRUnichar>
      PRInt32 FindCharInSet( const PRUnichar* aString, PRInt32 aOffset=0 ) const;


        /**
         * This method searches this string for the last character found in
         * the given string.
         *
         * @param aString contains set of chars to be found
         * @param aOffset tells us where in this string to start searching
         *        (counting from left)
         * @return offset in string, or kNotFound
         */

      PRInt32 RFindCharInSet( const char_type* aString, PRInt32 aOffset=-1 ) const;
      PRInt32 RFindCharInSet( const nsCString& aString, PRInt32 aOffset=-1 ) const
        {
          return RFindCharInSet(aString.get(), aOffset);
        }


        /**
         * Compares a given string to this string. 
         *
         * @param   aString is the string to be compared
         * @param   aIgnoreCase tells us how to treat case
         * @param   aCount tells us how many chars to compare
         * @return  -1,0,1
         */

        // NOTE: this method is only implemented for nsTStringBase<char>
      PRInt32 Compare( const char* aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aCount=-1 ) const;

        // NOTE: this method is only implemented for nsTStringBase<PRUnichar>
      PRInt32 CompareWithConversion( const char* aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aCount=-1 ) const;


        /**
         * Equality check between given string and this string.
         *
         * @param   aString is the string to check
         * @param   aIgnoreCase tells us how to treat case
         * @param   aCount tells us how many chars to compare
         * @return  boolean
         */

      PRBool EqualsWithConversion( const char* aString, PRBool aIgnoreCase=PR_FALSE, PRInt32 aCount=-1 ) const;
      PRBool EqualsIgnoreCase( const char* aString, PRInt32 aCount=-1 ) const
        {
          return EqualsWithConversion(aString, PR_TRUE, aCount);
        }


        /**
         *  Determine if given buffer is plain ascii
         *  
         *  @param   aBuffer -- if null, then we test *this, otherwise we test given buffer
         *  @return  TRUE if is all ascii chars or if strlen==0
         */

        // NOTE: this method is only implemented for nsTStringBase<PRUnichar>
      PRBool IsASCII(const PRUnichar* aBuffer=0);


        /**
         *  Determine if given char is a valid space character
         *  
         *  @param   aChar is character to be tested
         *  @return  TRUE if is valid space char
         */

        // NOTE: this method is only implemented for nsTStringBase<PRUnichar>
      static  PRBool IsSpace(PRUnichar ch);

      
        /**
         * Copies data from internal buffer onto given char* buffer
         *
         * NOTE: This only copies as many chars as will fit in given buffer (clips)
         * @param aBuf is the buffer where data is stored
         * @param aBuflength is the max # of chars to move to buffer
         * @param aOffset is the offset to copy from
         * @return ptr to given buffer
         */

        // NOTE: this method is only implemented for nsTStringBase<PRUnichar>
      char* ToCString( char* aBuf, PRUint32 aBufLength, PRUint32 aOffset=0 ) const;


        /**
         * Perform string to float conversion.
         *
         * @param   aErrorCode will contain error if one occurs
         * @return  float rep of string value
         */
      float ToFloat( PRInt32* aErrorCode ) const;


        /**
         * Perform string to int conversion.
         * @param   aErrorCode will contain error if one occurs
         * @param   aRadix tells us which radix to assume; kAutoDetect tells us to determine the radix for you.
         * @return  int rep of string value, and possible (out) error code
         */
      PRInt32 ToInteger( PRInt32* aErrorCode, PRUint32 aRadix=kRadix10 ) const;
      

        /**
         * |Left|, |Mid|, and |Right| are annoying signatures that seem better almost
         * any _other_ way than they are now.  Consider these alternatives
         * 
         * aWritable = aReadable.Left(17);   // ...a member function that returns a |Substring|
         * aWritable = Left(aReadable, 17);  // ...a global function that returns a |Substring|
         * Left(aReadable, 17, aWritable);   // ...a global function that does the assignment
         * 
         * as opposed to the current signature
         * 
         * aReadable.Left(aWritable, 17);    // ...a member function that does the assignment
         * 
         * or maybe just stamping them out in favor of |Substring|, they are just duplicate functionality
         *         
         * aWritable = Substring(aReadable, 0, 17);
         */

      size_type Mid( self_type& aResult, PRUint32 aStartPos, PRUint32 aCount ) const;

      size_type Left( self_type& aResult, size_type aCount ) const
        {
          return Mid(aResult, 0, aCount);
        }

      size_type Right( self_type& aResult, size_type aCount ) const
        {
          aCount = NS_MIN(mLength, aCount);
          return Mid(aResult, mLength - aCount, aCount);
        }


        /**
         * Set a char inside this string at given index
         *
         * @param aChar is the char you want to write into this string
         * @param anIndex is the ofs where you want to write the given char
         * @return TRUE if successful
         */

      PRBool SetCharAt( PRUnichar aChar, PRUint32 aIndex );


        /**
         *  These methods are used to remove all occurances of the
         *  characters found in aSet from this string.
         *  
         *  @param  aSet -- characters to be cut from this
         */
      void StripChars( const char* aSet );


        /**
         *  This method is used to remove all occurances of aChar from this
         * string.
         *  
         *  @param  aChar -- char to be stripped
         *  @param  aOffset -- where in this string to start stripping chars
         */
         
      void StripChar( char_type aChar, PRInt32 aOffset=0 );


        /**
         *  This method strips whitespace throughout the string.
         */
      void StripWhitespace();


        /**
         *  swaps occurence of 1 string for another
         */

      void ReplaceChar( char_type aOldChar, char_type aNewChar );
      void ReplaceChar( const char* aSet, char_type aNewChar );
      void ReplaceSubstring( const self_type& aTarget, const self_type& aNewValue);
      void ReplaceSubstring( const char_type* aTarget, const char_type* aNewValue);


        /**
         *  This method trims characters found in aTrimSet from
         *  either end of the underlying string.
         *  
         *  @param   aSet -- contains chars to be trimmed from both ends
         *  @param   aEliminateLeading
         *  @param   aEliminateTrailing
         *  @param   aIgnoreQuotes -- if true, causes surrounding quotes to be ignored
         *  @return  this
         */
      void Trim( const char* aSet, PRBool aEliminateLeading=PR_TRUE, PRBool aEliminateTrailing=PR_TRUE, PRBool aIgnoreQuotes=PR_FALSE );

        /**
         *  This method strips whitespace from string.
         *  You can control whether whitespace is yanked from start and end of
         *  string as well.
         *  
         *  @param   aEliminateLeading controls stripping of leading ws
         *  @param   aEliminateTrailing controls stripping of trailing ws
         */
      void CompressWhitespace( PRBool aEliminateLeading=PR_TRUE, PRBool aEliminateTrailing=PR_TRUE );


        /**
         * assign/append/insert with _LOSSY_ conversion
         */

      void AssignWithConversion( const nsTAString<incompatible_char_type>& aString );
      void AssignWithConversion( const incompatible_char_type* aData, PRInt32 aLength=-1 );

      void AppendWithConversion( const nsTAString<incompatible_char_type>& aString );
      void AppendWithConversion( const incompatible_char_type* aData, PRInt32 aLength=-1 );

        // NOTE: this method is only implemented for nsTStringBase<PRUnichar>
      void InsertWithConversion( const incompatible_char_type* aData, PRUint32 aOffset, PRInt32 aCount=-1 );

        /**
         * Append the given integer to this string 
         */

      void AppendInt( PRInt32 aInteger, PRInt32 aRadix=kRadix10 ); //radix=8,10 or 16

        /**
         * Append the given float to this string 
         */

      void AppendFloat( double aFloat );


#endif // !MOZ_STRING_WITH_OBSOLETE_API

    public:

        /**
         * this is public to support automatic conversion of tuple to string
         * base type, which helps avoid converting to nsTAString.
         */
      nsTStringBase(const string_tuple_type& tuple)
        : abstract_string_type(nsnull, 0, F_NONE)
        {
          Assign(tuple);
        }

    protected:

      friend class nsTObsoleteAStringThunk<char_type>;
      friend class nsTAString<char_type>;
      friend class nsTStringTuple<char_type>;

        // default initialization 
      nsTStringBase()
        : abstract_string_type(
              NS_CONST_CAST(char_type*, char_traits::sEmptyBuffer), 0, F_TERMINATED) {}

        // allow subclasses to initialize fields directly
      nsTStringBase( char_type *data, size_type length, PRUint32 flags )
        : abstract_string_type(data, length, flags) {}

        // version of constructor that leaves mData and mLength uninitialized
      nsTStringBase( PRUint32 flags )
        : abstract_string_type(flags) {}

        // copy-constructor, constructs as dependent on given object
        // (NOTE: this is for internal use only)
      nsTStringBase( const self_type& str )
        : abstract_string_type(
              str.mData, str.mLength, str.mFlags & (F_TERMINATED | F_VOIDED)) {}

      void      ReleaseData();
      PRBool    MutatePrep( size_type, char_type**, PRUint32* );
      void      ReplacePrep( index_type cutStart, size_type cutLength, size_type newLength);
      size_type Capacity() const;
      void      EnsureMutable();

        /**
         * returns true if this string overlaps with the given string fragment.
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

      PRBool IsShared() const
        {
          return mFlags & F_SHARED;
        }

      PRBool IsOwned() const
        {
          return mFlags & F_OWNED;
        }

      PRBool IsFixed() const
        {
          return mFlags & F_FIXED;
        }

    public:

      // mFlags is a bitwise combination of the following flags
      enum
        {
          F_NONE       = 0,       // no flags
          F_TERMINATED = 1 << 0,  // IsTerminated returns true
          F_VOIDED     = 1 << 1,  // IsVoid returns true
          F_SHARED     = 1 << 2,  // mData[0] is prefixed with additional fields
          F_OWNED      = 1 << 3,  // mData is owned by this class
          F_FIXED      = 1 << 4   // mData is pointing at nsTAutoString::mFixedBuf
        };
  };

#endif // !defined(nsTStringBase_h___)
