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
   PrefsPageLIServer.cpp -- class for LI server preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspageliserver_h
#define _xfe_prefspageliserver_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"
#include "PrefsData.h"

class XFE_PrefsPageLIServer : public XFE_PrefsPage
{
 public:

    XFE_PrefsPageLIServer(XFE_PrefsDialog *dialog);
    XFE_PrefsPageLIServer(Widget parent);
    virtual ~XFE_PrefsPageLIServer();

    virtual void create();
    virtual void init();
    virtual void install();
    virtual void save();

    virtual Boolean verify();

	Widget get_frame();

	void toggleCallback(Widget, XtPointer);
	static void cb_toggle(Widget, XtPointer, XtPointer);

 private:
	Widget m_ldap_toggle;
	Widget m_ldap_addr_text;
	Widget m_ldap_base_text;

	Widget m_http_toggle;
	Widget m_http_base_text;
};

#endif
