*** tmplout.c	Tue Feb  8 11:07:01 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/tmplout.c	Tue May 16 15:28:58 2000
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
   * tmplout.c:  display template library output routines for LDAP clients
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
   * tmplout.c:  display template library output routines for LDAP clients
***************
*** 32,54 ****
  #endif
  
  
- /* HCL 1.56 caused some wierdness, because ttypdefaults.h on SSF defines CTIME 0
-  * This little bit fixes that 
-  */
- #if  defined(OSF1)
- #if  defined(CTIME)
- #undef CTIME
- #define CTIME( c, b, l )		ctime( c )
- #endif
- #endif
- 
- #if defined(LINUX2_0) || defined(LINUX2_1)
- #if defined(CTIME)
- #undef CTIME
- #define CTIME( c, b, l )                ctime( c )
- #endif
- #endif
- 
  /* This is totally lame, since it should be coming from time.h, but isn't. */
  #if defined(SOLARIS) 
  char *ctime_r(const time_t *, char *, int);
--- 28,33 ----
***************
*** 919,925 ****
      time_t		gmttime;
  /* CTIME for this platform doesn't use this. */
  #if !defined(SUNOS4) && !defined(BSDI) && !defined(LINUX1_2) && \
!     !defined(SNI) && !defined(_WIN32) && !defined(macintosh)
      char		buf[26];
  #endif
  
--- 898,904 ----
      time_t		gmttime;
  /* CTIME for this platform doesn't use this. */
  #if !defined(SUNOS4) && !defined(BSDI) && !defined(LINUX1_2) && \
!     !defined(SNI) && !defined(_WIN32) && !defined(macintosh) && !defined(LINUX)
      char		buf[26];
  #endif
  
***************
*** 963,969 ****
      }
  
      gmttime = gtime( &t );
!     timestr = CTIME( &gmttime, buf, sizeof(buf) );
  
      timestr[ strlen( timestr ) - 1 ] = zone;	/* replace trailing newline */
      if ( dateonly ) {
--- 942,948 ----
      }
  
      gmttime = gtime( &t );
!     timestr = NSLDAPI_CTIME( &gmttime, buf, sizeof(buf) );
  
      timestr[ strlen( timestr ) - 1 ] = zone;	/* replace trailing newline */
      if ( dateonly ) {
