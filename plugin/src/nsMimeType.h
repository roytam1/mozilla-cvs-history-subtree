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

#ifndef nsMimeType_h__
#define nsMimeType_h__

#include "nsISupports.h"
#include "nsIMimeType.h"
#include "nsIPluginClass.h"
#include "npassoc.h"

#include "net.h"
#include "xp.h"

/**
 * A plugin mime type class object, which holds information about the
 * mime type and any file associations that it has.
 */

class nsMimeType : public nsIMimeType
{
protected:

    /**
     * The default handler for the mime type.
     */
    nsIContentHandler* fHandler;

    /**
     * The mime type string
     */
    char* fMimeType;

    FO_Present_Types* fFormatOuts;

    PRUint32 fFormatOutCnt;

    /**
     * The file association for this plugin mimetype. Currently, this uses
     * The NPL_ file type association functions. Should eventually become a
     * first class object.
     */
    NPFileTypeAssoc* fAssociation;

    /**
     * Destroy the mime type class and clean up any file associations.
     */
    virtual ~nsMimeType(void);


    /**
     * Create a stream
     */
    static NET_StreamClass*
    CreateStream(FO_Present_Types format_out,
                 void* type,
                 URL_Struct* urls,
                 MWContext* cx);


public:
    virtual
    operator const char*(void) const;

    NS_IMETHOD_(char*)
    ToNewCString(void);

    NS_IMETHOD_(char*)
    ToCString(char* buf, PRUint32 len);

    NS_IMETHOD_(PRBool)
    Equals(const char* mimeType);

    NS_IMETHOD_(nsresult)
    SetHandler(nsIContentHandler* handler);

    NS_IMETHOD_(nsIContentHandler*)
    GetHandler(void);

    ////////////////////////////////////////////////////////////////////////
    // File association stuff.

    /**
     * Create a file association for this mime type class with the specified
     * file extension, description, and file type.
     */
    NS_IMETHOD_(nsresult)
    CreateFileAssociation(const char* extensions,
                          const char* description,
                          void* fileType);

    /**
     * Register the file association with netlib.
     */
    NS_IMETHOD_(nsresult)
    RegisterFileAssociation(void);

    /**
     * Delete the file association information and unregister the association
     * from netlib.
     */
    NS_IMETHOD_(nsresult)
    DeleteFileAssociation(void);

    /**
     * Retrieve the NPFileTypeAssociation for this mimetype.
     */
    NS_IMETHOD_(NPFileTypeAssoc*)
    GetFileAssociation() { return fAssociation; };

    NS_DECL_ISUPPORTS

    /**
     * Construct a new nsPluginMimeTypeClass with the specified
     * mimetype. 
     */
    nsMimeType(const char* mimetype,
               const FO_Present_Types formatOuts[],
               PRUint32 formatOutCnt);

private:
    // XXX Not to be used because of the NPFileAssociation global
    // registry...
    nsMimeType(void) {
        PR_ASSERT(FALSE);
    };

    // XXX Not meant to be used.
    nsMimeType(nsMimeType& obj) {
        PR_ASSERT(FALSE);
    };

    // XXX Not meant to be used.
    virtual nsMimeType&
    operator =(nsMimeType& obj) {
        PR_ASSERT(FALSE);
        return *(new nsMimeType());
    };
};





#endif // nsMimeType_h__
