
#ifndef _xfe_subupgradedialog_h
#define _xfe_subupgradedialog_h

#include "Dialog.h"
#include "msgcom.h"
#include "Xm/Xm.h"

class XFE_SubUpgradeDialog: public XFE_Dialog
{
 public:
    XFE_SubUpgradeDialog(MWContext *context,
                         const char *hostName);
    
    virtual ~XFE_SubUpgradeDialog();
    
    MSG_IMAPUpgradeType prompt();

    void create();
    void init();
    
    
 private:
    void ok();
    void cancel();
    static void cb_ok(Widget, XtPointer, XtPointer);
    static void cb_cancel(Widget, XtPointer, XtPointer);

    Widget m_automatic_toggle;
    Widget m_custom_toggle;
    
    MSG_IMAPUpgradeType m_retVal;

    XP_Bool m_doneWithLoop;
    const char *m_hostname;
};

#endif
