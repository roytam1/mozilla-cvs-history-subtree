*** ldap.h	Wed Aug 16 10:03:32 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/include/ldap.h	Tue Jun  6 13:49:07 2000
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
  /* ldap.h - general header file for libldap */
  #ifndef _LDAP_H
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
  /* ldap.h - general header file for libldap */
  #ifndef _LDAP_H
***************
*** 39,45 ****
  #include <utime.h>
  #endif
  #ifndef LDAP_TYPE_SOCKET_DEFINED	/* API extension */
! #include "macsocket.h"
  #endif
  #else /* everything else, e.g., Unix */
  #ifndef LDAP_TYPE_TIMEVAL_DEFINED
--- 35,41 ----
  #include <utime.h>
  #endif
  #ifndef LDAP_TYPE_SOCKET_DEFINED	/* API extension */
! #include "macsock.h"
  #endif
  #else /* everything else, e.g., Unix */
  #ifndef LDAP_TYPE_TIMEVAL_DEFINED
***************
*** 67,73 ****
  #define LDAP_VERSION_MIN	LDAP_VERSION1
  #define LDAP_VERSION_MAX	LDAP_VERSION3
  
! #define LDAP_VENDOR_VERSION	400	/* 4.0 */
  #define LDAP_VENDOR_NAME	"Netscape Communications Corp."
  /*
   * The following will be an RFC number once the LDAP C API Internet Draft
--- 63,69 ----
  #define LDAP_VERSION_MIN	LDAP_VERSION1
  #define LDAP_VERSION_MAX	LDAP_VERSION3
  
! #define LDAP_VENDOR_VERSION	410	/* version # * 100 */
  #define LDAP_VENDOR_NAME	"Netscape Communications Corp."
  /*
   * The following will be an RFC number once the LDAP C API Internet Draft
***************
*** 92,97 ****
--- 88,94 ----
  #define LDAP_API_FEATURE_X_LDERRNO		1
  #define LDAP_API_FEATURE_X_MEMCACHE		1
  #define LDAP_API_FEATURE_X_IO_FUNCTIONS		1
+ #define LDAP_API_FEATURE_X_EXTIO_FUNCTIONS	1
  #define LDAP_API_FEATURE_X_DNS_FUNCTIONS	1
  #define LDAP_API_FEATURE_X_MEMALLOC_FUNCTIONS	1
  #define LDAP_API_FEATURE_X_THREAD_FUNCTIONS	1
***************
*** 211,217 ****
  #define LDAP_RES_COMPARE                0x6fL	/* 111 */
  #define LDAP_RES_SEARCH_REFERENCE       0x73L	/* 115 */
  #define LDAP_RES_EXTENDED               0x78L	/* 120 */
! #define LDAP_RES_ANY                    (-1L)
  #define LDAP_RES_UNSOLICITED		0
  
  /* built-in SASL methods */
--- 208,216 ----
  #define LDAP_RES_COMPARE                0x6fL	/* 111 */
  #define LDAP_RES_SEARCH_REFERENCE       0x73L	/* 115 */
  #define LDAP_RES_EXTENDED               0x78L	/* 120 */
! 
! /* Special values for ldap_result() "msgid" parameter */
! #define LDAP_RES_ANY                (-1)
  #define LDAP_RES_UNSOLICITED		0
  
  /* built-in SASL methods */
***************
*** 324,333 ****
--- 323,338 ----
  #define LDAP_CONTROL_VLVREQUEST    	"2.16.840.1.113730.3.4.9"
  #define LDAP_CONTROL_VLVRESPONSE	"2.16.840.1.113730.3.4.10"
  #define LDAP_CONTROL_PROXYAUTH		"2.16.840.1.113730.3.4.12"
+ 
+ /* Authentication request and response controls */
+ #define LDAP_CONTROL_AUTH_REQUEST	"2.16.840.1.113730.3.4.16"
+ #define LDAP_CONTROL_AUTH_RESPONSE	"2.16.840.1.113730.3.4.15"
+ 
  /* Password information sent back to client */
  #define LDAP_CONTROL_PWEXPIRED		"2.16.840.1.113730.3.4.4"
  #define LDAP_CONTROL_PWEXPIRING		"2.16.840.1.113730.3.4.5"
  
+ 
  /*
   * Client controls we know about
   */
***************
*** 699,713 ****
   * Thread callback functions:
   */
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_ALLOC_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_FREE_CALLBACK)( void * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_LOCK_CALLBACK)( void * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_UNLOCK_CALLBACK)( void * );
  typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_ERRNO_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SET_ERRNO_CALLBACK)( int  );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_LDERRNO_CALLBACK)( char **, 
! 	char **, void * );
! typedef void    (LDAP_C LDAP_CALLBACK LDAP_TF_SET_LDERRNO_CALLBACK)( int, 
! 	char *, char *, void * );
  
  /*
   * Structure to hold thread function pointers:
--- 704,718 ----
   * Thread callback functions:
   */
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_ALLOC_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_FREE_CALLBACK)( void *m );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_LOCK_CALLBACK)( void *m );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_UNLOCK_CALLBACK)( void *m );
  typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_ERRNO_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SET_ERRNO_CALLBACK)( int e );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_LDERRNO_CALLBACK)(
! 	char **matchedp, char **errmsgp, void *arg );
! typedef void    (LDAP_C LDAP_CALLBACK LDAP_TF_SET_LDERRNO_CALLBACK)( int err, 
! 	char *matched, char *errmsg, void *arg );
  
  /*
   * Structure to hold thread function pointers:
***************
*** 726,733 ****
--- 731,868 ----
  
  
  /*
+  * Extended I/O function callbacks option (an API extension --
+  * LDAP_API_FEATURE_X_EXTIO_FUNCTIONS).
+  */
+ #define LDAP_X_OPT_EXTIO_FN_PTRS   (LDAP_OPT_PRIVATE_EXTENSION_BASE + 0x0F00)		/* 0x4000 + 0x0F00 = 0x4F00 = 20224 - API extension */
+ 
+ /*
+  * These extended I/O function callbacks echo the BSD socket API but accept
+  * an extra pointer parameter at the end of their argument list that can
+  * be used by client applications for their own needs.  For some of the calls,
+  * the pointer is a session argument of type struct lextiof_session_private *
+  * that is associated with the LDAP session handle (LDAP *).  For others, the
+  * pointer is a socket specific struct lextiof_socket_private * argument that
+  * is associated with a particular socket (a TCP connection).
+  *
+  * The lextiof_session_private and lextiof_socket_private structures are not
+  * defined by the LDAP C API; users of this extended I/O interface should
+  * define these themselves.
+  *
+  * The combination of the integer socket number (i.e., lpoll_fd, which is
+  * the value returned by the CONNECT callback) and the application specific
+  * socket argument (i.e., lpoll_socketarg, which is the value set in *sockargpp
+  * by the CONNECT callback) must be unique.
+  *
+  * The types for the extended READ and WRITE callbacks are actually in lber.h.
+  *
+  * The CONNECT callback gets passed both the session argument (sessionarg)
+  * and a pointer to a socket argument (socketargp) so it has the
+  * opportunity to set the socket-specific argument.  The CONNECT callback
+  * also takes a timeout parameter whose value can be set by calling
+  * ldap_set_option( ld, LDAP_X_OPT_..., &val ).  The units used for the
+  * timeout parameter are milliseconds.
+  *
+  * A POLL interface is provided instead of a select() one.  The timeout is
+  * in milliseconds.
+ 
+  * A NEWHANDLE callback function is also provided.  It is called right
+  * after the LDAP session handle is created, e.g., during ldap_init().
+  * If the NEWHANDLE callback returns anything other than LDAP_SUCCESS,
+  * the session handle allocation fails.
+  *
+  * A DISPOSEHANDLE callback function is also provided.  It is called right
+  * before the LDAP session handle and its contents are destroyed, e.g.,
+  * during ldap_unbind().
+  */
+ 
+ /*
+  * Special timeout values for poll and connect:
+  */
+ #define LDAP_X_IO_TIMEOUT_NO_WAIT	0	/* return immediately */
+ #define LDAP_X_IO_TIMEOUT_NO_TIMEOUT	(-1)	/* block indefinitely */
+ 
+ /* LDAP poll()-like descriptor:
+  */
+ typedef struct ldap_x_pollfd {	   /* used by LDAP_X_EXTIOF_POLL_CALLBACK */
+     int		lpoll_fd;	   /* integer file descriptor / socket */
+     struct lextiof_socket_private
+ 		*lpoll_socketarg;
+ 				   /* pointer socket and for use by */
+ 				   /* application */
+     short	lpoll_events;      /* requested event */
+     short	lpoll_revents;     /* returned event */
+ } LDAP_X_PollFD;
+ 
+ /* Event flags for lpoll_events and lpoll_revents:
+  */
+ #define LDAP_X_POLLIN    0x01  /* regular data ready for reading */
+ #define LDAP_X_POLLPRI   0x02  /* high priority data available */
+ #define LDAP_X_POLLOUT   0x04  /* ready for writing */
+ #define LDAP_X_POLLERR   0x08  /* error occurred -- only in lpoll_revents */
+ #define LDAP_X_POLLHUP   0x10  /* connection closed -- only in lpoll_revents */
+ #define LDAP_X_POLLNVAL  0x20  /* invalid lpoll_fd -- only in lpoll_revents */
+ 
+ /* Options passed to LDAP_X_EXTIOF_CONNECT_CALLBACK to modify socket behavior:
+  */
+ #define LDAP_X_EXTIOF_OPT_NONBLOCKING	0x01  /* turn on non-blocking mode */
+ #define LDAP_X_EXTIOF_OPT_SECURE	0x02  /* turn on 'secure' mode */
+ 
+ 
+ /* extended I/O callback function prototypes:
+  */
+ typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_CONNECT_CALLBACK )(
+ 	    const char *hostlist, int port, /* host byte order */
+ 	    int timeout /* milliseconds */,
+ 	    unsigned long options, /* bitmapped options */
+ 	    struct lextiof_session_private *sessionarg,
+ 	    struct lextiof_socket_private **socketargp );
+ typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_CLOSE_CALLBACK )(
+ 	    int s, struct lextiof_socket_private *socketarg );
+ typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_POLL_CALLBACK)(
+ 	    LDAP_X_PollFD fds[], int nfds, int timeout /* milliseconds */,
+ 	    struct lextiof_session_private *sessionarg );
+ typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_NEWHANDLE_CALLBACK)(
+ 	    LDAP *ld, struct lextiof_session_private *sessionarg );
+ typedef void	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_DISPOSEHANDLE_CALLBACK)(
+ 	    LDAP *ld, struct lextiof_session_private *sessionarg );
+ 
+ 
+ /* Structure to hold extended I/O function pointers:
+  */
+ struct ldap_x_ext_io_fns {
+ 	/* lextiof_size should always be set to LDAP_X_EXTIO_FNS_SIZE */
+ 	int					lextiof_size;
+ 	LDAP_X_EXTIOF_CONNECT_CALLBACK		*lextiof_connect;
+ 	LDAP_X_EXTIOF_CLOSE_CALLBACK		*lextiof_close;
+ 	LDAP_X_EXTIOF_READ_CALLBACK		*lextiof_read;
+ 	LDAP_X_EXTIOF_WRITE_CALLBACK		*lextiof_write;
+ 	LDAP_X_EXTIOF_POLL_CALLBACK		*lextiof_poll;
+ 	LDAP_X_EXTIOF_NEWHANDLE_CALLBACK	*lextiof_newhandle;
+ 	LDAP_X_EXTIOF_DISPOSEHANDLE_CALLBACK	*lextiof_disposehandle;
+ 	void					*lextiof_session_arg;
+ };
+ #define LDAP_X_EXTIO_FNS_SIZE	sizeof(struct ldap_x_ext_io_fns)
+ 
+ 
+ /*
+  * Utility functions for parsing space-separated host lists (useful for
+  * implementing an extended I/O CONNECT callback function).
+  */
+ struct ldap_x_hostlist_status;
+ LDAP_API(int) LDAP_CALL ldap_x_hostlist_first( const char *hostlist,
+ 	int defport, char **hostp, int *portp /* host byte order */,
+ 	struct ldap_x_hostlist_status **statusp );
+ LDAP_API(int) LDAP_CALL ldap_x_hostlist_next( char **hostp,
+ 	int *portp /* host byte order */, struct ldap_x_hostlist_status *status );
+ LDAP_API(void) LDAP_CALL ldap_x_hostlist_statusfree(
+ 	struct ldap_x_hostlist_status *status );
+ 
+ 
+ /*
   * I/O function callbacks option (an API extension --
   * LDAP_API_FEATURE_X_IO_FUNCTIONS).
+  * Use of the extended I/O functions instead is recommended; see above.
   */
  #define LDAP_OPT_IO_FN_PTRS		0x0B	/* 11 - API extension */
  
***************
*** 735,750 ****
   * I/O callback functions (note that types for the read and write callbacks
   * are actually in lber.h):
   */
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SELECT_CALLBACK)( int, fd_set *, 
! 	fd_set *, fd_set *, struct timeval * );
! typedef LBER_SOCKET (LDAP_C LDAP_CALLBACK LDAP_IOF_SOCKET_CALLBACK)( int, 
! 	int, int );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_IOCTL_CALLBACK)( LBER_SOCKET, 
! 	int, ... );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CONNECT_CALLBACK )( LBER_SOCKET, 
! 	struct sockaddr *, int );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CLOSE_CALLBACK )( LBER_SOCKET );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SSL_ENABLE_CALLBACK )( LBER_SOCKET );
  
  /*
   * Structure to hold I/O function pointers:
--- 870,888 ----
   * I/O callback functions (note that types for the read and write callbacks
   * are actually in lber.h):
   */
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SELECT_CALLBACK)( int nfds,
! 	fd_set *readfds, fd_set *writefds, fd_set *errorfds,
! 	struct timeval *timeout );
! typedef LBER_SOCKET (LDAP_C LDAP_CALLBACK LDAP_IOF_SOCKET_CALLBACK)(
! 	int domain, int type, int protocol );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_IOCTL_CALLBACK)( LBER_SOCKET s, 
! 	int option, ... );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CONNECT_CALLBACK )(
! 	LBER_SOCKET s, struct sockaddr *name, int namelen );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CLOSE_CALLBACK )(
! 	LBER_SOCKET s );
! typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SSL_ENABLE_CALLBACK )(
! 	LBER_SOCKET s );
  
  /*
   * Structure to hold I/O function pointers:
***************
*** 776,784 ****
  typedef void (LDAP_C LDAP_CALLBACK
  	LDAP_KEYFREE_CALLBACK)( void *arg, const struct berval* );
  typedef int (LDAP_C LDAP_CALLBACK
! 	LDAP_CMP_CALLBACK)(const char*, const char*);
  typedef int (LDAP_C LDAP_CALLBACK
! 	LDAP_VALCMP_CALLBACK)(const char**, const char**);
  
  /*
   * Client side sorting functions:
--- 914,922 ----
  typedef void (LDAP_C LDAP_CALLBACK
  	LDAP_KEYFREE_CALLBACK)( void *arg, const struct berval* );
  typedef int (LDAP_C LDAP_CALLBACK
! 	LDAP_CMP_CALLBACK)(const char *val1, const char *val2);
  typedef int (LDAP_C LDAP_CALLBACK
! 	LDAP_VALCMP_CALLBACK)(const char **val1p, const char **val2p);
  
  /*
   * Client side sorting functions:
***************
*** 890,895 ****
--- 1028,1039 ----
  };
  
  
+ /*
+  * Timeout value for nonblocking connect call
+  */
+ #define	LDAP_X_OPT_CONNECT_TIMEOUT    (LDAP_OPT_PRIVATE_EXTENSION_BASE + 0x0F01)
+         /* 0x4000 + 0x0F01 = 0x4F01 = 20225 - API extension */
+ 
  /********* the functions in the following section are experimental ***********/
  
  /*
***************
*** 923,933 ****
   */
  #define LDAP_OPT_EXTRA_THREAD_FN_PTRS  0x65	/* 101 - API extension */
  
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_TRYLOCK_CALLBACK)( void * );
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_ALLOC_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_FREE_CALLBACK)( void * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_WAIT_CALLBACK)( void * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_POST_CALLBACK)( void * );
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_THREADID_CALLBACK)(void);
  
  struct ldap_extra_thread_fns {
--- 1067,1077 ----
   */
  #define LDAP_OPT_EXTRA_THREAD_FN_PTRS  0x65	/* 101 - API extension */
  
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_TRYLOCK_CALLBACK)( void *m );
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_ALLOC_CALLBACK)( void );
! typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_FREE_CALLBACK)( void *s );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_WAIT_CALLBACK)( void *s );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_POST_CALLBACK)( void *s );
  typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_THREADID_CALLBACK)(void);
  
  struct ldap_extra_thread_fns {
***************
*** 949,972 ****
   */
  #define LDAP_OPT_ASYNC_CONNECT          0x63	/* 99 - API extension */
  
- #define LDAP_OPT_ASYNC_RECONNECT_FN_PTR 0x64	/* 100 - API extension */
- /* 
-  * this function sets the connect status of the ld so that a client 
-  * can do dns and connect, and then tell the sdk to ignore the connect phase 
-  */ 
- LDAP_API(int) LDAP_CALL ldap_set_connected( LDAP *ld, const int currentstatus );   
- /* 
-  * callback definition for reconnect request from a referral 
-  */ 
- typedef int( LDAP_C LDAP_CALLBACK LDAP_ASYNC_RECONNECT)( LBER_SOCKET, 
- 	struct sockaddr *, int ); 
- 
- struct ldap_async_connect_fns 
- { 
-     LDAP_ASYNC_RECONNECT *lac_reconnect; 
- }; 
- 
- 
  /*
   * Debugging level (an API extension)
   */
--- 1093,1098 ----
***************
*** 978,984 ****
  extern int		*module_ldap_debug;
  typedef void (*set_debug_level_fn_t)(int*);
  #endif
- 
   
  /************************ end of experimental section ************************/
  
--- 1104,1109 ----
***************
*** 1122,1148 ****
  #define LDAP_CACHE_POPULATE             1
  #define LDAP_CACHE_LOCALDB              2
  
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_BIND_CALLBACK)( LDAP *, int,
! 	unsigned long, const char *, const struct berval *, int );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_UNBIND_CALLBACK)( LDAP *, int,
! 	unsigned long );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_SEARCH_CALLBACK)( LDAP *,
! 	int, unsigned long, const char *, int, const char LDAP_CALLBACK *,
! 	char **, int );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_COMPARE_CALLBACK)( LDAP *, int,
! 	unsigned long, const char *, const char *, const struct berval * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_ADD_CALLBACK)( LDAP *, int,
! 	unsigned long, const char *, LDAPMod ** );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_DELETE_CALLBACK)( LDAP *, int,
! 	unsigned long, const char * );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODIFY_CALLBACK)( LDAP *, int,
! 	unsigned long, const char *, LDAPMod ** );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODRDN_CALLBACK)( LDAP *, int,
! 	unsigned long, const char *, const char *, int );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_RESULT_CALLBACK)( LDAP *, int,
! 	int, struct timeval *, LDAPMessage ** );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_FLUSH_CALLBACK)( LDAP *,
! 	const char *, const char * );
  
  struct ldap_cache_fns {
  	void    *lcf_private;
--- 1247,1276 ----
  #define LDAP_CACHE_POPULATE             1
  #define LDAP_CACHE_LOCALDB              2
  
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_BIND_CALLBACK)( LDAP *ld, int msgid,
! 	unsigned long tag, const char *dn, const struct berval *creds,
! 	int method);
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_UNBIND_CALLBACK)( LDAP *ld,
! 	int unused0, unsigned long unused1 );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_SEARCH_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *base, int scope,
! 	const char LDAP_CALLBACK *filter, char **attrs, int attrsonly );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_COMPARE_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *dn, const char *attr,
! 	const struct berval *value );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_ADD_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *dn, LDAPMod **attrs );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_DELETE_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *dn );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODIFY_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *dn, LDAPMod **mods );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODRDN_CALLBACK)( LDAP *ld,
! 	int msgid, unsigned long tag, const char *dn, const char *newrdn,
! 	int deleteoldrdn );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_RESULT_CALLBACK)( LDAP *ld,
! 	int msgid, int all, struct timeval *timeout, LDAPMessage **result );
! typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_FLUSH_CALLBACK)( LDAP *ld,
! 	const char *dn, const char *filter );
  
  struct ldap_cache_fns {
  	void    *lcf_private;
