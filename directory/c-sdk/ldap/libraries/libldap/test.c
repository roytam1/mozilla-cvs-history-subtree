*** test.c	Tue Feb  8 11:07:01 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/test.c	Thu Apr 27 14:49:08 2000
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
  /* test.c - a simple test harness. */
  #include <stdio.h>
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
  /* test.c - a simple test harness. */
  #include <stdio.h>
***************
*** 58,63 ****
--- 54,66 ----
  #endif /* DOS */
  #endif /* MACOS */
  
+ #undef NET_SSL
+ 
+ #if defined(NET_SSL)
+ #include <sec.h>
+ static SECCertDBHandle        certdbhandle;
+ #endif
+ 
  #include "ldap.h"
  #include "disptmpl.h"
  #include "ldaplog.h"
***************
*** 532,542 ****
  #ifndef _WIN32
  #ifdef LDAP_DEBUG
  			ldap_debug = atoi( optarg ) | LDAP_DEBUG_ANY;
- #if 0
  			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
! 				lber_debug = ldap_debug;
  			}
- #endif
  #else
  			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
  #endif
--- 535,544 ----
  #ifndef _WIN32
  #ifdef LDAP_DEBUG
  			ldap_debug = atoi( optarg ) | LDAP_DEBUG_ANY;
  			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
! 				ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL,
! 					&ldap_debug );
  			}
  #else
  			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
  #endif
***************
*** 891,901 ****
  #ifdef LDAP_DEBUG
  			getline( line, sizeof(line), stdin, "debug level? " );
  			ldap_debug = atoi( line ) | LDAP_DEBUG_ANY;
- #if 0
  			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
! 				lber_debug = ldap_debug;
  			}
- #endif
  #else
  			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
  #endif
--- 893,902 ----
  #ifdef LDAP_DEBUG
  			getline( line, sizeof(line), stdin, "debug level? " );
  			ldap_debug = atoi( line ) | LDAP_DEBUG_ANY;
  			if ( ldap_debug & LDAP_DEBUG_PACKETS ) {
! 				ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL,
! 					&ldap_debug );
  			}
  #else
  			printf( "Compile with -DLDAP_DEBUG for debugging\n" );
  #endif
***************
*** 1250,1255 ****
--- 1251,1281 ----
  					    NULL );
  				}
  			}
+ #ifdef NET_SSL
+ 			getline( line, sizeof(line), stdin,
+ 				"Use Secure Sockets Layer - SSL (0=no, 1=yes)?" );
+ 			optval = ( atoi( line ) != 0 );
+ 			if ( optval ) {
+ 				getline( line, sizeof(line), stdin,
+ 				    "security DB path?" ); 
+ 				if ( ldapssl_client_init( (*line == '\0') ?
+ 				    NULL : line, &certdbhandle ) < 0 ) {
+ 					perror( "ldapssl_client_init" );
+ 					optval = 0;     /* SSL not avail. */
+ 				} else if ( ldapssl_install_routines( ld )
+ 				    < 0 ) {
+ 					ldap_perror( ld,
+ 					    "ldapssl_install_routines" );
+ 					optval = 0;     /* SSL not avail. */
+ 				}
+ 			}
+ 
+ #ifdef LDAP_SSLIO_HOOKS
+ 			ldap_set_option( ld, LDAP_OPT_SSL,
+ 			    optval ? LDAP_OPT_ON : LDAP_OPT_OFF );
+ #endif
+ #endif
+ 
  			getline( line, sizeof(line), stdin, "Reconnect?" );
  			ldap_set_option( ld, LDAP_OPT_RECONNECT,
  			    ( atoi( line ) == 0 ) ? LDAP_OPT_OFF :
