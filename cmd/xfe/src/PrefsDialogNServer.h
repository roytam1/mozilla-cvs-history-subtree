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
   PrefsDialogNServer.h -- Multiple news server preferences dialog
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Alec Flett <alecf@netscape.com>
 */

#ifndef _xfe_prefsdialognserver_h
#define _xfe_prefsdialognserver_h

#include "Dialog.h"

class XFE_PrefsNServerDialog : public XFE_Dialog
{
 public:

    XFE_PrefsNServerDialog(Widget parent);
    void create();
    void init(const char *server, int32 port, XP_Bool ssl, XP_Bool password);
    void setPort(int32 port);
    char *getServerName();
    int32 getPort();
    XP_Bool getSsl();
    XP_Bool getPassword();
    XP_Bool prompt();
    
    static void cb_cancel(Widget, XtPointer, XtPointer);
    static void cb_ok(Widget, XtPointer, XtPointer);
    static void cb_ssl_toggle(Widget, XtPointer, XtPointer);

    void sslToggle(XmToggleButtonCallbackStruct *);

 private:
    Widget m_server_text;
    Widget m_port_text;
    Widget m_ssl_toggle;
    Widget m_password_toggle;

    XP_Bool m_doneWithLoop;
    XP_Bool m_retVal;
    
};


#endif
