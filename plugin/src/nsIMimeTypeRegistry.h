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

#ifndef nsIMimeTypeRegistry_h__
#define nsIMimeTypeRegistry_h__

#include "nsISupports.h"
#include "nsIMimeType.h"
#include "nsIEnumerator.h"

class nsIMimeTypeRegistry : public nsISupports
{
public:

    /**
     * Add a new mime type class to the registry.
     */
    NS_IMETHOD_(nsresult)
    Register(nsIMimeType* mimeType) = 0;

    /**
     * Create an enumerator that will iterate through all of the
     * mime types in the registry.
     */
    NS_IMETHOD_(nsresult)
    CreateMimeTypeEnumerator(nsIEnumerator* *result) = 0;

    /**
     * Locate the mime type class entry for the specified mime type.
     *
     * @param type The mimetype to find.
     * @param result A pointer to a pointer for the resulting
     * <b>nsIMimeType</b> object. If NULL, no mime type class
     * is returned.
     * @returns <b>TRUE</b> if the mime type is contained in the
     * registry, <b>FALSE</b> otherwise.
     */
    NS_IMETHOD_(PRBool)
    Find(const char* mimetype, nsIMimeType* *result) = 0;

    /**
     * Locate the mimetype that most appropriately matches the specified
     * mimetype string. Wildcard matching is performed.
     */
    NS_IMETHOD_(PRBool)
    FindClosestMatch(const char* mimetype, nsIMimeType* *result) = 0;
};


// XXX Remember to get a GUID for this...
#define NS_IMIMETYPEREGISTRY_IID                     \
{ /* dd002ef0-a1bc-11d1-85ab-00805f0e4dff */         \
    0xdd002ef0,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xab, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xff} \
}



#endif // nsIMimeTypeRegistry_h__
