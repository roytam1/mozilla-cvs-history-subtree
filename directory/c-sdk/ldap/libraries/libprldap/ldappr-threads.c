/*
 * Copyright (c) 2000.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Thread callback functions for libldap that use the NSPR (Netscape
 * Portable Runtime) thread API.
 *
 */

#include "ldappr-int.h"

/*
 * Structures and types:
 */
/*
 * Structure used by libldap thread callbacks to maintain error information.
 */
typedef struct prldap_errorinfo {
    int		plei_lderrno;
    char	*plei_matched;
    char	*plei_errmsg;
} PRLDAP_ErrorInfo;


/*
 * Structure used by associate an NSPR thread-private-data index with an
 * LDAP session handle.
 */
typedef struct prldap_tpd_map {
    LDAP			*prtm_ld;	/* non-NULL if in use */
    PRUintn			prtm_index;	/* allocated by NSPR */
    struct prldap_tpd_map	*prtm_next;
} PRLDAP_TPDMap;



/*
 * Static Variables:
 */
/*
 * prldap_map_list points to all of the PRLDAP_TPDMap structures
 * we have ever allocated.  We recycle them as we open and close LDAP
 * sessions.
 */
static PRLDAP_TPDMap *prldap_map_list = NULL;


/*
 * The prldap_map_mutex is used to protect access to the prldap_map_list.
 */
static PRLock	*prldap_map_mutex = NULL;

/*
 * The prldap_callonce_init_map_list structure is used by NSPR to ensure
 * that prldap_init_map_list() is called at most once.
 */
static PRCallOnceType prldap_callonce_init_map_list = { 0, 0, 0 };


/*
 * Private function prototypes:
 */
static void prldap_set_ld_error( int err, char *matched, char *errmsg,
	void *errorarg );
static int prldap_get_ld_error( char **matchedp, char **errmsgp,
	void *errorarg );
static void *prldap_mutex_alloc( void );
static void prldap_mutex_free( void *mutex );
static int prldap_mutex_lock( void *mutex );
static int prldap_mutex_unlock( void *mutex );
static void *prldap_get_thread_id( void );
static PRStatus prldap_init_map_list( void );
static PRLDAP_TPDMap *prldap_allocate_map( LDAP *ld );
static void prldap_return_map( PRLDAP_TPDMap *map );


/*
 * Install NSPR thread functions into ld (if ld is NULL, they are installed
 * as the default functions for new LDAP * handles).
 *
 * Returns 0 if all goes well and -1 if not.
 */
int
prldap_install_thread_functions( LDAP *ld, int shared )
{
    struct ldap_thread_fns		tfns;
    struct ldap_extra_thread_fns	xtfns;

    if ( PR_CallOnce( &prldap_callonce_init_map_list, prldap_init_map_list )
		!= PR_SUCCESS ) {
	ldap_set_lderrno( ld, LDAP_LOCAL_ERROR, NULL, NULL );
	return( -1 );
    }

    /* set thread function pointers */
    memset( &tfns, '\0', sizeof(struct ldap_thread_fns) );
    tfns.ltf_get_errno = prldap_get_system_errno;
    tfns.ltf_set_errno = prldap_set_system_errno;
    if ( shared ) {
	tfns.ltf_mutex_alloc = prldap_mutex_alloc;
	tfns.ltf_mutex_free = prldap_mutex_free;
	tfns.ltf_mutex_lock = prldap_mutex_lock;
	tfns.ltf_mutex_unlock = prldap_mutex_unlock;
	tfns.ltf_get_lderrno = prldap_get_ld_error;
	tfns.ltf_set_lderrno = prldap_set_ld_error;
	if ( ld != NULL ) {
	    /*
	     * If this is a real ld (i.e., we are not setting the global
	     * defaults) allocate thread private data for error information.
	     * If ld is NULL we do not do this here but it is done in
	     * prldap_thread_new_handle().
	     */
	    if (( tfns.ltf_lderrno_arg = (void *)prldap_allocate_map( ld ))
		    == NULL ) {
		return( -1 );
	    }
	}
    }

    if ( ldap_set_option( ld, LDAP_OPT_THREAD_FN_PTRS,
	    (void *)&tfns ) != 0 ) {
	prldap_return_map( (PRLDAP_TPDMap *)tfns.ltf_lderrno_arg );
	return( -1 );
    }

    /* set extended thread function pointers */
    memset( &xtfns, '\0', sizeof(struct ldap_extra_thread_fns) );
    xtfns.ltf_threadid_fn = prldap_get_thread_id;
    if ( ldap_set_option( ld, LDAP_OPT_EXTRA_THREAD_FN_PTRS,
	    (void *)&xtfns ) != 0 ) {
	return( -1 );
    }

    return( 0 );
}


static void *
prldap_mutex_alloc( void )
{
    return( (void *)PR_NewLock());
}


static void
prldap_mutex_free( void *mutex )
{
    PR_DestroyLock( (PRLock *)mutex );
}


static int
prldap_mutex_lock( void *mutex )
{
    PR_Lock( (PRLock *)mutex );
    return( 0 );
}


static int
prldap_mutex_unlock( void *mutex )
{
    if ( PR_Unlock( (PRLock *)mutex ) == PR_FAILURE ) {
	return( -1 );
    }

    return( 0 );
}


static void *
prldap_get_thread_id( void )
{
    return( (void *)PR_GetCurrentThread());
}


static int
prldap_get_ld_error( char **matchedp, char **errmsgp, void *errorarg )
{
    PRLDAP_TPDMap	*map;
    PRLDAP_ErrorInfo	*eip;

    if (( map = (PRLDAP_TPDMap *)errorarg ) != NULL && ( eip =
	    (PRLDAP_ErrorInfo *)PR_GetThreadPrivate(
	    map->prtm_index )) != NULL ) {
	if ( matchedp != NULL ) {
	    *matchedp = eip->plei_matched;
	}
	if ( errmsgp != NULL ) {
	    *errmsgp = eip->plei_errmsg;
	}
	return( eip->plei_lderrno );
    } else {
	if ( matchedp != NULL ) {
	    *matchedp = NULL;
	}
	if ( errmsgp != NULL ) {
	    *errmsgp = NULL;
	}
	return( LDAP_LOCAL_ERROR );	/* punt */
    }
}


static void
prldap_set_ld_error( int err, char *matched, char *errmsg, void *errorarg )
{
    PRLDAP_TPDMap	*map;
    PRLDAP_ErrorInfo	*eip;

    if (( map = (PRLDAP_TPDMap *)errorarg ) != NULL ) {
	if (( eip = (PRLDAP_ErrorInfo *)PR_GetThreadPrivate(
		map->prtm_index )) == NULL ) {
	    /*
	     * error info. has not yet been allocated for this thread.
	     * do it now (we never free this memory since it is recycled
	     * and used by other LDAP sessions, etc.
	     */
	    eip = (PRLDAP_ErrorInfo *)PR_Calloc( 1,
		    sizeof( PRLDAP_ErrorInfo ));
	    if ( eip == NULL ) {
		return;	/* punt */
	    }
	    (void)PR_SetThreadPrivate( map->prtm_index, eip );
	}

	eip->plei_lderrno = err;
	if ( eip->plei_matched != NULL ) {
	    ldap_memfree( eip->plei_matched );
	}
	eip->plei_matched = matched;
	if ( eip->plei_errmsg != NULL ) {
	    ldap_memfree( eip->plei_errmsg );
	}
	eip->plei_errmsg = errmsg;
    }
}


/*
 * Called when a new LDAP * session handle is allocated.
 * Allocate thread-private data for error information, but only if
 * it has not already been allocated and the get_ld_error callback has
 * been installed.  If ld is not NULL when prldap_install_thread_functions()
 * is called, we will have already allocated the thread-private data there.
 */
int
prldap_thread_new_handle( LDAP *ld, void *sessionarg )
{
    struct ldap_thread_fns	tfns;

    if ( ldap_get_option( ld, LDAP_OPT_THREAD_FN_PTRS, (void *)&tfns ) != 0 ) {
	return( LDAP_LOCAL_ERROR );
    }

    if ( tfns.ltf_lderrno_arg == NULL && tfns.ltf_get_lderrno != NULL ) {
	if (( tfns.ltf_lderrno_arg = (void *)prldap_allocate_map( ld )) == NULL
		|| ldap_set_option( ld, LDAP_OPT_THREAD_FN_PTRS,
		(void *)&tfns ) != 0 ) {
	    return( LDAP_LOCAL_ERROR );
	}
    }

    return( LDAP_SUCCESS );
}


/*
 * Called when an LDAP * session handle is being destroyed.
 * Clean up our thread private data map.
 */
void
prldap_thread_dispose_handle( LDAP *ld, void *sessionarg )
{
    struct ldap_thread_fns	tfns;

    if ( ldap_get_option( ld, LDAP_OPT_THREAD_FN_PTRS,
	    (void *)&tfns ) == 0 &&
	    tfns.ltf_lderrno_arg != NULL ) {
	prldap_return_map( (PRLDAP_TPDMap *)tfns.ltf_lderrno_arg );
    }
}


static PRStatus
prldap_init_map_list( void )
{
    if (( prldap_map_mutex = PR_NewLock()) == NULL ) {
	return( PR_FAILURE );
    }

    prldap_map_list = NULL;

    return( PR_SUCCESS );
}


/*
 * Function: prldap_allocate_map()
 * Description: allocate a thread-private-data map to use for a new
 *	LDAP session handle.
 * Returns: a pointer to the tsd map or NULL if none available.
 */
static PRLDAP_TPDMap *
prldap_allocate_map( LDAP *ld )
{
    PRLDAP_TPDMap	*map, *prevmap;
    PRUintn		tsdindex;

    PR_Lock( prldap_map_mutex );

    /*
     * first look for a map that is already allocated but free to be re-used
     */
    prevmap = NULL;
    for ( map = prldap_map_list; map != NULL; map = map->prtm_next ) {
	if ( map->prtm_ld == NULL ) {
	    break;
	}
	prevmap = map;
    }

    /*
     * if none we found (map == NULL), try to allocate a new one and add it
     * to the end of our global list.
     */
    if ( map == NULL && PR_NewThreadPrivateIndex( &tsdindex, NULL )
	    == PR_SUCCESS ) {
	map = (PRLDAP_TPDMap *)PR_Malloc( sizeof( PRLDAP_TPDMap ));
	if ( map != NULL ) {
	    map->prtm_index = tsdindex;
	    map->prtm_next = NULL;
	    if ( prevmap == NULL ) {
		prldap_map_list = map;
	    } else {
		prevmap->prtm_next = map;
	    }
	}
    }

    if ( map != NULL ) {
	map->prtm_ld = ld;	/* now marked as "in use" */
				/* since we are reusing...reset */
				/* to initial state */
	(void)PR_SetThreadPrivate( map->prtm_index, NULL );
    }

    PR_Unlock( prldap_map_mutex );

    return( map );
}


/*
 * Function: prldap_return_map()
 * Description: return a thread-private-data map to the pool of ones
 *	available for re-use.
 */
static void
prldap_return_map( PRLDAP_TPDMap *map )
{
    PRLDAP_ErrorInfo	*eip;

    PR_Lock( prldap_map_mutex );

    /* dispose of thread-private LDAP error information */
    if (( eip = (PRLDAP_ErrorInfo *)PR_GetThreadPrivate( map->prtm_index ))
		!= NULL ) {
	if ( eip->plei_matched != NULL ) {
	    ldap_memfree( eip->plei_matched );
	}
	if ( eip->plei_errmsg != NULL ) {
	    ldap_memfree( eip->plei_errmsg );
	}

	PR_Free( eip );
    }

    /* mark map as available for re-use */
    map->prtm_ld = NULL;

    PR_Unlock( prldap_map_mutex );
}
