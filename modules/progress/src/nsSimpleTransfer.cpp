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

#include "nsSimpleTransfer.h"
#include "plstr.h"

static NS_DEFINE_IID(kITransferIID, NS_ITRANSFER_IID);

nsSimpleTransfer::nsSimpleTransfer(const char* url)
    : fState(TransferState_Start), fURL(NULL), fStart(PR_Now())
{
    NS_INIT_REFCNT();

    PR_ASSERT(url);
    if (url)
        fURL = PL_strdup(url);
}


nsSimpleTransfer::~nsSimpleTransfer(void)
{
    if (fURL) {
        PL_strfree(fURL);
        fURL = NULL;
    }
}


NS_IMPL_ISUPPORTS(nsSimpleTransfer, kITransferIID);


const char*
nsSimpleTransfer::GetURL(void)
{
    return fURL;
}


TransferState
nsSimpleTransfer::GetState(void)
{
    return fState;
}



void
nsSimpleTransfer::SetState(TransferState state)
{
    fState = state;
};


PRUint32
nsSimpleTransfer::GetElapsedTimeMSec(void)
{
    // Lovely 64-bit arithmetic. Somebody should write a C++ package
    // for dealing with PRTimes.
    PRTime now = PR_Now();
    PRTime elapsedInUSec;
    LL_SUB(elapsedInUSec, now, fStart);

    PRTime factor;
    LL_UI2L(factor, PR_USEC_PER_MSEC);

    PRTime elapsedInMSec;
    LL_DIV(elapsedInMSec, elapsedInUSec, factor);

    PRUint32 result;
    LL_L2UI(result, elapsedInMSec);

    return result;
}
