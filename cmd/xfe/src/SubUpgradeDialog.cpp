/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "felocale.h"
#include "xfe.h"
#include "SubUpgradeDialog.h"

extern "C" MSG_IMAPUpgradeType
fe_promptIMAPSubscriptionUpgrade(MWContext *context,
                                    const char *hostName)
{
    MSG_IMAPUpgradeType retval;
    XFE_SubUpgradeDialog *sud = new XFE_SubUpgradeDialog(context,hostName);

    retval=sud->prompt();
    delete sud;

    return retval;
}

XFE_SubUpgradeDialog::XFE_SubUpgradeDialog(MWContext *context,
                                           const char *hostName)
    :XFE_Dialog(CONTEXT_WIDGET(context),
                "SubUpgradeDialog",TRUE, TRUE, FALSE, FALSE, FALSE), // ok and cancel
     m_retVal(MSG_IMAPUpgradeDont),
     m_doneWithLoop(0)
{
    create();
    init();
}
XFE_SubUpgradeDialog::~XFE_SubUpgradeDialog() {

}

void XFE_SubUpgradeDialog::create()
{

    Widget form;
    Widget paragraph_label;

    Widget selection_radiobox;

    form=XmCreateForm(m_chrome, "form", NULL,0);
    
    paragraph_label=XmCreateLabelGadget(form, "paragraphLabel", NULL,0);
    selection_radiobox=XmCreateRadioBox(form, "selectionBox", NULL,0);
    
    m_automatic_toggle=XmCreateToggleButtonGadget(selection_radiobox,"automaticToggle",NULL,0);
    m_custom_toggle=XmCreateToggleButtonGadget(selection_radiobox,"customToggle",NULL,0);

    XtVaSetValues(paragraph_label,
                  XmNtopAttachment, XmATTACH_FORM,
                  XmNalignment, XmALIGNMENT_BEGINNING,
                  NULL);

    XtVaSetValues(selection_radiobox,
                  XmNtopAttachment, XmATTACH_WIDGET,
                  XmNtopWidget, paragraph_label,
                  XmNleftAttachment, XmATTACH_FORM,
                  XmNrightAttachment, XmATTACH_FORM,
                  XmNbottomAttachment, XmATTACH_FORM,
                  NULL);

    XtManageChild(m_custom_toggle);
    XtManageChild(m_automatic_toggle);
    XtManageChild(selection_radiobox);
    XtManageChild(paragraph_label);
    XtManageChild(form);

    XtAddCallback(m_chrome, XmNokCallback, cb_ok, this);
    XtAddCallback(m_chrome, XmNcancelCallback, cb_cancel, this);
    
}

void XFE_SubUpgradeDialog::init()
{
    XmToggleButtonGadgetSetState(m_automatic_toggle, True, True);
}

void XFE_SubUpgradeDialog::cb_ok(Widget, XtPointer closure, XtPointer)
{
    ((XFE_SubUpgradeDialog*)closure)->ok();
}



void XFE_SubUpgradeDialog::cb_cancel(Widget, XtPointer closure, XtPointer)
{
    ((XFE_SubUpgradeDialog*)closure)->cancel();
}

void XFE_SubUpgradeDialog::ok() {
    if (XmToggleButtonGadgetGetState(m_automatic_toggle))
        m_retVal=MSG_IMAPUpgradeAutomatic;
    else if (XmToggleButtonGadgetGetState(m_custom_toggle))
        m_retVal=MSG_IMAPUpgradeCustom;
    
    m_doneWithLoop=True;
    
}

void XFE_SubUpgradeDialog::cancel() {
    m_doneWithLoop=True;
}

MSG_IMAPUpgradeType XFE_SubUpgradeDialog::prompt() {
    m_doneWithLoop=False;
    show();
    while (!m_doneWithLoop)
        fe_EventLoop();

    hide();
    return m_retVal;
}
