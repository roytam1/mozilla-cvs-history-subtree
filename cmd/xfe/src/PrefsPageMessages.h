
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

    int32 getWrapLength();

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
