/* -*- Mode: C++; tab-width: 4 -*-
   XmlTabView.h -- class definition for XFE_XmlTabView
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#ifndef _xfe_xmltabview_h
#define _xfe_xmltabview_h

#include "View.h"

// This is a general wrapper of XmLFloder s tab form

class XFE_XmLTabView: public XFE_View {
public:
	XFE_XmLTabView(XFE_Component *top, /* the parent folderDialog */
				   XFE_View *view, /* the parent view */
				   int tab_string_id);

	virtual ~XFE_XmLTabView();
	
	virtual void setDlgValues() {};
	virtual void getDlgValues() {};
	virtual void apply(){};
	virtual void hide();
	virtual void show();

protected:
	Dimension m_labelWidth;

private:
	/* m_widget is the tab form
	 */
	Widget m_tab;
}; /* XFE_XmLTabView */

/* Offsets between widgets when hardwired is needed
 */
const int labelWidth = 125;
const int labelHeight = 30;
const int textFWidth = 175;
const int textFHeight = 30;

const int separatorHeight = 10;

const int majorVSpac = 6;
const int minorVSpac = 3;
const int majorHSpac = 6;
const int minorHSpac = 3;

#endif /* _xfe_xmltabview_h */
