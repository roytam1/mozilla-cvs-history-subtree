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

#include "mozSRoamingStream.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsXPIDLString.h"
#include "nsIIOService.h"
#include "nsNetUtil.h"

// UI
#include "nsIDialogParamBlock.h"
#include "nsIDOMWindow.h"
#include "nsIWindowWatcher.h"
#include "plstr.h"

// IID and CIDs of all the services needed
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#define kRegTreeStream (NS_LITERAL_STRING("Stream"))
#define kRegKeyURL (NS_LITERAL_STRING("BaseURL"))
#define kRegKeyUsername (NS_LITERAL_STRING("Username"))
#define kRegKeyPassword (NS_LITERAL_STRING("Password"))
#define kRegKeySavePassword (NS_LITERAL_CSTRING("SavePassword"))

#define kXferDlg "chrome://sroaming/content/transfer/progressDialog.xul"

// Internal helper functions unrelated to class

void AppendElements(nsCStringArray& target, nsCStringArray& source)
{
    for (PRInt32 i = source.Count() - 1; i >= 0; i--)
        target.AppendCString(*source.CStringAt(i));
}


mozSRoamingStream::mozSRoamingStream()
{
    printf("mozSRoamingStream ctor\n");
}

mozSRoamingStream::~mozSRoamingStream()
{
    printf("mozSRoamingStream dtor\n");
}



/*
 * nsSRoamingMethod implementation
 */

nsresult mozSRoamingStream::Init(mozSRoaming* aController)
{
    printf("mozSRoamingStream::Init\n");
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
                          kRegTreeStream.get(),
                          &regkey);
    if (NS_FAILED(rv))
    {
        printf("ERROR: Streaming (FTP, HTTP etc.) method for roaming "
               "not set up for this profile\n");
        return rv;
    }
    mRegkeyStream = regkey;

    nsXPIDLString remoteUrlPref;
    rv = registry->GetString(regkey, kRegKeyURL.get(),
                             getter_Copies(remoteUrlPref));
    if (NS_FAILED(rv))
    {
        printf("registry read of base URL failed: error 0x%x\n", rv);
        return rv;
    }
    mRemoteBaseUrl = remoteUrlPref;

    /* For easier mass-personalisation via scripts in corporate setups and
       for easier instructed setup by normal users,
       we allow the username to be separate from the URL. */
    nsXPIDLString usernamePref;
    rv = registry->GetString(regkey, kRegKeyUsername.get(),
                             getter_Copies(usernamePref));
    if (NS_SUCCEEDED(rv) && !usernamePref.IsEmpty() )
        /* failure is non-fatal, because username might be in the URL, and
           even if it isn't, the transfer will pull up a prompt. */
    {
      nsCOMPtr<nsIIOService> ioserv = do_GetService(kIOServiceCID, &rv);
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr<nsIURI> uri;
        rv = ioserv->NewURI(NS_ConvertUCS2toUTF8(mRemoteBaseUrl),nsnull,nsnull,
                            getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv))
        {
          rv = uri->SetUsername(NS_ConvertUCS2toUTF8(usernamePref));
          if (NS_SUCCEEDED(rv))
          {
            // Yikes. Did I mention that I hate that C/wide string crap?
            nsXPIDLCString spec;
            uri->GetSpec(spec);
            mRemoteBaseUrl = NS_ConvertUTF8toUCS2(spec);
            mRemoteBaseUrl.ReplaceSubstring(NS_LITERAL_STRING("$USERID").get(),
                                            usernamePref.get());
          }
        }
      }
    }

    // we need a / at the end (base URL), fix up, if user error
    if (mRemoteBaseUrl.Last() != (PRUnichar) '/')
      mRemoteBaseUrl += (PRUnichar) '/';

    nsXPIDLString passwordPref;
    rv = registry->GetString(regkey, kRegKeyPassword.get(),
                             getter_Copies(passwordPref));
    /* failure is non-fatal, because username if there's none, the
       transfer will pull up a prompt. */
    mPassword = passwordPref;

    PRInt32 savepw = 0;
    rv = registry->GetInt(regkey, kRegKeySavePassword.get(),
                          &savepw);
    mSavePassword = savepw == 0 ? PR_FALSE : PR_TRUE;

    nsCOMPtr<nsIFile> profiledir = mController->ProfileDir();
    if (!profiledir)
        return NS_ERROR_FILE_NOT_FOUND;

    nsCOMPtr<nsIIOService> ioserv = do_GetService(kIOServiceCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ioserv->NewFileURI(profiledir, getter_AddRefs(mProfileDir));
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLCString path;
    mProfileDir->GetSpec(path);
    printf("profile dir: -%s-\n", path.get());
    printf("remote dir: -%s-\n", NS_ConvertUCS2toUTF8(mRemoteBaseUrl).get());
    printf("password: -%s-\n", NS_ConvertUCS2toUTF8(mPassword).get());
    printf("save password: -%s-\n", mSavePassword?"yes":"no");

    return NS_OK;
}

nsresult mozSRoamingStream::Download()
{
    printf("mozSRoamingStream::Download\n");
    return DownUpLoad(PR_TRUE);

}

nsresult mozSRoamingStream::Upload()
{
    printf("mozSRoamingStream::Upload\n");
    return DownUpLoad(PR_FALSE);
}

nsresult mozSRoamingStream::DownUpLoad(PRBool download)
{
    nsresult rv = NS_OK;
    if (!mController)
    	return NS_ERROR_UNEXPECTED;

    /* Design

       I'd really like to
       - open a few channels (one per file) here in this function,
       - attach |nsIProgressEventSink|s to each and
       - let a Progress dialog track/reflect the progress sinks.
       That way, I would have a generic dialog for the UI somewhere
       and the logic (transfer) here in this file, where it belongs.

       Unfortunately, this doesn't work. I have to
       block this function until the transfer is done, but have the transfer
       itself and the progress UI working. darin says, I will get threading
       issues then. He suggested to initiate the transfer from the dialog
       (because that dialog uses another event quene) and let the dialog
       close itself when the transfer is done. Here in this function, I can
       block until the dialog is dismissed.
       (At least, that's what I understood from darin.)
       That's what I do, and it seems to work. It's still a poor design.

       Actually, it's really crap. It destroys my whole design with the
       pluggable "method"s (like stream, copy etc.). Basically everything
       contacting remote servers (i.e. needing a progress dialog) now has to
       either mess around in sroamingTransfer.js or duplicate a certain
       portion of code.
    */

    nsCOMPtr<nsIWindowWatcher> windowWatcher
              (do_GetService("@mozilla.org/embedcomp/window-watcher;1", &rv));
    if (NS_FAILED(rv))
        return rv;

    /* nsIDialogParamBlock is a method to pass ints and strings
       to and from XUL dialogs.
       To dialog (upon open)
         Int array
           Item 0: 1 = download, 2 = upload
           Item 1: 1 = serial (transfer one file after the other),
                   2 = parallel (start all file transfers at once)
           Item 2: Number of files (n below)
           Item 3: Save password (should pw be stored by default when asked?)
                   1 = true
                   0 = false
         String array
           Item 0: unused
           Item 1: URL of profile dir
           Item 2: URL of remote dir
           Item 3: Password
           Item 4..(n+3): filenames
       From dialog (upon close)
         Int array
           Item 0: 0 = do nothing
                   1 = Save Password (user checked the box in the prompt)
                   2 = Set SavePassword pref to false, ignore PW/Usern. retval
         String array
           Item 0: Saved username (as entered in the prompt)
           Item 1: Saved password (as entered in the prompt)
     */
    nsCOMPtr<nsIDialogParamBlock> ioParamBlock
             (do_CreateInstance("@mozilla.org/embedcomp/dialogparam;1", &rv));
    if (NS_FAILED(rv))
        return rv;

    // download/upload
    ioParamBlock->SetInt(0, download ? 1 : 2);
    ioParamBlock->SetInt(1, 2);

    nsCStringArray* files = mController->FilesToRoam();
    printf("got %d files\n", files->Count());
    ioParamBlock->SetInt(2, files->Count());

    ioParamBlock->SetInt(3, mSavePassword ? 1 : 0);

    nsXPIDLCString profile;
    mProfileDir->GetSpec(profile);
    ioParamBlock->SetString(1, NS_ConvertUTF8toUCS2(profile).get());
    ioParamBlock->SetString(2, mRemoteBaseUrl.get());
    ioParamBlock->SetString(3, mSavePassword
                               ? mPassword.get()
                               : NS_LITERAL_STRING("").get());

    // filenames
    for (PRInt32 i = files->Count() - 1; i >= 0; i--)
    {
      printf("want to transfer file %s\n", (*files->CStringAt(i)).get());
      NS_ConvertASCIItoUCS2 filename(*files->CStringAt(i));
      ioParamBlock->SetString(i + 4, filename.get());
                                                // filenames start at item 4
    }

    printf("Opening window...\n");
    nsCOMPtr<nsIDOMWindow> window;
    rv = windowWatcher->OpenWindow(nsnull,
                                   kXferDlg,
                                   nsnull,
                                   "centerscreen,chrome,modal,titlebar",
                                   ioParamBlock,
                                   getter_AddRefs(window));
    if (NS_FAILED(rv))
        return rv;

    printf("Back from dialog\n");

    PRInt32 savepw = 0;
    ioParamBlock->GetInt(0, &savepw);
    printf("got save password: -%d-\n", savepw);
   	
    if (savepw == 1)
    {    
      nsXPIDLString password, username;
      ioParamBlock->GetString(0, getter_Copies(username));
      ioParamBlock->GetString(1, getter_Copies(password));
      printf("got username: -%s-\n", NS_ConvertUCS2toUTF8(username).get());
      printf("got password: -%s-\n", NS_ConvertUCS2toUTF8(password).get());

      mPassword = password;
      printf("will save password:-%s-\n",NS_ConvertUCS2toUTF8(password).get());
      nsCOMPtr<nsIRegistry> registry = mController->Registry();
      rv = registry->SetInt(mRegkeyStream, kRegKeySavePassword.get(),  1);
      rv = registry->SetString(mRegkeyStream, kRegKeyUsername.get(),
                               username.get());
      rv = registry->SetString(mRegkeyStream, kRegKeyPassword.get(),
                               mPassword.get());
      // failure is not fatal. then it's not saved *shrug*. ;-)
    }

    printf("Done\n");
    
    return NS_OK;
}
