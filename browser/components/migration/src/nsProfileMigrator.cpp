/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is The Browser Profile Migrator.
 *
 * The Initial Developer of the Original Code is Ben Goodger.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ben Goodger <ben@bengoodger.com>
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

#include "nsProfileMigrator.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#ifdef XP_WIN
#include <windows.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// nsBrowserProfileMigrator

NS_IMPL_ISUPPORTS1(nsProfileMigrator, nsIProfileMigrator)

NS_IMETHODIMP
nsProfileMigrator::Migrate()
{
  nsCOMPtr<nsIBrowserProfileMigrator> bpm;
  GetDefaultBrowserMigrator(getter_AddRefs(bpm));

  if (bpm)
    return bpm->Migrate(nsIBrowserProfileMigrator::ALL, PR_TRUE);
  
  return NS_OK;
}

#ifdef XP_WIN
typedef struct {
  WORD wLanguage;
  WORD wCodePage;
} LANGANDCODEPAGE;

#define INTERNAL_NAME_FIREBIRD        "Firebird"
#define INTERNAL_NAME_IEXPLORE        "iexplore"
#define INTERNAL_NAME_SEAMONKEY       "apprunner"
#define INTERNAL_NAME_DOGBERT         "NETSCAPE"
#define INTERNAL_NAME_OPERA           "Opera"
#endif

nsresult
nsProfileMigrator::GetDefaultBrowserMigrator(nsIBrowserProfileMigrator** aResult)
{
#ifdef XP_WIN
  HKEY hkey;

  const char* kCommandKey = "SOFTWARE\\Classes\\HTTP\\shell\\open\\command";
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, kCommandKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
    DWORD type;
    DWORD length = MAX_PATH;
    unsigned char value[MAX_PATH];
    if (::RegQueryValueEx(hkey, NULL, 0, &type, value, &length) == ERROR_SUCCESS) {
      nsCAutoString str; str.Assign((char*)value);

      PRInt32 lastIndex = str.Find(".exe", PR_TRUE);

      nsCAutoString filePath;
      if (str.CharAt(1) == ':') 
        filePath = Substring(str, 0, lastIndex + 4);
      else
        filePath = Substring(str, 1, lastIndex + 3);        

      char* strCpy = ToNewCString(filePath);

      // We want to find out what the default browser is but the path in and of itself
      // isn't enough. Why? Because sometimes on Windows paths get truncated like so:
      // C:\PROGRA~1\MOZILL~2\MOZILL~1.EXE
      // How do we know what product that is? Mozilla or Mozilla Firebird? etc. Mozilla's
      // file objects do nothing to 'normalize' the path so we need to attain an actual
      // product descriptor from the file somehow, and in this case it means getting
      // the "InternalName" field of the file's VERSIONINFO resource. 
      //
      // In the file's resource segment there is a VERSIONINFO section that is laid 
      // out like this:
      //
      // VERSIONINFO
      //   StringFileInfo
      //     <TranslationID>
      //       InternalName           "iexplore"
      //   VarFileInfo
      //     Translation              <TranslationID>
      //
      // By Querying the VERSIONINFO section for its Tranlations, we can find out where
      // the InternalName lives. (A file can have more than one translation of its 
      // VERSIONINFO segment, but we just assume the first one). 
      DWORD dummy;
      DWORD size = ::GetFileVersionInfoSize(strCpy, &dummy);
      if (size) {
        void* ver = malloc(size);  
        memset(ver, 0, size);

        if (::GetFileVersionInfo(strCpy, 0, size, ver)) {
          LANGANDCODEPAGE* translate;
          UINT pageCount;
          ::VerQueryValue(ver, TEXT("\\VarFileInfo\\Translation"), (void**)&translate, &pageCount);

          if (pageCount > 0) {
            TCHAR subBlock[MAX_PATH];
            wsprintf(subBlock, TEXT("\\StringFileInfo\\%04x%04x\\InternalName"), translate[0].wLanguage, translate[0].wCodePage);

            LPVOID internalName = NULL;
            UINT size;
            ::VerQueryValue(ver, subBlock, (void**)&internalName, &size);

            printf("*** default browser is %s\n", internalName);
            if (!nsCRT::strcasecmp((char*)internalName, INTERNAL_NAME_IEXPLORE)) {
              printf("*** default browser is %s\n", internalName);
            }
            else if (!nsCRT::strcasecmp((char*)internalName, INTERNAL_NAME_SEAMONKEY)) {
              printf("*** default browser is %s\n", internalName);
            }
            else if (!nsCRT::strcasecmp((char*)internalName, INTERNAL_NAME_DOGBERT)) {
              printf("*** default browser is %s\n", internalName);
            }
            else if (!nsCRT::strcasecmp((char*)internalName, INTERNAL_NAME_OPERA)) {
              printf("*** default browser is %s\n", internalName);
            }
          }
        }
      }
      printf("***goats\n");
    }
  }
#endif
  return NS_OK;
}


