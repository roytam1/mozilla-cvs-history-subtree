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
   * The base for string comparators
   */
class NS_COM nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const = 0;
      virtual int operator()( char_type, char_type ) const = 0;
  };


  /**
   * The default string comparator (case-sensitive comparision)
   */
class NS_COM nsTDefaultStringComparator_CharT
    : public nsTStringComparator_CharT
  {
    public:
      typedef CharT char_type;

      nsTDefaultStringComparator_CharT() {}

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const;
      virtual int operator()( char_type, char_type ) const;
  };


  /**
   * XXX FIXME
   * |nsTAString| is the most abstract class in the string hierarchy.
   * Strings implementing |nsTAString| may be stored in multiple
   * fragments.  They need not be null-terminated and they may contain
   * embedded null characters.  They may be dependent objects that
   * depend on other strings.
   *
   * See also |nsASingleFragmentC?String| and |nsAFlatC?String|, the
   * other main abstract classes in the string hierarchy.
   */

class nsTAString_CharT
  {
    public:

      typedef CharT                                  char_type;
      typedef nsCharTraits<char_type>                char_traits;

      typedef char_traits::incompatible_char_type    incompatible_char_type;

      typedef nsTAString_CharT                       self_type;
      typedef nsTAString_CharT                       abstract_string_type;
      typedef nsTObsoleteAString_CharT               obsolete_string_type;
      typedef nsTSubstring_CharT                     substring_type;
      typedef nsTSubstringTuple_CharT                substring_tuple_type;

      typedef nsReadingIterator<char_type>           const_iterator;
      typedef nsWritingIterator<char_type>           iterator;

      typedef nsTStringComparator_CharT              comparator_type;

      typedef PRUint32                               size_type;
      typedef PRUint32                               index_type;

    public:

        // this acts like a virtual destructor
      NS_COM ~nsTAString_CharT();

      inline const_iterator& BeginReading( const_iterator& iter ) const
        {
          size_type len = GetReadableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mStart;
          return iter;
        }

      inline const_iterator& EndReading( const_iterator& iter ) const
        {
          size_type len = GetReadableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      inline iterator& BeginWriting( iterator& iter )
        {
          size_type len = GetWritableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mStart;
          return iter;
        }

      inline iterator& EndWriting( iterator& iter )
        {
          size_type len = GetWritableBuffer(&iter.mStart);
          iter.mEnd = iter.mStart + len;
          iter.mPosition = iter.mEnd;
          return iter;
        }

      NS_COM size_type Length() const;
      PRBool IsEmpty() const { return Length() == 0; }

      NS_COM PRBool Equals( const self_type& ) const;
      NS_COM PRBool Equals( const self_type&, const comparator_type& ) const;
      NS_COM PRBool Equals( const char_type* ) const;
      NS_COM PRBool Equals( const char_type*, const comparator_type& ) const;

      NS_COM PRBool IsVoid() const;
      NS_COM void SetIsVoid( PRBool );

      NS_COM PRBool IsTerminated() const;

        /**
         * these are contant time since nsTAString_CharT uses flat storage
         */
      NS_COM char_type First() const;
      NS_COM char_type Last() const;

      NS_COM size_type CountChar( char_type ) const;

      NS_COM PRInt32 FindChar( char_type, index_type offset = 0 ) const;

        /**
         * |SetCapacity| is not required to do anything; however, it can be
         * used as a hint to the implementation to reduce allocations.
         * |SetCapacity(0)| is a suggestion to discard all associated storage.
         */
      NS_COM void SetCapacity( size_type );

        /**
         * |SetLength| is used in two ways:
         *   1) to |Cut| a suffix of the string;
         *   2) to prepare to |Append| or move characters around.
         *
         * External callers are not allowed to use |SetLength| in this
         * latter capacity, and should prefer |Truncate| for the former.
         * In other words, |SetLength| is deprecated for all use outside
         * of the string library and the internal use may at some point
         * be replaced as well.
         *
         * Should this really be a public operation?
         *
         * Additionally, your implementation of |SetLength| need not
         * satisfy (2) if and only if you override the |do_...| routines
         * to not need this facility.
         *
         * This distinction makes me think the two different uses should
         * be split into two distinct functions.
         */
      NS_COM void SetLength( size_type );


        /**
         * Can't use Truncate to make a string longer!
         */
      NS_COM void Truncate( size_type aNewLength=0 ) { SetLength(aNewLength); }


        /**
         * |Assign()| and |operator=()| make |this| equivalent to the
         * string or buffer given as an argument.  If possible, they do
         * this by sharing a refcounted buffer (see
         * |nsSharableC?String|, |nsXPIDLC?String|.  If not, they copy
         * the buffer into their own buffer.
         */

      NS_COM void Assign( const self_type& readable );
      NS_COM void Assign( const substring_tuple_type& tuple );
      NS_COM void Assign( const char_type* data );
      NS_COM void Assign( const char_type* data, size_type length );
      NS_COM void Assign( char_type c );

        // copy-assignment operator.  I must define my own if I don't want the compiler to make me one
      self_type& operator=( const self_type& readable )                                             { Assign(readable); return *this; }
      self_type& operator=( const substring_tuple_type& tuple )                                     { Assign(tuple); return *this; }
      self_type& operator=( const char_type* data )                                                 { Assign(data); return *this; }
      self_type& operator=( char_type c )                                                           { Assign(c); return *this; }



        /**
         * |Append()|, |operator+=()|
         */ 

      NS_COM void Append( const self_type& readable );
      NS_COM void Append( const substring_tuple_type& tuple );
      NS_COM void Append( const char_type* data );
      NS_COM void Append( const char_type* data, size_type length );
      NS_COM void Append( char_type c );

      self_type& operator+=( const self_type& readable )                                            { Append(readable); return *this; }
      self_type& operator+=( const substring_tuple_type& tuple )                                    { Append(tuple); return *this; }
      self_type& operator+=( const char_type* data )                                                { Append(data); return *this; }
      self_type& operator+=( char_type c )                                                          { Append(c); return *this; }

        /**
         * |Insert()|
         *  Note: I would really like to move the |pos| parameter to the front of the argument list
         */ 

      NS_COM void Insert( const self_type& readable, index_type pos );
      NS_COM void Insert( const substring_tuple_type& tuple, index_type pos );
      NS_COM void Insert( const char_type* data, index_type pos );
      NS_COM void Insert( const char_type* data, index_type pos, size_type length );
      NS_COM void Insert( char_type c, index_type pos );

      NS_COM void Cut( index_type cutStart, size_type cutLength );

      NS_COM void Replace( index_type cutStart, size_type cutLength, const self_type& readable );
      NS_COM void Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& readable );

    private:
        // NOT TO BE IMPLEMENTED
      //size_type  CountChar( incompatible_char_type );
      void operator=  ( incompatible_char_type );
      void Assign     ( incompatible_char_type );
      void operator+= ( incompatible_char_type );
      void Append     ( incompatible_char_type );
      void Insert     ( incompatible_char_type, index_type );

    protected:

      friend class nsTSubstringTuple_CharT;

      // XXX still needed now that these aren't template types??
      // GCC 3.2 erroneously needs these (they are subclasses!)
      friend class nsTSubstring_CharT;
      friend class nsTDependentSubstring_CharT;
      friend class nsTPromiseFlatString_CharT;

        /**
         * the address of our virtual function table.  required for backwards
         * compatibility with Mozilla 1.0 frozen nsAC?String interface.
         */
      const void* mVTable;

        /**
         * these fields are "here" only when mVTable == sCanonicalVTable
         * XXX add comments explaining why these are here!!
         */
      char_type*  mData;
      size_type   mLength;
      PRUint32    mFlags;

        /**
         * nsTAString_CharT must be subclassed before it can be instantiated.
         */
      nsTAString_CharT(char_type* data, size_type length, PRUint32 flags)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mData(data)
        , mLength(length)
        , mFlags(flags)
        {}

        /**
         * optional ctor for use by subclasses.
         *
         * NOTE: mData and mLength are intentionally left uninitialized.
         */
      explicit
      nsTAString_CharT(PRUint32 flags)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mFlags(flags)
        {}

    public:
      
        /**
         * this is public to support automatic conversion of tuple to abstract
         * string, which is necessary to support our API.
         */
      nsTAString_CharT(const substring_tuple_type& tuple)
        : mVTable(obsolete_string_type::sCanonicalVTable)
        , mData(nsnull)
        , mLength(0)
        , mFlags(0)
        {
          Assign(tuple);
        }

    protected:

        /**
         * get pointer to internal string buffer (may not be null terminated).
         * return length of buffer.
         */
      NS_COM size_type GetReadableBuffer( const char_type **data ) const;
      NS_COM size_type GetWritableBuffer(       char_type **data );

        /**
         * returns true if this tuple is dependent on (i.e., overlapping with)
         * the given char sequence.
         */
      PRBool IsDependentOn(const char_type *start, const char_type *end) const;

        /**
         * we can be converted to a const nsTSubstring (dependent on this)
         */
      const substring_type ToSubstring() const;

    private:

        /**
         * type cast helpers
         */

      const obsolete_string_type* AsObsoleteString() const
        {
          return NS_REINTERPRET_CAST(const obsolete_string_type*, this);
        }

      obsolete_string_type* AsObsoleteString()
        {
          return NS_REINTERPRET_CAST(obsolete_string_type*, this);
        }

      const substring_type* AsSubstring() const
        {
          return NS_REINTERPRET_CAST(const substring_type*, this);
        }

      substring_type* AsSubstring()
        {
          return NS_REINTERPRET_CAST(substring_type*, this);
        }
  };


NS_COM
int Compare( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs, const nsTStringComparator_CharT& = nsTDefaultStringComparator_CharT() );


inline
PRBool operator!=( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return !lhs.Equals(rhs);
  }

inline
PRBool operator< ( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return Compare(lhs, rhs)< 0;
  }

inline
PRBool operator<=( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return Compare(lhs, rhs)<=0;
  }

inline
PRBool operator==( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return lhs.Equals(rhs);
  }

inline
PRBool operator>=( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return Compare(lhs, rhs)>=0;
  }

inline
PRBool operator> ( const nsTAString_CharT& lhs, const nsTAString_CharT& rhs )
  {
    return Compare(lhs, rhs)> 0;
  }
