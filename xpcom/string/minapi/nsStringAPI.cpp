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

#include "nsString.h"
#include "nsString2.h"
#include "nsCharTraits.h"

#define nsAString_external nsAString_external_
#define nsACString_external nsACString_external_

#include "nsStringAPI.h"

/* ------------------------------------------------------------------------- */

NS_STRINGAPI(PRBool)
NS_StringContainerInit(nsStringContainer &aContainer)
{
  NS_ASSERTION(sizeof(nsStringContainer) >= sizeof(nsString),
      "nsStringContainer is not large enough");

  // use placement new to avoid heap allocating nsString object
  new (&aContainer) nsString();

  return PR_TRUE;
}

NS_STRINGAPI(void)
NS_StringContainerFinish(nsStringContainer &aContainer)
{
  // call the nsString dtor
  NS_REINTERPRET_CAST(nsString *, &aContainer)->~nsString();
}

/* ------------------------------------------------------------------------- */

NS_STRINGAPI(PRUint32)
NS_StringGetLength(const nsAString &aStr)
{
  return aStr.Length();
}

NS_STRINGAPI(const PRUnichar *)
NS_StringGetDataPtr(const nsAString &aStr)
{
  if (aStr.IsTerminated())
  {
    const nsAFlatString &flat = NS_STATIC_CAST(const nsAFlatString &, aStr);
    return flat.get();
  }
  return nsnull;
}

NS_STRINGAPI(PRUint32)
NS_StringGetData(const nsAString &aStr, PRUnichar *aBuf, PRUint32 aBufLen)
{
  nsAString::const_iterator start, end;
  aStr.BeginReading(start);
  aStr.EndReading(end);
  PRUint32 result = Distance(start, end);
  if (result >= aBufLen)
  {
    end = start;
    end.advance(aBufLen - 1);
    result = aBufLen - 1;
  }
  *copy_string(start, end, aBuf) = PRUnichar(0);
  return result;
}

NS_STRINGAPI(void)
NS_StringSetData(nsAString &aStr, const PRUnichar *aBuf, PRUint32 aCount)
{
  if (aCount == PR_UINT32_MAX)
    aCount = (PRUint32) nsCharTraits<PRUnichar>::length(aBuf);

  aStr.Assign(aBuf, aCount);
}

NS_STRINGAPI(void)
NS_StringCopy(nsAString &aDest, const nsAString &aSrc, PRUint32 aOffset,
              PRUint32 aCount)
{
  if (aOffset == 0 && aCount == PR_UINT32_MAX)
    aDest.Assign(aSrc);
  else
    aDest.Assign(Substring(aSrc, aOffset, aCount));
}

/* ------------------------------------------------------------------------- */

NS_STRINGAPI(PRBool)
NS_CStringContainerInit(nsCStringContainer &aContainer)
{
  NS_ASSERTION(sizeof(nsCStringContainer) >= sizeof(nsCString),
      "nsCStringContainer is not large enough");

  // use placement new to avoid heap allocating nsCString object
  new (&aContainer) nsCString();

  return PR_TRUE;
}

NS_STRINGAPI(void)
NS_CStringContainerFinish(nsCStringContainer &aContainer)
{
  // call the nsCString dtor
  NS_REINTERPRET_CAST(nsCString *, &aContainer)->~nsCString();
}

/* ------------------------------------------------------------------------- */

NS_STRINGAPI(PRUint32)
NS_CStringGetLength(const nsACString &aStr)
{
  return aStr.Length();
}

NS_STRINGAPI(const char *)
NS_CStringGetDataPtr(const nsACString &aStr)
{
  if (aStr.IsTerminated())
  {
    const nsAFlatCString &flat = NS_STATIC_CAST(const nsAFlatCString &, aStr);
    return flat.get();
  }
  return nsnull;
}

NS_STRINGAPI(PRUint32)
NS_CStringGetData(const nsACString &aStr, char *aBuf, PRUint32 aBufLen)
{
  nsACString::const_iterator start, end;
  aStr.BeginReading(start);
  aStr.EndReading(end);
  PRUint32 result = Distance(start, end);
  if (result >= aBufLen)
  {
    end = start;
    end.advance(aBufLen - 1);
    result = aBufLen - 1;
  }
  *copy_string(start, end, aBuf) = char(0);
  return result;
}

NS_STRINGAPI(void)
NS_CStringSetData(nsACString &aStr, const char *aBuf, PRUint32 aCount)
{
  if (aCount == PR_UINT32_MAX)
    aCount = (PRUint32) nsCharTraits<char>::length(aBuf);

  aStr.Assign(aBuf, aCount);
}

NS_STRINGAPI(void)
NS_CStringCopy(nsACString &aDest, const nsACString &aSrc, PRUint32 aOffset,
               PRUint32 aCount)
{
  if (aOffset == 0 && aCount == PR_UINT32_MAX)
    aDest.Assign(aSrc);
  else
    aDest.Assign(Substring(aSrc, aOffset, aCount));
}
