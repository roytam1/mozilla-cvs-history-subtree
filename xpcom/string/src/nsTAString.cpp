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

#include "nsTAString.h"
#include "nsTObsoleteAString.h"
#include "nsTString.h"

  /**
   * Some comments on this implementation...
   *
   * This class is a bridge between the old string implementation and the new
   * string implementation.  If mVTable points to the canonical vtable for the
   * new string implementation, then we can cast directly to the new string
   * classes (helped by the AsString() methods).  However, if mVTable is not
   * ours, then we need to call through the vtable to satisfy the nsTAString
   * methods.
   *
   * In most cases we will avoid the vtable.
   */

  // MSVC6 does not support template instantiation of destructors
NS_SPECIALIZE_TEMPLATE
nsTAString<char>::~nsTAString()
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->ReleaseData();
    else
      AsObsoleteString()->~nsTObsoleteAString();
  }
NS_SPECIALIZE_TEMPLATE
nsTAString<PRUnichar>::~nsTAString()
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->ReleaseData();
    else
      AsObsoleteString()->~nsTObsoleteAString();
  }

template <class CharT>
typename
nsTAString<CharT>::size_type
nsTAString<CharT>::Length() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Length();

    return AsObsoleteString()->Length();
  }

template <class CharT>
PRBool
nsTAString<CharT>::Equals( const self_type& readable ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(readable);

    return ToString().Equals(readable);
  }

template <class CharT>
PRBool
nsTAString<CharT>::Equals( const self_type& readable, const comparator_type& comparator ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(readable, comparator);

    return ToString().Equals(readable, comparator);
  }

template <class CharT>
PRBool
nsTAString<CharT>::Equals( const char_type* data ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(data);

    return ToString().Equals(data);
  }

template <class CharT>
PRBool
nsTAString<CharT>::Equals( const char_type* data, const comparator_type& comparator ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(data, comparator);

    return ToString().Equals(data, comparator);
  }

template <class CharT>
PRBool
nsTAString<CharT>::IsVoid() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsVoid();

    return AsObsoleteString()->IsVoid();
  }

template <class CharT>
void
nsTAString<CharT>::SetIsVoid( PRBool val )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetIsVoid(val);
    else
      AsObsoleteString()->SetIsVoid(val);
  }

template <class CharT>
PRBool
nsTAString<CharT>::IsTerminated() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsTerminated();

    return AsObsoleteString()->GetFlatBufferHandle() != nsnull;
  }

template <class CharT>
CharT
nsTAString<CharT>::First() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->First();

    return ToString().First();
  }

template <class CharT>
CharT
nsTAString<CharT>::Last() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Last();

    return ToString().Last();
  }

template <class CharT>
typename
nsTAString<CharT>::size_type
nsTAString<CharT>::CountChar( char_type c ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->CountChar(c);

    return ToString().CountChar(c);
  }

template <class CharT>
PRInt32
nsTAString<CharT>::FindChar( char_type c, index_type offset ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->FindChar(c, offset);

    return ToString().FindChar(c, offset);
  }

template <class CharT>
void
nsTAString<CharT>::SetCapacity( size_type size )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetCapacity(size);
    else
      AsObsoleteString()->SetCapacity(size);
  }

template <class CharT>
void
nsTAString<CharT>::SetLength( size_type size )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetLength(size);
    else
      AsObsoleteString()->SetLength(size);
  }

template <class CharT>
void
nsTAString<CharT>::Assign( const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(readable);
    else
      AsObsoleteString()->do_AssignFromReadable(*readable.AsObsoleteString());
  }

template <class CharT>
void
nsTAString<CharT>::Assign( const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(tuple);
    else
      AsObsoleteString()->do_AssignFromReadable(*nsTAutoString<char_type>(tuple).AsObsoleteString());
  }

template <class CharT>
void
nsTAString<CharT>::Assign( const char_type* data )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(data);
    else // XXX null check input arg??
      AsObsoleteString()->do_AssignFromElementPtr(data);
  }

template <class CharT>
void
nsTAString<CharT>::Assign( const char_type* data, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(data, length);
    else
      AsObsoleteString()->do_AssignFromElementPtrLength(data, length);
  }

template <class CharT>
void
nsTAString<CharT>::Assign( char_type c )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(c);
    else
      AsObsoleteString()->do_AssignFromElement(c);
  }

template <class CharT>
void
nsTAString<CharT>::Append( const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(readable);
    else
      AsObsoleteString()->do_AppendFromReadable(*readable.AsObsoleteString());
  }

template <class CharT>
void
nsTAString<CharT>::Append( const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(tuple);
    else
      AsObsoleteString()->do_AppendFromReadable(*nsTAutoString<char_type>(tuple).AsObsoleteString());
  }

template <class CharT>
void
nsTAString<CharT>::Append( const char_type* data )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(data);
    else // XXX null check data??
      AsObsoleteString()->do_AppendFromElementPtr(data);
  }

template <class CharT>
void
nsTAString<CharT>::Append( const char_type* data, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(data, length);
    else
      AsObsoleteString()->do_AppendFromElementPtrLength(data, length);
  }

template <class CharT>
void
nsTAString<CharT>::Append( char_type c )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(c);
    else
      AsObsoleteString()->do_AppendFromElement(c);
  }

template <class CharT>
void
nsTAString<CharT>::Insert( const self_type& readable, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(readable, pos);
    else
      AsObsoleteString()->do_InsertFromReadable(*readable.AsObsoleteString(), pos);
  }

template <class CharT>
void
nsTAString<CharT>::Insert( const string_tuple_type& tuple, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(tuple, pos);
    else
      AsObsoleteString()->do_InsertFromReadable(*nsTAutoString<char_type>(tuple).AsObsoleteString(), pos);
  }

template <class CharT>
void
nsTAString<CharT>::Insert( const char_type* data, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(data, pos);
    else
      AsObsoleteString()->do_InsertFromElementPtr(data, pos);
  }

template <class CharT>
void
nsTAString<CharT>::Insert( const char_type* data, index_type pos, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(data, pos, length);
    else
      AsObsoleteString()->do_InsertFromElementPtrLength(data, pos, length);
  }

template <class CharT>
void
nsTAString<CharT>::Insert( char_type c, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(c, pos);
    else
      AsObsoleteString()->do_InsertFromElement(c, pos);
  }

template <class CharT>
void
nsTAString<CharT>::Cut( index_type cutStart, size_type cutLength )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Cut(cutStart, cutLength);
    else
      AsObsoleteString()->Cut(cutStart, cutLength);
  }

template <class CharT>
void
nsTAString<CharT>::Replace( index_type cutStart, size_type cutLength, const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Replace(cutStart, cutLength, readable);
    else
      AsObsoleteString()->do_ReplaceFromReadable(cutStart, cutLength, *readable.AsObsoleteString());
  }

template <class CharT>
void
nsTAString<CharT>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Replace(cutStart, cutLength, tuple);
    else
      AsObsoleteString()->do_ReplaceFromReadable(cutStart, cutLength, *nsTAutoString<char_type>(tuple).AsObsoleteString());
  }

template <class CharT>
typename
nsTAString<CharT>::size_type
nsTAString<CharT>::GetReadableBuffer( const char_type **data ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        const string_base_type* str = AsString();
        *data = str->mData;
        return str->mLength;
      }

    typename obsolete_string_type::const_fragment_type frag;
    AsObsoleteString()->GetReadableFragment(frag, kFirstFragment, 0);
    *data = frag.mStart;
    return (frag.mEnd - frag.mStart);
  }

template <class CharT>
typename
nsTAString<CharT>::size_type
nsTAString<CharT>::GetWritableBuffer(char_type **data)
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        string_base_type* str = AsString();
        str->BeginWriting(*data);
        return str->Length();
      }

    typename obsolete_string_type::fragment_type frag;
    AsObsoleteString()->GetWritableFragment(frag, kFirstFragment, 0);
    *data = frag.mStart;
    return (frag.mEnd - frag.mStart);
  }

template <class CharT>
PRBool
nsTAString<CharT>::IsDependentOn(const char_type* start, const char_type *end) const
  {
      // this is an optimization...
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsDependentOn(start, end);

    return ToString().IsDependentOn(start, end);
  }


  /**
   * explicit template instantiation
   */

template PRUint32  nsTAString<char>::Length() const;
template PRBool    nsTAString<char>::Equals( const self_type& readable ) const;
template PRBool    nsTAString<char>::Equals( const self_type& readable, const comparator_type& comparator ) const;
template PRBool    nsTAString<char>::Equals( const char_type* data ) const;
template PRBool    nsTAString<char>::Equals( const char_type* data, const comparator_type& comparator ) const;
template PRBool    nsTAString<char>::IsVoid() const;
template void      nsTAString<char>::SetIsVoid( PRBool val );
template PRBool    nsTAString<char>::IsTerminated() const;
template char      nsTAString<char>::First() const;
template char      nsTAString<char>::Last() const;
template PRUint32  nsTAString<char>::CountChar( char_type c ) const;
template PRInt32   nsTAString<char>::FindChar( char_type c, index_type offset ) const;
template void      nsTAString<char>::SetCapacity( size_type size );
template void      nsTAString<char>::SetLength( size_type size );
template void      nsTAString<char>::Assign( const self_type& readable );
template void      nsTAString<char>::Assign( const string_tuple_type& tuple );
template void      nsTAString<char>::Assign( const char_type* data );
template void      nsTAString<char>::Assign( const char_type* data, size_type length );
template void      nsTAString<char>::Assign( char_type c );
template void      nsTAString<char>::Append( const self_type& readable );
template void      nsTAString<char>::Append( const string_tuple_type& tuple );
template void      nsTAString<char>::Append( const char_type* data );
template void      nsTAString<char>::Append( const char_type* data, size_type length );
template void      nsTAString<char>::Append( char_type c );
template void      nsTAString<char>::Insert( const self_type& readable, index_type pos );
template void      nsTAString<char>::Insert( const string_tuple_type& tuple, index_type pos );
template void      nsTAString<char>::Insert( const char_type* data, index_type pos );
template void      nsTAString<char>::Insert( const char_type* data, index_type pos, size_type length );
template void      nsTAString<char>::Insert( char_type c, index_type pos );
template void      nsTAString<char>::Cut( index_type cutStart, size_type cutLength );
template void      nsTAString<char>::Replace( index_type cutStart, size_type cutLength, const self_type& readable );
template void      nsTAString<char>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple );
template PRUint32  nsTAString<char>::GetReadableBuffer( const char_type **data ) const;
template PRUint32  nsTAString<char>::GetWritableBuffer(char_type **data);
template PRBool    nsTAString<char>::IsDependentOn(const char_type* start, const char_type *end) const;

template PRUint32  nsTAString<PRUnichar>::Length() const;
template PRBool    nsTAString<PRUnichar>::Equals( const self_type& readable ) const;
template PRBool    nsTAString<PRUnichar>::Equals( const self_type& readable, const comparator_type& comparator ) const;
template PRBool    nsTAString<PRUnichar>::Equals( const char_type* data ) const;
template PRBool    nsTAString<PRUnichar>::Equals( const char_type* data, const comparator_type& comparator ) const;
template PRBool    nsTAString<PRUnichar>::IsVoid() const;
template void      nsTAString<PRUnichar>::SetIsVoid( PRBool val );
template PRBool    nsTAString<PRUnichar>::IsTerminated() const;
template PRUnichar nsTAString<PRUnichar>::First() const;
template PRUnichar nsTAString<PRUnichar>::Last() const;
template PRUint32  nsTAString<PRUnichar>::CountChar( char_type c ) const;
template PRInt32   nsTAString<PRUnichar>::FindChar( char_type c, index_type offset ) const;
template void      nsTAString<PRUnichar>::SetCapacity( size_type size );
template void      nsTAString<PRUnichar>::SetLength( size_type size );
template void      nsTAString<PRUnichar>::Assign( const self_type& readable );
template void      nsTAString<PRUnichar>::Assign( const string_tuple_type& tuple );
template void      nsTAString<PRUnichar>::Assign( const char_type* data );
template void      nsTAString<PRUnichar>::Assign( const char_type* data, size_type length );
template void      nsTAString<PRUnichar>::Assign( char_type c );
template void      nsTAString<PRUnichar>::Append( const self_type& readable );
template void      nsTAString<PRUnichar>::Append( const string_tuple_type& tuple );
template void      nsTAString<PRUnichar>::Append( const char_type* data );
template void      nsTAString<PRUnichar>::Append( const char_type* data, size_type length );
template void      nsTAString<PRUnichar>::Append( char_type c );
template void      nsTAString<PRUnichar>::Insert( const self_type& readable, index_type pos );
template void      nsTAString<PRUnichar>::Insert( const string_tuple_type& tuple, index_type pos );
template void      nsTAString<PRUnichar>::Insert( const char_type* data, index_type pos );
template void      nsTAString<PRUnichar>::Insert( const char_type* data, index_type pos, size_type length );
template void      nsTAString<PRUnichar>::Insert( char_type c, index_type pos );
template void      nsTAString<PRUnichar>::Cut( index_type cutStart, size_type cutLength );
template void      nsTAString<PRUnichar>::Replace( index_type cutStart, size_type cutLength, const self_type& readable );
template void      nsTAString<PRUnichar>::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple );
template PRUint32  nsTAString<PRUnichar>::GetReadableBuffer( const char_type **data ) const;
template PRUint32  nsTAString<PRUnichar>::GetWritableBuffer(char_type **data);
template PRBool    nsTAString<PRUnichar>::IsDependentOn(const char_type* start, const char_type *end) const;
