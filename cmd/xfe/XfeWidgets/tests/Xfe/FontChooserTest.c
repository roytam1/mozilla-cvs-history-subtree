/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 * 
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 * 
 * The Original Code is the Xfe Widgets.
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		FontChooserTest.c										*/
/* Description:	Test for XfeFontChooser widget.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static String font_names[] = 
{
	"-adobe-courier-medium-r-normal--20-140-100-100-m-110-iso8859-1",
	"-adobe-helvetica-medium-r-normal--20-140-100-100-p-100-iso8859-1",
	"-adobe-new century schoolbook-medium-r-normal--20-140-100-100-p-103-iso8859-1",
	"-adobe-symbol-medium-r-normal--20-140-100-100-p-107-adobe-fontspecific",
	"-adobe-times-medium-r-normal--20-140-100-100-p-96-iso8859-1",
	"-b&h-lucida-medium-r-normal-sans-20-140-100-100-p-114-iso8859-1",
	"-b&h-lucidabright-medium-r-normal--20-140-100-100-p-114-iso8859-1",
	"-b&h-lucidatypewriter-medium-r-normal-sans-20-140-100-100-m-120-iso8859-1",
	"-misc-fixed-medium-r-normal--20-140-100-100-c-100-iso8859-1",
};

static String label_names[] = 
{
	"Courier",
	"Helvetica",
	"New Century Schoolbook",
	"Symbol",
	"Times",
	"Lucida",
	"Lucidabright",
	"Lucidatypewriter",
	"Fixed",
};

#define num_font_names XtNumber(font_names)

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget			form;
	Widget			frame;
    Widget			font_chooser;
	XmString *		xm_string_table;
	XmFontList *	font_list_table;
    
	XfeAppCreateSimple("FontChooserTest",&argc,argv,"MainFrame",&frame,&form);

	xm_string_table = XfeGetXmStringTable(label_names,num_font_names);

	font_list_table = XfeGetFontListTable(font_names,num_font_names);
    
    font_chooser = XtVaCreateManagedWidget("FontChooser",
										   xfeFontChooserWidgetClass,
										   form,
										   NULL);

    XtVaSetValues(font_chooser,
				  XmNnumFontItems,		num_font_names,
				  XmNfontItemLabels,	xm_string_table,
				  XmNfontItemFonts,		font_list_table,
				  NULL);
	
	XtPopup(frame,XtGrabNone);
	
    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
