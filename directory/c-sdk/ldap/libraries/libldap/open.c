*** open.c	Tue Feb  8 11:06:59 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/open.c	Tue Jun  6 13:49:09 2000
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
*** 81,87 ****
  	memset( &nsldapi_memalloc_fns, 0, sizeof( nsldapi_memalloc_fns ));
  	memset( &nsldapi_ld_defaults, 0, sizeof( nsldapi_ld_defaults ));
  	nsldapi_ld_defaults.ld_options = LDAP_BITOPT_REFERRALS;
! 	nsldapi_ld_defaults.ld_version = LDAP_VERSION;
  	nsldapi_ld_defaults.ld_lberoptions = LBER_OPT_USE_DER;
  	nsldapi_ld_defaults.ld_refhoplimit = LDAP_DEFAULT_REFHOPLIMIT;
  
--- 77,83 ----
  	memset( &nsldapi_memalloc_fns, 0, sizeof( nsldapi_memalloc_fns ));
  	memset( &nsldapi_ld_defaults, 0, sizeof( nsldapi_ld_defaults ));
  	nsldapi_ld_defaults.ld_options = LDAP_BITOPT_REFERRALS;
! 	nsldapi_ld_defaults.ld_version = LDAP_VERSION2;
  	nsldapi_ld_defaults.ld_lberoptions = LBER_OPT_USE_DER;
  	nsldapi_ld_defaults.ld_refhoplimit = LDAP_DEFAULT_REFHOPLIMIT;
  
***************
*** 92,97 ****
--- 88,97 ----
  	    ldap_t61_to_8859 );
  #endif /* LDAP_CHARSET_8859 == LDAP_DEFAULT_CHARSET */
  #endif /* STR_TRANSLATION && LDAP_DEFAULT_CHARSET */
+ 
+         /* set default connect timeout (in milliseconds) */
+         /* this was picked as it is the standard tcp timeout as well */
+         nsldapi_ld_defaults.ld_connect_timeout = LDAP_X_IO_TIMEOUT_NO_TIMEOUT;
  }
  
  
***************
*** 214,245 ****
  
  	/* copy defaults */
  	SAFEMEMCPY( ld, &nsldapi_ld_defaults, sizeof( struct ldap ));
  
! 	if (( ld->ld_selectinfo = nsldapi_new_select_info()) == NULL ||
! 	    ( ld->ld_sbp = ber_sockbuf_alloc()) == NULL ||
  	    ( defhost != NULL &&
  	    ( ld->ld_defhost = nsldapi_strdup( defhost )) == NULL ) ||
  	    ((ld->ld_mutex = (void **) NSLDAPI_CALLOC( LDAP_MAX_LOCK, sizeof(void *))) == NULL )) {
  		if ( ld->ld_sbp != NULL ) {
  			ber_sockbuf_free( ld->ld_sbp );
- 		}
- 		if ( ld->ld_selectinfo != NULL ) {
- 			nsldapi_free_select_info( ld->ld_selectinfo );
  		}
! 		if( ld->ld_mutex != NULL )
  			NSLDAPI_FREE( ld->ld_mutex );
  		NSLDAPI_FREE( (char*)ld );
  		return( NULL );
  	}
  
  	/* install Sockbuf I/O functions if set in LDAP * */
! 	if ( ld->ld_read_fn != NULL ) {
! 		ber_sockbuf_set_option( ld->ld_sbp, LBER_SOCKBUF_OPT_READ_FN,
! 			(void *)ld->ld_read_fn );
! 	}
! 	if ( ld->ld_write_fn != NULL ) {
! 		ber_sockbuf_set_option( ld->ld_sbp, LBER_SOCKBUF_OPT_WRITE_FN,
! 			(void *)ld->ld_write_fn );
  	}
  
  	/* allocate mutexes */
--- 214,267 ----
  
  	/* copy defaults */
  	SAFEMEMCPY( ld, &nsldapi_ld_defaults, sizeof( struct ldap ));
+ 	if ( nsldapi_ld_defaults.ld_io_fns_ptr != NULL ) {
+ 		if (( ld->ld_io_fns_ptr = (struct ldap_io_fns *)NSLDAPI_MALLOC(
+ 		    sizeof( struct ldap_io_fns ))) == NULL ) {
+ 			NSLDAPI_FREE( (char *)ld );
+ 			return( NULL );
+ 		}
+ 		/* struct copy */
+ 		*(ld->ld_io_fns_ptr) = *(nsldapi_ld_defaults.ld_io_fns_ptr);
+ 	}
+ 
+ 	/* call the new handle I/O callback if one is defined */
+ 	if ( ld->ld_extnewhandle_fn != NULL ) {
+ 		/*
+ 		 * We always pass the session extended I/O argument to
+ 		 * the new handle callback.
+ 		 */
+ 		if ( ld->ld_extnewhandle_fn( ld, ld->ld_ext_session_arg )
+ 		    != LDAP_SUCCESS ) {
+ 			NSLDAPI_FREE( (char*)ld );
+ 			return( NULL );
+ 		}
+ 	}
  
! 	/* allocate session-specific resources */
! 	if (( ld->ld_sbp = ber_sockbuf_alloc()) == NULL ||
  	    ( defhost != NULL &&
  	    ( ld->ld_defhost = nsldapi_strdup( defhost )) == NULL ) ||
  	    ((ld->ld_mutex = (void **) NSLDAPI_CALLOC( LDAP_MAX_LOCK, sizeof(void *))) == NULL )) {
  		if ( ld->ld_sbp != NULL ) {
  			ber_sockbuf_free( ld->ld_sbp );
  		}
! 		if( ld->ld_mutex != NULL ) {
  			NSLDAPI_FREE( ld->ld_mutex );
+ 		}
  		NSLDAPI_FREE( (char*)ld );
  		return( NULL );
  	}
  
  	/* install Sockbuf I/O functions if set in LDAP * */
! 	if ( ld->ld_extread_fn != NULL || ld->ld_extwrite_fn != NULL ) {
! 		struct lber_x_ext_io_fns lberiofns;
! 
! 		lberiofns.lbextiofn_size = LBER_X_EXTIO_FNS_SIZE;
! 		lberiofns.lbextiofn_read = ld->ld_extread_fn;
! 		lberiofns.lbextiofn_write = ld->ld_extwrite_fn;
! 		lberiofns.lbextiofn_socket_arg = NULL;
! 		ber_sockbuf_set_option( ld->ld_sbp, LBER_SOCKBUF_OPT_EXT_IO_FNS,
! 			(void *)&lberiofns );
  	}
  
  	/* allocate mutexes */
***************
*** 256,329 ****
  }
  
  
- int
- nsldapi_open_ldap_connection( LDAP *ld, Sockbuf *sb, char *host, int defport,
- 	char **krbinstancep, int async, int secure )
- {
- 	int 			rc = 0, port;
- 	char			*p, *q, *r;
- 	char			*curhost, hostname[ 2*MAXHOSTNAMELEN ];
- 
- 	LDAPDebug( LDAP_DEBUG_TRACE, "nsldapi_open_ldap_connection\n", 0, 0,
- 	    0 );
- 
- 	defport = htons( (unsigned short)defport );
- 
- 	if ( host != NULL && *host != '\0' ) {
- 		for ( p = host; p != NULL && *p != '\0'; p = q ) {
- 			if (( q = strchr( p, ' ' )) != NULL ) {
- 				strncpy( hostname, p, q - p );
- 				hostname[ q - p ] = '\0';
- 				curhost = hostname;
- 				while ( *q == ' ' ) {
- 				    ++q;
- 				}
- 			} else {
- 				curhost = p;	/* avoid copy if possible */
- 				q = NULL;
- 			}
- 
- 			if (( r = strchr( curhost, ':' )) != NULL ) {
- 			    if ( curhost != hostname ) {
- 				strcpy( hostname, curhost );	/* now copy */
- 				r = hostname + ( r - curhost );
- 				curhost = hostname;
- 			    }
- 			    *r++ = '\0';
- 			    port = htons( (short)atoi( r ));
- 			} else {
- 			    port = defport;   
- 			}
- 
- 			if (( rc = nsldapi_connect_to_host( ld, sb, curhost,
- 			    0, port, async, secure )) != -1 ) {
- 				break;
- 			}
- 		}
- 	} else {
- 		rc = nsldapi_connect_to_host( ld, sb, NULL, htonl( INADDR_LOOPBACK ),
- 		    defport, async, secure );
- 	}
- 
- 	if ( rc == -1 ) {
- 		return( rc );
- 	}
- 
- 	if ( krbinstancep != NULL ) {
- #ifdef KERBEROS
- 		if (( *krbinstancep = nsldapi_host_connected_to( sb )) != NULL &&
- 		    ( p = strchr( *krbinstancep, '.' )) != NULL ) {
- 			*p = '\0';
- 		}
- #else /* KERBEROS */
- 		krbinstancep = NULL;
- #endif /* KERBEROS */
- 	}
- 
- 	return( rc );
- }
- 
- 
  /* returns 0 if connection opened and -1 if an error occurs */
  int
  nsldapi_open_ldap_defconn( LDAP *ld )
--- 278,283 ----
***************
*** 356,361 ****
--- 310,423 ----
  
  	return( 0 );
  }
+ 
+ 
+ struct ldap_x_hostlist_status {
+ 	char	*lhs_hostlist;
+ 	char	*lhs_nexthost;
+ 	int	lhs_defport;
+ };
+ 
+ /*
+  * Return the first host and port in hostlist (setting *hostp and *portp).
+  * Return value is an LDAP API error code (LDAP_SUCCESS if all goes well).
+  * Note that a NULL or zero-length hostlist causes the host "127.0.0.1" to
+  * be returned.
+  */
+ int LDAP_CALL
+ ldap_x_hostlist_first( const char *hostlist, int defport, char **hostp,
+     int *portp, struct ldap_x_hostlist_status **statusp )
+ {
+ 
+ 	if ( NULL == hostp || NULL == portp || NULL == statusp ) {
+ 		return( LDAP_PARAM_ERROR );
+ 	}
+ 
+ 	if ( NULL == hostlist || *hostlist == '\0' ) {
+ 		*hostp = nsldapi_strdup( "127.0.0.1" );
+ 		if ( NULL == *hostp ) {
+ 			return( LDAP_NO_MEMORY );
+ 		}
+ 		*portp = defport;
+ 		*statusp = NULL;
+ 		return( LDAP_SUCCESS );
+ 	}
+ 
+ 	*statusp = NSLDAPI_CALLOC( 1, sizeof( struct ldap_x_hostlist_status ));
+ 	if ( NULL == *statusp ) {
+ 		return( LDAP_NO_MEMORY );
+ 	}
+ 	(*statusp)->lhs_hostlist = nsldapi_strdup( hostlist );
+ 	if ( NULL == (*statusp)->lhs_hostlist ) {
+ 		return( LDAP_NO_MEMORY );
+ 	}
+ 	(*statusp)->lhs_nexthost = (*statusp)->lhs_hostlist;
+ 	(*statusp)->lhs_defport = defport;
+ 	return( ldap_x_hostlist_next( hostp, portp, *statusp ));
+ }
+ 
+ /*
+  * Return the next host and port in hostlist (setting *hostp and *portp).
+  * Return value is an LDAP API error code (LDAP_SUCCESS if all goes well).
+  * If no more hosts are available, LDAP_SUCCESS is returned but *hostp is set
+  * to NULL.
+  */
+ int LDAP_CALL
+ ldap_x_hostlist_next( char **hostp, int *portp,
+ 	struct ldap_x_hostlist_status *status )
+ {
+ 	char	*q;
+ 
+ 	if ( NULL == hostp || NULL == portp || NULL == status ) {
+ 		return( LDAP_PARAM_ERROR );
+ 	}
+ 
+ 	if ( NULL == status->lhs_nexthost ) {
+ 		*hostp = NULL;
+ 		return( LDAP_SUCCESS );
+ 	}
+ 
+ 	/* copy host into *hostp */
+ 	if ( NULL != ( q = strchr( status->lhs_nexthost, ' ' ))) {
+ 		size_t	len = q - status->lhs_nexthost;
+ 		*hostp = NSLDAPI_MALLOC( len + 1 );
+ 		if ( NULL == *hostp ) {
+ 			return( LDAP_NO_MEMORY );
+ 		}
+ 		strncpy( *hostp, status->lhs_nexthost, len );
+ 		(*hostp)[len] = '\0';
+ 		status->lhs_nexthost += ( len + 1 );
+ 	} else {	/* last host */
+ 		*hostp = nsldapi_strdup( status->lhs_nexthost );
+ 		if ( NULL == *hostp ) {
+ 			return( LDAP_NO_MEMORY );
+ 		}
+ 		status->lhs_nexthost = NULL;
+ 	}
+ 
+ 	/* determine and set port */
+ 	if ( NULL != ( q = strchr( *hostp, ':' ))) {
+ 		*q++ = '\0';
+ 		*portp = atoi( q );
+ 	} else {
+ 		*portp = status->lhs_defport;
+ 	}
+ 
+ 	return( LDAP_SUCCESS );
+ }
+ 
+ 
+ void LDAP_CALL
+ ldap_x_hostlist_statusfree( struct ldap_x_hostlist_status *status )
+ {
+ 	if ( NULL != status ) {
+ 		if ( NULL != status->lhs_hostlist ) {
+ 			NSLDAPI_FREE( status->lhs_hostlist );
+ 		}
+ 		NSLDAPI_FREE( status );
+ 	}
+ }
+ 
  
  
  /*
