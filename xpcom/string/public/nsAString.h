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

#ifndef nsAString_h___
#define nsAString_h___

#ifndef nsStringFwd_h___
#include "nsStringFwd.h"
#endif

#ifndef nsStringIterator_h___
#include "nsStringIterator.h"
#endif

#ifndef nsObsoleteAString_h___
#include "nsObsoleteAString.h"
#endif


  // declare nsAString
#include "string-template-def-unichar.h"
#include "nsTAString.h"
#include "string-template-undef.h"


  // declare nsACString
#include "string-template-def-char.h"
#include "nsTAString.h"
#include "string-template-undef.h"


  /**
   * ASCII case-insensitive comparator.  (for Unicode case-insensitive
   * comparision, see nsUnicharUtils.h)
   */
class NS_COM nsCaseInsensitiveCStringComparator
    : public nsCStringComparator
  {
    public:
      typedef char char_type;

      virtual int operator()( const char_type*, const char_type*, PRUint32 length ) const;
      virtual int operator()( char_type, char_type ) const;
  };


  // included here for backwards compatibility
#ifndef nsStringTuple_h___
#include "nsStringTuple.h"
#endif

#endif // !defined(nsAString_h___)
