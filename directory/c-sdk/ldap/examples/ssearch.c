/*
 * Copyright (c) 2000.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Use an SSL connection to search the directory for all people whose
 * surname (last name) is "Jensen".  Since the "sn" attribute is a
 * caseignorestring (cis), case is not significant when searching.
 *
 * Authenticate using a client certificate.
 *
 */

#include "examples.h"
#include "ldap_ssl.h"

#define MY_CERTDB		"/usr/netscape4/alias/client-cert.db"
#define MY_KEYDB		"/usr/netscape4/alias/client-key.db"
#define MY_KEYNICKNAME	"Server-Key"
#define MY_CERTNICKNAME	"Server-Cert"
#define MY_KEYPASSWD	"secret"

int
main( int argc, char **argv )
{
	LDAP		*ld;
	LDAPMessage	*result, *e;
	BerElement	*ber;
	char		*a, *dn;
	char		**vals;
	int		i;

	/* Initialize the client */
	if ( ldapssl_clientauth_init( MY_CERTDB, NULL, 1 /* need key db */,
	    MY_KEYDB, NULL ) < 0 ) {
		perror( "ldapssl_clientauth_init" );
		return( 1 );
	}

	/* get a handle to an LDAP connection */
	if ( (ld = ldapssl_init( MY_HOST, MY_SSL_PORT, 1 )) == NULL ) {
		perror( "ldapssl_init" );
		return( 1 );
	}

	/* use LDAPv3 */
	i = LDAP_VERSION3;
	if ( ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &i ) < 0 ) {
		ldap_perror( ld, "ldap_set_option LDAPv3" );
		ldap_unbind( ld );
		return( 1 );
	}

	/* enable certificate-based client authentication. */
	if ( ldapssl_enable_clientauth( ld, MY_KEYNICKNAME, MY_KEYPASSWD,
			MY_CERTNICKNAME ) != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldapssl_enable_clientauth" );
		ldap_unbind( ld );
		return( 1 );
	}

	if ( ldap_sasl_bind_s( ld, NULL, LDAP_SASL_EXTERNAL, NULL, NULL, NULL,
			NULL ) != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_sasl_bind_s EXTERNAL" );
		ldap_unbind( ld );
		return( 1 );
	}

	/* search for all entries with surname of Jensen */
	if ( ldap_search_s( ld, "o=Airius.com", LDAP_SCOPE_SUBTREE,
		"(sn=jensen)", NULL, 0, &result ) != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_search_s" );
		if ( result == NULL ) {
			ldap_unbind( ld );
			return( 1 );
		}
	}
	/* for each entry print out name + all attrs and values */
	for ( e = ldap_first_entry( ld, result ); e != NULL;
	    e = ldap_next_entry( ld, e ) ) {
		if ( (dn = ldap_get_dn( ld, e )) != NULL ) {
		    printf( "dn: %s\n", dn );
		    ldap_memfree( dn );
		}
		for ( a = ldap_first_attribute( ld, e, &ber );
		    a != NULL; a = ldap_next_attribute( ld, e, ber ) ) {
			if ((vals = ldap_get_values( ld, e, a)) != NULL ) {
				for ( i = 0; vals[i] != NULL; i++ ) {
				    printf( "%s: %s\n", a, vals[i] );
				}
				ldap_value_free( vals );
			}
			ldap_memfree( a );
		}
		if ( ber != NULL ) {
			ber_free( ber, 0 );
		}
		printf( "\n" );
	}
	ldap_msgfree( result );
	ldap_unbind( ld );
	return( 0 );
}
