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
/*
   PrefsPageLIGeneral.h -- class for LI general preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspageligeneral_h
#define _xfe_prefspageligeneral_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"

class XFE_PrefsPageLIGeneral : public XFE_PrefsPage
{
 public:

    XFE_PrefsPageLIGeneral(XFE_PrefsDialog *dialog);
    XFE_PrefsPageLIGeneral(Widget parent);
    virtual ~XFE_PrefsPageLIGeneral();

    virtual void create();
    virtual void init();
    virtual void install();
    virtual void save();

    virtual Boolean verify();

	void syncToggle(Widget, XtPointer);
	static void cb_toggle(Widget, XtPointer, XtPointer);

	Widget get_top_frame();
	Widget get_bottom_frame();

 private:
	Widget m_top_frame;
	Widget m_bottom_frame;

	Widget m_enable_toggle;
	Widget m_sync_toggle;
	Widget m_sync_text;

	Widget m_user_text;
	Widget m_password_text;
};

#endif
