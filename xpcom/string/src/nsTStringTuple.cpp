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

#include "nsTStringTuple.h"

#define TO_STRING(_v)                                           \
    ( (ptrdiff_t(_v) & 0x1)                                     \
        ? NS_REINTERPRET_CAST(const abstract_string_type*,      \
            ((unsigned long)_v & ~0x1))->ToString()             \
        : *NS_REINTERPRET_CAST(const string_base_type*, (_v)) )


  /**
   * computes the aggregate string length
   */

template <class CharT>
typename
nsTStringTuple<CharT>::size_type
nsTStringTuple<CharT>::Length() const
  {
    // fragments are enumerated right to left

    const string_base_type& b = TO_STRING(mFragB);

    PRUint32 len = b.Length();
    if (mHead)
      {
        len += mHead->Length();
      }
    else
      {
        const string_base_type& a = TO_STRING(mFragA);
        len += a.Length();
      }
    return len;
  }


  /**
   * writes the aggregate string to the given buffer.  bufLen is assumed
   * to be equal to or greater than the value returned by the Length()
   * method.  the string written to |buf| is not null-terminated.
   */

template <class CharT>
void
nsTStringTuple<CharT>::WriteTo( char_type *buf, PRUint32 bufLen ) const
  {
    // we need to write out data into buf, ending at end.  so our data
    // needs to preceed |end| exactly.  we trust that the buffer was
    // properly sized!

    const string_base_type& b = TO_STRING(mFragB);

    char_traits::copy(buf + bufLen - b.Length(), b.Data(), b.Length());

    bufLen -= b.Length();

    if (mHead)
      {
        mHead->WriteTo(buf, bufLen);
      }
    else
      {
        const string_base_type& a = TO_STRING(mFragA);
        char_traits::copy(buf + bufLen - a.Length(), a.Data(), a.Length());
      }
  }


  /**
   * returns true if this tuple is dependent on (i.e., overlapping with)
   * the given char sequence.
   */

template <class CharT>
PRBool
nsTStringTuple<CharT>::IsDependentOn( const char_type *start, const char_type *end ) const
  {
    // fragments are enumerated right to left

    const string_base_type& b = TO_STRING(mFragB);

    PRBool dependent = b.IsDependentOn(start, end);
    if (!dependent)
    {
      if (mHead)
        {
          dependent = mHead->IsDependentOn(start, end);
        }
      else
        {
          const string_base_type& a = TO_STRING(mFragA);
          dependent = a.IsDependentOn(start ,end);
        }
    }
    return dependent;
  }


  /**
   * explicit template instantiation
   */

template PRUint32 nsTStringTuple<char>::Length() const;
template void     nsTStringTuple<char>::WriteTo( char_type *buf, PRUint32 bufLen ) const;
template PRBool   nsTStringTuple<char>::IsDependentOn( const char_type *start, const char_type *end ) const;

template PRUint32 nsTStringTuple<PRUnichar>::Length() const;
template void     nsTStringTuple<PRUnichar>::WriteTo( char_type *buf, PRUint32 bufLen ) const;
template PRBool   nsTStringTuple<PRUnichar>::IsDependentOn( const char_type *start, const char_type *end ) const;
