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

#include "pluginimpl.h" // for thePluginManager
#include "nsString.h"
#include "nsWin32PluginClass.h"
#include "nsMimeType.h"
#include "prmem.h"

////////////////////////////////////////////////////////////////////////

const char* const
nsWin32PluginClass::DESCRIPTION_BLOCK =  "StringFileInfo\\040904E4\\FileDescription";

const char* const
nsWin32PluginClass::FILE_EXTENTS_BLOCK = "StringFileInfo\\040904E4\\FileExtents";

const char* const
nsWin32PluginClass::FILE_OPEN_BLOCK = "StringFileInfo\\040904E4\\FileOpenName";

const char* const
nsWin32PluginClass::MIME_TYPE_BLOCK = "StringFileInfo\\040904E4\\MIMEType";

const char* const
nsWin32PluginClass::NAME_BLOCK = "StringFileInfo\\040904E4\\ProductName";

const char* const
nsWin32PluginClass::VERSION_BLOCK = "\\";

////////////////////////////////////////////////////////////////////////

nsWin32PluginClass::nsWin32PluginClass(char* name,
                                       char* filename,
                                       char* description,
                                       DWORD dwVersionMS,
                                       DWORD dwVersionLS)
    : nsPluginClass(name, filename, description),
      fVersionMS(dwVersionMS), fVersionLS(dwVersionLS)
{
}



nsresult
nsWin32PluginClass::RegisterMimeTypes(const char* pszMimeTypes,
                                      const char* pszFileExtents,
                                      const char* pszFileOpens)
{
    nsresult error = NS_OK;

    // Split each of the parallel arrays into their component parts
    nsString* mimeTypes = NULL;
    PRUint32 mimeTypeCount = nsString(pszMimeTypes).Split(&mimeTypes, '|');

    nsString* fileExtents = NULL;
    PRUint32 fileExtentCount = nsString(pszFileExtents).Split(&fileExtents, '|');

    nsString* fileOpens = NULL;
    PRUint32 fileOpenCount = nsString(pszFileOpens).Split(&fileOpens, '|');

    // Assert that the arrays are all the same length
    PR_ASSERT(fileOpenCount == mimeTypeCount);
    PR_ASSERT(fileExtentCount == mimeTypeCount);

    // Take the shortest so we don't barf in opt
    PRUint32 count;
    count = PR_MIN(fileOpenCount, mimeTypeCount);
    count = PR_MIN(count, fileExtentCount);

    // Iterate through each element in the parallel arrays,
    // registering the mimetype, etc.
    nsIMimeTypeRegistry* registry = thePluginManager->GetMimeTypeRegistry();
    for (PRUint32 i = 0; i < count; ++i) {
        nsIMimeType* mimeType = NULL;

        PRBool wasAlreadyRegistered
            = registry->Find(mimeTypes[i], &mimeType);

        if (!wasAlreadyRegistered) {
            static FO_Present_Types formatOuts[] = {
                FO_EMBED, FO_PLUGIN, FO_BYTERANGE
            };
            static PRUint32 formatOutCnt = 3;

            mimeType = new nsMimeType(mimeTypes[i], formatOuts, formatOutCnt);

            if (mimeType == NULL) {
                error = NS_ERROR_OUT_OF_MEMORY;
                break;
            }

            registry->Register(mimeType);
        }
        mimeType->SetHandler(this);

        // XXX what about fileopens & extents???
    }
    registry->Release();

    delete[] fileOpens;
    delete[] fileExtents;
    delete[] mimeTypes;

    return error;
}


nsresult
nsWin32PluginClass::Create(const char* filename, nsIPluginClass* *result)
{
    nsWin32PluginClass* pluginClass = NULL;
    nsresult error;
    char* pszMimeTypes = NULL;
    char* pszFileExtents = NULL;
    char* pszFileOpens = NULL;
    char* pszName = NULL;
    char* pszDescription = NULL;
    void* lpBuffer   = NULL;
    DWORD dwVersionMS;
    DWORD dwVersionLS;

    UINT uValueSize;

    // prepare to read the version info tags
    DWORD dwHandle = NULL;
    DWORD dwVerInfoSize = ::GetFileVersionInfoSize((char*) filename,
                                                   &dwHandle);

    if (dwVerInfoSize == 0)
        return NS_ERROR_FAILURE;

    VS_FIXEDFILEINFO* pVersionInfo =
        (VS_FIXEDFILEINFO*) PR_MALLOC(dwVerInfoSize);

    if (pVersionInfo == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    if(! ::GetFileVersionInfo((char*) filename, dwHandle,
                              dwVerInfoSize,
                              pVersionInfo)) {
        error = NS_ERROR_FAILURE;
        goto done;
    }

    // Query the key values of the plugin from the version
    // information. The next two are required...
    if (! ::VerQueryValue(pVersionInfo, (char*) MIME_TYPE_BLOCK,
                          (void**) &pszMimeTypes,
                          &uValueSize)) {
        error = NS_ERROR_FAILURE;
        goto done;
    }

    if (! ::VerQueryValue(pVersionInfo, (char*) VERSION_BLOCK,
                          &lpBuffer, &uValueSize)) {
        error = NS_ERROR_FAILURE;
        goto done;
    }

    dwVersionMS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionMS;
    dwVersionLS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionLS;

    // The rest of the values are not required...
    ::VerQueryValue(pVersionInfo, (char*) FILE_EXTENTS_BLOCK,
                    (void**) &pszFileExtents, &uValueSize);

    ::VerQueryValue(pVersionInfo, (char*) FILE_OPEN_BLOCK,
                    (void**) &pszFileOpens, &uValueSize);

    ::VerQueryValue(pVersionInfo, (char*) NAME_BLOCK,
                    (void**) &pszName, &uValueSize);

    ::VerQueryValue(pVersionInfo, (char*) DESCRIPTION_BLOCK,
                    (void**) &pszDescription, &uValueSize);

    // construct the result...
    pluginClass
        = new nsWin32PluginClass(pszName,
                                 (char*) filename,
                                 pszDescription,
                                 dwVersionMS,
                                 dwVersionLS
                                 );

    if (pluginClass == NULL) {
        error = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    if ((error = pluginClass->RegisterMimeTypes(pszMimeTypes,
                                                pszFileExtents,
                                                pszFileOpens)) != NS_OK) {
        pluginClass->Release();
        goto done;
    }

    *result = pluginClass;
    error = NS_OK;

done:
    PR_FREEIF(pVersionInfo);
    return error;
}


