/*
 * Copyright (c) 1996.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Use ldap_compare() to compare values agains values contained in entry
 * ENTRYDN (defined in examples.h)
 * We test to see if (1) the value "person" is one of the values in the
 * objectclass attribute (it is), and if (2) the value "xyzzy" is in the
 * objectlass attribute (it isn't, or at least, it shouldn't be).
 *
 */

#include "examples.h"

int
main( int main, char **argv )
{
    LDAP	*ld;
    int		rc;

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

    /* compare the value "person" against the objectclass attribute */
    rc = ldap_compare_s( ld, ENTRYDN, "objectclass", "person" );
    switch ( rc ) {
    case LDAP_COMPARE_TRUE:
	printf( "The value \"person\" is contained in the objectclass "
		"attribute.\n" );
	break;
    case LDAP_COMPARE_FALSE:
	printf( "The value \"person\" is not contained in the objectclass "
		"attribute.\n" );
	break;
    default:
	ldap_perror( ld, "ldap_compare_s" );
    }

    /* compare the value "xyzzy" against the objectclass attribute */
    rc = ldap_compare_s( ld, ENTRYDN, "objectclass", "xyzzy" );
    switch ( rc ) {
    case LDAP_COMPARE_TRUE:
	printf( "The value \"xyzzy\" is contained in the objectclass "
		"attribute.\n" );
	break;
    case LDAP_COMPARE_FALSE:
	printf( "The value \"xyzzy\" is not contained in the objectclass "
		"attribute.\n" );
	break;
    default:
	ldap_perror( ld, "ldap_compare_s" );
    }

    ldap_unbind( ld );
    return( 0 );
}
