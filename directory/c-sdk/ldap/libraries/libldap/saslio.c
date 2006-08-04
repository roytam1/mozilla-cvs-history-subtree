/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Sun LDAP C SDK.
 *
 * The Initial Developer of the Original Code is Sun Microsystems, Inc.
 *
 * Portions created by Sun Microsystems, Inc are Copyright (C) 2005
 * Sun Microsystems, Inc. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef LDAP_SASLIO_HOOKS
#include <assert.h>
#include "ldap-int.h"
#include "../liblber/lber-int.h"
#include <sasl.h>
/* Should be pulled in from lber-int.h */
#define READBUFSIZ	8192

#define SEARCH_TIMEOUT_SECS	120
#define NSLDAPI_SM_BUF	128

/*
 * SASL Dependent routines
 *
 * SASL security and integrity options are supported through the
 * use of the extended I/O functionality.  Because the extended
 * I/O functions may already be in use prior to enabling encryption,
 * when SASL encryption si enabled, these routine interpose themselves
 * over the exitng extended I/O routines and add an additional level
 * of indirection.
 *  IE: Before SASL:  client->libldap->lber->extio
 *      After  SASL:  client->libldap->lber->saslio->extio
 * Any extio function are stilled used for the raw i/O [IE prldap]
 * but SASL will decrypt before passing to lber.
 * SASL cannot decrypt a stream so full packaets must be read
 * before proceeding.
 */

/*
 * Get the 4 octet header [size] for a sasl encrypted buffer.
 * See RFC222 [section 3].
 */
static int
nsldapi_sasl_pktlen( char *buf, int maxbufsize )
{
        int     size;

        size = ntohl(*(uint32_t *)buf);

        if ( size < 0 || size > maxbufsize ) {
                return (-1 );
        }

        return( size + 4 ); /* include the first 4 bytes */
}

/*
 * SASL encryption routines
 */

static int
nsldapi_sasl_read( int s, void *buf, int  len,
	struct lextiof_socket_private *arg)
{
	Sockbuf		*sb = (Sockbuf *)arg;
	LDAP		*ld;
	const char	*dbuf;
	char		*cp;
	int		ret;
	unsigned	dlen, blen;
   
	ld = (LDAP *)sb->sb_sasl_prld;

	/* Is there anything left in the existing buffer? */
	if ((ret = sb->sb_sasl_ilen) > 0) {
		ret = (ret > len ? len : ret);
		SAFEMEMCPY( buf, sb->sb_sasl_iptr, ret );
		if (ret == sb->sb_sasl_ilen) {
			sb->sb_sasl_ilen = 0;
			sb->sb_sasl_iptr = NULL;
		} else {
			sb->sb_sasl_ilen -= ret;
			sb->sb_sasl_iptr += ret;
		}
		return( ret );
	}

	/* buffer is empty - fill it */
	cp = sb->sb_sasl_ibuf;
	dlen = 0;
	
	/* Read the length of the packet */
	while ( dlen < 4 ) {
		if (sb->sb_sasl_fns.lbextiofn_read != NULL) {
			ret = sb->sb_sasl_fns.lbextiofn_read(
				s, cp, 4 - dlen,
				sb->sb_sasl_fns.lbextiofn_socket_arg);
		} else {
			ret = read( sb->sb_sd, cp, 4 - dlen );
		}
#ifdef EINTR
		if ( ( ret < 0 ) && ( LDAP_GET_ERRNO(ld) == EINTR ) )
			continue;
#endif
		if ( ret <= 0 )
			return( ret );

		cp += ret;
		dlen += ret;
	}

	blen = 4;

	ret = nsldapi_sasl_pktlen( sb->sb_sasl_ibuf, sb->sb_sasl_bfsz );
	if (ret < 0) {
		LDAP_SET_ERRNO(ld, EIO);
		return( -1 );
	}
	dlen = ret - dlen;

	/* read the rest of the encrypted packet */
	while ( dlen > 0 ) {
		if (sb->sb_sasl_fns.lbextiofn_read != NULL) {
			ret = sb->sb_sasl_fns.lbextiofn_read(
				s, cp, dlen,
				sb->sb_sasl_fns.lbextiofn_socket_arg);
		} else {
			ret = read( sb->sb_sd, cp, dlen );
		}

#ifdef EINTR
		if ( ( ret < 0 ) && ( LDAP_GET_ERRNO(ld) == EINTR ) )
			continue;
#endif
		if ( ret <= 0 )
			return( ret );

		cp += ret;
		blen += ret;
		dlen -= ret;
   	}

	/* Decode the packet */
	ret = sasl_decode( sb->sb_sasl_ctx,
			   sb->sb_sasl_ibuf, blen,
			   &dbuf, &dlen);
	if ( ret != SASL_OK ) {
		/* sb_sasl_read: failed to decode packet, drop it, error */
		sb->sb_sasl_iptr = NULL;
		sb->sb_sasl_ilen = 0;
		LDAP_SET_ERRNO(ld, EIO);
		return( -1 );
	}
	
	/* copy decrypted packet to the input buffer */
	SAFEMEMCPY( sb->sb_sasl_ibuf, dbuf, dlen );
	sb->sb_sasl_iptr = sb->sb_sasl_ibuf;
	sb->sb_sasl_ilen = dlen;

	ret = (dlen > (unsigned) len ? len : dlen);
	SAFEMEMCPY( buf, sb->sb_sasl_iptr, ret );
	if (ret == sb->sb_sasl_ilen) {
		sb->sb_sasl_ilen = 0;
		sb->sb_sasl_iptr = NULL;
	} else {
		sb->sb_sasl_ilen -= ret;
		sb->sb_sasl_iptr += ret;
	}
	return( ret );
}

static int
nsldapi_sasl_write( int s, const void *buf, int  len,
	struct lextiof_socket_private *arg)
{
	Sockbuf		*sb = (Sockbuf *)arg;
	int			ret = 0;
	const char	*obuf, *optr, *cbuf = (const char *)buf;
	unsigned	olen, clen, tlen = 0;
	unsigned	*maxbuf; 
	
	ret = sasl_getprop(sb->sb_sasl_ctx, SASL_MAXOUTBUF,
					   (const void **)&maxbuf);
	if ( ret != SASL_OK ) {
		/* just a sanity check, should never happen */
		return( -1 );
	}
	
	while (len > 0) {
		clen = (len > *maxbuf) ? *maxbuf : len;
		/* encode the next packet. */
		ret = sasl_encode( sb->sb_sasl_ctx, cbuf, clen, &obuf, &olen);
		if ( ret != SASL_OK ) {
			/* XXX Log error? "sb_sasl_write: failed to encode packet..." */
			return( -1 );
		}
		/* Write everything now, buffer is only good until next sasl_encode */
		optr = obuf;
		while (olen > 0) {
			if (sb->sb_sasl_fns.lbextiofn_write != NULL) {
				ret = sb->sb_sasl_fns.lbextiofn_write(
					s, optr, olen,
					sb->sb_sasl_fns.lbextiofn_socket_arg);
			} else {
				ret = write( sb->sb_sd, optr, olen);
			}
			if ( ret < 0 )
				return( ret );
			optr += ret;
			olen -= ret;
		}
		len -= clen;
		cbuf += clen;
		tlen += clen;
	}
	return( tlen );
}

static int
nsldapi_sasl_poll(
	LDAP_X_PollFD fds[], int nfds, int timeout,
	struct lextiof_session_private *arg ) 
{
	Sockbuf		*sb = (Sockbuf *)arg;
	LDAP		*ld;
	int		i;

	if (sb == NULL) {
		return( -1 );
	}
	ld = (LDAP *)sb->sb_sasl_prld;

	if (fds && nfds > 0) {
		for(i = 0; i < nfds; i++) {
			if (fds[i].lpoll_socketarg ==
			     (struct lextiof_socket_private *)sb) {
				fds[i].lpoll_socketarg =
					(struct lextiof_socket_private *)
					sb->sb_sasl_fns.lbextiofn_socket_arg;
			}

		}
	}
	return ( ld->ld_sasl_io_fns.lextiof_poll( fds, nfds, timeout,
		(void *)ld->ld_sasl_io_fns.lextiof_session_arg) );
}

int
nsldapi_sasl_open( LDAP *ld, Sockbuf *sb, const char * host, sasl_ssf_t ssf )
{
        int saslrc;
        sasl_conn_t *ctx;

        if ( host == NULL ) {
                LDAP_SET_LDERRNO( ld, LDAP_LOCAL_ERROR, NULL, NULL );
                return( LDAP_LOCAL_ERROR );
        }

        saslrc = sasl_client_new( "ldap", host,
                NULL, NULL, /* iplocalport, ipremoteport - use defaults */
                NULL, 0, &ctx );

        if ( saslrc != SASL_OK ) {
                return( nsldapi_sasl_cvterrno( ld, saslrc, NULL ) );
        }

        sb->sb_sasl_ctx = (void *)ctx;

        if( ssf ) {
                sasl_ssf_t extprops;
                memset(&extprops, 0L, sizeof(extprops));
                extprops = ssf;

                (void) sasl_setprop( ctx, SASL_SSF_EXTERNAL,
                        (void *) &extprops );
        }

        return( LDAP_SUCCESS );
}

static int
nsldapi_sasl_close( LDAP *ld, Sockbuf *sb )
{
	sasl_conn_t	*ctx = (sasl_conn_t *)sb->sb_sasl_ctx;
	
	if( ctx != NULL ) {
		sasl_dispose( &ctx );
		sb->sb_sasl_ctx = NULL;
	}
	
	return( LDAP_SUCCESS );
}

static int
nsldapi_sasl_close_socket(int s, struct lextiof_socket_private *arg ) 
{
	Sockbuf		*sb = (Sockbuf *)arg;
	LDAP		*ld;

	if (sb == NULL) {
		return( -1 );
	}
	ld = (LDAP *)sb->sb_sasl_prld;
	/* undo function pointer interposing */
	ldap_set_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS, &ld->ld_sasl_io_fns );
	ber_sockbuf_set_option( sb,
			LBER_SOCKBUF_OPT_EXT_IO_FNS,
			(void *)&sb->sb_sasl_fns);

	/* undo SASL */
	nsldapi_sasl_close( ld, sb );

	return ( ld->ld_sasl_io_fns.lextiof_close( s,
		(struct lextiof_socket_private *)
		sb->sb_sasl_fns.lbextiofn_socket_arg ) );
}

/*
 * install encryption routines if security has been negotiated
 */
int
nsldapi_sasl_install( LDAP *ld, Sockbuf *sb, void *ctx_arg )
{
        struct lber_x_ext_io_fns        fns;
        struct ldap_x_ext_io_fns        iofns;
        sasl_security_properties_t      *secprops;
        int     rc, value;
        int     bufsiz;

        rc = ber_sockbuf_get_option( sb,
                        LBER_SOCKBUF_OPT_TO_FILE_ONLY,
                        (void *) &value);
        if (rc != 0 || value != 0)
                return( LDAP_LOCAL_ERROR );

        /* initialize input buffer - use MAX SIZE to avoid reallocs */
        sb->sb_sasl_ctx = (sasl_conn_t *)ctx_arg;
        rc = sasl_getprop( sb->sb_sasl_ctx, SASL_SEC_PROPS,
                           (const void **)&secprops );
        if (rc != SASL_OK)
                return( LDAP_LOCAL_ERROR );
        bufsiz = secprops->maxbufsize;
        if (bufsiz <= 0) {
                return( LDAP_LOCAL_ERROR );
        }
        if ((sb->sb_sasl_ibuf = NSLDAPI_MALLOC(bufsiz)) == NULL) {
                return( LDAP_LOCAL_ERROR );
        }
        sb->sb_sasl_iptr = NULL;
        sb->sb_sasl_bfsz = bufsiz;
        sb->sb_sasl_ilen = 0;

        /* Reset Session then Socket Args */
        /* Get old values */
        (void) memset( &sb->sb_sasl_fns, 0, LBER_X_EXTIO_FNS_SIZE);
        sb->sb_sasl_fns.lbextiofn_size = LBER_X_EXTIO_FNS_SIZE;
        rc = ber_sockbuf_get_option( sb,
                        LBER_SOCKBUF_OPT_EXT_IO_FNS,
                        (void *)&sb->sb_sasl_fns);
        memset( &ld->ld_sasl_io_fns, 0, sizeof(iofns));
        ld->ld_sasl_io_fns.lextiof_size = LDAP_X_EXTIO_FNS_SIZE;
        rc = ldap_get_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS,
                              &ld->ld_sasl_io_fns );
        if (rc != 0 )
                return( LDAP_LOCAL_ERROR );

        /* Set new values */
        if (  ld->ld_sasl_io_fns.lextiof_read != NULL ||
              ld->ld_sasl_io_fns.lextiof_write != NULL ||
              ld->ld_sasl_io_fns.lextiof_poll != NULL ||
              ld->ld_sasl_io_fns.lextiof_connect != NULL ||
              ld->ld_sasl_io_fns.lextiof_close != NULL ) {
                memset( &iofns, 0, sizeof(iofns));
                iofns.lextiof_size = LDAP_X_EXTIO_FNS_SIZE;
                iofns.lextiof_read = nsldapi_sasl_read;
                iofns.lextiof_write = nsldapi_sasl_write;
                iofns.lextiof_poll = nsldapi_sasl_poll;
                iofns.lextiof_connect = ld->ld_sasl_io_fns.lextiof_connect;
                iofns.lextiof_close = nsldapi_sasl_close_socket;
                iofns.lextiof_newhandle = ld->ld_sasl_io_fns.lextiof_newhandle;
                iofns.lextiof_disposehandle =
                                ld->ld_sasl_io_fns.lextiof_disposehandle;
                iofns.lextiof_session_arg =
                                (void *) sb;
                                /* ld->ld_sasl_io_fns.lextiof_session_arg; */
                rc = ldap_set_option( ld, LDAP_X_OPT_EXTIO_FN_PTRS,
                              &iofns );
                if (rc != 0 )
                        return( LDAP_LOCAL_ERROR );
                sb->sb_sasl_prld = (void *)ld;
        }

        (void) memset( &fns, 0, LBER_X_EXTIO_FNS_SIZE);
        fns.lbextiofn_size = LBER_X_EXTIO_FNS_SIZE;
        fns.lbextiofn_read = nsldapi_sasl_read;
        fns.lbextiofn_write = nsldapi_sasl_write;
        fns.lbextiofn_socket_arg =
                        (void *) sb;
                        /* (void *)sb->sb_sasl_fns.lbextiofn_socket_arg; */
        rc = ber_sockbuf_set_option( sb,
                        LBER_SOCKBUF_OPT_EXT_IO_FNS,
                        (void *)&fns);
        if (rc != 0)
                return( LDAP_LOCAL_ERROR );

        return( LDAP_SUCCESS );
}

#endif
