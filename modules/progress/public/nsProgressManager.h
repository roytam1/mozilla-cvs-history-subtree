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


#ifndef nsProgressManager_h__
#define nsProgressManager_h__

#include "nsITransferObserver.h"
#include "nsITransfer.h"
#include "nsISupportsArray.h"
#include "nsSimpleTransfer.h"

#include "prtypes.h"
#include "prtime.h"
#include "plhash.h"

#include "structs.h" // for MWContext

class nsProgressManager : public nsITransferObserver, public nsSimpleTransfer
{
protected:
    MWContext* fContext;

    /**
     * Transfers that are currently being observed
     */
    nsISupportsArray* fTransfers;

    /**
     * The amount of progress made; ranges from zero to 100.
     */
    PRUint16 fProgress;

    /**
     * The URLs that the progress manager is tracking.
     */
    PLHashTable* fURLs;

    /**
     * An FE_Timeout() object
     */
    void* fTimeout;

    /**
     * Static entry point called by the FE_Timeout().
     */
    static void TimeoutCallback(void* self);

    virtual void Tick(void);

    static void DisplayStatusCallback(void* self, char* message);
    virtual void DisplayStatus(char* message);

public:
    nsProgressManager(MWContext* context, const char* url);
    virtual ~nsProgressManager(void);

    // The nsITransferObserver interface
    virtual void Add(const char* url);
    virtual void NotifyBegin(nsITransfer* transfer);

    // The nsITransfer interface
    virtual PRUint32
    GetTimeRemainingMSec(void);

    virtual void
    DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback);


    NS_DECL_ISUPPORTS
};


#endif

