/*
 * Copyright (c) 1996.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Search the directory for the specific entry ENTRYDN (defined in examples.h)
 * Retrieve all attributes from the entry.
 *
 */

#include "examples.h"

int
main( int argc, char **argv )
{
    LDAP	*ld;
    LDAPMessage	*result, *e;
    BerElement	*ber;
    char	*a, *dn;
    char	**vals;
    int		i;

    /* get a handle to an LDAP connection */
    if ( (ld = ldap_init( MY_HOST, MY_PORT )) == NULL ) {
	perror( "ldap_init" );
	return( 1 );
    }
    /* authenticate to the directory as nobody */
    if ( ldap_simple_bind_s( ld, NULL, NULL ) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_simple_bind_s" );
	return( 1 );
    }
    /* search for Babs' entry */
    if ( ldap_search_s( ld, ENTRYDN, LDAP_SCOPE_SUBTREE,
	    "(objectclass=*)", NULL, 0, &result ) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_search_s" );
	return( 1 );
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
