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

#ifndef nsHTTPTransfer_h__
#define nsHTTPTransfer_h__

#include "nsSimpleTransfer.h"
#include "mkgeturl.h"
#include "prtime.h"

class nsHTTPTransfer : public nsSimpleTransfer
{
public:
    nsHTTPTransfer(ActiveEntry* entry);

    // nsITransfer methods

    virtual PRUint32
    GetTimeRemainingMSec(void);

    virtual void
    DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback);

    // Other public methods
    virtual void
    SetState(State state);

protected:
    virtual PRUint32 GetHTTPTimeRemainingMSec(void);
    ActiveEntry* fEntry;
};


#endif /* nsHTTPTransfer_h__ */

