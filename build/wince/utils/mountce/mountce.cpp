/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code, released
 * Jan 28, 2003.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2003 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 28-January-2003
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */

#include <windows.h>
#include <winnetwk.h>


//
//  This utility exists so that a user can map a network drive from the CE device.
//  Generally, I have used this to debug a large set of binaries from a PC share
//      instead of taking up the CE device memory.
//


static LPTSTR getOption(LPCTSTR inCmdLine, LPCTSTR inOption)
//
//  Caller must free return value.
//
{
    LPTSTR retval = NULL;

    if(NULL != inCmdLine && NULL != inOption)
    {
        LPTSTR found = _tcsstr(inCmdLine, inOption);
        if(NULL != found)
        {
            int optionLength = _tcslen(inOption);
            if(0 != optionLength)
            {
                found += optionLength;

                //
                //  Return value is until space.
                //
                int length;
                for(length = 0; _T('\0') != found[length] && 0 == _istspace(found[length]); length++)
                {
                    //  No body.
                }

                if(0 != length)
                {
                    retval = (LPTSTR)malloc(sizeof(TCHAR) * (length + 1));
                    if(NULL != retval)
                    {
                        _tcsncpy(retval, found, length);
                        retval[length] = _T('\0');
                    }
                }
            }
        }
    }

    return retval;
}


static BOOL setDeviceName(LPCTSTR inName)
//
//  Change the CE device's network ID by directly manipulating the registry.
//
{
    BOOL retval = FALSE;


    HKEY identKey = NULL;
    LONG openRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Ident"), 0, 0, &identKey);
    if(ERROR_SUCCESS == openRes)
    {
        LONG setRes = RegSetValueEx(identKey, _T("Name"), 0, REG_SZ, (LPBYTE)inName, (_tcslen(inName) + 1) * sizeof(TCHAR));
        if(ERROR_SUCCESS == setRes)
        {
            retval = TRUE;
        }

        LONG closeRes = RegCloseKey(identKey);
    }

    return retval;
}


static BOOL registerSFRoot(void)
//
//  Make it so that \\network shows up in enumerations.
//
{
    BOOL retval = FALSE;


    HKEY redirKey = NULL;
    DWORD disp = 0;
    LONG createRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("Comm\\Redir"), 0, NULL, 0, 0, NULL, &redirKey, &disp);
    if(ERROR_SUCCESS == createRes)
    {
        DWORD value = (DWORD)-1;
        LONG setRes = RegSetValueEx(redirKey, _T("RegisterFSRoot"), 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
        if(ERROR_SUCCESS == setRes)
        {
            retval = TRUE;
        }

        LONG closeRes = RegCloseKey(redirKey);
    }

    return retval;
}


int WINAPI WinMain(HINSTANCE inInstance, HINSTANCE inPrevInstance, LPTSTR inCmdLine, int inShowCmd)
{
    int retval = 0;

    //
    //  Make sure \\network shows up.
    //
    if(FALSE == registerSFRoot())
    {
        retval = __LINE__;
    }

    //
    //  Set the device ID.
    //  The default won't allow access to the network.
    //
    LPTSTR name = getOption(inCmdLine, _T("/name:"));
    if(NULL != name)
    {
        if(FALSE == setDeviceName(name))
        {
            retval = __LINE__;
        }
        free(name);
        name = NULL;
    }

    //
    //  Mount a particular network path.
    //
    LPTSTR user = getOption(inCmdLine, _T("/user:"));
    LPTSTR password = getOption(inCmdLine, _T("/password:"));
    LPTSTR rmount = getOption(inCmdLine, _T("/rmount:"));
    LPTSTR lmount = getOption(inCmdLine, _T("/lmount:"));
    if(NULL != rmount && NULL != lmount)
    {
        NETRESOURCE netr;
        memset(&netr, 0, sizeof(netr));

        netr.dwType = RESOURCETYPE_ANY;
        netr.lpLocalName = lmount;
        netr.lpRemoteName = rmount;
        DWORD addRes = WNetAddConnection3(NULL, &netr, password, user, CONNECT_UPDATE_PROFILE);
        if(ERROR_SUCCESS != addRes)
        {
            retval = __LINE__;
        }
        else
        {
            TCHAR shortcut[MAX_PATH];
            TCHAR destination[MAX_PATH];

            //
            //  Give it some shortcuts for fast/visible access.
            //
            _sntprintf(destination, sizeof(destination) / sizeof(TCHAR), _T("\\network\\%s"), lmount);
            _sntprintf(shortcut, sizeof(shortcut) / sizeof(TCHAR), _T("\\%s.lnk"), lmount);
            if(FALSE == SHCreateShortcut(shortcut, destination))
            {
                retval = __LINE__;
            }
            _sntprintf(shortcut, sizeof(shortcut) / sizeof(TCHAR), _T("\\My Documents\\%s.lnk"), lmount);
            if(FALSE == SHCreateShortcut(shortcut, destination))
            {
                retval = __LINE__;
            }
        }

        free(rmount);
        rmount = NULL;
        free(lmount);
        lmount = NULL;
    }
    if(NULL != user)
    {
        free(user);
        user = NULL;
    }
    if(NULL != password)
    {
        free(password);
        password = NULL;
    }

    return retval;
}

