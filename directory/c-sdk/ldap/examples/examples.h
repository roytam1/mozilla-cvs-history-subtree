/*
 * Copyright (c) 1996.  Netscape Communications Corporation.  All
 * rights reserved.
 * 
 * Common definitions for ldap example programs.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ldap.h>

/*
 * Host name of LDAP server
 */
#define MY_HOST		"localhost"

/*
 * Port number where LDAP server is running
 */
#define	MY_PORT		LDAP_PORT

/*
 * Port number where LDAPS server is running
 */
#define	MY_SSL_PORT		LDAPS_PORT

/*
 * DN of directory manager entry.  This entry should have write access to
 * the entire directory.
 */
#define MGR_DN		"cn=Directory Manager"

/*
 * Password for manager DN.
 */
#define MGR_PW		"secret99"

/*
 * Subtree to search
 */
#define	MY_SEARCHBASE	"o=Airius.com"

/*
 * Place where people entries are stored
 */
#define PEOPLE_BASE	"ou=People, " MY_SEARCHBASE

/*
 * DN of a user entry.  This entry does not need any special access to the
 * directory (it is not used to perform modifies, for example).
 */
#define USER_DN		"uid=scarter, " PEOPLE_BASE

/*
 * Password of the user entry.
 */
#define USER_PW		"sprain"

/* 
 * Filter to use when searching.  This one searches for all entries with the
 * surname (last name) of "Jensen".
 */
#define	MY_FILTER	"(sn=Jensen)"

/*
 * Entry to retrieve
 */
#define ENTRYDN "uid=bjensen, " PEOPLE_BASE

/*
 * Password for Babs' entry
 */
#define ENTRYPW "hifalutin"

/*
 * Name of file containing filters
 */
#define MY_FILTERFILE   "xmplflt.conf"
 
/*
 * Tag to use when retrieveing filters
 */
#define MY_FILTERTAG    "ldap-example"

