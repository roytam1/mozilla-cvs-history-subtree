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

#ifndef nsPrivateSharableString_h___
#include "nsPrivateSharableString.h"
#endif

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
#endif


  /**
   *
   */

class nsAString
    : public nsPrivateSharableString<PRUnichar>
  {
    public:
      typedef PRUint32                      size_type;

      typedef nsReadingIterator<PRUnichar>  const_iterator;
      typedef nsWritingIterator<PRUnichar>  iterator;

      // nsAString();                           // auto-generated default constructor OK (we're abstract anyway)
      // nsAString( const nsAString<CharT>& );  // auto-generated copy-constructor OK (again, only because we're abstract)
      virtual ~nsAString() { }                  // ...yes, I expect to be sub-classed

      const_iterator& BeginReading( const_iterator& ) const;
      const_iterator& EndReading( const_iterator& ) const;

      iterator& BeginWriting( iterator& );
      iterator& EndWriting( iterator& );

      virtual size_type Length() const = 0;
      PRBool IsEmpty() const;

      
      // ...
  };


  /**
   *
   */

class nsACString
    : public nsPrivateSharableString<char>
  {
    public:
      typedef PRUint32                      size_type;

      typedef nsReadingIterator<PRUnichar>  const_iterator;
      typedef nsWritingIterator<PRUnichar>  iterator;

      // nsACString();                            // auto-generated default constructor OK (we're abstract anyway)
      // nsACString( const nsACString<CharT>& );  // auto-generated copy-constructor OK (again, only because we're abstract)
      virtual ~nsACString() { }                   // ...yes, I expect to be sub-classed

      const_iterator& BeginReading( const_iterator& ) const;
      const_iterator& EndReading( const_iterator& ) const;

      iterator& BeginWriting( iterator& );
      iterator& EndWriting( iterator& );

      virtual size_type Length() const = 0;
      PRBool IsEmpty() const;

      
      // ...
  };

#endif // !defined(nsAString_h___)
