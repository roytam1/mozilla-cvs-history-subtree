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
 * The Original Code is Mozilla
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Krishna Mohan Khandrika (kkhandrika@netscape.com)
 * Contributor(s): Rajiv Dayal (rdayal@netscape.com)
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

#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <mapidefs.h>
#include <mapi.h>
#include "msgMapi.h"
#include "msgMapiMain.h"

nsIMapi *pMainReference = NULL;
BOOL bUnInitialize = FALSE;

#define MAX_RECIPS  100
#define MAX_FILES   100

const CLSID CLSID_nsMapiImp = {0x29f458be, 0x8866, 0x11d5,
                              {0xa3, 0xdd, 0x0, 0xb0, 0xd0, 0xf3, 0xba, 0xa7}};
const IID IID_nsIMapi = {0x6EDCD38E,0x8861,0x11d5,
                        {0xA3,0xDD,0x00,0xB0,0xD0,0xF3,0xBA,0xA7}};

HANDLE hMutex;

BOOL StartMozilla()
{
    // tries to get an nsIMapi interface.  if mozilla is not running
    // tries to start mozilla

    HRESULT hRes = 0;

    hRes = ::CoInitialize(NULL);
    bUnInitialize = (hRes == S_OK);

    hRes = ::CoCreateInstance(CLSID_nsMapiImp, NULL, CLSCTX_LOCAL_SERVER,
                              IID_nsIMapi, (LPVOID *)&pMainReference);
    if (hRes != 0)
    {
        SetLastError(hRes);
        return FALSE;
    }

    // initialize the server side stuff

    hRes = pMainReference->Initialize();
    if (hRes != 0)
    {
        pMainReference->Release();
        pMainReference = NULL;
        SetLastError(hRes);
        return FALSE;
    }

    return TRUE;
}

void ShutDownMozilla()
{
    // releases the interface.

    WaitForSingleObject(hMutex, INFINITE);
    if (pMainReference != NULL)
    {
        pMainReference->CleanUp();
        pMainReference->Release();
        pMainReference = NULL;
    }
    ReleaseMutex(hMutex);
    if (bUnInitialize)
        ::CoUninitialize();
}

BOOL CheckMozilla()
{
    // cheks whether the interface is valid or not.
    // if not tries get a reference to mapi support
    // in mozilla.

    BOOL bRetValue = TRUE;

    WaitForSingleObject(hMutex, INFINITE);
    if (pMainReference == NULL)
        bRetValue = StartMozilla();

    ReleaseMutex(hMutex);

    return bRetValue;
}

BOOL WINAPI DllMain(HINSTANCE aInstance, DWORD aReason, LPVOID aReserved)
{
    switch (aReason)
    {
        case DLL_PROCESS_ATTACH :
        {
            hMutex = CreateMutex(NULL, FALSE, NULL);
            break;
        }
        case DLL_PROCESS_DETACH :
        {
            ShutDownMozilla();
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            break;
        }
    }

    return TRUE;
}

BOOL GetMozillaReference(nsIMapi **retValue)
{
    HRESULT hRes = 0;
    BOOL bInit = FALSE;

    if (!CheckMozilla())
        return FALSE;

    hRes = ::CoInitialize(NULL);
    if (hRes == S_OK)
    {
        // Its a different apartment.  So need get the 
        // reference to nsIMapi again

        hRes = ::CoCreateInstance(CLSID_nsMapiImp, NULL, CLSCTX_LOCAL_SERVER, \
                              IID_nsIMapi, (LPVOID *)retValue);
        if (*retValue == NULL)
            ::CoUninitialize();
        else
            bInit = TRUE;
    }
    else
    {
	if (pMainReference)
	{
        	*retValue = pMainReference;
        	(*retValue)->AddRef();
	}
    }   

    return bInit;
}

////////////////////////////////////////////////////////////////////////////////////////
// The MAPILogon function begins a Simple MAPI session, loading the default message ////
// store and address book providers                            ////
////////////////////////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPILogon(ULONG aUIParam, LPTSTR aProfileName,
                            LPTSTR aPassword, FLAGS aFlags,
                            ULONG aReserved, LPLHANDLE aSession)
{
    HRESULT hr = 0;
    ULONG nSessionId = 0;

    nsIMapi *pNsMapi = NULL;
    BOOL bComInitialized = FALSE;
    ULONG retValue = SUCCESS_SUCCESS;

    if (!(aFlags & MAPI_UNICODE))
    {
        // Need to convert the parameters to Unicode.

        char *pUserName = (char *) aProfileName;
        char *pPassWord = (char *) aPassword;

        TCHAR ProfileName[MAX_NAME_LEN] = {0};
        TCHAR PassWord[MAX_PW_LEN] = {0};

        if (pUserName != NULL)
        {
            if (!MultiByteToWideChar(CP_ACP, 0, pUserName, -1, ProfileName,
                                                            MAX_NAME_LEN))
                return MAPI_E_FAILURE;
        }

        if (pPassWord != NULL)
        {
            if (!MultiByteToWideChar(CP_ACP, 0, pPassWord, -1, PassWord,
                                                            MAX_NAME_LEN))
                return MAPI_E_FAILURE;
        }

        bComInitialized = GetMozillaReference(&pNsMapi);
        if (pNsMapi == NULL)
            return MAPI_E_FAILURE;

        hr = pNsMapi->Login(aUIParam, ProfileName, PassWord, aFlags,
                                                        &nSessionId);
    }
    else
    {
        bComInitialized = GetMozillaReference(&pNsMapi);
        if (pNsMapi == NULL)
            return MAPI_E_FAILURE;

        hr = pNsMapi->Login(aUIParam, aProfileName, aPassword,
                                                aFlags, &nSessionId);
    }

    if (hr == S_OK)
        (*aSession) = (LHANDLE) nSessionId;
    else
        retValue = nSessionId;

    pNsMapi->Release();

    if (bComInitialized)
        ::CoUninitialize();

    return retValue;
}

ULONG FAR PASCAL MAPILogoff (LHANDLE aSession, ULONG aUIParam,
                                            FLAGS aFlags, ULONG aReserved)
{
    HRESULT hr = 0;
    nsIMapi *pNsMapi = NULL;
    BOOL bComInitialized = FALSE;
    ULONG retValue = SUCCESS_SUCCESS;

    bComInitialized = GetMozillaReference(&pNsMapi);
    if (pNsMapi == NULL)
        return MAPI_E_FAILURE;

    hr = pNsMapi->Logoff((ULONG) aSession);
    if (hr != 0)
        retValue = MAPI_E_INVALID_SESSION;

    pNsMapi->Release();

    if (bComInitialized)
        ::CoUninitialize();

    return retValue;
}

ULONG FAR PASCAL MAPISendMail (LHANDLE lhSession, ULONG ulUIParam, lpnsMapiMessage lpMessage,
                FLAGS flFlags, ULONG ulReserved )
{
    HRESULT hr = 0;
    BOOL bTempSession = FALSE ;

    BOOL bCoUnInit = FALSE;

    if (lpMessage->nRecipCount > MAX_RECIPS)
        return MAPI_E_TOO_MANY_RECIPIENTS ;

    if (lpMessage->nFileCount > MAX_FILES)
        return MAPI_E_TOO_MANY_FILES ;

    if ( (!(flFlags & MAPI_DIALOG)) && (lpMessage->lpRecips == NULL) )
        return MAPI_E_UNKNOWN_RECIPIENT ;

    if (!lhSession)
    {
        FLAGS LoginFlag ;
        if ( (flFlags & MAPI_LOGON_UI) && (flFlags & MAPI_NEW_SESSION) )
            LoginFlag = MAPI_LOGON_UI | MAPI_NEW_SESSION ;
        else if (flFlags & MAPI_LOGON_UI) 
            LoginFlag = MAPI_LOGON_UI ;

        hr = MAPILogon (ulUIParam, (LPTSTR) NULL, (LPTSTR) NULL, LoginFlag, 0, &lhSession) ;
        if (hr != SUCCESS_SUCCESS)
            return MAPI_E_LOGIN_FAILURE ;
        bTempSession = TRUE ;
    }

    // we need to deal with null data passed in by MAPI clients, specially when MAPI_DIALOG is set.
    // The MS COM type lib code generated by MIDL for the MS COM interfaces checks for these parameters
    // to be non null, although null is a valid value for them here. 
    nsMapiRecipDesc * lpRecips ;
    nsMapiFileDesc * lpFiles ;

    nsMapiMessage Message ;
	memset (&Message, 0, sizeof (nsMapiMessage) ) ;
    nsMapiRecipDesc Recipient ;
	memset (&Recipient, 0, sizeof (nsMapiRecipDesc) );
    nsMapiFileDesc Files ;
	memset (&Files, 0, sizeof (nsMapiFileDesc) ) ;

    if(!lpMessage)
    {
       lpMessage = &Message ;
    }
    if(!lpMessage->lpRecips)
    {
        lpRecips = &Recipient ;
    }
    else
        lpRecips = lpMessage->lpRecips ;
    if(!lpMessage->lpFiles)
    {
        lpFiles = &Files ;
    }
    else
        lpFiles = lpMessage->lpFiles ;

    nsIMapi *pNsMapi = NULL;
    BOOL bComInitialized = FALSE;

    bComInitialized = GetMozillaReference(&pNsMapi);
    if (pNsMapi == NULL)
        return MAPI_E_FAILURE;

    hr = pNsMapi->SendMail (lhSession, lpMessage, 
                            (short) lpMessage->nRecipCount, lpRecips,
                            (short) lpMessage->nFileCount, lpFiles,
                            flFlags, ulReserved);

    if (bTempSession)
        MAPILogoff (lhSession, ulUIParam, 0,0) ;

    pNsMapi->Release();
    if (bComInitialized)
        ::CoUninitialize();

    // we are seeing a problem when using Word, although we return success from the MAPI support
    // MS COM interface in mozilla, we are getting this error here. This is a temporary hack !!
    if (hr == 0x800703e6)
        return SUCCESS_SUCCESS;
    
    return hr ; 
}


ULONG FAR PASCAL MAPISendDocuments(ULONG ulUIParam, LPTSTR lpszDelimChar, LPTSTR lpszFilePaths,
                                LPTSTR lpszFileNames, ULONG ulReserved)
{
    LHANDLE lhSession ;
    unsigned long result = MAPILogon (ulUIParam, (LPTSTR) NULL, (LPTSTR) NULL, MAPI_LOGON_UI, 0, &lhSession) ;
    if (result != SUCCESS_SUCCESS)
        return MAPI_E_LOGIN_FAILURE ;

    HRESULT hr;
    nsIMapi *pNsMapi = NULL;
    BOOL bComInitialized = FALSE;

    hr = pNsMapi->SendDocuments(lhSession, (LPTSTR) lpszDelimChar, (LPTSTR) lpszFilePaths, 
                                    (LPTSTR) lpszFileNames, ulReserved) ;

    MAPILogoff (lhSession, ulUIParam, 0,0) ;

    pNsMapi->Release();
    if (bComInitialized)
        ::CoUninitialize();

    return hr ;
}

ULONG FAR PASCAL MAPIFindNext(LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszMessageType,
                              LPTSTR lpszSeedMessageID, FLAGS flFlags, ULONG ulReserved,
                              LPTSTR lpszMessageID)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIReadMail(LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszMessageID,
                              FLAGS flFlags, ULONG ulReserved, lpMapiMessage FAR *lppMessage)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPISaveMail(LHANDLE lhSession, ULONG ulUIParam, lpMapiMessage lpMessage,
                              FLAGS flFlags, ULONG ulReserved, LPTSTR lpszMessageID)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIDeleteMail(LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszMessageID,
                                FLAGS flFlags, ULONG ulReserved)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIAddress(LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszCaption,
                             ULONG nEditFields, LPTSTR lpszLabels, ULONG nRecips,
                             lpMapiRecipDesc lpRecips, FLAGS flFlags,
                             ULONG ulReserved, LPULONG lpnNewRecips,
                             lpMapiRecipDesc FAR *lppNewRecips)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIDetails(LHANDLE lhSession, ULONG ulUIParam, lpMapiRecipDesc lpRecip,
                             FLAGS flFlags, ULONG ulReserved)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIResolveName(LHANDLE lhSession, ULONG ulUIParam, LPTSTR lpszName,
                                 FLAGS flFlags, ULONG ulReserved, lpMapiRecipDesc FAR *lppRecip)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL MAPIFreeBuffer(LPVOID pv)
{
    if (CheckMozilla() == FALSE)
        return MAPI_E_FAILURE;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL GetMapiDllVersion()
{
    return 94;
}




