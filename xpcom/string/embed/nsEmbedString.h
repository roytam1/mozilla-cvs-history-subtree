/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is a small implementation of the nsAString and nsACString.
 *
 * The Initial Developer of the Original Code is
 *   Peter Annema <jaggernaut@netscape.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
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

#ifndef nsEmbedString_h___
#define nsEmbedString_h___

#include "nsStringAPI.h"

class nsEmbedString
  {
    protected:
      nsStringContainer mImpl;

    public:
      typedef PRUnichar        char_type;
      typedef nsEmbedString    self_type;
      typedef nsAString        abstract_string_type;
      typedef PRUint32         size_type;
      typedef PRUint32         index_type;
    
      nsEmbedString()
        {
          NS_StringContainerInit(mImpl);
        }

      nsEmbedString(const self_type& aString)
        {
          NS_StringContainerInit(mImpl);
          NS_StringCopy(mImpl, aString);
        }

      explicit
      nsEmbedString(const abstract_string_type& aReadable)
        {
          NS_StringContainerInit(mImpl);
          NS_StringCopy(mImpl, aReadable);
        }

      explicit
      nsEmbedString(const char_type* aData, size_type aLength = PR_UINT32_MAX)
        {
          //NS_StringContainerInit(mImpl, aData, aLength);
          NS_StringContainerInit(mImpl);
          NS_StringSetData(mImpl, aData, aLength);
        }
      
      ~nsEmbedString()
        {
          NS_StringContainerFinish(mImpl);
        }

      operator const abstract_string_type&() const
        {
          return mImpl;
        }

      operator abstract_string_type&()
        {
          return mImpl;
        }
      
      const char_type* get() const
        {
          return NS_StringGetDataPtr(mImpl);
        }

      size_type Length() const
        {
          return NS_StringGetLength(mImpl);
        }
      
      void Assign(const self_type& aString)
        {
          NS_StringCopy(mImpl, aString);
        }

      void Assign(const abstract_string_type& aReadable)
        {
          NS_StringCopy(mImpl, aReadable);
        }

      void Assign(const char_type* aData, size_type aLength = PR_UINT32_MAX)
        {
          NS_StringSetData(mImpl, aData, aLength);
        }

      void Assign(char_type aChar)
        {
          NS_StringSetData(mImpl, &aChar, 1);
        }
      
      self_type& operator=(const self_type& aString)              { Assign(aString);   return *this; }
      self_type& operator=(const abstract_string_type& aReadable) { Assign(aReadable); return *this; }
      self_type& operator=(const char_type* aPtr)                 { Assign(aPtr);      return *this; }
      self_type& operator=(char_type aChar)                       { Assign(aChar);     return *this; }
  };

class nsEmbedCString
  {
    protected:
      nsCStringContainer mImpl;

    public:
      typedef char             char_type;
      typedef nsEmbedCString   self_type;
      typedef nsACString       abstract_string_type;
      typedef PRUint32         size_type;
      typedef PRUint32         index_type;
    
      nsEmbedCString()
        {
          NS_CStringContainerInit(mImpl);
        }

      nsEmbedCString(const self_type& aString)
        {
          NS_CStringContainerInit(mImpl);
          NS_CStringCopy(mImpl, aString);
        }

      explicit
      nsEmbedCString(const abstract_string_type& aReadable)
        {
          NS_CStringContainerInit(mImpl);
          NS_CStringCopy(mImpl, aReadable);
        }

      explicit
      nsEmbedCString(const char_type* aData, size_type aLength = PR_UINT32_MAX)
        {
          //NS_CStringContainerInit(mImpl, aData, aLength);
          NS_CStringContainerInit(mImpl);
          NS_CStringSetData(mImpl, aData, aLength);
        }
      
      ~nsEmbedCString()
        {
          NS_CStringContainerFinish(mImpl);
        }

      operator const abstract_string_type&() const
        {
          return mImpl;
        }

      operator abstract_string_type&()
        {
          return mImpl;
        }
      
      const char_type* get() const
        {
          return NS_CStringGetDataPtr(mImpl);
        }

      size_type Length() const
        {
          return NS_CStringGetLength(mImpl);
        }
      
      void Assign(const self_type& aString)
        {
          NS_CStringCopy(mImpl, aString);
        }

      void Assign(const abstract_string_type& aReadable)
        {
          NS_CStringCopy(mImpl, aReadable);
        }

      void Assign(const char_type* aData, size_type aLength = PR_UINT32_MAX)
        {
          NS_CStringSetData(mImpl, aData, aLength);
        }

      void Assign(char_type aChar)
        {
          NS_CStringSetData(mImpl, &aChar, 1);
        }
      
      self_type& operator=(const self_type& aString)              { Assign(aString);   return *this; }
      self_type& operator=(const abstract_string_type& aReadable) { Assign(aReadable); return *this; }
      self_type& operator=(const char_type* aPtr)                 { Assign(aPtr);      return *this; }
      self_type& operator=(char_type aChar)                       { Assign(aChar);     return *this; }
  };

#endif // !defined(nsEmbedString_h___)
