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

#ifndef nsSimpleTransfer_h__
#define nsSimpleTransfer_h__

#include "nsITransfer.h"

class nsSimpleTransfer : public nsITransfer
{
public:
    nsSimpleTransfer(const char* url);
    virtual ~nsSimpleTransfer(void);

    NS_DECL_ISUPPORTS

    // The nsITransfer interface

    /**
     * Return the transfer's object
     */
    virtual const char*
    GetURL(void);

    /**
     * Return the transfer's state
     */
    virtual TransferState
    GetState(void);

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


    // Other public methods
    virtual void
    SetState(TransferState state);

protected:
    virtual PRUint32 GetElapsedTimeMSec(void);
    
    TransferState fState;
    char*         fURL;

    /**
     * The time at which the transfer began.
     */
    PRTime       fStart;
};


#endif /* nsSimpleTransfer_h__ */
