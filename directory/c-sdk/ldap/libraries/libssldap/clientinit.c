/*
 * Copyright (c) 1996 Netscape Communications Corp.
 *
 * clientinit.c
 */

#if defined(NET_SSL)


#if defined( _WINDOWS )
#include <windows.h>
#include "proto-ntutil.h"
#endif

#include <nspr.h>
#include <plstr.h>
#include <cert.h>
#include <key.h>
#include <secrng.h>	/* for RNG_RNGInit() and RNG_SystemInfoForRNG() */
#include <secmod.h>	/* for SECMOD_init() */
#include <ssl.h>
#include <sslproto.h>
#include <ldap.h>
#include <ldap_ssl.h>
#include <nss.h>


#ifndef FILE_PATHSEP
#define FILE_PATHSEP '/'
#endif


static PRStatus local_SSLPLCY_Install(void);

/*
 * This little tricky guy keeps us from initializing twice 
 */
static int		inited = 0;
static int ssl_strength = LDAPSSL_AUTH_CERT;
static char  tokDes[34] = "Internal (Software) Database     ";
static char ptokDes[34] = "Internal (Software) Token        ";

static PRStatus local_SSLPLCY_Install(void)
{
	SECStatus s;

#ifdef NS_DOMESTIC
	s = NSS_SetDomesticPolicy(); 
#elif NS_EXPORT
	s = NSS_SetExportPolicy(); 
#else
	s = PR_FAILURE;
#endif
	return s?PR_FAILURE:PR_SUCCESS;
}



static void
ldapssl_basic_init( void )
{
    /* PR_Init() must to be called before everything else... */
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);

    PR_SetConcurrency( 4 );	/* work around for NSPR 3.x I/O hangs */

    /* Initilize NSS' random number generator */
    RNG_SystemInfoForRNG();
    RNG_RNGInit();
}



/*
 * Cover  functions for malloc(), calloc(), strdup() and free() that are
 * compatible with the NSS libraries (they seem to use the C runtime
 * library malloc/free so these functions are quite simple right now).
 */
static void *
ldapssl_malloc( size_t size )
{
    void	*p;

    p = malloc( size );
    return p;
}


static void *
ldapssl_calloc( int nelem, size_t elsize )
{
    void	*p;

    p = calloc( nelem, elsize );
    return p;
}


static char *
ldapssl_strdup( const char *s )
{
    char	*scopy;

    if ( NULL == s ) {
	scopy = NULL;
    } else {
	scopy = strdup( s );
    }
    return scopy;
}


static void
ldapssl_free( void **pp )
{
    if ( NULL != pp && NULL != *pp ) {
	free( (void *)*pp );
	*pp = NULL;
    }
}


static char *
buildDBName(const char *basename, const char *dbname)
{
	char		*result;
	PRUint32	len, pathlen, addslash;

	if (basename)
	{
	    if (( len = PL_strlen( basename )) > 3
		&& PL_strcasecmp( ".db", basename + len - 3 ) == 0 ) {
		return (ldapssl_strdup(basename));
	    }
	    
	    pathlen = len;
	    len = pathlen + PL_strlen(dbname) + 1;
	    addslash = ( pathlen > 0 &&
		(( *(basename + pathlen - 1) != FILE_PATHSEP ) || 
		( *(basename + pathlen - 1) != '\\'  )));

	    if ( addslash ) {
		++len;
	    }
	    if (( result = ldapssl_malloc( len )) != NULL ) {
		PL_strcpy( result, basename );
		if ( addslash ) {
		    *(result+pathlen) = FILE_PATHSEP;  /* replaces '\0' */
		    ++pathlen;
		}
		PL_strcpy(result+pathlen, dbname);
	    }
	    
	}


	return result;
}

char *
GetCertDBName(void *alias, int dbVersion)
{
    char		*source;
    char dbname[128];
    
    source = (char *)alias;
    
    if (!source)
    {
	source = "";
    }
    
    sprintf(dbname, "cert%d.db",dbVersion);
    return(buildDBName(source, dbname));


}

/*
 * return database name by appending "dbname" to "path".
 * this code doesn't need to be terribly efficient (not called often).
 */
/* XXXceb this is the old function.  To be removed eventually */
static char *
GetDBName(const char *dbname, const char *path)
{
    char		*result;
    PRUint32	len, pathlen;
    int		addslash;
    
    if ( dbname == NULL ) {
	dbname = "";
    }
    
    if ((path == NULL) || (*path == 0)) {
	result = ldapssl_strdup(dbname);
    } else {
	pathlen = PL_strlen(path);
	len = pathlen + PL_strlen(dbname) + 1;
	addslash = ( path[pathlen - 1] != '/' );
	if ( addslash ) {
	    ++len;
	}
	if (( result = ldapssl_malloc( len )) != NULL ) {
	    PL_strcpy( result, path );
	    if ( addslash ) {
		*(result+pathlen) = '/';  /* replaces '\0' */
		++pathlen;
	    }
	    PL_strcpy(result+pathlen, dbname);
	}
    }
    
    return result;
}

/*
 * Initialize ns/security so it can be used for SSL client authentication.
 * It is safe to call this more than once.
 *
 * If needkeydb == 0, no key database is opened and SSL server authentication
 * is supported but not client authentication.
 *
 * If "certdbpath" is NULL or "", the default cert. db is used (typically
 * ~/.netscape/cert7.db).
 *
 * If "certdbpath" ends with ".db" (case-insensitive compare), then
 * it is assumed to be a full path to the cert. db file; otherwise,
 * it is assumed to be a directory that contains a file called
 * "cert7.db" or "cert.db".
 *
 * If certdbhandle is non-NULL, it is assumed to be a pointer to a
 * SECCertDBHandle structure.  It is fine to pass NULL since this
 * routine will allocate one for you (CERT_GetDefaultDB() can be
 * used to retrieve the cert db handle).
 *
 * If "keydbpath" is NULL or "", the default key db is used (typically
 * ~/.netscape/key3.db).
 *
 * If "keydbpath" ends with ".db" (case-insensitive compare), then
 * it is assumed to be a full path to the key db file; otherwise,
 * it is assumed to be a directory that contains a file called
 * "key3.db" 
 *
 * If certdbhandle is non-NULL< it is assumed to be a pointed to a
 * SECKEYKeyDBHandle structure.  It is fine to pass NULL since this
 * routine will allocate one for you (SECKEY_GetDefaultDB() can be
 * used to retrieve the cert db handle).
 */
int
LDAP_CALL
ldapssl_clientauth_init( const char *certdbpath, void *certdbhandle, 
    const int needkeydb, const char *keydbpath, void *keydbhandle )

{

    char		*certdbName, *keydbName, *s;
    static char         *secmodname =  "secmod.db";
    int			rc, len, use_default_db;
    SECStatus		rv;
    CERTCertDBHandle	*certdbh;
    SECKEYKeyDBHandle	*keydbh;
     
    /*
     *     LDAPDebug(LDAP_DEBUG_TRACE, "ldapssl_clientauth_init\n",0 ,0 ,0);
     */
    if ( inited ) {
	return( 0 );
    }

    ldapssl_basic_init();

    certdbh = (CERTCertDBHandle *)certdbhandle;
    if ( certdbh == NULL ) {
	/* XXXmcs: certdbh is leaked... */
	if (( certdbh = (CERTCertDBHandle *)ldapssl_calloc( 1,
		sizeof( CERTCertDBHandle ))) == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}
    } else {
	memset( certdbh, 0, sizeof( CERTCertDBHandle ));
    }

    use_default_db = ( certdbpath == NULL || *certdbpath == '\0' );
    if ( use_default_db ) {
	certdbName = NULL;
    } else if (( len = PL_strlen( certdbpath )) > 3
	    && PL_strcasecmp( ".db", certdbpath + len - 3 ) == 0 ) {
	certdbName = GetDBName(certdbpath, NULL);
    } else {
	certdbName = GetDBName("cert7.db", certdbpath);
    }

    /* Open the certificate database */
    rv = CERT_OpenCertDBFilename(certdbh, certdbName, PR_TRUE);
    if ( certdbName != NULL ) ldapssl_free((void **)&certdbName);
    if (rv != SECSuccess && !use_default_db) {
	/* fallback:  try to open cert5.db (version 5 database) */
	if (( certdbName = GetDBName("cert5.db", certdbpath)) != NULL ) {
	    rv = CERT_OpenCertDBFilename(certdbh, certdbName, PR_TRUE);
	    ldapssl_free((void **)&certdbName);
	}
    }

    if (rv != SECSuccess) {
	if (( rc = PR_GetError()) >= 0 ) {
	    rc = -1;
	}
	return( rc );
    }
    CERT_SetDefaultCertDB(certdbh);

    if (SSL_EnableDefault(SSL_ENABLE_SSL2, PR_FALSE)
	    || SSL_EnableDefault(SSL_ENABLE_SSL3, PR_TRUE)) {
	if (( rc = PR_GetError()) >= 0 ) {
	    rc = -1;
	}
	return( rc );
    }

    if ( needkeydb ) {
	/* XXXmcs: keydbh is leaked */
	if (( keydbh = (SECKEYKeyDBHandle *)keydbhandle ) == NULL ) {
	    if (( keydbh = (SECKEYKeyDBHandle *)ldapssl_calloc( 1,
		    sizeof( SECKEYKeyDBHandle ))) == NULL ) {
		rc = PR_GetError();
		return( rc == 0 ? -1 : rc );
	    }
	} else {
	    memset( keydbh, 0, sizeof( SECKEYKeyDBHandle ));
	}

	use_default_db = ( keydbpath == NULL || *keydbpath == '\0' );

	if ( use_default_db ) {
	    keydbName = NULL;
	} else if (( len = PL_strlen( keydbpath )) > 3
		&& PL_strcasecmp( ".db", keydbpath + len - 3 ) == 0 ) {
	    keydbName = GetDBName(keydbpath, NULL);
	} else {
	    keydbName = GetDBName("key3.db", keydbpath);
	}

	keydbh = SECKEY_OpenKeyDBFilename( keydbName, PR_TRUE );
	keydbName = NULL;	/* OpenKeyDBFilename() frees keydbName! */
	if ( keydbh == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}

	SECKEY_SetDefaultKeyDB( keydbh );
    }

    /*
     * SECMOD_init() should be done last (after key and cert dbs are open)
     */

    if (( len = PL_strlen( certdbpath )) > 3
	&& PL_strcasecmp( ".db", certdbpath + len - 3 ) == 0 ) {
	s = ldapssl_strdup(certdbpath);
	if (PL_strrchr( s, FILE_PATHSEP )) {
	    char *s2;
	    
	    *(PL_strrchr(s, FILE_PATHSEP)) = '\0';
	    s2 = buildDBName( s, secmodname);
	    SECMOD_init( s2 );
	    ldapssl_free((void **) &s2 );
	    ldapssl_free((void **) &s );
	}	
	else 
	{
	    SECMOD_init( secmodname );		
	}
	
    }
    else
    {
	s = buildDBName( certdbpath, secmodname);
	SECMOD_init( s );
	ldapssl_free((void **) &s );
    }



#if defined(NS_DOMESTIC)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#elif(NS_EXPORT)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#else
    return( -1 );
#endif

    inited = 1;

    return( 0 );

}


/* 
 * This is not the most elegant solution to SSL strength, but it
 * works because ldapssl_advclientauth_init() is only called once.
 */

int get_ssl_strength( void )
{
  return ssl_strength;
}

/* 
 * At some point we might want to consider protecting this 
 * with a mutex..  For now there is no need.
 */
int set_ssl_strength(int strength_val)
{

  if (strength_val == LDAPSSL_AUTH_WEAK ||
      strength_val == LDAPSSL_AUTH_CERT ||
      strength_val == LDAPSSL_AUTH_CNCHECK ) {
    ssl_strength = strength_val;
    return LDAP_SUCCESS;
  }
  return LDAP_PARAM_ERROR;

}



/*
 * Initialize ns/security so it can be used for SSL client authentication.
 * It is safe to call this more than once.
 *
 * If needkeydb == 0, no key database is opened and SSL server authentication
 * is supported but not client authentication.
 *
 * If "certdbpath" is NULL or "", the default cert. db is used (typically
 * ~/.netscape/cert7.db).
 *
 * If "certdbpath" ends with ".db" (case-insensitive compare), then
 * it is assumed to be a full path to the cert. db file; otherwise,
 * it is assumed to be a directory that contains a file called
 * "cert7.db" or "cert.db".
 *
 * If certdbhandle is non-NULL, it is assumed to be a pointer to a
 * SECCertDBHandle structure.  It is fine to pass NULL since this
 * routine will allocate one for you (CERT_GetDefaultDB() can be
 * used to retrieve the cert db handle).
 *
 * If "keydbpath" is NULL or "", the default key db is used (typically
 * ~/.netscape/key3.db).
 *
 * If "keydbpath" ends with ".db" (case-insensitive compare), then
 * it is assumed to be a full path to the key db file; otherwise,
 * it is assumed to be a directory that contains a file called
 * "key3.db" 
 *
 * If certdbhandle is non-NULL< it is assumed to be a pointed to a
 * SECKEYKeyDBHandle structure.  It is fine to pass NULL since this
 * routine will allocate one for you (SECKEY_GetDefaultDB() can be
 * used to retrieve the cert db handle).  */
int
LDAP_CALL
ldapssl_advclientauth_init( 
    const char *certdbpath, void *certdbhandle, 
    const int needkeydb, const char *keydbpath, void *keydbhandle,  
    const int needsecmoddb, const char *secmoddbpath,
    const int sslstrength )
{

    char		*certdbName, *keydbName, *secmoddbName, *s;
    static char         *secmodname =  "secmod.db";
    
    int			rc, len, use_default_db;
    SECStatus		rv;
    CERTCertDBHandle	*certdbh;
    SECKEYKeyDBHandle	*keydbh;


    if ( inited ) {
	return( 0 );
    }
    /*
     *    LDAPDebug(LDAP_DEBUG_TRACE, "ldapssl_advclientauth_init\n",0 ,0 ,0);
     */


    ldapssl_basic_init();

    certdbh = (CERTCertDBHandle *)certdbhandle;
    if ( certdbh == NULL ) {
	if (( certdbh = (CERTCertDBHandle *)ldapssl_calloc( 1,
		sizeof( CERTCertDBHandle ))) == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}
    } else {
	memset( certdbh, 0, sizeof( CERTCertDBHandle ));
    }
    certdbName = ldapssl_strdup( certdbpath );
    
    /* Open the certificate database */

    rv = CERT_OpenCertDB(certdbh, PR_TRUE, GetCertDBName, certdbName );
    if (rv)
	return( -1 );
    
    CERT_SetDefaultCertDB(certdbh);

    if (SSL_EnableDefault(SSL_ENABLE_SSL2, PR_FALSE)
	|| SSL_EnableDefault(SSL_ENABLE_SSL3, PR_TRUE)) {
	if (( rc = PR_GetError()) >= 0 ) {
	    rc = -1;
	}
	return( rc );
    }

    if ( needkeydb ) {
#if 0
	/*
	 * XXXmcs: this code seems to do nothing except leak memory...
	 * keydbh is reassigned below in the call to SECKEY_OpenKeyDBFilename().
	 */
	if (( keydbh = (SECKEYKeyDBHandle *)keydbhandle ) == NULL ) {
	    if (( keydbh = (SECKEYKeyDBHandle *)ldapssl_calloc( 1,
		    sizeof( SECKEYKeyDBHandle ))) == NULL ) {
		rc = PR_GetError();
		return( rc == 0 ? -1 : rc );
	    }
	} else {
	    memset( keydbh, 0, sizeof( SECKEYKeyDBHandle ));
	}
#endif /* 0 */

	use_default_db = ( keydbpath == NULL || *keydbpath == '\0' );

	if ( use_default_db ) {
	    keydbName = NULL;
	} else if (( len = PL_strlen( keydbpath )) > 3
		&& PL_strcasecmp( ".db", keydbpath + len - 3 ) == 0 ) {
	    keydbName = GetDBName(keydbpath, NULL);
	} else {
	    keydbName = GetDBName("key3.db", keydbpath);
	}

	keydbh = SECKEY_OpenKeyDBFilename( keydbName, PR_TRUE );
	keydbName = NULL;	/* OpenKeyDBFilename() frees keydbName! */
	if ( keydbh == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}

	SECKEY_SetDefaultKeyDB( keydbh );
    }

    /*
     * SECMOD_init() should be done last (after key and cert dbs are open)
     */
    if (needsecmoddb )
    {
	if ( secmoddbpath) {
	    
	    secmoddbName = buildDBName( secmoddbpath, secmodname);
	    SECMOD_init( secmoddbName);
	    ldapssl_free((void **) &secmoddbName );
	}
	else 
	    SECMOD_init( secmodname);
    }
    
    else {
	
	if (( len = PL_strlen( certdbName )) > 3
	    && PL_strcasecmp( ".db", certdbName + len - 3 ) == 0 ) {
	    s = ldapssl_strdup(certdbName);
	    if (PL_strrchr( s, FILE_PATHSEP )) {
		char *s2;
		
		*(PL_strrchr(s, FILE_PATHSEP)) = '\0';
		s2 = buildDBName( s, secmodname);
	
		SECMOD_init( s2 );
		ldapssl_free((void **) &s2 );
		ldapssl_free((void **) &s );
	    }	
	    else 
	    {
		SECMOD_init( secmodname );		
	    }

	}
	else
	{
	    s = buildDBName( certdbName, secmodname);
	    SECMOD_init( s );
	    ldapssl_free((void **) &s );
	}
    }
    

#if defined(NS_DOMESTIC)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#elif(NS_EXPORT)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#else
    return( -1 );
#endif

    inited = 1;
    
    set_ssl_strength( sslstrength );

    return( 0 );

}


/*
 * Initialize ns/security so it can be used for SSL client authentication.
 * It is safe to call this more than once.
  */

/* 
 * XXXceb  This is a hack until the new IO functions are done.
 * this function lives in ldapsinit.c
 */
void set_using_pkcs_functions( int val );

int
LDAP_CALL
ldapssl_pkcs_init( const struct ldapssl_pkcs_fns *pfns )
{

    char		*certdbName, *s, *keydbpath;
    static char         *secmodname =  "secmod.db";
    
    int			rc, len, use_default_db;
    SECStatus		rv;
    CERTCertDBHandle	*certdbh;
    SECKEYKeyDBHandle	*keydbh;
#if 0
    int			err;
#endif /* 0 */
    
    if ( inited ) {
	return( 0 );
    }
/* 
 * XXXceb  This is a hack until the new IO functions are done.
 * this function MUST be called before ldap_enable_clienauth.
 * 
 */
    set_using_pkcs_functions( 1 );
    
    /*
     *    LDAPDebug(LDAP_DEBUG_TRACE, "ldapssl_pkcs_init\n",0 ,0 ,0);
     */


    ldapssl_basic_init();

    if (( certdbh = (CERTCertDBHandle *)ldapssl_calloc( 1,
	sizeof( CERTCertDBHandle ))) == NULL ) {
      rc = PR_GetError();
      return( rc == 0 ? -1 : rc );
    }

    pfns->pkcs_getcertpath( NULL, &s);
    certdbName = ldapssl_strdup( s );
    

    
    /* Open the certificate database */

    rv = CERT_OpenCertDB(certdbh, PR_TRUE, GetCertDBName, certdbName );

    /* XXXceb SSL Err needed here */
    if (rv)
	return( -1 );
    
    CERT_SetDefaultCertDB(certdbh);

    pfns->pkcs_getkeypath( NULL, &s);
    keydbpath = ldapssl_strdup( s );
    {
	char	*keydbName;
#if 0
	/*
	 * XXXmcs: this code seems to do nothing except leak memory...
	 * keydbh is reassigned below in the call to SECKEY_OpenKeyDBFilename().
	 */
	if (( keydbh = (SECKEYKeyDBHandle *)ldapssl_calloc( 1,
            sizeof( SECKEYKeyDBHandle ))) == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}
	memset( keydbh, 0, sizeof( SECKEYKeyDBHandle ));
#endif /* 0 */

	use_default_db = ( keydbpath == NULL || *keydbpath == '\0' );
	
	if ( use_default_db ) {
	    keydbName = NULL;
	} else if (( len = PL_strlen( keydbpath )) > 3
		   && PL_strcasecmp( ".db", keydbpath + len - 3 ) == 0 ) {
	    keydbName = GetDBName(keydbpath, NULL);
	} else {
	    keydbName = GetDBName("key3.db", keydbpath);
	}
	
	keydbh = SECKEY_OpenKeyDBFilename( keydbName, PR_TRUE );
	keydbName = NULL;	/* OpenKeyDBFilename() frees keydbName! */
	if ( keydbh == NULL ) {
	    rc = PR_GetError();
	    return( rc == 0 ? -1 : rc );
	}

	SECKEY_SetDefaultKeyDB( keydbh );
    }
    ldapssl_free((void **) &keydbpath );

    /* this is odd */
    PK11_ConfigurePKCS11(NULL, NULL, tokDes, ptokDes, NULL, NULL, NULL, NULL, 0, 0 );

    /*
     * SECMOD_init() should be done last (after key and cert dbs are open)
     */
	
    if (( len = PL_strlen( certdbName )) > 3
	&& PL_strcasecmp( ".db", certdbName + len - 3 ) == 0 ) {
      s = ldapssl_strdup(certdbName);
      if (PL_strrchr( s, FILE_PATHSEP )) {
	char *s2;
	
	*(PL_strrchr(s, FILE_PATHSEP)) = '\0';
	s2 = buildDBName( s, secmodname);
	
	SECMOD_init( s2 );
	ldapssl_free((void **) &s2 );
	ldapssl_free((void **) &s );
      }	
      else 
	{
	  SECMOD_init( secmodname );		
	}
      
    }
    else
    {
	s = buildDBName( certdbName, secmodname);
	SECMOD_init( s );
	ldapssl_free((void **) &s );
    }

#if 0    
    /* Create and register the pin object for PKCS 11 */
    pfns->pkcs_getdonglefilename(NULL, &filename);

    pfns->pkcs_getpin(NULL, "", &pin);

#ifndef _WIN32
    
    if ( SVRCORE_CreateStdPinObj(&StdPinObj, filename, PR_TRUE) !=
	 SVRCORE_Success) {
	printf("Security Initialization: Unable to create PinObj "
	       "(%d)", PR_GetError());
	return -1;
    }
    if (pin != NULL)
    {

      pfns->pkcs_gettokenname(NULL, &tokenName);

      SVRCORE_CreateArgPinObj(&ArgPinObj, tokenName, pin, (SVRCOREPinObj *)StdPinObj);

      SVRCORE_RegisterPinObj((SVRCOREPinObj *)ArgPinObj);
    }
    else
    {
      SVRCORE_RegisterPinObj((SVRCOREPinObj *)StdPinObj);
    }
	
    
#else

    if (NULL != pin)
    {
	
	pfns->pkcs_gettokenname(NULL, &tokenName);

	if ((err = SVRCORE_CreateNTUserPinObj(&NTUserPinObj)) != SVRCORE_Success){
	    printf("Security Initialization: Unable to create NTUserPinObj "
		   "(%d)", PR_GetError());
	    return -1;
	}

	if ((err = SVRCORE_CreateArgPinObj(&ArgPinObj, tokenName, pin,
	    (SVRCOREPinObj *)NTUserPinObj)) != SVRCORE_Success)
	{
	    printf("Security Initialization: Unable to create ArgPinObj "
		   "(%d)", PR_GetError());
	    return -1;
	}

	SVRCORE_RegisterPinObj((SVRCOREPinObj *)ArgPinObj);

    }
    else
    {
	if ((err = SVRCORE_CreateNTUserPinObj(&NTUserPinObj)) != SVRCORE_Success){
	    printf("Security Initialization: Unable to create NTUserPinObj "
		   "(%d)", PR_GetError());
	    return -1;
	}
	if (filename && *filename)
	{
	if ((err = SVRCORE_CreateFilePinObj(&FilePinObj, filename)) !=
	    SVRCORE_Success) {
	    printf("Security Initialization: Unable to create FilePinObj "
		   "(%d)", PR_GetError());
	    return -1;
	    
	}
	if ((err = SVRCORE_CreateAltPinObj(&AltPinObj, (SVRCOREPinObj *)FilePinObj,
					  (SVRCOREPinObj *)NTUserPinObj)) != SVRCORE_Success) {
	    printf("Security Initialization: Unable to create AltPinObj "
		   "(%d)", PR_GetError());
	    return -1;
	}
	SVRCORE_RegisterPinObj((SVRCOREPinObj *)AltPinObj);
	}
	else
	{
		SVRCORE_RegisterPinObj((SVRCOREPinObj *)NTUserPinObj);
	}
    }
#endif

#endif

/*
 *    if (filename) free(filename);
 */

    if (SSL_EnableDefault(SSL_ENABLE_SSL2, PR_FALSE)
	|| SSL_EnableDefault(SSL_ENABLE_SSL3, PR_TRUE)) {
	if (( rc = PR_GetError()) >= 0 ) {
	    rc = -1;
	}
	
	return( rc );
    }
    
#if defined(NS_DOMESTIC)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#elif(NS_EXPORT)
    if (local_SSLPLCY_Install() == PR_FAILURE)
      return( -1 );
#else
    return( -1 );
#endif

    inited = 1;

    if ( certdbName != NULL ) {
	ldapssl_free((void **) &certdbName );
    }
    
    /*
    set_ssl_strength( sslstrength );
    */

    set_ssl_strength( LDAPSSL_AUTH_CERT );
    return( 0 );


#if 0
    { 
      char *man= PORT_Strdup(MyInternationalizedString(SEC_PK11_MANUFACTURER)); 
      char *libdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_LIBARARY)); 
      char *tokdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_TOKEN)); 
      char *ptokdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_PRIV_TOKEN)); 
      char *slotdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_SLOT)); 
      char *pslotdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_PRIV_SLOT)); 
      char *fslotdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_FIPS_SLOT)); 
      char *fpslotdes = PORT_Strdup(MyInternationalizedString(SEC_PK11_FIPS_PRIV_SLOT)); 
      
      PK11_ConfigurePKCS11(man,libdes,tokdes,ptokdes,slotdes,pslotdes,fslotdes,fpslotdes); 
    } 

    /* 
     * set our default password function 
     */ 
    PK11_SetPasswordFunc(MyPasswordFunc); 
#endif

}


/*
 * ldapssl_client_init() is a server-authentication only version of
 * ldapssl_clientauth_init().
 */
int
LDAP_CALL
ldapssl_client_init(const char* certdbpath, void *certdbhandle )
{
    return( ldapssl_clientauth_init( certdbpath, certdbhandle,
	    0, NULL, NULL ));
}
#endif /* NET_SSL */
