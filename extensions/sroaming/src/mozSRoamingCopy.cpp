/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Mozilla Roaming code.
 *
 * The Initial Developer of the Original Code is 
 * Ben Bucksch <http://www.bucksch.org> of
 * Beonex <http://www.beonex.com>
 * Portions created by the Initial Developer are Copyright (C) 2002
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

#include "mozSRoamingCopy.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsXPIDLString.h"

// IID and CIDs of all the services needed
static NS_DEFINE_CID(kSRoamingCID, MOZ_SROAMING_CID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

#define kRegTreeCopy (NS_LITERAL_STRING("Copy"))
#define kRegKeyRemote (NS_LITERAL_STRING("RemoteDir"))

// Internal helper functions unrelated to class

nsresult CopyFile(nsCOMPtr<nsIFile> fromDir,
                  nsCOMPtr<nsIFile> toDir,
                  nsAString& fileSubPath)
    // XXX: works for subpaths or just filenames?
{
    nsresult rv;

    printf("4.1\n");
    nsCOMPtr<nsIFile> fromFile;
    rv = fromDir->Clone(getter_AddRefs(fromFile));
    if (NS_FAILED(rv))
        return rv;
    printf("4.2\n");
    rv = fromFile->Append(fileSubPath);
    if (NS_FAILED(rv))
        return rv;

    printf("4.3\n");
    nsCOMPtr<nsIFile> toFileOld;
    rv = toDir->Clone(getter_AddRefs(toFileOld));
    if (NS_FAILED(rv))
        return rv;
    printf("4.4\n");
    rv = toFileOld->Append(fileSubPath);
    if (NS_FAILED(rv))
        return rv;

    printf("4.5\n");
    PRBool exists;
    rv = fromFile->Exists(&exists);
    if (NS_FAILED(rv))
        return rv;
    printf("4.6\n");

    nsXPIDLCString path1, path2;
    fromFile->GetNativePath(path1);
    toFileOld->GetNativePath(path2);
    printf("trying to copy from -%s- to -%s-\n", path1.get(), path2.get());

    if (exists)
    {
        printf("4.7\n");
        rv = toFileOld->Remove(PR_FALSE);  // XXX needed?
        if (NS_FAILED(rv))
            printf("couldn't remove\n");
        printf("4.8\n");
        rv = fromFile->CopyTo(toDir, fileSubPath);
    }
    else
    {
        printf("source file not found\n");
        rv = NS_ERROR_FILE_NOT_FOUND;
    }

	return rv;
}

void AppendElementsToStrArray(nsCStringArray& target, nsCStringArray& source)
{
    for (PRInt32 i = source.Count() - 1; i >= 0; i--)
        target.AppendCString(*source.CStringAt(i));
}


mozSRoamingCopy::mozSRoamingCopy()
{
    printf("mozSRoamingCopy ctor\n");
}

mozSRoamingCopy::~mozSRoamingCopy()
{
    printf("mozSRoamingCopy dtor\n");
}



/*
 * nsSRoamingProtocol implementation
 */

nsresult mozSRoamingCopy::Init(mozSRoaming* aController)
{
    printf("mozSRoamingCopy::Init\n");
    nsresult rv;
    mController = aController;
    if (!mController)
        return NS_ERROR_INVALID_ARG;

    // Get prefs
    nsCOMPtr<nsIRegistry> registry = mController->Registry();
    if (!registry)
        return NS_ERROR_UNEXPECTED;
    nsRegistryKey regkey = mController->RegistryTree();
    rv = registry->GetKey(regkey,
                          kRegTreeCopy.get(),
                          &regkey);
    if (NS_FAILED(rv))
    {
        printf("ERROR: File copy method for roaming "
               "not set up for this profile\n");
        return rv;
    }

    nsXPIDLString remoteDirPref;
    rv = registry->GetString(regkey, kRegKeyRemote.get(),
                             getter_Copies(remoteDirPref));
    if (NS_FAILED(rv))
    {
        printf("registry read of remote dir failed: error 0x%x\n", rv);
        return rv;
    }

    printf("3.1\n");
    nsCOMPtr<nsILocalFile> lf;  // getting around dumb getter
    rv = NS_NewNativeLocalFile(NS_ConvertUCS2toUTF8(remoteDirPref), PR_FALSE,
                         getter_AddRefs(lf));
    if (NS_FAILED(rv))
        return rv;
    printf("3.5\n");
    mRemoteDir = do_QueryInterface(lf, &rv);
    if (NS_FAILED(rv))
        return rv;

    printf("3.6\n");
    nsXPIDLCString path;
    rv = lf->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;
    printf("remote dir: -%s-\n", path.get());

    mProfileDir = mController->ProfileDir();
    if (!mProfileDir)
        return NS_ERROR_FILE_NOT_FOUND;

    printf("3.7\n");
    mProfileDir->GetNativePath(path);
    printf("profile dir: -%s-\n", path.get());

    return NS_OK;
}

nsresult mozSRoamingCopy::Download()
{
    printf("mozSRoamingCopy::Download\n");
    return DownUpLoad(PR_TRUE);
}

nsresult mozSRoamingCopy::Upload()
{
    printf("mozSRoamingCopy::Upload\n");
    return DownUpLoad(PR_FALSE);
}

nsresult mozSRoamingCopy::DownUpLoad(PRBool download)
{
    nsresult rv = NS_OK;

    nsCStringArray* files = mController->FilesToRoam();
    printf("got %d files\n", files->Count());

    // Check for conflicts
    nsCStringArray conflicts(10);
    nsCStringArray copyfiles(10);
    PRInt32 i;
    for (i = files->Count() - 1; i >= 0; i--)
    {
        nsCString& file = *files->CStringAt(i);
        printf("checking file -%s-\n", file.get());
        NS_ConvertASCIItoUCS2 fileL(file);

        nsCOMPtr<nsIFile> profileFile;
        printf("2.1\n");
        rv = mProfileDir->Clone(getter_AddRefs(profileFile));
        if (NS_FAILED(rv))
            return rv;
        printf("2.2\n");
        rv = profileFile->Append(fileL);
        if (NS_FAILED(rv))
            return rv;

        printf("2.3\n");
        nsCOMPtr<nsIFile> remoteFile;
        rv = mRemoteDir->Clone(getter_AddRefs(remoteFile));
        if (NS_FAILED(rv))
            return rv;
        printf("2.4\n");
        rv = remoteFile->Append(fileL);
        if (NS_FAILED(rv))
            return rv;

        printf("2.5\n");
        PRInt64 profileTime, remoteTime;
        profileFile->GetLastModifiedTime(&profileTime);
        remoteFile->GetLastModifiedTime(&remoteTime);
        //printf("mod time profile: %qd, remote: %qd\n", profileTime, remoteTime);

        // do we have a conflict?
        if (download
            ? profileTime > remoteTime
            : profileTime < remoteTime )
        {
            printf("conflict found!\n");
            conflicts.AppendCString(file);
        }
        else
            copyfiles.AppendCString(file);
    }

    // Ask user about conflicts
    nsCStringArray copyfiles_conflicts(10);
    rv = mController->ConflictResolveUI(download, conflicts,
                                        &copyfiles_conflicts);
    if (NS_FAILED(rv))
        return rv;
    AppendElementsToStrArray(copyfiles, copyfiles_conflicts);

    // Copy
    printf("copying %d files\n", copyfiles.Count());
    for (i = copyfiles.Count() - 1; i >= 0; i--)
    {
        nsCString& file = *copyfiles.CStringAt(i);
        printf("copying file -%s-\n", file.get());
        NS_ConvertASCIItoUCS2 fileL(file);
        if (download)
            rv = CopyFile(mRemoteDir, mProfileDir, fileL);
        else
            rv = CopyFile(mProfileDir, mRemoteDir, fileL);
        if (NS_FAILED(rv))
            printf("Copy of file -%s- failed!\n", file.get());
    }

    return rv;
}
