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

#include "nsMimeType.h"
#include "nsIPluginClass.h"
#include "nsISupports.h"
#include "plstr.h"
#include "prlog.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginClassIID, NS_IPLUGINCLASS_IID);
static NS_DEFINE_IID(kIMimeTypeIID, NS_IMIMETYPE_IID);


////////////////////////////////////////////////////////////////////////


nsMimeType::nsMimeType(const char *mimetype,
                       const FO_Present_Types formatOuts[],
                       PRUint32 formatOutCnt)
    : fHandler(NULL),
      fAssociation(NULL), // XXX
      fFormatOuts(NULL),
      fFormatOutCnt(formatOutCnt)
{
    NS_INIT_REFCNT();
    PR_ASSERT(mimetype != NULL);
    fMimeType = (mimetype != NULL) ? PL_strdup(mimetype) : NULL;

    fFormatOuts = new FO_Present_Types[fFormatOutCnt];
    for (PRUint32 i = 0; i < fFormatOutCnt; ++i) {
        fFormatOuts[i] = formatOuts[i];
        NET_RegisterContentTypeConverter(fMimeType,
                                         fFormatOuts[i],
                                         this,
                                         CreateStream);
    }
}


nsMimeType::~nsMimeType(void)
{
    for (PRUint32 i = 0; i < fFormatOutCnt; ++i)
        NET_DeregisterContentTypeConverter(fMimeType, fFormatOuts[i]);

    // XXX Is this right?
    if (fAssociation)
        DeleteFileAssociation();

    if (fHandler != NULL)
        fHandler->Release();

    if (fMimeType != NULL)
        PL_strfree(fMimeType);
}


NS_IMPL_ADDREF(nsMimeType);
NS_IMPL_RELEASE(nsMimeType);

NS_METHOD
nsMimeType::QueryInterface(const nsIID& iid, void** instance)
{
    if (iid.Equals(kISupportsIID) ||
        iid.Equals(kIMimeTypeIID)) {
        *instance = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


nsMimeType::operator const char*(void) const
{
    return fMimeType;
}

NS_IMETHODIMP_(char*)
nsMimeType::ToNewCString(void)
{
    return PL_strdup(fMimeType);
}


NS_IMETHODIMP_(char*)
nsMimeType::ToCString(char* buf, PRUint32 len)
{
    PL_strncpy(buf, fMimeType, len);
    return buf;
}


NS_IMETHODIMP_(PRBool)
nsMimeType::Equals(const char* mimeType)
{
    return (PL_strcasecmp(fMimeType, mimeType) == 0) ? TRUE : FALSE;
}


NS_IMETHODIMP_(nsresult)
nsMimeType::SetHandler(nsIContentHandler* handler)
{
    if (fHandler != NULL)
        fHandler->Release();

    fHandler = handler;
    fHandler->AddRef();
    return NS_OK;
}


NS_IMETHODIMP_(nsIContentHandler*)
nsMimeType::GetHandler(void)
{
    if (fHandler)
        fHandler->AddRef();

    return fHandler;
}


NS_IMETHODIMP_(nsresult)
nsMimeType::CreateFileAssociation(const char* extensions,
                                             const char* description,
                                             void* fileType)
{
    fAssociation = NPL_NewFileAssociation(fMimeType, extensions, description, fileType);
    if (fAssociation == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}


NS_IMETHODIMP_(nsresult)
nsMimeType::RegisterFileAssociation(void)
{
    NPL_RegisterFileAssociation(fAssociation);
    return NS_OK;
}


NS_IMETHODIMP_(nsresult)
nsMimeType::DeleteFileAssociation(void)
{
    if (fAssociation != NULL) {
        NPL_DeleteFileAssociation(fAssociation);
        fAssociation = NULL;
    }

    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

NET_StreamClass*
nsMimeType::CreateStream(FO_Present_Types format_out,
                         void* type,
                         URL_Struct* urls,
                         MWContext* cx)
{
    NET_StreamClass* result = NULL;

    nsIMimeType* mimeType = (nsIMimeType*) type;
    nsIContentHandler* handler = mimeType->GetHandler();
    if (handler != NULL)
        result = handler->CreateStream(format_out, urls, cx);
    handler->Release();
    return result;
}
