/*
 * Copyright (c) 2000.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Extended I/O callback functions for libldap that use
 * NSPR (Netscape Portable Runtime) I/O.
 *
 * High level strategy: we use the socket-specific arg to hold our own data
 * structure that includes the NSPR file handle (PRFileDesc *), among other
 * useful information.  We use the default argument to hold an LDAP session
 * handle specific data structure.
 */

#include "ldappr-int.h"

#define PRLDAP_POLL_ARRAY_GROWTH  5  /* grow arrays 5 elements at a time */

/*
 * XXXmcs: change the following #define to PR_AF_INET6 once we upgrade
 * to NSPR 4 (or for IPv6 support with NSPR 3.5.x).
 */
#define PRLDAP_DEFAULT_ADDRESS_FAMILY	PR_AF_INET

/*
 * Local function prototypes:
 */
static PRIntervalTime prldap_timeout2it( int ms_timeout );
static int LDAP_CALLBACK prldap_read( int s, void *buf, int bufsize,
	struct lextiof_socket_private *socketarg );
static int LDAP_CALLBACK prldap_write( int s, const void *buf, int len,
	struct lextiof_socket_private *socketarg );
static int LDAP_CALLBACK prldap_poll( LDAP_X_PollFD fds[], int nfds,
	int timeout, struct lextiof_session_private *sessionarg );
static int LDAP_CALLBACK prldap_connect( const char *hostlist, int defport,
	int timeout, unsigned long options,
	struct lextiof_session_private *sessionarg,
	struct lextiof_socket_private **socketargp );
static int LDAP_CALLBACK prldap_close( int s,
	struct lextiof_socket_private *socketarg );
static int LDAP_CALLBACK prldap_newhandle( LDAP *ld,
	struct lextiof_session_private *sessionarg );
static void LDAP_CALLBACK prldap_disposehandle( LDAP *ld,
	struct lextiof_session_private *sessionarg );
static int LDAP_CALLBACK prldap_shared_newhandle( LDAP *ld,
	struct lextiof_session_private *sessionarg );
static void LDAP_CALLBACK prldap_shared_disposehandle( LDAP *ld,
	struct lextiof_session_private *sessionarg );
static PRLDAPIOSessionArg *prldap_session_arg_alloc( void );
static void prldap_session_arg_free( PRLDAPIOSessionArg **prsesspp );
static PRLDAPIOSocketArg *prldap_socket_arg_alloc( void );
static void prldap_socket_arg_free( PRLDAPIOSocketArg **prsockpp );
static void *prldap_safe_realloc( void *ptr, PRUint32 size );



/*
 * Local macros:
 */
/* given a socket-specific arg, return the corresponding PRFileDesc * */
#define PRLDAP_GET_PRFD( socketarg )	\
		(((PRLDAPIOSocketArg *)(socketarg))->prsock_prfd)


/*
 * Install NSPR I/O functions into ld (if ld is NULL, they are installed
 * as the default functions for new LDAP * handles).
 *
 * Returns 0 if all goes well and -1 if not.
 */
int
prldap_install_io_functions( LDAP *ld, int shared )
{
    struct ldap_x_ext_io_fns	iofns;

    memset( &iofns, 0, sizeof(iofns));
    iofns.lextiof_size = LDAP_X_EXTIO_FNS_SIZE;
    iofns.lextiof_read = prldap_read;
    iofns.lextiof_write = prldap_write;
    iofns.lextiof_poll = prldap_poll;
    iofns.lextiof_connect = prldap_connect;
    iofns.lextiof_close = prldap_close;
    if ( shared ) {
	iofns.lextiof_newhandle = prldap_shared_newhandle;
	iofns.lextiof_disposehandle = prldap_shared_disposehandle;
    } else {
	iofns.lextiof_newhandle = prldap_newhandle;
	iofns.lextiof_disposehandle = prldap_disposehandle;
    }
    if ( NULL != ld ) {
	/*
	 * If we are dealing with a real ld, we allocate the session specific
	 * data structure now.  If not allocated here, it will be allocated
	 * inside prldap_newhandle() or prldap_shared_newhandle().
	 */
	if ( NULL ==
		( iofns.lextiof_session_arg = prldap_session_arg_alloc())) {
	    ldap_set_lderrno( ld, LDAP_NO_MEMORY, NULL, NULL );
	    return( -1 );
	}
    } else {
	iofns.lextiof_session_arg = NULL;
    }

    if ( ldap_set_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS, &iofns ) != 0 ) {
	prldap_session_arg_free(
		(PRLDAPIOSessionArg **) &iofns.lextiof_session_arg );
	return( -1 );
    }

    return( 0 );
}


static PRIntervalTime
prldap_timeout2it( int ms_timeout )
{
    PRIntervalTime	prit;

    if ( LDAP_X_IO_TIMEOUT_NO_WAIT == ms_timeout ) {
	prit = PR_INTERVAL_NO_WAIT;
    } else if ( LDAP_X_IO_TIMEOUT_NO_TIMEOUT == ms_timeout ) {
	prit = PR_INTERVAL_NO_TIMEOUT;
    } else {
	prit = PR_MillisecondsToInterval( ms_timeout );
    }

    return( prit );
}


static int LDAP_CALLBACK
prldap_read( int s, void *buf, int bufsize,
	struct lextiof_socket_private *socketarg )
{
    return( PR_Read( PRLDAP_GET_PRFD(socketarg), buf, bufsize ));
}


static int LDAP_CALLBACK
prldap_write( int s, const void *buf, int len,
	struct lextiof_socket_private *socketarg )
{
    return( PR_Write( PRLDAP_GET_PRFD(socketarg), buf, len ));
}


struct prldap_eventmap_entry {
    PRInt16	evm_nspr;	/* corresponding NSPR PR_Poll() event */
    int		evm_ldap;	/* LDAP poll event */
};

static struct prldap_eventmap_entry prldap_eventmap[] = {
    { PR_POLL_READ,	LDAP_X_POLLIN },
    { PR_POLL_EXCEPT,	LDAP_X_POLLPRI },
    { PR_POLL_WRITE,	LDAP_X_POLLOUT },
    { PR_POLL_ERR,	LDAP_X_POLLERR },
    { PR_POLL_HUP,	LDAP_X_POLLHUP },
    { PR_POLL_NVAL,	LDAP_X_POLLNVAL },
};

#define PRLDAP_EVENTMAP_ENTRIES	\
	sizeof(prldap_eventmap)/sizeof(struct prldap_eventmap_entry )

static int LDAP_CALLBACK
prldap_poll( LDAP_X_PollFD fds[], int nfds, int timeout,
	struct lextiof_session_private *sessionarg )
{
    PRLDAPIOSessionArg	*prsessp = sessionarg;
    PRPollDesc		*pds;
    int			i, j, rc;

    if ( NULL == prsessp ) {
	prldap_set_system_errno( EINVAL );
	return( -1 );
    }

    /* allocate or resize NSPR poll descriptor array */
    if ( prsessp->prsess_pollds_count < nfds ) {
	pds = prldap_safe_realloc( prsessp->prsess_pollds,
		( nfds + PRLDAP_POLL_ARRAY_GROWTH )
		* sizeof( PRPollDesc ));
	if ( NULL == pds ) {
	    prldap_set_system_errno( prldap_prerr2errno());
	    return( -1 );
	}
	prsessp->prsess_pollds = pds;
	prsessp->prsess_pollds_count = nfds + PRLDAP_POLL_ARRAY_GROWTH;
    } else {
	pds = prsessp->prsess_pollds;
    }

    /* populate NSPR poll info. based on LDAP info. */
    for ( i = 0; i < nfds; ++i ) {
	if ( NULL == fds[i].lpoll_socketarg ) {
	    pds[i].fd = NULL;
	} else {
	    pds[i].fd = PRLDAP_GET_PRFD( fds[i].lpoll_socketarg );
	}
	pds[i].in_flags = pds[i].out_flags = 0;
	if ( fds[i].lpoll_fd >= 0 ) {
	    for ( j = 0; j < PRLDAP_EVENTMAP_ENTRIES; ++j ) {
		if (( fds[i].lpoll_events & prldap_eventmap[j].evm_ldap )
		    != 0 ) {
			pds[i].in_flags |= prldap_eventmap[j].evm_nspr;
		}
	    }
	}
	fds[i].lpoll_revents = 0;	/* clear revents */
    }

    /* call PR_Poll() to do the real work */
    rc = PR_Poll( pds, nfds, prldap_timeout2it( timeout ));

    /* populate LDAP info. based on NSPR results */
    for ( i = 0; i < nfds; ++i ) {
	if ( pds[i].fd != NULL ) {
	    for ( j = 0; j < PRLDAP_EVENTMAP_ENTRIES; ++j ) {
		if (( pds[i].out_flags & prldap_eventmap[j].evm_nspr )
			!= 0 ) {
		    fds[i].lpoll_revents |= prldap_eventmap[j].evm_ldap;
		}
	    }
	}
    }

    return( rc );
}


/*
 * Utility function to try one TCP connect()
 * Returns 1 if successful and -1 if not.  Sets the NSPR fd inside prsockp.
 */
static int
prldap_try_one_address( struct lextiof_socket_private *prsockp,
    PRNetAddr *addrp, int port, int timeout, unsigned long options )
{
    /*
     * Set up address and open a TCP socket:
     */
    if ( PR_SUCCESS != PR_SetNetAddr( PR_IpAddrNull, /* don't touch IP addr. */
		PRLDAP_DEFAULT_ADDRESS_FAMILY, (PRUint16)port, addrp )) { 
	return( -1 );
    }

    if (( prsockp->prsock_prfd = PR_OpenTCPSocket(
		PRLDAP_DEFAULT_ADDRESS_FAMILY )) == NULL ) {
	return( -1 );
    }

    /*
     * Set nonblocking option if requested:
     */
    if ( 0 != ( options & LDAP_X_EXTIOF_OPT_NONBLOCKING )) {
	PRSocketOptionData	optdata;

	optdata.option = PR_SockOpt_Nonblocking;
	optdata.value.non_blocking = PR_TRUE;
	if ( PR_SetSocketOption( prsockp->prsock_prfd, &optdata )
		    != PR_SUCCESS ) {
	    prldap_set_system_errno( prldap_prerr2errno());
	    PR_Close( prsockp->prsock_prfd );
	    return( -1 );
	}
    }

    /*
     * Try to open the TCP connection itself:
     */
    if ( PR_SUCCESS != PR_Connect( prsockp->prsock_prfd, addrp,
		prldap_timeout2it( timeout ))) {
	PR_Close( prsockp->prsock_prfd );
	prsockp->prsock_prfd = NULL;
	return( -1 );
    }

    /*
     * Success.  Return a valid file descriptor (1 is always valid)
     */
    return( 1 );
}


/*
 * XXXmcs: At present, this code ignores the timeout when doing DNS lookups.
 */
static int LDAP_CALLBACK
prldap_connect( const char *hostlist, int defport, int timeout,
	unsigned long options, struct lextiof_session_private *sessionarg,
	struct lextiof_socket_private **socketargp )
{
    int					rc, parse_err, port;
    char				*host, hbuf[ PR_NETDB_BUF_SIZE ];
    struct ldap_x_hostlist_status	*status;
    struct lextiof_socket_private	*prsockp;
    PRNetAddr				addr;
    PRHostEnt				hent;

    if ( 0 != ( options & LDAP_X_EXTIOF_OPT_SECURE )) {
	prldap_set_system_errno( EINVAL );
	return( -1 );
    }

    if ( NULL == ( prsockp = prldap_socket_arg_alloc())) {
	prldap_set_system_errno( prldap_prerr2errno());
	return( -1 );
    }

    rc = -1;	/* pessimistic */
    for ( parse_err = ldap_x_hostlist_first( hostlist, defport, &host, &port,
		&status );
		rc < 0 && LDAP_SUCCESS == parse_err && NULL != host;
		parse_err = ldap_x_hostlist_next( &host, &port, status )) {

	if ( PR_SUCCESS == PR_StringToNetAddr( host, &addr )) {
	    rc = prldap_try_one_address( prsockp, &addr, port,
			timeout, options );
	} else {
	    if ( PR_SUCCESS == PR_GetIPNodeByName( host,
			PRLDAP_DEFAULT_ADDRESS_FAMILY, PR_AI_DEFAULT, hbuf, 
			sizeof( hbuf ), &hent )) {
		PRIntn enumIndex = 0;

		while ( rc < 0 && ( enumIndex = PR_EnumerateHostEnt(
			    enumIndex, &hent, (PRUint16)port, &addr )) > 0 ) {
		    rc = prldap_try_one_address( prsockp, &addr, port,
				timeout, options );
		}
	    }
	}

	ldap_memfree( host );
    }

    ldap_x_hostlist_statusfree( status );

    if ( rc < 0 ) {
	prldap_set_system_errno( prldap_prerr2errno());
	prldap_socket_arg_free( &prsockp );
    } else {
	*socketargp = prsockp;
    }

    return( rc );
}


static int LDAP_CALLBACK
prldap_close( int s, struct lextiof_socket_private *socketarg )
{
    int		rc;

    rc = 0;
    if ( PR_Close( PRLDAP_GET_PRFD(socketarg)) != PR_SUCCESS ) {
	rc = -1;
	prldap_set_system_errno( prldap_prerr2errno());
    }
    prldap_socket_arg_free( &socketarg );

    return( rc );
}


/*
 * LDAP session handle creation callback.
 *
 * Allocate a session argument if not already done, and then call the
 * threads new handle function.
 */
static int LDAP_CALLBACK
prldap_newhandle( LDAP *ld, struct lextiof_session_private *sessionarg )
{

    if ( NULL == sessionarg ) {
	struct ldap_x_ext_io_fns	iofns;

	memset( &iofns, 0, sizeof(iofns));
	iofns.lextiof_size = LDAP_X_EXTIO_FNS_SIZE;
	if ( ldap_get_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS,
		(void *)&iofns ) < 0 ) {
	    return( ldap_get_lderrno( ld, NULL, NULL ));
	}
	if ( NULL ==
		( iofns.lextiof_session_arg = prldap_session_arg_alloc())) {
	    return( LDAP_NO_MEMORY );
	}
	if ( ldap_set_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS,
		    (void *)&iofns ) < 0 ) {
	    return( ldap_get_lderrno( ld, NULL, NULL ));
	}
    }

    return( LDAP_SUCCESS );
}


/* only called/installed if shared is non-zero. */
static int LDAP_CALLBACK
prldap_shared_newhandle( LDAP *ld, struct lextiof_session_private *sessionarg )
{
    int		rc;

    if (( rc = prldap_newhandle( ld, sessionarg )) == LDAP_SUCCESS ) {
	rc = prldap_thread_new_handle( ld, sessionarg );
    }

    return( rc );
}


static void LDAP_CALLBACK
prldap_disposehandle( LDAP *ld, struct lextiof_session_private *sessionarg )
{
    prldap_session_arg_free( &sessionarg );
}


/* only called/installed if shared is non-zero */
static void LDAP_CALLBACK
prldap_shared_disposehandle( LDAP *ld,
	struct lextiof_session_private *sessionarg )
{
    prldap_thread_dispose_handle( ld, sessionarg );
    prldap_disposehandle( ld, sessionarg );
}


/*
 * Allocate a session argument.
 */
static PRLDAPIOSessionArg *
prldap_session_arg_alloc( void )
{
    PRLDAPIOSessionArg		*prsessp;

    prsessp = PR_Calloc( 1, sizeof( PRLDAPIOSessionArg ));
    return( prsessp );
}


static void
prldap_session_arg_free( PRLDAPIOSessionArg **prsesspp )
{
    if ( NULL != prsesspp && NULL != *prsesspp ) {
	if ( NULL != (*prsesspp)->prsess_pollds ) {
	    PR_Free( (*prsesspp)->prsess_pollds );
	    (*prsesspp)->prsess_pollds = NULL;
	}
	PR_Free( *prsesspp );
	*prsesspp = NULL;
    }
}


/*
 * Given an LDAP session handle, retrieve a session argument.
 * Returns an LDAP error code.
 */
int
prldap_session_arg_from_ld( LDAP *ld, PRLDAPIOSessionArg **sessargpp )
{
    struct ldap_x_ext_io_fns	iofns;

    if ( NULL == ld || NULL == sessargpp ) {
	/* XXXmcs: NULL ld's are not supported */
	ldap_set_lderrno( ld, LDAP_PARAM_ERROR, NULL, NULL );
	return( LDAP_PARAM_ERROR );
    }

    memset( &iofns, 0, sizeof(iofns));
    iofns.lextiof_size = LDAP_X_EXTIO_FNS_SIZE;
    if ( ldap_get_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS, (void *)&iofns ) < 0 ) {
	return( ldap_get_lderrno( ld, NULL, NULL ));
    }

    if ( NULL == iofns.lextiof_session_arg ) {
	ldap_set_lderrno( ld, LDAP_LOCAL_ERROR, NULL, NULL );
	return( LDAP_LOCAL_ERROR );
    }

    *sessargpp = iofns.lextiof_session_arg;
    return( LDAP_SUCCESS );
}


/*
 * Allocate a socket argument.
 */
static PRLDAPIOSocketArg *
prldap_socket_arg_alloc( void )
{
    PRLDAPIOSocketArg		*prsockp;

    prsockp = PR_Calloc( 1, sizeof( PRLDAPIOSocketArg ));
    return( prsockp );
}


static void
prldap_socket_arg_free( PRLDAPIOSocketArg **prsockpp )
{
    if ( NULL != prsockpp && NULL != *prsockpp ) {
	PR_Free( *prsockpp );
	*prsockpp = NULL;
    }
}


static void *
prldap_safe_realloc( void *ptr, PRUint32 size )
{
    void	*p;

    if ( NULL == ptr ) {
	p = PR_Malloc( size );
    } else {
	p = PR_Realloc( ptr, size );
    }

    return( p );
}
