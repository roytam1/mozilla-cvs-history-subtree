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

#include "nsITransferListener.h"
#include "nsTime.h"

#include "prtypes.h"
#include "plhash.h"

#include "structs.h" // for MWContext

class nsProgressManager : public nsITransferListener
{
protected:
    MWContext* fContext;

    nsProgressManager(MWContext* context);
    virtual ~nsProgressManager(void);

public:

    static void Ensure(MWContext* context);
    static void Release(MWContext* context);

    NS_DECL_ISUPPORTS

    // The nsITransferListener interface.

    // XXX This is _really_ similar to the nsIStreamListener
    // interface, which I want to supplant nsITransferListener at some
    // point. For now, bringing in nsIURL, nsString, etc. is just to
    // heavyweight.
    NS_IMETHOD
    OnStartBinding(const URL_Struct* url) = 0;

    NS_IMETHOD
    OnProgress(const URL_Struct* url, PRUint32 bytesReceived, PRUint32 contentLength) = 0;

    NS_IMETHOD
    OnStatus(const URL_Struct* url, const char* message) = 0;

    NS_IMETHOD
    OnStopBinding(const URL_Struct* url, PRInt32 status, const char* message) = 0;
};




#endif // nsProgressManager_h__

