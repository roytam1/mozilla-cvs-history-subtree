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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Srilatha Moturi <srilatha@netscape.com>
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

#undef UNICODE
#undef _UNICODE

#include "nsIServiceManager.h"
#include "nsMapiRegistryUtils.h"
#include "nsString.h"
#include "nsIStringBundle.h"
#include "nsXPIDLString.h"
#include "nsSpecialSystemDirectory.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"

// returns TRUE if the Mapi32.dll is smart dll.
static PRBool isSmartDll();

// returns TRUE if the Mapi32.dll is a Mozilla dll.
static PRBool isMozDll();

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
const CLSID CLSID_nsMapiImp = {0x29f458be, 0x8866, 0x11d5, {0xa3, 0xdd, 0x0, 0xb0, 0xd0, 0xf3, 0xba, 0xa7}};

// Returns the (fully-qualified) name of this executable.
static nsCString thisApplication() {
    static nsCAutoString result;

    if (result.IsEmpty()) {
        char buffer[MAX_PATH] = {0};
        DWORD len = ::GetModuleFileName(NULL, buffer, sizeof buffer);
        len = ::GetShortPathName(buffer, buffer, sizeof buffer);
    
        result = buffer;
        result.ToUpperCase();
    }

    return result;
}

/** This returns the brand name for this application
  */
static nsCString brandName() {
    static nsCAutoString brand;
    nsresult rv;
    if (brand.IsEmpty()) {
        nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(
                                         kStringBundleServiceCID, &rv));
        if (NS_SUCCEEDED(rv) && bundleService) {
            nsCOMPtr<nsIStringBundle> brandBundle;
            rv = bundleService->CreateBundle(
                        "chrome://global/locale/brand.properties",
                        getter_AddRefs(brandBundle));
            if (NS_SUCCEEDED(rv)) {
                nsXPIDLString brandName;
                rv = brandBundle->GetStringFromName(
                           NS_LITERAL_STRING("brandShortName").get(),
                           getter_Copies(brandName));
                if (NS_SUCCEEDED(rv)) {
                    brand.AssignWithConversion(brandName.get());
                }
            }
        }
    }
    return brand;
}

// verifyRestrictedAccess - Returns PR_TRUE if this user only has restricted access
// to the registry keys we need to modify.
PRBool verifyRestrictedAccess() {
    char   subKey[] = "Software\\Mozilla - Test Key";
    PRBool result = PR_FALSE;
    DWORD  dwDisp = 0;
    HKEY   key;
    // Try to create/open a subkey under HKLM.
    DWORD rc = ::RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                subKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &key,
                                &dwDisp);

    if (rc == ERROR_SUCCESS) {
        // Key was opened; first close it.
        ::RegCloseKey(key);
        // Delete it if we just created it.
        switch(dwDisp) {
            case REG_CREATED_NEW_KEY:
                ::RegDeleteKey(HKEY_LOCAL_MACHINE, subKey);
                break;
            case REG_OPENED_EXISTING_KEY:
                break;
        }
    } else {
        // Can't create/open it; we don't have access.
        result = PR_TRUE;
    }
    return result;
}

nsresult SetRegistryKey(HKEY baseKey, const char * keyName, 
                        const char * valueName, char * value)
{
    nsresult result = NS_ERROR_FAILURE;
    HKEY   key;
    LONG   rc = ::RegCreateKey(baseKey, keyName, &key);
  
    if (rc == ERROR_SUCCESS) {
        char buffer[MAX_PATH] = {0};
        DWORD len = sizeof buffer;
        rc = ::RegQueryValueEx(key, valueName, NULL, NULL, (LPBYTE)buffer, 
                                &len );
        if (strcmp(value, buffer) != 0 ) {
            rc = ::RegSetValueEx(key, valueName, NULL, REG_SZ, 
                                 (LPBYTE)(const char*)value, strlen(value));
            if (rc == ERROR_SUCCESS) {
                result = NS_OK;
            }
        } else {
            result = NS_OK;
        }
        ::RegCloseKey(key);
    }
    return result;
}

nsresult DeleteRegistryValue(HKEY baseKey, const char * keyName, 
                        const char * valueName)
{
    nsresult result = NS_ERROR_FAILURE;
    HKEY   key;
    LONG   rc = ::RegOpenKey(baseKey, keyName, &key);
  
    if (rc == ERROR_SUCCESS) {
        rc = ::RegDeleteValue(key, valueName);
        if (rc == ERROR_SUCCESS)
            result = NS_OK;
        ::RegCloseKey(key);
    }
    return result;
}

nsCString GetRegistryKey(HKEY baseKey, const char * keyName, 
                         const char * valueName)
{
    nsCAutoString value;
    HKEY   key;
    LONG   rc = ::RegOpenKey(baseKey, keyName, &key);
    if (rc == ERROR_SUCCESS) {
        char buffer[MAX_PATH] = {0};
        DWORD len = sizeof buffer;
        rc = ::RegQueryValueEx(key, valueName, NULL, NULL, 
                               (LPBYTE)buffer, &len);
        if (rc == ERROR_SUCCESS) {
            if (len)
                value = buffer;
        }
        ::RegCloseKey(key);
     }
     return value;
}

#define EXE_EXTENSION ".exe" 

/** Returns TRUE if the current application is default mail client.
 */
PRBool IsDefaultMailClient()
{
    if (!isSmartDll() && !isMozDll()) 
        return PR_FALSE;
    nsCAutoString name(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                                      "Software\\Clients\\Mail", ""));
    if (!name.IsEmpty()) {
         nsCString keyName("Software\\Clients\\Mail\\");
         keyName += name.get(); 
         keyName += "\\protocols\\mailto\\shell\\open\\command";

         nsCAutoString result(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                                             keyName.get(), ""));
         if (!result.IsEmpty()) {
             nsCAutoString strExtension;
             strExtension.Assign(EXE_EXTENSION);
             result.ToUpperCase();
             strExtension.ToUpperCase();
             PRInt32 index = result.RFind(strExtension.get());
             if (index != kNotFound) {
                 result.Truncate(index + strExtension.Length());
             }
             return (result == thisApplication());
        }
    }
    return PR_FALSE;

}

/** Saves the current setting of the default Mail Client in 
 * HKEY_LOCAL_MACHINE\\Software\\Mozilla\\Desktop
 */
nsresult saveDefaultMailClient()
{
    nsresult rv;
    nsCAutoString name(GetRegistryKey(HKEY_LOCAL_MACHINE,
                                    "Software\\Clients\\Mail", ""));
    if (!name.IsEmpty()) {
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            "Software\\Mozilla\\Desktop", 
                            "HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail", 
                            (char *)name.get());
        if (NS_SUCCEEDED(rv)) {
            nsCString keyName("Software\\Clients\\Mail\\");
            keyName += name.get(); 
            keyName += "\\protocols\\mailto\\shell\\open\\command";
            nsCAutoString appPath(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                                               keyName.get(), ""));
            if (!appPath.IsEmpty()) {
                nsCString stringName("HKEY_LOCAL_MACHINE\\");
                stringName += keyName.get();
                rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                  "Software\\Mozilla\\Desktop", 
                                  stringName.get(), (char *)appPath.get());
            }
        }
    }
    else
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            "Software\\Mozilla\\Desktop", 
                            "HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail", 
                            "");
    return rv;
} 

/** Saves the current user setting of the default Mail Client in 
 * HKEY_LOCAL_MACHINE\\Software\\Mozilla\\Desktop
 */
nsresult saveUserDefaultMailClient()
{
    nsresult rv;
    nsCAutoString name(GetRegistryKey(HKEY_CURRENT_USER,
                                    "Software\\Clients\\Mail", ""));
    if (!name.IsEmpty()) {
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            "Software\\Mozilla\\Desktop", 
                            "HKEY_CURRENT_USER\\Software\\Clients\\Mail", 
                            (char *)name.get());
    }
    else {
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            "Software\\Mozilla\\Desktop", 
                            "HKEY_CURRENT_USER\\Software\\Clients\\Mail", 
                            "");
   }
   return rv;
}

/**
 * Check whether it is a smart dll or not. Smart dll is the one installed by
 * IE5 or Outlook Express which forwards the MAPI calls to the dll based on the 
 * registry key setttings.
 * Returns TRUE if is a smart dll.
 */

typedef HRESULT (FAR PASCAL GetOutlookVersionFunc)(); 
static PRBool isSmartDll() 
{ 
    char buffer[MAX_PATH] = {0};
    if (GetSystemDirectory(buffer, sizeof(buffer)) == 0) 
        return PR_FALSE;
    PL_strcatn(buffer, sizeof(buffer), "\\Mapi32.dll");
    
    HINSTANCE hInst; 
    GetOutlookVersionFunc *doesExist = nsnull;
    hInst = LoadLibrary(buffer); 
    if (hInst == nsnull) 
        return PR_FALSE;
        
    doesExist = (GetOutlookVersionFunc *) GetProcAddress (hInst, "GetOutlookVersion"); 
    FreeLibrary(hInst); 

    return (doesExist != nsnull); 
} 

typedef HRESULT (FAR PASCAL GetMapiDllVersion)(); 
/**
 * Checks whether mapi32.dll is installed by this app. 
 * Returns TRUE if it is.
 */
static PRBool isMozDll() 
{ 
    char buffer[MAX_PATH] = {0};
    if (GetSystemDirectory(buffer, sizeof(buffer)) == 0) 
        return PR_FALSE;
    PL_strcatn(buffer, sizeof(buffer), "\\Mapi32.dll"); 

    HINSTANCE hInst; 
    GetMapiDllVersion *doesExist = nsnull;
    hInst = LoadLibrary(buffer); 
    if (hInst == nsnull) 
        return PR_FALSE;
        
    doesExist = (GetMapiDllVersion *) GetProcAddress (hInst, "GetMapiDllVersion"); 
    FreeLibrary(hInst); 

    return (doesExist != nsnull); 
} 

/** Renames Mapi32.dl in system directory to Mapi32_moz_bak.dll
 *  copies the mozMapi32.dll from bin directory to the system directory
 */
nsresult CopyMozMapiToWinSysDir()
{
    nsresult rv;
    char buffer[MAX_PATH] = {0};
    if (GetSystemDirectory(buffer, sizeof(buffer)) == 0) 
        return NS_ERROR_FAILURE;

    nsCAutoString filePath(buffer);
    filePath.Append("\\Mapi32_moz_bak.dll");

    nsCOMPtr<nsILocalFile> pCurrentMapiFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !pCurrentMapiFile) return rv;        
    pCurrentMapiFile->InitWithPath(filePath.get());

    nsCOMPtr<nsIFile> pMozMapiFile;
    nsCOMPtr<nsIProperties> directoryService =
          do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    if (!directoryService) return NS_ERROR_FAILURE;
    rv = directoryService->Get(NS_OS_CURRENT_PROCESS_DIR,
                              NS_GET_IID(nsIFile),
                              getter_AddRefs(pMozMapiFile));

    if (NS_FAILED(rv)) return rv;
    pMozMapiFile->Append("mozMapi32.dll");

    PRBool bExist;
    rv = pMozMapiFile->Exists(&bExist);
    if (NS_FAILED(rv) || !bExist) return rv;

    rv = pCurrentMapiFile->Exists(&bExist);
    if (NS_SUCCEEDED(rv) && bExist)
    {
        rv = pCurrentMapiFile->Remove(PR_FALSE);
    }
    if (NS_FAILED(rv)) return rv;
    filePath.Assign(buffer);
    filePath.Append("\\Mapi32.dll");
    pCurrentMapiFile->InitWithPath(filePath.get());
    rv = pCurrentMapiFile->Exists(&bExist);
    if (NS_SUCCEEDED(rv) && bExist)
    {
        rv = pCurrentMapiFile->MoveTo(nsnull, "Mapi32_moz_bak.dll");
        if (NS_FAILED(rv)) return rv;
        nsCAutoString fullFilePath(buffer);
        fullFilePath.Append("\\Mapi32_moz_bak.dll");
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            "Software\\Mozilla\\Desktop", 
                            "Mapi_backup_dll", 
                            (char *)fullFilePath.get());
        if (NS_FAILED(rv)) {
             RestoreBackedUpMapiDll();
             return rv;
        }
    }
    
    nsAutoString fileName;
    fileName.AssignWithConversion("Mapi32.dll");
    filePath.Assign(buffer);
    pCurrentMapiFile->InitWithPath(filePath.get());
    rv = pMozMapiFile->CopyToUnicode(pCurrentMapiFile, fileName.get());
    if (NS_FAILED(rv))
        RestoreBackedUpMapiDll();
    return rv;
}

/** deletes the Mapi32.dll in system directory and renames Mapi32_moz_bak.dll
 *  to Mapi32.dll
 */
nsresult RestoreBackedUpMapiDll()
{
    nsresult rv;
    char buffer[MAX_PATH] = {0};
    if (GetSystemDirectory(buffer, sizeof(buffer)) == 0) 
        return NS_ERROR_FAILURE;

    nsCAutoString filePath(buffer);
    nsCAutoString previousFileName(buffer);
    filePath.Append("\\Mapi32.dll");
    previousFileName.Append("\\Mapi32_moz_bak.dll");

    nsCOMPtr <nsILocalFile> pCurrentMapiFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !pCurrentMapiFile) return NS_ERROR_FAILURE;        
    pCurrentMapiFile->InitWithPath(filePath);
    
    nsCOMPtr<nsILocalFile> pPreviousMapiFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !pPreviousMapiFile) return NS_ERROR_FAILURE;       
    pPreviousMapiFile->InitWithPath(previousFileName);

    PRBool bExist;
    rv = pCurrentMapiFile->Exists(&bExist);
    if (NS_SUCCEEDED(rv) && bExist) {
        rv = pCurrentMapiFile->Remove(PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    rv = pPreviousMapiFile->Exists(&bExist);
    if (NS_SUCCEEDED(rv) && bExist)
        rv = pPreviousMapiFile->MoveTo(nsnull, "Mapi32.dll");
    if (NS_SUCCEEDED(rv))
        DeleteRegistryValue(HKEY_LOCAL_MACHINE,
                            "Software\\Mozilla\\Desktop", 
                            "Mapi_backup_dll");
    return rv;
}

/** Sets Mozilla as default Mail Client
 */
nsresult setDefaultMailClient()
{
    nsresult rv;
    if (verifyRestrictedAccess()) return NS_ERROR_FAILURE;
    if (!isSmartDll()) {
        if (NS_FAILED(CopyMozMapiToWinSysDir())) return NS_ERROR_FAILURE;
    }
    rv = saveDefaultMailClient();
    if (NS_FAILED(saveUserDefaultMailClient()) ||
        NS_FAILED(rv)) return NS_ERROR_FAILURE;
    nsCAutoString keyName("Software\\Clients\\Mail\\");

    nsCAutoString appName(brandName());
    if (!appName.IsEmpty()) {
        keyName.Append(appName.get());
        // hardcoding this for 0.9.4 branch
        // need to change it before merging into the trunk
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                        (char *)keyName.get(), 
                        "", "Netscape 6.2 Mail"); 
    }
    else
        rv = NS_ERROR_FAILURE;
    if (NS_SUCCEEDED(rv)) {
        nsCAutoString dllPath(thisApplication());
        PRInt32 index = dllPath.RFind("\\");
        if (index != kNotFound)
            dllPath.Truncate(index + 1);
        dllPath += "mozMapi32.dll";
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                            (char *)keyName.get(), "DLLPath", 
                            (char *)dllPath.get());
        if (NS_SUCCEEDED(rv)) {
            keyName.Append("\\protocols\\mailto");
            rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                (char *)keyName.get(), 
                                "", "URL:MailTo Protocol");
            if (NS_SUCCEEDED(rv)) {
                nsCAutoString appPath(thisApplication());
                appPath += " \"%1\"";
                keyName.Append("\\shell\\open\\command");
                rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                       (char *)keyName.get(), 
                       "", (char *)appPath.get());
                if (NS_SUCCEEDED(rv)) {
                    rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                        "Software\\Clients\\Mail", 
                                        "", (char *)appName.get());
                }
                if (NS_SUCCEEDED(rv)) {
                    nsCAutoString mailAppPath(thisApplication());
                    mailAppPath += " -mail";
                    nsCAutoString appKeyName ("Software\\Clients\\Mail\\");
                    appKeyName.Append(appName.get());
                    appKeyName.Append("\\shell\\open\\command");
                    rv = SetRegistryKey(HKEY_LOCAL_MACHINE,
                                        (char *)appKeyName.get(),
                                        "", (char *)mailAppPath.get());
                }
                if (NS_SUCCEEDED(rv)) {
                    nsCAutoString iconPath(thisApplication());
                    iconPath += ",0";
                    nsCAutoString iconKeyName ("Software\\Clients\\Mail\\");
                    iconKeyName.Append(appName.get());
                    iconKeyName.Append("\\DefaultIcon");
                    rv = SetRegistryKey(HKEY_LOCAL_MACHINE,
                                        (char *)iconKeyName.get(),
                                        "", (char *)iconPath.get());
                }
            }            
        }
    }
    
    nsresult result = SetRegistryKey(HKEY_CURRENT_USER, "Software\\Clients\\Mail",
                                "", (char *)appName.get());
    if (NS_SUCCEEDED(result)) {
       result = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                           "Software\\Mozilla\\Desktop", 
                           "defaultMailHasBeenSet", "1");
    }

    if (NS_SUCCEEDED(rv)) {
        RegisterServer(CLSID_nsMapiImp, "Mozilla MAPI", "mozMapi", "mozMapi.1");
        if (NS_SUCCEEDED(result))
            return result;
    }
    
    return NS_ERROR_FAILURE;
}

/** Removes Mozilla as the default Mail client and restores the previous setting
 */
nsresult unsetDefaultMailClient() {
    nsresult result = NS_OK;
    if (verifyRestrictedAccess()) return NS_ERROR_FAILURE;
    if (!isSmartDll()) {
        if (NS_FAILED(RestoreBackedUpMapiDll())) return NS_ERROR_FAILURE;
    }
    nsCAutoString name(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                                      "Software\\Mozilla\\Desktop", 
                                      "HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail"));
    nsCAutoString appName(brandName());

    if (!name.IsEmpty() && !appName.IsEmpty() && name.Equals(appName)) {
        nsCAutoString keyName("HKEY_LOCAL_MACHINE\\Software\\Clients\\Mail\\");
        keyName.Append(appName.get());
        keyName.Append("\\protocols\\mailto\\shell\\open\\command");
        nsCAutoString appPath(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                        "Software\\Mozilla\\Desktop", 
                        (char *)keyName.get()));
        if (!appPath.IsEmpty()) {
            keyName.Assign("Software\\Clients\\Mail\\");
            keyName.Append(appName.get());
            keyName.Append("\\protocols\\mailto\\shell\\open\\command");
            result = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                       (char *) keyName.get(), 
                       "", (char *)appPath.get());
            if (NS_SUCCEEDED(result)) {
                PRInt32 index = appPath.RFind("\\");
                if (index != kNotFound)
                    appPath.Truncate(index + 1);
                appPath += "mozMapi32.dll";
                keyName.Assign("Software\\Clients\\Mail\\");
                keyName.Append(appName.get());
                result = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                        (char *)keyName.get(), 
                                        "DLLPath", (char *) appPath.get());
            }
        }
    }
    if (!name.IsEmpty())
        if (NS_SUCCEEDED(result))
            result = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                "Software\\Clients\\Mail", 
                                "", (char *)name.get());
    else
        result = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                "Software\\Clients\\Mail", 
                                "", "");
   
    nsCAutoString userAppName(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                              "Software\\Mozilla\\Desktop", 
                              "HKEY_CURRENT_USER\\Software\\Clients\\Mail"));
    nsresult rv = NS_OK;
    if (!userAppName.IsEmpty()) {
        rv = SetRegistryKey(HKEY_CURRENT_USER, 
                                "Software\\Clients\\Mail", 
                                "", (char *)userAppName.get());
    }
    else {
        DeleteRegistryValue(HKEY_CURRENT_USER, "Software\\Clients\\Mail", "");
    }
    if (NS_SUCCEEDED(rv)) {
        rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                "Software\\Mozilla\\Desktop", 
                                "defaultMailHasBeenSet", "0");
    }
    if (NS_SUCCEEDED(result)) {
        UnregisterServer(CLSID_nsMapiImp, "mozMapi", "mozMapi.1");
        if (NS_SUCCEEDED(rv))
            return rv;
    }
    return NS_ERROR_FAILURE;
}

/** Returns FALSE if showMapiDialog is set to 0.
 *  Returns TRUE otherwise
 *  Also returns TRUE if the Mozilla has been set as the default mail client
 *  and some other application has changed that setting.
 *  This function gets called only if the current app is not the default mail
 *  client
 */
PRBool getShowDialog() {
    PRBool rv = PR_FALSE;
    nsCString showDialog(GetRegistryKey(HKEY_LOCAL_MACHINE, 
                                        "Software\\Mozilla\\Desktop", 
                                        "showMapiDialog"));
    // if the user has not selected the checkbox, show dialog 
    if (showDialog.IsEmpty() || showDialog.Equals("1"))
            rv = PR_TRUE;

    if (!rv) {
        // even if the user has selected the checkbox
        // show it if some other application has changed the 
        // default setting.
        nsCAutoString setMailDefault(GetRegistryKey(HKEY_LOCAL_MACHINE,
                                       "Software\\Mozilla\\Desktop", 
                                       "defaultMailHasBeenSet"));
        if (setMailDefault.Equals("1")) {
            // need to reset the defaultMailHasBeenSet to "0"
            // so that after the dialog is displayed once,
            // we do not keep displaying this dialog after the user has
            // selected the checkbox
            rv = SetRegistryKey(HKEY_LOCAL_MACHINE, 
                                "Software\\Mozilla\\Desktop", 
                                "defaultMailHasBeenSet", "0");
            rv = PR_TRUE;
        }
    }
    return rv;
}
