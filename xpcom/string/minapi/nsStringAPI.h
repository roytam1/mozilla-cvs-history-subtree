/* vim:set ts=2 sw=2 et cindent: */
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
 * IBM Corporation.  All Rights Reserved.
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

#ifndef nsStringAPI_h__
#define nsStringAPI_h__

/**
 * nsStringAPI.h
 *
 * This file describes a minimal API for working with XPCOM's abstract
 * string classes.  It divorces the consumer from having any run-time
 * dependency on the implementation details of the abstract string types.
 */

#include "nsStringFwd.h"

#define NS_STRINGAPI(x) extern "C" NS_COM x

/* ------------------------------------------------------------------------- */

/**
 * nsStringContainer
 *
 * This is an opaque data type that is large enough to hold the canonical 
 * implementation of nsAString.  The binary structure of this class is an
 * implementation detail.
 *
 * The string data stored in a string container is always single fragment
 * and null-terminated.
 *
 * Typically, string containers are allocated on the stack for temporary
 * use.  However, they can also be malloc'd if necessary.  In either case,
 * a string container is not useful until it has been initialized with a
 * call to NS_StringContainerInit.  The following example shows how to use
 * a string container to call a function that takes a |nsAString &| out-param.
 *
 *   NS_METHOD GetBlah(nsAString &aBlah);
 *
 *   void MyCode()
 *   {
 *     nsStringContainer sc;
 *     if (NS_StringContainerInit(sc))
 *     {
 *       nsresult rv = GetBlah(sc);
 *       if (NS_SUCCEEDED(rv))
 *       {
 *         const PRUnichar *data = NS_StringGetDataPtr(sc);
 *         //
 *         // |data| now points to the result of the GetBlah function
 *         //
 *       }
 *       NS_StringContainerFinish(sc);
 *     }
 *   }
 *
 * The following example show how to use a string container to pass a string
 * parameter to a function taking a |const nsAString &| in-param.
 *
 *   NS_METHOD SetBlah(const nsAString &aBlah);
 *
 *   void MyCode()
 *   {
 *     nsStringContainer sc;
 *     if (NS_StringContainerInit(sc))
 *     {
 *       const PRUnichar kData[] = {'x','y','z','\0'};
 *       NS_StringSetData(sc, kData, sizeof(kData)-1);
 *
 *       SetBlah(sc);
 *
 *       NS_StringContainerFinish(sc);
 *     }
 *   }
 */
class nsStringContainer
{
  private:
    void    *d1,*d2;
    PRUint32 d3, d4;

  public:
    /**
     * A nsStringContainer can be automatically cast to a nsAString.
     */
    operator const nsAString&() const
      { return *NS_REINTERPRET_CAST(const nsAString *, this); }
    operator nsAString&()
      { return *NS_REINTERPRET_CAST(nsAString *, this); }
};

/**
 * NS_StringContainerInit
 *
 * @param aContainer    string container reference
 * @return              true if string container successfully initialized
 *
 * This function may allocate additional memory for aContainer.  When
 * aContainer is no longer needed, NS_StringContainerFinish should be called.
 */
NS_STRINGAPI(PRBool)
NS_StringContainerInit(nsStringContainer &aContainer);

/**
 * NS_StringContainerFinish
 * 
 * @param aContainer    string container reference
 *
 * This function frees any memory owned by aContainer.
 */
NS_STRINGAPI(void)
NS_StringContainerFinish(nsStringContainer &aContainer);

/* ------------------------------------------------------------------------- */

/**
 * NS_StringGetLength
 *
 * @param aStr          abstract string reference
 * @return              length of string (number of storage units excluding
 *                      terminating null character)
 */
NS_STRINGAPI(PRUint32)
NS_StringGetLength(const nsAString &aStr);

/**
 * NS_StringGetDataPtr
 *
 * @param aStr          abstract string reference
 * @return              pointer to null-terminated string data or NULL if string
 *                      is not single fragment and null-terminated
 *
 * If this function returns NULL, then the string data should be read by
 * calling NS_StringGetData.  If aStr is a reference to a nsStringContainer,
 * then this function will never return NULL.
 */
NS_STRINGAPI(const PRUnichar *)
NS_StringGetDataPtr(const nsAString &aStr);

/**
 * NS_StringGetData
 *
 * @param aStr          abstract string reference
 * @param aBuf          character buffer
 * @param aBufLen       number of characters that can be written to aBuf,
 *                      including space for the null-terminator.
 * @return              number of characters written to aBuf
 *
 * This function null-terminates aBuf, provided aBufLen > 0.
 */
NS_STRINGAPI(PRUint32)
NS_StringGetData(const nsAString &aStr, PRUnichar *aBuf, PRUint32 aBufLen);

/**
 * NS_StringSetData
 *
 * @param aStr          abstract string reference
 * @param aBuf          character buffer
 * @param aCount        number of characters to copy from source string (pass
 *                      PR_UINT32_MAX to copy until end of aSrc, designated by
 *                      a null character)
 *
 * This function does not necessarily null-terminate aStr after copying data
 * from aBuf.  The behavior depends on the implementation of the abstract 
 * string, aStr.  If aStr is a reference to a nsStringContainer, then its data
 * will be null-terminated by this function.
 */
NS_STRINGAPI(void)
NS_StringSetData(nsAString &aStr, const PRUnichar *aBuf,
                 PRUint32 aCount = PR_UINT32_MAX);

/**
 * NS_StringCopy
 *
 * @param aDest         abstract string reference to be modified
 * @param aSrc          abstract string reference containing source string
 * @param aOffset       offset into source string from which to start copying
 * @param aCount        number of characters to copy from source string (pass
 *                      PR_UINT32_MAX to copy until end of aSrc)
 *
 * This function does not necessarily null-terminate aDest after copying data
 * from aSrc.  The behavior depends on the implementation of the abstract 
 * string, aDest.  If aDest is a reference to a nsStringContainer, then its data
 * will be null-terminated by this function.
 */
NS_STRINGAPI(void)
NS_StringCopy(nsAString &aDest, const nsAString &aSrc,
              PRUint32 aOffset = 0, PRUint32 aCount = PR_UINT32_MAX);

/* ------------------------------------------------------------------------- */

/**
 * nsCStringContainer
 *
 * This is an opaque data type that is large enough to hold the canonical 
 * implementation of nsACString.  The binary structure of this class is an
 * implementation detail.
 *
 * The string data stored in a string container is always single fragment
 * and null-terminated.
 *
 * @see nsStringContainer for use cases and further documentation.
 */
class nsCStringContainer
{
  private:
    void    *d1,*d2;
    PRUint32 d3, d4;

  public:
    /**
     * A nsStringContainer can be automatically cast to a nsAString.
     */
    operator const nsACString&() const
      { return *NS_REINTERPRET_CAST(const nsACString *, this); }
    operator nsACString&()
      { return *NS_REINTERPRET_CAST(nsACString *, this); }
};

/**
 * NS_CStringContainerInit
 *
 * @param aContainer    string container reference
 * @return              true if string container successfully initialized
 *
 * This function may allocate additional memory for aContainer.  When
 * aContainer is no longer needed, NS_CStringContainerFinish should be called.
 */
NS_STRINGAPI(PRBool)
NS_CStringContainerInit(nsCStringContainer &aContainer);

/**
 * NS_CStringContainerFinish
 * 
 * @param aContainer    string container reference
 *
 * This function frees any memory owned by aContainer.
 */
NS_STRINGAPI(void)
NS_CStringContainerFinish(nsCStringContainer &aContainer);

/* ------------------------------------------------------------------------- */

/**
 * NS_CStringGetLength
 *
 * @param aStr          abstract string reference
 * @return              length of string (number of storage units excluding
 *                      terminating null character)
 */
NS_STRINGAPI(PRUint32)
NS_CStringGetLength(const nsACString &aStr);

/**
 * NS_CStringGetDataPtr
 *
 * @param aStr          abstract string reference
 * @return              pointer to null-terminated string data or NULL if string
 *                      is not single fragment and null-terminated
 *
 * If this function returns NULL, then the string data should be read by
 * calling NS_CStringGetData.  If aStr is a reference to a nsCStringContainer,
 * then this function will never return NULL.
 */
NS_STRINGAPI(const char *)
NS_CStringGetDataPtr(const nsACString &aStr);

/**
 * NS_CStringGetData
 *
 * @param aStr          abstract string reference
 * @param aBuf          character buffer
 * @param aBufLen       number of characters that can be written to aBuf,
 *                      including space for the null-terminator.
 * @return              number of characters written to aBuf
 *
 * This function null-terminates aBuf, provided aBufLen > 0.
 */
NS_STRINGAPI(PRUint32)
NS_CStringGetData(const nsACString &aStr, char *aBuf, PRUint32 aBufLen);

/**
 * NS_CStringSetData
 *
 * @param aStr          abstract string reference
 * @param aBuf          character buffer
 * @param aCount        number of characters to copy from source string (pass
 *                      PR_UINT32_MAX to copy until end of aSrc, designated by
 *                      a null character)
 *
 * This function does not necessarily null-terminate aStr after copying data
 * from aBuf.  The behavior depends on the implementation of the abstract 
 * string, aStr.  If aStr is a reference to a nsCStringContainer, then its data
 * will be null-terminated by this function.
 */
NS_STRINGAPI(void)
NS_CStringSetData(nsACString &aStr, const char *aBuf,
                  PRUint32 aCount = PR_UINT32_MAX);

/**
 * NS_CStringCopy
 *
 * @param aDest         abstract string reference to be modified
 * @param aSrc          abstract string reference containing source string
 * @param aOffset       offset into source string from which to start copying
 * @param aCount        number of characters to copy from source string (pass
 *                      PR_UINT32_MAX to copy until end of aSrc)
 *
 * This function does not necessarily null-terminate aDest after copying data
 * from aSrc.  The behavior depends on the implementation of the abstract 
 * string, aDest.  If aDest is a reference to a nsCStringContainer, then its
 * data will be null-terminated by this function.
 */
NS_STRINGAPI(void)
NS_CStringCopy(nsACString &aDest, const nsACString &aSrc,
               PRUint32 aOffset = 0, PRUint32 aCount = PR_UINT32_MAX);

#endif // nsStringAPI_h__
