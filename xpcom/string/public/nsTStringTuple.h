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
   * nsTStringTuple_CharT
   *
   * Represents a tuple of string fragments.  Built as a recursive binary tree.
   * It is used to implement the concatenation of two or more string objects.
   *
   * NOTE: This class is a private implementation detail and should never be 
   * referenced outside the string code.
   */
class nsTStringTuple_CharT
  {
    public:

      typedef CharT                      char_type;
      typedef nsCharTraits<char_type>    char_traits;

      typedef nsTStringTuple_CharT       self_type;
      typedef nsTStringBase_CharT        string_base_type;
      typedef nsTString_CharT            string_type;
      typedef nsTAString_CharT           abstract_string_type;

      typedef PRUint32                   size_type;

    public:

      nsTStringTuple_CharT(const void* a, const void* b)
        : mHead(nsnull)
        , mFragA(a)
        , mFragB(b) {}

      nsTStringTuple_CharT(const self_type& head, const void* frag)
        : mHead(&head)
        , mFragA(nsnull) // this fragment is ignored when head != nsnull
        , mFragB(frag) {}

        /**
         * computes the aggregate string length
         */
      NS_COM size_type Length() const;

        /**
         * writes the aggregate string to the given buffer.  bufLen is assumed
         * to be equal to or greater than the value returned by the Length()
         * method.  the string written to |buf| is not null-terminated.
         */
      NS_COM void WriteTo(char_type *buf, PRUint32 bufLen) const;

        /**
         * returns true if this tuple is dependent on (i.e., overlapping with)
         * the given char sequence.
         */
      NS_COM PRBool IsDependentOn(const char_type *start, const char_type *end) const;

    private:

      const self_type* mHead;
      const void*      mFragA;
      const void*      mFragB;

      // type of mFrag? is given by the least significant bit.  if set, the
      // type is nsTAString_CharT, else it is nsTStringBase_CharT.
  };


#define NS_FLAG_READABLE(_r) ((void*)( ((unsigned long) _r) | 0x1 ))

inline
const nsTStringTuple_CharT
operator+(const nsTStringBase_CharT& a, const nsTStringBase_CharT& b)
  {
    return nsTStringTuple_CharT(&a, &b);
  }

inline
const nsTStringTuple_CharT
operator+(const nsTStringTuple_CharT& tuple, const nsTStringBase_CharT& str)
  {
    return nsTStringTuple_CharT(tuple, &str);
  }

inline
const nsTStringTuple_CharT
operator+(const nsTAString_CharT& a, const nsTAString_CharT& b)
  {
    return nsTStringTuple_CharT(NS_FLAG_READABLE(&a), NS_FLAG_READABLE(&b));
  }

inline
const nsTStringTuple_CharT
operator+(const nsTStringTuple_CharT& tuple, const nsTAString_CharT& readable)
  {
    return nsTStringTuple_CharT(tuple, NS_FLAG_READABLE(&readable));
  }

inline
const nsTStringTuple_CharT
operator+(const nsTStringBase_CharT& str, const nsTAString_CharT& readable)
  {
    return nsTStringTuple_CharT(&str, NS_FLAG_READABLE(&readable));
  }

inline
const nsTStringTuple_CharT
operator+(const nsTAString_CharT& readable, const nsTStringBase_CharT& str)
  {
    return nsTStringTuple_CharT(NS_FLAG_READABLE(&readable), &str);
  }

#undef NS_FLAG_READABLE
