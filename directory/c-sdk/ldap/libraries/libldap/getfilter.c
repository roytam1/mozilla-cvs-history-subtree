*** getfilter.c	Tue Feb  8 11:06:58 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/getfilter.c	Thu May 25 10:28:35 2000
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
   *  Copyright (c) 1993 Regents of the University of Michigan.
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
   *  Copyright (c) 1993 Regents of the University of Michigan.
***************
*** 105,112 ****
      char		*tag, **tok;
      int			tokcnt, i;
  
!     if ( buflen < 0 || ( lfdp = (LDAPFiltDesc *)NSLDAPI_CALLOC( 1,
! 	    sizeof( LDAPFiltDesc))) == NULL ) {
  	return( NULL );
      }
  
--- 101,109 ----
      char		*tag, **tok;
      int			tokcnt, i;
  
!     if ( (buf == NULL) || (buflen < 0) ||
! 	 ( lfdp = (LDAPFiltDesc *)NSLDAPI_CALLOC(1, sizeof( LDAPFiltDesc)))
! 	 == NULL ) {
  	return( NULL );
      }
  
***************
*** 253,259 ****
  {
      LDAPFiltList	*flp;
  
!     if ( lfdp == NULL ) {
  	return( NULL );	/* punt */
      }
  
--- 250,256 ----
  {
      LDAPFiltList	*flp;
  
!     if ( lfdp == NULL || tagpat == NULL || value == NULL ) {
  	return( NULL );	/* punt */
      }
  
