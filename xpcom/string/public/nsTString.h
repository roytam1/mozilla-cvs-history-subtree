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

#ifndef nsTString_h___
#define nsTString_h___

#ifndef nsTStringBase_h___
#include "nsTStringBase.h"
#endif

#ifndef nsTDependentSubstring_h___
#include "nsTDependentSubstring.h"
#endif

#ifndef nsReadableUtils_h___
#include "nsReadableUtils.h"
#endif


  /**
   * This is the canonical null-terminated string class.  All subclasses
   * promise null-terminated storage.  Instances of this class allocate
   * strings on the heap.
   */
template <class CharT>
class nsTString : public nsTStringBase<CharT>
  {
    public:

      typedef CharT                                              char_type;

      typedef nsTString<char_type>                               self_type;
      typedef nsTStringBase<char_type>                           string_base_type;

      typedef typename string_base_type::string_tuple_type       string_tuple_type;
      typedef typename string_base_type::abstract_string_type    abstract_string_type;
      typedef typename string_base_type::size_type               size_type;

    public:

        /**
         * constructors
         */

      nsTString()
        : string_base_type() {}

      explicit
      nsTString( char_type c )
        : string_base_type()
        {
          Assign(c);
        }

      explicit
      nsTString( const char_type* data, size_type length = size_type(-1) )
        : string_base_type()
        {
          Assign(data, length);
        }

      nsTString( const self_type& str )
        : string_base_type()
        {
          Assign(str);
        }

      nsTString( const string_tuple_type& tuple )
        : string_base_type()
        {
          Assign(tuple);
        }

      explicit
      nsTString( const abstract_string_type& readable )
        : string_base_type()
        {
          Assign(readable);
        }

        // |operator=| does not inherit, so we must define our own
      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const string_base_type& str )                                       { Assign(str);      return *this; }
      self_type& operator=( const string_tuple_type& tuple )                                    { Assign(tuple);    return *this; }
      self_type& operator=( const abstract_string_type& readable )                              { Assign(readable); return *this; }

        // returns null-terminated string
      const char_type* get() const
        {
          return mData;
        }

    protected:

      nsTString( PRUint32 flags )
        : string_base_type(flags) {}

        // allow subclasses to initialize fields directly
      nsTString( char_type* data, size_type length, PRUint32 flags )
        : string_base_type(data, length, flags) {}
  };


  /**
   * CBufDescriptor
   *
   * Allows a nsTAutoString to be configured to use a custom buffer.
   */
class CBufDescriptor
  {
    public:

      CBufDescriptor(char* aString, PRBool aStackBased, PRUint32 aCapacity, PRInt32 aLength=-1)
        {
          mStr = aString;
          mCapacity = aCapacity;
          mLength = aLength;
          mFlags = F_SINGLE_BYTE | (aStackBased ? F_STACK_BASED : 0);
        }

      CBufDescriptor(const char* aString, PRBool aStackBased, PRUint32 aCapacity, PRInt32 aLength=-1)
        {
          mStr = NS_CONST_CAST(char*, aString);
          mCapacity = aCapacity;
          mLength = aLength;
          mFlags = F_SINGLE_BYTE | F_CONST | (aStackBased ? F_STACK_BASED : 0);
        }

      CBufDescriptor(PRUnichar* aString, PRBool aStackBased, PRUint32 aCapacity, PRInt32 aLength=-1)
        {
          mUStr = aString;
          mCapacity = aCapacity;
          mLength = aLength;
          mFlags = F_DOUBLE_BYTE | (aStackBased ? F_STACK_BASED : 0);
        }

      CBufDescriptor(const PRUnichar* aString, PRBool aStackBased, PRUint32 aCapacity, PRInt32 aLength=-1)
        {
          mUStr = NS_CONST_CAST(PRUnichar*, aString);
          mCapacity = aCapacity;
          mLength = aLength;
          mFlags = F_DOUBLE_BYTE | F_CONST | (aStackBased ? F_STACK_BASED : 0);
        }

      union
        {
          char*      mStr;
          PRUnichar* mUStr;
        };

      PRUint32  mCapacity;
      PRInt32   mLength;

      enum
        {
          F_CONST       = (1 << 0),
          F_STACK_BASED = (1 << 1),
          F_SINGLE_BYTE = (1 << 2),
          F_DOUBLE_BYTE = (1 << 3)
        };

      PRUint32  mFlags;
  };


  /**
   * nsTAutoString
   *
   * Subclass of nsTString that adds support for stack-based string allocation.
   * Do not allocate this class on the heap! ;-)
   */
template <class CharT>
class nsTAutoString : public nsTString<CharT>
  {
    public:

      typedef CharT                                         char_type;

      typedef nsTAutoString<char_type>                      self_type;
      typedef nsTString<char_type>                          string_type;

      typedef typename string_type::string_base_type        string_base_type;
      typedef typename string_type::string_tuple_type       string_tuple_type;
      typedef typename string_type::abstract_string_type    abstract_string_type;
      typedef typename string_type::size_type               size_type;

    public:

        /**
         * constructors
         */

      nsTAutoString()
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          mFixedBuf[0] = char_type(0);
        }

      explicit
      nsTAutoString( char_type c )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(c);
        }

      explicit
      nsTAutoString( const char_type* data, size_type length = size_type(-1) )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(data, length);
        }

      nsTAutoString( const self_type& str )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(str);
        }

      explicit
      nsTAutoString( const string_base_type& str )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(str);
        }

      nsTAutoString( const string_tuple_type& tuple )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(tuple);
        }

      explicit
      nsTAutoString( const abstract_string_type& readable )
        : string_type(mFixedBuf, 0, F_TERMINATED | F_FIXED), mFixedCapacity(kDefaultStringSize - 1)
        {
          Assign(readable);
        }

      explicit
      nsTAutoString( const CBufDescriptor& aBufDesc )
        : string_type(PRUint32(F_TERMINATED))
        {
          Init(aBufDesc);
        }

        // |operator=| does not inherit, so we must define our own
      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const string_base_type& str )                                       { Assign(str);      return *this; }
      self_type& operator=( const string_tuple_type& tuple )                                    { Assign(tuple);    return *this; }
      self_type& operator=( const abstract_string_type& readable )                              { Assign(readable); return *this; }

      enum { kDefaultStringSize = 64 };

    private:

      friend class nsTStringBase<char_type>;

      void Init( const CBufDescriptor& aBufDesc );

      size_type mFixedCapacity;
      char_type mFixedBuf[kDefaultStringSize];
  };


template <class CharT>
class nsTXPIDLString : public nsTString<CharT>
  {
    public:

      typedef CharT                        char_type;

      typedef nsTXPIDLString<char_type>    self_type;
      typedef nsTString<char_type>         string_type;

      typedef typename string_type::string_base_type        string_base_type;
      typedef typename string_type::string_tuple_type       string_tuple_type;
      typedef typename string_type::abstract_string_type    abstract_string_type;
      typedef typename string_type::size_type               size_type;
      typedef typename string_type::index_type              index_type;

    public:

      nsTXPIDLString()
        : string_type(nsnull, 0, 0) {}

        // copy-constructor required to avoid default
      nsTXPIDLString( const self_type& str )
        : string_type(str) {}

        // this case operator is the reason why this class cannot just be a
        // typedef for nsTString
      operator const char_type*() const
        {
          return get();
        }

        // need this to diambiguous operator[int]
      char_type operator[]( PRInt32 i ) const
        {
          return CharAt(index_type(i));
        }

        // |operator=| does not inherit, so we must define our own
      self_type& operator=( char_type c )                                                       { Assign(c);        return *this; }
      self_type& operator=( const char_type* data )                                             { Assign(data);     return *this; }
      self_type& operator=( const self_type& str )                                              { Assign(str);      return *this; }
      self_type& operator=( const string_base_type& str )                                       { Assign(str);      return *this; }
      self_type& operator=( const string_tuple_type& tuple )                                    { Assign(tuple);    return *this; }
      self_type& operator=( const abstract_string_type& readable )                              { Assign(readable); return *this; }
  };

  /**
   * getter_Copies support for use with raw string out params:
   *
   *    NS_IMETHOD GetBlah(char**);
   *    
   *    void some_function()
   *    {
   *      nsTString<char> blah;
   *      GetBlah(getter_Copies(blah));
   *      // ...
   *    }
   */
template <class CharT>
class getter_Copies_t
  {
    public:
      typedef CharT char_type;

      getter_Copies_t(nsTXPIDLString<CharT>& str)
        : mString(str), mData(nsnull) {}

      ~getter_Copies_t()
        {
          mString.Adopt(mData); // OK if mData is null
        }

      operator char_type**()
        {
          return &mData;
        }

    private:
      nsTXPIDLString<char_type>& mString;
      char_type*                 mData;
  };

template <class CharT>
inline
getter_Copies_t<CharT>
getter_Copies( nsTXPIDLString<CharT>& aString )
  {
    return getter_Copies_t<CharT>(aString);
  }


  /**
   * A helper class that converts a UTF-16 string to ASCII in a lossy manner
   */
class NS_COM NS_LossyConvertUTF16toASCII : public nsCAutoString
  {
    public:
      explicit
      NS_LossyConvertUTF16toASCII( const PRUnichar* aString )
        {
          LossyAppendUTF16toASCII(aString, *this);
        }

      NS_LossyConvertUTF16toASCII( const PRUnichar* aString, PRUint32 aLength )
        {
          LossyCopyUTF16toASCII(nsDependentSubstring(aString, aString + aLength), *this);
        }

      explicit
      NS_LossyConvertUTF16toASCII( const nsAString& aString )
        {
          LossyCopyUTF16toASCII(aString, *this);
        }

    private:
        // NOT TO BE IMPLEMENTED
      NS_LossyConvertUTF16toASCII( char );
  };


class NS_COM NS_ConvertASCIItoUTF16 : public nsAutoString
  {
    public:
      explicit
      NS_ConvertASCIItoUTF16( const char* aCString )
        {
          AppendASCIItoUTF16(aCString, *this);
        }

      NS_ConvertASCIItoUTF16( const char* aCString, PRUint32 aLength )
        {
          CopyASCIItoUTF16(nsDependentCSubstring(aCString, aCString + aLength), *this);
        }

      explicit
      NS_ConvertASCIItoUTF16( const nsACString& aCString )
        {
          CopyASCIItoUTF16(aCString, *this);
        }

    private:
        // NOT TO BE IMPLEMENTED
      NS_ConvertASCIItoUTF16( PRUnichar );
  };


  /**
   * A helper class that converts a UTF-16 string to UTF-8
   */
class NS_COM NS_ConvertUTF16toUTF8 : public nsCAutoString
  {
    public:
      explicit
      NS_ConvertUTF16toUTF8( const PRUnichar* aString )
        {
          CopyUTF16toUTF8(aString, *this);
        }

      NS_ConvertUTF16toUTF8( const PRUnichar* aString, PRUint32 aLength )
        {
          CopyUTF16toUTF8(nsDependentSubstring(aString, aString + aLength), *this);
        }

      explicit
      NS_ConvertUTF16toUTF8( const nsAString& aString )
        {
          CopyUTF16toUTF8(aString, *this);
        }

    private:
        // NOT TO BE IMPLEMENTED
      NS_ConvertUTF16toUTF8( char );
  };


class NS_COM NS_ConvertUTF8toUTF16 : public nsAutoString
  {
    public:
      explicit
      NS_ConvertUTF8toUTF16( const char* aCString )
        {
          CopyUTF8toUTF16(aCString, *this);
        }

      NS_ConvertUTF8toUTF16( const char* aCString, PRUint32 aLength )
        {
          CopyUTF8toUTF16(nsDependentCSubstring(aCString, aCString + aLength), *this);
        }

      explicit
      NS_ConvertUTF8toUTF16( const nsACString& aCString )
        {
          CopyUTF8toUTF16(aCString, *this);
        }

    private:
      NS_ConvertUTF8toUTF16( PRUnichar );
  };

// Backward compatibility
typedef NS_ConvertUTF16toUTF8 NS_ConvertUCS2toUTF8;
typedef NS_LossyConvertUTF16toASCII NS_LossyConvertUCS2toASCII;
typedef NS_ConvertASCIItoUTF16 NS_ConvertASCIItoUCS2;
typedef NS_ConvertUTF8toUTF16 NS_ConvertUTF8toUCS2;
typedef nsAutoString nsVoidableString;

#endif // !defined(nsTString_h___)
