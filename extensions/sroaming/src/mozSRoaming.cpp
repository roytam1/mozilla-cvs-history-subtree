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
 * The Original Code is Mozilla Roaming code.
 *
 * The Initial Developer of the Original Code is 
 * Ben Bucksch <http://www.bucksch.org> of
 * Beonex <http://www.beonex.com>
 * Portions created by the Initial Developer are Copyright (C) 2002-2003
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
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "mozSRoaming.h"
#include "nsIComponentManager.h"
#include "nsIRegistry.h"
#include "nsIProfile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "mozSRoamingCopy.h"
#include "mozSRoamingStream.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"

// UI
#include "nsIDialogParamBlock.h"
#include "nsIDOMWindow.h"
#include "nsIWindowWatcher.h"
#include "plstr.h"

#define kRegTreeProfile (NS_LITERAL_STRING("Profiles"))

#define kRegTreeRoaming (NS_LITERAL_STRING("Roaming"))
#define kRegKeyEnabled (NS_LITERAL_CSTRING("Enabled"))
#define kRegKeyMethod (NS_LITERAL_STRING("Method"))
#define kRegKeyFiles (NS_LITERAL_STRING("Files"))
#define kRegValMethodStream (NS_LITERAL_STRING("stream"))
#define kRegValMethodCopy (NS_LITERAL_STRING("copy"))

#define kConflDlg "chrome://sroaming/content/transfer/conflictResolve.xul"


NS_IMPL_ISUPPORTS1(mozSRoaming,
                   nsISessionRoaming)

mozSRoaming::mozSRoaming()
    : mFiles(10)
{
    mHavePrefs = PR_FALSE;
    mIsRoaming = PR_FALSE;
    mMethod = 0;
}

mozSRoaming::~mozSRoaming()
{
}



/*
 * mozISRoaming implementation
 */

NS_IMETHODIMP
mozSRoaming::BeginSession()
{
printf("\n\n\n!!!! beginsession\n\n\n\n");
    nsresult rv = NS_OK;

    if (!mHavePrefs)
        rv = ReadRoamingPrefs();
    if (NS_FAILED(rv))
        printf("count not get prefs - error 0x%x\n", rv);

    if (!mIsRoaming)
        return NS_OK;

    mozSRoamingProtocol* proto = CreateMethodHandler();
    if (!proto)
        return NS_ERROR_ABORT;

    rv = proto->Init(this);
    if (NS_FAILED(rv))
    {
        printf("error 0x%x\n", rv);
        return rv;
    }

    PrefsDone();

    rv = proto->Download();
    if (NS_FAILED(rv))
    {
        printf("error 0x%x\n", rv);
        return rv;
    }

    delete proto;
    return NS_OK;
}

NS_IMETHODIMP
mozSRoaming::EndSession()
{
printf("\n\n\n!!!! endsession\n\n\n\n");
    nsresult rv = NS_OK;

    if (!mHavePrefs)
        rv = ReadRoamingPrefs();
    if (NS_FAILED(rv))
        printf("count not get prefs - error 0x%x\n", rv);

    if (!mIsRoaming)
        return NS_OK;

    mozSRoamingProtocol* proto = CreateMethodHandler();
    if (!proto)
        return NS_ERROR_ABORT;

    rv = proto->Init(this);
    if (NS_FAILED(rv))
    {
        printf("error 0x%x\n", rv);
        return rv;
    }

    PrefsDone();

    RestoreCloseNet(PR_TRUE);

    rv = proto->Upload();
    if (NS_FAILED(rv))
    {
        printf("error 0x%x\n", rv);
        return rv;
    }

    RestoreCloseNet(PR_FALSE);

    delete proto;
    return NS_OK;
}

NS_IMETHODIMP
mozSRoaming::IsRoaming(PRBool *_retval)
{
    if (!mHavePrefs)
        ReadRoamingPrefs();

    *_retval = IsRoaming();
    return NS_OK;
}



/*
 * Public functions
 */


PRBool mozSRoaming::IsRoaming()
{
    return mIsRoaming;
}

nsCStringArray* mozSRoaming::FilesToRoam()
{
    return &mFiles;
}

nsCOMPtr<nsIFile> mozSRoaming::ProfileDir()
{
    nsCOMPtr<nsIFile> result;
    NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                           getter_AddRefs(result));
    return result;
}

PRInt32 mozSRoaming::Method()
{
    return mMethod;
}

nsresult
mozSRoaming::ConflictResolveUI(PRBool download, const nsCStringArray& files,
                               nsCStringArray* result)
{
    printf("mozSRoaming::CheckConflicts\n");
    //nsCStringArray* result = new nsCStringArray();
    if (files.Count() < 1)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsIWindowWatcher> windowWatcher
      //       (do_GetService("@mozilla.org/embedcomp/window-watcher;1", &rv));
                             (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return rv;

    /* nsIDialogParamBlock is a method to pass ints and strings
       to and from XUL dialogs.
       To dialog (upon open)
         Int array
           Item 0: 1 = download, 2 = upload
           Item 1: Number of files (n below)
         String array
           Item 1..(n): Either filename or comma-separated (no spaces) string:
                        - filename
                        - last modified (unix time) server
                        - size (bytes) server
                        - last modified (unix time) local
                        - size (bytes) local
                        e.g. "bookmarks.html,100024563,325,100024535,245" or
                        "bookmarks.html"
       From dialog (upon close)
         Int array
           Item 0:      3 = OK, 4 = Cancel
           Item 1..(n): if OK:
                        1 = Use server version, 2 = Use local version.
                        For each file. Indices are the same as To/String
     */
    nsCOMPtr<nsIDialogParamBlock> ioParamBlock
             (do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1", &rv));
    if (NS_FAILED(rv))
        return rv;

    // download/upload
    ioParamBlock->SetInt(0, download ? 1 : 2);

    // filenames
    ioParamBlock->SetInt(1, files.Count());
    PRInt32 i;
    for (i = files.Count() - 1; i >= 0; i--)
    {
        NS_ConvertASCIItoUCS2 filename(*files.CStringAt(i));
        ioParamBlock->SetString(i + 1, filename.get());
    }

    nsCOMPtr<nsIDOMWindow> window;
    rv = windowWatcher->OpenWindow(nsnull,
                                   kConflDlg,
                                   nsnull,
                                   "centerscreen,chrome,modal,titlebar",
                                   ioParamBlock,
                                   getter_AddRefs(window));
    if (NS_FAILED(rv))
        return rv;

    PRInt32 value = 0;
    ioParamBlock->GetInt(0, &value);
    printf("got back: %s - %d\n", files.CStringAt(i)->get(), value);
    if (value != 3 && value != 4)
      return NS_ERROR_INVALID_ARG;
    if (value == 4) // cancel
    {
      printf("Cancel clicked\n");
      return NS_ERROR_ABORT;
    }
    printf("OK clicked\n");

    /* I am assuming that the sequence of iteration here is the same as in the
       last |for| statement. If that is not true, the indices gotten from
       param block will not match the array and we will interpret the result
       wrongly. */
    for (i = files.Count() - 1; i >= 0; i--)
    {
        ioParamBlock->GetInt(i + 1, &value);
        printf("got back: %s - %d\n", files.CStringAt(i)->get(), value);
        if (value != 1 && value != 2)
            return NS_ERROR_INVALID_ARG;
        if (download
            ? value == 1
            : value == 2)
            result->AppendCString(*files.CStringAt(i));
    }

    //*result = files;
    printf("CheckConflicts done\n");
    return NS_OK;
}


nsCOMPtr<nsIRegistry> mozSRoaming::Registry()
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIRegistry> registry(do_CreateInstance(NS_REGISTRY_CONTRACTID,
                                                     &rv));
    if (NS_FAILED(rv))
        return nsnull;
    rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    if (NS_FAILED(rv))
        return nsnull;
    return registry;
}

nsRegistryKey mozSRoaming::RegistryTree()
{
    nsresult rv = NS_OK;
    nsRegistryKey regkey = 0;

    nsXPIDLString profile;
    nsCOMPtr<nsIProfile> profMan(do_GetService(NS_PROFILE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return 0;
    rv = profMan->GetCurrentProfile(getter_Copies(profile));
    if (NS_FAILED(rv))
        return 0; 

    nsCOMPtr<nsIRegistry> registry = Registry();
    if (NS_FAILED(rv))
        return 0;

    rv = registry->GetKey(nsIRegistry::Common,
                          kRegTreeProfile.get(),
                          &regkey);
    if (NS_FAILED(rv))
    {
        printf("registry read of Profiles failed: error 0x%x\n", rv);
        return 0;
    }
    rv = registry->GetKey(regkey,
                          profile.get(),
                          &regkey);
    if (NS_FAILED(rv))
    {
        printf("registry read of current profile failed: error 0x%x\n", rv);
        return 0;
    }
    rv = registry->GetKey(regkey,
                          kRegTreeRoaming.get(),
                          &regkey);
    if (NS_FAILED(rv))
    {
        printf("Roaming not set up for this profile\n");
        return 0;
    }
    return regkey;
}


/*
 * Internal functions
 */

nsresult mozSRoaming::ReadRoamingPrefs()
{
    printf("mozSRoaming::ReadRoamingPrefs\n");
    nsresult rv = NS_OK;
    nsCOMPtr<nsIRegistry> registry = Registry();
    if (!registry)
        return NS_ERROR_UNEXPECTED;
    nsRegistryKey regkey = RegistryTree();

    if (!regkey)
    {
        mIsRoaming = PR_FALSE;
    }
    else
    {
        PRInt32 enabled;
        rv = registry->GetInt(regkey, kRegKeyEnabled.get(),
                              &enabled);
        if (NS_FAILED(rv))
        {
            printf("registry read of Enabled failed: error 0x%x\n", rv);
            mIsRoaming = PR_FALSE;
            return rv;
        }
        mIsRoaming = enabled == 0 ? PR_FALSE : PR_TRUE;
    }

    printf("roaming enabled: %s\n", mIsRoaming?"Yes":"No");
    if (!mIsRoaming)
      return rv;

    // Method
    nsXPIDLString proto;
    rv = registry->GetString(regkey, kRegKeyMethod.get(),
                             getter_Copies(proto));
    if (NS_FAILED(rv))
    {
        printf("registry read of Method failed: error 0x%x\n", rv);
        return rv;
    }
    if (proto == kRegValMethodStream)
        mMethod = 1;
    else if (proto == kRegValMethodCopy)
        mMethod = 2;
    printf("method: %d\n", mMethod);

    // Files
    nsXPIDLString files_reg;
    rv = registry->GetString(regkey, kRegKeyFiles.get(),
                             getter_Copies(files_reg));
    if (NS_FAILED(rv))
    {
        printf("registry read of Files failed: error 0x%x\n", rv);
        return rv;
    }
    NS_ConvertUCS2toUTF8 files_pref(files_reg);
    printf("files pref: -%s-\n", files_pref.get());

    mFiles.ParseString(files_pref.get(), ",");

    nsCStringArray* files = FilesToRoam();
    for (PRInt32 i = files->Count() - 1; i >= 0; i--)
    {
        nsCString& file = *files->CStringAt(i);
        printf(" have file -%s-\n", file.get());
    }

    printf("leaving readprefs\n");
    return NS_OK;
}

void mozSRoaming::PrefsDone()
{
    //    mPrefService->ResetPrefs();
}


mozSRoamingProtocol* mozSRoaming::CreateMethodHandler()
{
    printf("mozSRoaming::CreateMethodHandler\n");
    if (mMethod == 1)
        return new mozSRoamingStream;
    else if (mMethod == 2)
        return new mozSRoamingCopy;
    else // 0=unknown, e.g. prefs not yet read, or invalid
        return 0;
}

/* A workaround for the fact that the network library is shut down during
   profile shutdown (for dynamic profile changing), *before* the
   app is told to write down profile changes and thus before we try to upload,
   so our upload will fail.
   So, I just power netlib up again and then power it down again by sending
   the corresponding profile change notification. Might not be the
   best solution, but has small impact on existing code and WFM. */
nsresult mozSRoaming::RestoreCloseNet(PRBool restore)
{
  const char* topic = restore ? "profile-change-net-restore"
                              : "profile-change-net-teardown";
  printf("%s...\n", topic);
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService
                       (do_GetService("@mozilla.org/observer-service;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> subject(do_GetService(NS_PROFILE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->NotifyObservers(subject, topic,
                                        NS_LITERAL_STRING("switch").get());
  printf("%s done\n", topic);
  return rv;
}
