/* -*- Mode: C++; tab-width: 4 -*-
   ToolbarButton.cpp -- implementation file for buttons that can appear in 
                        the toolbar
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 13-Aug-96.
 */

#include "ToolbarButton.h"
#include <Xm/PushB.h>
#include "xp_mcom.h"

ToolbarButton::ToolbarButton(Toolbar *toolbar,
			     XFEView *respView,
			     const char *button_name,
			     XFECommandType button_command,
			     fe_icon *button_icon) : XFEComponent()
{
  Widget button;

  parentToolbar = toolbar;
  name = XP_STRDUP(name);
  command = button_command;
  icon = button_icon;
  v = respView;

  // this isn't really right...
  button = XtVaCreateManagedWidget(button_name,
				   xmPushButtonWidgetClass,
				   toolbar->baseWidget(),
				   NULL);

  setBaseWidget(button);

  XtAddCallback(button, XmNactivateCallback, activateCallback, this);
}

void
ToolbarButton::activateCallback(Widget, XtPointer clientData, XtPointer)
{
  ToolbarButton *obj = (ToolbarButton*)clientData;

  obj->activate();
}

void
ToolbarButton::activate()
{
  v->doCommand(command);
}
