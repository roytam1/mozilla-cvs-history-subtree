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

#ifndef nsTAString_h___
#define nsTAString_h___

#ifndef nsStringFwd_h___
#include "nsStringFwd.h"
#endif

#ifndef nsStringIterator_h___
#include "nsStringIterator.h"
#endif

#ifndef nsTObsoleteAString_h___
#include "nsTObsoleteAString.h"
#endif


  /**
   * The base for string comparators
   */
template <class CharT>
class NS_COM nsTStringComparator
  {
    public:
      typedef CharT char_type;

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const = 0;
      virtual int operator()( char_type, char_type ) const = 0;
  };

template <class CharT>
class NS_COM nsTDefaultStringComparator
    : public nsTStringComparator<CharT>
  {
    public:
      typedef CharT char_type;

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const;
      virtual int operator()( char_type, char_type ) const;
  };

class NS_COM nsCaseInsensitiveCStringComparator
    : public nsCStringComparator
  {
    public:
      typedef char char_type;

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

template<class CharT>
class NS_COM nsTAString
  {
    public:

      typedef CharT                                           char_type;
      typedef nsCharTraits<char_type>                         char_traits;

      typedef typename char_traits::incompatible_char_type    incompatible_char_type;

      typedef nsTAString<char_type>                           self_type;
      typedef nsTAString<char_type>                           abstract_string_type;
      typedef nsTObsoleteAString<char_type>                   obsolete_string_type;
      typedef nsTStringBase<char_type>                        string_base_type;
      typedef nsTStringTuple<char_type>                       string_tuple_type;

      typedef nsReadingIterator<char_type>                    const_iterator;
      typedef nsWritingIterator<char_type>                    iterator;

      typedef nsTStringComparator<char_type>                  comparator_type;

      typedef PRUint32                                        size_type;
      typedef PRUint32                                        index_type;

    public:

        // this acts like a virtual destructor
      ~nsTAString();

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

      size_type Length() const;
      PRBool IsEmpty() const { return Length() == 0; }

      PRBool Equals( const self_type& ) const;
      PRBool Equals( const self_type&, const comparator_type& ) const;
      PRBool Equals( const char_type* ) const;
      PRBool Equals( const char_type*, const comparator_type& ) const;

      PRBool IsVoid() const;
      void SetIsVoid( PRBool );

      PRBool IsTerminated() const;

        /**
         * these are contant time since nsTAString uses flat storage
         */
      char_type First() const;
      char_type Last() const;

      size_type CountChar( char_type ) const;

      PRInt32 FindChar( char_type, index_type offset = 0 ) const;

        /**
         * |SetCapacity| is not required to do anything; however, it can be
         * used as a hint to the implementation to reduce allocations.
         * |SetCapacity(0)| is a suggestion to discard all associated storage.
         */
      void SetCapacity( size_type );

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
      void SetLength( size_type );


        /**
         * Can't use Truncate to make a string longer!
         */
      void
      Truncate( size_type aNewLength=0 ) { SetLength(aNewLength); }


        /**
         * |Assign()| and |operator=()| make |this| equivalent to the
         * string or buffer given as an argument.  If possible, they do
         * this by sharing a refcounted buffer (see
         * |nsSharableC?String|, |nsXPIDLC?String|.  If not, they copy
         * the buffer into their own buffer.
         */

      void Assign( const self_type& readable );
      void Assign( const string_tuple_type& tuple );
      void Assign( const char_type* data );
      void Assign( const char_type* data, size_type length );
      void Assign( char_type c );

        // copy-assignment operator.  I must define my own if I don't want the compiler to make me one
      self_type& operator=( const self_type& readable )                                             { Assign(readable); return *this; }
      self_type& operator=( const string_tuple_type& tuple )                                        { Assign(tuple); return *this; }
      self_type& operator=( const char_type* data )                                                 { Assign(data); return *this; }
      self_type& operator=( char_type c )                                                           { Assign(c); return *this; }



        /**
         * |Append()|, |operator+=()|
         */ 

      void Append( const self_type& readable );
      void Append( const string_tuple_type& tuple );
      void Append( const char_type* data );
      void Append( const char_type* data, size_type length );
      void Append( char_type c );

      self_type& operator+=( const self_type& readable )                                            { Append(readable); return *this; }
      self_type& operator+=( const string_tuple_type& tuple )                                       { Append(tuple); return *this; }
      self_type& operator+=( const char_type* data )                                                { Append(data); return *this; }
      self_type& operator+=( char_type c )                                                          { Append(c); return *this; }

        /**
         * |Insert()|
         *  Note: I would really like to move the |pos| parameter to the front of the argument list
         */ 

      void Insert( const self_type& readable, index_type pos );
      void Insert( const string_tuple_type& tuple, index_type pos );
      void Insert( const char_type* data, index_type pos );
      void Insert( const char_type* data, index_type pos, size_type length );
      void Insert( char_type c, index_type pos );

      void Cut( index_type cutStart, size_type cutLength );

      void Replace( index_type cutStart, size_type cutLength, const self_type& readable );
      void Replace( index_type cutStart, size_type cutLength, const string_tuple_type& readable );

    private:
        // NOT TO BE IMPLEMENTED
      //size_type  CountChar( incompatible_char_type );
      void operator=  ( incompatible_char_type );
      void Assign     ( incompatible_char_type );
      void operator+= ( incompatible_char_type );
      void Append     ( incompatible_char_type );
      void Insert     ( incompatible_char_type, index_type );

    protected:

      friend class nsTStringTuple<char_type>;

      // GCC 3.2 erroneously needs these (they are subclasses!)
      friend class nsTStringBase<char_type>;
      friend class nsTDependentSubstring<char_type>;
      friend class nsTPromiseFlatString<char_type>;

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
         * nsTAString must be subclassed before it can be instantiated.
         */
      nsTAString(char_type* data, size_type length, PRUint32 flags)
        : mVTable(nsTObsoleteAString<char_type>::sCanonicalVTable)
        , mData(data)
        , mLength(length)
        , mFlags(flags)
        {}

        /**
         * optional ctor for use by subclasses.
         *
         * mData and mLength are intentionally left uninitialized.
         */
      nsTAString(PRUint32 flags)
        : mVTable(nsTObsoleteAString<char_type>::sCanonicalVTable)
        , mFlags(flags)
        {}

    public:
      
        /**
         * this is public to support automatic conversion of tuple to abstract
         * string, which is necessary to support our API.
         */
      nsTAString(const string_tuple_type& tuple)
        : mVTable(nsTObsoleteAString<char_type>::sCanonicalVTable)
        , mData(nsnull)
        , mLength(0)
        , mFlags(0)
        {
          Assign(tuple);
        }

    protected:

      //nsTAString( const self_type& readable );
        /*
        : mVTable(readable.mVTable)
        {}
        */

        /**
         * get pointer to internal string buffer (may not be null terminated).
         * return length of buffer.
         */
      size_type GetReadableBuffer( const char_type **data ) const;
      size_type GetWritableBuffer(       char_type **data );

        /**
         * returns true if this tuple is dependent on (i.e., overlapping with)
         * the given char sequence.
         */
      PRBool IsDependentOn(const char_type *start, const char_type *end) const;

        /**
         * we can be converted to a const nsTStringBase (dependent on this)
         */
      const string_base_type ToString() const
        {
          const char_type* data;
          size_type length = GetReadableBuffer(&data);
          return string_base_type(NS_CONST_CAST(char_type*, data), length, 0);
        }

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

      const string_base_type* AsString() const
        {
          return NS_REINTERPRET_CAST(const string_base_type*, this);
        }

      string_base_type* AsString()
        {
          return NS_REINTERPRET_CAST(string_base_type*, this);
        }
  };


template <class CharT>
NS_COM
int Compare( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs, const nsTStringComparator<CharT>& = nsTDefaultStringComparator<CharT>() );


template <class CharT>
inline
PRBool operator!=( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return !lhs.Equals(rhs);
  }

template <class CharT>
inline
PRBool operator< ( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return Compare(lhs, rhs)< 0;
  }

template <class CharT>
inline
PRBool operator<=( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return Compare(lhs, rhs)<=0;
  }

template <class CharT>
inline
PRBool operator==( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return lhs.Equals(rhs);
  }

template <class CharT>
inline
PRBool operator>=( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return Compare(lhs, rhs)>=0;
  }

template <class CharT>
inline
PRBool operator> ( const nsTAString<CharT>& lhs, const nsTAString<CharT>& rhs )
  {
    return Compare(lhs, rhs)> 0;
  }


#ifndef nsTStringTuple_h___
#include "nsTStringTuple.h"
#endif


#endif // !defined(nsTAString_h___)
