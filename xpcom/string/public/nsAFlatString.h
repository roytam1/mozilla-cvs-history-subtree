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

#ifndef nsAFlatString_h___
#define nsAFlatString_h___

#ifndef nsAString_h___
#include "nsAString.h"
#endif

class nsAFlatString
    : public nsAString
  {
    public:
      const PRUnichar* get() const              { return GetBufferHandle()->DataStart(); }
      PRUnichar  operator[]( PRUint32 i ) const { return get()[ i ]; }
      PRUnichar  CharAt( PRUint32 ) const;
  };

class nsAFlatCString
    : public nsACString
  {
    public:
      const char* get() const                   { return GetBufferHandle()->DataStart(); }
      char  operator[]( PRUint32 i ) const      { return get()[ i ]; }
      char  CharAt( PRUint32 ) const;
  };

inline
PRUnichar
nsAFlatString::CharAt( PRUint32 i ) const
  {
    NS_ASSERTION(i<Length(), "|CharAt| out-of-range");
    return operator[](i);
  }

inline
char
nsAFlatCString::CharAt( PRUint32 i ) const
  {
    NS_ASSERTION(i<Length(), "|CharAt| out-of-range");
    return operator[](i);
  }



#endif /* !defined(nsAFlatString_h___) */
