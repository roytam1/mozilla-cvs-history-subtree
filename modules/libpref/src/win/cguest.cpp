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
 
// cguest.cpp : implementation file
//

#define _NSPR_NO_WINDOWS_H
#include "cguest.h"
#include "prefapi.h"
#include "cremote.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProfileGuestLogin dialog


CProfileGuestLogin::CProfileGuestLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CProfileGuestLogin::IDD, pParent)
{
    PROFILE_ERROR   err;
    int             bufLen = 255;
    char            buffer[255];

    m_Maximized = FALSE;

	//{{AFX_DATA_INIT(CProfileGuestLogin)
	m_LDAPServer = _T("");
	m_LDAPSearchBase = _T("");
	m_HTTPServer = _T("");
	m_StoreLocalProfile = FALSE;
	m_Password = _T("");
	m_Username = _T("");
	m_AddressBook = FALSE;
	m_Bookmarks = FALSE;
	m_Cookies = FALSE;
	m_MailFilters = FALSE;
	m_Java = FALSE;
	m_Security = FALSE;
	m_History = FALSE;
	m_Prefs = FALSE;
	//}}AFX_DATA_INIT

    err = PREF_GetCharPref("li.server.ldap.url", buffer, &bufLen);
    if (err == PREF_OK) {
        m_LDAPServer = buffer;
    }

    err = PREF_GetCharPref("li.server.ldap.userbase", buffer, &bufLen);
    if (err == PREF_OK) {
        m_LDAPSearchBase = buffer;
    }

    err = PREF_GetCharPref("li.server.http.baseURL", buffer, &bufLen);
    if (err == PREF_OK) {
        m_HTTPServer = buffer;
    }

	PREF_GetBoolPref("li.client.bookmarks", &m_Bookmarks);
	PREF_GetBoolPref("li.client.cookies",  &m_Cookies);
	PREF_GetBoolPref("li.client.addressbook",  &m_AddressBook);
	PREF_GetBoolPref("li.client.globalhistory", &m_History);
	PREF_GetBoolPref("li.client.filters",  &m_MailFilters);
	PREF_GetBoolPref("li.client.javasecurity",  &m_Java);
	PREF_GetBoolPref("li.client.security",  &m_Security);
	PREF_GetBoolPref("li.client.liprefs",  &m_Prefs);

    err = PREF_GetCharPref("li.server.http.baseURL", buffer, &bufLen);
    if (err == PREF_OK) {
        m_HTTPServer = buffer;
    }

    err = PREF_GetCharPref("li.protocol", buffer, &bufLen);
    if (err == PREF_OK) {
    	if (!_stricmp(buffer,"http")) {
            m_LIProtocol = PROF_REMOTE_LI_USEHTTP;
        } else {
            m_LIProtocol = PROF_REMOTE_LI_USELDAP;
        }
    } else {
        m_LIProtocol = PROF_REMOTE_LI_USELDAP;
    }
}

CProfileGuestLogin::~CProfileGuestLogin()
{
    return;
}

static void EnableDlgItem(HWND hWnd, UINT nID, BOOL bEnable)
{
	HWND hWndItem = ::GetDlgItem(hWnd, nID);
	if (hWndItem != NULL)
		::EnableWindow(hWndItem, bEnable);
}

void CProfileGuestLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProfileGuestLogin)
	DDX_Control(pDX, IDC_HTTP_SERVER, m_HTTPRadio);
	DDX_Control(pDX, IDC_LDAP_SERVER, m_LDAPRadio);
	DDX_Control(pDX, IDC_SERVER_INFO, m_ServerGroupBox);
	DDX_Control(pDX, IDOK, m_OKBtn);
	DDX_Control(pDX, IDCANCEL, m_CancelBtn);
	DDX_Control(pDX, IDC_ADVANCED, m_AdvancedBtn);
	DDX_Control(pDX, IDC_LDAP_ADDRESS, m_LDAPServerField);
	DDX_Control(pDX, IDC_HTTP_ADDRESS, m_HTTPServerField);
	DDX_Control(pDX, IDC_LDAP_SEARCHBASE, m_LDAPSearchBaseField);
	DDX_Text(pDX, IDC_LDAP_ADDRESS, m_LDAPServer);
	DDX_Text(pDX, IDC_LDAP_SEARCHBASE, m_LDAPSearchBase);
	DDX_Text(pDX, IDC_HTTP_ADDRESS, m_HTTPServer);
	DDX_Check(pDX, IDC_STORE_LOCAL, m_StoreLocalProfile);
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	DDX_Text(pDX, IDC_EDIT_NAME, m_Username);
	DDX_Check(pDX, IDC_ADDBOOK, m_AddressBook);
	DDX_Check(pDX, IDC_BOOKMARKS, m_Bookmarks);
	DDX_Check(pDX, IDC_COOKIES, m_Cookies);
	DDX_Check(pDX, IDC_FILTERS, m_MailFilters);
	DDX_Check(pDX, IDC_JAVA, m_Java);
	DDX_Check(pDX, IDC_SECURITY_TYPE, m_Security);
	DDX_Check(pDX, IDC_HISTORY, m_History);
	DDX_Check(pDX, IDC_SUGGESTIONS, m_Prefs);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        if ((m_HTTPRadio.GetState() & 0x003) == 1) {
            m_LIProtocol = PROF_REMOTE_LI_USEHTTP;
        } else {
            m_LIProtocol = PROF_REMOTE_LI_USELDAP;
        }
    } else {
        m_LDAPRadio.SetCheck(m_LIProtocol == PROF_REMOTE_LI_USELDAP);
        m_HTTPRadio.SetCheck(m_LIProtocol == PROF_REMOTE_LI_USEHTTP);
    }
}


BEGIN_MESSAGE_MAP(CProfileGuestLogin, CDialog)
	//{{AFX_MSG_MAP(CProfileGuestLogin)
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvancedToggle)
	ON_BN_CLICKED(IDC_LDAP_SERVER, OnLDAPSelected)
	ON_BN_CLICKED(IDC_HTTP_SERVER, OnHTTPSelected)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfileGuestLogin message handlers

BOOL CProfileGuestLogin::OnInitDialog() 
{
    RECT            windowPos;

    CDialog::OnInitDialog();
    GetWindowRect(&windowPos);
    m_SizeX = windowPos.right - windowPos.left;
    m_Size2Y = windowPos.bottom - windowPos.top;

    m_AdvancedBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);

    m_MinY = windowPos.top;

    m_Size1Y = windowPos.bottom + ::GetSystemMetrics(SM_CYSIZE) + 20;
    SetWindowPos(NULL, 0, 0, m_SizeX, m_Size1Y, SWP_NOZORDER | SWP_NOMOVE);
	
    m_CancelBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);
    m_CancelBtn.SetWindowPos(NULL, windowPos.left, m_MinY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    m_OKBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);
    m_OKBtn.SetWindowPos(NULL, windowPos.left, m_MinY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    m_MaxY = windowPos.top;

    if (m_LIProtocol == PROF_REMOTE_LI_USELDAP) {
        OnLDAPSelected();
    } else {
        OnHTTPSelected();
    }
//	EnableDlgItem(m_hWnd, IDC_STORE_LOCAL, !PREF_PrefIsLocked("")); REMIND fix me
	EnableDlgItem(m_hWnd, IDC_EDIT_NAME, !PREF_PrefIsLocked("li.login.name"));
	EnableDlgItem(m_hWnd, IDC_PASSWORD, !PREF_PrefIsLocked("li.login.password"));
	EnableDlgItem(m_hWnd, IDC_LDAP_SERVER, !PREF_PrefIsLocked("li.protocol"));
	EnableDlgItem(m_hWnd, IDC_HTTP_SERVER, !PREF_PrefIsLocked("li.protocol"));
	EnableDlgItem(m_hWnd, IDC_BOOKMARKS, !PREF_PrefIsLocked("li.client.bookmarks"));
	EnableDlgItem(m_hWnd, IDC_COOKIES, !PREF_PrefIsLocked("li.client.cookies"));
	EnableDlgItem(m_hWnd, IDC_FILTERS, !PREF_PrefIsLocked("li.client.filters"));
	EnableDlgItem(m_hWnd, IDC_ADDBOOK, !PREF_PrefIsLocked("li.client.addressbook"));
	EnableDlgItem(m_hWnd, IDC_HISTORY, !PREF_PrefIsLocked("li.client.globalhistory"));
	EnableDlgItem(m_hWnd, IDC_SUGGESTIONS, !PREF_PrefIsLocked("li.client.liprefs"));
	EnableDlgItem(m_hWnd, IDC_SECURITY_TYPE, !PREF_PrefIsLocked("li.client.security"));
	EnableDlgItem(m_hWnd, IDC_JAVA, !PREF_PrefIsLocked("li.client.javasecurity"));
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProfileGuestLogin::OnAdvancedToggle() 
{
    int     advancedX, okX, cancelX;
    RECT    windowPos;

    m_AdvancedBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);
    advancedX = windowPos.left;
    
    m_OKBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);
    okX = windowPos.left;
    
    m_CancelBtn.GetWindowRect(&windowPos);
    this->ScreenToClient(&windowPos);
    cancelX = windowPos.left;
    
    if (m_Maximized) {
        m_AdvancedBtn.SetWindowPos(NULL, advancedX, m_MinY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        m_OKBtn.SetWindowPos(NULL, okX, m_MinY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        m_CancelBtn.SetWindowPos(NULL, cancelX, m_MinY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        m_ServerGroupBox.ShowWindow(SW_HIDE);
        m_LDAPRadio.ShowWindow(SW_HIDE);

        SetWindowPos(NULL, 0, 0, m_SizeX, m_Size1Y, SWP_NOZORDER | SWP_NOMOVE);

        m_Maximized = FALSE;
    } else {
        m_AdvancedBtn.SetWindowPos(NULL, advancedX, m_MaxY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        m_OKBtn.SetWindowPos(NULL, okX, m_MaxY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        m_CancelBtn.SetWindowPos(NULL, cancelX, m_MaxY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        m_ServerGroupBox.ShowWindow(SW_SHOW);
        m_LDAPRadio.ShowWindow(SW_SHOW);

        SetWindowPos(NULL, 0, 0, m_SizeX, m_Size2Y, SWP_NOZORDER | SWP_NOMOVE);

        m_Maximized = TRUE;
    }
	
}

BOOL CProfileGuestLogin::UseLDAPLI() 
{
    return (m_LIProtocol == PROF_REMOTE_LI_USELDAP);
}

BOOL CProfileGuestLogin::UseHTTPLI() 
{
    return (m_LIProtocol == PROF_REMOTE_LI_USEHTTP);
}

CString CProfileGuestLogin::GetSuppliedPassword()
{
    return m_Password;
}

void CProfileGuestLogin::OnLDAPSelected() 
{
    m_LDAPServerField.EnableWindow(!PREF_PrefIsLocked("li.server.ldap.url"));
    m_LDAPSearchBaseField.EnableWindow(!PREF_PrefIsLocked("li.server.ldap.userbase"));
	
    m_HTTPServerField.EnableWindow(FALSE);

    return;
}

void CProfileGuestLogin::OnHTTPSelected() 
{
    m_LDAPServerField.EnableWindow(FALSE);
    m_LDAPSearchBaseField.EnableWindow(FALSE);
	
    m_HTTPServerField.EnableWindow(!PREF_PrefIsLocked("li.server.http.baseURL"));
    
    return;
}


