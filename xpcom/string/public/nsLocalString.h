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

#ifndef nsLocalString_h___
#define nsLocalString_h___

#ifndef nsAFlatString_h___
#include "nsAFlatString.h"
#endif

    /*
      ...this class wraps a constant literal string and lets it act like an |nsAReadable...|.
      
      Use it like this:

        SomeFunctionTakingACString( nsLiteralCString("Hello, World!") );

      With some tweaking, I think I can make this work as well...

        SomeStringFunc( nsLiteralString( L"Hello, World!" ) );

      This class just holds a pointer.  If you don't supply the length, it must calculate it.
      No copying or allocations are performed.

      |const nsLocalString&| appears frequently in interfaces because it
      allows the automatic conversion of a |PRUnichar*|.
    */

class NS_COM nsLocalString
      : public nsAFlatString
  {
    protected:
      virtual const PRUnichar* GetReadableFragment( nsReadableFragment<PRUnichar>&, nsFragmentRequest, PRUint32 ) const;
      virtual       PRUnichar* GetWritableFragment( nsWritableFragment<PRUnichar>&, nsFragmentRequest, PRUint32 ) { }

    public:
    
      explicit
      nsLocalString( const PRUnichar* aLiteral )
          : mStart(aLiteral),
            mEnd(mStart ? (mStart + nsCharTraits<PRUnichar>::length(mStart)) : mStart)
        {
          // nothing else to do here
        }

      nsLocalString( const PRUnichar* aLiteral, PRUint32 aLength )
          : mStart(aLiteral),
            mEnd(mStart + aLength)
        {
            // This is an annoying hack.  Callers should be fixed to use the other
            //  constructor if they don't really know the length.
          if ( aLength == PRUint32(-1) )
            {
//            NS_WARNING("Tell scc: Caller constructing a string doesn't know the real length.  Please use the other constructor.");
              mEnd = mStart ? (mStart + nsCharTraits<PRUnichar>::length(mStart)) : mStart;
            }
        }

      // nsLocalString( const nsLocalString& );  // auto-generated copy-constructor OK
      // ~nsLocalString();                       // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsLocalString& );    // we're immutable

    public:

      virtual PRUint32 Length() const;
      virtual void SetLength( size_type ) { }

    private:
      const PRUnichar* mStart;
      const PRUnichar* mEnd;
  };



class NS_COM nsLocalCString
      : public nsAFlatCString
  {
    protected:
      virtual const char* GetReadableFragment( nsReadableFragment<char>&, nsFragmentRequest, PRUint32 ) const;
      virtual       char* GetWritableFragment( nsWritableFragment<char>&, nsFragmentRequest, PRUint32 ) { }

    public:
    
      explicit
      nsLocalCString( const char* aLiteral )
          : mStart(aLiteral),
            mEnd(mStart ? (mStart + nsCharTraits<char>::length(mStart)) : mStart)
        {
          // nothing else to do here
        }

      nsLocalCString( const char* aLiteral, PRUint32 aLength )
          : mStart(aLiteral),
            mEnd(mStart + aLength)
        {
            // This is an annoying hack.  Callers should be fixed to use the other
            //  constructor if they don't really know the length.
          if ( aLength == PRUint32(-1) )
            {
//            NS_WARNING("Tell scc: Caller constructing a string doesn't know the real length.  Please use the other constructor.");
              mEnd = mStart ? (mStart + nsCharTraits<char>::length(mStart)) : mStart;
            }
        }

      // nsLocalCString( const nsLocalCString& );   // auto-generated copy-constructor OK
      // ~nsLocalCString();                         // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsLocalCString& );      // we're immutable

    public:

      virtual PRUint32 Length() const;
      virtual void SetLength( size_type ) { }

    private:
      const char* mStart;
      const char* mEnd;
  };

#endif /* !defined(nsLocalString_h___) */
