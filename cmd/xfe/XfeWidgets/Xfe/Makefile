#!gmake
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
# 
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
# 
# The Original Code is the Xfe Widgets.
# 
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the terms of
# either of the GNU General Public License Version 2 or later (the "GPL"),
# or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
# 
# ***** END LICENSE BLOCK ***** 


##########################################################################
#
# Name:			Makefile
# Description:	Makefile for XfeWidgets library
# Author:		Ramiro Estrugo <ramiro@netscape.com>
#
##########################################################################

DEPTH			= ../../../..

# XfeWidgets headers are exported to dist/include/Xfe
MODULE			= XfeWidgets/Xfe
INCL_SUBDIR	= /Xfe

LIBRARY_NAME	= XfeWidgets

# Hardcode everything to on for now.
XFE_WIDGETS_WANT_ARROW=1
XFE_WIDGETS_WANT_BUTTON=1
XFE_WIDGETS_WANT_BYPASS_SHELL=1
XFE_WIDGETS_WANT_CASCADE=1
XFE_WIDGETS_WANT_DIVIDER=1
XFE_WIDGETS_WANT_DYNAMIC_MANAGER=1
XFE_WIDGETS_WANT_FONT_CHOOSER=1
XFE_WIDGETS_WANT_FRAME_SHELL=1
XFE_WIDGETS_WANT_LABEL=1
XFE_WIDGETS_WANT_LOGO=1
XFE_WIDGETS_WANT_MANAGER=1
XFE_WIDGETS_WANT_ORIENTED=1
XFE_WIDGETS_WANT_PANE=1
XFE_WIDGETS_WANT_PRIMITIVE=1
XFE_WIDGETS_WANT_PROGRESS_BAR=1

# Main XfeWidgets stuff
MAIN_CSRCS = \
				CallProcs.c \
				ClientData.c \
				Converters.c \
				Cursor.c \
				Debug.c \
				Draw.c \
				Find.c \
				Geometry.c \
				Linked.c \
				Private.c \
				Resources.c \
				SetValues.c \
				$(NULL)

MAIN_EXPORTS = \
				BasicDefines.h \
				ClientData.h \
				Converters.h \
				Debug.h \
				Draw.h \
				Find.h \
				Geometry.h \
				GeometryP.h \
				Linked.h \
				LinkedP.h \
				Resources.h \
				Xfe.h \
				XfeP.h \
				$(NULL)

# Misc utilities
UTIL_CSRCS = \
				ChildrenUtil.c \
				DialogUtil.c \
				GcUtil.c \
				MenuUtil.c \
				ListUtil.c \
				PixmapUtil.c \
				ShellUtil.c \
				StringUtil.c \
				Util.c \
				WmUtil.c \
				$(NULL)

UTIL_EXPORTS = \
				ChildrenUtil.h \
				DialogUtil.h \
				ListUtil.h \
				ListUtilP.h \
				MenuUtil.h \
				ShellUtil.h \
				StringUtil.h \
				WmUtil.h \
				$(NULL)

# XfeArrow
ifdef XFE_WIDGETS_WANT_ARROW
ARROW_CSRCS				= Arrow.c
ARROW_EXPORTS			= Arrow.h ArrowP.h
endif

# XfeButton
ifdef XFE_WIDGETS_WANT_BUTTON
BUTTON_CSRCS			= Button.c
BUTTON_EXPORTS			= Button.h ButtonP.h
endif

# XfeBypassShell
ifdef XFE_WIDGETS_WANT_BYPASS_SHELL
BYPASS_SHELL_CSRCS		= BypassShell.c
BYPASS_SHELL_EXPORTS	= BypassShell.h BypassShellP.h
endif

# XfeCascade
ifdef XFE_WIDGETS_WANT_CASCADE
CASCADE_CSRCS			= Cascade.c
CASCADE_EXPORTS			= Cascade.h CascadeP.h
endif

# XfeDivider
ifdef XFE_WIDGETS_WANT_DIVIDER
DIVIDER_CSRCS			= Divider.c
DIVIDER_EXPORTS			= Divider.h DividerP.h
endif

# XfeDynamicManager
ifdef XFE_WIDGETS_WANT_DYNAMIC_MANAGER
DYNAMIC_MANAGER_CSRCS	= DynamicManager.c
DYNAMIC_MANAGER_EXPORTS	= DynamicManager.h DynamicManagerP.h
endif

# XfeFontChooser
ifdef XFE_WIDGETS_WANT_FONT_CHOOSER
FONT_CHOOSER_CSRCS		= FontChooser.c
FONT_CHOOSER_EXPORTS	= FontChooser.h FontChooserP.h
endif

# XfeFrameShell
ifdef XFE_WIDGETS_WANT_FRAME_SHELL
FRAME_SHELL_CSRCS		= FrameShell.c
FRAME_SHELL_EXPORTS		= FrameShell.h FrameShellP.h
endif

# XfeLabel
ifdef XFE_WIDGETS_WANT_LABEL
LABEL_CSRCS				= Label.c
LABEL_EXPORTS			= Label.h LabelP.h
endif

# XfeLogo
ifdef XFE_WIDGETS_WANT_LOGO
LOGO_CSRCS				= Logo.c
LOGO_EXPORTS			= Logo.h LogoP.h
endif

# XfeManager
ifdef XFE_WIDGETS_WANT_MANAGER
MANAGER_CSRCS			= Manager.c ManagerChildren.c
MANAGER_EXPORTS			= Manager.h ManagerP.h
endif

# XfeOriented
ifdef XFE_WIDGETS_WANT_ORIENTED
ORIENTED_CSRCS			= Oriented.c
ORIENTED_EXPORTS		= Oriented.h OrientedP.h
endif

# XfePane
ifdef XFE_WIDGETS_WANT_PANE
PANE_CSRCS				= Pane.c
PANE_EXPORTS			= Pane.h PaneP.h
endif

# XfePrimitive
ifdef XFE_WIDGETS_WANT_PRIMITIVE
PRIMITIVE_CSRCS			= Primitive.c
PRIMITIVE_EXPORTS		= Primitive.h PrimitiveP.h
endif

# XfeProgressBar
ifdef XFE_WIDGETS_WANT_PROGRESS_BAR
PROGRESS_BAR_CSRCS		= ProgressBar.c
PROGRESS_BAR_EXPORTS	= ProgressBar.h ProgressBarP.h
endif


CSRCS			= \
				$(MAIN_CSRCS) \
				$(UTIL_CSRCS) \
				$(ARROW_CSRCS) \
				$(BUTTON_CSRCS) \
				$(BYPASS_SHELL_CSRCS) \
				$(CASCADE_CSRCS) \
				$(DIVIDER_CSRCS) \
				$(DYNAMIC_MANAGER_CSRCS) \
				$(FONT_CHOOSER_CSRCS) \
				$(FRAME_SHELL_CSRCS) \
				$(LABEL_CSRCS) \
				$(LOGO_CSRCS) \
				$(MANAGER_CSRCS) \
				$(ORIENTED_CSRCS) \
				$(PANE_CSRCS) \
				$(PRIMITIVE_CSRCS) \
				$(PROGRESS_BAR_CSRCS) \
				$(NULL)

REQUIRES		= XfeWidgets

EXPORTS			= \
				$(MAIN_EXPORTS) \
				$(UTIL_EXPORTS) \
				$(ARROW_EXPORTS) \
				$(BUTTON_EXPORTS) \
				$(BYPASS_SHELL_EXPORTS) \
				$(CASCADE_EXPORTS) \
				$(DIVIDER_EXPORTS) \
				$(DYNAMIC_MANAGER_EXPORTS) \
				$(FONT_CHOOSER_EXPORTS) \
				$(FRAME_SHELL_EXPORTS) \
				$(LABEL_EXPORTS) \
				$(LOGO_EXPORTS) \
				$(MANAGER_EXPORTS) \
				$(ORIENTED_EXPORTS) \
				$(PANE_EXPORTS) \
				$(PRIMITIVE_EXPORTS) \
				$(PROGRESS_BAR_EXPORTS) \
				$(NULL)

include $(DEPTH)/config/rules.mk
