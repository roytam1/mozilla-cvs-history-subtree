/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Gagan Saksena <gagan@netscape.com> (original author)
 *   Darin Fisher <darin@netscape.com>
 */

#ifndef nsHttp_h__
#define nsHttp_h__

#include "nsError.h"
#include "plstr.h"
#include "prlog.h"
#include "nsAString.h"

#if defined(PR_LOGGING)
//
// Log module for HTTP Protocol logging...
//
// To enable logging (see prlog.h for full details):
//
//    set NSPR_LOG_MODULES=nsHttp2:5
//    set NSPR_LOG_FILE=http.log
//
// this enables PR_LOG_ALWAYS level information and places all output in
// the file http.log
//
extern PRLogModuleInfo *gHttpLog;
#endif
#define LOG(args) PR_LOG(gHttpLog, PR_LOG_DEBUG, args)

#define NS_HTTP_SEGMENT_SIZE 2048
#define NS_HTTP_BUFFER_SIZE  2048*4

enum nsHttpVersion {
    HTTP_VERSION_UNKNOWN,
    HTTP_VERSION_0_9,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1
};

//-----------------------------------------------------------------------------
// http atoms...
//-----------------------------------------------------------------------------

struct nsHttpAtom
{
    operator const char *() { return _val; }
    const char *get() { return _val; }

    // don't access this directly
    const char *_val;
};

struct nsHttp
{
    static void DestroyAtomTable();

    static nsHttpAtom ResolveAtom(const nsACString &);

    /* Declare all atoms
     *
     * The atom names and values are stored in nsHttpAtomList.h and
     * are brought to you by the magic of C preprocessing
     *
     * Add new atoms to nsHttpAtomList and all support logic will be auto-generated
     */
#define HTTP_ATOM(_name, _value) static nsHttpAtom _name;
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
};

//-----------------------------------------------------------------------------
// utilities...
//-----------------------------------------------------------------------------

static inline nsresult
DupString(const char *src, char **dst)
{
    *dst = PL_strdup(src);
    return *dst ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

#endif
