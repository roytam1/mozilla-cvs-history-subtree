/* -*- Mode: C++; tab-width: 4 -*- */
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
