*** setoption.c	Tue Feb  8 11:07:00 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/setoption.c	Mon May  1 10:47:14 2000
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
  /*
   * setoption.c - ldap_set_option implementation 
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
  /*
   * setoption.c - ldap_set_option implementation 
***************
*** 99,105 ****
  	}
  
  	rc = 0;
! 	LDAP_MUTEX_LOCK( ld, LDAP_OPTION_LOCK );
  	switch( option ) {
  	/* options that can be turned on and off */
  #ifdef LDAP_DNS
--- 95,103 ----
  	}
  
  	rc = 0;
! 	if ( ld != &nsldapi_ld_defaults ) {
! 	    LDAP_MUTEX_LOCK( ld, LDAP_OPTION_LOCK );
! 	}
  	switch( option ) {
  	/* options that can be turned on and off */
  #ifdef LDAP_DNS
***************
*** 173,194 ****
  #ifdef LDAP_SSLIO_HOOKS
  	/* i/o function pointers */
  	case LDAP_OPT_IO_FN_PTRS:
! 		/* struct copy */
! 		ld->ld_io = *((struct ldap_io_fns *) optdata);
! 		if ( NULL != ld->ld_sbp ) {
! 			rc = ber_sockbuf_set_option( ld->ld_sbp,
! 			    LBER_SOCKBUF_OPT_READ_FN,
! 			    (void *) ld->ld_read_fn );
! 			rc |= ber_sockbuf_set_option( ld->ld_sbp,
! 			    LBER_SOCKBUF_OPT_WRITE_FN,
! 			    (void *) ld->ld_write_fn );
! 			if ( rc != 0 ) {
! 				LDAP_SET_LDERRNO( ld, LDAP_LOCAL_ERROR,
! 				    NULL, NULL );
! 				rc = -1;
! 			}
  		}
  		break;
  #endif
  
  	/* thread function pointers */
--- 171,193 ----
  #ifdef LDAP_SSLIO_HOOKS
  	/* i/o function pointers */
  	case LDAP_OPT_IO_FN_PTRS:
! 		if (( rc = nsldapi_install_compat_io_fns( ld,
! 		    (struct ldap_io_fns *)optdata )) != LDAP_SUCCESS ) {
! 			LDAP_SET_LDERRNO( ld, rc, NULL, NULL );
! 			rc = -1;
  		}
  		break;
+ 
+ 	/* extended i/o function pointers */
+ 	case LDAP_X_OPT_EXTIO_FN_PTRS:
+ 		/* struct copy */
+ 		ld->ld_ext_io_fns = *((struct ldap_x_ext_io_fns *) optdata);
+ 		if (( rc = nsldapi_install_lber_extiofns( ld, ld->ld_sbp ))
+ 		    != LDAP_SUCCESS ) {
+ 			LDAP_SET_LDERRNO( ld, rc, NULL, NULL );
+ 			rc = -1;
+                 }
+ 		break;
  #endif
  
  	/* thread function pointers */
***************
*** 199,206 ****
  		    ld != &nsldapi_ld_defaults &&
  		    ld->ld_mutex != NULL ) {
  			int i;
! 			for( i=0; i<LDAP_MAX_LOCK; i++ )
  				ld->ld_mutex[i] = (ld->ld_mutex_alloc_fn)();
  		}
  		/*
  		 * Because we have just replaced the locking functions,
--- 198,206 ----
  		    ld != &nsldapi_ld_defaults &&
  		    ld->ld_mutex != NULL ) {
  			int i;
! 			for( i=0; i<LDAP_MAX_LOCK; i++ ) {
  				ld->ld_mutex[i] = (ld->ld_mutex_alloc_fn)();
+ 			}
  		}
  		/*
  		 * Because we have just replaced the locking functions,
***************
*** 304,314 ****
  		ld->ld_defhost = nsldapi_strdup((char *) optdata);
  		break;
  
  	default:
  		LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
  		rc = -1;
  	}
  
! 	LDAP_MUTEX_UNLOCK( ld, LDAP_OPTION_LOCK );
  	return( rc );
  }
--- 304,320 ----
  		ld->ld_defhost = nsldapi_strdup((char *) optdata);
  		break;
  
+ 	case LDAP_X_OPT_CONNECT_TIMEOUT:
+ 		ld->ld_connect_timeout = *((int *) optdata);
+ 		break;
+ 
  	default:
  		LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
  		rc = -1;
  	}
  
! 	if ( ld != &nsldapi_ld_defaults ) {
! 	    LDAP_MUTEX_UNLOCK( ld, LDAP_OPTION_LOCK );
! 	}
  	return( rc );
  }
