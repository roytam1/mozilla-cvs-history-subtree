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
#include "prtypes.h"
#ifdef NSPR20
#include "prio.h"
#else
#include "prfile.h"
#endif

#include "prefapi.h"
#include "cprofmgr.h"

#include "assert.h"

#include "afxwin.h"

#include "xp_mcom.h"
#include "xp_str.h"


#include <direct.h>

#define NOT_NULL(X)	X
#ifdef XP_ASSERT
#undef XP_ASSERT
#endif

#define XP_ASSERT(X) assert(X)
#define LINEBREAK "\n"

/* Functions defined in this file */

char *  profmgr_NativeGetLastUser();
int     profmgr_NativeCountProfiles(void);
void    profmgr_NativeGetProfileDisplay(uint16 style, void **pItems);
PRBool  profmgr_NativeProfileExists(const char *profileName);
int     profmgr_NativeCreateDirectory(const char *path);
int     profmgr_NativeCreateUserDirectories(const char *path);
char *  profmgr_NativeGetTemporaryProfileDirectory(void);
PRBool  profmgr_FilenameIsFullpath(const char *filename);
PRBool  profmgr_NativeNeedsUpgrade(void);
int     profmgr_NativeMarkUpgraded(const char *key);
int     profmgr_NativeDoUpgrade(CProfile *pProfile, int moveFileOption);
int     profmgr_NativeDeleteProfile(const char *profileName);
int     profmgr_NativeDeleteProfileDir(const char *dirpath);
int     profmgr_NativeRecursivelyDeleteDirectory(const char *dirpath);

extern  int     login_UpdateFilesToNewLocation(const char * path,
                                               CWnd *pParent, BOOL bCopyDontMove);
extern  int     login_UpdatePreferencesToJavaScript(const char * path);

#include "afxdllx.h"

static AFX_EXTENSION_MODULE extensionDLL;

extern "C" int APIENTRY 
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      // Extension DLL one-time initialization 
      if (!AfxInitExtensionModule(
             extensionDLL, hInstance))
         return 0;

      // TODO: perform other initialization tasks here
   }
   else if (dwReason == DLL_PROCESS_DETACH)
   {
      // Extension DLL per-process termination
      AfxTermExtensionModule(extensionDLL);

          // TODO: perform other cleanup tasks here
   }
   return 1;   // ok
}
 

/////////////////////////////////////////////////////////////////////////////

// Exported DLL initialization is run in context of running application
extern "C" void WinInitPrefs()
{
	// create a new CDynLinkLibrary for this app
	new CDynLinkLibrary(extensionDLL);
	// nothing more to do
}


#ifdef XP_WIN32

    char *
profmgr_NativeGetLastUser()
{
	long result;
    DWORD type, size;
	HKEY hKeyRet;
    const char *csSub = "SOFTWARE\\Netscape\\Netscape Navigator\\Users\\";

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						csSub,
						NULL,
						KEY_QUERY_VALUE,
						&hKeyRet);

	if (result != ERROR_SUCCESS) {
		return NULL;
	}

	// see how much space we need
	result = RegQueryValueEx(hKeyRet,
							  (char *) "CurrentUser",
							  NULL,
							  &type,
							  NULL,
							  &size);

	// if we didn't find it error!
	if((result != ERROR_SUCCESS) || (size == 0))
		return NULL;

	// allocate space to hold the string
	char * pString = (char *) XP_ALLOC(size * sizeof(char));

	// actually load the string now that we have the space
	result = RegQueryValueEx(hKeyRet,
							  (char *) "CurrentUser",
							  NULL,
							  &type,
							  (LPBYTE) pString,
							  &size);

	if((result != ERROR_SUCCESS) || (size == 0)) {
        XP_FREE(pString);
		return NULL;
    }

	if (hKeyRet) RegCloseKey(hKeyRet);

    return pString;
}

#elif defined XP_WIN16
    char *
profmgr_NativeGetLastUser()
{
    auto char ca_iniBuff[_MAX_PATH];
	CString csNSCPini;
	login_GetIniFilePath(csNSCPini);

	if (::GetPrivateProfileString("Users Additional Info", "CurrentUser","",ca_iniBuff,_MAX_PATH,csNSCPini))
		return XP_STRDUP(ca_iniBuff);
	else return NULL;
}
#endif

/* profmgr_NativeCountProfiles on the windows platform enumerates through the Windows
   system registry and counts subkeys beneath SOFTWARE\\Netscape\\Netscape Navigator\\Users\\
   and beneath , the NT network install subkey */

    int
profmgr_NativeCountProfiles(void)
{
	int         idx = 0, profiles = 0;
	long        result;
    const char  *csNetSub = "SOFTWARE\\Netscape\\Netscape Navigator\\UserInfo";
    const char  *csSub = "SOFTWARE\\Netscape\\Netscape Navigator\\Users\\";
    CString     keyName = csSub;
	char        lpBuf[MAX_PATH + 1];
	HKEY        hKeyRet, hSubkey;

    /* First, check for an NT network profile */

	result = RegOpenKeyEx(HKEY_CURRENT_USER,
						csNetSub,
						NULL,
						KEY_READ,
						&hKeyRet);
    
	if (result == ERROR_SUCCESS) {
        idx++;
	}

    if (hKeyRet) RegCloseKey(hSubkey);

    /* Then, check for "regular" profiles */

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						csSub,
						NULL,
						KEY_READ,
						&hKeyRet);
    
	if (result != ERROR_SUCCESS) {
		return profiles;
	}

    while(RegEnumKey(hKeyRet, idx, lpBuf, MAX_PATH+1) == ERROR_SUCCESS) {
        keyName = csSub;
        keyName += lpBuf;

	    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						    keyName,
						    NULL,
						    KEY_QUERY_VALUE,
						    &hSubkey);

        if (result == ERROR_SUCCESS) {
            result = RegQueryValueEx(hKeyRet,
			    				     (char *) "Obsoleted",
				    			     NULL,
					    		     NULL,
						    	     NULL,
							         NULL);
            if (result != ERROR_SUCCESS) {
                profiles++;
            }
        }

        if (hSubkey) RegCloseKey(hSubkey);

        idx++;
    }

	if (hKeyRet) RegCloseKey(hKeyRet);

    return profiles;
}


    void
profmgr_NativeGetProfileDisplay(uint16 style, void **pItems)
{
	int                             idx = 0, profiles = 0;
    DWORD                           size;
	long                            result;
    const char                      *csNetSub = "SOFTWARE\\Netscape\\Netscape Navigator\\UserInfo\\";
    const char                      *csSub = "SOFTWARE\\Netscape\\Netscape Navigator\\Users\\";
    CString                         keyName = csSub;
	char                            lpBuf[MAX_PATH + 1];
	HKEY                            hKeyRet, hSubkey;
    CProfileManager::ProfDisplay    *pProfItem;
    CProfileManager::ProfDisplay    *pNativeStart = NULL, *pProfLast=NULL;

    *pItems = NULL;

    /* First, see if a networked installation exists here */

	result = RegOpenKeyEx(HKEY_CURRENT_USER,
						csNetSub,
						NULL,
						KEY_READ,
						&hKeyRet);
    
	if (result == ERROR_SUCCESS) {

        pProfItem = (CProfileManager::ProfDisplay *) XP_ALLOC(sizeof(CProfileManager::ProfDisplay));

		size = MAX_PATH;
        ::GetUserName(lpBuf, &size); 

        pProfItem->profileName = XP_STRDUP(lpBuf);
        pProfItem->profileLocation = NULL;

        result = RegQueryValueEx(hSubkey, (char *) "DirRoot",
				    			 NULL, NULL, NULL, &size);
        
        if (result == ERROR_SUCCESS) {
            size++;
            pProfItem->profileLocation = (char *) XP_ALLOC(size * sizeof(char));
            
            result = RegQueryValueEx(hSubkey, (char *) "DirRoot",
				    			     NULL, NULL, (LPBYTE) pProfItem->profileLocation, &size);
        }

        pProfItem->style = PROFINFO_NETWORK;
        pProfItem->platformSpecificData = NULL;
        pProfItem->upgradeStatus = XP_STRDUP(keyName);
        pProfItem->next = NULL;

        if (pNativeStart == NULL) {
            pNativeStart = pProfItem;
        }

        if (pProfLast != NULL) {
            pProfLast->next = pProfItem;
        }

        pProfLast = pProfItem;
    }

    if (hKeyRet) RegCloseKey(hKeyRet);

    /* Then check for "regular" profiles */

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						csSub,
						NULL,
						KEY_READ,
						&hKeyRet);
    
	if (result != ERROR_SUCCESS) {
		return;
	}

    while(RegEnumKey(hKeyRet, idx, lpBuf, MAX_PATH+1) == ERROR_SUCCESS) {
        keyName = csSub;
        keyName += lpBuf;

	    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						    keyName,
						    NULL,
						    KEY_QUERY_VALUE,
						    &hSubkey);

        if (result == ERROR_SUCCESS) {
            result = RegQueryValueEx(hSubkey,
			    				     (char *) "Obsoleted",
				    			     NULL,
					    		     NULL,
						    	     NULL,
							         NULL);

            if (result != ERROR_SUCCESS) {

                pProfItem = (CProfileManager::ProfDisplay *) XP_ALLOC(sizeof(CProfileManager::ProfDisplay));

                pProfItem->profileName = XP_STRDUP(lpBuf);
                pProfItem->profileLocation = NULL;

                result = RegQueryValueEx(hSubkey, (char *) "DirRoot",
				    			         NULL, NULL, NULL, &size);
                
                if (result == ERROR_SUCCESS) {
                    size++;
                    pProfItem->profileLocation = (char *) XP_ALLOC(size * sizeof(char));
                    
                    result = RegQueryValueEx(hSubkey, (char *) "DirRoot",
				    			             NULL, NULL, (LPBYTE) pProfItem->profileLocation, &size);
                }

                pProfItem->style = PROFINFO_NATIVE;
                pProfItem->platformSpecificData = NULL;
                pProfItem->upgradeStatus = XP_STRDUP(keyName);
                pProfItem->next = NULL;

                if (pNativeStart == NULL) {
                    pNativeStart = pProfItem;
                }

                if (pProfLast != NULL) {
                    pProfLast->next = pProfItem;
                }

                pProfLast = pProfItem;
            }
        }

        if (hSubkey) RegCloseKey(hSubkey);

        idx++;
    }

	if (hKeyRet) RegCloseKey(hKeyRet);

    *pItems = pNativeStart;
    return;
}


    PRBool
profmgr_NativeProfileExists(const char *profileName)
{
    // FIXME!
    return PR_FALSE;
}


    int
profmgr_NativeCreateDirectory(const char *path)
{
    int     ret;
	char * slash = strchr(path,'\\');

	while (slash) {
		slash[0] = NULL;
		ret = _mkdir(path);
		slash[0] = '\\';
		if (slash+1)
            slash = strchr(slash+1,'\\');
	}

    ret = _mkdir(path);
	if (ret == -1) {
/*		AfxMessageBox(szLoadString(IDS_UNABLE_CREATE_DIR),MB_OK);*/
        ASSERT(0);
		return FALSE;
	}

    return ret;
}

    int
profmgr_NativeCreateUserDirectories(const char *path)
{
    int     ret = 0;
    CString csTmp;
	
	csTmp = path;
	csTmp += "\\News";
	_mkdir(csTmp);

	csTmp = path;
	csTmp += "\\Mail";
	_mkdir(csTmp);
	
	csTmp = path;
	csTmp += "\\Cache";
	_mkdir(csTmp);

    return ret;
}

    char *
profmgr_NativeGetTemporaryProfileDirectory(void)
{
    char            *name;
    unsigned int    pathLength;
    char            tempBuffer[_MAX_PATH];
    char            *filePart;

   
    name = _tempnam("C:\\TEMP", "nscp");
    pathLength = GetFullPathName(name, _MAX_PATH, tempBuffer, &filePart);
    *filePart = '\0';

    return XP_STRDUP(tempBuffer);
}


/* note, should we be using _fullpath here? */


    PRBool
profmgr_FilenameIsFullpath(const char *filename)
{
    unsigned int    pathLength;
    char            tempBuffer[1];
    char            *filePart;

    pathLength = GetFullPathName(filename, 1, tempBuffer, &filePart);

    if (pathLength-1 == XP_STRLEN(filename)) {
        return PR_TRUE;
    } else {
        return PR_FALSE;
    }
}

    PRBool
profmgr_NativeNeedsUpgrade(void)
{
    CWinApp     *pApp = AfxGetApp();

    CString csPref= pApp->GetProfileString("Main","Home Page","");
	if (!csPref.IsEmpty()) return PR_TRUE;

	csPref= pApp->GetProfileString("User","User_Addr","");
	if (!csPref.IsEmpty()) return PR_TRUE;

	csPref= pApp->GetProfileString("User","User_Name","");
	if (!csPref.IsEmpty()) return PR_TRUE;

	csPref= pApp->GetProfileString("Bookmark List","File Location","");
	if (!csPref.IsEmpty()) return PR_TRUE;

	return PR_FALSE;
}



/* MarkUpdgraded makes an annotation in the native (local) registry that tells it
   that we've already moved the records to the XP NS Reg and that we shouldn't bother
   to do it again. For Windows32, this is the Windows registry. */

    int
profmgr_NativeMarkUpgraded(const char *key)
{
	long result;
	HKEY hKeyRet;
    const char *versionNum = "4.5";

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						key,
						NULL,
						KEY_SET_VALUE,
						&hKeyRet);
    
	if (result != ERROR_SUCCESS) {
		return PREF_ERROR;
	}

    result = RegSetValueEx(hKeyRet, "Obsoleted", 0, REG_SZ, (const BYTE *) versionNum, 3);

	if (hKeyRet) RegCloseKey(hKeyRet);

	if (result != ERROR_SUCCESS) {
		return PREF_ERROR;
	} else {
        return PREF_OK;
    }

}

    int
profmgr_NativeDoUpgrade(CProfile *pProfile)
{
    char       userDir[FILENAME_MAX + 1];
    int        nameLength = FILENAME_MAX;
    uint32     profileFlags = pProfile->GetFlags();

    pProfile->GetCharPref("profile.directory", userDir, &nameLength, PR_TRUE);

    login_UpdateFilesToNewLocation(userDir, NULL,
        profileFlags & PROFILE_COPY_UPGRADED_FILES);
    login_UpdatePreferencesToJavaScript(userDir);

    return 0;
}


    int
profmgr_NativeDeleteProfile(const char *profileName)
{
	long result;
    DWORD type, size;
	HKEY hKeyRet;
	char * pDirectory;
    CString csSub = "SOFTWARE\\Netscape\\Netscape Navigator\\Users\\";
    CString subKeyName;

    csSub += profileName;
    subKeyName = csSub;
    subKeyName+="\\";

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						subKeyName,
						NULL,
						KEY_WRITE | KEY_READ,
						&hKeyRet);

	if (result != ERROR_SUCCESS) {
		return NULL;
	}

	// see how much space we need
	result = RegQueryValueEx(hKeyRet,
							  (char *) "DirRoot",
							  NULL,
							  &type,
							  NULL,
							  &size);

	// if we didn't find it error!
	if((result != ERROR_SUCCESS) || (size == 0))
		return NULL;

	// allocate space to hold the string
	pDirectory = (char *) XP_ALLOC((size+1) * sizeof(char));

	// actually load the string now that we have the space
	result = RegQueryValueEx(hKeyRet,
							  (char *) "DirRoot",
							  NULL,
							  &type,
							  (LPBYTE) pDirectory,
							  &size);

	if((result != ERROR_SUCCESS) || (size == 0)) {
        XP_FREE(pDirectory);
		return NULL;
    }


    profmgr_NativeRecursivelyDeleteDirectory(pDirectory);

    RegDeleteValue(hKeyRet, "DirRoot");
    RegDeleteValue(hKeyRet, "EmailAddr");
    RegDeleteValue(hKeyRet, "UserName");

	if (hKeyRet) RegCloseKey(hKeyRet);

    RegDeleteKey(HKEY_LOCAL_MACHINE, csSub);

    XP_FREE(pDirectory);

    return 0;

}

    int
profmgr_NativeDeleteProfileDir(const char *dirpath)
{
    profmgr_NativeRecursivelyDeleteDirectory(dirpath);
    return 0;
}

    int
profmgr_NativeRecursivelyDeleteDirectory(const char *dirpath)
{
    HANDLE                      fileHandle;
    CString                     dirName, fileName;
    WIN32_FIND_DATA             fileInfo;
    DWORD                       error = 0;
    int                         count = 0;

    dirName = dirpath;
    dirName += "\\*.*";

    fileHandle = FindFirstFile(dirName, &fileInfo);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        error = GetLastError();
    }

    while (error == 0) {
        fileName = dirpath;
        fileName += "\\";
        fileName += fileInfo.cFileName;

        if (!((XP_STRCMP(fileInfo.cFileName, ".") == 0) || 
            (XP_STRCMP(fileInfo.cFileName, "..") == 0))) {

            if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                count += profmgr_NativeRecursivelyDeleteDirectory(fileName);
            } else {
                DeleteFile(fileName);
                count++;
            }
        }

        if (!FindNextFile(fileHandle, &fileInfo)) {
            error = GetLastError();
        }
    }
    
    FindClose(fileHandle);

    if (_rmdir(dirpath) == -1) {
        error = errno;
    }

    return count;
}
