/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/********************************************************************/
 
#include "afxwin.h"
#include <afxdlgs.h>
#include "res/resource.h"

// cguest.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProfileGuestLogin dialog

class CProfileGuestLogin : public CDialog
{
protected:
    BOOL        m_Maximized;
    int         m_MinY, m_MaxY;
    int         m_SizeX, m_Size1Y, m_Size2Y;

// Construction
public:
	CProfileGuestLogin(CWnd* pParent = NULL);   // standard constructor
    virtual ~CProfileGuestLogin();

// Dialog Data
	//{{AFX_DATA(CProfileGuestLogin)
	enum { IDD = IDD_LOGIN_GUEST };
	CButton	m_HTTPRadio;
	CButton	m_LDAPRadio;
	CButton	m_ServerGroupBox;
	CButton	m_OKBtn;
	CButton	m_CancelBtn;
	CButton	m_AdvancedBtn;
	CEdit	m_LDAPServerField;
	CEdit	m_HTTPServerField;
	CEdit	m_LDAPSearchBaseField;
	CString	m_LDAPServer;
	CString	m_LDAPSearchBase;
	CString	m_HTTPServer;
	BOOL	m_StoreLocalProfile;
	CString	m_Password;
	CString	m_Username;
	BOOL	m_AddressBook;
	BOOL	m_Bookmarks;
	BOOL	m_Cookies;
	BOOL	m_MailFilters;
	BOOL	m_Java;
	BOOL	m_Security;
	BOOL	m_History;
	BOOL	m_Prefs;
	//}}AFX_DATA

    int     m_LIProtocol;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProfileGuestLogin)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    BOOL  UseLDAPLI(void);
    BOOL  UseHTTPLI(void);
    CString GetSuppliedPassword(void);

protected:

	// Generated message map functions
	//{{AFX_MSG(CProfileGuestLogin)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdvancedToggle();
	afx_msg void OnLDAPSelected();
	afx_msg void OnHTTPSelected();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
