*** portable.h	Fri Feb 11 16:48:13 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/include/portable.h	Tue Jun  6 13:49:08 2000
***************
*** 21,30 ****
   * edit this file.
   */
  
- #if defined(LINUX) || defined ( linux )
- #define LINUX2_1
- #endif
- 
  #ifndef SYSV
  #if defined( hpux ) || defined( sunos5 ) || defined ( sgi ) || defined( SVR4 )
  #define SYSV
--- 21,26 ----
***************
*** 124,129 ****
--- 120,135 ----
  
  
  /*
+  * Is snprintf() part of the standard C runtime library?
+  */
+ #if !defined(HAVE_SNPRINTF)
+ #if defined(SOLARIS) || defined(LINUX) || defined(HPUX)
+ #define HAVE_SNPRINTF
+ #endif
+ #endif
+ 
+ 
+ /*
   * Async IO.  Use a non blocking implementation of connect() and 
   * dns functions
   */
***************
*** 236,242 ****
  
  #if defined(_WINDOWS) || defined(macintosh) || defined(XP_OS2)
  #define GETHOSTBYNAME( n, r, b, l, e )  gethostbyname( n )
! #define CTIME( c, b, l )		ctime( c )
  #define STRTOK( s1, s2, l )		strtok( s1, s2 )
  #else /* UNIX */
  #if defined(sgi) || defined(HPUX9) || defined(LINUX1_2) || defined(SCOOS) || \
--- 242,248 ----
  
  #if defined(_WINDOWS) || defined(macintosh) || defined(XP_OS2)
  #define GETHOSTBYNAME( n, r, b, l, e )  gethostbyname( n )
! #define NSLDAPI_CTIME( c, b, l )	ctime( c )
  #define STRTOK( s1, s2, l )		strtok( s1, s2 )
  #else /* UNIX */
  #if defined(sgi) || defined(HPUX9) || defined(LINUX1_2) || defined(SCOOS) || \
***************
*** 269,284 ****
      defined(SCOOS) || defined(BSDI) || defined(NCR) || \
      defined(NEC) || ( defined(HPUX10) && !defined(_REENTRANT)) || \
      (defined(AIX) && !defined(USE_REENTRANT_LIBC))
! #define CTIME( c, b, l )		ctime( c )
  #elif defined(HPUX10) && defined(_REENTRANT) && !defined(HPUX11)
! #define CTIME( c, b, l )		nsldapi_compat_ctime_r( c, b, l )
  #elif defined( IRIX6_2 ) || defined( IRIX6_3 ) || defined(UNIXWARE) \
  	|| defined(OSF1V4) || defined(AIX) || defined(UnixWare) || defined(hpux) || defined(HPUX11)
! #define CTIME( c, b, l )                ctime_r( c, b )
  #elif defined( OSF1V3 )
! #define CTIME( c, b, l )		(ctime_r( c, b, l ) ? NULL : b)
  #else
! #define CTIME( c, b, l )		ctime_r( c, b, l )
  #endif
  #if defined(hpux9) || defined(LINUX1_2) || defined(SUNOS4) || defined(SNI) || \
      defined(SCOOS) || defined(BSDI) || defined(NCR) || \
--- 275,290 ----
      defined(SCOOS) || defined(BSDI) || defined(NCR) || \
      defined(NEC) || ( defined(HPUX10) && !defined(_REENTRANT)) || \
      (defined(AIX) && !defined(USE_REENTRANT_LIBC))
! #define NSLDAPI_CTIME( c, b, l )	ctime( c )
  #elif defined(HPUX10) && defined(_REENTRANT) && !defined(HPUX11)
! #define NSLDAPI_CTIME( c, b, l )	nsldapi_compat_ctime_r( c, b, l )
  #elif defined( IRIX6_2 ) || defined( IRIX6_3 ) || defined(UNIXWARE) \
  	|| defined(OSF1V4) || defined(AIX) || defined(UnixWare) || defined(hpux) || defined(HPUX11)
! #define NSLDAPI_CTIME( c, b, l )        ctime_r( c, b )
  #elif defined( OSF1V3 )
! #define NSLDAPI_CTIME( c, b, l )	(ctime_r( c, b, l ) ? NULL : b)
  #else
! #define NSLDAPI_CTIME( c, b, l )	ctime_r( c, b, l )
  #endif
  #if defined(hpux9) || defined(LINUX1_2) || defined(SUNOS4) || defined(SNI) || \
      defined(SCOOS) || defined(BSDI) || defined(NCR) || \
