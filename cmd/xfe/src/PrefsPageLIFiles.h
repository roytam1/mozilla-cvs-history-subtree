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
   PrefsPageLIFiles.h -- class for LI file preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspagelifiles_h
#define _xfe_prefspagelifiles_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"

class XFE_PrefsPageLIFiles : public XFE_PrefsPage
{
 public:

    XFE_PrefsPageLIFiles(XFE_PrefsDialog *dialog);
    XFE_PrefsPageLIFiles(Widget parent);
    virtual ~XFE_PrefsPageLIFiles();

    virtual void create();
    virtual void init();
    virtual void install();
    virtual void save();

    virtual Boolean verify();

	Widget get_frame();

 private:
	Widget m_bookmark_toggle;
	Widget m_cookies_toggle;
	Widget m_filter_toggle;
	Widget m_addrbook_toggle;
	Widget m_navcenter_toggle;
	Widget m_prefs_toggle;
	Widget m_javasec_toggle;
	Widget m_cert_toggle;
};

#endif
