*** regex.c	Wed Aug 16 10:03:33 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/regex.c	Fri May  5 10:48:51 2000
***************
*** 1,23 ****
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public
!  * License Version 1.1 (the "License"); you may not use this file
!  * except in compliance with the License. You may obtain a copy of
!  * the License at http://www.mozilla.org/NPL/
!  *
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
  #if defined( macintosh ) || defined( DOS ) || defined( _WINDOWS ) || defined( NEED_BSDREGEX ) || defined( XP_OS2)
--- 1,19 ----
  /* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
   *
!  * The contents of this file are subject to the Netscape Public License
!  * Version 1.0 (the "NPL"); you may not use this file except in
!  * compliance with the NPL.  You may obtain a copy of the NPL at
!  * http://www.mozilla.org/NPL/
!  *
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
  #if defined( macintosh ) || defined( DOS ) || defined( _WINDOWS ) || defined( NEED_BSDREGEX ) || defined( XP_OS2)
***************
*** 241,248 ****
  					/* pre-set bits...   */
  static REGEXCHAR bitarr[] = {1,2,4,8,16,32,64,128};
  
- static void nfadump( REGEXCHAR *ap);
- 
  static void
  chset(REGEXCHAR c)
  {
--- 237,242 ----
***************
*** 268,278 ****
  	register REGEXCHAR mask;	/* xor mask -CCL/NCL */
  	int c1, c2;
  		
! 	if (!pat || !*pat)
! 		if (sta)
  			return 0;
! 		else
  			return badpat("No previous regular expression");
  	sta = NOP;
  
  	for (p = (REGEXCHAR*)pat; *p; p++) {
--- 262,274 ----
  	register REGEXCHAR mask;	/* xor mask -CCL/NCL */
  	int c1, c2;
  		
! 	if (!pat || !*pat) {
! 		if (sta) {
  			return 0;
! 		} else {
  			return badpat("No previous regular expression");
+ 		}
+ 	}
  	sta = NOP;
  
  	for (p = (REGEXCHAR*)pat; *p; p++) {
***************
*** 799,805 ****
  	return 1;
  }
  			
! #ifdef LDAP_REGEX_DEBUG
  
  /* No printf or exit in 16-bit Windows */
  #if defined( _WINDOWS ) && !defined( _WIN32 )
--- 795,801 ----
  	return 1;
  }
  			
! #ifdef DEBUG
  
  /* No printf or exit in 16-bit Windows */
  #if defined( _WINDOWS ) && !defined( _WIN32 )
***************
*** 814,821 ****
  	return 0;
  }
  #define exit(v) return
! #endif
  
  /*
   * symbolic - produce a symbolic dump of the nfa
   */
--- 810,822 ----
  	return 0;
  }
  #define exit(v) return
! #endif /* 16-bit Windows */
! 
! 
! #ifdef REGEX_DEBUG
  
+ static void nfadump( REGEXCHAR *ap);
+ 
  /*
   * symbolic - produce a symbolic dump of the nfa
   */
***************
*** 895,900 ****
  			break;
  		}
  }
! #endif /* LDAP_REGEX_DEBUG */
! 
  #endif /* macintosh or DOS or _WINDOWS or NEED_BSDREGEX */
--- 896,901 ----
  			break;
  		}
  }
! #endif /* REGEX_DEBUG */
! #endif /* DEBUG */
  #endif /* macintosh or DOS or _WINDOWS or NEED_BSDREGEX */
