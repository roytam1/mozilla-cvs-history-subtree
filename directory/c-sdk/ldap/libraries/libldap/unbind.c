*** unbind.c	Tue Feb  8 11:07:01 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/unbind.c	Thu May  4 11:50:35 2000
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
   *  Copyright (c) 1990 Regents of the University of Michigan.
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
   *  Copyright (c) 1990 Regents of the University of Michigan.
***************
*** 121,126 ****
--- 117,131 ----
  		LDAP_MUTEX_UNLOCK( ld, LDAP_CACHE_LOCK );
  	}
  
+ 	/* call the dispose handle I/O callback if one is defined */
+ 	if ( ld->ld_extdisposehandle_fn != NULL ) {
+ 	    /*
+ 	     * We always pass the session extended I/O argument to
+ 	     * the dispose handle callback.
+ 	     */
+ 	    ld->ld_extdisposehandle_fn( ld, ld->ld_ext_session_arg );
+ 	}
+ 
  	if ( ld->ld_error != NULL )
  		NSLDAPI_FREE( ld->ld_error );
  	if ( ld->ld_matched != NULL )
***************
*** 135,142 ****
  		NSLDAPI_FREE( ld->ld_abandoned );
  	if ( ld->ld_sbp != NULL )
  		ber_sockbuf_free( ld->ld_sbp );
- 	if ( ld->ld_selectinfo != NULL )
- 		nsldapi_free_select_info( ld->ld_selectinfo );
  	if ( ld->ld_defhost != NULL )
  		NSLDAPI_FREE( ld->ld_defhost );
  	if ( ld->ld_servercontrols != NULL )
--- 140,145 ----
***************
*** 145,150 ****
--- 148,154 ----
  		ldap_controls_free( ld->ld_clientcontrols );
  	if ( ld->ld_preferred_language != NULL )
  		NSLDAPI_FREE( ld->ld_preferred_language );
+ 	nsldapi_iostatus_free( ld );
  
  	/*
  	 * XXXmcs: should use cache function pointers to hook in memcache
***************
*** 153,160 ****
  		ldap_memcache_set( ld, NULL );
  	}
  
!         for( i=0; i<LDAP_MAX_LOCK; i++ )
  		LDAP_MUTEX_FREE( ld, ld->ld_mutex[i] );
  
  	NSLDAPI_FREE( ld->ld_mutex );
  	NSLDAPI_FREE( (char *) ld );
--- 157,165 ----
  		ldap_memcache_set( ld, NULL );
  	}
  
!         for( i=0; i<LDAP_MAX_LOCK; i++ ) {
  		LDAP_MUTEX_FREE( ld, ld->ld_mutex[i] );
+ 	}
  
  	NSLDAPI_FREE( ld->ld_mutex );
  	NSLDAPI_FREE( (char *) ld );
