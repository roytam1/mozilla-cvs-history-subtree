/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* C-visible definitions for C wrappers to C++ API
 *
 * basically, these are simply C-friendly templates so that we
 * can call into libxfe2 from a C file
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "pw_public.h"

pw_ptr pw_Create(MWContext *parent, PW_WindowType type);

void pw_SetCancelCallback(pw_ptr pw, PW_CancelCallback cancelcb,
                          void *cancelClosure);
void pw_Show(pw_ptr pw);

void pw_Hide(pw_ptr pw);

void pw_Destroy(pw_ptr pw);

void pw_SetWindowTitle(pw_ptr pw, const char *title);

void pw_SetLine1(pw_ptr pw, const char *text);

void pw_SetLine2(pw_ptr pw, const char *text);

void pw_SetProgressText(pw_ptr pw, const char * text);

void pw_SetProgressRange(pw_ptr pw, int32 minimum, int32 maximum);

void pw_SetProgressValue(pw_ptr pw, int32 value);

MWContext * pw_CreateProgressContext();

void pw_DestroyProgressContext(MWContext * context);

void pw_AssociateWindowWithContext(MWContext * context,
                                   pw_ptr pw);

void XFE_PW_AssociateWindowWithContext(MWContext *context,
                                      pw_ptr pw);
    
void fe_pw_AssociateWindowWithContext(MWContext *context, pw_ptr pw);
    
#ifdef __cplusplus
}
#endif
