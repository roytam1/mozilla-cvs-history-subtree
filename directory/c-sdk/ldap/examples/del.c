/*
 * Copyright (c) 1996.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Delete an entry from the directory.
 *
 * Instead of calling the synchronous ldap_delete_s() routine, we call
 * the asynchronous routine ldap_delete() and poll for results using
 * ldap_result().
 *
 * Since it is an error to attempt to delete an entry which does not
 * exist, you cannot run this example until you have added the entry
 * with the aadd.c example program.
 *
 */

#include "examples.h"

static void do_other_work();
unsigned long	global_counter = 0;

int
main( int argc, char **argv )
{
    LDAP	    	*ld;
    LDAPMessage	    	*result;
    char	    	*dn;
    int		   	rc;
    int			msgid;
    int			finished;
    struct timeval	zerotime;

    zerotime.tv_sec = zerotime.tv_usec = 0L;

    /* Specify the DN we're deleting */
    dn = "cn=William B Jensen, " PEOPLE_BASE;	/* see examples.h */

    /* get a handle to an LDAP connection */
    if ( (ld = ldap_init( MY_HOST, MY_PORT )) == NULL ) {
	perror( "ldap_init" );
	return( 1 );
    }
    /* authenticate to the directory as the Directory Manager */
    if ( ldap_simple_bind_s( ld, MGR_DN, MGR_PW ) != LDAP_SUCCESS ) {
	ldap_perror( ld, "ldap_simple_bind_s" );
	return( 1 );
    }
    /* Initiate the delete operation */
    if (( msgid = ldap_delete( ld, dn )) < 0 ) {
	ldap_perror( ld, "ldap_delete" );
	return( 1 );
    }

    /* Poll for the result */
    finished = 0;
    while ( !finished ) {
	rc = ldap_result( ld, msgid, LDAP_MSG_ONE, &zerotime, &result );
	switch ( rc ) {
	case -1:
	    /* some error occurred */
	    ldap_perror( ld, "ldap_result" );
	    return( 1 );
	case 0:
	    /* Timeout was exceeded.  No entries are ready for retrieval */
	    break;
	default:
	    /* Should be finished here */
	    finished = 1;
	    if (( rc = ldap_result2error( ld, result, 0 )) == LDAP_SUCCESS ) {
		printf( "Entry deleted successfully.  I counted to %ld "
			"while waiting.\n", global_counter );
	    } else {
		printf( "Error while deleting entry: %s\n",
			ldap_err2string( rc ));
	    }
	    ldap_msgfree( result );
	}
	do_other_work();
    }
    ldap_unbind( ld );
    return 0;
}


/*
 * Perform other work while polling for results.  This doesn't do anything
 * useful, but it could.
 */
static void
do_other_work()
{
    global_counter++;
}
