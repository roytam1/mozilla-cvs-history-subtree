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

#include "mozilla.h"
#include "xfe.h"
#include "pw_public.h"
#include "progress.h"

pw_ptr PW_Create(MWContext *parent,
                        PW_WindowType type) {
    
    return pw_Create(parent,type);
}


void PW_SetCancelCallback(pw_ptr pw,
                                     PW_CancelCallback cancelcb,
                                     void *cancelClosure)
{
    pw_SetCancelCallback(pw,cancelcb,cancelClosure);
}

void PW_Show(pw_ptr pw)
{
    pw_Show(pw);
}

void PW_Hide(pw_ptr pw)
{
    pw_Hide(pw);
}

void PW_Destroy(pw_ptr pw)
{
    pw_Destroy(pw);
}

void PW_SetWindowTitle(pw_ptr pw, const char *title)
{
    pw_SetWindowTitle(pw,title);
}

void PW_SetLine1(pw_ptr pw, const char *text)
{
    pw_SetLine1(pw,text);
}

void PW_SetLine2(pw_ptr pw, const char *text)
{
    pw_SetLine2(pw,text);
}

void PW_SetProgressText(pw_ptr pw, const char * text)
{
    pw_SetProgressText(pw,text);
}

void PW_SetProgressRange(pw_ptr pw, int32 minimum, int32 maximum)
{
    pw_SetProgressRange(pw,minimum, maximum);
}

void PW_SetProgressValue(pw_ptr pw, int32 value)
{
    pw_SetProgressValue(pw,value);
}

void
XFE_PW_AssociateWindowWithContext(MWContext *context, pw_ptr pw)
{
    fe_pw_AssociateWindowWithContext(context,pw);
}
