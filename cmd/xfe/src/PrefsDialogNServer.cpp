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
   PrefsDialogNServer.cpp -- Multiple news server preferences dialog
   Created: Alec Flett <alecf@netscape.com>
 */

#include "PrefsDialogNServer.h"
#include "felocale.h"
#include "prprf.h"
#include "msgcom.h"
#include "Xfe/Geometry.h"

#include "xfe.h"

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>


XFE_PrefsNServerDialog::XFE_PrefsNServerDialog(Widget parent)
    : XFE_Dialog(parent, "NewsServerInfo", 
                 TRUE, TRUE, TRUE, FALSE, TRUE, TRUE),
      m_server_text(0),
      m_port_text(0),
      m_ssl_toggle(0),
      m_password_toggle(0)
{
    

}

void
XFE_PrefsNServerDialog::create() {
    Widget kids[7];
    int i=0;
    
    Widget form = XmCreateForm(m_chrome, "form", NULL, 0);
    Widget server_label = kids[i++] =
        XmCreateLabelGadget(form, "serverLabel", NULL, 0);
    m_server_text = kids[i++] =
        XmCreateText(form, "serverText", NULL, 0);
    
    Widget port_label = kids[i++] =
        XmCreateLabelGadget(form, "portLabel", NULL, 0);
    m_port_text = kids[i++] =
        XmCreateText(form, "portText", NULL, 0);

    m_ssl_toggle = kids[i++] =
        XmCreateToggleButtonGadget(form, "sslToggle", NULL, 0);
    m_password_toggle = kids[i++] =
        XmCreateToggleButtonGadget(form, "passwordToggle", NULL, 0);

    int max_height1 = XfeVaGetTallestWidget(server_label, m_server_text, NULL);
    int max_height2 = XfeVaGetTallestWidget(port_label, m_port_text, NULL);
    // now arrange the widgets
    XtVaSetValues(server_label,
                  XmNheight, max_height1,
                  XmNtopAttachment, XmATTACH_FORM,
                  XmNleftAttachment, XmATTACH_FORM,
                  NULL);
    XtVaSetValues(m_server_text,
                  XmNheight, max_height1,
                  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNtopWidget, server_label,
                  XmNleftAttachment, XmATTACH_WIDGET,
                  XmNleftWidget, server_label,
                  XmNrightAttachment, XmATTACH_FORM,
                  NULL);
    XtVaSetValues(port_label,
                  XmNheight, max_height2,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, server_label,
                  XmNleftAttachment, XmATTACH_FORM,
                  NULL);
    XtVaSetValues(m_port_text,
                  XmNheight, max_height2,
                  XmNcolumns,5,
                  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNtopWidget, port_label,
                  XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNleftWidget, m_server_text,
                  NULL);
    XtVaSetValues(m_ssl_toggle,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, m_port_text,
                  XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNleftWidget, m_port_text,
                  NULL);
    XtVaSetValues(m_password_toggle,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, m_ssl_toggle,
                  XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNleftWidget, m_ssl_toggle,
                  NULL);

    XtManageChildren(kids,i);
    XtManageChild(form);

    XtAddCallback(m_chrome, XmNokCallback, cb_ok, this);
    XtAddCallback(m_chrome, XmNcancelCallback, cb_cancel, this);
    XtAddCallback(m_ssl_toggle, XmNvalueChangedCallback, cb_ssl_toggle, this);

    setPort(NEWS_PORT);
}

void
XFE_PrefsNServerDialog::init(const char *server, int32 port,
                             XP_Bool ssl, XP_Bool password)
{

    if (m_server_text) fe_SetTextField(m_server_text, server);

    setPort(port);

    if (m_ssl_toggle)
        XmToggleButtonSetState(m_ssl_toggle, ssl, False);
    if (m_password_toggle)
        XmToggleButtonSetState(m_password_toggle, password, False);

}

void
XFE_PrefsNServerDialog::cb_cancel(Widget, XtPointer closure, XtPointer)
{
    XFE_PrefsNServerDialog *theDialog =
        (XFE_PrefsNServerDialog *)closure;

    theDialog->hide();
    theDialog->m_retVal=FALSE;
    theDialog->m_doneWithLoop=TRUE;
}

void
XFE_PrefsNServerDialog::cb_ok(Widget, XtPointer closure, XtPointer)
{
    XFE_PrefsNServerDialog *theDialog =
        (XFE_PrefsNServerDialog *)closure;

    theDialog->hide();
    theDialog->m_retVal=TRUE;
    theDialog->m_doneWithLoop=TRUE;
}

void
XFE_PrefsNServerDialog::cb_ssl_toggle(Widget,
                                      XtPointer closure,
                                      XtPointer callData)
{
    XFE_PrefsNServerDialog *theDialog =
        (XFE_PrefsNServerDialog *)closure;
    XmToggleButtonCallbackStruct *cbs =
        (XmToggleButtonCallbackStruct *)callData;
    theDialog->sslToggle(cbs);
}

void
XFE_PrefsNServerDialog::sslToggle(XmToggleButtonCallbackStruct *cbs)
{
    if (cbs->set)
        setPort(SECURE_NEWS_PORT);
    else
        setPort(NEWS_PORT);
}

XP_Bool
XFE_PrefsNServerDialog::prompt()
{

    m_doneWithLoop=False;
    show();

    while (!m_doneWithLoop)
        fe_EventLoop();

    return m_retVal;
}

void XFE_PrefsNServerDialog::setPort(int32 port)
{
    XP_ASSERT(m_ssl_toggle);
    if (!m_ssl_toggle) return;

    char *charval=PR_smprintf("%d",port);
    fe_SetTextField(m_port_text, charval);
    XP_FREE(charval);
}

char *
XFE_PrefsNServerDialog::getServerName()
{
    return fe_GetTextField(m_server_text);
}

int32
XFE_PrefsNServerDialog::getPort()
{
    char *charval = fe_GetTextField(m_port_text);
    int32 port = XP_ATOI(charval);
    XP_FREE(charval);

    return port;
}

XP_Bool
XFE_PrefsNServerDialog::getSsl()
{
    XP_ASSERT(m_ssl_toggle);
    if (!m_ssl_toggle) return FALSE;
    return (XmToggleButtonGetState(m_ssl_toggle)==True) ? TRUE : FALSE;
}

XP_Bool
XFE_PrefsNServerDialog::getPassword()
{
    XP_ASSERT(m_password_toggle);
    if (!m_password_toggle) return FALSE;
    return (XmToggleButtonGetState(m_password_toggle)==True) ? TRUE : FALSE;
}
