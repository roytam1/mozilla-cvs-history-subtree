/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsHTTPTransfer.h"
#include "mkhttp.h"

nsHTTPTransfer::nsHTTPTransfer(ActiveEntry* entry)
    : nsSimpleTransfer(entry->URL_s->address), fEntry(entry)
{
}


PRUint32
nsHTTPTransfer::GetTimeRemainingMSec(void)
{
    switch (fState) {
    case Start:
        return REMAINING_TIME_UNKNOWN;

    case Running:
        return GetHTTPTimeRemainingMSec();

    case Complete:
    case Error:
        return 0;
    }

    PR_ASSERT(0);
    return REMAINING_TIME_UNKNOWN;
}


void
nsHTTPTransfer::DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback)
{
}


void
nsHTTPTransfer::SetState(State state)
{
    switch (state) {
    case Complete:
    case Error:
        // The ActiveEntry struct is no longer valid...
        fEntry = NULL;
        break;

    default:
        break;
    }
    nsSimpleTransfer::SetState(state);
}


PRUint32
nsHTTPTransfer::GetHTTPTimeRemainingMSec(void)
{
    PR_ASSERT(fEntry);
    if (! fEntry)
        return 0;

    PR_ASSERT(fEntry->con_data);
    if (! fEntry->con_data)
        return 0;

    HTTPConData* cd = (HTTPConData*) fEntry->con_data;

    switch (cd->next_state) {
    case HTTP_START_CONNECT:
    case HTTP_FINISH_CONNECT:
        return 250 + 500 + 1000;

    case HTTP_SEND_PROXY_TUNNEL_REQUEST:
    case HTTP_BEGIN_UPLOAD_FILE:
        return 500 + 1000;

    case HTTP_SEND_REQUEST:
        return 500 + 1000;

    case HTTP_SEND_POST_DATA:
        return 1000 + 1000;

    case HTTP_PARSE_FIRST_LINE:
        return 1000;

    case HTTP_PARSE_MIME_HEADERS:
        return 1000;

    case HTTP_SETUP_STREAM:
        return 1000;

    case HTTP_BEGIN_PUSH_PARTIAL_CACHE_FILE:
    case HTTP_PUSH_PARTIAL_CACHE_FILE:
        return 250;

    case HTTP_PULL_DATA:
        // XXX look at how much data is left to pull, how long the
        // pull has been going, etc. to compute a reasonable value.
        return 1000;

    case HTTP_DONE:
    case HTTP_ERROR_DONE:
        return 0;

    case HTTP_FREE:
        // shouldn't ever get here.
        break;
    }

    PR_ASSERT(0);
    return REMAINING_TIME_UNKNOWN;
}
