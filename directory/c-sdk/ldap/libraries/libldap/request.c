*** request.c	Fri Feb 11 16:39:09 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/request.c	Tue Jun  6 13:49:10 2000
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
   *  Copyright (c) 1995 Regents of the University of Michigan.
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
   *  Copyright (c) 1995 Regents of the University of Michigan.
***************
*** 35,57 ****
  
  #include "ldap-int.h"
  
- /* HCL 1.56 caused some wierdness, because ttypdefaults.h on SSF defines CTIME 0
-  * This little bit fixes that 
-  */
- #if defined(LDAP_DEBUG) && defined(OSF1)
- #if defined(CTIME)
- #undef CTIME
- #define CTIME( c, b, l )		ctime( c )
- #endif
- #endif
- 
- #if defined(LINUX2_0) || defined(LINUX2_1)
- #if defined(CTIME)
- #undef CTIME
- #define CTIME( c, b, l )		ctime( c )
- #endif
- #endif
- 
  static LDAPConn *find_connection( LDAP *ld, LDAPServer *srv, int any );
  static void use_connection( LDAP *ld, LDAPConn *lc );
  static void free_servers( LDAPServer *srvlist );
--- 31,36 ----
***************
*** 293,299 ****
  		/* need to continue write later */
  		if (ld->ld_options & LDAP_BITOPT_ASYNC && err == -2 ) {	
  			lr->lr_status = LDAP_REQST_WRITING;
! 			nsldapi_mark_select_write( ld, lc->lconn_sb );
  		} else {
  
  			LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
--- 272,278 ----
  		/* need to continue write later */
  		if (ld->ld_options & LDAP_BITOPT_ASYNC && err == -2 ) {	
  			lr->lr_status = LDAP_REQST_WRITING;
! 			nsldapi_iostatus_interest_write( ld, lc->lconn_sb );
  		} else {
  
  			LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
***************
*** 311,322 ****
  		}
  
  		/* sent -- waiting for a response */
!         if (ld->ld_options & LDAP_BITOPT_ASYNC)
!         {
!             lc->lconn_status = LDAP_CONNST_CONNECTED;
!         }
  
! 		nsldapi_mark_select_read( ld, lc->lconn_sb );
  	}
  	LDAP_MUTEX_UNLOCK( ld, LDAP_REQ_LOCK );
  	LDAP_MUTEX_UNLOCK( ld, LDAP_CONN_LOCK );
--- 290,300 ----
  		}
  
  		/* sent -- waiting for a response */
! 		if (ld->ld_options & LDAP_BITOPT_ASYNC) {
! 			lc->lconn_status = LDAP_CONNST_CONNECTED;
! 		}
  
! 		nsldapi_iostatus_interest_read( ld, lc->lconn_sb );
  	}
  	LDAP_MUTEX_UNLOCK( ld, LDAP_REQ_LOCK );
  	LDAP_MUTEX_UNLOCK( ld, LDAP_CONN_LOCK );
***************
*** 395,403 ****
  		 * we have allocated a new sockbuf
  		 * set I/O routines to match those in default LDAP sockbuf
  		 */
! 		IFP	sb_fn;
  
  		if ( ber_sockbuf_get_option( ld->ld_sbp,
  		    LBER_SOCKBUF_OPT_READ_FN, (void *)&sb_fn ) == 0
  		    && sb_fn != NULL ) {
  			ber_sockbuf_set_option( sb, LBER_SOCKBUF_OPT_READ_FN,
--- 373,389 ----
  		 * we have allocated a new sockbuf
  		 * set I/O routines to match those in default LDAP sockbuf
  		 */
! 		IFP				sb_fn;
! 		struct lber_x_ext_io_fns	extiofns;
! 		
! 		extiofns.lbextiofn_size = LBER_X_EXTIO_FNS_SIZE;
  
  		if ( ber_sockbuf_get_option( ld->ld_sbp,
+ 		    LBER_SOCKBUF_OPT_EXT_IO_FNS, &extiofns ) == 0 ) {
+ 			ber_sockbuf_set_option( sb,
+ 			    LBER_SOCKBUF_OPT_EXT_IO_FNS, &extiofns );
+ 		}
+ 		if ( ber_sockbuf_get_option( ld->ld_sbp,
  		    LBER_SOCKBUF_OPT_READ_FN, (void *)&sb_fn ) == 0
  		    && sb_fn != NULL ) {
  			ber_sockbuf_set_option( sb, LBER_SOCKBUF_OPT_READ_FN,
***************
*** 421,429 ****
           * save the return code for later
           */ 
  		for ( srv = *srvlistp; srv != NULL; srv = srv->lsrv_next ) {
! 			rc = nsldapi_open_ldap_connection( ld, lc->lconn_sb,
! 				   srv->lsrv_host, srv->lsrv_port, &lc->lconn_krbinstance, 1,
! 			       (  srv->lsrv_options & LDAP_SRV_OPT_SECURE ) != 0 );
  			if (rc != -1) {
  				break;
  			}
--- 407,416 ----
           * save the return code for later
           */ 
  		for ( srv = *srvlistp; srv != NULL; srv = srv->lsrv_next ) {
! 			rc = nsldapi_connect_to_host( ld, lc->lconn_sb,
! 				   srv->lsrv_host, srv->lsrv_port,
! 			       (  srv->lsrv_options & LDAP_SRV_OPT_SECURE ) != 0,
! 					&lc->lconn_krbinstance );
  			if (rc != -1) {
  				break;
  			}
***************
*** 579,591 ****
  
  	if ( force || --lc->lconn_refcnt <= 0 ) {
  		if ( lc->lconn_status == LDAP_CONNST_CONNECTED ) {
! 			nsldapi_mark_select_clear( ld, lc->lconn_sb );
  			if ( unbind ) {
  				nsldapi_send_unbind( ld, lc->lconn_sb,
  				    serverctrls, clientctrls );
  			}
- 			nsldapi_close_connection( ld, lc->lconn_sb );
  		}
  		prevlc = NULL;
  		for ( tmplc = ld->ld_conns; tmplc != NULL;
  		    tmplc = tmplc->lconn_next ) {
--- 566,578 ----
  
  	if ( force || --lc->lconn_refcnt <= 0 ) {
  		if ( lc->lconn_status == LDAP_CONNST_CONNECTED ) {
! 			nsldapi_iostatus_interest_clear( ld, lc->lconn_sb );
  			if ( unbind ) {
  				nsldapi_send_unbind( ld, lc->lconn_sb,
  				    serverctrls, clientctrls );
  			}
  		}
+ 		nsldapi_close_connection( ld, lc->lconn_sb );
  		prevlc = NULL;
  		for ( tmplc = ld->ld_conns; tmplc != NULL;
  		    tmplc = tmplc->lconn_next ) {
***************
*** 636,642 ****
  	LDAPConn	*lc;
  	char        msg[256];
  /* CTIME for this platform doesn't use this. */
! #if !defined(SUNOS4) && !defined(_WIN32)
  	char		buf[26];
  #endif
  
--- 623,629 ----
  	LDAPConn	*lc;
  	char        msg[256];
  /* CTIME for this platform doesn't use this. */
! #if !defined(SUNOS4) && !defined(_WIN32) && !defined(LINUX)
  	char		buf[26];
  #endif
  
***************
*** 662,668 ****
  		    "Connected" );
  		ber_err_print( msg );
  		sprintf( msg, "  last used: %s",
! 		    CTIME( (time_t *) &lc->lconn_lastused, buf, sizeof(buf) ));
  		ber_err_print( msg );
  		if ( lc->lconn_ber != NULLBER ) {
  			ber_err_print( "  partial response has been received:\n" );
--- 649,656 ----
  		    "Connected" );
  		ber_err_print( msg );
  		sprintf( msg, "  last used: %s",
! 		    NSLDAPI_CTIME( (time_t *) &lc->lconn_lastused, buf,
! 				sizeof(buf) ));
  		ber_err_print( msg );
  		if ( lc->lconn_ber != NULLBER ) {
  			ber_err_print( "  partial response has been received:\n" );
***************
*** 968,974 ****
  
  	secure = (( ludp->lud_options & LDAP_URL_OPT_SECURE ) != 0 );
  
! 	if ( secure && ld->ld_ssl_enable_fn == NULL ) {
  		LDAPDebug( LDAP_DEBUG_TRACE,
  		    "ignoring LDAPS %s <%s>\n", desc, refurl, 0 );
  		*unknownp = 1;
--- 956,963 ----
  
  	secure = (( ludp->lud_options & LDAP_URL_OPT_SECURE ) != 0 );
  
! /* XXXmcs: can't tell if secure is supported by connect callback */
! 	if ( secure && ld->ld_extconnect_fn == NULL ) {
  		LDAPDebug( LDAP_DEBUG_TRACE,
  		    "ignoring LDAPS %s <%s>\n", desc, refurl, 0 );
  		*unknownp = 1;
***************
*** 1227,1233 ****
  			lr->lr_status = LDAP_REQST_CONNDEAD;
  			if ( lr->lr_conn != NULL ) {
  				lr->lr_conn->lconn_status = LDAP_CONNST_DEAD;
! 				nsldapi_mark_select_clear( ld,
  				    lr->lr_conn->lconn_sb );
  			}
  		}
--- 1216,1222 ----
  			lr->lr_status = LDAP_REQST_CONNDEAD;
  			if ( lr->lr_conn != NULL ) {
  				lr->lr_conn->lconn_status = LDAP_CONNST_DEAD;
! 				nsldapi_iostatus_interest_clear( ld,
  				    lr->lr_conn->lconn_sb );
  			}
  		}
