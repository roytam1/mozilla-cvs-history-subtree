/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsSoftwareUninstall_h__
#define nsSoftwareUninstall_h__

#include "nsISupports.h"
#include "nsIXPIPackageInfo.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsVoidArray.h"

#define NS_XPINSTALLPACKAGEINFO_CONTRACTID \
  "@mozilla.org/xpinstall/xpinstall-package-info;1"

#define KEY_DO_NOT_UNINSTALL            "(*dnu*) "
#define KEY_SHARED_FILE                 "(*shared_file*) "
#define KEY_XPCOM_COMPONENT             "(*xpcom-component*) "
#define KEY_INSTALL_FILE                "Installing: "
#define KEY_INSTALL_WIN_SHARED          "Installing Shared: " //wrong?
#define KEY_REPLACE_FILE                "Replacing: "
#define KEY_REPLACE_WIN_SHARED          "Replacing Shared: " // wrong??
#define KEY_CREATE_REG_KEY              "Create Registry Key: "
#define KEY_STORE_REG_VALUE_STR         "Store Registry Value String: "
#define KEY_STORE_REG_VALUE_NUM         "Store Registry Value Number: "
#define KEY_STORE_REG_VALUE             "Store Registry Value: "
#define KEY_CREATE_DIR                  "Create Folder: "
#define KEY_REGISTER_UNINSTALL_COMMAND  "Uninstall Command: "
#define KEY_WINDOWS_SHORTCUT            "Windows Shortcut: "
#define KEY_COPY_FILE                   "Copy File: "
#define KEY_MOVE_FILE                   "Move File: "
#define KEY_RENAME_FILE                 "Rename File: "
#define KEY_CREATE_FOLDER               "Create Folder: "
#define KEY_RENAME_DIR                  "Rename Dir: "

class nsSoftwareUninstall
{
public:
    nsSoftwareUninstall(nsCString *aRegName, nsCString *aPrettyName);

    nsresult PerformUninstall();
    nsresult GetInstalledPackages(nsISimpleEnumerator **aResult);

private:
    nsCString* mRegName;
    nsCString* mPrettyName;

    PRBool   PerformCopyFileDelete(nsCString anAction);
    void     PerformFileDelete(char *anAction, char* aKey, char *filename);
    void     PerformFileDoDeleteFile(char *aFilename);
    void     PerformFileDeleteShared(char *aFilename);
    void     PerformFileDeleteXPCOMComp(char *aFilename);
    void     PerformUninstallActions(nsVoidArray *aActions);
    nsresult GetUninstallLogContents(char **aBuf, PRInt32 *aSize);
    nsresult GetUninstallLogPath(nsString *aPath);
    PRBool   PerformRegDelete(nsCString anAction);

};

class nsXPIPackageInfo : public nsIXPIPackageInfo
{

    nsCString* mPrettyName;
    nsCString* mRegName;
    nsCString* mVersionString;    
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPIPACKAGEINFO

    nsXPIPackageInfo();
    nsXPIPackageInfo(nsCString *aPrettyName, nsCString *aRegName, nsCString *aVersionString);
    virtual ~nsXPIPackageInfo();
};

#endif /* nsSoftwareUninstall_h__ */
