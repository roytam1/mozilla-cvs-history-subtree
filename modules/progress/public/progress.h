/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*
 * A C-interface to the progress manager.
 */

#ifndef progress_h__
#define progress_h__

#include "structs.h"

PR_BEGIN_EXTERN_C

extern void
PM_CreateProgressManager(MWContext* context, const char* url);

extern void
PM_DestroyProgressManager(MWContext* context);

PR_END_EXTERN_C

#endif /* progress_h__ */
