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
#include "nsCRT.h"
#include "nsDogbertProfileMigrator.h"
#include "nsIBookmarksService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsIObserverService.h"
#include "nsIOutputStream.h"
#include "nsIProfile.h"
#include "nsIProfileInternal.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "prprf.h"

#if defined(XP_MAC) || defined(XP_MACOSX)
#define NEED_TO_FIX_4X_COOKIES 1
#endif /* XP_MAC */
#ifdef NEED_TO_FIX_4X_COOKIES
#define SECONDS_BETWEEN_1900_AND_1970 2208988800UL
#endif

#define PREF_FILE_HEADER_STRING "# Mozilla User Preferences    " 

#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define PREF_FILE_NAME_IN_4x      NS_LITERAL_STRING("preferences.js")
#define COOKIES_FILE_NAME_IN_4x   NS_LITERAL_STRING("cookies")
#define BOOKMARKS_FILE_NAME_IN_4x NS_LITERAL_STRING("bookmarks.html")
#define PSM_CERT7_DB              "cert7.db"
#define PSM_KEY3_DB               "key3.db"
#define PSM_SECMODULE_DB          "secmodule.db"
#elif defined(XP_MAC) || defined(XP_MACOSX)
#define PREF_FILE_NAME_IN_4x      NS_LITERAL_STRING("Netscape Preferences")
#define COOKIES_FILE_NAME_IN_4x   NS_LITERAL_STRING("MagicCookie")
#define BOOKMARKS_FILE_NAME_IN_4x NS_LITERAL_STRING("Bookmarks.html")
#define SECURITY_PATH             "Security"
#define PSM_CERT7_DB              "Certificates7"
#define PSM_KEY3_DB               "Key Database3"
#define PSM_SECMODULE_DB          "Security Modules"
#else /* XP_WIN || XP_OS2 */
#define PREF_FILE_NAME_IN_4x      NS_LITERAL_STRING("prefs.js")
#define COOKIES_FILE_NAME_IN_4x   NS_LITERAL_STRING("cookies.txt")
#define BOOKMARKS_FILE_NAME_IN_4x NS_LITERAL_STRING("bookmark.htm")
#define PSM_CERT7_DB              "cert7.db"
#define PSM_KEY3_DB               "key3.db"
#define PSM_SECMODULE_DB          "secmod.db"
#endif /* XP_UNIX */

#define COOKIES_FILE_NAME_IN_5x   NS_LITERAL_STRING("cookies.txt")
#define BOOKMARKS_FILE_NAME_IN_5x NS_LITERAL_STRING("bookmarks.html")
#define PREF_FILE_NAME_IN_5x      NS_LITERAL_STRING("prefs.js")

#define PREF_CACHE_DIRECTORY      "browser.cache.directory"
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
    // Lordy, this API sucketh.
    rv = pmi->GetProfileListX(nsIProfileInternal::LIST_FOR_IMPORT, &profileCount, &profileNames);
    if (NS_FAILED(rv)) return rv;

    for (PRUint32 i = 0; i < profileCount; ++i) {
      nsCOMPtr<nsISupportsString> string(do_CreateInstance("@mozilla.org/supports-string;1"));
      string->SetData(nsDependentString(profileNames[i]));
      mProfiles->AppendElement(string);
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(profileCount, profileNames);
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
  nsresult rv;

  // 1) Copy Preferences
  rv = CopyFile(PREF_FILE_NAME_IN_4x, PREF_FILE_NAME_IN_5x);
  if (NS_FAILED(rv)) return rv;


#if 0
  /* Copy the old prefs file to the new profile directory for modification and reading.  
     after copying it, rename it to pref.js, the 5.x pref file name on all platforms */
  nsCOMPtr<nsIFileSpec> oldPrefsFile;
  rv = NS_NewFileSpec(getter_AddRefs(oldPrefsFile)); 
  if (NS_FAILED(rv)) return rv;
  
  rv = oldPrefsFile->FromFileSpec(oldProfilePath);
  if (NS_FAILED(rv)) return rv;
  
  rv = oldPrefsFile->AppendRelativeUnixPath(PREF_FILE_NAME_IN_4x);
  if (NS_FAILED(rv)) return rv;


  /* the new prefs file */
  nsCOMPtr<nsIFileSpec> newPrefsFile;
  rv = NS_NewFileSpec(getter_AddRefs(newPrefsFile)); 
  if (NS_FAILED(rv)) return rv;
  
  rv = newPrefsFile->FromFileSpec(newProfilePath);
  if (NS_FAILED(rv)) return rv;
  
  rv = newPrefsFile->Exists(&exists);
  if (!exists)
  {
	  rv = newPrefsFile->CreateDir();
  }

  rv = oldPrefsFile->CopyToDir(newPrefsFile);
  NS_ASSERTION(NS_SUCCEEDED(rv),"failed to copy prefs file");

  rv = newPrefsFile->AppendRelativeUnixPath(PREF_FILE_NAME_IN_4x);
  rv = newPrefsFile->Rename(PREF_FILE_NAME_IN_5x);
#endif


  // 2) Copy Certficates
  return NS_OK;
}

nsresult
nsDogbertProfileMigrator::CopyFile(const nsAString& aSourceFileName, const nsAString& aTargetFileName)
{
  nsCOMPtr<nsIFile> dogbertPrefFile;
  mSourceProfile->Clone(getter_AddRefs(dogbertPrefFile));

  dogbertPrefFile->Append(aSourceFileName);
  PRBool exists = PR_FALSE;
  dogbertPrefFile->Exists(&exists);
  if (!exists)
    return NS_ERROR_FILE_NOT_FOUND;

  return dogbertPrefFile->CopyTo(mTargetProfile, aTargetFileName);
}

nsresult
nsDogbertProfileMigrator::CopyCookies(PRBool aReplace)
{
#ifdef NEED_TO_FIX_4X_COOKIES
  nsresult rv = CopyFile(COOKIES_FILE_NAME_IN_4x, COOKIES_FILE_NAME_IN_5x);
  if (NS_FAILED(rv)) return rv;

  return FixDogbertCookies();
#else
  return CopyFile(COOKIES_FILE_NAME_IN_4x, COOKIES_FILE_NAME_IN_5x);
#endif
}

#ifdef NEED_TO_FIX_4X_COOKIES
nsresult
nsDogbertProfileMigrator::FixDogbertCookies()
{
  nsCOMPtr<nsIFile> dogbertCookiesFile;
  mSourceProfile->Clone(getter_AddRefs(dogbertCookiesFile));
  dogbertCookiesFile->Append(COOKIES_FILE_NAME_IN_4x);

  nsCOMPtr<nsIInputStream> fileInputStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), dogbertCookiesFile);
  if (!fileInputStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIFile> firebirdCookiesFile;
  mTargetProfile->Clone(getter_AddRefs(firebirdCookiesFile));
  firebirdCookiesFile->Append(COOKIES_FILE_NAME_IN_5x);

  nsCOMPtr<nsIOutputStream> fileOutputStream;
  NS_NewLocalFileOutputStream(getter_AddRefs(fileOutputStream), firebirdCookiesFile);
  if (!fileOutputStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineInputStream(do_QueryInterface(fileInputStream));
  nsAutoString buffer, outBuffer;
  PRBool moreData = PR_FALSE;
  PRUint32 written = 0;
  do {
    nsresult rv = lineInputStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv)) return rv;

    /* skip line if it is a comment or null line */
    if (buffer.IsEmpty() || buffer.CharAt(0) == '#' ||
        buffer.CharAt(0) == nsCRT::CR || buffer.CharAt(0) == nsCRT::LF) {
      fileOutputStream->Write((const char*)buffer.get(), buffer.Length(), &written);
      continue;
    }

    /* locate expire field, skip line if it does not contain all its fields */
    int hostIndex, isDomainIndex, pathIndex, xxxIndex, expiresIndex, nameIndex, cookieIndex;
    hostIndex = 0;
    if ((isDomainIndex = buffer.FindChar('\t', hostIndex)+1) == 0 ||
        (pathIndex = buffer.FindChar('\t', isDomainIndex)+1) == 0 ||
        (xxxIndex = buffer.FindChar('\t', pathIndex)+1) == 0 ||
        (expiresIndex = buffer.FindChar('\t', xxxIndex)+1) == 0 ||
        (nameIndex = buffer.FindChar('\t', expiresIndex)+1) == 0 ||
        (cookieIndex = buffer.FindChar('\t', nameIndex)+1) == 0 )
      continue;

    /* separate the expires field from the rest of the cookie line */
    nsAutoString prefix, expiresString, suffix;
    buffer.Mid(prefix, hostIndex, expiresIndex-hostIndex-1);
    buffer.Mid(expiresString, expiresIndex, nameIndex-expiresIndex-1);
    buffer.Mid(suffix, nameIndex, buffer.Length()-nameIndex);

    /* correct the expires field */
    char* expiresCString = ToNewCString(expiresString);
    unsigned long expires = strtoul(expiresCString, nsnull, 10);
    nsCRT::free(expiresCString);

    /* if the cookie is supposed to expire at the end of the session
     * expires == 0.  don't adjust those cookies.
     */
    if (expires)
    	expires -= SECONDS_BETWEEN_1900_AND_1970;
    char dateString[36];
    PR_snprintf(dateString, sizeof(dateString), "%lu", expires);

    /* generate the output buffer and write it to file */
    outBuffer = prefix;
    outBuffer.Append(PRUnichar('\t'));
    outBuffer.AppendWithConversion(dateString);
    outBuffer.Append(PRUnichar('\t'));
    outBuffer.Append(suffix);

    fileOutputStream->Write((const char*)outBuffer.get(), outBuffer.Length(), &written);
  }
  while (moreData);
}

#endif /* NEED_TO_FIX_4X_COOKIES */


nsresult
nsDogbertProfileMigrator::CopyBookmarks(PRBool aReplace)
{
  // If we're blowing away existing content, just copy the file, don't do fancy importing.
  if (aReplace)
    return CopyFile(BOOKMARKS_FILE_NAME_IN_4x, BOOKMARKS_FILE_NAME_IN_5x);

  nsCOMPtr<nsIFile> bookmarksFile;
  mSourceProfile->Clone(getter_AddRefs(bookmarksFile));
  bookmarksFile->Append(BOOKMARKS_FILE_NAME_IN_4x);

  nsCOMPtr<nsIBookmarksService> bms(do_GetService("@mozilla.org/browser/bookmarks-service;1"));
  nsCOMPtr<nsISupportsArray> params;
  NS_NewISupportsArray(getter_AddRefs(params));

  nsCOMPtr<nsIRDFService> rdfs(do_GetService("@mozilla.org/rdf/rdf-service;1"));
  nsCOMPtr<nsIRDFResource> prop;
  rdfs->GetResource(NS_LITERAL_CSTRING("http://home.netscape.com/NC_NS#URL"), 
                    getter_AddRefs(prop));
  nsCOMPtr<nsIRDFLiteral> url;
  nsAutoString path;
  bookmarksFile->GetPath(path);
  rdfs->GetLiteral(path.get(), getter_AddRefs(url));

  params->AppendElement(prop);
  params->AppendElement(url);

  nsCOMPtr<nsIRDFResource> importCmd;
  rdfs->GetResource(NS_LITERAL_CSTRING("http://home.netscape.com/NC_NS#command?cmd=import"), 
                    getter_AddRefs(importCmd));

  nsCOMPtr<nsIRDFResource> root;
  rdfs->GetResource(NS_LITERAL_CSTRING("NC:BookmarksRoot"), getter_AddRefs(root));

  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(kStringBundleServiceCID));
  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle(MIGRATION_BUNDLE, getter_AddRefs(bundle));

  nsXPIDLString importedDogbertBookmarksTitle;
  bundle->GetStringFromName(NS_LITERAL_STRING("importedDogbertBookmarksTitle").get(), getter_Copies(importedDogbertBookmarksTitle));

  nsCOMPtr<nsIRDFResource> folder;
  bms->CreateFolderInContainer(importedDogbertBookmarksTitle.get(), root, -1, getter_AddRefs(folder));

  nsCOMPtr<nsISupportsArray> sources;
  NS_NewISupportsArray(getter_AddRefs(sources));

  sources->AppendElement(folder);

  nsCOMPtr<nsIRDFDataSource> ds(do_QueryInterface(bms));
  return ds->DoCommand(sources, importCmd, params);
}

