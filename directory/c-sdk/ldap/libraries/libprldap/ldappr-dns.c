/*
 * Copyright (c) 2000.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * DNS callback functions for libldap that use the NSPR (Netscape
 * Portable Runtime) thread API.
 *
 */

#include "ldappr-int.h"

static LDAPHostEnt *prldap_gethostbyname( const char *name,
	LDAPHostEnt *result, char *buffer, int buflen, int *statusp,
	void *extradata );
static LDAPHostEnt *prldap_gethostbyaddr( const char *addr, int length,
	int type, LDAPHostEnt *result, char *buffer, int buflen,
	int *statusp, void *extradata );
static LDAPHostEnt *prldap_convert_hostent( LDAPHostEnt *ldhp,
	PRHostEnt *prhp );


/*
 * Install NSPR DNS functions into ld (if ld is NULL, they are installed
 * as the default functions for new LDAP * handles).
 *
 * Returns 0 if all goes well and -1 if not.
 */
int
prldap_install_dns_functions( LDAP *ld )
{
    struct ldap_dns_fns			dnsfns;

    memset( &dnsfns, '\0', sizeof(struct ldap_dns_fns) );
    dnsfns.lddnsfn_bufsize = PR_NETDB_BUF_SIZE;
    dnsfns.lddnsfn_gethostbyname = prldap_gethostbyname;
    dnsfns.lddnsfn_gethostbyaddr = prldap_gethostbyaddr;
    if ( ldap_set_option( ld, LDAP_OPT_DNS_FN_PTRS, (void *)&dnsfns ) != 0 ) {
	return( -1 );
    }

    return( 0 );
}


static LDAPHostEnt *
prldap_gethostbyname( const char *name, LDAPHostEnt *result,
	char *buffer, int buflen, int *statusp, void *extradata )
{
	PRHostEnt	prhent;

	if( !statusp || ( *statusp = (int)PR_GetHostByName( name, buffer, 
			buflen, &prhent )) == PR_FAILURE ) {
		return( NULL );
	}

	return( prldap_convert_hostent( result, &prhent ));
}


static LDAPHostEnt *
prldap_gethostbyaddr( const char *addr, int length, int type,
	LDAPHostEnt *result, char *buffer, int buflen, int *statusp,
	void *extradata )
{
    PRHostEnt	prhent;
    PRNetAddr	iaddr;

    if ( PR_InitializeNetAddr(PR_IpAddrNull, 0, &iaddr) == PR_FAILURE ||
	    (iaddr.inet.ip = inet_addr(addr)) == -1 ) {
        return( NULL );
    }

    if( !statusp || (*statusp = PR_GetHostByAddr(&iaddr, buffer,
	     buflen, &prhent )) == PR_FAILURE ) {
	return( NULL );
    }

    return( prldap_convert_hostent( result, &prhent ));
}


/*
 * Function: prldap_convert_hostent()
 * Description: copy the fields of a PRHostEnt struct to an LDAPHostEnt
 * Returns: the LDAPHostEnt pointer passed in.
 */
static LDAPHostEnt *
prldap_convert_hostent( LDAPHostEnt *ldhp, PRHostEnt *prhp )
{
	ldhp->ldaphe_name = prhp->h_name;
	ldhp->ldaphe_aliases = prhp->h_aliases;
	ldhp->ldaphe_addrtype = prhp->h_addrtype;
	ldhp->ldaphe_length =  prhp->h_length;
	ldhp->ldaphe_addr_list =  prhp->h_addr_list;
	return( ldhp );
}
