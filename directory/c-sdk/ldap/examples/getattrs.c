/*
 * Copyright (c) 1996.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Retrieve several attributes of a particular entry.
 */

#include "examples.h"


int
main( int argc, char **argv )
{
    LDAP		*ld;
    LDAPMessage		*result, *e;
    char		**vals, *attrs[ 5 ];
    int			i;

    /* get a handle to an LDAP connection */
    if ( (ld = ldap_init( MY_HOST, MY_PORT )) == NULL ) {
	perror( "ldap_init" );
	return( 1 );
    }

    attrs[ 0 ] = "cn";			/* Get canonical name(s) (full name) */
    attrs[ 1 ] = "sn";			/* Get surname(s) (last name) */
    attrs[ 2 ] = "mail";		/* Get email address(es) */
    attrs[ 3 ] = "telephonenumber";	/* Get telephone number(s) */
    attrs[ 4 ] = NULL;

    if ( ldap_search_s( ld, ENTRYDN, LDAP_SCOPE_BASE,
	    "(objectclass=*)", attrs, 0, &result ) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_search_s" );
	return( 1 );
    }

    /* print it out */
    if (( e = ldap_first_entry( ld, result )) != NULL ) {
	if (( vals = ldap_get_values( ld, e, "cn" )) != NULL ) {
	    printf( "Full name:\n" );
	    for ( i = 0; vals[i] != NULL; i++ ) {
		printf( "\t%s\n", vals[i] );
	    }
	    ldap_value_free( vals );
	}
	if (( vals = ldap_get_values( ld, e, "sn" )) != NULL ) {
	    printf( "Last name (surname):\n" );
	    for ( i = 0; vals[i] != NULL; i++ ) {
		printf( "\t%s\n", vals[i] );
	    }
	    ldap_value_free( vals );
	}
	if (( vals = ldap_get_values( ld, e, "mail" )) != NULL ) {
	    printf( "Email address:\n" );
	    for ( i = 0; vals[i] != NULL; i++ ) {
		printf( "\t%s\n", vals[i] );
	    }
	    ldap_value_free( vals );
	}
	if (( vals = ldap_get_values( ld, e, "telephonenumber" )) != NULL ) {
	    printf( "Telephone number:\n" );
	    for ( i = 0; vals[i] != NULL; i++ ) {
		printf( "\t%s\n", vals[i] );
	    }
	    ldap_value_free( vals );
	}
    }
    ldap_msgfree( result );
    ldap_unbind( ld );
    return( 0 );
}
