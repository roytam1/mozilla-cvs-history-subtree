/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
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

