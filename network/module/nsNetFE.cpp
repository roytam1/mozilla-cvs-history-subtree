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

#include "mkutils.h"
#include "mkfe.h"

/* print network progress to the front end
 */

MODULE_PRIVATE extern "C" void
NET_Progress(MWContext *window_id, char *msg)
{
  FE_Progress(window_id, msg);
}

MODULE_PRIVATE PRBool 
NET_Confirm(MWContext *window_id, const char *msg)
{
  return FE_Confirm(window_id, msg);
}

MODULE_PRIVATE void 
NET_Alert(MWContext *window_id, const char *msg)
{
  FE_Alert(window_id, msg);
}

MODULE_PRIVATE PRBool
NET_PromptUsernameAndPassword(MWContext *window_id,
			      const char *msg,
			      char **username,
			      char **password)
{
  return FE_PromptUsernameAndPassword(window_id, msg, username, password);
}

MODULE_PRIVATE void
NET_EnableClicking(MWContext *window_id)
{
  FE_EnableClicking(window_id);
}

MODULE_PRIVATE void NET_GraphProgressInit(MWContext *window_id, 
					  URL_Struct *URL_s, 
					  int32 content_length)
{
  FE_GraphProgressInit(window_id, URL_s, content_length);
}

MODULE_PRIVATE void NET_GraphProgress(MWContext *window_id,
				      URL_Struct *URL_s,
				      int32 bytes_received,
				      int32 chunk_size,
				      int32 content_length)
{
  FE_GraphProgress(window_id, URL_s, bytes_received, chunk_size,
		   content_length);
}

MODULE_PRIVATE void NET_SetProgressBarPercent(MWContext *window_id,
					      int32 percent)
{
  FE_SetProgressBarPercent(window_id, percent);
}

MODULE_PRIVATE void NET_GraphProgressDestroy(MWContext *window_id,
					     URL_Struct *URL_s,
					     int32 content_length,
					     int32 bytes_received)
{
  FE_GraphProgressDestroy(window_id, URL_s, content_length, bytes_received);
}

MODULE_PRIVATE void NET_AllConnectionsComplete(MWContext *window_id)
{
  FE_AllConnectionsComplete(window_id);
}

MODULE_PRIVATE History *NET_GetHistory(MWContext *window_id)
{
  return &window_id->hist;
}

MODULE_PRIVATE PRBool NET_IsEditor(MWContext *window_id)
{
  return EDT_IS_EDITOR(window_id);
}

MODULE_PRIVATE MWContext *NET_GetParent(MWContext *window_id)
{
  return window_id->grid_parent;
}

MODULE_PRIVATE MWContextType NET_GetWindowType(MWContext *window_id)
{
  return window_id->type;
}
