/* -*- C -*-
 *
 * Copyright (c) 1998-1999 Innosoft International, Inc.  All Rights Reserved.
 *
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Innosoft International, 
 * Inc. or a written license from an organization licensed by Innosoft
 * International, Inc. to grant such a license.
 *
 *
 * Copyright (c) 1996-1997 Critical Angle Inc. All Rights Reserved.
 * 
 */

/*
 * $RCSfile$ $Revision$ $Date$ $State$
 *
 * $Log$
 * Revision 1.32  1999/11/30 19:22:13  wahl
 * A9911304A test for IDDS_POSIX64
 *
 * Revision 1.31  1999/06/01 17:04:25  wahl
 * ensure date is visible in debug.log messages
 *
 * Revision 1.30  1999/05/17 17:51:25  wahl
 * fix typo
 *
 * Revision 1.29  1999/05/11 21:31:58  wahl
 * fcntl.h needed on UNIX as well
 *
 * Revision 1.28  1999/05/11 18:22:34  Administrator
 * convert from FILE to file descriptor
 *
 * Revision 1.27  1999/04/06 19:52:39  kvc
 * VMS port
 *
 * Revision 1.26  1999/03/22 23:58:19  Administrator
 * update Copyright statements in ILC-SDK
 *
 * Revision 1.25  1999/02/08 20:12:56  wahl
 * add/remote casts for AIX
 *
 * Revision 1.24  1999/01/04 02:45:17  wahl
 * add debug tracking for core file
 *
 * Revision 1.23  1998/10/16 03:30:47  wahl
 * A9810154B remove slash from pathname
 *
 * Revision 1.22  1998/09/15 00:04:25  wahl
 * A9809144B change VMS
 *
 * Revision 1.21  1998/09/12 00:47:57  wahl
 * call ldap_log_set_path
 *
 * Revision 1.20  1998/08/12 20:51:22  kvc
 * VMS port
 *
 * Revision 1.19  1998/06/24 17:31:34  Administrator
 * use ASCII Win32 functions
 *
 * Revision 1.18  1998/06/21 22:08:32  wahl
 * change Debug to ldap_log_message
 *
 * Revision 1.17  1998/04/14 13:42:50  Administrator
 * A9804144A rename registry key to match Installshield setting
 *
 * Revision 1.16  1998/04/10 20:31:45  Administrator
 * update registry key name
 *
 * Revision 1.15  1998/03/06 21:35:11  wahl
 * A9803064B osf1 changes
 *
 * Revision 1.14  1998/01/08 00:26:13  wahl
 * include ldap-int before windows.h
 *
 * Revision 1.13  1997/12/03 00:16:31  wahl
 * fmt arg to Debug is const
 *
 * Revision 1.12  1997/11/04 02:15:33  wahl
 * test _POSIX_THREAD_SAFE_FUNCTIONS
 *
 * Revision 1.11  1997/10/25 17:20:19  wahl
 * fix RCS header
 *
 * Revision 1.10  1997/10/18 00:21:50  wahl
 * copyright notice updates
 *
 * Revision 1.9  1997/10/07 05:55:23  wahl
 * use _getpid on Win32
 *
 * Revision 1.8  1997/10/07 04:38:21  wahl
 * replace pthread_mutex_t with LDAP_MUTEX_T
 *
 * Revision 1.7  1997/10/04 20:22:37  wahl
 * log to file as well
 *
 * Revision 1.6  1997/09/07 18:32:37  wahl
 * additional include file simplification
 *
 * Revision 1.5  1997/09/07 08:51:12  wahl
 * C++ type checks and LDAP_PARAM_ERROR detection
 *
 * Revision 1.4  1997/09/07 07:00:04  wahl
 * include file simplification
 *
 * Revision 1.3  1997/07/31 06:00:17  wahl
 * prepare for API draft update
 *
 * Revision 1.2  1997/03/31 23:07:05  wahl
 * *** empty log message ***
 *
 * Revision 1.1  1997/03/29 19:11:56  wahl
 * import
 *
 * Revision 1.2  1997/03/29 04:05:26  wahl
 * fixes for Win32
 *
 * Revision 1.1  1997/03/19 01:23:22  wahl
 * reorganize line64.c
 *
 *
 */

#ifdef _WIN32
#include <WINDOWS.H>
#include <WINREG.H>
#endif

#if defined(_WIN32)

int ldap_reg_open(char *k,int for_writing,HKEY *phkey)
{
    char *p;
    HKEY hkin;

    *phkey = NULL;

    if (k[0] == '\\') {
	k++;
    }
    p = strchr(k,'\\');
    if (p == NULL) {
	return -1;
    }
    p++;
    
    if (strncmp(k,
	       "HKEY_LOCAL_MACHINE",
	       strlen("HKEY_LOCAL_MACHINE")) == 0) {
	hkin = HKEY_LOCAL_MACHINE;
    } else if (strncmp(k,
		       "HKEY_USERS",
		       strlen("HKEY_USERS")) == 0) {
	hkin = HKEY_USERS;
    } else if (strncmp(k,
		       "HKEY_CLASSES_ROOT",
		       strlen("HKEY_CLASSES_ROOT")) == 0) {
	hkin = HKEY_CLASSES_ROOT;
    } else if (strncmp(k,
		       "HKEY_CURRENT_USER",
		       strlen("HKEY_CURRENT_USER")) == 0) {
	hkin = HKEY_CURRENT_USER;
    } else {
	return -1;
    }
    
    if (RegOpenKeyExA(hkin,p,0,for_writing == 1 ? KEY_WRITE : KEY_READ,
		     phkey) == ERROR_SUCCESS) {
	return 0;
    }
    return -1;
}

void ldap_reg_close(HKEY hk)
{

    RegCloseKey(hk);
}

int ldap_reg_get_string(HKEY hk,char *valname,char *destbuf,int len)
{
    DWORD l = len;  /* modified */
    DWORD typ = REG_SZ;

    if (RegQueryValueExA (hk,
			 valname,
			 NULL,
			 &typ,
			 destbuf,
			 &l) == ERROR_SUCCESS) {
	return 0;
    }
    
   
    return -1;
}
#endif /* _WIN32 */
