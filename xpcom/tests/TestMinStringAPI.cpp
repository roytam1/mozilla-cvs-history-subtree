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

#include "nsStringAPI.h"
#include "nsCRT.h"

static const char kAsciiData[] = "hello world";

static const PRUnichar kUnicodeData[] =
  {'h','e','l','l','o',' ','w','o','r','l','d','\0'};

static PRBool
TestACString(nsACString &s)
{
  const char *ptr;
  char buf[256];
  PRUint32 len;

  ptr = NS_CStringGetDataPtr(s);
  if (ptr == nsnull || *ptr != '\0')
  {
    NS_BREAK();
    return PR_FALSE;
  }

  NS_CStringSetData(s, kAsciiData, PR_UINT32_MAX);
  ptr = NS_CStringGetDataPtr(s);
  if (ptr == nsnull || strcmp(ptr, kAsciiData) != 0)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  len = NS_CStringGetLength(s);
  if (len != sizeof(kAsciiData)-1)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  NS_CStringGetData(s, buf, sizeof(buf));
  if (strcmp(buf, kAsciiData) != 0)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  return PR_TRUE;
}

static PRBool
TestAString(nsAString &s)
{
  const PRUnichar *ptr;
  PRUnichar buf[256];
  PRUint32 len;

  ptr = NS_StringGetDataPtr(s);
  if (ptr == nsnull || *ptr != '\0')
  {
    NS_BREAK();
    return PR_FALSE;
  }

  NS_StringSetData(s, kUnicodeData, PR_UINT32_MAX);
  ptr = NS_StringGetDataPtr(s);
  if (ptr == nsnull || nsCRT::strcmp(ptr, kUnicodeData) != 0)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  len = NS_StringGetLength(s);
  if (len != (sizeof(kUnicodeData)-1)/2)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  NS_StringGetData(s, buf, sizeof(buf));
  if (nsCRT::strcmp(buf, kUnicodeData) != 0)
  {
    NS_BREAK();
    return PR_FALSE;
  }

  return PR_TRUE;
}

int main()
{
  PRBool rv;

  // TestCStringContainer
  {
    nsCStringContainer s;
    NS_CStringContainerInit(s);
    rv = TestACString(s);
    NS_CStringContainerFinish(s);
    printf("TestACString\t%s\n", rv ? "PASSED" : "FAILED");
  }

  // TestStringContainer
  {
    nsStringContainer s;
    NS_StringContainerInit(s);
    rv = TestAString(s);
    NS_StringContainerFinish(s);
    printf("TestAString\t%s\n", rv ? "PASSED" : "FAILED");
  }
}
