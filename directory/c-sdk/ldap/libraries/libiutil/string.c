/* -*- C -*-
 *
 * Copyright (c) 1998-1999 Innosoft International, Inc.  All Rights Reserved.
 *
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Innosoft International, 
 * Inc. or a written license from an organization licensed by Innosoft
 * International, Inc. to grant such a license.
 *
 *
 * Copyright (c) 1996-1997 Critical Angle Inc. All Rights Reserved.
 * 
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Critical Angle Inc.,
 * or a written license from an organization licensed by Critical Angle
 * Inc. to grant such a license.
 *
 */

/*
 * $RCSfile$ $Revision$ $Date$ $State$
 *
 * $Log$
 * Revision 1.15  1999/07/06 17:52:08  ac
 * fix ldap_utf8_strcasecmp
 *
 * Revision 1.14  1999/05/11 22:30:08  ac
 * optimization of ldap_utf8_strcasecmp for ASCII
 *
 * Revision 1.13  1999/05/11 18:22:53  Administrator
 * add utf-8 aware strcasecmp
 *
 * Revision 1.12  1999/03/22 23:58:21  Administrator
 * update Copyright statements in ILC-SDK
 *
 * Revision 1.11  1998/10/06 05:27:27  kvc
 * A98100413A changes to support VMS V6
 *
 * Revision 1.10  1998/08/12 20:51:24  kvc
 * VMS port
 *
 * Revision 1.9  1998/06/21 22:08:43  wahl
 * change Debug to ldap_log_message
 *
 * Revision 1.8  1998/06/12 19:43:04  wahl
 * use ch_malloc in ldap_strdup
 *
 * Revision 1.7  1998/04/10 20:32:02  Administrator
 * add ldap_strdup, primarily for Win32
 *
 * Revision 1.6  1998/01/15 02:18:43  nr
 * add utf8 processing functions
 *
 * Revision 1.5  1997/10/25 17:20:51  wahl
 * fix RCS header
 *
 * Revision 1.4  1997/09/09 00:28:54  wahl
 * NeXTstep port
 *
 * Revision 1.3  1997/09/07 18:32:39  wahl
 * additional include file simplification
 *
 * Revision 1.2  1997/09/07 07:00:05  wahl
 * include file simplification
 *
 * Revision 1.1  1997/07/31 06:06:26  wahl
 * prepare for API draft update
 *
 *
 *
 */

#include "ldap-int.h"

char *ldap_strdup( const char *s )
{
	char	*p;
	int l;

	if (s == NULL) return NULL;
	
	l = strlen(s);

	if ( (p = (char *) ldap_x_malloc( l + 1 )) == NULL )
		return( NULL );

	memcpy(p,s,l);
	p[l] = '\0';

	return( p );
}

