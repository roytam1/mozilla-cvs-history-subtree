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


  /**
   * nsTSubstring_CharT
   *
   * The base string type.  This type is not instantiated directly.  A sub-
   * class is instantiated instead.  For example, see nsTString.
   *
   * NOTE: This class name should never be referenced outside the string
   * codebase.  If you need to refer to this string class, use the typedef
   * nsASingleFragment[C]String instead.
   */
class nsTSubstring_CharT : public nsTAString_CharT
  {
    public:

      typedef nsTSubstring_CharT    self_type;
      typedef nsTString_CharT       string_type;

      typedef char_type*            char_iterator;
      typedef const char_type*      const_char_iterator;

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

      NS_COM size_type CountChar( char_type ) const;
      NS_COM PRInt32 FindChar( char_type, index_type offset = 0 ) const;


        /**
         * equality
         */

      NS_COM PRBool Equals( const self_type& ) const;
      NS_COM PRBool Equals( const self_type&, const comparator_type& ) const;

      NS_COM PRBool Equals( const abstract_string_type& readable ) const;
      NS_COM PRBool Equals( const abstract_string_type& readable, const comparator_type& comp ) const;

      NS_COM PRBool Equals( const char_type* data ) const;
      NS_COM PRBool Equals( const char_type* data, const comparator_type& comp ) const;


        /**
         * assignment
         */

      void Assign( char_type c )                                                                { Assign(&c, 1); }
      NS_COM void Assign( const char_type* data, size_type length = size_type(-1) );
      NS_COM void Assign( const self_type& );
      NS_COM void Assign( const substring_tuple_type& );
      NS_COM void Assign( const abstract_string_type& );

      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const substring_tuple_type& tuple )                                 { Assign(tuple);    return *this; }
      self_type& operator=( const abstract_string_type& readable )                              { Assign(readable); return *this; }

      NS_COM void Adopt( char_type* data, size_type length = size_type(-1) );


        /**
         * buffer manipulation
         */

             void Replace( index_type cutStart, size_type cutLength, char_type c )               { Replace(cutStart, cutLength, &c, 1); }
      NS_COM void Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length = size_type(-1) );
             void Replace( index_type cutStart, size_type cutLength, const self_type& str )      { Replace(cutStart, cutLength, str.Data(), str.Length()); }
      NS_COM void Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& tuple );
      NS_COM void Replace( index_type cutStart, size_type cutLength, const abstract_string_type& readable );

      void Append( char_type c )                                                                 { Replace(mLength, 0, c); }
      void Append( const char_type* data, size_type length = size_type(-1) )                     { Replace(mLength, 0, data, length); }
      void Append( const self_type& str )                                                        { Replace(mLength, 0, str); }
      void Append( const substring_tuple_type& tuple )                                           { Replace(mLength, 0, tuple); }
      void Append( const abstract_string_type& readable )                                        { Replace(mLength, 0, readable); }

      self_type& operator+=( char_type c )                                                       { Append(c);        return *this; }
      self_type& operator+=( const char_type* data )                                             { Append(data);     return *this; }
      self_type& operator+=( const self_type& str )                                              { Append(str);      return *this; }
      self_type& operator+=( const substring_tuple_type& tuple )                                 { Append(tuple);    return *this; }
      self_type& operator+=( const abstract_string_type& readable )                              { Append(readable); return *this; }

      void Insert( char_type c, index_type pos )                                                 { Replace(pos, 0, c); }
      void Insert( const char_type* data, index_type pos, size_type length = size_type(-1) )     { Replace(pos, 0, data, length); }
      void Insert( const self_type& str, index_type pos )                                        { Replace(pos, 0, str); }
      void Insert( const substring_tuple_type& tuple, index_type pos )                           { Replace(pos, 0, tuple); }
      void Insert( const abstract_string_type& readable, index_type pos )                        { Replace(pos, 0, readable); }

      void Cut( index_type cutStart, size_type cutLength )                                       { Replace(cutStart, cutLength, char_traits::sEmptyBuffer, 0); }


        /**
         * buffer sizing
         */

      NS_COM void SetCapacity( size_type capacity );

      NS_COM void SetLength( size_type );

      void Truncate( size_type newLength = 0 )
        {
          NS_ASSERTION(newLength <= mLength, "Truncate cannot make string longer");
          SetLength(newLength);
        }


        /**
         * string data is never null, but can be marked void.  if true, the
         * string will be truncated.  @see nsTSubstring_CharT::IsVoid
         */

      NS_COM void SetIsVoid( PRBool );


    public:

        /**
         * this is public to support automatic conversion of tuple to string
         * base type, which helps avoid converting to nsTAString.
         */
      nsTSubstring_CharT(const substring_tuple_type& tuple)
        : abstract_string_type(nsnull, 0, F_NONE)
        {
          Assign(tuple);
        }

    protected:

      friend class nsTObsoleteAStringThunk_CharT;
      friend class nsTAString_CharT;
      friend class nsTSubstringTuple_CharT;

        // default initialization 
      nsTSubstring_CharT()
        : abstract_string_type(
              NS_CONST_CAST(char_type*, char_traits::sEmptyBuffer), 0, F_TERMINATED) {}

        // allow subclasses to initialize fields directly
      nsTSubstring_CharT( char_type *data, size_type length, PRUint32 flags )
        : abstract_string_type(data, length, flags) {}

        // version of constructor that leaves mData and mLength uninitialized
      explicit
      nsTSubstring_CharT( PRUint32 flags )
        : abstract_string_type(flags) {}

        // copy-constructor, constructs as dependent on given object
        // (NOTE: this is for internal use only)
      nsTSubstring_CharT( const self_type& str )
        : abstract_string_type(
              str.mData, str.mLength, str.mFlags & (F_TERMINATED | F_VOIDED)) {}

      void      ReleaseData();
      PRBool    MutatePrep( size_type, char_type**, PRUint32* );
      void      ReplacePrep( index_type cutStart, size_type cutLength, size_type newLength);
      size_type Capacity() const;

      NS_COM void EnsureMutable();

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

      PRBool IsMutable() const
        {
          return mFlags & (F_SHARED | F_OWNED | F_FIXED);
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
          F_FIXED      = 1 << 4   // mData is pointing at a fixed-size writable buffer
        };
  };
