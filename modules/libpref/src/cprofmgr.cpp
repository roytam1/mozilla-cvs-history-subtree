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
 
/*

  Profile Manager class

  In 4.5, Profile Information is stored in the Netscape XP registry.
  Each profile is stored in a separate subkey underneath the
  Users key.  The following is the structure in the XP registry:

  ROOTKEY_USERS/
     <ProfileName>/
        ProfileLocation  a local file (file:///) or remote (ldap://) URL to the info
        UpgradeFrom      if present, the profile was upgraded from a pre 4.5 profile
        LastLogin        a string which indicates the last login time (privacy issue?)
        

*/

#include "prtypes.h"
#ifdef NSPR20
#include "prio.h"
#else
#include "prfile.h"
#endif

#include "xp_core.h"
#include "xp_mcom.h"
#include "xp_str.h"

#include "nsreg.h"

#include "prefapi.h"
#include "prefpriv.h"
#include "cprofmgr.h"
#include "cprofile.h"
#include "clocal.h"

#include "secnav.h"

/* These are implemented natively for each platform */

extern char *  profmgr_NativeGetLastUser();
extern int     profmgr_NativeCountProfiles(void);
extern PRBool  profmgr_NativeProfileExists(const char *profileName);
extern int     profmgr_NativeCreateDirectory(const char *path);
extern int     profmgr_NativeCreateUserDirectories(const char *path);
extern char *  profmgr_NativeGetTemporaryProfileDirectory(void);
extern PRBool  profmgr_FilenameIsFullpath(const char *filename);
extern PRBool  profmgr_NativeNeedsUpgrade(void);
extern int     profmgr_NativeMarkUpgraded(const char *key);
extern int     profmgr_NativeDoUpgrade(CProfile *pProfile);
extern int     profmgr_NativeCopyStarterFiles(const char *userDir);
extern int     profmgr_NativeDeleteProfile(const char *profileName);
extern int     profmgr_NativeDeleteProfileDir(const char *dirpath);

extern int     profmgr_ShowPasswordDialog(uint16 style, char **pPassword);


VR_INTERFACE(REGERR) NR_RegCountSubkeys(
         HREG    hReg,        /* handle of open registry */
         RKEY    key,         /* containing key */
         uint32  *count,      /* number of entries */
         uint32  style        /* 0: children only; REGENUM_DESCEND: sub-tree */
    );

CProfileManager::CProfileManager(CProfile *workingProfile)
{
    m_WorkingProfile = workingProfile;  /* get a profile to work with */

    CleanupTemporaryProfiles(TRUE);

    return;
}


CProfileManager::~CProfileManager()
{
    return;
}


    PROFILE_ERROR
CProfileManager::CreateNewProfile(const char *profilePath, const char *profileName, XP_Bool temporary)
{
    PROFILE_ERROR   err = PREF_OK;
    char            *newProfPath = NULL;
    char            *suggestedName = (char *) profileName;
    PRFileInfo      fInfo;

    if (!suggestedName) {
        suggestedName = GenerateUniqueName();
    } else if (ProfileExists(profileName)) {
        return PREF_PROFILE_EXISTS;
    }

    newProfPath = GenerateProfileDirectory(profilePath, suggestedName, temporary);

    /* now that we've got a full directory path, make sure that it's legal and not
       overlapping with an existing directory. */

	if (!newProfPath) {
        err = PREF_OUT_OF_MEMORY;
    } else if ( PR_GetFileInfo(newProfPath, &fInfo) != -1 ) {
        err = PREF_PROFILE_EXISTS;
        /* actually, throw up a confirmation here and drop through if
           overwriting the existing directory */
    } else {
        REGERR      regErr = REGERR_OK;
        HREG        reg;
        RKEY        rkey;
        CPrefStore  *pNSCPCfg;

        if (!temporary) {
            /* Update the registry */

            regErr = NR_RegOpen(NULL, &reg);

            if (regErr == REGERR_OK) {
                err = NR_RegAddKey(reg, ROOTKEY_USERS, suggestedName, &rkey);

                if (err == REGERR_OK) {
                    err = NR_RegSetEntryString(reg, rkey, "ProfileLocation",
                        newProfPath);
                }
                
                NR_RegClose(reg);
            }
        }

        m_WorkingProfile->SetProfileName(suggestedName);

        /* create the profile directory */
        profmgr_NativeCreateDirectory(newProfPath);

        /* create the ancillary directories */
        profmgr_NativeCreateUserDirectories(newProfPath);

        /* copy the starter files */
        if (!temporary) {
            profmgr_NativeCopyStarterFiles(newProfPath);
        }

        /* clear out the preferences created by the default .cfg file */
        m_WorkingProfile->RemovePrefsStore("netscape.cfg", TRUE);

        /* add the various stores to the profile, including the netscape.cfg in the right place */
        pNSCPCfg = new CPrefStoreLockFile("netscape.cfg", "netscape.cfg", PREF_REQUIRED_STORE);
        m_WorkingProfile->AddPrefsStoreLast(pNSCPCfg);

        m_WorkingProfile->CreateAndAddLocalStores(newProfPath);

        SetProfilePrefs(suggestedName, newProfPath);
    }

    XP_FREEIF(newProfPath);

    if (suggestedName && (suggestedName != profileName)) {
        XP_FREE(suggestedName);
    }

    return err;
}

    PROFILE_ERROR
CProfileManager::CreateTemporaryProfile()
{
    PROFILE_ERROR       err = PREF_ERROR;
    char                *tempName = GenerateUniqueName();

    err = CreateNewProfile(tempName, tempName, TRUE);

    XP_FREEIF(tempName);
    return err;
}


    PROFILE_ERROR
CProfileManager::DoNewProfileWizard(uint16 dialogStyle)
{
    PROFILE_ERROR       err, err2;
    char                *profileDirectory;
    char                profileName[256];
    
    err2 = profmgr_ShowNewProfileWizard(dialogStyle, m_WorkingProfile, &profileDirectory);

    if (err2 >= PREF_OK) {
        m_WorkingProfile->GetProfileName(profileName, 255);
        err = CreateNewProfile(profileDirectory, profileName);
    } else {
        return err2;
    }

    if (err2 == PREF_PROFILE_UPGRADE) {
        err = profmgr_NativeDoUpgrade(m_WorkingProfile);
    }

    if (err == PREF_OK) {
        m_WorkingProfile->SavePrefs();
    }

    return err;
}

    PROFILE_ERROR
CProfileManager::ChooseProfile(uint16 style, const char *defaultName)
{
    XP_Bool         bShowManager;
    ProfDisplay     *pProfileInfo;
    ProfDisplay     *pSelectedProfile;
    char            *password = NULL;
    PROFILE_ERROR   err = PREF_OK;

    /* we only show profile selection if we're forced to or
       if there's more than one profile
       or if we always need to show the Guest option */

    bShowManager = (style & PROFMGR_FORCE_SHOW) || (GetProfileCount() > 1) ||
        AlwaysShowGuest();
    

    if (bShowManager) {
        XP_Bool     showProfMgr = FALSE;
        pProfileInfo = GetProfileDisplay(style);

        do {
            if (style & PROFMGR_SHOW_CONTROLS) {
                err = PROFMGR_ERROR_EDIT_PROFILE;
            } else {
                err = profmgr_ShowProfMgrDialog(style, (void *) pProfileInfo, (void **) &pSelectedProfile, &password);
                showProfMgr = FALSE;
            }

            if (err == PROFMGR_ERROR_EDIT_PROFILE) {
                err = profmgr_ShowProfMgrControls(this, style, (void *) pProfileInfo);

                style = (style & ~PROFMGR_SHOW_CONTROLS);

                if (err == PROFMGR_ERROR_EDIT_PROFILE) {
                    /* show...controls returns this if we're to return to the picker */
                    showProfMgr = TRUE;

                    /* profiles may have been added or deleted */
                    FreeProfDisplay(pProfileInfo);
                    pProfileInfo = GetProfileDisplay(style);

                    err = PREF_OK;
                } else if (err == PREF_OK) {
                    /* ... and this if we've created a new profile */

                    FreeProfDisplay(pProfileInfo);
                    pProfileInfo = NULL;
                    return err;
                }
            } else {

                if ((err == PREF_OK) && (pSelectedProfile->style == PROFINFO_GUEST)) {
                    err = profmgr_GetGuestLogin(this, style, (void **) &pSelectedProfile, &password);
                    if (err == PROFMGR_ERROR_CANCELLED) {
                        showProfMgr = TRUE;
                        err = PREF_OK;
                    }
                }
            }

            /* FIXME: validate that the local diretory exists */
        } while ((showProfMgr) && (err == PREF_OK));

        if (err == PREF_OK) {
            RegisterProfile(pSelectedProfile);
        
            if (pSelectedProfile->style != PROFINFO_GUEST) {
                m_WorkingProfile->SetProfileName(pSelectedProfile->profileName);

            
                /* add the stores and such */
                m_WorkingProfile->CreateAndAddLocalStores(pSelectedProfile->profileLocation);
                SetProfilePrefs(pSelectedProfile->profileName, pSelectedProfile->profileLocation);
            }
            
        } else {
            /* possibilities are error, cancel and new profile */
        }

        FreeProfDisplay(pProfileInfo);
        pProfileInfo = NULL;

    } else if (GetProfileCount() == 0) {
        /* no profiles */

        /* check for upgrade first */
        int         prevVersion = profmgr_NativeNeedsUpgrade();

        if (prevVersion && prevVersion < 4) {
            style |= PROFMGR_NEWPROF_UPGRADE;
        }

        return DoNewProfileWizard(style);
    } else {
        /* there's only one profile, so we select it (i.e., the first one) by default */
        pProfileInfo = GetProfileDisplay(style);

        RegisterProfile(pProfileInfo);
    
        if (pProfileInfo->style != PROFINFO_GUEST) {
            m_WorkingProfile->SetProfileName(pProfileInfo->profileName);

            /* add the stores and such */
            m_WorkingProfile->CreateAndAddLocalStores(pProfileInfo->profileLocation);
            SetProfilePrefs(pProfileInfo->profileName, pProfileInfo->profileLocation);
        }
    
        FreeProfDisplay(pProfileInfo);

        err = PromptPassword();
    
    }

    if ((err == PREF_OK) && password)
        m_WorkingProfile->SetProfilePassword(password);	

    return err;
}


    PROFILE_ERROR
CProfileManager::PromptPassword()
{
    char        *password;
    int         result;

    result = profmgr_ShowPasswordDialog(0, &password);
    
    if (result == 0) {
        m_WorkingProfile->SetProfilePassword(password);	
    }

    return result;
}


    PROFILE_ERROR
CProfileManager::DeleteProfile(const char *profileName, XP_Bool deleteFiles)
{
    HREG        reg;
    char        dirName[_MAX_PATH+1];
    RKEY        key;
    REGERR      err = REGERR_OK;

    /* get its registry entry */
    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegGetKey(reg, ROOTKEY_USERS, (char *) profileName, &key);

        if (err == REGERR_OK) {
            if ((deleteFiles) && (NR_RegGetEntryString(reg, key, "ProfileLocation",
                    dirName, _MAX_PATH) == REGERR_OK)) {

                profmgr_NativeDeleteProfileDir(dirName);
            }

            err = NR_RegDeleteKey(reg, ROOTKEY_USERS, (char *) profileName);            
        } else {
            err = profmgr_NativeDeleteProfile(profileName);
        }

        NR_RegClose(reg);
    }

    return err;
}


    PROFILE_ERROR
CProfileManager::GetProfileFromName(const char *profileName)
{
    PROFILE_ERROR   err = PREF_DOES_NOT_EXIST;
    char            *profilePath;

    profilePath = GetProfileDirFromName(profileName);

    if (profilePath) {
        m_WorkingProfile->SetProfileName(profileName);

        /* add the directories to it. */
        m_WorkingProfile->CreateAndAddLocalStores(profilePath);
        SetProfilePrefs(profileName, profilePath);

        err = PREF_OK;
    }

    return err;
}


    PROFILE_ERROR
CProfileManager::GetLastProfile()
{
    char *profileName;
        
    profileName = GetLastUserProfileName();

    return GetProfileFromName(profileName);
}


    int
CProfileManager::GetProfileCount()
{
    /* Starting with 4.5, the names of all of the profiles we
       track are stored in the Netscape Registry under the Users subkey.  In order
       to determine how many there are, we need only to count the number of subkeys
       underneath the Users subkey.  
       
       If there are no subkeys, we fall back on the 4.0x mechanisms which are platform
       dependent.
    */

    REGERR      err;
    HREG        reg;
    char        *retValue = NULL;
    uint32      count = 0;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegCountSubkeys(reg, ROOTKEY_USERS, &count, 0);

        NR_RegClose(reg);
    }

    /* need to fall back */

    count += profmgr_NativeCountProfiles();

    return count;
}


    char *
CProfileManager::GetProfileDir()
{
/* The algorithm for determining where the profiles live is as follows:

  1) Go to the last profile opened (if it exists) and, if it's a file-based profile,
     strip out the last component of its directory.

  2) For Windows, determine the location of the currently running executable and see if there's 
     a "Users" directory one level up from there.

     For Macintosh, check for the existence of a Netscape Users* directory in the 
     Preferences folder of the System folder

  3) For Windows, use the default "C:\\Program Files\\Netscape" for Win32 or "C:\\Netscape"
     for Win16.

     For Macintosh, use "Netscape Users"* in the Preferences folder.

  * localization adjusted

*/
    char *profileDir;
        
    profileDir = GetProfileDirFromName(GetLastUserProfileName());

    if (profileDir) {
	    char *pSeparator = strrchr(profileDir, PR_DIRECTORY_SEPARATOR);
	    if(pSeparator)
		    *pSeparator = '\0';
    } else {
        /* */
#ifdef XP_WIN32
			profileDir = "C:\\Program Files\\Netscape\\Users\\";
#else
			profileDir = "C:\\Netscape\\Users\\";
#endif
    }
    
    return XP_STRDUP(profileDir);
}

    char *
CProfileManager::GetTemporaryProfileDir()
{
    return profmgr_NativeGetTemporaryProfileDirectory();
}

    char *
CProfileManager::GenerateProfileDirectory(const char *profilePath, const char *profileName, XP_Bool temporary)
{
    int     i, maxLen;
    char    *newProfPath = NULL;
    char    *suggestedPath = (char *) profilePath;

    if (!profilePath) {
        /* first, derive an interesting path name from the profile name.
           The algorithm we use here is to take up to the first 8 alphanumeric
           characters from the profileName and, for consistency, convert them
           to lower case.  Non alphanumeric characters are converted to underscores */

        maxLen = (XP_STRLEN(profileName) > 8 ? 8 : XP_STRLEN(profileName));
    
        suggestedPath = (char *) XP_ALLOC((maxLen + 1) * sizeof(char));

        for (i=0; i < maxLen; i++) {
            if (isalnum(profileName[i])) {
                suggestedPath[i] = tolower(profileName[i]);
            } else {
                suggestedPath[i] = '_';
            }
        }
        suggestedPath[maxLen] = '\0';
    }

    if (suggestedPath) {
        /* first, figure out if it's a relative path */
        if (profmgr_FilenameIsFullpath(suggestedPath)) {
            newProfPath = XP_STRDUP(suggestedPath);
        } else {
            /* if not, append it to the standard profile path */
            char                *profDir;
            unsigned int        dirLen;

            if (!temporary)
                profDir = GetProfileDir();
            else
                profDir = GetTemporaryProfileDir();

            dirLen = XP_STRLEN(profDir);
            /* validate that it ends with the proper directory separator character' */

            XP_ASSERT(profDir[dirLen -1 ] == PR_DIRECTORY_SEPARATOR);

            newProfPath = (char *) XP_ALLOC((dirLen + XP_STRLEN(suggestedPath) + 1) * sizeof(char));
            XP_STRCPY(newProfPath, profDir);
            XP_STRCPY(&newProfPath[dirLen], suggestedPath);
            XP_FREE(profDir);
        }

        if (suggestedPath != profilePath) {
            XP_FREE(suggestedPath);
        }
    }

    return newProfPath;
}

    PROFILE_ERROR
CProfileManager::RegisterProfile(ProfDisplay *pProfileInfo)
{
    REGERR      err = PREF_OK;
    HREG        reg;
    RKEY        rkey;
    char        *retValue = NULL;

    XP_ASSERT(pProfileInfo);

    switch (pProfileInfo->style) {
        case PROFINFO_NATIVE:
            /* we want to move this from a native (i.e., HKey or Netscape Users resource)
               profile into the XP registry.  The key info is in the upgradeInfo pointer,
               and everything else is already extracted into pProfileInfo */
            
            err = NR_RegOpen(NULL, &reg);

            if (err == REGERR_OK) {
                err = NR_RegAddKey(reg, ROOTKEY_USERS, pProfileInfo->profileName, &rkey);

                if (err == REGERR_OK) {
                    err = NR_RegSetEntryString(reg, rkey, "ProfileLocation",
                        pProfileInfo->profileLocation);

                    err = NR_RegSetEntryString(reg, rkey, "UpgradeFrom", 
                        pProfileInfo->upgradeStatus);

                    profmgr_NativeMarkUpgraded(pProfileInfo->upgradeStatus);
                }
                
                NR_RegClose(reg);
            }
            break;
        case PROFINFO_GUEST:
            err = NR_RegOpen(NULL, &reg);

            if (err == REGERR_OK) {
                err = NR_RegAddKey(reg, ROOTKEY_USERS, pProfileInfo->profileName, &rkey);

                if (err == REGERR_OK) {
                    err = NR_RegSetEntryString(reg, rkey, "ProfileLocation",
                        pProfileInfo->profileLocation);

                    err = NR_RegSetEntryString(reg, rkey, "Temporary", "Temporary");

                }
                
                NR_RegClose(reg);
            }
            break;
        default:
            break;
    }

    return err;
}

    
    char *
CProfileManager::GetLastUserProfileName()
{
    /* The name of the last profile used on the machine is stored in the Netscape
       Registry as the LastNetscapeUser under the subkey Users.  
        
       If that fails, we fall back on the 4.0x mechanisms which are platform dependent
    */

    REGERR      err;
    HREG        reg;
    char        buf[MAXREGNAMELEN];
    uint32      bufSize = MAXREGNAMELEN;
    char        *retValue = NULL;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegGetEntryString(reg, ROOTKEY_USERS, "LastNetscapeUser", buf, bufSize);

        if (err == REGERR_OK) {
            retValue = XP_STRDUP(buf);
        }

        NR_RegClose(reg);
    }

    if (retValue == NULL) {
        /* Fall back */
        retValue = profmgr_NativeGetLastUser();
    }

    return retValue;
}


    char *
CProfileManager::GetProfileDirFromName(const char *profileName)
{
    HREG        reg;
    char        str256[256];
    char        *pDirectoryName = NULL;
    RKEY        key;
    REGERR      err = REGERR_OK;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegGetKey(reg, ROOTKEY_USERS, (char *) profileName, &key);
        
        if (err == REGERR_OK) {
            err = NR_RegGetEntryString(reg, key, "ProfileLocation",
                str256, 256);

            pDirectoryName = XP_STRDUP(str256);
        }

        NR_RegClose(reg);
    }

    if (!pDirectoryName) {
        /* fall back on native code */

    }

    return NULL;
}


    CProfileManager::ProfDisplay *
CProfileManager::GetProfileDisplay(uint16 style)
{
    HREG                            reg;
    char                            keyName[MAXREGNAMELEN];
    char                            str256[256];
    REGENUM                         state = NULL;
    RKEY                            key;
    REGERR                          err = REGERR_OK, err2 = REGERR_OK;
    CProfileManager::ProfDisplay    *pProfItem;
    CProfileManager::ProfDisplay    *pProfChain = NULL, *pProfLast=NULL, *pNativeChain = NULL;

    err = NR_RegOpen(NULL, &reg);

    while (err == REGERR_OK) {
        err = NR_RegEnumSubkeys(reg, ROOTKEY_USERS, &state, keyName, MAXREGNAMELEN, 0);

        if ((err == REGERR_OK) || (err == REGERR_NOMORE)) {
            err2 = NR_RegGetKey(reg, ROOTKEY_USERS, keyName, &key);
            
            if (err2 == REGERR_OK) {
                err2 = NR_RegGetEntryString(reg, key, "ProfileLocation",
                    str256, 256);

                if (err2 == REGERR_OK) {
                    pProfItem = (CProfileManager::ProfDisplay *) XP_ALLOC(sizeof(CProfileManager::ProfDisplay));

                    if (pProfChain == NULL) {
                        pProfChain = pProfItem;
                    }

                    pProfItem->profileName = XP_STRDUP(keyName);
                    pProfItem->profileLocation = XP_STRDUP(str256);
                    pProfItem->style = PROFINFO_LOCAL;
                    pProfItem->platformSpecificData = NULL;
                    pProfItem->upgradeStatus = NULL;
                    pProfItem->next = NULL;

                    if (pProfLast != NULL) {
                        pProfLast->next = pProfItem;
                    }

                    pProfLast = pProfItem;

                    err2 = NR_RegGetEntryString(reg, key, "UpgradeFrom",
                        str256, 256);
    
                    if (err2 == REGERR_OK) {
                        pProfItem->upgradeStatus = XP_STRDUP(str256);
                    }
                }
            }
        }
    }

    /* Fallback for any other profiles */
    profmgr_NativeGetProfileDisplay(style, (void **) &pNativeChain);

    if (pProfLast != NULL) {
        pProfLast->next = pNativeChain;
    }

    if (pProfChain == NULL) {
        pProfChain = pNativeChain;
    }

    while (pNativeChain != NULL) {
        pProfLast = pNativeChain;
        pNativeChain = pNativeChain->next;
    }

    /* if guest access is enabled, add that item */
    err = NR_RegGetEntryString(reg, ROOTKEY_USERS, "GuestAccess", str256, 256);

    if ((err == REGERR_BADLOCN) || (err == REGERR_NOFIND) || ((err == REGERR_OK) && (XP_STRCMP(str256, "never")))) {
        /* guest access is set as either "always, never, or allowed" */
        pProfItem = (CProfileManager::ProfDisplay *) XP_ALLOC(sizeof(CProfileManager::ProfDisplay));

        pProfItem->profileName = XP_STRDUP("Guest");    /* hardcoded string! */
        pProfItem->profileLocation = NULL;
        pProfItem->style = PROFINFO_GUEST;
        pProfItem->platformSpecificData = NULL;
        pProfItem->upgradeStatus = NULL;
        pProfItem->next = NULL;

        if (pProfLast == NULL) {
            pProfLast = pProfItem;
        } else {
            CProfileManager::ProfDisplay    *pSeparator;

            pSeparator = (CProfileManager::ProfDisplay *) XP_ALLOC(sizeof(CProfileManager::ProfDisplay));
            pSeparator->profileName = NULL;
            pSeparator->profileLocation = NULL;
            pSeparator->style = PROFINFO_SEPARATOR;
            pSeparator->platformSpecificData = NULL;
            pSeparator->upgradeStatus = NULL;
            pSeparator->next = pProfItem;

            pProfLast->next = pSeparator;
        }
    }
    
    NR_RegClose(reg);

    return pProfChain;
}

    PROFILE_ERROR
CProfileManager::SetLastUserProfileName(const char *profileName)
{
    /* The name of the last profile used on the machine is stored in the Netscape
       Registry as the LastNetscapeUser under the subkey Users.  
        
    */

    REGERR      err;
    HREG        reg;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegSetEntryString(reg, ROOTKEY_USERS, "LastNetscapeUser", (char *) profileName);

        NR_RegClose(reg);
    }

    return err;
}


    char *
CProfileManager::GenerateUniqueName()
{
    char        tempName[10];
    int         i = 0;

    XP_STRCPY(tempName, "nstemp");

    while (ProfileExists(tempName)) {
        PR_snprintf(tempName, 10, "nstemp%02d", i++);

        XP_ASSERT(i<100);
    }

    return XP_STRDUP(tempName);
}

    int
CProfileManager::CleanupTemporaryProfiles(XP_Bool deleteContent)
{
    HREG            reg;
    char            keyName[MAXREGNAMELEN];
    REGENUM         state = NULL;
    RKEY            key;
    REGERR          err = REGERR_OK;
    REGINFO         rInfo;
    int             count = 0;

    err = NR_RegOpen(NULL, &reg);
    rInfo.size = sizeof(REGINFO);

    while (err == REGERR_OK) {
        err = NR_RegEnumSubkeys(reg, ROOTKEY_USERS, &state, keyName, MAXREGNAMELEN, 0);

        if (err == REGERR_OK) {
            err = NR_RegGetKey(reg, ROOTKEY_USERS, keyName, &key);
            
            if (err == REGERR_OK) {
                err = NR_RegGetEntryInfo(reg, key, "Temporary", &rInfo);

                if (err == REGERR_OK) {
                    /* Here's a profile that shouldn't be here */
                    count++;
                    DeleteProfile(keyName, deleteContent);
                }
            }
        }
    }

    if (reg) {
        NR_RegClose(reg);
    }

    return count;
}



/* To check for the existence of a profile, we first check for the existence of 
   the key in the new xp registry.  If we don't find something, check the native methods */


    XP_Bool
CProfileManager::ProfileExists(const char *profileName)
{

    REGERR      err;
    HREG        reg;
    RKEY        key;
    XP_Bool     retValue = FALSE;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
        err = NR_RegGetKey(reg, ROOTKEY_USERS, (char *) profileName, &key);

        if (err == REGERR_OK) {
            /* It's possible that the user thought deleting the files would
               delete the profile.  Let's let them believe that. */
            PRFileInfo      fInfo;
            char            profilePath[_MAX_PATH];

            err = NR_RegGetEntryString(reg, key, "ProfileLocation", profilePath, _MAX_PATH);

            if ((err == REGERR_OK) && (PR_GetFileInfo(profilePath, &fInfo) != -1 )) {
                retValue = TRUE;
            } else {
                /* Well, no files, but there's still a registry entry.
                   Let's clean that up the registry */

            err = NR_RegDeleteKey(reg, ROOTKEY_USERS, (char *) profileName);
            }
        }

        NR_RegClose(reg);
    }

    if (!retValue) {
        /* Fall back */
        retValue = profmgr_NativeProfileExists(profileName);
    }

    return retValue;
}

    XP_Bool
CProfileManager::AlwaysShowGuest()
{
    HREG        reg;
    char        str256[256];
    REGERR      err = REGERR_OK;
    XP_Bool     bShowGuest = TRUE;

    err = NR_RegOpen(NULL, &reg);

    if (err == REGERR_OK) {
       err = NR_RegGetEntryString(reg, ROOTKEY_USERS, "GuestAccess", str256, 256);

       if ((err == REGERR_OK) && (!XP_STRCMP(str256, "always"))) {
            bShowGuest = TRUE;
       }
            
       NR_RegClose(reg);
    }

    return bShowGuest;
}

    void
CProfileManager::SetProfilePrefs(const char *name, const char *path)
{
    PREF_SetDefaultCharPref( "profile.directory", path );

    SetLastUserProfileName(name);

}

    void
CProfileManager::FreeProfDisplay(ProfDisplay *pItems)
{
    CProfileManager::ProfDisplay  *pCurrentItem = pItems;
    CProfileManager::ProfDisplay  *pLastItem;

    while (pCurrentItem) {

        XP_FREEIF(pCurrentItem->profileName);
        XP_FREEIF(pCurrentItem->profileLocation);
        XP_FREEIF(pCurrentItem->upgradeStatus);
        XP_FREEIF(pCurrentItem->platformSpecificData);
        
        pLastItem = pCurrentItem;
        pCurrentItem = pCurrentItem->next;

        XP_FREE(pLastItem);
    }

    return;
}



/* temporarily in here until we move it to the registry code */

VR_INTERFACE(REGERR) NR_RegCountSubkeys(
         HREG    hReg,        /* handle of open registry */
         RKEY    key,         /* containing key */
         uint32  *count,      /* number of entries */
         uint32  style        /* 0: children only; REGENUM_DESCEND: sub-tree */
       )
{
    char        keyName[MAXREGNAMELEN];
    REGENUM     state = NULL;
    REGERR      err = REGERR_OK;

    *count = 0;
    while (err == REGERR_OK) {
        err = NR_RegEnumSubkeys(hReg, key, &state, keyName, MAXREGNAMELEN, style);
        (*count)++;
    }

    return err;
}

