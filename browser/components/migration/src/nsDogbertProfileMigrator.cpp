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

#include "nsAppDirectoryServiceDefs.h"
#include "nsBrowserProfileMigratorUtils.h"
#include "nsDogbertProfileMigrator.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsIProfile.h"
#include "nsIProfileInternal.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"

#define PREF_FILE_HEADER_STRING "# Mozilla User Preferences    " 

#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define COOKIES_FILE_NAME_IN_4x   "cookies"
#define BOOKMARKS_FILE_NAME_IN_4x "bookmarks.html"
#define PSM_CERT7_DB              "cert7.db"
#define PSM_KEY3_DB               "key3.db"
#define PSM_SECMODULE_DB          "secmodule.db"
#elif defined(XP_MAC) || defined(XP_MACOSX)
#define COOKIES_FILE_NAME_IN_4x   "MagicCookie"
#define BOOKMARKS_FILE_NAME_IN_4x "Bookmarks.html"
#define SECURITY_PATH             "Security"
#define PSM_CERT7_DB              "Certificates7"
#define PSM_KEY3_DB               "Key Database3"
#define PSM_SECMODULE_DB          "Security Modules"
#else /* XP_WIN || XP_OS2 */
#define COOKIES_FILE_NAME_IN_4x   "cookies.txt"
#define BOOKMARKS_FILE_NAME_IN_4x "bookmark.htm"
#define PSM_CERT7_DB              "cert7.db"
#define PSM_KEY3_DB               "key3.db"
#define PSM_SECMODULE_DB          "secmod.db"
#endif /* XP_UNIX */

#define COOKIES_FILE_NAME_IN_5x   "cookies.txt"
#define BOOKMARKS_FILE_NAME_IN_5x "bookmarks.html"
#define HISTORY_FILE_NAME_IN_5x   "history.dat"

#define MIGRATION_BUNDLE          "chrome://browser/locale/migration/migration.properties"

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

///////////////////////////////////////////////////////////////////////////////
// nsDogbertProfileMigrator

NS_IMPL_ISUPPORTS1(nsDogbertProfileMigrator, nsIBrowserProfileMigrator)

static nsIObserverService* sObserverService = nsnull;

nsDogbertProfileMigrator::nsDogbertProfileMigrator()
{
  CallGetService("@mozilla.org/observer-service;1", &sObserverService);
}

nsDogbertProfileMigrator::~nsDogbertProfileMigrator()
{
  NS_IF_RELEASE(sObserverService);
}

///////////////////////////////////////////////////////////////////////////////
// nsIBrowserProfileMigrator

NS_IMETHODIMP
nsDogbertProfileMigrator::Migrate(PRUint32 aItems, PRBool aReplace, const PRUnichar* aProfile)
{
  nsresult rv = NS_OK;

  NOTIFY_OBSERVERS(MIGRATION_STARTED, nsnull);

  CreateTemplateProfile(aProfile);

  if (aReplace) {
    COPY_DATA(CopyPreferences,  aReplace, nsIBrowserProfileMigrator::SETTINGS,  NS_LITERAL_STRING("settings").get());
    COPY_DATA(CopyCookies,      aReplace, nsIBrowserProfileMigrator::COOKIES,   NS_LITERAL_STRING("cookies").get());
  }
  COPY_DATA(CopyBookmarks,    aReplace, nsIBrowserProfileMigrator::BOOKMARKS, NS_LITERAL_STRING("bookmarks").get());

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsDogbertProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
{
  nsCOMPtr<nsISupportsArray> profiles;
  GetSourceProfiles(getter_AddRefs(profiles));

  if (profiles) {
    PRUint32 count;
    profiles->Count(&count);
    *aResult = count > 1;
  }
  else
    *aResult = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsDogbertProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  if (!mProfiles) {
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(mProfiles));
    if (NS_FAILED(rv)) return rv;

    // Our profile manager stores information about the set of Dogbert Profiles we have.
    nsCOMPtr<nsIProfileInternal> pmi(do_CreateInstance("@mozilla.org/profile/manager;1"));
    PRUnichar** profileNames = nsnull;
    PRUint32 profileCount = 0;
    rv = pmi->GetProfileListX(nsIProfileInternal::LIST_FOR_IMPORT, &profileCount, &profileNames);
    if (NS_FAILED(rv)) return rv;

    for (PRUint32 i = 0; i < profileCount; ++i) {
      nsCOMPtr<nsISupportsString> string(do_CreateInstance("@mozilla.org/supports-string;1"));
      string->SetData(nsDependentString(profileNames[i]));
      mProfiles->AppendElement(string);
    }
  }
  
  NS_IF_ADDREF(*aResult = mProfiles);
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// nsDogbertProfileMigrator

nsresult
nsDogbertProfileMigrator::CreateTemplateProfile(const PRUnichar* aSuggestedName)
{
  nsCOMPtr<nsIFile> profilesDir;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(profilesDir));

  nsXPIDLString profileName;
  GetUniqueProfileName(profilesDir, aSuggestedName, getter_Copies(profileName));

  nsAutoString profilesDirPath;
  profilesDir->GetPath(profilesDirPath);

  nsCOMPtr<nsIProfile> pm(do_GetService("@mozilla.org/profile/manager;1"));
  pm->CreateNewProfile(profileName.get(), profilesDirPath.get(), nsnull, PR_TRUE);

  nsCOMPtr<nsIProfileInternal> pmi(do_QueryInterface(pm));
  nsCOMPtr<nsIFile> target;
  pmi->GetProfileDir(profileName.get(), getter_AddRefs(target));
  mTargetProfile = do_QueryInterface(target);
  pmi->GetOriginalProfileDir(aSuggestedName, getter_AddRefs(mSourceProfile));

  return NS_OK;
}

void
nsDogbertProfileMigrator::GetUniqueProfileName(nsIFile* aProfilesDir, 
                                               const PRUnichar* aSuggestedName,
                                               PRUnichar** aUniqueName)
{
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(kStringBundleServiceCID));

  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle(MIGRATION_BUNDLE, getter_AddRefs(bundle));

  PRBool exists = PR_FALSE;
  PRUint32 count = 1;
  nsXPIDLString profileName;

  do {
    nsAutoString countString;
    countString.AppendInt(count);
    const PRUnichar* strings[2] = { aSuggestedName, countString.get() };
    bundle->FormatStringFromName(NS_LITERAL_STRING("profileName_format").get(), strings, 2, getter_Copies(profileName));

    nsCOMPtr<nsIFile> newProfileDir;
    aProfilesDir->Clone(getter_AddRefs(newProfileDir));
    newProfileDir->Append(profileName);
    newProfileDir->Exists(&exists);

    ++count;
  } 
  while (exists);
  
  *aUniqueName = ToNewUnicode(profileName);
}


nsresult
nsDogbertProfileMigrator::CopyPreferences(PRBool aReplace)
{
  // 1) Copy Preferences
  // 2) Copy Certficates
  return NS_OK;
}

nsresult
nsDogbertProfileMigrator::CopyCookies(PRBool aReplace)
{

  return NS_OK;
}

nsresult
nsDogbertProfileMigrator::CopyBookmarks(PRBool aReplace)
{

  return NS_OK;
}

