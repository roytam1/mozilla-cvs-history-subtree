/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/*
   PrefsDialogMServer.h -- Headers for multiple mail server preferences dialog
   Created: Arun Sharma <asharma@netscape.com>, Thu Mar 19 14:37:46 PST 1998
 */

#ifndef _xfe_prefsdialog_mserver_h
#define _xfe_prefsdialog_mserver_h

#include "PropertySheetDialog.h"
#include "PropertySheetView.h"
#include "PropertyTabView.h"
#include "structs.h"

#define TYPE_POP 0
#define TYPE_IMAP 1

typedef void (closureCallback)(void *);

class XFE_PrefsMServerGeneralTab : public XFE_PropertyTabView
{
private:
	Widget m_server_label;
	Widget m_server_text;
	Widget m_server_type_menu;
	Widget m_server_type_label;
	Widget m_server_type_option;
	Widget m_server_user_label;
	Widget m_server_user_name;
	Widget m_remember_password;
	Widget m_check_mail;
	Widget m_check_time;
	Widget m_minute_label;
    Widget m_imap_button;
    Widget m_pop_button;
    
    closureCallback *m_imap_callback;
    void *m_imap_closure;
    closureCallback *m_pop_callback;
    void *m_pop_closure;
    Widget m_parent;
    XP_Bool m_is_imap;
    XP_Bool m_is_new;
    XP_Bool m_originally_imap;

public:
	XFE_PrefsMServerGeneralTab(XFE_Component *top, XFE_View *view,
                               Widget parent, XP_Bool allow_pop);
	virtual ~XFE_PrefsMServerGeneralTab() {};
    void create(XP_Bool allow_pop);
    void init(char *server_name, XP_Bool is_imap);
	void apply();

    void setIMAPCallback(closureCallback cb, void *closure);
    void setPOPCallback(closureCallback cb, void *closure);
    
	XP_Bool	getRememberPassword() { 
		return XmToggleButtonGadgetGetState(m_remember_password);
	};

	XP_Bool getCheckMail() {
		return XmToggleButtonGadgetGetState(m_check_mail);
	};

    // Need to be careful here not to leak memory
	char 	*getServerName() {
		return XmTextFieldGetString(m_server_text);
	};

	char 	*getUserName() {
		return XmTextFieldGetString(m_server_user_name);
	};

	int		getWaitTime() {
		char *p = XmTextFieldGetString(m_check_time);
		int i = atoi(p);
		XtFree(p);
		return i;
	};

	static void cb_optionImap(Widget, XtPointer, XtPointer);
	static void cb_optionPop(Widget, XtPointer, XtPointer);
};

class XFE_PrefsMServerIMAPTab : public XFE_PropertyTabView
{

private:
	Widget m_delete_trash_toggle;
    Widget m_delete_mark_toggle;
    Widget m_delete_remove_toggle;
	Widget m_use_ssl;
    Widget m_use_sub;

public:
	XFE_PrefsMServerIMAPTab(XFE_Component *top, XFE_View *view);
	virtual ~XFE_PrefsMServerIMAPTab() {};
    void create();
    void init(char *server_name);
    virtual void apply() {};
	void apply(char *server_name);

	XP_Bool getImapSsl() {
		return XmToggleButtonGadgetGetState(m_use_ssl);
	};

    XP_Bool getUseSub() {
        if (m_use_sub) 
            return XmToggleButtonGadgetGetState(m_use_sub);
        else
            return TRUE;
    };

};

class XFE_PrefsMServerAdvancedTab : public XFE_PropertyTabView
{
private:
	Widget m_path_prefs_label;
	Widget m_personal_dir_label;
	Widget m_personal_dir_text;
	Widget m_public_dir_label;
	Widget m_public_dir_text;
	Widget m_other_label;
	Widget m_other_text;
	Widget m_allow_server;

public:
	XFE_PrefsMServerAdvancedTab(XFE_Component *top, XFE_View *view);
	virtual ~XFE_PrefsMServerAdvancedTab() {};
    void create();
    void init(char *server_name);
    virtual void apply() {};
	void apply(char *server_name);

	char 	*getImapPersonalDir() {
		return XmTextFieldGetString(m_personal_dir_text);
	};

	char 	*getImapPublicDir() {
		return XmTextFieldGetString(m_public_dir_text);
	};

	char 	*getImapOthersDir() {
		return XmTextFieldGetString(m_other_text);
	};

	XP_Bool getOverrideNamespaces() {
		return XmToggleButtonGadgetGetState(m_allow_server);
	};

};


class XFE_PrefsMServerPOPTab : public XFE_PropertyTabView
{
private:
	Widget m_leave_messages;

public:
	XFE_PrefsMServerPOPTab(XFE_Component *top, XFE_View *view);
	virtual ~XFE_PrefsMServerPOPTab() {};

	XP_Bool	getLeaveMessages() { 
		return XmToggleButtonGadgetGetState(m_leave_messages);
	};

    void create();
    void init();
	void apply();
};


class XFE_PrefsMServerDialog : public XFE_PropertySheetDialog
{
private:
	// Real data
    XP_Bool m_is_imap;
    XP_Bool m_allow_pop;
    char *m_server_name;


	// Various tabs
	XFE_PrefsMServerGeneralTab *m_general_tab;
	XFE_PrefsMServerIMAPTab *m_imap_tab;
	XFE_PrefsMServerAdvancedTab *m_imap_advanced_tab;
	XFE_PrefsMServerPOPTab *m_pop_tab;
    static void changedToIMAP(void *closure);
    static void changedToPOP(void *closure);
    
public:
	XFE_PrefsMServerDialog(Widget parent, char *name,
                           XP_Bool allow_pop,
                           MWContext *context);

    Widget getChrome() { return m_chrome; };
    void create();
    void init();
    virtual void apply();
	void ChangeToIMAP();
	void ChangeToPOP();
};

#endif /* _xfe_prefsdialog_mserver_h */
