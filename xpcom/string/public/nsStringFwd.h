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

/* nsStringFwd.h --- forward declarations for string classes */

#ifndef nsStringFwd_h___
#define nsStringFwd_h___

#ifndef nsStringDefines_h___
#include "nsStringDefines.h"
#endif

#ifndef nscore_h___
#include "nscore.h"
#endif


  /**
   * template types:
   */

template <class CharT> class nsTAString;
template <class CharT> class nsTObsoleteAString;
template <class CharT> class nsTStringBase;
template <class CharT> class nsTStringTuple;
template <class CharT> class nsTString;
template <class CharT> class nsTAutoString;
template <class CharT> class nsTDependentString;
template <class CharT> class nsTDependentSubstring;
template <class CharT> class nsTPromiseFlatString;
template <class CharT> class nsTStringComparator;
template <class CharT> class nsTDefaultStringComparator;
template <class CharT> class nsTXPIDLString;

  
  /**
   * for everything else, we typedef :-)
   */

typedef nsTAString<char>                         nsACString;
typedef nsTStringBase<char>                      nsASingleFragmentCString;
typedef nsTString<char>                          nsAFlatCString;
typedef nsTStringTuple<char>                     nsDependentCConcatenation;
typedef nsTDependentSubstring<char>              nsDependentSingleFragmentCSubstring;
typedef nsTDependentSubstring<char>              nsDependentCSubstring;
typedef nsTDependentString<char>                 nsDependentCString;
typedef nsTPromiseFlatString<char>               nsPromiseFlatCString;
typedef nsTString<char>                          nsCString;
typedef nsTAutoString<char>                      nsCAutoString;
typedef nsTXPIDLString<char>                     nsXPIDLCString;
typedef nsTString<char>                          nsSharableCString;
typedef nsTStringComparator<char>                nsCStringComparator;
typedef nsTDefaultStringComparator<char>         nsDefaultCStringComparator;

typedef nsTAString<PRUnichar>                    nsAString;
typedef nsTStringBase<PRUnichar>                 nsASingleFragmentString;
typedef nsTString<PRUnichar>                     nsAFlatString;
typedef nsTStringTuple<PRUnichar>                nsDependentConcatenation;
typedef nsTDependentSubstring<PRUnichar>         nsDependentSingleFragmentSubstring;
typedef nsTDependentSubstring<PRUnichar>         nsDependentSubstring;
typedef nsTDependentString<PRUnichar>            nsDependentString;
typedef nsTPromiseFlatString<PRUnichar>          nsPromiseFlatString;
typedef nsTString<PRUnichar>                     nsString;
typedef nsTAutoString<PRUnichar>                 nsAutoString;
typedef nsTXPIDLString<PRUnichar>                nsXPIDLString;
typedef nsTString<PRUnichar>                     nsSharableString;
typedef nsTStringComparator<PRUnichar>           nsStringComparator;
typedef nsTDefaultStringComparator<PRUnichar>    nsDefaultStringComparator;

#endif /* !defined(nsStringFwd_h___) */
