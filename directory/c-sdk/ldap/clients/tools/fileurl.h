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
