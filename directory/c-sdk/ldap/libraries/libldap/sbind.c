*** sbind.c	Tue Feb  8 11:07:00 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/sbind.c	Tue Apr 25 13:26:48 2000
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
!  *
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
   *  Copyright (c) 1993 Regents of the University of Michigan.
--- 1,19 ----
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public License
!  * Version 1.0 (the "NPL"); you may not use this file except in
!  * compliance with the NPL.  You may obtain a copy of the NPL at
!  * http://www.mozilla.org/NPL/
!  *
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
   *  Copyright (c) 1993 Regents of the University of Michigan.
***************
*** 200,206 ****
  	    && 0 == strcmp( dn, binddn )) {
  		rc = LDAP_SUCCESS;
  		LDAP_SET_LDERRNO( ld, rc, NULL, NULL );
! 		goto unlock_and_return;
  	}
  
  	/*
--- 196,202 ----
  	    && 0 == strcmp( dn, binddn )) {
  		rc = LDAP_SUCCESS;
  		LDAP_SET_LDERRNO( ld, rc, NULL, NULL );
! 		return rc;
  	}
  
  	/*
***************
*** 226,232 ****
--- 222,233 ----
  
  	/*
  	 * finally, bind (this will open a new connection if necessary)
+ 	 *
+ 	 * do everything under the protection of the result lock to
+ 	 * ensure that only one thread will be in this code at a time.
+ 	 * XXXmcs: we should use a condition variable instead?
  	 */
+ 	LDAP_MUTEX_LOCK( ld, LDAP_RESULT_LOCK );
  	if ( (msgid = simple_bind_nolock( ld, dn, passwd, 0 )) == -1 ) {
  		rc = LDAP_GET_LDERRNO( ld, NULL, NULL );
  		goto unlock_and_return;
***************
*** 252,256 ****
--- 253,258 ----
  	rc = ldap_result2error( ld, result, 1 );
  
  unlock_and_return:
+ 	LDAP_MUTEX_UNLOCK( ld, LDAP_RESULT_LOCK );
  	return( rc );
  }
