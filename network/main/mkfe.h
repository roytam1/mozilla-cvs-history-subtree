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

#ifndef MKFE_H
#define MKFE_H
#include "prtypes.h"

PR_BEGIN_EXTERN_C

/*
 * Dialogs
 */

MODULE_PRIVATE PRBool NET_Confirm(MWContext *window_id, const char *msg);
MODULE_PRIVATE void NET_Alert(MWContext *window_id, const char *msg);
MODULE_PRIVATE PRBool NET_PromptUsernameAndPassword(MWContext *window_id,
						    const char *msg,
						    char **username,
						    char **password);

/*
 * Window Behavior
 */

MODULE_PRIVATE void NET_EnableClicking(MWContext *window_id);

/*
 * Progress/Status
 */

MODULE_PRIVATE void NET_GraphProgressInit(MWContext *window_id, 
					  URL_Struct *URL_s, 
					  int32 content_length);
MODULE_PRIVATE void NET_GraphProgress(MWContext *window_id,
				      URL_Struct *URL_s,
				      int32 bytes_received,
				      int32 chunk_size,
				      int32 content_length);
MODULE_PRIVATE void NET_SetProgressBarPercent(MWContext *window_id,
					      int32 percent);
MODULE_PRIVATE void NET_GraphProgressDestroy(MWContext *window_id,
					     URL_Struct *URL_s,
					     int32 content_length,
					     int32 bytes_received);
MODULE_PRIVATE void NET_AllConnectionsComplete(MWContext *window_id);

/*
 * Window Properties
 */

MODULE_PRIVATE PRBool NET_IsEditor(MWContext *window_id);
MODULE_PRIVATE History *NET_GetHistory(MWContext *window_id);
MODULE_PRIVATE MWContext *NET_GetParent(MWContext *window_id);
MODULE_PRIVATE MWContextType NET_GetWindowType(MWContext *window_id);

PR_END_EXTERN_C

#endif
