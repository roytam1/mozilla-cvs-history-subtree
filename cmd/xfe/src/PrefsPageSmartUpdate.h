/* -*- Mode: C++; tab-width: 4 -*- */
/*
   PrefsPageSmartUpdate.cpp -- class for Smart Update preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspagesmartupdate_h
#define _xfe_prefspagesmartupdate_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"

class XFE_PrefsPageSmartUpdate : public XFE_PrefsPage
{
 public:

    XFE_PrefsPageSmartUpdate(XFE_PrefsDialog *dialog);
    virtual ~XFE_PrefsPageSmartUpdate();

    virtual void create();
    virtual void init();
    virtual void install();
    virtual void save();

    virtual Boolean verify();

	void uninstallCB(Widget, XtPointer);
	static void uninstall_cb(Widget, XtPointer, XtPointer);

 private:
	void populateList();
	Widget m_enable_toggle;
	Widget m_confirm_toggle;
	Widget m_uninstall_list;
	Widget m_fake_list;
};

#endif
