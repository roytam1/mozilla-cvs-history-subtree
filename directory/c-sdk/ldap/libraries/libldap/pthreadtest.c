*** pthreadtest.c	Tue Feb  8 11:06:59 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/pthreadtest.c	Thu Apr 27 14:49:06 2000
***************
*** 1,24 ****
- /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
-  *
-  * The contents of this file are subject to the Netscape Public
-  * License Version 1.1 (the "License"); you may not use this file
-  * except in compliance with the License. You may obtain a copy of
-  * the License at http://www.mozilla.org/NPL/
-  *
-  * Software distributed under the License is distributed on an "AS
-  * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
-  * implied. See the License for the specific language governing
-  * rights and limitations under the License.
-  *
-  * The Original Code is mozilla.org code.
-  *
-  * The Initial Developer of the Original Code is Netscape
-  * Communications Corporation.  Portions created by Netscape are
-  * Copyright (C) 1998 Netscape Communications Corporation. All
-  * Rights Reserved.
-  *
-  * Contributor(s): 
-  */
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
--- 1,3 ----
***************
*** 966,981 ****
  
  
  static int
! get_ld_error( char **matched, char **errmsg, void *dummy )
  {
  	struct ldap_error *le;
  
  	le = pthread_getspecific( key );
! 	if ( matched != NULL ) {
! 		*matched = le->le_matched;
  	}
! 	if ( errmsg != NULL ) {
! 		*errmsg = le->le_errmsg;
  	}
  	return( le->le_errno );
  }
--- 945,960 ----
  
  
  static int
! get_ld_error( char **matchedp, char **errmsgp, void *dummy )
  {
  	struct ldap_error *le;
  
  	le = pthread_getspecific( key );
! 	if ( matchedp != NULL ) {
! 		*matchedp = le->le_matched;
  	}
! 	if ( errmsgp != NULL ) {
! 		*errmsgp = le->le_errmsg;
  	}
  	return( le->le_errno );
  }
