/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Membuf server code
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Joe Hewitt <hewitt@netscape.com>
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
 */

#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "membufDirProvider.h"

#ifdef XP_UNIX
#include "prenv.h"
#endif

// WARNING: These hard coded names need to go away. They need to
// come from localizable resources

#define APP_REGISTRY_NAME NS_LITERAL_CSTRING("registry.dat")

membufDirProvider::membufDirProvider()
{
  printf("membufDirProvider::membufDirProvider() \n");
  NS_INIT_ISUPPORTS();
}

membufDirProvider::~membufDirProvider()
{
  printf("membufDirProvider::~membufDirProvider() \n");
}

NS_IMPL_ISUPPORTS1(membufDirProvider, nsIDirectoryServiceProvider);

NS_IMETHODIMP
membufDirProvider::GetFile( const char *aProperty,
                            PRBool *aPersistent,
                            nsIFile **aFile )
{
  printf("membufDirProvider::GetFile(%s) \n", aProperty);
  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile;

  *aPersistent = PR_TRUE;

  if (!strcmp(aProperty, NS_APP_APPLICATION_REGISTRY_DIR)) {
    printf("membufDirProvider::GetFile() - App Reg Dir \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
  }
  else if (!strcmp(aProperty, NS_APP_APPLICATION_REGISTRY_FILE)) {
    printf("membufDirProvider::GetFile() - App Reg File \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
    if (NS_SUCCEEDED(rv))
      rv = localFile->AppendNative(APP_REGISTRY_NAME);
  }
  else if (!strcmp(aProperty, NS_APP_USER_PROFILES_ROOT_DIR)) {
    printf("membufDirProvider::GetFile() - App User Prof Root \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
    NS_ENSURE_SUCCESS(rv, rv);

#if !defined(XP_UNIX) || defined(XP_MACOSX)
    rv = localFile->AppendRelativeNativePath(NS_LITERAL_CSTRING("Profiles"));
    NS_ENSURE_SUCCESS(rv, rv);
#endif

    // We must create the profile directory here if it does not exist.
    rv = EnsureDirectoryExists(localFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (localFile) {
    printf("membufDirProvider::GetFile() - Have localFile \n");
    return CallQueryInterface(localFile, aFile);
  }

  printf("membufDirProvider::GetFile() - don't have localFile \n");
  return NS_ERROR_FAILURE;
}

nsresult
membufDirProvider::GetProductDirectory( nsILocalFile **aFile )
{
  printf("membufDirProvider::GetProductDirectory() \n");
  // Copied from nsAppFileLocationProvider (more or less)
  nsresult rv;
  nsCOMPtr<nsILocalFile> localDir;

  nsCOMPtr<nsIProperties> directoryService =  do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_WIN)
  PRBool exists;
  rv = directoryService->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
  if (NS_SUCCEEDED(rv))
    rv = localDir->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    // On some Win95 machines, NS_WIN_APPDATA_DIR does not exist - revert to NS_WIN_WINDOWS_DIR
    localDir = 0;
    rv = directoryService->Get(NS_WIN_WINDOWS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localDir));
  }
#elif defined(XP_UNIX)
  rv = NS_NewNativeLocalFile(nsDependentCString(PR_GetEnv("HOME")), PR_TRUE, getter_AddRefs(localDir));
#else
#error dont_know_how_to_get_product_dir_on_your_platform
#endif

  NS_ENSURE_SUCCESS(rv, rv);
  rv = localDir->AppendRelativeNativePath(NS_LITERAL_CSTRING(".mozURLimg"));
  NS_ENSURE_SUCCESS(rv, rv);


  //rv = EnsureDirectoryExists(localDir);
  rv = InitProfileDir(localDir);
  NS_ENSURE_SUCCESS(rv, rv);

  *aFile = localDir;
  NS_ADDREF(*aFile);
  return NS_OK;
}

nsresult
membufDirProvider::EnsureDirectoryExists(nsILocalFile* aDirectory)
{
  printf("membufDirProvider::EnsureDirectoryExists() \n");
  PRBool exists;
  nsresult rv = aDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    rv = aDirectory->Create(nsIFile::DIRECTORY_TYPE, 0775);

  return rv;
}


nsresult
membufDirProvider::InitProfileDir(nsIFile *profileDir)
{
  printf("membufDirProvider::InitProfileDir() \n");
  // Make sure our "Profile" folder exists.
  // If it does not, copy the profile defaults to its location.

  nsresult rv;
  PRBool exists;
  rv = profileDir->Exists(&exists);
  if (NS_FAILED(rv))
    return rv;

  if (!exists) {
    nsCOMPtr<nsIFile> profileDefaultsDir;
    nsCOMPtr<nsIFile> profileDirParent;
    nsCAutoString profileDirName;

    (void)profileDir->GetParent(getter_AddRefs(profileDirParent));
    if (!profileDirParent)
      return NS_ERROR_FAILURE;
    rv = profileDir->GetNativeLeafName(profileDirName);
    if (NS_FAILED(rv))
      return rv;

    rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_50_DIR, getter_AddRefs(profileDefaultsDir));
    if (NS_FAILED(rv)) {
      rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR, getter_AddRefs(profileDefaultsDir));
      if (NS_FAILED(rv))
        return rv;
    }
    rv = profileDefaultsDir->CopyToNative(profileDirParent, profileDirName);
    if (NS_FAILED(rv)) {
        // if copying failed, lets just ensure that the profile directory exists.
        profileDirParent->AppendNative(profileDirName);
        rv = profileDirParent->Create(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv))
            return rv;
    }

#ifndef XP_MAC
    rv = profileDir->SetPermissions(0700);
    if (NS_FAILED(rv))
      return rv;
#endif

  }
  else {
    PRBool isDir;
    rv = profileDir->IsDirectory(&isDir);

    if (NS_FAILED(rv))
      return rv;
    if (!isDir)
      return NS_ERROR_FILE_NOT_DIRECTORY;
  }

  if (!mNonSharedDirName.IsEmpty())
    rv = InitNonSharedProfileDir();

  return rv;
}

nsresult
membufDirProvider::InitNonSharedProfileDir()
{
  printf("membufDirProvider::InitNonSharedProfileDir() \n");
  nsresult rv;

  NS_ENSURE_STATE(mProfileDir);
  NS_ENSURE_STATE(!mNonSharedDirName.IsEmpty());

  nsCOMPtr<nsIFile> localDir;
  rv = mProfileDir->Clone(getter_AddRefs(localDir));
  if (NS_SUCCEEDED(rv)) {
    rv = localDir->Append(mNonSharedDirName);
    if (NS_SUCCEEDED(rv)) {
      PRBool exists;
      rv = localDir->Exists(&exists);
      if (NS_SUCCEEDED(rv)) {
        if (!exists) {
          rv = localDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
        }
        else {
          PRBool isDir;
          rv = localDir->IsDirectory(&isDir);
          if (NS_SUCCEEDED(rv)) {
            if (!isDir)
              rv = NS_ERROR_FILE_NOT_DIRECTORY;
          }
        }
        if (NS_SUCCEEDED(rv))
          mNonSharedProfileDir = localDir;
      }
    }
  }
  return rv;
}
