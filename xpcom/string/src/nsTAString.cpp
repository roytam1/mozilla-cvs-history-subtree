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


nsTAString_CharT::~nsTAString_CharT()
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->ReleaseData();
    else
      AsObsoleteString()->~nsTObsoleteAString_CharT();
  }


nsTAString_CharT::size_type
nsTAString_CharT::Length() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Length();

    return AsObsoleteString()->Length();
  }

PRBool
nsTAString_CharT::Equals( const self_type& readable ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(readable);

    return ToString().Equals(readable);
  }

PRBool
nsTAString_CharT::Equals( const self_type& readable, const comparator_type& comparator ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(readable, comparator);

    return ToString().Equals(readable, comparator);
  }

PRBool
nsTAString_CharT::Equals( const char_type* data ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(data);

    return ToString().Equals(data);
  }

PRBool
nsTAString_CharT::Equals( const char_type* data, const comparator_type& comparator ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Equals(data, comparator);

    return ToString().Equals(data, comparator);
  }

PRBool
nsTAString_CharT::IsVoid() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsVoid();

    return AsObsoleteString()->IsVoid();
  }

void
nsTAString_CharT::SetIsVoid( PRBool val )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetIsVoid(val);
    else
      AsObsoleteString()->SetIsVoid(val);
  }

PRBool
nsTAString_CharT::IsTerminated() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsTerminated();

    return AsObsoleteString()->GetFlatBufferHandle() != nsnull;
  }

CharT
nsTAString_CharT::First() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->First();

    return ToString().First();
  }

CharT
nsTAString_CharT::Last() const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->Last();

    return ToString().Last();
  }

nsTAString_CharT::size_type
nsTAString_CharT::CountChar( char_type c ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->CountChar(c);

    return ToString().CountChar(c);
  }

PRInt32
nsTAString_CharT::FindChar( char_type c, index_type offset ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->FindChar(c, offset);

    return ToString().FindChar(c, offset);
  }

void
nsTAString_CharT::SetCapacity( size_type size )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetCapacity(size);
    else
      AsObsoleteString()->SetCapacity(size);
  }

void
nsTAString_CharT::SetLength( size_type size )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->SetLength(size);
    else
      AsObsoleteString()->SetLength(size);
  }

void
nsTAString_CharT::Assign( const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(readable);
    else
      AsObsoleteString()->do_AssignFromReadable(*readable.AsObsoleteString());
  }

void
nsTAString_CharT::Assign( const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        AsString()->Assign(tuple);
      }
    else
      {
        // XXX MSVC refuses access to AsObsoleteString on a subclass of nsTAString_CharT :-(
        const nsTAString_CharT& temp = nsTAutoString_CharT(tuple);
        AsObsoleteString()->do_AssignFromReadable(*temp.AsObsoleteString());
      }
  }

void
nsTAString_CharT::Assign( const char_type* data )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(data);
    else // XXX null check input arg??
      AsObsoleteString()->do_AssignFromElementPtr(data);
  }

void
nsTAString_CharT::Assign( const char_type* data, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(data, length);
    else
      AsObsoleteString()->do_AssignFromElementPtrLength(data, length);
  }

void
nsTAString_CharT::Assign( char_type c )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Assign(c);
    else
      AsObsoleteString()->do_AssignFromElement(c);
  }

void
nsTAString_CharT::Append( const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(readable);
    else
      AsObsoleteString()->do_AppendFromReadable(*readable.AsObsoleteString());
  }

void
nsTAString_CharT::Append( const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        AsString()->Append(tuple);
      }
    else
      {
        // XXX MSVC refuses access to AsObsoleteString on a subclass of nsTAString_CharT :-(
        const nsTAString_CharT& temp = nsTAutoString_CharT(tuple);
        AsObsoleteString()->do_AppendFromReadable(*temp.AsObsoleteString());
      }
  }

void
nsTAString_CharT::Append( const char_type* data )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(data);
    else // XXX null check data??
      AsObsoleteString()->do_AppendFromElementPtr(data);
  }

void
nsTAString_CharT::Append( const char_type* data, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(data, length);
    else
      AsObsoleteString()->do_AppendFromElementPtrLength(data, length);
  }

void
nsTAString_CharT::Append( char_type c )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Append(c);
    else
      AsObsoleteString()->do_AppendFromElement(c);
  }

void
nsTAString_CharT::Insert( const self_type& readable, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(readable, pos);
    else
      AsObsoleteString()->do_InsertFromReadable(*readable.AsObsoleteString(), pos);
  }

void
nsTAString_CharT::Insert( const string_tuple_type& tuple, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        AsString()->Insert(tuple, pos);
      }
    else
      {
        // XXX MSVC refuses access to AsObsoleteString on a subclass of nsTAString_CharT :-(
        const nsTAString_CharT& temp = nsTAutoString_CharT(tuple);
        AsObsoleteString()->do_InsertFromReadable(*temp.AsObsoleteString(), pos);
      }
  }

void
nsTAString_CharT::Insert( const char_type* data, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(data, pos);
    else
      AsObsoleteString()->do_InsertFromElementPtr(data, pos);
  }

void
nsTAString_CharT::Insert( const char_type* data, index_type pos, size_type length )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(data, pos, length);
    else
      AsObsoleteString()->do_InsertFromElementPtrLength(data, pos, length);
  }

void
nsTAString_CharT::Insert( char_type c, index_type pos )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Insert(c, pos);
    else
      AsObsoleteString()->do_InsertFromElement(c, pos);
  }

void
nsTAString_CharT::Cut( index_type cutStart, size_type cutLength )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Cut(cutStart, cutLength);
    else
      AsObsoleteString()->Cut(cutStart, cutLength);
  }

void
nsTAString_CharT::Replace( index_type cutStart, size_type cutLength, const self_type& readable )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      AsString()->Replace(cutStart, cutLength, readable);
    else
      AsObsoleteString()->do_ReplaceFromReadable(cutStart, cutLength, *readable.AsObsoleteString());
  }

void
nsTAString_CharT::Replace( index_type cutStart, size_type cutLength, const string_tuple_type& tuple )
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        AsString()->Replace(cutStart, cutLength, tuple);
      }
    else
      {
        // XXX MSVC refuses access to AsObsoleteString on a subclass of nsTAString_CharT :-(
        const nsTAString_CharT& temp = nsTAutoString_CharT(tuple);
        AsObsoleteString()->do_ReplaceFromReadable(cutStart, cutLength, *temp.AsObsoleteString());
      }
  }

nsTAString_CharT::size_type
nsTAString_CharT::GetReadableBuffer( const char_type **data ) const
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        const string_base_type* str = AsString();
        *data = str->mData;
        return str->mLength;
      }

    obsolete_string_type::const_fragment_type frag;
    AsObsoleteString()->GetReadableFragment(frag, obsolete_string_type::kFirstFragment, 0);
    *data = frag.mStart;
    return (frag.mEnd - frag.mStart);
  }

nsTAString_CharT::size_type
nsTAString_CharT::GetWritableBuffer(char_type **data)
  {
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      {
        string_base_type* str = AsString();
        str->BeginWriting(*data);
        return str->Length();
      }

    obsolete_string_type::fragment_type frag;
    AsObsoleteString()->GetWritableFragment(frag, obsolete_string_type::kFirstFragment, 0);
    *data = frag.mStart;
    return (frag.mEnd - frag.mStart);
  }

PRBool
nsTAString_CharT::IsDependentOn(const char_type* start, const char_type *end) const
  {
      // this is an optimization...
    if (mVTable == obsolete_string_type::sCanonicalVTable)
      return AsString()->IsDependentOn(start, end);

    return ToString().IsDependentOn(start, end);
  }
