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

#ifndef nsWin32PluginClass_h__
#define nsWin32PluginClass_h__

#include "nsPluginClass.h"
#include <winbase.h>

class nsWin32PluginClass : public nsPluginClass {
protected:
    // For use with VerQueryValue()
    static const char* const DESCRIPTION_BLOCK;
    static const char* const FILE_EXTENTS_BLOCK;
    static const char* const FILE_OPEN_BLOCK;
    static const char* const MIME_TYPE_BLOCK;
    static const char* const NAME_BLOCK;
    static const char* const VERSION_BLOCK;

    DWORD fVersionMS; // XXX These don't ever seem to be used...
    DWORD fVersionLS;

    nsresult
    RegisterMimeTypes(const char* mimeTypes,
                      const char* fileExtents,
                      const char* fileOpens);

public:
    nsWin32PluginClass(char* name, char* filename, char* description,
                       DWORD dwVersionMS, DWORD dwVersionLS);

    static nsresult
    Create(const char* filename, nsIPluginClass* *result);
};

#endif // nsWin32PluginClass_h__
