/* -*- Mode: C++; tab-width: 4 -*- */
/*
   PrefsPageLIGeneral.cpp -- class for LI general preferences dialog.
   Created: Daniel Malmer <malmer@netscape.com>
 */

#ifndef _xfe_prefspageligeneral_h
#define _xfe_prefspageligeneral_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"

class XFE_PrefsPageLIGeneral : public XFE_PrefsPage
{
 public:

	enum DialogUsage {pref, login};

    XFE_PrefsPageLIGeneral(XFE_PrefsDialog *dialog, DialogUsage usage = pref);
    XFE_PrefsPageLIGeneral(Widget parent, DialogUsage usage = pref);
    virtual ~XFE_PrefsPageLIGeneral();

    virtual void create();
    virtual void init();
    virtual void install();
    virtual void save();

    virtual Boolean verify();

#ifdef LI_BACKGROUND_SYNC_ENABLED
	void syncToggle(Widget, XtPointer);
	static void cb_toggle(Widget, XtPointer, XtPointer);
#endif

	Widget get_top_frame();
	Widget get_bottom_frame();

 private:
	DialogUsage m_usage;

	Widget m_top_frame;
	Widget m_bottom_frame;

	Widget m_enable_toggle;

#ifdef LI_BACKGROUND_SYNC_ENABLED
	Widget m_sync_toggle;
	Widget m_sync_text;
#endif

	Widget m_user_text;
	Widget m_password_text;
	Widget m_save_password_toggle;
};

#endif
