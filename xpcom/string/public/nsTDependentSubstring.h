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

#ifndef nsTDependentSubstring_h___
#define nsTDependentSubstring_h___

#ifndef nsTStringBase_h___
#include "nsTStringBase.h"
#endif


  /**
   * nsTDependentSubstring
   */
template <class CharT>
class nsTDependentSubstring : public nsTStringBase<CharT>
  {
    public:

      typedef CharT                                              char_type;

      typedef nsTDependentSubstring<char_type>                   self_type;
      typedef nsTStringBase<char_type>                           string_base_type;

      typedef typename string_base_type::abstract_string_type    abstract_string_type;
      typedef typename string_base_type::const_iterator          const_iterator;
      typedef typename string_base_type::size_type               size_type;

    public:

      void Rebind( const abstract_string_type&, PRUint32 startPos, PRUint32 length = size_type(-1) );
      void Rebind( const string_base_type&, PRUint32 startPos, PRUint32 length = size_type(-1) );

      void Rebind( const char_type* start, const char_type* end )
        {
          NS_ASSERTION(start && end, "nsTDependentSubstring must wrap a non-NULL buffer");
          mData = start;
          mLength = end - start;
        }

      nsTDependentSubstring( const abstract_string_type& str, PRUint32 startPos, PRUint32 length = size_type(-1) )
        : string_base_type(F_NONE)
        {
          Rebind(str, startPos, length);
        }

      nsTDependentSubstring( const string_base_type& str, PRUint32 startPos, PRUint32 length = size_type(-1) )
        : string_base_type(F_NONE)
        {
          Rebind(str, startPos, length);
        }

      nsTDependentSubstring( const char_type* start, const char_type* end )
        : string_base_type(NS_CONST_CAST(char_type*, start), end - start, F_NONE) {}

      nsTDependentSubstring( const const_iterator& start, const const_iterator& end )
        : string_base_type(NS_CONST_CAST(char_type*, start.get()), end.get() - start.get(), F_NONE) {}

      // auto-generated copy-constructor OK (XXX really?? what about base class copy-ctor?)

    private:
        // NOT USED
      void operator=( const self_type& ) {}        // we're immutable, you can't assign into a substring
  };

template <class CharT>
const nsTDependentSubstring<CharT>
Substring( const nsTAString<CharT>& str, PRUint32 startPos, PRUint32 length = PRUint32(-1) )
  {
    return nsTDependentSubstring<CharT>(str, startPos, length);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
Substring( const nsTStringBase<CharT>& str, PRUint32 startPos, PRUint32 length = PRUint32(-1) )
  {
    return nsTDependentSubstring<CharT>(str, startPos, length);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
Substring( const nsReadingIterator<CharT>& start, const nsReadingIterator<CharT>& end )
  {
    return nsTDependentSubstring<CharT>(start.get(), end.get());
  }

template <class CharT>
const nsTDependentSubstring<CharT>
Substring( const CharT* start, const CharT* end )
  {
    return nsTDependentSubstring<CharT>(start, end);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
StringHead( const nsTAString<CharT>& str, PRUint32 count )
  {
    return nsTDependentSubstring<CharT>(str, 0, count);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
StringHead( const nsTStringBase<CharT>& str, PRUint32 count )
  {
    return nsTDependentSubstring<CharT>(str, 0, count);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
StringTail( const nsTAString<CharT>& str, PRUint32 count )
  {
    return nsTDependentSubstring<CharT>(str, str.Length() - count, count);
  }

template <class CharT>
const nsTDependentSubstring<CharT>
StringTail( const nsTStringBase<CharT>& str, PRUint32 count )
  {
    return nsTDependentSubstring<CharT>(str, str.Length() - count, count);
  }

#endif // !defined(nsTDependentSubstring_h___)
