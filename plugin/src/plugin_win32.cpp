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

#include "plugin_md.h"
#include "prmem.h"
#include "nsWin32PluginClass.h"

#include <winbase.h>  // for GetModuleHandle(), etc.
#include <direct.h>   // for _chdir(), etc.


////////////////////////////////////////////////////////////////////////




nsString
nplmd_GetPluginDir(void)
{
    nsString result;

    char programFileName[_MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL),
                        programFileName,
                        sizeof(programFileName));

    result = programFileName;
    PRUint32 lastSlash = result.RFind(SEPARATOR);

    result.Cut(0, lastSlash);
    result += SEPARATOR;
    result += "plugins";

    return result;
}



PRBool
nplmd_IsPlugin(const nsString& filename)
{
    PRUint32 len = filename.Length();
    if (len > 6) { // at least "np.dll"
        nsString prefix = filename;
        prefix.Cut(0, 2);

        nsString suffix = filename;
        suffix.Cut(len - 4, 4);

        if (prefix.EqualsIgnoreCase("np") && suffix.EqualsIgnoreCase(".dll"))
            return TRUE;
    }

    return FALSE;
}



PRBool
nplmd_IsComposerPlugin(const nsString& filename)
{
    PRUint32 len = filename.Length();
    if (len > 6) { // at least "cp.dll"
        nsString prefix = filename;
        prefix.Cut(0, 2);

        nsString suffix = filename;
        suffix.Cut(len - 4, 4);

        if (prefix.EqualsIgnoreCase("cp") && suffix.EqualsIgnoreCase(".dll"))
            return TRUE;
    }

    return FALSE;
}



nsresult
nplmd_CreatePluginClass(const char* filename, nsIPluginClass* *result)
{
    return nsWin32PluginClass::Create(filename, result);
}



nsresult
nplmd_ChangeDir(nsString dir)
{
    if (dir.CharAt(1) == ':') {
        // There's a drive letter; do a _chdrive().
        int drive = toupper(dir.CharAt(0) - 'A' + 1);
        _chdrive(drive);

        dir.Cut(2, dir.Length() - 2);
    }

    char path[_MAX_PATH];
    dir.ToCString(path, sizeof(path));
    _chdir(path);

    return NS_OK;
}



nsString
nplmd_GetCurrentDir(void)
{
    nsString dir;
    char drive[3];
    drive[0] = _getdrive() + 'A' - 1;
    drive[1] = ':';
    drive[2] = '\0';

    dir = drive;
    char path[_MAX_PATH];
    _getcwd(path, sizeof(path));

    dir += path;
    return dir;
}

