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
 *  ntstubs.c - Stubs needed on NT when linking in
 *  the SSL code. If these stubs were not here, the 
 *  named functions below would not be located at link
 *  time, because there is no implementation of the 
 *  functions for Win32 in cross-platform libraries.
 *
 ******************************************************/

#if defined( _WIN32 ) && defined ( NET_SSL )
/*
XXXceb  This is no longer needed with HCL

#include <windows.h>
#include <nspr.h>

char* XP_FileName (const char* name, XP_FileType type)
{
    return NULL;
}

XP_File XP_FileOpen(const char* name, XP_FileType type, 
		    const XP_FilePerm permissions)
{
    return NULL;
}


char *
WH_FileName (const char *name, XP_FileType type)
{
	return NULL;
}
*/

#endif /* WIN32 && NET_SSL */

