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

#ifndef plugin_md_h__
#define plugin_md_h__

#include "nsCom.h"
#include "nsString.h"
#include "nsIPluginClass.h"
#include "prtypes.h"

#ifdef XP_WIN
#define SEPARATOR '\\'
#endif

#ifdef XP_MAC
#define SEPARATOR ':'
#endif

#ifdef XP_UNIX
#define SEPARATOR '/'
#endif

/**
 * Returns the platform-specific directory that contains the
 * plugins. This directory is searched recursively for plugin
 * libraries.
 */
extern nsString
nplmd_GetPluginDir(void);

/**
 * Returns <b>TRUE</b> if the specified filename matches the
 * expected pattern for a plugin library.
 */
extern PRBool
nplmd_IsPlugin(const nsString& filename);

/**
 * Determine if the specified file is a composer plugin.
 */
extern PRBool
nplmd_IsComposerPlugin(const nsString& filename);


/**
 * Do the platform-specific thing to actually create a plugin
 * registry entry.
 */
extern nsresult
nplmd_CreatePluginClass(const char* filename, nsIPluginClass* *result);

/**
 * Change directories. Not threadsafe. Wish this was in NSPR...
 */
extern nsresult
nplmd_ChangeDir(nsString dir);


/**
 * Return the current working directory. Wish this was in NSPR...
 */
extern nsString
nplmd_GetCurrentDir(void);


#endif // plugin_md_h__
