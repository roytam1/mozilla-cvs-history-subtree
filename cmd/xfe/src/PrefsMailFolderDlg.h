/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/* -*- Mode: C++; tab-width: 4 -*-
   PrefsMailFolderDlg.h -- Generic dialog for choosing a mail folder
   Created: Alec Flett <alecf@netscape.com>, 05-Mar-98
 */

#ifndef _xfe_prefsfolderdialog_h
#define _xfe_prefsfolderdialog_h
#include "msgmast.h"
#include "Dialog.h"
#include "FolderDropdown.h"

class XFE_PrefsMailFolderDialog : public XFE_Dialog
{
 public:
    XFE_PrefsMailFolderDialog(Widget parent,
                              MSG_Master *master,
                              MWContext *context);

    virtual ~XFE_PrefsMailFolderDialog();

    MSG_FolderInfo *prompt();
    virtual void initPage();    // initialize dialog
    void setFolder(MSG_FolderInfo *folder, int l10n_name=0);
    // get the chosen mail folder
    MSG_FolderInfo *getFolder();

    MWContext *getContext();
    
private:

    static void cb_ok(Widget, XtPointer, XtPointer);
    static void cb_cancel(Widget, XtPointer, XtPointer);
    static void cb_newFolder(Widget, XtPointer, XtPointer);
    static void cb_folderClick(Widget, XtPointer, XtPointer);
    static void cb_serverClick(Widget, XtPointer, XtPointer);
    static uint32 filterServersOnly(MSG_FolderInfo *);
    static uint32 filterFoldersOnly(MSG_FolderInfo *);
    void folderToggle(XmToggleButtonCallbackStruct *cbs);
    void serverToggle(XmToggleButtonCallbackStruct *cbs);
    void newFolder();

    MWContext *m_context;
    
    Widget m_instruct_label;
    Widget m_server_toggle;
    Widget m_folder_toggle;
    Widget m_new_folder_button;
    XFE_FolderDropdown *m_serverDropdown;
    XFE_FolderDropdown *m_folderDropdown;
    
    MSG_Master *m_master;
    MSG_FolderInfo *m_folder, *m_server;
    MSG_FolderLine *m_folderLine, *m_serverLine;
    char *m_l10n_name;
    
    XP_Bool m_doneWithLoop;
    MSG_FolderInfo *m_retVal;    
};

#endif
