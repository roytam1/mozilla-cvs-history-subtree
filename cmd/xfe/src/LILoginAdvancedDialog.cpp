/**********************************************************************
 LILoginAdvancedDialog.cpp
 By Daniel Malmer
 5/4/98

**********************************************************************/

#include "LILoginAdvancedDialog.h"


XFE_LILoginAdvancedDialog::XFE_LILoginAdvancedDialog(Widget parent) : XFE_Dialog(parent, "liLoginOptions", TRUE, TRUE, FALSE, FALSE, FALSE)
{
	Widget form;

	form = XmCreateForm(m_chrome, "prefs", NULL, 0);
	XtManageChild(form);

	m_serverFrame = new XFE_PrefsPageLIServer(form);
	m_filesFrame = new XFE_PrefsPageLIFiles(form);

	m_serverFrame->create();
	m_serverFrame->init();

	m_filesFrame->create();
	m_filesFrame->init();

	XtVaSetValues(m_serverFrame->get_frame(), 
					XmNbottomAttachment,  XmATTACH_NONE, 
					NULL);
	XtVaSetValues(m_filesFrame->get_frame(), 
					XmNtopAttachment,  XmATTACH_WIDGET,
					XmNtopWidget, m_serverFrame->get_frame(), 
					NULL);

	XtAddCallback(m_chrome, XmNokCallback, ok_callback, this);
	XtAddCallback(m_chrome, XmNcancelCallback, cancel_callback, this);
}


XFE_LILoginAdvancedDialog::~XFE_LILoginAdvancedDialog()
{
}


void
XFE_LILoginAdvancedDialog::ok_callback(Widget w, XtPointer closure, XtPointer data)
{
	((XFE_LILoginAdvancedDialog*) closure)->okCallback(w, data);
}


void
XFE_LILoginAdvancedDialog::cancel_callback(Widget w, XtPointer closure, XtPointer data)
{
	((XFE_LILoginAdvancedDialog*) closure)->cancelCallback(w, data);
}


void
XFE_LILoginAdvancedDialog::okCallback(Widget w, XtPointer data)
{
	m_serverFrame->save();
	m_filesFrame->save();
	cancelCallback(w, data);
}


void
XFE_LILoginAdvancedDialog::cancelCallback(Widget w, XtPointer data)
{
	hide();
}


