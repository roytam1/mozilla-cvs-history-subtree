/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/******************************************************
 *
 *  ntreg.c - Reads configuration parameters for the 
 *			  Directory Server from the NT Registry.
 *
 ******************************************************/

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include "ldap.h"

int SlapdGetRegSZ( LPTSTR lpszRegKey, LPSTR lpszValueName, LPTSTR lpszValue )
{
	HKEY hKey;
	DWORD dwType, dwNumBytes;
	LONG lResult;

	/* Open the registry, get the required key handle. */	
	lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, lpszRegKey, 
				0L,	KEY_QUERY_VALUE, &hKey );
	if (lResult == ERROR_SUCCESS) 
	{ 
		dwNumBytes = sizeof( DWORD );
		lResult = RegQueryValueEx( hKey, lpszValueName, 0, 
					&dwType, NULL, &dwNumBytes );
		if( lResult == ERROR_SUCCESS ) 
		{
			RegQueryValueEx( hKey, lpszValueName, 0, &dwType, 
							(LPBYTE)lpszValue, &dwNumBytes );
			*(lpszValue+dwNumBytes) = 0;

			/*  Close the Registry. */
			RegCloseKey(hKey);
			return 0;
		}
		else
		{
			/* No config file location stored in the Registry. */
			RegCloseKey(hKey);
			return 1;
		}
	}
	else
	{
  		return 1;
	}
}	/* SlapdGetRegSZ */


int SlapdSetRegSZ( LPTSTR lpszKey, LPSTR lpszValueName, LPTSTR lpszValue )
{
	HKEY hKey;
	LONG lResult;

	/* Open the registry, get a handle to the desired key. */	
	lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, lpszKey, 0, 
				KEY_ALL_ACCESS, &hKey );
	if (lResult == ERROR_SUCCESS) 
	{ 
		/* Set the value to the value-name at the key location. */
		RegSetValueEx( hKey, lpszValueName, 0, REG_SZ, 
					   (CONST BYTE*)lpszValue, strlen(lpszValue) );

		/* Close the registry */
		RegCloseKey(hKey);
		return 0;
	} 
	else 
	{
		return 1;
	}
}	/* SlapdSetRegSZ */

/* converts '/' chars to '\' */
void
unixtodospath(char *szText)
{
    if(szText)
    {
        while(*szText)
        {
            if( *szText == '/' )
                *szText = '\\';
            szText++;
        }
    }
}

/* converts '\' chars to '/' */
void
dostounixpath(char *szText)
{
    if(szText)
    {
        while(*szText)
        {
            if( *szText == '\\' )
                *szText = '/';
            szText++;
        }
    }
}

#endif /* _WIN32 */
