/* -*- Mode: C++; tab-width: 4 -*-
   ToolbarButton.h -- class definition for the toolbar buttons
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 13-Aug-96.
 */

#ifndef _xfe_toolbarbutton_h
#define _xfe_toolbarbutton_h

#include "xp_core.h" 

#include "XFEComponent.h"
#include "XFEView.h"
#include "Command.h"
#include "Toolbar.h"

#include "mozilla.h" /* for MWContext!!! */
#include "icons.h"

class ToolbarButton : public XFEComponent
{
private:
  Toolbar *parentToolbar;
  XFECommandType command;
  const char *name;
  fe_icon *icon;
  XFEView *v;

  void activate();

  static void activateCallback(Widget, XtPointer, XtPointer);

public:
  ToolbarButton(Toolbar *toolbar, 
		XFEView *respView,
		const char *button_name, 
		XFECommandType button_command, 
		fe_icon *button_icon);

  Toolbar *getToolbar() { return parentToolbar; }
};

#endif /* _xfe_toolbar_h */
