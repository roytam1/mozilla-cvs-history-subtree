/* -*- Mode: C++; tab-width: 4 -*-
   XmlFolderView.h -- class definition for XFE_XmlFolderView
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#ifndef _xfe_xmlfolderview_h
#define _xfe_xmlfolderview_h

#include "MNView.h"

class XFE_XmLTabView;

// This is a general wrapper of XmLFloder widget
class XFE_XmLFolderView: public XFE_MNView {

public:
  XFE_XmLFolderView(XFE_Component *top, /* the parent folderDialog */
					Widget         parent, 
					int           *tabNameId, 
					int nTabs);
  virtual ~XFE_XmLFolderView();

  //
  XFE_XmLTabView* addTab(int tabNameId);
  void addTab(XFE_XmLTabView* tab, XP_Bool show=TRUE);

  virtual void setDlgValues();
  virtual void getDlgValues();
  virtual void apply();

protected:

  // invoked when you switch tabs
  virtual void tabActivate(int pos);
  static  void tabActivateCB(Widget, XtPointer, XtPointer);

private:
  /* m_widget is the folder widget 
   */
  /* m_subviews contains the sub folder view: XFE_FolderView
   */
  XFE_XmLTabView *m_activeTab;
}; /* XFE_XmLFolderView */

#endif /* _xfe_xmlfolderview_h */
