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

#ifndef nsITransfer_h__
#define nsITransfer_h__

#include "nsISupports.h"
#include "prtypes.h"
#include "net.h" // for URL_Struct

// {791eafa1-29e6-11d1-8031-006008159b5a}
#define NS_ITRANSFER_IID \
{0x791eafa1, 0x29e6, 0x11d1,  \
    {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}


#define REMAINING_TIME_UNKNOWN ((PRUint32) -1)

typedef void (*nsITransferDisplayStatusFunc)(void* data, char* message);

enum TransferState {
    TransferState_Start,
    TransferState_Running,
    TransferState_Complete,
    TransferState_Error
};

class nsITransfer : public nsISupports
{
public:
    /**
     * Return the transfer's object
     */
    virtual const char*
    GetURL(void) = 0;

    /**
     * Return the transfer's state
     */
    virtual TransferState
    GetState(void) = 0;

    /**
     * Return an estimate of the remaining time (in msec)
     */
    virtual PRUint32
    GetTimeRemainingMSec(void) = 0;

    /**
     * Display a detailed, user consumable status message using
     * the specified callback routine.
     */
    virtual void
    DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback) = 0;
};


#endif /* nsITransfer_h__ */
