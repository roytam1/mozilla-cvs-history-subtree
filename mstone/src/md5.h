/*
 * Trivial modifications by Sean O'Rourke, Feb 2000, for inclusion in
 * Mailstone.
 */

/*
 * Copyright 1998,1999 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted only
 * as authorized by the OpenLDAP Public License.  A copy of this
 * license is available at http://www.OpenLDAP.org/license.html or
 * in file LICENSE in the top-level directory of the distribution.
 */

/* See md5.c for explanation and copyright information.  */

#ifndef _LUTIL_MD5_H_
#define _LUTIL_MD5_H_

/* Unlike previous versions of this code, uint32 need not be exactly
   32 bits, merely 32 bits or more.  Choosing a data type which is 32
   bits instead of 64 is not important; speed is considerably more
   important.  ANSI guarantees that "unsigned long" will be big enough,
   and always using it seems to have few disadvantages.  */

typedef unsigned long uint32;

struct lutil_MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void lutil_MD5Init(struct lutil_MD5Context *context);

void lutil_MD5Update(struct lutil_MD5Context *context,
		     unsigned char const *buf,
		     unsigned len);

void lutil_MD5Final(unsigned char digest[16],
		    struct lutil_MD5Context *context);

void lutil_MD5Transform(uint32 buf[4],
			const unsigned char in[64]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct lutil_MD5Context lutil_MD5_CTX;

#endif /* _LUTIL_MD5_H_ */
