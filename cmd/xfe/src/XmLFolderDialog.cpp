/* -*- Mode: C++; tab-width: 4 -*-
   XmLFolderDialog.cpp -- class definition for XmLFolderDialog
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#include "XmLFolderDialog.h"
#include "XmLFolderView.h"

//
// This is the dialog it self
//
XFE_XmLFolderDialog::XFE_XmLFolderDialog(XFE_View  *view, 
										 Widget     parent,
										 char      *name,
										 MWContext *context,
										 Boolean    ok, 
										 Boolean    cancel,
										 Boolean    help, 
										 Boolean    apply, 
										 Boolean    separator, 
										 Boolean    modal):
	XFE_ViewDialog(view, 
				   parent, 
				   name,
				   context,
				   ok,
				   cancel,
				   help,
				   apply,
				   separator,
				   modal)
{
  
  /* 1. Dialog frame is created in XFE_ViewDialog::XFE_ViewDialog
   */
  m_clientData = 0;

  /* 2. Create a folder view. m_chrome is the dialog widget
   */
  XFE_XmLFolderView *folderview = 
    new XFE_XmLFolderView(this /* topComponent */, m_chrome, 
			  0, 0);

  folderview->show();

  //
  // we don't want a default value, since return does other stuff for
  // us.
  XtVaSetValues(m_chrome, /* the dialog */
		XmNdefaultButton, NULL,
		NULL);

  /* set and show view 
   */
  setView(folderview);


}

XFE_XmLFolderDialog::~XFE_XmLFolderDialog()
{
	/* 100892: Caldera: address book problem causes continuous loop 
	 * reset flag to leave fe_eventloop()
	 */
	if (m_clientData)
		*m_clientData = (ANS_t) eCANCEL;
}

void XFE_XmLFolderDialog::setDlgValues()
{
  if (m_view)
    ((XFE_XmLFolderView *) m_view)->setDlgValues();
#if defined(DEBUG_tao_)
  else
    printf("\n***[XFE_XmLFolderDialog::setDlgValues] m_view is not set!");
#endif
}

void XFE_XmLFolderDialog::getDlgValues()
{
  if (m_view)
    ((XFE_XmLFolderView *) m_view)->getDlgValues();
#if defined(DEBUG_tao_)
  else
    printf("\n***[XFE_XmLFolderDialog::setDlgValues] m_view is not set!");
#endif
}

void XFE_XmLFolderDialog::cancel()
{
  // default setting
  if (m_clientData)
	  *m_clientData = (ANS_t) eCANCEL;
  else
	  hide();
}

void XFE_XmLFolderDialog::apply()
{

  // default setting
  if (m_clientData)
	  *m_clientData = (ANS_t) eAPPLY;

  if (m_view)
    ((XFE_XmLFolderView *) m_view)->apply();
}

void XFE_XmLFolderDialog::ok()
{
  // default setting
  if (m_clientData)
	  *m_clientData = (ANS_t) eAPPLY;

  apply();

  if (!m_clientData && m_okToDestroy)
	  hide();
}
