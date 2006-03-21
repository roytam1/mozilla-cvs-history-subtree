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
 * The Original Code is Membuf server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
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

#include "membufScreenManager.h"
#include "membufScreen.h"

NS_IMPL_ISUPPORTS1(membufScreenManager, nsIScreenManager)

membufScreenManager::membufScreenManager()
{
    NS_INIT_ISUPPORTS();
}

membufScreenManager::~membufScreenManager()
{
}

nsIScreen*
membufScreenManager::CreateNewScreenObject()
{
    nsIScreen* retval = nsnull;
    if (!mCachedMainScreen)
        mCachedMainScreen = new membufScreen();

    NS_IF_ADDREF(retval = mCachedMainScreen.get());

    return retval;
}


NS_IMETHODIMP
membufScreenManager::ScreenForRect(PRInt32 /*inLeft*/,
                                   PRInt32 /*inTop*/,
                                   PRInt32 /*inWidth*/,
                                   PRInt32 /*inHeight*/,
                                   nsIScreen **outScreen )
{
    GetPrimaryScreen(outScreen);
    return NS_OK;
}

NS_IMETHODIMP
membufScreenManager::GetPrimaryScreen(nsIScreen **aPrimaryScreen)
{
    *aPrimaryScreen = CreateNewScreenObject();
    return NS_OK;
}

NS_IMETHODIMP
membufScreenManager::GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
    *aNumberOfScreens = 1;
    return NS_OK;
}
