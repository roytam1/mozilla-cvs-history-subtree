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
   PrefsPageMessages.h - Messages preference pane
   Created: Alec Flett <alecf@netscape.com>
 */

#ifndef _xfe_prefspagemessages_h
#define _xfe_prefspagemessages_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"

// positions of the autoquote styles inthe DtComboBox
// these must be increasing and added to the combobox int this order
#define AUTOQUOTE_PREF_BELOW 0
#define AUTOQUOTE_PREF_ABOVE 1
#define AUTOQUOTE_PREF_SELECT 2
#define AUTOQUOTE_ITEMS 3

#define FORWARD_PREF_ATTACH 0
#define FORWARD_PREF_QUOTED 1
#define FORWARD_PREF_INLINE 2
#define FORWARD_ITEMS 3

class XFE_PrefsPageMessages : public XFE_PrefsPage
{
 public:

    XFE_PrefsPageMessages(XFE_PrefsDialog *dialog);
    virtual ~XFE_PrefsPageMessages();

    virtual void create();

    virtual void init();
    virtual void install();

    virtual void save();

    virtual Boolean verify();
    
 private:

    Widget createReplyFrame(Widget parent, Widget attachTo);
    Widget createSpellFrame(Widget parent, Widget attachTo);
    Widget createWrapFrame(Widget parent, Widget attachTo);
    Widget createEightBitFrame(Widget parent, Widget attachTo);

    Widget m_forward_combo;
    
    Widget m_autoquote_toggle;
    Widget m_autoquote_style_combo;

    Widget m_spellcheck_toggle;

    Widget m_wrap_toggle;
    Widget m_wrap_length_text;

    Widget m_eightbit_asis_toggle;
    Widget m_eightbit_quoted_toggle;

    XmString m_autoquote_strings[AUTOQUOTE_ITEMS+1];
    XmString m_forward_strings[FORWARD_ITEMS+1];
};

#endif
