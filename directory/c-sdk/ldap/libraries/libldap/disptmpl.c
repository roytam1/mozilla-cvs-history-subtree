*** disptmpl.c	Tue Feb  8 11:06:58 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/disptmpl.c	Fri May  5 10:48:48 2000
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
   * Copyright (c) 1993, 1994 Regents of the University of Michigan.
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
   * Copyright (c) 1993, 1994 Regents of the University of Michigan.
***************
*** 735,745 ****
  };
  
  static struct tmplerror ldap_tmplerrlist[] = {
! 	LDAP_TMPL_ERR_VERSION, "Bad template version",
! 	LDAP_TMPL_ERR_MEM,     "Out of memory", 
! 	LDAP_TMPL_ERR_SYNTAX,  "Bad template syntax",
! 	LDAP_TMPL_ERR_FILE,    "File error reading template",
! 	-1, 0
  };
  
  char *
--- 731,741 ----
  };
  
  static struct tmplerror ldap_tmplerrlist[] = {
! 	{ LDAP_TMPL_ERR_VERSION, "Bad template version"		},
! 	{ LDAP_TMPL_ERR_MEM,     "Out of memory"		}, 
! 	{ LDAP_TMPL_ERR_SYNTAX,  "Bad template syntax"		},
! 	{ LDAP_TMPL_ERR_FILE,    "File error reading template"	},
! 	{ -1, 0 }
  };
  
  char *
