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

#include "nsTStringFragment.h"
#include "nsAlgorithm.h"

// ---------------------------------------------------------------------------

static PRUnichar gNullChar = 0;

const char*      nsCharTraits<char>     ::sEmptyBuffer = (const char*) &gNullChar;
const PRUnichar* nsCharTraits<PRUnichar>::sEmptyBuffer =               &gNullChar;

// ---------------------------------------------------------------------------

template <class CharT>
void
nsTStringFragment<CharT>::InitFromAString( const abstract_string_type& readable )
  {
    mVTable = nsTObsoleteAString<char_type>::sCanonicalVTable;

    if (readable.mVTable == mVTable)
      {
        *this = readable.AsStringFragment();
      }
    else
      {
        mLength = readable.GetReadableBuffer((const char_type**) &mData);

        // test if string is flat...
        if (readable.AsObsoleteString().GetFlatBufferHandle())
          mFlags = F_TERMINATED;
        else
          mFlags = 0;
      }
  }

template <class CharT>
PRBool
nsTStringFragment<CharT>::Equals( const self_type& frag ) const
  {
    return mLength == frag.mLength && char_traits::compare(mData, frag.mData, mLength) == 0;
  }

template <class CharT>
PRBool
nsTStringFragment<CharT>::Equals( const self_type& frag, const comparator_type& comp ) const
  {
    return mLength == frag.mLength && comp(mData, frag.mData, mLength) == 0;
  }

template <class CharT>
typename
nsTStringFragment<CharT>::size_type
nsTStringFragment<CharT>::CountChar( char_type c ) const
  {
    const char_type *start = mData;
    const char_type *end   = mData + mLength;

    return NS_COUNT(start, end, c);
  }

template <class CharT>
PRInt32
nsTStringFragment<CharT>::FindChar( char_type c, index_type offset ) const
  {
    if (offset < mLength)
      {
        const char_type* result = char_traits::find(mData + offset, mLength, c);
        if (result)
          return result - mData;
      }
    return -1;
  }


  /**
   * explicit template instantiation
   */

template void     nsTStringFragment<char>::InitFromAString( const abstract_string_type& );
template PRBool   nsTStringFragment<char>::Equals         ( const self_type& ) const;
template PRBool   nsTStringFragment<char>::Equals         ( const self_type&, const comparator_type& ) const;
template PRUint32 nsTStringFragment<char>::CountChar      ( char_type ) const;
template PRInt32  nsTStringFragment<char>::FindChar       ( char_type, index_type ) const;

template void     nsTStringFragment<PRUnichar>::InitFromAString( const abstract_string_type& );
template PRBool   nsTStringFragment<PRUnichar>::Equals         ( const self_type& ) const;
template PRBool   nsTStringFragment<PRUnichar>::Equals         ( const self_type&, const comparator_type& ) const;
template PRUint32 nsTStringFragment<PRUnichar>::CountChar      ( char_type ) const;
template PRInt32  nsTStringFragment<PRUnichar>::FindChar       ( char_type, index_type ) const;
