/* -*- Mode: C++; tab-width: 4 -*- */
/*
   PrefsDialog.cpp -- class for XFE preferences dialogs.
   Created: Alec Flett <alecf@netscape.com>
 */


#ifndef _xfe_prefspagenserver_h
#define _xfe_prefspagenserver_h

#include <Xm/Xm.h>
#include "PrefsDialog.h"
#include "PrefsDialogNServer.h" 
#include "msgcom.h"

class XFE_PrefsPageNServer : public XFE_PrefsPage
{
 public:
    
    XFE_PrefsPageNServer(XFE_PrefsDialog *dialog);
    virtual ~XFE_PrefsPageNServer();

    virtual void create();
    virtual void init();
    virtual void save();
    virtual void install();
    virtual Boolean verify();


    // widget callbacks
    static void cb_addServer(Widget, XtPointer, XtPointer);
    static void cb_editServer(Widget, XtPointer, XtPointer);
    static void cb_deleteServer(Widget, XtPointer, XtPointer);
    static void cb_setDefault(Widget, XtPointer, XtPointer);
    static void cb_serverSelected(Widget, XtPointer, XtPointer);
    static void cb_chooseDir(Widget, XtPointer, XtPointer);
    
    void addServer();
    void editServer();
    void deleteServer();
    void setDefault();
    void serverSelected();
    void chooseDir();

    // utility functions
    void refreshList();
    MSG_Host *getSelected();
    void saveServer();
 private:

    Widget createServerListFrame(Widget, Widget);
    Widget createLocalFrame(Widget, Widget);

    Widget m_server_list;
    Widget m_directory_text;
    Widget m_directory_button;
    Widget m_size_limit_toggle;
    Widget m_size_limit_text;
    
    MSG_NewsHost **m_newsHosts;
    XFE_PrefsNServerDialog *m_newsDialog;
};

#endif    

