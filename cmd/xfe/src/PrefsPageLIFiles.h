/* -*- Mode: C++; tab-width: 4 -*- */
/*
   PrefsPageLIFiles.cpp -- class for LI file preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspagelifiles_h
#define _xfe_prefspagelifiles_h

#include "rosetta.h"
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
	HG89217
};

#endif
