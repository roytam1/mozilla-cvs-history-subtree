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
 
#ifndef operaprofilemigrator___h___
#define operaprofilemigrator___h___

#include "nsCOMPtr.h"
#include "nsIBinaryInputStream.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsISupportsArray.h"
#include "nsString.h"
#include "nsVoidArray.h"

class nsILocalFile;
class nsINIParser;
class nsIPrefBranch;

class nsOperaProfileMigrator : public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsOperaProfileMigrator();
  virtual ~nsOperaProfileMigrator();

public:

  typedef enum { STRING, INT, BOOL, COLOR } PrefType;

  typedef nsresult(*prefConverter)(void*, nsIPrefBranch*);

  typedef struct {
    char*         sectionName;
    char*         keyName;
    PrefType      type;
    char*         targetPrefName;
    prefConverter prefSetterFunc;
    PRBool        prefHasValue;
    union {
      PRInt32     intValue;
      PRBool      boolValue;
      char*       stringValue;
    };
  } PREFTRANSFORM;

  static nsresult SetFile(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetCookieBehavior(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetImageBehavior(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetBool(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetWString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetInt(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetString(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult CopyPreferences(PRBool aReplace);
  nsresult ParseColor(nsINIParser* aParser, char* aSectionName, char** aResult);
  nsresult CopyUserContentSheet(nsINIParser* aParser);

  nsresult CopyCookies(PRBool aReplace);
  nsresult CopyHistory(PRBool aReplace);
  nsresult CopyFormData(PRBool aReplace);
  nsresult CopyPasswords(PRBool aReplace);
  nsresult CopyHotlist(PRBool aReplace);
  nsresult CopyOtherData(PRBool aReplace);

  void     GetOperaProfile(const PRUnichar* aProfile, nsILocalFile** aFile);

private:
  nsCOMPtr<nsILocalFile> mOperaProfile;
  nsCOMPtr<nsISupportsArray> mProfiles;
};

class nsOperaCookieMigrator
{
public:
  nsOperaCookieMigrator(nsIBinaryInputStream* aStream);
  virtual ~nsOperaCookieMigrator() { };

  nsresult Migrate();

  typedef enum { OPEN_DOMAIN  = 0x01, 
                 DOMAIN_NAME  = 0x1E,
                 SEGMENT_INFO = 0x03,
                 COOKIE_ID    = 0x10,
                 COOKIE_DATA  = 0x11,
                 EXPIRY_TIME  = 0x12,
                 LAST_TIME    = 0x13,
                 TERMINATOR   = 0x9B } TAG;

protected:
  nsresult ReadHeader();

private:
  nsCOMPtr<nsIBinaryInputStream> mStream;

  nsVoidArray mDomainStack;
  nsVoidArray mTagStack;

  PRUint32 mAppVersion;
  PRUint32 mFileVersion;
  PRUint8  mTagTypeLength;
  PRUint16 mPayloadTypeLength;
};

#endif

