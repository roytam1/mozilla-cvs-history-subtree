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

#ifndef nsPromiseFlatString_h___
#define nsPromiseFlatString_h___

#ifndef nsString_h___
#include "nsString.h"
#endif

  // declare nsPromiseFlatString
#include "string-template-def-unichar.h"
#include "nsTPromiseFlatString.h"
#include "string-template-undef.h"

  // declare nsPromiseFlatCString
#include "string-template-def-char.h"
#include "nsTPromiseFlatString.h"
#include "string-template-undef.h"


inline
const nsPromiseFlatString
PromiseFlatString( const nsAString& str )
  {
    return nsPromiseFlatString(str);
  }

  // e.g., PromiseFlatString(Substring(s))
inline
const nsPromiseFlatString
PromiseFlatString( const nsStringBase& frag )
  {
    return nsPromiseFlatString(frag);
  }

  // e.g., PromiseFlatString(a + b)
inline
const nsPromiseFlatString
PromiseFlatString( const nsStringTuple& tuple )
  {
    return nsPromiseFlatString(tuple);
  }


inline
const nsPromiseFlatCString
PromiseFlatCString( const nsACString& str )
  {
    return nsPromiseFlatCString(str);
  }

  // e.g., PromiseFlatCString(Substring(s))
inline
const nsPromiseFlatCString
PromiseFlatCString( const nsCStringBase& frag )
  {
    return nsPromiseFlatCString(frag);
  }

  // e.g., PromiseFlatCString(a + b)
inline
const nsPromiseFlatCString
PromiseFlatCString( const nsCStringTuple& tuple )
  {
    return nsPromiseFlatCString(tuple);
  }

#endif /* !defined(nsPromiseFlatString_h___) */
