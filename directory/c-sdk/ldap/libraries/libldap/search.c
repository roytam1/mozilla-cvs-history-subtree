*** search.c	Tue Feb  8 11:07:00 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/search.c	Tue Apr 25 14:55:33 2000
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
*** 174,180 ****
  	}
  
  	if ( msgidp == NULL || ( scope != LDAP_SCOPE_BASE
! 	    && scope != LDAP_SCOPE_ONELEVEL && scope != LDAP_SCOPE_SUBTREE )) {
  		LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
                  return( LDAP_PARAM_ERROR );
          }
--- 170,177 ----
  	}
  
  	if ( msgidp == NULL || ( scope != LDAP_SCOPE_BASE
! 	    && scope != LDAP_SCOPE_ONELEVEL && scope != LDAP_SCOPE_SUBTREE )
! 		|| ( sizelimit < -1 )) {
  		LDAP_SET_LDERRNO( ld, LDAP_PARAM_ERROR, NULL, NULL );
                  return( LDAP_PARAM_ERROR );
          }
***************
*** 979,988 ****
--- 976,991 ----
  	if (( err = nsldapi_search( ld, base, scope, filter, attrs, attrsonly,
  	    serverctrls, clientctrls, timelimit, sizelimit, &msgid ))
  	    != LDAP_SUCCESS ) {
+ 		if ( res != NULL ) {
+ 			*res = NULL;
+ 		}
  		return( err );
  	}
  
  	if ( ldap_result( ld, msgid, 1, localtimeoutp, res ) == -1 ) {
+ 		/*
+ 		 * Error.  ldap_result() sets *res to NULL for us.
+ 		 */
  		return( LDAP_GET_LDERRNO( ld, NULL, NULL ) );
  	}
  
***************
*** 990,995 ****
--- 993,1001 ----
  		(void) ldap_abandon( ld, msgid );
  		err = LDAP_TIMEOUT;
  		LDAP_SET_LDERRNO( ld, err, NULL, NULL );
+ 		if ( res != NULL ) {
+ 			*res = NULL;
+ 		}
  		return( err );
  	}
  
