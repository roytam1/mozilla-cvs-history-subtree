/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#ifndef nsLiteralString_h___
#define nsLiteralString_h___

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsLocalString_h___
#include "nsLocalString.h"
#endif

typedef const nsLocalString       nsLiteralString;
typedef const nsLocalCString      nsLiteralCString;

#ifndef nsStringTraits_h___
#include "nsStringTraits.h"
#endif

template <class CharT>
inline
const typename nsStringTraits<CharT>::literal_string_type
literal_string( const CharT* aPtr )
  {
    return nsStringTraits<CharT>::literal_string_type(aPtr);
  }

template <class CharT>
inline
const typename nsStringTraits<CharT>::literal_string_type
literal_string( const CharT* aPtr, PRUint32 aLength )
  {
    return nsStringTraits<CharT>::literal_string_type(aPtr, aLength);
  }

#ifdef HAVE_CPP_2BYTE_WCHAR_T
  #define NS_L(s)                                 L##s
  #define NS_MULTILINE_LITERAL_STRING(s)          nsLiteralString(s, (sizeof(s)/sizeof(wchar_t))-1)
  #define NS_NAMED_MULTILINE_LITERAL_STRING(n,s)  nsLiteralString n(s, (sizeof(s)/sizeof(wchar_t))-1)
#else
  #define NS_L(s)                                 s
  #define NS_MULTILINE_LITERAL_STRING(s)          NS_ConvertASCIItoUCS2(s, sizeof(s)-1)
  #define NS_NAMED_MULTILINE_LITERAL_STRING(n,s)  NS_ConvertASCIItoUCS2 n(s, sizeof(s)-1)
#endif

#define NS_LITERAL_STRING(s)                      NS_MULTILINE_LITERAL_STRING(NS_L(s))
#define NS_NAMED_LITERAL_STRING(n,s)              NS_NAMED_MULTILINE_LITERAL_STRING(n,NS_L(s))

#define NS_LITERAL_CSTRING(s)                     nsLiteralCString(s, sizeof(s)-1)
#define NS_NAMED_LITERAL_CSTRING(n,s)             nsLiteralCString n(s, sizeof(s)-1)

#endif /* !defined(nsLiteralString_h___) */
