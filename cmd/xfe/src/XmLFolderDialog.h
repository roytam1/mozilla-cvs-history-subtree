/* -*- Mode: C++; tab-width: 4 -*-
   XmLFolderDialog.h -- class definition for XmLFolderDialog
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#ifndef _xfe_xmlfolderdialog_h
#define _xfe_xmlfolderdialog_h

#include "ViewDialog.h"

// This is a general wrapper of dialog with XmLFloder widget
class XFE_XmLFolderDialog: public XFE_ViewDialog {
public:
  XFE_XmLFolderDialog(XFE_View *view, /* the parent view */
		      Widget    parent,
		      char     *name,
		      MWContext *context,
		      Boolean   ok = TRUE,
		      Boolean   cancel = TRUE,
		      Boolean   help = TRUE,  
		      Boolean   apply = TRUE, 
		      Boolean   separator = TRUE,
		      Boolean   modal = TRUE);

  virtual ~XFE_XmLFolderDialog();

  virtual void setDlgValues();
  virtual void getDlgValues();

  typedef enum {
	  eWAITING = 0, eCANCEL, eAPPLY, eOTHERS
  } ANS_t;

  // 
  void    setClientData(ANS_t *data) {m_clientData = data;}

protected:
  virtual void cancel();
  virtual void apply();
  virtual void ok();

  ANS_t *m_clientData;

private:


}; /* XFE_XmLFolderDialog */

#endif /* _xfe_xmlfolderdialog_h */
