/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/********************************************************************/
 
#define _NSPR_NO_WINDOWS_H
#include "cprofile.h"
#include "cprofmgr.h"
#include "cremote.h"
#include "CProfDlg.h"
#include "cnewprof.h"
#include "cguest.h"
#include "../prefpriv.h"
#include <afx.h>
#include <afxdlgs.h>
#include "res/resource.h"

#include "xp_mcom.h"
#include "xp_str.h"

extern int     profmgr_NativeDoUpgrade(CProfile *pProfile, int moveFileOption);

    int
profmgr_ShowNewProfileWizard(uint16 style, CProfile *pWorkingProfile, char **pProfileDirectory)
{
    PROFILE_ERROR       err = PREF_OK;
    CNewProfileSheet    *pWizard;
    CString             profileName, profileDir, csDialogValue, csShortMailName;
    int                 mailType;

    if ((style & PROFMGR_NEWPROF_TYPE_MASK) == PROFMGR_NEWPROF_NETWORK) {
    	XP_Bool     bShowProfWizard = FALSE;

        PREF_GetDefaultBoolPref("netinst.profile.show_profile_wizard", &bShowProfWizard);
        if (!bShowProfWizard || 
            ((style & PROFMGR_NEWPROF_FLAGS_MASK) & PROFMGR_NEWPROF_UPGRADE)) {
            style = PROFMGR_DEFAULT | (style & PROFMGR_NEWPROF_FLAGS_MASK);
        }
    }

    pWizard = new CNewProfileSheet(pWorkingProfile, style);
    err = pWizard->DoDialog();

    if ((err == PREF_OK) || (err == PREF_PROFILE_UPGRADE)) {
        profileName = pWizard->GetProfileName();
        pWorkingProfile->SetProfileName(profileName);

        ASSERT(pProfileDirectory);

        profileDir = pWizard->GetProfileDirectory();
        *pProfileDirectory = XP_STRDUP(profileDir); 
    }

#ifndef MOZ_LITE
    if (err == PREF_OK) {
        csDialogValue = pWizard->GetMailUsername();
        if (!csDialogValue.IsEmpty()) {
            pWorkingProfile->SetPref("mail.identity.username", csDialogValue);
        }

        csDialogValue = pWizard->GetUserEmail();
        if (!csDialogValue.IsEmpty()) {
            pWorkingProfile->SetPref("mail.identity.useremail", csDialogValue);
        }

        csDialogValue = pWizard->GetSMTPServer();
        if (!csDialogValue.IsEmpty()) {
            pWorkingProfile->SetPref("network.hosts.smtp_server", csDialogValue);
        }

        csDialogValue = pWizard->GetNNTPServer();
        if (!csDialogValue.IsEmpty()) {
            pWorkingProfile->SetPref("network.hosts.nntp_server", csDialogValue);
            pWorkingProfile->SetPref("news.server_port", (int32) pWizard->GetNNTPPort());
            pWorkingProfile->SetPref("news.server_is_secure", pWizard->GetNewsIsSecure());
        } else {
            pWorkingProfile->SetPref("network.hosts.nntp_server", "");
            pWorkingProfile->SetPref("news.server_port", (int32) NEWS_PORT);
            pWorkingProfile->SetPref("news.server_is_secure", PR_FALSE);
        }

        csShortMailName = pWizard->GetMailUsername();
        if (!csShortMailName.IsEmpty()) {
        	int iAtSign = csShortMailName.Find('@');
            
            if (iAtSign != -1) {
                csShortMailName = csShortMailName.Left(iAtSign);
            }
        } else {
            csShortMailName = "default";
        }

        mailType = pWizard->GetMailServerType();

		PREF_SetIntPref("mail.server_type", mailType);

        if (mailType == MSG_Pop3) {
            csDialogValue = pWizard->GetMailServer();
            if (!csDialogValue.IsEmpty()) {
                pWorkingProfile->SetPref("network.hosts.pop_server", csDialogValue);
            }

			pWorkingProfile->SetPref("mail.pop_name", csShortMailName);
			pWorkingProfile->SetPref("mail.leave_on_server", PR_FALSE);
        } else {
            CString         csServerPrefName = "mail.imap.server.";

            csDialogValue = pWizard->GetMailServer();
            if (!csDialogValue.IsEmpty()) {
                PREF_AppendListPref("network.hosts.imap_servers", csDialogValue);
            }
            csServerPrefName += csDialogValue;
            csServerPrefName += ".userName";

            pWorkingProfile->SetPref(csServerPrefName, csShortMailName);
            pWorkingProfile->SetPref("mail.leave_on_server", PR_TRUE);
        }

    }
#endif

    if (err == PREF_PROFILE_UPGRADE) {
        if (pWizard->CopyUpgradedFiles())
            pWorkingProfile->AddFlag(PROFILE_COPY_UPGRADED_FILES);
    }

    delete pWizard;

    return err;
}


    int
profmgr_ShowProfMgrDialog(uint16 style, void *pItems, void **selectedItem, char **pPassword)
{
    int         dialogReturn;
    int         result = PREF_OK;
    char        *lastProfileName = NULL;

    CUserProfileDialog            *dlg = new CUserProfileDialog(pItems);

    lastProfileName = CProfileManager::GetLastUserProfileName();
    dlg->SetDefaultName(lastProfileName);

	dialogReturn = dlg->DoModal();

    if (dialogReturn == IDOK) {
        *pPassword = XP_STRDUP(dlg->GetSuppliedPassword());
        *selectedItem = dlg->GetSelectedProfile();
    } else {
        *pPassword = NULL;
        *selectedItem = NULL;

        if (dialogReturn == IDC_PROFILE_EDIT) {
            result = PROFMGR_ERROR_EDIT_PROFILE;
        } else {
            result = PROFMGR_ERROR_CANCELLED;
        }
    }

    XP_FREEIF(lastProfileName);
    return result;
}

    int
profmgr_GetGuestLogin(CProfileManager *pProfMgr, uint16 style, void **pSelectedProfile, char **pPassword)
{
    int                     dialogReturn;
    int                     error;
    CProfile                *pProfile;
    CProfileGuestLogin      *pDlg;
    CPrefStoreRemoteLDAP    *pRemoteStore;
    char                    profileName[_MAX_PATH];
    int                     liFlags = 0;
    int                     length = _MAX_PATH;

    pDlg = new CProfileGuestLogin;

    
    dialogReturn = pDlg->DoModal();

    if (dialogReturn == IDOK) {
        *pPassword = XP_STRDUP(pDlg->GetSuppliedPassword());
    } else {
        *pPassword = NULL;
        return PROFMGR_ERROR_CANCELLED;
    }

    if (pDlg->m_StoreLocalProfile) {
        /* The user wants to create a locally cached remote profile */

        error = pProfMgr->DoNewProfileWizard(PROFMGR_NEWPROF_REMOTE);
    } else {
        error = pProfMgr->CreateTemporaryProfile();
    }

    if (error == PREF_OK) {

    	PREF_SetBoolPref("li.enabled",TRUE);

        pRemoteStore = new CPrefStoreRemoteLDAP("Remote_Profile");
        pRemoteStore->SetUsername(pDlg->m_Username);
        PREF_SetCharPref("li.login.name",pDlg->m_Username);

        if (pDlg->UseLDAPLI()) {
            pRemoteStore->SetLIProtocol(PROF_REMOTE_LI_USELDAP);
            pRemoteStore->SetURL(pDlg->m_LDAPServer);
            pRemoteStore->SetLISearchBase(pDlg->m_LDAPSearchBase);
        } else if (pDlg->UseHTTPLI()) {
            pRemoteStore->SetLIProtocol(PROF_REMOTE_LI_USEHTTP);
            pRemoteStore->SetURL(pDlg->m_HTTPServer);
        }

        if (pDlg->m_AddressBook) {
            liFlags |= PROF_REMOTE_LI_ADDRESSBOOK;
        }

        if (pDlg->m_Bookmarks) {
            liFlags |= PROF_REMOTE_LI_BOOKMARKS;
        }

        if (pDlg->m_Cookies) {
            liFlags |= PROF_REMOTE_LI_COOKIES;
        }

        if (pDlg->m_MailFilters) {
            liFlags |= PROF_REMOTE_LI_FILTERS;
        }

        if (pDlg->m_Java) {
            liFlags |= PROF_REMOTE_LI_JAVA;
        }

        if (pDlg->m_Security) {
            liFlags |= PROF_REMOTE_LI_SECURITY;
        }

        if (pDlg->m_History) {
            liFlags |= PROF_REMOTE_LI_HISTORY;
        }

        if (pDlg->m_Prefs) {
            liFlags |= PROF_REMOTE_LI_PREFS;
        }

        pRemoteStore->SetLIParams(liFlags);

        pProfile = pProfMgr->GetWorkingProfile();
        pProfile->AddPrefsStoreBefore(pRemoteStore, "netscape.cfg");

        pProfile->GetProfileName(profileName, _MAX_PATH);
        XP_FREEIF(((CProfileManager::ProfDisplay *) (*pSelectedProfile))->profileName);

        ((CProfileManager::ProfDisplay *) (*pSelectedProfile))->profileName = XP_STRDUP(profileName);

        pProfile->GetCharPref("profile.directory", profileName, &length);
        XP_FREEIF(((CProfileManager::ProfDisplay *) (*pSelectedProfile))->profileLocation);
        ((CProfileManager::ProfDisplay *) (*pSelectedProfile))->profileLocation = XP_STRDUP(profileName);

    }

    /* FIXME: Potential Leak */
/*    delete pDlg;*/

    if (dialogReturn == IDCANCEL)
        return PROFMGR_ERROR_CANCELLED;
    else 
        return error;
}

    int
profmgr_ShowProfMgrControls(CProfileManager *pProfMgr, uint16 style, void *pItems)
{
    int         dialogReturn;
    int         result = PROFMGR_ERROR_EDIT_PROFILE;

    CProfileMgrDlg  *dlg = new CProfileMgrDlg(pProfMgr, pItems);

	dialogReturn = dlg->DoModal();

    if (dialogReturn == IDC_NEW) {
        pProfMgr->DoNewProfileWizard(PROFMGR_DEFAULT);
        result = PREF_OK;
    }

    return result;
}

    int
profmgr_ShowPasswordDialog(uint16 style, char **pPassword)
{
    CNewProfPasswordDlg     dlg;
    int                     dialogReturn, result = PREF_OK;

	dialogReturn = dlg.DoModal();

    if (dialogReturn == IDOK) {
        *pPassword = XP_STRDUP(dlg.GetSuppliedPassword());
    } else {
        *pPassword = NULL;
        result = PROFMGR_ERROR_CANCELLED;
    }

    return result;
}


