/* -*- Mode: C++; tab-width: 4 -*- */
/*
   PrefsDialogNServer.h -- Multiple news server preferences dialog
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Alec Flett <alecf@netscape.com>
 */

#ifndef _xfe_prefsdialognserver_h
#define _xfe_prefsdialognserver_h

#include "rosetta.h"
#include "Dialog.h"

class XFE_PrefsNServerDialog : public XFE_Dialog
{
 public:

    XFE_PrefsNServerDialog(Widget parent);
    void create();
    void init(const char *server, int32 port, XP_Bool xxx, XP_Bool password);
    void setPort(int32 port);
    char *getServerName();
    int32 getPort();
    HG78266
    XP_Bool getPassword();
    XP_Bool prompt();
    
    static void cb_cancel(Widget, XtPointer, XtPointer);
    static void cb_ok(Widget, XtPointer, XtPointer);
    HG18177

 private:
    Widget m_server_text;
    Widget m_port_text;
    HG18760
    Widget m_password_toggle;

    XP_Bool m_doneWithLoop;
    XP_Bool m_retVal;
    
};


#endif
