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

#ifndef nsIMimeType_h__
#define nsIMimeType_h__

#include "nsISupports.h"
#include "nsIContentHandler.h"
#include "npassoc.h" // XXX
#include "prtypes.h"

////////////////////////////////////////////////////////////////////////

/**
 * A mime type class object, which holds information about the
 * mime type and any file associations that it has.
 */

class nsIMimeType : public nsISupports
{
public:
    // XXX This is bad for Unicode, but until Unicode gets into NSPR,
    // it's just too painful to use ToNewCString(), etc.
    virtual
    operator const char*(void) const = 0;
    
    NS_IMETHOD_(char*)
    ToNewCString(void) = 0;

    NS_IMETHOD_(char*)
    ToCString(char* buf, PRUint32 len) = 0;

    NS_IMETHOD_(PRBool)
    Equals(const char* mimeType) = 0;

    NS_IMETHOD_(nsIContentHandler*)
    GetHandler(void) = 0;

    NS_IMETHOD_(nsresult)
    SetHandler(nsIContentHandler* handler) = 0;

    ////////////////////////////////////////////////////////////////////////
    // File association stuff. This is just inheirited more or less
    // wholesale from the previous implementation because I haven't
    // had time to look at it yet...
    //

    /**
     * Create a file association for this mime type class with the specified
     * file extension, description, and file type.
     */
    NS_IMETHOD_(nsresult)
    CreateFileAssociation(const char* extensions,
                          const char* description,
                          void* fileType)
        = 0;

    /**
     * Register the file association with netlib.
     */
    NS_IMETHOD_(nsresult)
    RegisterFileAssociation(void) = 0;

    /**
     * Delete the file association information and unregister the association
     * from netlib.
     */
    NS_IMETHOD_(nsresult)
    DeleteFileAssociation(void) = 0;

    /**
     * Retrieve the NPFileTypeAssociation for this mimetype.
     */
    NS_IMETHOD_(NPFileTypeAssoc*)
    GetFileAssociation() = 0;
};

// XXX Remember to get a GUID for this...
#define NS_IMIMETYPE_IID                             \
{ /* 5d852ef0-a1bc-11d1-85b1-00805f0e4dff */         \
    0x5d852ef0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xff} \
}

#endif // nsIMimeType_h__
