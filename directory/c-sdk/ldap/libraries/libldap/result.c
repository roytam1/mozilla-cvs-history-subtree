*** result.c	Wed Aug 16 10:03:33 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/result.c	Fri May  5 10:48:52 2000
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
*** 51,59 ****
  static int cldap_select1( LDAP *ld, struct timeval *timeout );
  #endif
  static void link_pend( LDAP *ld, LDAPPend *lp );
  static void unlink_pend( LDAP *ld, LDAPPend *lp );
  static int unlink_msg( LDAP *ld, int msgid, int all );
! static int nsldapi_mutex_trylock( LDAP *ld, LDAPLock lockno );
  
  /*
   * ldap_result - wait for an ldap result response to a message from the
--- 47,56 ----
  static int cldap_select1( LDAP *ld, struct timeval *timeout );
  #endif
  static void link_pend( LDAP *ld, LDAPPend *lp );
+ #if 0 /* these functions are no longer used */
  static void unlink_pend( LDAP *ld, LDAPPend *lp );
  static int unlink_msg( LDAP *ld, int msgid, int all );
! #endif /* 0 */
  
  /*
   * ldap_result - wait for an ldap result response to a message from the
***************
*** 79,85 ****
      LDAPMessage		**result
  )
  {
! 	int		rc, ret;
  
  	LDAPDebug( LDAP_DEBUG_TRACE, "ldap_result\n", 0, 0, 0 );
  
--- 76,82 ----
      LDAPMessage		**result
  )
  {
! 	int		rc;
  
  	LDAPDebug( LDAP_DEBUG_TRACE, "ldap_result\n", 0, 0, 0 );
  
***************
*** 87,108 ****
  		return( -1 );	/* punt */
  	}
  
! 	while( 1 ) {
! 		if( (ret = nsldapi_mutex_trylock( ld, LDAP_RESULT_LOCK )) == 0 )
! 		{
! 			LDAP_MUTEX_BC_LOCK( ld, LDAP_RESULT_LOCK );
! 			rc= nsldapi_result_nolock( ld, msgid, all, 1, timeout, result );
! 			break;
! 		}
! 		else {
! 			rc = nsldapi_wait_result( ld, msgid, all, timeout, result );
! 			if( rc == -2 )
! 				continue;
! 			else
! 				break;
! 		}
! 	}
  
  	return( rc );
  }
  
--- 84,95 ----
  		return( -1 );	/* punt */
  	}
  
! 	LDAP_MUTEX_LOCK( ld, LDAP_RESULT_LOCK );
  
+ 	rc = nsldapi_result_nolock(ld, msgid, all, 1, timeout, result);
+ 
+ 	LDAP_MUTEX_UNLOCK( ld, LDAP_RESULT_LOCK );
+ 
  	return( rc );
  }
  
***************
*** 145,152 ****
  		    (all || NSLDAPI_IS_SEARCH_RESULT( rc )), *result );
  	}
  
- 	LDAP_MUTEX_UNLOCK( ld, LDAP_RESULT_LOCK );
- 	POST( ld, LDAP_RES_ANY, NULL );
  	return( rc );
  }
  
--- 132,137 ----
***************
*** 223,230 ****
  	 * a request that is still pending, return failure.
  	 */
  	if ( lm == NULL 
!              || ( lr = nsldapi_find_request_by_msgid( ld, lm->lm_msgid ))
! 		   != NULL && lr->lr_outrefcnt > 0 ) {
  		LDAP_MUTEX_UNLOCK( ld, LDAP_RESP_LOCK );
  		LDAPDebug( LDAP_DEBUG_TRACE,
  		    "<= check_response_queue NOT FOUND\n",
--- 208,215 ----
  	 * a request that is still pending, return failure.
  	 */
  	if ( lm == NULL 
!              || (( lr = nsldapi_find_request_by_msgid( ld, lm->lm_msgid ))
! 		   != NULL && lr->lr_outrefcnt > 0 )) {
  		LDAP_MUTEX_UNLOCK( ld, LDAP_RESP_LOCK );
  		LDAPDebug( LDAP_DEBUG_TRACE,
  		    "<= check_response_queue NOT FOUND\n",
***************
*** 326,332 ****
  			LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
  			return( -1 );	/* connection dead */
  		}
!         LDAP_MUTEX_UNLOCK( ld, LDAP_REQ_LOCK );
  	}
  
  	if ( timeout == NULL ) {
--- 311,317 ----
  			LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
  			return( -1 );	/* connection dead */
  		}
! 		LDAP_MUTEX_UNLOCK( ld, LDAP_REQ_LOCK );
  	}
  
  	if ( timeout == NULL ) {
***************
*** 359,370 ****
  		LDAP_MUTEX_UNLOCK( ld, LDAP_CONN_LOCK );
  
  		if ( lc == NULL ) {
! 			rc = nsldapi_do_ldap_select( ld, tvp );
  
  #if defined( LDAP_DEBUG ) && !defined( macintosh ) && !defined( DOS )
  			if ( rc == -1 ) {
  			    LDAPDebug( LDAP_DEBUG_TRACE,
! 				    "nsldapi_do_ldap_select returned -1: errno %d\n",
  				    LDAP_GET_ERRNO( ld ), 0, 0 );
  			}
  #endif
--- 344,355 ----
  		LDAP_MUTEX_UNLOCK( ld, LDAP_CONN_LOCK );
  
  		if ( lc == NULL ) {
! 			rc = nsldapi_iostatus_poll( ld, tvp );
  
  #if defined( LDAP_DEBUG ) && !defined( macintosh ) && !defined( DOS )
  			if ( rc == -1 ) {
  			    LDAPDebug( LDAP_DEBUG_TRACE,
! 				    "nsldapi_iostatus_poll returned -1: errno %d\n",
  				    LDAP_GET_ERRNO( ld ), 0, 0 );
  			}
  #endif
***************
*** 399,413 ****
  					nextlc = lc->lconn_next;
  					if ( lc->lconn_status ==
  					    LDAP_CONNST_CONNECTED &&
! 					    nsldapi_is_read_ready( ld,
  					    lc->lconn_sb )) {
  						rc = read1msg( ld, msgid, all,
  						    lc->lconn_sb, lc, result );
  					}
  					else if (ld->ld_options & LDAP_BITOPT_ASYNC) {
!                         if(   lr
                                && lc->lconn_status == LDAP_CONNST_CONNECTING
!                               && nsldapi_is_write_ready( ld, lc->lconn_sb ) ) {
                              rc = nsldapi_ber_flush( ld, lc->lconn_sb, lr->lr_ber, 0, 1 );
                              if ( rc == 0 ) {
                                  rc = LDAP_RES_BIND;
--- 384,399 ----
  					nextlc = lc->lconn_next;
  					if ( lc->lconn_status ==
  					    LDAP_CONNST_CONNECTED &&
! 					    nsldapi_iostatus_is_read_ready( ld,
  					    lc->lconn_sb )) {
  						rc = read1msg( ld, msgid, all,
  						    lc->lconn_sb, lc, result );
  					}
  					else if (ld->ld_options & LDAP_BITOPT_ASYNC) {
!                         if ( lr
                                && lc->lconn_status == LDAP_CONNST_CONNECTING
!                               && nsldapi_iostatus_is_write_ready( ld,
! 			      lc->lconn_sb ) ) {
                              rc = nsldapi_ber_flush( ld, lc->lconn_sb, lr->lr_ber, 0, 1 );
                              if ( rc == 0 ) {
                                  rc = LDAP_RES_BIND;
***************
*** 415,421 ****
                                  
                                  lr->lr_ber->ber_end = lr->lr_ber->ber_ptr;
                                  lr->lr_ber->ber_ptr = lr->lr_ber->ber_buf;
!                                 nsldapi_mark_select_read( ld, lc->lconn_sb );
                              }
                              else if ( rc == -1 ) {
                                  LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
--- 401,407 ----
                                  
                                  lr->lr_ber->ber_end = lr->lr_ber->ber_ptr;
                                  lr->lr_ber->ber_ptr = lr->lr_ber->ber_buf;
!                                 nsldapi_iostatus_interest_read( ld, lc->lconn_sb );
                              }
                              else if ( rc == -1 ) {
                                  LDAP_SET_LDERRNO( ld, LDAP_SERVER_DOWN, NULL, NULL );
***************
*** 1031,1041 ****
  
  #if defined( CLDAP )
  #if !defined( macintosh ) && !defined( DOS ) && !defined( _WINDOWS ) && !defined(XP_OS2)
  static int
  cldap_select1( LDAP *ld, struct timeval *timeout )
  {
! 	fd_set		readfds;
  	static int	tblsize = 0;
  
  	if ( tblsize == 0 ) {
  #ifdef USE_SYSCONF
--- 1017,1029 ----
  
  #if defined( CLDAP )
  #if !defined( macintosh ) && !defined( DOS ) && !defined( _WINDOWS ) && !defined(XP_OS2)
+ /* XXXmcs: was revised to support extended I/O callbacks but never compiled! */
  static int
  cldap_select1( LDAP *ld, struct timeval *timeout )
  {
! 	int		rc;
  	static int	tblsize = 0;
+ 	NSLDAPIIOStatus	*iosp = ld->ld_iostatus;
  
  	if ( tblsize == 0 ) {
  #ifdef USE_SYSCONF
***************
*** 1052,1066 ****
  		tblsize = FD_SETSIZE - 1;
  	}
  
! 	FD_ZERO( &readfds );
! 	FD_SET( ld->ld_sbp->sb_sd, &readfds );
  
! 	if ( ld->ld_select_fn != NULL ) {
! 		return( ld->ld_select_fn( tblsize, &readfds, 0, 0, timeout ) );
! 	} else {
  		/* XXXmcs: UNIX platforms should use poll() */
! 		return( select( tblsize, &readfds, 0, 0, timeout ) );
  	}
  }
  #endif /* !macintosh */
  
--- 1040,1070 ----
  		tblsize = FD_SETSIZE - 1;
  	}
  
! 	if ( NSLDAPI_IOSTATUS_TYPE_OSNATIVE == iosp->ios_type ) {
! 		fd_set		readfds;
  
! 		FD_ZERO( &readfds );
! 		FD_SET( ld->ld_sbp->sb_sd, &readfds );
! 
  		/* XXXmcs: UNIX platforms should use poll() */
! 		rc = select( tblsize, &readfds, 0, 0, timeout ) );
! 
! 	} else if ( NSLDAPI_IOSTATUS_TYPE_CALLBACK == iosp->ios_type ) {
! 		LDAP_X_PollFD	pollfds[ 1 ];
! 
! 		pollfds[0].lpoll_fd = ld->ld_sbp->sb_sd;
! 		pollfds[0].lpoll_arg = ld->ld_sbp->sb_arg;
! 		pollfds[0].lpoll_events = LDAP_X_POLLIN;
! 		pollfds[0].lpoll_revents = 0;
! 		rc = ld->ld_extpoll_fn( pollfds, 1, nsldapi_tv2ms( timeout ),
! 		    ld->ld_ext_session_arg );
! 	} else {
! 		LDAPDebug( LDAP_DEBUG_ANY,
! 		    "nsldapi_iostatus_poll: unknown I/O type %d\n",
! 		rc = 0; /* simulate a timeout (what else to do?) */
  	}
+ 
+ 	return( rc );
  }
  #endif /* !macintosh */
  
***************
*** 1069,1080 ****
--- 1073,1086 ----
  static int
  cldap_select1( LDAP *ld, struct timeval *timeout )
  {
+ 	/* XXXmcs: needs to be revised to support I/O callbacks */
  	return( tcpselect( ld->ld_sbp->sb_sd, timeout ));
  }
  #endif /* macintosh */
  
  
  #if (defined( DOS ) && defined( WINSOCK )) || defined( _WINDOWS ) || defined(XP_OS2)
+ /* XXXmcs: needs to be revised to support extended I/O callbacks */
  static int
  cldap_select1( LDAP *ld, struct timeval *timeout )
  {
***************
*** 1084,1094 ****
      FD_ZERO( &readfds );
      FD_SET( ld->ld_sbp->sb_sd, &readfds );
  
!     if ( ld->ld_select_fn != NULL ) {
! 		rc = ld->ld_select_fn( 1, &readfds, 0, 0, timeout );
! 	} else {
! 		rc = select( 1, &readfds, 0, 0, timeout );
! 	}
      return( rc == SOCKET_ERROR ? -1 : rc );
  }
  #endif /* WINSOCK || _WINDOWS */
--- 1090,1107 ----
      FD_ZERO( &readfds );
      FD_SET( ld->ld_sbp->sb_sd, &readfds );
  
!     if ( NSLDAPI_IO_TYPE_STANDARD == ld->ldiou_type &&
! 	NULL != ld->ld_select_fn ) {
! 	    rc = ld->ld_select_fn( 1, &readfds, 0, 0, timeout );
!     } else if ( NSLDAPI_IO_TYPE_EXTENDED == ld->ldiou_type &&
! 	NULL != ld->ld_extselect_fn ) {
! 	    rc = ld->ld_extselect_fn( ld->ld_ext_session_arg, 1, &readfds, 0,
! 		0, timeout ) );
!     } else {
! 	    /* XXXmcs: UNIX platforms should use poll() */
! 	    rc = select( 1, &readfds, 0, 0, timeout ) );
!     }
! 
      return( rc == SOCKET_ERROR ? -1 : rc );
  }
  #endif /* WINSOCK || _WINDOWS */
***************
*** 1247,1344 ****
  #endif /* CLDAP */
  
  int
- nsldapi_wait_result( LDAP *ld, int msgid, int all, struct timeval *timeout,
-     LDAPMessage **result )
- {
- 	LDAPPend	*lp;
- 
- 	LDAP_MUTEX_LOCK( ld, LDAP_PEND_LOCK );
- 	for( lp = ld->ld_pend; lp != NULL; lp = lp->lp_next )
- 	{
- 		if( lp->lp_msgid == msgid )
- 			break;
- 	}
- 	if( lp == NULL )
- 	{
- 		lp = (LDAPPend *) NSLDAPI_CALLOC( 1, sizeof( LDAPPend ) );
- 		if( lp == NULL )
- 		{
- 			LDAP_MUTEX_UNLOCK( ld, LDAP_PEND_LOCK );
- 			LDAP_SET_LDERRNO( ld, LDAP_NO_MEMORY, NULL, NULL );
- 			*result = NULL;
- 			return (-1);
- 		}
- 
- 		lp->lp_sema = (void *) LDAP_SEMA_ALLOC( ld );
- 		if( lp->lp_sema == NULL )
- 		{
- 			NSLDAPI_FREE( lp );
- 			LDAP_MUTEX_UNLOCK( ld, LDAP_PEND_LOCK );
- 			LDAP_SET_LDERRNO( ld, LDAP_NO_MEMORY, NULL, NULL );
- 			*result = NULL;
- 			return (-1);
- 		}
- 
- 		lp->lp_msgid = msgid;
- 		lp->lp_result = NULL;
- 		link_pend( ld, lp );
- 	}
- 	else
- 	{
- 		int rc;
- 
- 		if( (rc = unlink_msg( ld, lp->lp_msgid, all )) == -2  )
- 		{
- 			*result = NULL;
- 		}
- 		else
- 		{
- 			*result = lp->lp_result;
- 		}
- 		unlink_pend( ld, lp );
- 		LDAP_MUTEX_UNLOCK( ld, LDAP_PEND_LOCK );
- 		NSLDAPI_FREE( lp );
- 		LDAP_SET_LDERRNO( ld, LDAP_SUCCESS, NULL, NULL );
- 		return ( rc );
- 	}
- 	LDAP_MUTEX_UNLOCK( ld, LDAP_PEND_LOCK );
- 
- 	LDAP_SEMA_WAIT( ld, lp );
- 	LDAP_MUTEX_LOCK( ld, LDAP_PEND_LOCK );
- 	*result = lp->lp_result;
- 	{
- 		int rc;
- 
- 		if( *result == NULL )
- 			rc = -2;
- 		else
- 		{
- 			if( (rc = unlink_msg( ld, lp->lp_msgid, all )) == -2 )
- 			{
- 				*result = NULL;
- 			}
- 			else
- 			{
- 				if ( ld->ld_memcache != NULL &&
- 					NSLDAPI_SEARCH_RELATED_RESULT( rc ) &&
- 					!((*result)->lm_fromcache ))
- 				{
- 				  ldap_memcache_append( ld, (*result)->lm_msgid,
- 		    			(all || NSLDAPI_IS_SEARCH_RESULT( rc )),
- 						*result );
- 				}
- 			}
- 
- 		}
- 		unlink_pend( ld, lp );
- 		LDAP_MUTEX_UNLOCK( ld, LDAP_PEND_LOCK );
- 		LDAP_SEMA_FREE( ld, lp );
- 		NSLDAPI_FREE( lp );
- 		return ( rc );
- 	}
- }
- 
- int
  nsldapi_post_result( LDAP *ld, int msgid, LDAPMessage *result )
  {
  	LDAPPend	*lp;
--- 1260,1265 ----
***************
*** 1422,1427 ****
--- 1343,1349 ----
  	lp->lp_prev = NULL; 
  }
  
+ #if 0 /* these functions are no longer used */
  static void
  unlink_pend( LDAP *ld, LDAPPend *lp )
  {
***************
*** 1514,1545 ****
  	LDAP_MUTEX_UNLOCK( ld, LDAP_RESP_LOCK );
  	return ( rc );
  }
! 
! 
! static int nsldapi_mutex_trylock(LDAP *ld, LDAPLock i) 
! { 
!     if (ld->ld_mutex_trylock_fn == NULL) { 
!         return 0; 
!     } else { 
!         if (ld->ld_threadid_fn != NULL) { 
!             if (ld->ld_mutex_threadid[i] == ld->ld_threadid_fn()) { 
!                 /* This thread already owns the lock */ 
!                 ld->ld_mutex_refcnt[i]++; 
!                 return 0; 
!             } else { 
!                 if (ld->ld_mutex_trylock_fn(ld->ld_mutex[i]) == 0) { 
!                     /* This thread just acquired the lock */ 
!                     ld->ld_mutex_refcnt[i] = 1; 
!                     ld->ld_mutex_threadid[i] = ld->ld_threadid_fn(); 
!                     return 0; 
!                 } else { 
!                     /* A different thread owns the lock */ 
!                     return 1; 
!                 } 
!             } 
!         } else { 
!             return ld->ld_mutex_trylock_fn(ld->ld_mutex[i]); 
!         } 
!     } 
! }
! 
--- 1436,1439 ----
  	LDAP_MUTEX_UNLOCK( ld, LDAP_RESP_LOCK );
  	return ( rc );
  }
! #endif /* 0 */
