*** ldap-int.h	Tue Feb  8 11:06:59 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/ldap-int.h	Tue Jun  6 13:49:09 2000
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
  #ifndef _LDAPINT_H
  #define _LDAPINT_H
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
  #ifndef _LDAPINT_H
  #define _LDAPINT_H
***************
*** 27,32 ****
--- 23,29 ----
  #include <stdlib.h>
  #include <errno.h>
  #include <time.h>
+ #include <fcntl.h>
  #ifdef hpux
  #include <strings.h>
  #endif /* hpux */
***************
*** 55,60 ****
--- 52,58 ----
  #include <bstring.h>
  #endif /* IRIX */
  
+ #define NSLBERI_LBER_INT_FRIEND
  #ifdef macintosh
  #include "lber-int.h"
  #else /* macintosh */
***************
*** 84,90 ****
  #define NSLDAPI_HAVE_POLL	1
  #endif
  
! #define SSL_VERSION 0
  
  
  #define LDAP_URL_URLCOLON	"URL:"
--- 82,93 ----
  #define NSLDAPI_HAVE_POLL	1
  #endif
  
! /* SSL version, or 0 if not built with SSL */
! #if defined(NET_SSL)
! #  define SSL_VERSION 3
! #else
! #  define SSL_VERSION 0
! #endif
  
  
  #define LDAP_URL_URLCOLON	"URL:"
***************
*** 113,119 ****
      LDAP_OPTION_LOCK, 
      LDAP_ERR_LOCK, 
      LDAP_CONN_LOCK, 
!     LDAP_SELECT_LOCK,
      LDAP_RESULT_LOCK, 
      LDAP_PEND_LOCK, 
      LDAP_THREADID_LOCK, 
--- 116,122 ----
      LDAP_OPTION_LOCK, 
      LDAP_ERR_LOCK, 
      LDAP_CONN_LOCK, 
!     LDAP_IOSTATUS_LOCK,		/* serializes access to ld->ld_iostatus */
      LDAP_RESULT_LOCK, 
      LDAP_PEND_LOCK, 
      LDAP_THREADID_LOCK, 
***************
*** 206,211 ****
--- 209,220 ----
  } LDAPPend;
  
  /*
+  * forward declaration for I/O status structure (defined in os-ip.c)
+  */
+ typedef struct nsldapi_iostatus_info NSLDAPIIOStatus;
+ 
+ 
+ /*
   * structure representing an ldap connection
   */
  struct ldap {
***************
*** 252,273 ****
  	BERTranslateProc ld_lber_decode_translate_proc;
  	LDAPConn	*ld_defconn;	/* default connection */
  	LDAPConn	*ld_conns;	/* list of all server connections */
! 	void		*ld_selectinfo;	/* platform specifics for select */
! 	int		ld_selectreadcnt;  /* count of read sockets */
! 	int		ld_selectwritecnt; /* count of write sockets */
  	LDAP_REBINDPROC_CALLBACK *ld_rebind_fn;
  	void		*ld_rebind_arg;
  
! 	/* function pointers, etc. for io */
! 	struct ldap_io_fns	ld_io;
! #define ld_read_fn		ld_io.liof_read
! #define ld_write_fn		ld_io.liof_write
! #define ld_select_fn		ld_io.liof_select
! #define ld_socket_fn		ld_io.liof_socket
! #define ld_ioctl_fn		ld_io.liof_ioctl
! #define ld_connect_fn		ld_io.liof_connect
! #define ld_close_fn		ld_io.liof_close
! #define ld_ssl_enable_fn	ld_io.liof_ssl_enable
  
  	/* function pointers, etc. for DNS */
  	struct ldap_dns_fns	ld_dnsfn;
--- 261,285 ----
  	BERTranslateProc ld_lber_decode_translate_proc;
  	LDAPConn	*ld_defconn;	/* default connection */
  	LDAPConn	*ld_conns;	/* list of all server connections */
! 	NSLDAPIIOStatus	*ld_iostatus;	/* status info. about network sockets */
  	LDAP_REBINDPROC_CALLBACK *ld_rebind_fn;
  	void		*ld_rebind_arg;
  
! 	/* function pointers, etc. for extended I/O */
! 	struct ldap_x_ext_io_fns ld_ext_io_fns;
! #define ld_extio_size		ld_ext_io_fns.lextiof_size
! #define ld_extclose_fn		ld_ext_io_fns.lextiof_close
! #define ld_extconnect_fn	ld_ext_io_fns.lextiof_connect
! #define ld_extread_fn		ld_ext_io_fns.lextiof_read
! #define ld_extwrite_fn		ld_ext_io_fns.lextiof_write
! #define ld_extpoll_fn		ld_ext_io_fns.lextiof_poll
! #define ld_extnewhandle_fn	ld_ext_io_fns.lextiof_newhandle
! #define ld_extdisposehandle_fn	ld_ext_io_fns.lextiof_disposehandle
! #define ld_ext_session_arg	ld_ext_io_fns.lextiof_session_arg
! 
! 	/* allocated pointer for older I/O functions */
! 	struct ldap_io_fns	*ld_io_fns_ptr;
! #define NSLDAPI_USING_CLASSIC_IO_FUNCTIONS( ld ) ((ld)->ld_io_fns_ptr != NULL)
  
  	/* function pointers, etc. for DNS */
  	struct ldap_dns_fns	ld_dnsfn;
***************
*** 325,330 ****
--- 337,348 ----
  
  	/* extra thread function pointers */
  	struct ldap_extra_thread_fns	ld_thread2;
+ 
+ 	/* With the 4.0 version of the LDAP SDK */
+ 	/* the extra thread functions except for */
+ 	/* the ld_threadid_fn has been disabled */
+ 	/* Look at the release notes for the full */
+ 	/* explanation */
  #define ld_mutex_trylock_fn		ld_thread2.ltf_mutex_trylock
  #define ld_sema_alloc_fn		ld_thread2.ltf_sema_alloc
  #define ld_sema_free_fn			ld_thread2.ltf_sema_free
***************
*** 335,340 ****
--- 353,361 ----
  	/* extra data for mutex handling in referrals */
  	void 			*ld_mutex_threadid[LDAP_MAX_LOCK];
  	unsigned long		ld_mutex_refcnt[LDAP_MAX_LOCK];
+ 
+ 	/* connect timeout value */
+ 	int				ld_connect_timeout;
  };
  
  /* allocate/free mutex */
***************
*** 383,392 ****
--- 404,421 ----
  
  /* Backward compatibility locks */
  #define LDAP_MUTEX_BC_LOCK( ld, i ) \
+ 	/* the ld_mutex_trylock_fn is always set to NULL */ \
+ 	/* in setoption.c as the extra thread functions were */ \
+ 	/* turned off in the 4.0 SDK.  This check will  */ \
+ 	/* always be true */ \
  	if( (ld)->ld_mutex_trylock_fn == NULL ) { \
  		LDAP_MUTEX_LOCK( ld, i ) ; \
  	}
  #define LDAP_MUTEX_BC_UNLOCK( ld, i ) \
+ 	/* the ld_mutex_trylock_fn is always set to NULL */ \
+ 	/* in setoption.c as the extra thread functions were */ \
+ 	/* turned off in the 4.0 SDK.  This check will  */ \
+ 	/* always be true */ \
  	if( (ld)->ld_mutex_trylock_fn == NULL ) { \
  		LDAP_MUTEX_UNLOCK( ld, i ) ; \
  	}
***************
*** 409,414 ****
--- 438,447 ----
  		(ld)->ld_sema_post_fn( lp->lp_sema ); \
  	}
  #define POST( ld, y, z ) \
+ 	/* the ld_mutex_trylock_fn is always set to NULL */ \
+ 	/* in setoption.c as the extra thread functions were */ \
+ 	/* turned off in the 4.0 SDK.  This check will  */ \
+ 	/* always be false */ \
  	if( (ld)->ld_mutex_trylock_fn != NULL ) { \
  		nsldapi_post_result( ld, y, z ); \
  	}
***************
*** 577,584 ****
   * in open.c
   */
  void nsldapi_initialize_defaults( void );
- int nsldapi_open_ldap_connection( LDAP *ld, Sockbuf *sb, char *host,
- 	int defport, char **krbinstancep, int async, int secure );
  int nsldapi_open_ldap_defconn( LDAP *ld );
  void *nsldapi_malloc( size_t size );
  void *nsldapi_calloc( size_t nelem, size_t elsize );
--- 610,615 ----
***************
*** 589,606 ****
  /*
   * in os-ip.c
   */
! int nsldapi_connect_to_host( LDAP *ld, Sockbuf *sb, char *host,
! 	nsldapi_in_addr_t address, int port, int async, int secure );
  void nsldapi_close_connection( LDAP *ld, Sockbuf *sb );
  
! int nsldapi_do_ldap_select( LDAP *ld, struct timeval *timeout );
! void *nsldapi_new_select_info( void );
! void nsldapi_free_select_info( void *vsip );
! void nsldapi_mark_select_write( LDAP *ld, Sockbuf *sb );
! void nsldapi_mark_select_read( LDAP *ld, Sockbuf *sb );
! void nsldapi_mark_select_clear( LDAP *ld, Sockbuf *sb );
! int nsldapi_is_read_ready( LDAP *ld, Sockbuf *sb );
! int nsldapi_is_write_ready( LDAP *ld, Sockbuf *sb );
  
  /*
   * if referral.c
--- 620,638 ----
  /*
   * in os-ip.c
   */
! int nsldapi_connect_to_host( LDAP *ld, Sockbuf *sb, const char *host,
! 	int port, int secure, char **krbinstancep );
  void nsldapi_close_connection( LDAP *ld, Sockbuf *sb );
  
! int nsldapi_iostatus_poll( LDAP *ld, struct timeval *timeout );
! void nsldapi_iostatus_free( LDAP *ld );
! int nsldapi_iostatus_interest_write( LDAP *ld, Sockbuf *sb );
! int nsldapi_iostatus_interest_read( LDAP *ld, Sockbuf *sb );
! int nsldapi_iostatus_interest_clear( LDAP *ld, Sockbuf *sb );
! int nsldapi_iostatus_is_read_ready( LDAP *ld, Sockbuf *sb );
! int nsldapi_iostatus_is_write_ready( LDAP *ld, Sockbuf *sb );
! int nsldapi_install_lber_extiofns( LDAP *ld, Sockbuf *sb );
! int nsldapi_install_compat_io_fns( LDAP *ld, struct ldap_io_fns *iofns );
  
  /*
   * if referral.c
