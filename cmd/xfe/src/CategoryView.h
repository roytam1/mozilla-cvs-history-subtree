/* -*- Mode: C++; tab-width: 4 -*-
   CategoryView.h -- class definition for CategoryView.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 29-Aug-96.
 */

#ifndef _xfe_categoryview_h
#define _xfe_categoryview_h

#include "MNListView.h"
#include "Outlinable.h"

class XFE_CategoryView : public XFE_MNListView
{
public:
	XFE_CategoryView(XFE_Component *toplevel_component, Widget parent,
					 XFE_View *parent_view, MWContext *context,
					 MSG_Pane *p = NULL);
	
	virtual ~XFE_CategoryView();
	
	void loadFolder(MSG_FolderInfo *folderinfo);

	void selectCategory(MSG_FolderInfo *category);

	virtual Boolean isCommandEnabled(CommandType command, void *calldata = NULL,
									 XFE_CommandInfo* i = NULL);
	virtual Boolean handlesCommand(CommandType command, void *calldata = NULL,
								   XFE_CommandInfo* i = NULL);
	virtual void doCommand(CommandType command, void *calldata = NULL,
						   XFE_CommandInfo* i = NULL);
	
	/* Outlinable interface methods */
	virtual void *ConvFromIndex(int index);
	virtual int ConvToIndex(void *item);
	
	virtual char *getColumnName(int column);
	
	virtual char *getColumnHeaderText(int column);
	virtual fe_icon *getColumnHeaderIcon(int column);
	virtual EOutlinerTextStyle getColumnHeaderStyle(int column);
	virtual void *aquireLineData(int line);
	virtual void getTreeInfo(XP_Bool *expandable, XP_Bool *is_expanded, int *depth, OutlinerAncestorInfo **ancestor);
	virtual EOutlinerTextStyle getColumnStyle(int column);
	virtual char *getColumnText(int column);
	virtual fe_icon *getColumnIcon(int column);
	virtual void releaseLineData();

	virtual void Buttonfunc(const OutlineButtonFuncData *data);
	virtual void Flippyfunc(const OutlineFlippyFuncData *data);

	static const char *categorySelected;

private:
	static const int OUTLINER_COLUMN_NAME;
	
	MSG_FolderInfo *m_folderInfo;
	
	// for the outlinable stuff
	MSG_FolderLine m_categoryLine;
	OutlinerAncestorInfo *m_ancestorInfo;
};

#endif /* _xfe_categoryview_h */

