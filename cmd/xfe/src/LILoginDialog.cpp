/**********************************************************************
 LILoginDialog.cpp
 By Daniel Malmer
 5/4/98

**********************************************************************/

#include "LILoginDialog.h"
#include "LILoginAdvancedDialog.h"

extern "C" void
fe_createLILoginDialog(Widget parent)
{
	XFE_LILoginDialog* dialog = new XFE_LILoginDialog(parent);

	dialog->show();

	while ( dialog->selection_made() == 0 ) {
		FEU_StayingAlive();
	}

	delete(dialog);
}


XFE_LILoginDialog::XFE_LILoginDialog(Widget parent) : XFE_Dialog(parent, "liLogin", TRUE, TRUE, FALSE, FALSE, FALSE)
{
	Widget form;
	Widget button;

	m_selection_made = 0;

	form = XmCreateForm(m_chrome, "prefs", NULL, 0);
	XtManageChild(form);

	m_userFrame = new XFE_PrefsPageLIGeneral(form, XFE_PrefsPageLIGeneral::login);

	m_userFrame->create();
	m_userFrame->init();

	XtUnmanageChild(m_userFrame->get_top_frame());

	XtVaSetValues(m_userFrame->get_bottom_frame(),
					XmNtopAttachment, XmATTACH_FORM,
					NULL);

	button = XmCreatePushButton(form, "liLoginAdvancedButton", NULL, 0);
	XtManageChild(button);

	XtVaSetValues(button, XmNrightAttachment, XmATTACH_FORM,
						  XmNtopAttachment, XmATTACH_WIDGET,
						  XmNtopOffset, 4,
						  XmNtopWidget, m_userFrame->get_bottom_frame(),
						  NULL);

	XtAddCallback(button, XmNactivateCallback, advanced_callback, this);

	XtAddCallback(m_chrome, XmNokCallback, ok_callback, this);
	XtAddCallback(m_chrome, XmNcancelCallback, cancel_callback, this);
}


XFE_LILoginDialog::~XFE_LILoginDialog()
{
}


void
XFE_LILoginDialog::advanced_callback(Widget w, XtPointer closure, XtPointer data)
{
	((XFE_LILoginDialog*) closure)->advancedCallback(w, data);
}


void
XFE_LILoginDialog::advancedCallback(Widget w, XtPointer data)
{
	XFE_LILoginAdvancedDialog* dialog;

	dialog = new XFE_LILoginAdvancedDialog(m_wParent);

	dialog->show();
}


void
XFE_LILoginDialog::ok_callback(Widget w, XtPointer closure, XtPointer data)
{
	((XFE_LILoginDialog*) closure)->okCallback(w, data);
}


void
XFE_LILoginDialog::cancel_callback(Widget w, XtPointer closure, XtPointer data)
{
	((XFE_LILoginDialog*) closure)->cancelCallback(w, data);
}


void
XFE_LILoginDialog::okCallback(Widget w, XtPointer data)
{
	m_userFrame->save();
	cancelCallback(w, data);
}


void
XFE_LILoginDialog::cancelCallback(Widget w, XtPointer data)
{
	m_selection_made = 1;
	hide();
}


