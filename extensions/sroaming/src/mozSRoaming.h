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

/* Central object for session roaming. Controls program sequence and provides
   common functions. */

#ifndef _MOZSROAMING_H_
#define _MOZSROAMING_H_


#include "nsISessionRoaming.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIRegistry.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsVoidArray.h"


class mozSRoamingProtocol;

class mozSRoaming: public nsISessionRoaming
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISESSIONROAMING

    mozSRoaming();
    virtual ~mozSRoaming();

    // Is a roaming profile (if not, then nothing to do)
    PRBool IsRoaming();

    // Which profile files should be stored on the server
    // @return pointer to the internal nsDeque object. Do not free it.
    nsCStringArray* FilesToRoam();

    // Local profile directory
    nsCOMPtr<nsIFile> ProfileDir();

    // Method used to store remote files
    // 0=unknown, 1=HTTP, 2=Copy
    PRInt32 Method();

    /* If we'd normally overwrite a newer file. Ask user, which file to keep.
     * @param download  direction: true = download, false = upload
     * @param files     conflicting files
     * @param result    Files for which conflicts should be ignored.
     *                  This is a subset of the files param and those should
     *                  be uploaded / downloaded. The rest of files in the
     *                  files param should *not* be uploaded / downloaded.
     *                  Wants an array passed in, items will be added to that.
     * @return NS_ERROR_ABORT, if the user clicked Cancel.
     */
    nsresult ConflictResolveUI(PRBool download, const nsCStringArray& files,
                               nsCStringArray* result);

    nsCOMPtr<nsIRegistry> Registry();
    nsRegistryKey RegistryTree();

    /* At the time we attempt to upload, the network lib has already been
       shut down. So, temporarily restore it and then close it down again.
       Of course, this is a hack, until we (me, ccarlen, darin) have found a
       better solution and it has been tested thoroughly for regressions.
       This problem currently doesn't appear during download (at startup).

       @param restore  if true, then restore, otherwise close
       @param topic  the notification topic.
                     either "profile-change-net-restore"
                     or "profile-change-net-teardown"
    */
    nsresult RestoreCloseNet(PRBool restore);

protected:
    // Data (see getters above)
    PRBool mIsRoaming;
    PRInt32 mMethod;
    nsCStringArray mFiles;

    // Cache
    PRBool mHavePrefs;
    nsCOMPtr<nsIRegistry> mRegistry;

protected:

    // Reads liprefs: is roaming profile?, files, server info etc.
    nsresult ReadRoamingPrefs();

    // Factory method for a new method handler that can handle this method
    // We'll use a new object for down-/upload respectively
    // @return new object. you have to free it with delete.
    mozSRoamingProtocol* CreateMethodHandler();

    /* When all roaming prefs, including the method-specific ones, are read,
       this method must be called to close down the prefs system
       (otherwise, the main app will use the roaming prefs file) */ 
    void PrefsDone();
};

#endif
