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

#include "nsBrowserProfileMigratorUtils.h"
#include "nsCRT.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsIBrowserHistory.h"
#include "nsIGlobalHistory.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsILocalFile.h"
#include "nsINIParser.h"
#include "nsIObserverService.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPrefService.h"
#include "nsIProperties.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsOperaProfileMigrator.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#ifdef XP_WIN
#include <windows.h>
#endif

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

NS_IMETHODIMP
nsOperaProfileMigrator::Migrate(PRUint32 aItems, PRBool aReplace, const PRUnichar* aProfile)
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
  COPY_DATA(CopyOtherData,    aReplace, nsIBrowserProfileMigrator::OTHERDATA, NS_LITERAL_STRING("otherdata").get());

  NOTIFY_OBSERVERS(MIGRATION_ENDED, nsnull);

  return rv;
}

NS_IMETHODIMP
nsOperaProfileMigrator::GetSourceHasMultipleProfiles(PRBool* aResult)
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
nsOperaProfileMigrator::GetSourceProfiles(nsISupportsArray** aResult)
{
  if (!mProfiles) {
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(mProfiles));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1"));
    nsCOMPtr<nsILocalFile> file;
    fileLocator->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(file));

    // Opera profile lives under %APP_DATA%\Opera\<operaver>\profile 
    file->Append(NS_LITERAL_STRING("Opera"));

    nsCOMPtr<nsISimpleEnumerator> e;
    file->GetDirectoryEntries(getter_AddRefs(e));

    PRBool hasMore;
    e->HasMoreElements(&hasMore);
    while (hasMore) {
      nsCOMPtr<nsILocalFile> curr;
      e->GetNext(getter_AddRefs(curr));

      PRBool isDirectory = PR_FALSE;
      curr->IsDirectory(&isDirectory);
      if (isDirectory) {
        nsCOMPtr<nsISupportsString> string(do_CreateInstance("@mozilla.org/supports-string;1"));
        nsAutoString leafName;
        curr->GetLeafName(leafName);
        string->SetData(leafName);
        mProfiles->AppendElement(string);
      }

      e->HasMoreElements(&hasMore);
    }
  }

  *aResult = mProfiles;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

#define _OPM(type) nsOperaProfileMigrator::type

static
nsOperaProfileMigrator::PREFTRANSFORM gTransforms[] = {
  { "User Prefs", "Download Directory", _OPM(STRING), "browser.download.defaultFolder", _OPM(SetFile), PR_FALSE, -1 },
  { nsnull, "Enable Cookies", _OPM(INT), "network.cookie.cookieBehavior", _OPM(SetCookieBehavior), PR_FALSE, -1 },
  { nsnull, "Accept Cookies Session Only", _OPM(BOOL), "network.cookie.enableForCurrentSessionOnly", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to resize window", _OPM(BOOL), "dom.disable_window_move_resize", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to move window", _OPM(BOOL), "dom.disable_window_move_resize", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to raise window", _OPM(BOOL), "dom.disable_window_flip", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Allow script to change status", _OPM(BOOL), "dom.disable_window_status_change", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Home URL", _OPM(STRING), "browser.startup.homepage", _OPM(SetWString), PR_FALSE, -1 },
  { nsnull, "Ignore Unrequested Popups", _OPM(BOOL), "dom.disable_open_during_load", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Load Figures", _OPM(BOOL), "network.image.imageBehavior", _OPM(SetImageBehavior), PR_FALSE, -1 },

  { "Visited link", nsnull, _OPM(COLOR), "browser.visited_color", _OPM(SetString), PR_FALSE, -1 },
  { "Link", nsnull, _OPM(COLOR), "browser.anchor_color", _OPM(SetString), PR_FALSE, -1 },
  { nsnull, "Underline", _OPM(BOOL), "browser.underline_anchors", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Expiry", _OPM(INT), "browser.history_expire_days", _OPM(SetInt), PR_FALSE, -1 },

  { "Security Prefs", "Enable SSL v2", _OPM(BOOL), "security.enable_ssl2", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Enable SSL v3", _OPM(BOOL), "security.enable_ssl3", _OPM(SetBool), PR_FALSE, -1 },
  { nsnull, "Enable TLS v1.0", _OPM(BOOL), "security.enable_tls", _OPM(SetBool), PR_FALSE, -1 },

  { "Extensions", "Scripting", _OPM(BOOL), "javascript.enabled", _OPM(SetBool), PR_FALSE, -1 }
};

nsresult 
nsOperaProfileMigrator::SetFile(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsCOMPtr<nsILocalFile> lf(do_CreateInstance("@mozilla.org/file/local;1"));
  lf->InitWithNativePath(nsDependentCString(xform->stringValue));
  return aBranch->SetComplexValue(xform->targetPrefName, NS_GET_IID(nsILocalFile), lf);
}

nsresult 
nsOperaProfileMigrator::SetCookieBehavior(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  PRInt32 val = (xform->intValue == 3) ? 0 : (xform->intValue == 0) ? 2 : 1;
  return aBranch->SetIntPref(xform->targetPrefName, val);
}

nsresult 
nsOperaProfileMigrator::SetImageBehavior(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetIntPref(xform->targetPrefName, xform->boolValue ? 0 : 2);
}

nsresult 
nsOperaProfileMigrator::SetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetBoolPref(xform->targetPrefName, xform->intValue);
}

nsresult 
nsOperaProfileMigrator::SetWString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
  nsAutoString data; data.AssignWithConversion(xform->stringValue);
  pls->SetData(data.get());
  return aBranch->SetComplexValue(xform->targetPrefName, NS_GET_IID(nsIPrefLocalizedString), pls);
}

nsresult 
nsOperaProfileMigrator::SetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetIntPref(xform->targetPrefName, xform->intValue);
}

nsresult 
nsOperaProfileMigrator::SetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PREFTRANSFORM* xform = (PREFTRANSFORM*)aTransform;
  return aBranch->SetCharPref(xform->targetPrefName, xform->stringValue);
}

nsresult
nsOperaProfileMigrator::CopyPreferences(PRBool aReplace)
{
  nsCOMPtr<nsIFile> operaPrefs;
  mOperaProfile->Clone(getter_AddRefs(operaPrefs));
  operaPrefs->Append(NS_LITERAL_STRING("opera6.ini"));

  nsCAutoString path;
  operaPrefs->GetNativePath(path);
  char* pathCopy = ToNewCString(path);
  if (!pathCopy)
    return NS_ERROR_OUT_OF_MEMORY;
  nsINIParser* parser = new nsINIParser(pathCopy);
  if (!parser)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIPrefBranch> branch(do_GetService(NS_PREFSERVICE_CONTRACTID));

  // Traverse the standard transforms
  PREFTRANSFORM* transform;
  PREFTRANSFORM* end = gTransforms + sizeof(gTransforms)/sizeof(PREFTRANSFORM);

  PRInt32 length;
  char* lastSectionName = nsnull;
  for (transform = gTransforms; transform < end; ++transform) {
    if (transform->sectionName)
      lastSectionName = transform->sectionName;

    if (transform->type == _OPM(COLOR)) {
      char* colorString = (char*)malloc(sizeof(char) * 8);
      if (!colorString)
        return NS_ERROR_OUT_OF_MEMORY;

      nsresult rv = ParseColor(parser, lastSectionName, &colorString);
      if (NS_SUCCEEDED(rv)) {
        transform->stringValue = colorString;
   
        transform->prefHasValue = PR_TRUE;
        transform->prefSetterFunc(transform, branch);
      }
      nsCRT::free(colorString);
    }
    else {
      char* val = nsnull;
      PRInt32 err = parser->GetStringAlloc(lastSectionName, transform->keyName, &val, &length);
      if (err == nsINIParser::OK) {
        nsCAutoString valStr;
        PRInt32 strerr;
        switch (transform->type) {
        case _OPM(STRING):
          transform->stringValue = val;
          break;
        case _OPM(INT):
          valStr = val;
          transform->intValue = valStr.ToInteger(&strerr);
          break;
        case _OPM(BOOL):
          valStr = val;
          transform->boolValue = valStr.ToInteger(&strerr) != 0;
          break;
        }
        transform->prefHasValue = PR_TRUE;
        transform->prefSetterFunc(transform, branch);
      }
    }
  }

  // Copy Proxy Settings

  // Copy User Content Sheet
  if (aReplace)
    CopyUserContentSheet(parser);

  nsCRT::free(pathCopy);

  delete parser;
  parser = nsnull;

  return NS_OK;
}

nsresult
nsOperaProfileMigrator::ParseColor(nsINIParser* aParser, char* aSectionName, char** aResult)
{
#define CHAR_BUF_LENGTH 5
  char rbuf[CHAR_BUF_LENGTH], gbuf[CHAR_BUF_LENGTH], bbuf[CHAR_BUF_LENGTH];
  PRInt32 bufSize = CHAR_BUF_LENGTH;
  PRInt32 r, g, b;

  nsCAutoString valStr;
  PRInt32 err, strerr;
  err = aParser->GetString(aSectionName, "Red", rbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = rbuf;
  r = valStr.ToInteger(&strerr);
  
  bufSize = CHAR_BUF_LENGTH;
  err = aParser->GetString(aSectionName, "Green", gbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = gbuf;
  g = valStr.ToInteger(&strerr);

  bufSize = CHAR_BUF_LENGTH;
  err = aParser->GetString(aSectionName, "Blue", bbuf, &bufSize);
  if (err != nsINIParser::OK) return NS_ERROR_FAILURE;
  valStr = bbuf;
  b = valStr.ToInteger(&strerr);

  sprintf(*aResult, "#%02X%02X%02X", r, g, b);

  return NS_OK;
}

nsresult 
nsOperaProfileMigrator::CopyUserContentSheet(nsINIParser* aParser)
{
  char* userContentCSS = nsnull;
  PRInt32 size;
  PRInt32 err = aParser->GetStringAlloc("User Prefs", "Local CSS File", &userContentCSS, &size);
  if (err == nsINIParser::OK && userContentCSS) {
    // Copy the file
  }
  return NS_OK;
}

nsresult
nsOperaProfileMigrator::CopyCookies(PRBool aReplace)
{
  printf("*** copy opera cookies\n");

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("cookies4.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIBinaryInputStream> binaryStream = do_QueryInterface(fileStream);
  nsOperaCookieMigrator* ocm = new nsOperaCookieMigrator(binaryStream);
  if (!ocm)
    return NS_ERROR_OUT_OF_MEMORY;

  ocm->Migrate();

  return NS_OK;
}

nsOperaCookieMigrator::nsOperaCookieMigrator(nsIBinaryInputStream* aStream)
{
  mStream = aStream;
}

nsresult
nsOperaCookieMigrator::Migrate()
{
  if (!mStream)
    return NS_ERROR_FAILURE;

  nsresult rv = ReadHeader();
  printf("*** rv = %d\n", rv);
  if (NS_FAILED(rv))
    return NS_OK;

  PRUint8 tag;
  PRUint16 length;
  char* buf;
  do {
    mStream->Read8(&tag);
    mTagStack.AppendElement((void*)tag);
    switch (tag) {
    case OPEN_DOMAIN:
      mStream->Read16(&length);
      break;
    case DOMAIN_NAME:
      mStream->Read16(&length);
      
      buf = (char*)malloc((sizeof(char) * length) + 1);
      mStream->ReadBytes(length, &buf);
      buf[length-1] = '\0';
      printf("*** domain = %s\n", buf);
      mDomainStack.AppendElement((void*)buf);
      break;
    case SEGMENT_INFO:

      break;
    case COOKIE_ID:
      break;
    case COOKIE_DATA:
      break;
    case EXPIRY_TIME:
      break;
    case LAST_TIME:
      break;
    case TERMINATOR:
      break;
    }
  }
  while (1);
}

nsresult
nsOperaCookieMigrator::ReadHeader()
{
  mStream->Read32(&mAppVersion);
  mStream->Read32(&mFileVersion);

  printf("*** app = %d, file = %d\n", mAppVersion, mFileVersion);
  if (mAppVersion & 0x1000 && mFileVersion & 0x2000) {
    mStream->Read8(&mTagTypeLength);
    mStream->Read16(&mPayloadTypeLength);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsOperaProfileMigrator::CopyHistory(PRBool aReplace)
{
  printf("*** copy opera history\n");

  nsCOMPtr<nsIBrowserHistory> hist(do_GetService(NS_GLOBALHISTORY_CONTRACTID));

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("global.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream);

  nsAutoString buffer, title, url;
  PRTime lastVisitDate;
  PRBool moreData = PR_FALSE;

  enum { TITLE, URL, LASTVISIT } state = TITLE;

  // Format is "title\nurl\nlastvisitdate"
  do {
    nsresult rv = lineStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv))
      return rv;

    switch (state) {
    case TITLE:
      title = buffer;
      state = URL;
      break;
    case URL:
      url = buffer;
      state = LASTVISIT;
      break;
    case LASTVISIT:
      // Opera time format is a second offset, PRTime is a microsecond offset
      PRInt32 err;
      lastVisitDate = buffer.ToInteger(&err);
      
      PRInt64 temp, million;
      LL_I2L(temp, lastVisitDate);
      LL_I2L(million, PR_USEC_PER_SEC);
      LL_MUL(lastVisitDate, temp, million);

      nsCAutoString urlStr; urlStr.AssignWithConversion(url);
      hist->AddPageWithDetails(urlStr.get(), title.get(), lastVisitDate);
      
      state = TITLE;
      break;
    }
  }
  while (moreData);

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
nsOperaProfileMigrator::CopyOtherData(PRBool aReplace)
{
  printf("*** copy opera other data\n");

  nsCOMPtr<nsIFile> temp;
  mOperaProfile->Clone(getter_AddRefs(temp));
  nsCOMPtr<nsILocalFile> historyFile(do_QueryInterface(temp));

  historyFile->Append(NS_LITERAL_STRING("download.dat"));

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), historyFile);
  if (!fileStream) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream);

  nsAutoString buffer;
  PRBool moreData = PR_FALSE;

  // Format is "title\nurl\nlastvisitdate"
  do {
    nsresult rv = lineStream->ReadLine(buffer, &moreData);
    if (NS_FAILED(rv))
      return rv;

    printf("*** download = %ws\n", buffer.get());
  }
  while (moreData);


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

