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
 *  ntdebug.c - Sends debug output to window and stdout
 *			    on Win32 platforms.
 *
 ******************************************************/

#if defined( _WIN32 )
#include <windows.h>
#include <time.h>
#include <stdio.h>		
#if defined( SLAPD_LOGGING )
#include "slap.h"
#include "proto-slap.h"
#else
#include "ldap.h"
#include "ldaplog.h"
#endif
int ldap_debug = LDAP_DEBUG_ANY;
FILE *error_logfp = NULL;

void LDAPDebug( int level, char *fmt, ... )
{
	va_list arg_ptr;
	va_start( arg_ptr, fmt ); 
	if ( ldap_debug & level ) 
	{ 
		char szFormattedString[512]; 
		_vsnprintf( szFormattedString, sizeof( szFormattedString ), fmt, arg_ptr ); 

#if defined( LDAP_DEBUG )
		/* Send to debug window ...*/
		OutputDebugString( szFormattedString );

		/* ... and to stderr */
		fprintf( stderr, szFormattedString );
#endif
#if defined( SLAPD_LOGGING )
	    if ( error_logfp != NULL ) 
			slapd_log_error( error_logfp, szFormattedString ); 
#endif
	}  
	va_end( arg_ptr );

}
#endif
