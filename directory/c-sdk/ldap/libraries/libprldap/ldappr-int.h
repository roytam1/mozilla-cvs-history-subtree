/*
 * Copyright (c) 2000.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Internal header for libprldap -- glue NSPR (Netscape Portable Runtime)
 * to libldap.
 *
 */

#include <ldap.h>
#include <nspr.h>
#include <ldappr.h>

#include <errno.h>
#include <string.h>

/*
 * Data structures:
 */

/* data structure that populates the I/O callback session arg. */
typedef struct lextiof_session_private {
	PRPollDesc	*prsess_pollds;		/* for poll callback */
	int		prsess_pollds_count;	/* # of elements in pollds */
	void		*prsess_appdata;	/* application specific data */
} PRLDAPIOSessionArg;

/* data structure that populates the I/O callback socket-specific arg. */
typedef struct lextiof_socket_private {
	PRFileDesc	*prsock_prfd;		/* associated NSPR file desc. */
	void		*prsock_appdata;	/* application specific data */
} PRLDAPIOSocketArg;


/*
 * Function prototypes:
 */

/*
 * From ldapprio.c:
 */
int prldap_install_io_functions( LDAP *ld, int shared );
int prldap_session_arg_from_ld( LDAP *ld, PRLDAPIOSessionArg **sessargpp );


/*
 * From ldapprthreads.c:
 */
int prldap_install_thread_functions( LDAP *ld, int shared );
int prldap_thread_new_handle( LDAP *ld, void *sessionarg );
void prldap_thread_dispose_handle( LDAP *ld, void *sessionarg );


/*
 * From ldapprdns.c:
 */
int prldap_install_dns_functions( LDAP *ld );


/*
 * From ldapprerror.c:
 */
void prldap_set_system_errno( int e );
int prldap_get_system_errno( void );
int prldap_prerr2errno( void );
