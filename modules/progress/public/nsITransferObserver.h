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

#ifndef nsITransferObserver_h__
#define nsITransferObserver_h__

#include "nsISupports.h"
#include "nsITransfer.h"
#include "net.h" // for URL_Struct

// {663f0fa1-edfe-11d1-8031-006008159b5a}
#define NS_ITRANSFEROBSERVER_IID \
{0x663f0fa1, 0xedfe, 0x11d1,  \
    {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}


class nsITransferObserver : public nsISupports
{
public:
    /**
     * Add an object that the observer should track.
     */
    virtual void
    Add(const char* url) = 0;

    /**
     * Notify the observer that the specified transfer
     * is beginning.
     */
    virtual void NotifyBegin(nsITransfer* transfer) = 0;
};


#endif /* nsITransferObserver_h__ */
