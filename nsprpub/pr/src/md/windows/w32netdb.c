/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  Garrett Arch Blythe, 02/07/2002
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * w32netdb.c
 *
 * This file exists mainly to provide an implementation of the socket
 *  functions that are missing from certain toolsets; namely
 *  MS eMbedded Visual Tools (Windows CE).
 */

#include "primpl.h"

/*
 * Our static protocols entries.
 */
static struct protoent sProtos[] = {
    { "tcp",    NULL,   IPPROTO_TCP },
    { "udp",    NULL,   IPPROTO_UDP },
    { "ip",     NULL,   IPPROTO_IP },
    { "icmp",   NULL,   IPPROTO_ICMP },
    { "ggp",    NULL,   IPPROTO_GGP },
    { "pup",    NULL,   IPPROTO_PUP },
    { "idp",    NULL,   IPPROTO_IDP },
    { "nd",     NULL,   IPPROTO_ND },
    { "raw",    NULL,   IPPROTO_RAW }
};

#define MAX_PROTOS (sizeof(sProtos) / sizeof(struct protoent))

/*
 * Wingetprotobyname
 *
 * As getprotobyname
 */
struct protoent* Wingetprotobyname(const char* inName)
{
    struct protoent* retval = NULL;

    if(NULL != inName)
    {
        unsigned uLoop;

        for(uLoop = 0; uLoop < MAX_PROTOS; uLoop++)
        {
            if(0 == stricmp(inName, sProtos[uLoop].p_name))
            {
                retval = &sProtos[uLoop];
                break;
            }
        }
    }

    return retval;
}

/*
 * Wingetprotobynumber
 *
 * As getprotobynumber
 */
struct protoent* Wingetprotobynumber(int inNumber)
{
    struct protoent* retval = NULL;
    unsigned uLoop;
    
    for(uLoop = 0; uLoop < MAX_PROTOS; uLoop++)
    {
        if(inNumber == sProtos[uLoop].p_proto)
        {
            retval = &sProtos[uLoop];
            break;
        }
    }

    return retval;
}
