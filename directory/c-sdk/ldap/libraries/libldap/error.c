*** error.c	Tue Feb  8 11:06:58 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/error.c	Thu Apr 27 14:49:01 2000
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
*** 112,117 ****
--- 108,127 ----
  	return( "Unknown error" );
  }
  
+ 
+ static char *
+ nsldapi_safe_strerror( e )
+ {
+ 	char *s;
+ 
+ 	if (( s = strerror( e )) == NULL ) {
+ 		s = "unknown error";
+ 	}
+ 
+ 	return( s );
+ }
+ 
+ 
  void
  LDAP_CALL
  ldap_perror( LDAP *ld, const char *s )
***************
*** 129,135 ****
  	}
  
  	if ( ld == NULL ) {
! 		sprintf( msg, "%s%s%s", s, separator, strerror( errno ) );
  		ber_err_print( msg );
  		return;
  	}
--- 139,146 ----
  	}
  
  	if ( ld == NULL ) {
! 		sprintf( msg, "%s%s%s", s, separator,
! 		    nsldapi_safe_strerror( errno ) );
  		ber_err_print( msg );
  		return;
  	}
***************
*** 142,149 ****
  				    ldap_errlist[i].e_reason );
  			ber_err_print( msg );
  			if ( err == LDAP_CONNECT_ERROR ) {
! 			    ber_err_print( " - " );
! 			    ber_err_print( strerror( LDAP_GET_ERRNO( ld )));
  			}
  			ber_err_print( "\n" );
  			if ( matched != NULL && *matched != '\0' ) {
--- 153,161 ----
  				    ldap_errlist[i].e_reason );
  			ber_err_print( msg );
  			if ( err == LDAP_CONNECT_ERROR ) {
! 				ber_err_print( " - " );
! 				ber_err_print( nsldapi_safe_strerror(
! 				    LDAP_GET_ERRNO( ld )));
  			}
  			ber_err_print( "\n" );
  			if ( matched != NULL && *matched != '\0' ) {
