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

#ifndef nsTStringTuple_h___
#define nsTStringTuple_h___

#ifndef nsTStringBase_h___
#include "nsTStringBase.h"
#endif

  /**
   * nsTStringTuple
   *
   * Represents a tuple of string fragments.  Built as a recursive binary tree.
   */
template <class CharT>
class NS_COM nsTStringTuple
  {
    public:

      typedef CharT                                              char_type;

      typedef nsTStringTuple<char_type>                          self_type;
      typedef nsTStringBase<char_type>                           string_base_type;
      typedef nsTString<char_type>                               string_type;

      typedef typename string_base_type::abstract_string_type    abstract_string_type;
      typedef typename string_base_type::char_traits             char_traits;
      typedef typename string_base_type::size_type               size_type;

    public:

      nsTStringTuple(const void* a, const void* b)
        : mHead(nsnull)
        , mFragA(a)
        , mFragB(b) {}

      nsTStringTuple(const self_type& head, const void* frag)
        : mHead(&head)
        , mFragA(nsnull) // this fragment is ignored when head != nsnull
        , mFragB(frag) {}

        /**
         * computes the aggregate string length
         */
      size_type Length() const;

        /**
         * writes the aggregate string to the given buffer.  bufLen is assumed
         * to be equal to or greater than the value returned by the Length()
         * method.  the string written to |buf| is not null-terminated.
         */
      void WriteTo(char_type *buf, PRUint32 bufLen) const;

        /**
         * returns true if this tuple is dependent on (i.e., overlapping with)
         * the given char sequence.
         */
      PRBool IsDependentOn(const char_type *start, const char_type *end) const;

        /**
         * allow automatic flattening (XXX would be better to fix callsites to avoid this)
         * but, at least our string type uses a shared buffer :)
         */
//XXX      operator const string_type() const { return string_type(*this); }

    private:

      const self_type* mHead;
      const void*      mFragA;
      const void*      mFragB;

      // type of mFrag? is given by the low-order bit.  if set, the type
      // is nsTAString, else it is nsTStringBase.
  };


#define NS_FLAG_READABLE(_r) ((void*)( ((unsigned long) _r) | 0x1 ))

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTStringBase<CharT>& a, const nsTStringBase<CharT>& b)
  {
    return nsTStringTuple<CharT>(&a, &b);
  }

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTStringTuple<CharT>& tuple, const nsTStringBase<CharT>& str)
  {
    return nsTStringTuple<CharT>(tuple, &str);
  }

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTAString<CharT>& a, const nsTAString<CharT>& b)
  {
    return nsTStringTuple<CharT>(NS_FLAG_READABLE(&a), NS_FLAG_READABLE(&b));
  }

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTStringTuple<CharT>& tuple, const nsTAString<CharT>& readable)
  {
    return nsTStringTuple<CharT>(tuple, NS_FLAG_READABLE(&readable));
  }

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTStringBase<CharT>& str, const nsTAString<CharT>& readable)
  {
    return nsTStringTuple<CharT>(&str, NS_FLAG_READABLE(&readable));
  }

template <class CharT>
inline
const nsTStringTuple<CharT>
operator+(const nsTAString<CharT>& readable, const nsTStringBase<CharT>& str)
  {
    return nsTStringTuple<CharT>(NS_FLAG_READABLE(&readable), &str);
  }

#undef NS_FLAG_READABLE

#endif // !defined(nsTStringTuple_h___)
