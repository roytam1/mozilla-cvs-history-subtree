*** lber.h	Tue Feb  8 11:06:53 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/include/lber.h	Mon May 22 09:30:16 2000
***************
*** 1,23 ****
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public
!  * License Version 1.1 (the "License"); you may not use this file
!  * except in compliance with the License. You may obtain a copy of
!  * the License at http://www.mozilla.org/NPL/
   *
!  * Software distributed under the License is distributed on an "AS
!  * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
!  * implied. See the License for the specific language governing
!  * rights and limitations under the License.
   *
!  * The Original Code is mozilla.org code.
!  *
!  * The Initial Developer of the Original Code is Netscape
   * Communications Corporation.  Portions created by Netscape are
!  * Copyright (C) 1998 Netscape Communications Corporation. All
!  * Rights Reserved.
!  *
!  * Contributor(s): 
   */
  /* lber.h - header file for ber_* functions */
  #ifndef _LBER_H
--- 1,19 ----
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public License
!  * Version 1.0 (the "NPL"); you may not use this file except in
!  * compliance with the NPL.  You may obtain a copy of the NPL at
!  * http://www.mozilla.org/NPL/
   *
!  * Software distributed under the NPL is distributed on an "AS IS" basis,
!  * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
!  * for the specific language governing rights and limitations under the
!  * NPL.
   *
!  * The Initial Developer of this code under the NPL is Netscape
   * Communications Corporation.  Portions created by Netscape are
!  * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
!  * Reserved.
   */
  /* lber.h - header file for ber_* functions */
  #ifndef _LBER_H
***************
*** 91,96 ****
--- 87,93 ----
  #define LBER_SOCKBUF_OPT_COPYDESC		0x020
  #define LBER_SOCKBUF_OPT_READ_FN		0x040
  #define LBER_SOCKBUF_OPT_WRITE_FN		0x080
+ #define LBER_SOCKBUF_OPT_EXT_IO_FNS		0x100
  
  #define LBER_OPT_ON	((void *) 1)
  #define LBER_OPT_OFF	((void *) 0)
***************
*** 148,161 ****
  #endif /* _WINDOWS */
  #endif /* LDAP_API */
  
  /*
   * libldap read and write I/O function callbacks.  The rest of the I/O callback
!  * types are in ldap.h
   */
! typedef int (LDAP_C LDAP_CALLBACK LDAP_IOF_READ_CALLBACK)( LBER_SOCKET,
! 	void *, int );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_IOF_WRITE_CALLBACK)( LBER_SOCKET,
! 	const void *, int );
  
  /*
   * liblber memory allocation callback functions.  These are global to all
--- 145,179 ----
  #endif /* _WINDOWS */
  #endif /* LDAP_API */
  
+ struct lextiof_socket_private;          /* Defined by the extended I/O */
+                                         /* callback functions */
+ struct lextiof_session_private;         /* Defined by the extended I/O */
+                                         /* callback functions */
+ 
  /*
   * libldap read and write I/O function callbacks.  The rest of the I/O callback
!  * types are defined in ldap.h
!  */
! typedef int (LDAP_C LDAP_CALLBACK LDAP_IOF_READ_CALLBACK)( LBER_SOCKET s,
! 	void *buf, int bufsize );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_IOF_WRITE_CALLBACK)( LBER_SOCKET s,
! 	const void *buf, int len );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_READ_CALLBACK)( int s,
! 	void *buf, int bufsize, struct lextiof_socket_private *arg );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_WRITE_CALLBACK)( int s,
! 	const void *buf, int len, struct lextiof_socket_private *arg );
! 
! /*
!  * Structure for use with LBER_SOCKBUF_OPT_EXT_IO_FNS:
   */
! struct lber_x_ext_io_fns {
! 	    /* lbextiofn_size should always be set to LBER_X_EXTIO_FNS_SIZE */
! 	int				lbextiofn_size;
! 	LDAP_X_EXTIOF_READ_CALLBACK	*lbextiofn_read;
! 	LDAP_X_EXTIOF_WRITE_CALLBACK	*lbextiofn_write;
! 	struct lextiof_socket_private	*lbextiofn_socket_arg;
! };
! #define LBER_X_EXTIO_FNS_SIZE sizeof(struct lber_x_ext_io_fns)
  
  /*
   * liblber memory allocation callback functions.  These are global to all
