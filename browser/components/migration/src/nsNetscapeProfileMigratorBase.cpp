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
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPrefService.h"
#include "nsIProfile.h"
#include "nsIProfileInternal.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"
#include "nsNetscapeProfileMigratorBase.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "prprf.h"

#if defined(XP_MAC) || defined(XP_MACOSX)
#define NEED_TO_FIX_4X_COOKIES 1
#define SECONDS_BETWEEN_1900_AND_1970 2208988800UL
#endif /* XP_MAC */

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define MIGRATION_BUNDLE "chrome://browser/locale/migration/migration.properties"

///////////////////////////////////////////////////////////////////////////////
// nsNetscapeProfileMigratorBase
nsNetscapeProfileMigratorBase::nsNetscapeProfileMigratorBase()
{
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(kStringBundleServiceCID));
  bundleService->CreateBundle(MIGRATION_BUNDLE, getter_AddRefs(mBundle));
}

nsresult
nsNetscapeProfileMigratorBase::CreateTemplateProfile(const PRUnichar* aSuggestedName)
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
nsNetscapeProfileMigratorBase::GetUniqueProfileName(nsIFile* aProfilesDir, 
                                                    const PRUnichar* aSuggestedName,
                                                    PRUnichar** aUniqueName)
{
  PRBool exists = PR_FALSE;
  PRUint32 count = 1;
  nsXPIDLString profileName;
  nsAutoString profileNameStr(aSuggestedName);
  
  nsCOMPtr<nsIFile> newProfileDir;
  aProfilesDir->Clone(getter_AddRefs(newProfileDir));
  newProfileDir->Append(profileNameStr);
  newProfileDir->Exists(&exists);

  while (exists) {
    nsAutoString countString;
    countString.AppendInt(count);
    const PRUnichar* strings[2] = { aSuggestedName, countString.get() };
    mBundle->FormatStringFromName(NS_LITERAL_STRING("profileName_format").get(), strings, 2, getter_Copies(profileName));

    nsCOMPtr<nsIFile> newProfileDir;
    aProfilesDir->Clone(getter_AddRefs(newProfileDir));
    newProfileDir->Append(profileName);
    newProfileDir->Exists(&exists);

    profileNameStr = profileName.get();
    ++count;
  }

  *aUniqueName = ToNewUnicode(profileNameStr);
}

nsresult 
nsNetscapeProfileMigratorBase::GetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->GetCharPref(xform->sourcePrefName, &xform->stringValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetCharPref(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, xform->stringValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetWString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
  nsAutoString data; data.AssignWithConversion(xform->stringValue);
  pls->SetData(data.get());
  return aBranch->SetComplexValue(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, NS_GET_IID(nsIPrefLocalizedString), pls);
}

nsresult 
nsNetscapeProfileMigratorBase::GetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->GetBoolPref(xform->sourcePrefName, &xform->boolValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetBoolPref(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, xform->boolValue);
}

nsresult 
nsNetscapeProfileMigratorBase::GetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->GetIntPref(xform->sourcePrefName, &xform->intValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetIntPref(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, xform->intValue);
}

nsresult
nsNetscapeProfileMigratorBase::TransformPreferences(PREFTRANSFORM* aTransforms, 
                                                    const nsAString& aSourcePrefFileName,
                                                    const nsAString& aTargetPrefFileName)
{
  PREFTRANSFORM* transform;
  PREFTRANSFORM* end = aTransforms + sizeof(aTransforms)/sizeof(PREFTRANSFORM);

  // Load the source pref file
  nsCOMPtr<nsIPrefService> psvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
  psvc->ResetPrefs();

  nsCOMPtr<nsIFile> sourcePrefsFile;
  mSourceProfile->Clone(getter_AddRefs(sourcePrefsFile));
  sourcePrefsFile->Append(aSourcePrefFileName);
  psvc->ReadUserPrefs(sourcePrefsFile);

  nsCOMPtr<nsIPrefBranch> branch(do_QueryInterface(psvc));
  for (transform = aTransforms; transform < end; ++transform)
    transform->prefGetterFunc(transform, branch);

  // Now that we have all the pref data in memory, load the target pref file,
  // and write it back out
  psvc->ResetPrefs();
  for (transform = aTransforms; transform < end; ++transform)
    transform->prefSetterFunc(transform, branch);

  nsCOMPtr<nsIFile> targetPrefsFile;
  mTargetProfile->Clone(getter_AddRefs(targetPrefsFile));
  targetPrefsFile->Append(aTargetPrefFileName);
  psvc->SavePrefFile(targetPrefsFile);

  return NS_OK;
}


nsresult
nsNetscapeProfileMigratorBase::CopyFile(const nsAString& aSourceFileName, const nsAString& aTargetFileName)
{
  nsCOMPtr<nsIFile> sourceFile;
  mSourceProfile->Clone(getter_AddRefs(sourceFile));

  sourceFile->Append(aSourceFileName);
  PRBool exists = PR_FALSE;
  sourceFile->Exists(&exists);
  if (!exists)
    return NS_ERROR_FILE_NOT_FOUND;

  nsCOMPtr<nsIFile> targetFile;
  mTargetProfile->Clone(getter_AddRefs(targetFile));
  
  targetFile->Append(aTargetFileName);
  targetFile->Exists(&exists);
  if (exists)
    targetFile->Remove(PR_FALSE);

  return sourceFile->CopyTo(mTargetProfile, aTargetFileName);
}

