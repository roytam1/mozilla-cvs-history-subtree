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

#include "prtypes.h"
#include "structs.h"
#include "net.h"

PR_BEGIN_EXTERN_C

/**
 * Ensure that the context has a progress manager
 */
extern void
PM_EnsureProgressManager(MWContext* context);

/**
 * Release the progress manager for a context.
 */
extern void
PM_ReleaseProgressManager(MWContext* context);

/**
 * Notify the progress manager for the specified context that the
 * specified URL has begun to connect.
 */
extern void
PM_StartBinding(MWContext* context, const URL_Struct* url);

/**
 * Notify the progress manager for the specified context that some
 * progress has been made for the specified URL.
 */
extern void
PM_Progress(MWContext* context, const URL_Struct* url, PRUint32 bytesReceived, PRUint32 contentLength);

/**
 * Notify the progress manager for the specified context of status
 * for the specified URL. The progress manager will make a copy of the
 * message.
 */
extern void
PM_Status(MWContext* context, const URL_Struct* url, const char* message);

/**
 * Notify the progress manager for the specified context that the
 * URL has completed.
 */
extern void
PM_StopBinding(MWContext* context, const URL_Struct* url, PRInt32 status, const char* message);

PR_END_EXTERN_C

#endif /* progress_h__ */
