*** sortctrl.c	Tue Feb  8 11:07:00 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/libraries/libldap/sortctrl.c	Fri Apr 28 11:49:03 2000
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
  #include "ldap-int.h"
  
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
  #include "ldap-int.h"
  
***************
*** 238,257 ****
  	return count;
  }
  
- /* Is this character a valid attribute description character ? */
- static int isattrdescchar(char c)
- {
- 	/* Alphanumeric chars are in */
- 	if (isalnum(c)) {
- 		return 1;
- 	}
- 	/* As is ';' */
- 	if (';' == c) {
- 		return 1;
- 	}
- 	/* Everything else is out */
- 	return 0;
- }
  
  static int read_next_token(const char **s,LDAPsortkey **key)
  {
--- 234,239 ----
***************
*** 370,376 ****
  	int count = 0;
  	LDAPsortkey **pointer_array = NULL;
  	const char *current_position = NULL;
- 	char *s = NULL;
  	int retval = 0;
  	int i = 0;
  
--- 352,357 ----
