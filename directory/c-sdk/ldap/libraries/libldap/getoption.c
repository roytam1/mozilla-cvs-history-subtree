*** getoption.c	Tue Feb  8 11:06:59 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/getoption.c	Tue May 16 14:46:02 2000
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
  #include "ldap-int.h"
  
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
  #include "ldap-int.h"
  
***************
*** 167,174 ****
  #ifdef LDAP_SSLIO_HOOKS
  	/* i/o function pointers */
  	case LDAP_OPT_IO_FN_PTRS:
! 		/* struct copy */
! 		*((struct ldap_io_fns *) optdata) = ld->ld_io;
  		break;
  #endif /* LDAP_SSLIO_HOOKS */
  
--- 163,187 ----
  #ifdef LDAP_SSLIO_HOOKS
  	/* i/o function pointers */
  	case LDAP_OPT_IO_FN_PTRS:
! 		if ( ld->ld_io_fns_ptr == NULL ) {
! 			memset( optdata, 0, sizeof( struct ldap_io_fns ));
! 		} else {
! 			/* struct copy */
! 			*((struct ldap_io_fns *)optdata) = *(ld->ld_io_fns_ptr);
! 		}
! 		break;
! 
! 	/* extended i/o function pointers */
! 	case LDAP_X_OPT_EXTIO_FN_PTRS:
! 		if ( ((struct ldap_x_ext_io_fns *) optdata)->lextiof_size !=
! 		    LDAP_X_EXTIO_FNS_SIZE ) {
! 			LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
! 			rc = -1;
! 		} else {
! 			/* struct copy */
! 			*((struct ldap_x_ext_io_fns *) optdata) =
! 			    ld->ld_ext_io_fns;
! 		}
  		break;
  #endif /* LDAP_SSLIO_HOOKS */
  
***************
*** 231,236 ****
--- 244,253 ----
  		*((char **) optdata) = nsldapi_strdup( ld->ld_defhost );
  		break;
  
+         case LDAP_X_OPT_CONNECT_TIMEOUT:
+                 *((int *) optdata) = ld->ld_connect_timeout;
+                 break;
+ 
  	default:
  		LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
  		rc = -1;
***************
*** 254,259 ****
--- 271,277 ----
      { 0, "X_LDERRNO",			LDAP_API_FEATURE_X_LDERRNO },
      { 0, "X_MEMCACHE",			LDAP_API_FEATURE_X_MEMCACHE },
      { 0, "X_IO_FUNCTIONS",		LDAP_API_FEATURE_X_IO_FUNCTIONS },
+     { 0, "X_EXTIO_FUNCTIONS",		LDAP_API_FEATURE_X_EXTIO_FUNCTIONS },
      { 0, "X_DNS_FUNCTIONS",		LDAP_API_FEATURE_X_DNS_FUNCTIONS },
      { 0, "X_MEMALLOC_FUNCTIONS",	LDAP_API_FEATURE_X_MEMALLOC_FUNCTIONS },
      { 0, "X_THREAD_FUNCTIONS",		LDAP_API_FEATURE_X_THREAD_FUNCTIONS },
