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
 
#ifndef __CPROFMGR_H
#define __CPROFMGR_H

#include "prtypes.h"
#include "cprofile.h"

#define PROFMGR_DEFAULT         0

/* Profile Manager Styles */

#define PROFMGR_FORCE_SHOW      4
#define PROFMGR_SHOW_CONTROLS   8
#define PROFMGR_USE_DIALUP      16

/* New Profile Dialog styles */

#define PROFMGR_NEWPROF_MINIMAL 1
#define PROFMGR_NEWPROF_NETWORK (PROFMGR_NEWPROF_MINIMAL)
#define PROFMGR_NEWPROF_PE      3
#define PROFMGR_NEWPROF_REMOTE  4

/* The following can be ANDed with any of the above styles */

#define PROFMGR_NEWPROF_UPGRADE 512

/* The following are masks that will get either the profile 
   dialog or flags */

#define PROFMGR_NEWPROF_TYPE_MASK   0x3F
#define PROFMGR_NEWPROF_FLAGS_MASK  0xFFC0

/* Profile Types */

#define PROFINFO_LOCAL          1
#define PROFINFO_NATIVE         2   /* a local profile derived from the native users implementation */
#define PROFINFO_NETWORK        3   /* a network installation; Unix and Win NT specific */
#define PROFINFO_REMOTE         4
#define PROFINFO_GUEST          5
#define PROFINFO_SEPARATOR      6

/* Profile Manager Errors */

#define PROFMGR_ERROR_CANCELLED     -1001
#define PROFMGR_ERROR_EDIT_PROFILE   1001


class PR_PUBLIC_API(CProfileManager) {
protected:

    typedef struct _ProfDisplay {
        char            *profileName;
        char            *profileLocation;
        uint16          style;
        char            *upgradeStatus;
        void            *platformSpecificData;
        _ProfDisplay    *next;
    } ProfDisplay;

private:
    CProfile            *m_WorkingProfile;

protected:
    char                *selectedProfileName;

public:
                            CProfileManager(CProfile *profile);
    virtual                 ~CProfileManager(void);


    virtual PROFILE_ERROR   CreateNewProfile(const char *profilePath, const char *profileName = NULL, PRBool temporary = PR_FALSE);
    virtual PROFILE_ERROR   CreateTemporaryProfile();
    virtual PROFILE_ERROR   DoNewProfileWizard(uint16 style=PROFMGR_DEFAULT);
    virtual PROFILE_ERROR   ChooseProfile(uint16 style, const char *defaultName = NULL);
    virtual PROFILE_ERROR   PromptPassword(void);
    virtual PROFILE_ERROR   DeleteProfile(const char *profileName, PRBool deleteFiles = PR_FALSE);
    virtual PROFILE_ERROR   GetProfileFromName(const char *profileName);
    virtual PROFILE_ERROR   GetLastProfile(void);

    CProfile *              GetWorkingProfile(void) {return m_WorkingProfile;}
    static int              GetProfileCount(void);
    static char *           GetProfileDir(void);
    static char *           GenerateProfileDirectory(const char *profilePath, const char *profileName, PRBool temporary = PR_FALSE);
    static char *           GetLastUserProfileName(void);

protected:
    PROFILE_ERROR           RegisterProfile(CProfile &profile);
    PROFILE_ERROR           RegisterProfile(ProfDisplay *pProfileInfo);
    PROFILE_ERROR           UnregisterProfile(CProfile &profile);

    static char *           GetProfileDirFromName(const char *profileName);
    static char *           GetTemporaryProfileDir(void);
    ProfDisplay *           GetProfileDisplay(uint16 style);

    PROFILE_ERROR           SetLastUserProfileName(const char *profileName);

           char *           GenerateUniqueName(void);
    int                     CleanupTemporaryProfiles(PRBool deleteContent);
    PRBool                  ProfileExists(const char *profileName);
    PRBool                  AlwaysShowGuest(void);

    void                    SetProfilePrefs(const char *name, const char *path);

protected:
    static void             FreeProfDisplay(ProfDisplay *pItems);

public:
    friend int              profmgr_ShowProfMgrDialog(uint16 style, void *pItems, void **selectedItem, char **password);
    friend int              profmgr_ShowProfMgrControls(CProfileManager *pProfMgr, uint16 style, void *pItems);
    friend int              profmgr_ShowNewProfileWizard(uint16 style, CProfile *workingProfile, char **pProfileDirectory);
    friend void             profmgr_NativeGetProfileDisplay(uint16 style, void **pItems);
    friend int              profmgr_GetGuestLogin(CProfileManager *pProfileManager,
                                uint16 style, void **pSelectedProfile, char **password);

#ifdef XP_PC
    friend class            CUserProfileDialog;
    friend class            CProfileMgrDlg;
#endif

};

#endif
