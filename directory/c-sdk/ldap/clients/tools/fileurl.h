/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/*
 *  LDAP tools fileurl.h -- defines for file URL functions.
 *  Used by ldapmodify.
 */

/*
 * ldaptool_fileurl2path() convert a file URL to a local path.
 *
 * If successful, LDAPTOOL_FILEURL_SUCCESS is returned and *localpathp is
 * set point to an allocated string.  If not, an differnet LDAPTOOL_FILEURL_
 * error code is returned.
 */
int ldaptool_fileurl2path( char *fileurl, char **localpathp );


/*
 * Convert a local path to a file URL.
 *
 * If successful, LDAPTOOL_FILEURL_SUCCESS is returned and *urlp is
 * set point to an allocated string.  If not, an different LDAPTOOL_FILEURL_
 * error code is returned.  At present, the only possible error is
 * LDAPTOOL_FILEURL_NOMEMORY.
 *
 */
int ldaptool_path2fileurl( char *path, char **urlp );


/*
 * Possible return codes for ldaptool_fileurl2path and ldaptool_path2fileurl.
 */
#define LDAPTOOL_FILEURL_SUCCESS	0
#define LDAPTOOL_FILEURL_NOTAFILEURL	1
#define LDAPTOOL_FILEURL_MISSINGPATH	2
#define LDAPTOOL_FILEURL_NONLOCAL	3
#define LDAPTOOL_FILEURL_NOMEMORY	4
