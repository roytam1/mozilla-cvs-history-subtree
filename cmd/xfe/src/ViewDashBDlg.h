/* -*- Mode: C++; tab-width: 4 -*-
   ViewDashBDlg.h -- View dialog with a dashboard.
   Created: Tao Cheng <tao@netscape.com>, 27-apr-98.
 */

/* Insert copyright and license here 1998 */


#ifndef _VIEWDASHBDLG_H_
#define _VIEWDASHBDLG_H_

#include "ViewDialog.h"
#include "Dashboard.h"

class XFE_ViewDashBDlg : public XFE_ViewDialog
{
public:
	XFE_ViewDashBDlg(Widget     parent, 
					 char      *name, 
					 MWContext *context,
					 Boolean    ok     = True, 
					 Boolean    cancel = True,
					 Boolean    help   = False, 
					 Boolean    apply  = False, 
					 Boolean    modal  = False);
	
	virtual ~XFE_ViewDashBDlg();


protected:
	static Widget create_chrome_widget(Widget   parent, 
									   char    *name,
									   Boolean  ok, 
									   Boolean  cancel,
									   Boolean  help, 
									   Boolean  apply,
									   Boolean  separator, 
									   Boolean  modal);

	Widget createButtonArea(Widget parent,
							Boolean ok, Boolean cancel,
							Boolean help, Boolean apply);

	virtual void attachView();

	XFE_Dashboard *m_dashboard;
	Widget         m_aboveButtonArea;
	Widget         m_okBtn;
	
private:
	Widget         m_buttonArea;
};

#endif /* _VIEWDASHBDLG_H_ */
