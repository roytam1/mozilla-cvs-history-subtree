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

#include "nsOperaProfileMigrator.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIProperties.h"
#include "nsILocalFile.h"
#ifdef XP_WIN
#include <windows.h>
#endif

#define MIGRATION_ITEMBEFOREMIGRATE "Migration:ItemBeforeMigrate"
#define MIGRATION_ITEMAFTERMIGRATE  "Migration:ItemAfterMigrate"
#define MIGRATION_STARTED           "Migration:Started"
#define MIGRATION_ENDED             "Migration:Ended"

///////////////////////////////////////////////////////////////////////////////
// nsBrowserProfileMigrator

NS_IMPL_ISUPPORTS1(nsOperaProfileMigrator, nsIBrowserProfileMigrator)

static nsIObserverService* sObserverService = nsnull;

nsOperaProfileMigrator::nsOperaProfileMigrator()
{
  CallGetService("@mozilla.org/observer-service;1", &sObserverService);
}

nsOperaProfileMigrator::~nsOperaProfileMigrator()
{
  NS_IF_RELEASE(sObserverService);
}

#define NOTIFY_OBSERVERS(message, item) \
  if (sObserverService) \
    sObserverService->NotifyObservers(nsnull, message, item)

#define COPY_DATA(func, replace, itemIndex, itemString) \
  if (NS_SUCCEEDED(rv) && (aItems & itemIndex)) { \
    NOTIFY_OBSERVERS(MIGRATION_ITEMBEFOREMIGRATE, itemString); \
    rv = func(replace); \
    NOTIFY_OBSERVERS(MIGRATION_ITEMAFTERMIGRATE, itemString); \
  }

NS_IMETHODIMP
nsOperaProfileMigrator::  Migrate(PRUint32 aItems, PRBool aReplace, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  if (!mOperaProfile)
    GetOperaProfile(aProfile, getter_AddRefs(mOperaProfile));

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  COPY_DATA(CopyPreferences,  aReplace, nsIBrowserProfileMigrator::SETTINGS,  NS_LITERAL_STRING("settings").get());
  COPY_DATA(CopyCookies,      aReplace, nsIBrowserProfileMigrator::COOKIES,   NS_LITERAL_STRING("cookies").get());
  COPY_DATA(CopyHistory,      aReplace, nsIBrowserProfileMigrator::HISTORY,   NS_LITERAL_STRING("history").get());
  COPY_DATA(CopyFormData,     aReplace, nsIBrowserProfileMigrator::FORMDATA,  NS_LITERAL_STRING("formdata").get());
  COPY_DATA(CopyPasswords,    aReplace, nsIBrowserProfileMigrator::PASSWORDS, NS_LITERAL_STRING("passwords").get());
  COPY_DATA(CopyHotlist,      aReplace, nsIBrowserProfileMigrator::BOOKMARKS, NS_LITERAL_STRING("bookmarks").get());
  COPY_DATA(CopyDownloads,    aReplace, nsIBrowserProfileMigrator::DOWNLOADS, NS_LITERAL_STRING("bookmarks").get());

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

nsresult
nsOperaProfileMigrator::CopyPreferences(PRBool aReplace)
{
  printf("*** copy opera preferences\n");
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyCookies(PRBool aReplace)
{
  printf("*** copy opera cookies\n");
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyHistory(PRBool aReplace)
{
  printf("*** copy opera history\n");
  
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyFormData(PRBool aReplace)
{
  printf("*** copy opera form data\n");
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyPasswords(PRBool aReplace)
{
  printf("*** copy opera passwords\n");
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyHotlist(PRBool aReplace)
{
  printf("*** copy opera hotlist\n");
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyDownloads(PRBool aReplace)
{
  printf("*** copy opera downloads\n");
  return NS_OK;
}

void
nsOperaProfileMigrator::GetOperaProfile(const PRUnichar* aProfile, nsILocalFile** aFile)
{
  nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1"));
  nsCOMPtr<nsILocalFile> file;
  fileLocator->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(file));

  // Opera profile lives under %APP_DATA%\Opera\<operaver>\profile 
  file->Append(NS_LITERAL_STRING("Opera"));
  file->Append(nsDependentString(aProfile));
  file->Append(NS_LITERAL_STRING("profile"));

  *aFile = file;
  NS_ADDREF(*aFile);
}

