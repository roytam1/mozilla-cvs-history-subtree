/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
 *
 *  ntgetpassword.c - Prompts for the key
 *  database passphrase.
 *
 ******************************************************/

#if defined( _WIN32 ) && defined ( NET_SSL )

#include <windows.h>
#include <plstr.h>
#if 0
/*
 * XXXmcs 4-Sep-97: including slap.h causes the SDK build to fail b/c slap.h
 * includes dirver.h (which we don't create during SDK builds).  Nothing in
 * this file seems to need slap.h so we no longer include it (or ldap_ssl.h,
 * which depends on the inclusion of ldap.h which is included in slap.h).
 * Clear as mud?
 */
#include "slap.h"
#include <ldap_ssl.h>
#endif

#include "proto-ntutil.h"
#include "ntresource.h"
#include "ntslapdmessages.h"
#include "ntwatchdog.h"

#undef Debug
#undef OFF
#undef LITTLE_ENDIAN

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static char password[512];

extern LPTSTR	pszServerName;

void CenterDialog(HWND hwndParent, HWND hwndDialog)
{
    RECT DialogRect;
    RECT ParentRect;
    POINT Point;
    int nWidth;
    int nHeight;
    
    // Determine if the main window exists. This can be useful when
    // the application creates the dialog box before it creates the
    // main window. If it does exist, retrieve its size to center
    // the dialog box with respect to the main window.
    if( hwndParent != NULL ) 
	{
		GetClientRect(hwndParent, &ParentRect);
    } 
	else 
	{
		// if main window does not exist, center with respect to desktop
		hwndParent = GetDesktopWindow();
		GetWindowRect(hwndParent, &ParentRect);
    }
    
    // get the size of the dialog box
    GetWindowRect(hwndDialog, &DialogRect);
    
    // calculate height and width for MoveWindow()
    nWidth = DialogRect.right - DialogRect.left;
    nHeight = DialogRect.bottom - DialogRect.top;
    
    // find center point and convert to screen coordinates
    Point.x = (ParentRect.right - ParentRect.left) / 2;
    Point.y = (ParentRect.bottom - ParentRect.top) / 2;
    
    ClientToScreen(hwndParent, &Point);
    
    // calculate new X, Y starting point
    Point.x -= nWidth / 2;
    Point.y -= nHeight / 2;
    
    MoveWindow(hwndDialog, Point.x, Point.y, nWidth, nHeight, FALSE);
}

BOOL CALLBACK PasswordDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) 
	{
      case WM_INITDIALOG:
				CenterDialog(NULL, hDlg);
				SendDlgItemMessage(hDlg, IDEDIT, EM_SETLIMITTEXT, sizeof(password), 0);
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
				return(FALSE);
	
      case WM_COMMAND:
				if(LOWORD(wParam) == IDEDIT) 
				{
					if(HIWORD(wParam) == EN_CHANGE) 
					{
						if(GetDlgItemText(hDlg, IDEDIT, password,
								  sizeof(password)) > 0) 
						{
							EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
						} 
						else 
						{
							EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
						}
					}
					return (FALSE);
				} 
				else if(LOWORD(wParam) == IDOK) 
				{
					GetDlgItemText(hDlg, IDEDIT, password, sizeof(password));
					EndDialog(hDlg, IDOK);
					return (TRUE);
				} 
				else if(LOWORD(wParam) == IDCANCEL) 
				{
					memset(password, 0, sizeof(password));
					EndDialog(hDlg, IDCANCEL);
					return(FALSE);
				}
    }
    return (FALSE);
}
static char*
slapd_PromptForPassword (int resource)
{
    int iResult = 0;
    
    iResult = DialogBox( GetModuleHandle( NULL ), MAKEINTRESOURCE(resource),
		       HWND_DESKTOP, (DLGPROC) PasswordDialogProc);
    if( iResult == -1 ) 
	{
		iResult = GetLastError();
		ReportSlapdEvent( EVENTLOG_INFORMATION_TYPE, 
			MSG_SERVER_PASSWORD_DIALOG_FAILED, 0, NULL );
		return NULL;
    }
	return PL_strdup(password);
}

/* This is the equivalent of SEC_GetPassword(), implemented in SSL
   on the UNIX platforms. First try to retrieve from the watchdog,
   if this is not available, prompt for the passphrase. */
char *Slapd_GetPassword()
{
	HWND hwndRemote;
	int cbRemotePassword = 0;
	char *szRemotePassword = NULL;
	HANDLE hRemoteProcess;
	DWORD dwNumberOfBytesRead;

	// Find Watchdog application window
	if( pszServerName )
	{
		if(hwndRemote = FindWindow("slapd", pszServerName))
		{
			if( hRemoteProcess = (HANDLE)GetWindowLong( hwndRemote, GWL_PROCESS_HANDLE) )
			{
				cbRemotePassword = GetWindowLong(hwndRemote, GWL_PASSWORD_LENGTH);
				if(cbRemotePassword > 512)
					cbRemotePassword = 512;
				if(szRemotePassword = (HANDLE)GetWindowLong(hwndRemote, GWL_PASSWORD_ADDR))
				{
					if(ReadProcessMemory(hRemoteProcess, szRemotePassword, 
								password, cbRemotePassword, &dwNumberOfBytesRead))
					   return PL_strdup(password);
				}
			}
		}
	}

	/* Didn't get the password from Watchdog, so: */
	return slapd_PromptForPassword(IDD_DATABASE_PASSWORD);
}

#ifdef FORTEZZA
char *Slapd_GetFortezzaPIN()
{
    /* Return the PIN from the watchdog, if possible.  Otherwise: */
    return slapd_PromptForPassword(IDD_FORTEZZA_PIN);
}
#endif /* FORTEZZA */

#endif /* defined( _WIN32 ) && defined ( NET_SSL ) */
