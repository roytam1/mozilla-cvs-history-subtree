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
   PrefsDialogMServer.cpp -- Multiple mail server preferences dialog
   Created: Arun Sharma <asharma@netscape.com>, Thu Mar 19 14:37:46 PST 1998
 */


#include "MozillaApp.h"
#include "prefapi.h"
#include "felocale.h"
#include "xpgetstr.h"
#include "PrefsDialogMServer.h"

#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ArrowBG.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h> 
#include <Xm/TextF.h> 
#include <Xm/ToggleBG.h> 
#include <Xfe/Xfe.h>

// IMAP pref stuff taken from WINFE. This code is XP really.
#define USERNAME ".userName"
#define REMEMBER_PASSWORD ".remember_password"
#define CHECK_NEW_MAIL ".check_new_mail"
#define CHECK_TIME ".check_time"
#define OVERRIDE_NAMESPACES ".override_namespaces"
#define DELETE_MODEL ".delete_model"
#define IS_SECURE ".isSecure"
#define NS_OTHER ".namespace.other_users"
#define NS_PERSONAL ".namespace.personal"
#define NS_PUBLIC ".namespace.public"
#define USE_SUB ".using_subscription"

#define POP_NAME "mail.pop_name"
#define POP_REMEMBER_PASSWORD "mail.remember_password"
#define POP_CHECK_NEW_MAIL "mail.check_new_mail"
#define POP_CHECK_TIME "mail.check_time"
#define POP_LEAVE_ON_SERVER "mail.leave_on_server"



extern int XFE_GENERAL_TAB;
extern int XFE_ADVANCED_TAB;
extern int XFE_IMAP_TAB;
extern int XFE_POP_TAB;

static const char PrefTemplate[] = "mail.imap.server.";

// Make sure XP_FREE() from the caller function;
char* 
IMAP_GetPrefName(const char *host_name, const char *pref)
{
	int		pref_size=sizeof(PrefTemplate) +
        XP_STRLEN(host_name) +
        XP_STRLEN(pref) + 1;
	char	*pref_name= (char *) XP_CALLOC(pref_size, sizeof(char));

    XP_STRCPY(pref_name, PrefTemplate);
    XP_STRCAT(pref_name, host_name);
    XP_STRCAT(pref_name, pref);

	return pref_name;
}

XP_Bool
IMAP_PrefIsLocked(const char *host_name, const char* pref)
{
	XP_Bool	result;
	char* pref_name = IMAP_GetPrefName(host_name, pref);
    
	if (!pref_name) return FALSE;

    result = PREF_PrefIsLocked(pref_name);
    XP_FREE(pref_name);

	return result;
}

void 
IMAP_SetCharPref(const char *host_name, const char* pref, const char* value)
{
	char* pref_name = IMAP_GetPrefName(host_name, pref);
    
	if (!pref_name) return;
    
    PREF_SetCharPref(pref_name, value);
    XP_FREE(pref_name);

}

void 
IMAP_SetIntPref(const char *host_name, const char* pref, int32 value)
{
	char* pref_name = IMAP_GetPrefName(host_name, pref);

	if (!pref_name) return;
    
    PREF_SetIntPref(pref_name, value);
    XP_FREE(pref_name);
}

void 
IMAP_SetBoolPref(const char *host_name, const char *pref, XP_Bool value) 
{
	char*	pref_name = IMAP_GetPrefName(host_name, pref);

	if (!pref_name) return;

    PREF_SetBoolPref(pref_name, value);
    XP_FREE(pref_name);
}

int
IMAP_CopyCharPref(const char *host_name, const char *pref, char **buf) 
{
    char*	pref_name = IMAP_GetPrefName(host_name, pref);
    if (!pref_name) return PREF_ERROR;
	if (pref_name)
	{
        int retval = PREF_CopyCharPref(pref_name, buf);
		XP_FREE(pref_name);
        return retval;
	}
    return PREF_ERROR;
}

int
IMAP_GetIntPref(const char *host_name, const char* pref, int32 *intval)
{
	char*	pref_name = IMAP_GetPrefName(host_name,pref);
	if (!pref_name) return PREF_ERROR;

    int retval = PREF_GetIntPref(pref_name, intval);
    XP_FREE(pref_name);

    return retval;
}

int
IMAP_GetBoolPref(const char *host_name, const char* pref, XP_Bool *boolval)
{
	char*	pref_name = IMAP_GetPrefName(host_name, pref);
	if (!pref_name) return PREF_ERROR;
    
    int retval = PREF_GetBoolPref(pref_name, boolval);
    XP_FREE(pref_name);
    
    return retval;
}


XFE_PrefsMServerDialog::XFE_PrefsMServerDialog(Widget parent, 
                                               char *server_name,
                                               XP_Bool allow_pop,
                                               MWContext *context)
	: XFE_PropertySheetDialog((XFE_View *) NULL,
						  parent, 
						  "MailServerInfo",
						  context,
						  TRUE, // ok
						  TRUE, // cancel
						  FALSE, // help
						  FALSE, // apply
						  TRUE, // separator
						  TRUE // modal
				 ), 
      m_allow_pop(allow_pop),
      m_server_name(server_name)
{
    int32 server_type;
    PREF_GetIntPref("mail.server_type",&server_type);
    if (server_type==TYPE_IMAP)
        m_is_imap=TRUE;
    else
        m_is_imap=FALSE;
    
    create();
    init();

}

void XFE_PrefsMServerDialog::create() {
	XFE_PropertySheetView* folder_view = (XFE_PropertySheetView *) m_view;


    // create tabs
    m_general_tab =
        new XFE_PrefsMServerGeneralTab(this,folder_view,m_wParent,
                                       m_allow_pop);
    m_imap_tab = new XFE_PrefsMServerIMAPTab(this, folder_view);
    m_imap_advanced_tab = new XFE_PrefsMServerAdvancedTab(this, folder_view);
    m_pop_tab = new XFE_PrefsMServerPOPTab(this, folder_view);

    // add tabs
    folder_view->addTab(m_general_tab);

    // add these tabs but don't show them
    folder_view->addTab(m_imap_tab);
    folder_view->addTab(m_imap_advanced_tab);
    folder_view->addTab(m_pop_tab);
    
    // start with general tab
    m_general_tab->show();

    m_general_tab->setIMAPCallback(changedToIMAP, (void *)this);
    m_general_tab->setPOPCallback(changedToPOP, (void *)this);
}

void XFE_PrefsMServerDialog::init() {

    m_general_tab->init(m_server_name, m_is_imap);
    if (m_is_imap) {
        m_imap_tab->show();
        m_imap_advanced_tab->show();
        m_pop_tab->hide();
    } else {
        m_pop_tab->show();
        m_imap_tab->hide();
        m_imap_advanced_tab->hide();
    }
    // always fill
    m_imap_advanced_tab->init(m_server_name);
    m_imap_tab->init(m_server_name);
    m_pop_tab->init();

}

void XFE_PrefsMServerDialog::apply() {

    
    // we need to tell the other panes what server they should use
    // and only apply the relevant panes
    char *server_name=m_general_tab->getServerName();
    m_general_tab->apply();
    if (m_is_imap) {
        m_imap_tab->apply(server_name);
        m_imap_advanced_tab->apply(server_name);
    } else 
        m_pop_tab->apply();

}

void
XFE_PrefsMServerDialog::changedToIMAP(void *closure) {
    ((XFE_PrefsMServerDialog *)closure)->ChangeToIMAP();
}

void
XFE_PrefsMServerDialog::changedToPOP(void *closure) {
    ((XFE_PrefsMServerDialog *)closure)->ChangeToPOP();
}

// callback when GeneralTab switches to IMAP
void 
XFE_PrefsMServerDialog::ChangeToIMAP()
{
    m_pop_tab->hide();
    m_imap_tab->show();
    m_imap_advanced_tab->show();
    m_is_imap=TRUE;
}

// callback when GeneralTab switches to POP
void 
XFE_PrefsMServerDialog::ChangeToPOP()
{
    m_imap_tab->hide();
    m_imap_advanced_tab->hide();
    m_pop_tab->show();
    m_is_imap=FALSE;
}


// Create the body of the IMAP General Tab
XFE_PrefsMServerGeneralTab::XFE_PrefsMServerGeneralTab(
	XFE_Component *top,
	XFE_View *view,
    Widget parent,
    XP_Bool allow_pop)
	: XFE_PropertyTabView(top, view, XFE_GENERAL_TAB),
      m_server_label(0),
      m_server_text(0),
      m_server_type_menu(0),
      m_server_type_label(0),
      m_server_type_option(0),
      m_server_user_label(0),
      m_server_user_name(0),
      m_remember_password(0),
      m_check_mail(0),
      m_check_time(0),
      m_minute_label(0),
      m_imap_button(0),
      m_pop_button(0),
      m_imap_callback(0),
      m_imap_closure(0),
      m_pop_callback(0),
      m_pop_closure(0),
      m_parent(parent)
{
    create(allow_pop);
}

void XFE_PrefsMServerGeneralTab::create(XP_Bool allow_pop) {
    Widget			kids[15];
	Arg				av[10];
	int				ac = 0, i  = 0;
	Widget			form;

	form = getBaseWidget();

	kids[i++] = m_server_label = 
		XmCreateLabelGadget(form, "ServerName", av, ac);

	kids[i++] = m_server_text =
		fe_CreateTextField(form, "ServerNameText", av, ac);


	Visual    *v = 0;
	Colormap   cmap = 0;
	Cardinal   depth = 0;

	ac = 0;

	XtVaGetValues (m_parent,
				   XtNvisual, &v,
				   XtNcolormap, &cmap,
				   XtNdepth, &depth, 
				   0);

	ac = 0;
	kids[i++] = m_server_type_label =
		XmCreateLabelGadget(form, "ServerType", av, ac);

	ac = 0;
	XtSetArg(av[ac], XmNvisual, v); ac++;
	XtSetArg(av[ac], XmNdepth, depth); ac++;
	XtSetArg(av[ac], XmNcolormap, cmap); ac++;
	m_server_type_menu = XmCreatePulldownMenu (form, "serverTypeMenu", av, ac);

	ac = 0;
	XtSetArg (av [ac], XmNsubMenuId, m_server_type_menu); ac++;
	kids[i++] = m_server_type_option = 
		fe_CreateOptionMenu (form, "serverTypeOption", av, ac);
	fe_UnmanageChild_safe(XmOptionLabelGadget(m_server_type_option));

	// Now add the entries

    // POP
    if (allow_pop) {
        m_pop_button = XmCreatePushButtonGadget(m_server_type_menu,
                                                "popOption", av, ac);
        XtAddCallback(m_pop_button, XmNactivateCallback,
                      cb_optionPop,(XtPointer)this);
        XtManageChild(m_pop_button);
    }
    
    // IMAP
	m_imap_button = XmCreatePushButtonGadget(m_server_type_menu,
                                             "imapOption", av, ac);
	XtAddCallback(m_imap_button, XmNactivateCallback,
				  cb_optionImap,(XtPointer)this);

	XtManageChild(m_imap_button);


    // IMAP is selected until init()
    XtVaSetValues (m_server_type_option,
                   XmNmenuHistory, m_imap_button,
                   NULL);

	ac = 0;
	kids[i++] = m_server_user_label = 
		XmCreateLabelGadget(form, "ServerUser", av, ac);

	kids[i++] = m_server_user_name = 
		fe_CreateTextField(form, "Username", av, ac);

	kids[i++] = m_remember_password = 
		XmCreateToggleButtonGadget(form, "RememberPass", av, ac);

	kids[i++] = m_check_mail = 
		XmCreateToggleButtonGadget(form, "CheckMail", av, ac);
	
	kids[i++] = m_check_time =
		fe_CreateTextField(form, "WaitTime", av, ac);

	kids[i++] = m_minute_label = 
		XmCreateLabelGadget(form, "MinuteLabel", av, ac);
    
    int max_height1 = XfeVaGetTallestWidget(m_server_label,
                                            m_server_text,
                                            NULL);
    int max_height2 = XfeVaGetTallestWidget(m_server_type_label,
                                            m_server_type_option,
                                            NULL) + 15;
    int max_height3 = XfeVaGetTallestWidget(m_server_user_label,
                                            m_server_user_name,
                                            NULL);
    int max_height5 = XfeVaGetTallestWidget(m_check_mail,
                                            m_check_time,
                                            m_minute_label,
                                            NULL);
	// Specify the geometry constraints
	XtVaSetValues(m_server_label,
                  XmNheight, max_height1,
				  XmNalignment, XmALIGNMENT_END,
				  XmNleftAttachment, XmATTACH_FORM,
                  XmNtopAttachment, XmATTACH_FORM,
				  NULL);

	XtVaSetValues(m_server_text,
                  XmNheight, max_height1,
                  XmNleftAttachment, XmATTACH_WIDGET,
                  XmNleftWidget, m_server_label,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNtopWidget, m_server_label,
				  NULL);

	XtVaSetValues(m_server_type_label,
                  XmNheight, max_height2,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_server_label,
				  NULL);

	XtVaSetValues(m_server_type_option,
                  XmNheight, max_height2,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, m_server_type_label,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_server_type_label,
				  NULL);

	XtVaSetValues(m_server_user_label,
                  XmNheight, max_height3,
				  XmNalignment, XmALIGNMENT_END,
				  XmNleftAttachment, XmATTACH_FORM,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, m_server_type_label,
				  NULL);

	XtVaSetValues(m_server_user_name,
                  XmNheight, max_height3,
				  XmNrightAttachment, XmATTACH_FORM,
                  XmNleftAttachment, XmATTACH_WIDGET,
                  XmNleftWidget, m_server_user_label,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_server_user_label,
				  NULL);

	XtVaSetValues(m_remember_password,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_server_user_label,
				  NULL);

	XtVaSetValues(m_check_mail,
                  XmNheight, max_height5,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_remember_password,
				  NULL);
											   
	XtVaSetValues(m_check_time,
                  XmNheight, max_height5,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, m_check_mail,
				  XmNtopAttachment,  XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_check_mail,
				  NULL);
				   
	XtVaSetValues(m_minute_label,
                  XmNheight, max_height5,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, m_check_time,
                  XmNrightWidget, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_check_time,
				  NULL);

	// Manage the kids
	XtManageChildren(kids, i);
}

void
XFE_PrefsMServerGeneralTab::init(char *server_name,
                                 XP_Bool is_imap)
{

    m_is_new=FALSE;
    m_is_imap=is_imap;

    // we need to know if the IMAPHost already existed
    // because if we change this host POP->IMAP we need to create
    // the IMAPHost
    m_originally_imap=is_imap;

    if (is_imap)
        XtVaSetValues (m_server_type_option,
                       XmNmenuHistory, m_imap_button,
                       NULL);
    else
        XtVaSetValues (m_server_type_option,
                       XmNmenuHistory, m_pop_button,
                       NULL);
    
    if (!server_name) {
        m_is_new=TRUE;
        return;   // new server, nothing to init
    }
    
    char *user_name=NULL;
    XP_Bool remember_password;
    XP_Bool check_mail;
    int32 check_time;

    fe_SetTextField(m_server_text, server_name);
    
    if (is_imap) {
        XtVaSetValues(m_server_text, // editing a server, can't change name
                      XmNsensitive, FALSE,
                      NULL);
        IMAP_CopyCharPref(server_name, USERNAME,&user_name);
        IMAP_GetBoolPref(server_name, REMEMBER_PASSWORD,&remember_password);
        IMAP_GetBoolPref(server_name, CHECK_NEW_MAIL,&check_mail);
        IMAP_GetIntPref(server_name, CHECK_TIME, &check_time);
    } else {
        PREF_CopyCharPref(POP_NAME,&user_name);
        PREF_GetBoolPref(POP_REMEMBER_PASSWORD,&remember_password);
        PREF_GetBoolPref(POP_CHECK_NEW_MAIL,&check_mail);
        PREF_GetIntPref(POP_CHECK_TIME, &check_time);
        
        XP_ASSERT(m_pop_button);
        
        // make sure we can always go from POP->IMAP

    }        


    if (user_name) fe_SetTextField(m_server_user_name,user_name);
    XmToggleButtonGadgetSetState(m_remember_password, remember_password,TRUE);
    XmToggleButtonGadgetSetState(m_check_mail, check_mail,TRUE);

    char *check_time_buf=PR_smprintf("%d",check_time);
    fe_SetTextField(m_check_time,check_time_buf);
    XP_FREE(check_time_buf);
    XP_FREE(user_name);
        
}

void 
XFE_PrefsMServerGeneralTab::cb_optionImap(Widget    /* w */,
										 XtPointer clientData,
                                          XtPointer /* callData */)
{
    XFE_PrefsMServerGeneralTab *tab=(XFE_PrefsMServerGeneralTab*)clientData;
    if (!tab) return;
    tab->m_is_imap=TRUE;
    if (!tab->m_imap_callback) return;
    tab->m_imap_callback(tab->m_imap_closure);
}

void 
XFE_PrefsMServerGeneralTab::cb_optionPop(Widget    /* w */,
										 XtPointer clientData,
										 XtPointer /* callData */)
{
    XFE_PrefsMServerGeneralTab *tab=(XFE_PrefsMServerGeneralTab*)clientData;
    if (!tab) return;
    tab->m_is_imap=FALSE;
    if (!tab->m_pop_callback) return;
    tab->m_pop_callback(tab->m_pop_closure);
}

void
XFE_PrefsMServerGeneralTab::setIMAPCallback(closureCallback cb, void* closure)
{
    m_imap_callback=cb;
    m_imap_closure=closure;
}

void
XFE_PrefsMServerGeneralTab::setPOPCallback(closureCallback cb, void* closure)
{
    m_pop_callback=cb;
    m_pop_closure=closure;
}


void
XFE_PrefsMServerGeneralTab::apply()
{

    // pull actual values out of the widgets
	char *server_name         = getServerName();	
	char *user_name           = getUserName();
    XP_Bool remember_password = getRememberPassword();
    XP_Bool check_mail        = getCheckMail();
    int32 check_time           = getWaitTime();
    MSG_Master *master        = fe_getMNMaster();
    if (m_is_imap) {
        // create a new server if:
        // m_is_new - this is a new server (from New button)
        // !m_originally_imap - going POP->IMAP need to create IMAP structs
        if (m_is_new) {
            // New server - set defaults for what we don't know
            MSG_CreateIMAPHost(master,
                               server_name,
                               FALSE, // isSecure
                               user_name, 
                               check_mail, 
                               check_time,
                               remember_password,
                               TRUE,
                               TRUE,
                               "",
                               "",
                               "");
        }

        PREF_SetIntPref("mail.server_type", TYPE_IMAP);
        PREF_SetCharPref("network.hosts.pop_server", server_name);
        IMAP_SetCharPref(server_name, USERNAME, user_name);
        IMAP_SetBoolPref(server_name, REMEMBER_PASSWORD, remember_password);
        IMAP_SetBoolPref(server_name, CHECK_NEW_MAIL, check_mail);
        if (check_mail)
            IMAP_SetIntPref(server_name, CHECK_TIME, check_time);
            
    } else {

        PREF_SetIntPref("mail.server_type", TYPE_POP);
        PREF_SetCharPref("network.hosts.pop_server", server_name);
        PREF_SetCharPref(POP_NAME, user_name);
        PREF_SetBoolPref(POP_REMEMBER_PASSWORD, remember_password);
        PREF_SetBoolPref(POP_CHECK_NEW_MAIL, check_mail);
        if (check_mail)
            PREF_SetIntPref(POP_CHECK_TIME, check_time);
    }
        
	// Free all the strings
	XtFree(server_name);
	XtFree(user_name); 

}

// Create the body of the Imap Tab
XFE_PrefsMServerIMAPTab::XFE_PrefsMServerIMAPTab(
	XFE_Component *top,
	XFE_View *view)
	: XFE_PropertyTabView(top, view, XFE_IMAP_TAB),
      m_delete_trash_toggle(0),
      m_delete_mark_toggle(0),
      m_delete_remove_toggle(0),
      m_use_ssl(0),
      m_use_sub(0)
{
    create();
}

void
XFE_PrefsMServerIMAPTab::create() {
	Widget			kids[10];
	int				i  = 0;
	Widget			form;
    Widget          delete_radiobox;
    Widget          delete_label;
    
	form = getBaseWidget();

    kids[i++] = delete_label =
        XmCreateLabelGadget(form, "deleteLabel", NULL, 0);
    kids[i++] = delete_radiobox =
        XmCreateRadioBox(form, "deleteRadioBox", NULL, 0);
    
	m_delete_trash_toggle =
		XmCreateToggleButtonGadget(delete_radiobox, "trashToggle", NULL, 0);
    m_delete_mark_toggle =
		XmCreateToggleButtonGadget(delete_radiobox, "markToggle", NULL, 0);
    m_delete_remove_toggle =
		XmCreateToggleButtonGadget(delete_radiobox, "removeToggle", NULL, 0);
        

	kids[i++] = m_use_ssl = 
		XmCreateToggleButtonGadget(form, "UseSSL", NULL, 0);
#if 0
    kids[i++] = m_use_sub =
        XmCreateToggleButtonGadget(form, "UseSub", NULL, 0);
#endif    

    
    XtVaSetValues(delete_label,
				  XmNtopAttachment, XmATTACH_FORM,
                  XmNleftAttachment, XmATTACH_FORM,
                  NULL);
                  
	XtVaSetValues(delete_radiobox,
				  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, delete_label,
				  XmNleftAttachment, XmATTACH_FORM,
				  NULL);

	XtVaSetValues(m_use_ssl,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, delete_radiobox,
                  XmNleftAttachment, XmATTACH_FORM,
				  NULL);
#if 0
    XtVaSetValues(m_use_sub,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, m_use_ssl,
                  XmNleftAttachment, XmATTACH_FORM,
                  NULL);
#endif

	// Manage the kids
    XtManageChild(m_delete_trash_toggle);
    XtManageChild(m_delete_mark_toggle);
    XtManageChild(m_delete_remove_toggle);
	XtManageChildren(kids, i);
}

void
XFE_PrefsMServerIMAPTab::init(char *server_name) {
    
    if (!server_name) return;

    XP_Bool is_secure, use_sub;
    int32 intval;

    IMAP_GetIntPref(server_name, DELETE_MODEL, &intval);
    IMAP_GetBoolPref(server_name, IS_SECURE, &is_secure);
    IMAP_GetBoolPref(server_name, USE_SUB, &use_sub);

    MSG_IMAPDeleteModel delete_model=(MSG_IMAPDeleteModel)intval;
    
    switch (delete_model) {
    case MSG_IMAPDeleteIsIMAPDelete:
        XmToggleButtonGadgetSetState(m_delete_mark_toggle, True, True);
        break;
    case MSG_IMAPDeleteIsMoveToTrash:
        XmToggleButtonGadgetSetState(m_delete_trash_toggle, True, True);
        break;
    case MSG_IMAPDeleteIsDeleteNoTrash:
        XmToggleButtonGadgetSetState(m_delete_remove_toggle, True, True);
        break;
    }

    XmToggleButtonGadgetSetState(m_use_ssl, is_secure,TRUE);
#if 0
    XmToggleButtonGadgetSetState(m_use_sub, use_sub,TRUE);
#endif

}

void
XFE_PrefsMServerIMAPTab::apply(char *server_name)
{
    XP_ASSERT(server_name);
    if (!server_name) return;

    int32 intval=(int32)MSG_IMAPDeleteIsMoveToTrash;
    
    if (XmToggleButtonGadgetGetState(m_delete_mark_toggle))
        intval=(int32)MSG_IMAPDeleteIsIMAPDelete;
    else if (XmToggleButtonGadgetGetState(m_delete_trash_toggle))
        intval=(int32)MSG_IMAPDeleteIsMoveToTrash;
    else if (XmToggleButtonGadgetGetState(m_delete_remove_toggle))
        intval=(int32)MSG_IMAPDeleteIsDeleteNoTrash;

    IMAP_SetIntPref(server_name, DELETE_MODEL, intval);
	IMAP_SetBoolPref(server_name, IS_SECURE, getImapSsl());
    IMAP_SetBoolPref(server_name, USE_SUB, getUseSub());

}

// Create the body of the Advanced Tab
XFE_PrefsMServerAdvancedTab::XFE_PrefsMServerAdvancedTab(
	XFE_Component *top,
	XFE_View *view)
	: XFE_PropertyTabView(top, view, XFE_ADVANCED_TAB),
       m_path_prefs_label(0),
	 m_personal_dir_label(0),
	 m_personal_dir_text(0),
	 m_public_dir_label(0),
	 m_public_dir_text(0),
	 m_other_label(0),
	 m_other_text(0),
	 m_allow_server(0)
{
    create();
}

void XFE_PrefsMServerAdvancedTab::create() {
	Widget			kids[10];
	Arg				av[10];
	int				ac = 0, i  = 0;
	Widget			form;

	form = getBaseWidget();

	kids[i++] = m_path_prefs_label = 
		XmCreateLabelGadget(form, "PathPrefsLabel", av, ac);

	kids[i++] = m_personal_dir_label = 
		XmCreateLabelGadget(form, "PersonalDir", av, ac);

	kids[i++] = m_personal_dir_text =
		fe_CreateTextField(form, "PersonalDirText", av, ac);

	kids[i++] = m_public_dir_label = 
		XmCreateLabelGadget(form, "PublicDir", av, ac);

	kids[i++] = m_public_dir_text =
		fe_CreateTextField(form, "PublicDirText", av, ac);

	kids[i++] = m_other_label = 
		XmCreateLabelGadget(form, "OtherUsers", av, ac);

	kids[i++] = m_other_text =
		fe_CreateTextField(form, "OtherUsersText", av, ac);

	kids[i++] = m_allow_server = 
		XmCreateToggleButtonGadget(form, "AllowServer", av, ac);
	
	// Specify the geometry constraints

	XtVaSetValues(m_path_prefs_label,
                  XmNalignment, XmALIGNMENT_BEGINNING,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_FORM,
				  NULL);

	XtVaSetValues(m_personal_dir_label,
				  XmNalignment, XmALIGNMENT_END,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_personal_dir_text,
				  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNbottomWidget, m_personal_dir_text,
				  NULL);

	XtVaSetValues(m_personal_dir_text,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_path_prefs_label,
				  NULL);

	XtVaSetValues(m_public_dir_label,
				  XmNalignment, XmALIGNMENT_END,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_public_dir_text,
				  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNbottomWidget, m_public_dir_text,
				  NULL);

	XtVaSetValues(m_public_dir_text,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_personal_dir_text,
				  NULL);

	XtVaSetValues(m_other_label,
				  XmNalignment, XmALIGNMENT_END,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNtopWidget, m_other_text,
				  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				  XmNbottomWidget, m_other_text,
				  NULL);

	XtVaSetValues(m_other_text,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_public_dir_text,
				  NULL);

	XtVaSetValues(m_allow_server,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, m_other_label,
				  NULL);

	// Manage the kids
	XtManageChildren(kids, i);
}

void
XFE_PrefsMServerAdvancedTab::init(char *server_name) {
    if (!server_name) return;
                          
    char *personal_dir=NULL,
        *public_dir=NULL,
        *other_dir=NULL;
    XP_Bool override=FALSE;

    if (IMAP_CopyCharPref(server_name, NS_PERSONAL, &personal_dir)==PREF_OK) {
        fe_SetTextField(m_personal_dir_text, personal_dir);
        XP_FREE(personal_dir);
    }
    
    if (IMAP_CopyCharPref(server_name, NS_PUBLIC, &public_dir)==PREF_OK) {
        fe_SetTextField(m_public_dir_text, public_dir);
        XP_FREE(public_dir);
    }
    if (IMAP_CopyCharPref(server_name, NS_OTHER, &other_dir)==PREF_OK) {
        fe_SetTextField(m_other_text, other_dir);
        XP_FREE(other_dir);
    }
    
    IMAP_GetBoolPref (server_name, OVERRIDE_NAMESPACES, &override);
    XmToggleButtonGadgetSetState(m_allow_server, override,TRUE);

}


void
XFE_PrefsMServerAdvancedTab::apply(char *server_name)
{

    XP_ASSERT(server_name);
    if (!server_name) return;
    
	char *personal_dir = getImapPersonalDir();
	char *public_dir = getImapPublicDir();
	char *others_dir = getImapOthersDir();
	XP_Bool override = getOverrideNamespaces();

	IMAP_SetCharPref(server_name, NS_PERSONAL, personal_dir);
	IMAP_SetCharPref(server_name, NS_PUBLIC, public_dir);
	IMAP_SetCharPref(server_name, NS_OTHER, others_dir);
	IMAP_SetBoolPref(server_name, OVERRIDE_NAMESPACES, override);

	XtFree(personal_dir);
	XtFree(public_dir); 
	XtFree(others_dir);

}


// Create the body of the POP Tab
XFE_PrefsMServerPOPTab::XFE_PrefsMServerPOPTab(XFE_Component *top,
											   XFE_View *view)
	: XFE_PropertyTabView(top, view, XFE_POP_TAB),
      m_leave_messages(0)
{
    create();
}

void
XFE_PrefsMServerPOPTab::create() {

	Widget			kids[10];
	Arg				av[10];
	int				ac = 0, i  = 0;
	Widget			form;

	form = getBaseWidget();

	kids[i++] = m_leave_messages = 
		XmCreateToggleButtonGadget(form, "LeaveMessages", av, ac);

	XtVaSetValues(m_leave_messages,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_FORM,
				  NULL);

	// Manage the kids
	XtManageChildren(kids, i);
}

void XFE_PrefsMServerPOPTab::init() {
    XP_Bool leave_messages;
    PREF_GetBoolPref(POP_LEAVE_ON_SERVER, &leave_messages);
    XmToggleButtonGadgetSetState(m_leave_messages, leave_messages,TRUE);
}

void
XFE_PrefsMServerPOPTab::apply()
{
	PREF_SetBoolPref(POP_LEAVE_ON_SERVER, getLeaveMessages());
}
