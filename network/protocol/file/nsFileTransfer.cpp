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

#include "nsFileTransfer.h"
#include "mkfile.h"

static NS_DEFINE_IID(kITransferIID, NS_ITRANSFER_IID);

nsFileTransfer::nsFileTransfer(ActiveEntry* entry)
    : nsSimpleTransfer(entry->URL_s->address), fEntry(entry)
{
}


PRUint32
nsFileTransfer::GetTimeRemainingMSec(void)
{
    switch (fState) {
    case Start:
        return REMAINING_TIME_UNKNOWN;

    case Running:
        return GetFileTimeRemainingMSec();

    case Complete:
    case Error:
        return 0;
    }

    PR_ASSERT(0);
    return REMAINING_TIME_UNKNOWN;
}


void
nsFileTransfer::DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback)
{
}


void
nsFileTransfer::SetState(State state)
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
};



PRUint32
nsFileTransfer::GetFileTimeRemainingMSec(void)
{
    PR_ASSERT(fEntry);
    if (! fEntry)
        return 0;

    PR_ASSERT(fEntry->con_data);
    if (! fEntry->con_data)
        return 0;

    FILEConData* cd = (FILEConData*) fEntry->con_data;

    switch (cd->next_state) {
	case NET_CHECK_FILE_TYPE:
        return 500;

	case NET_DELETE_FILE:
        return 500;

	case NET_MOVE_FILE:
        return 500;

	case NET_PUT_FILE:
        return 500;

	case NET_MAKE_DIRECTORY:
        return 500;

	case NET_FILE_SETUP_STREAM:
        return 250 + 500;

	case NET_OPEN_FILE:
        return 250 + 500;

	case NET_SETUP_FILE_STREAM:
        return 250 + 500;

	case NET_OPEN_DIRECTORY:
	case NET_READ_FILE_CHUNK:
	case NET_READ_DIRECTORY_CHUNK:
        return 250;

	case NET_BEGIN_PRINT_DIRECTORY:
	case NET_PRINT_DIRECTORY:
        // XXX
        return 0;

	case NET_FILE_DONE:
	case NET_FILE_ERROR_DONE:
        return 0;

    case NET_FILE_FREE:
        // Shouldn't ever get here.
        break;
    }

    PR_ASSERT(0);
    return REMAINING_TIME_UNKNOWN;
}
