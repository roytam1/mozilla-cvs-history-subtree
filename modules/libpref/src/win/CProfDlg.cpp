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
 
// CProfDlg.cpp : implementation file
//

#define _NSPR_NO_WINDOWS_H
#include <afx.h>
#include "resource.h"
#include "CProfMgr.h"
#include "CProfDlg.h"

#include "xp_mcom.h"
#include "xp_str.h"

#include "..\prefpriv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserProfileDialog dialog


CUserProfileDialog::CUserProfileDialog(void *pItems, CWnd* pParent /*=NULL*/)
	: CDialog(CUserProfileDialog::IDD, pParent)
{
    m_ProfileItems = pItems;
    m_SelectedProfile = NULL;
    m_DefaultSelection = NULL;

	//{{AFX_DATA_INIT(CUserProfileDialog)
	m_Password = _T("");
	//}}AFX_DATA_INIT
}

CUserProfileDialog::~CUserProfileDialog()
{
    if (m_DefaultSelection != NULL) {
        XP_FREEIF(m_DefaultSelection);
    }

    return;
}

void CUserProfileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserProfileDialog)
	DDX_Control(pDX, IDC_PASSWORD, m_PasswordField);
	DDX_Control(pDX, IDC_PROFILES, m_Profiles);
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	DDV_MaxChars(pDX, m_Password, 20);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUserProfileDialog, CDialog)
	//{{AFX_MSG_MAP(CUserProfileDialog)
	ON_CBN_SELCHANGE(IDC_PROFILES, OnSelchangeProfiles)
	ON_BN_CLICKED(IDC_PROFILE_EDIT, OnEditProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUserProfileDialog message handlers

BOOL CUserProfileDialog::OnInitDialog()
{
    CProfileManager::ProfDisplay  *pProfileItems = (CProfileManager::ProfDisplay *) m_ProfileItems;
    CProfileManager::ProfDisplay  *pCurrentItem = pProfileItems;

    CDialog::OnInitDialog();
    
    while (pCurrentItem) {
        AddEntry(pCurrentItem->profileName, pCurrentItem->style, (void *) pCurrentItem);
        pCurrentItem = pCurrentItem->next;
    }

    SelectProfileName(m_DefaultSelection);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CUserProfileDialog::AddEntry(const char *name, int style, void *pData)
{
	m_Profiles.AddEntry(name, style, pData);

	return;
}

void CUserProfileDialog::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}


void CUserProfileDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}


void *CUserProfileDialog::GetSelectedProfile()
{
    return m_SelectedProfile;
}

char *CUserProfileDialog::GetSuppliedPassword()
{
    return (char *) (LPCTSTR) m_Password;
}


void CUserProfileDialog::SetDefaultName(const char *name)
{
    if (m_DefaultSelection != NULL) {
        XP_FREEIF(m_DefaultSelection);
    }

    m_DefaultSelection = XP_STRDUP(name);
    return;
}

void CUserProfileDialog::SelectProfileName(const char *name)
{
    int     cbIndex;

    if (name) {
        cbIndex = m_Profiles.FindStringExact(-1, name); 
    }

    if ((!name) || (cbIndex == CB_ERR)) {
        cbIndex = 0;
    }

    m_Profiles.SetCurSel(cbIndex);
    OnSelchangeProfiles();

    return;
}

void CUserProfileDialog::OnSelchangeProfiles() 
{
    int                             itemIndex = m_Profiles.GetCurSel();
    CProfileManager::ProfDisplay    *pProfile;

    if (itemIndex != CB_ERR) {
        m_SelectedProfile = m_Profiles.GetProfileData(itemIndex);
        if (m_SelectedProfile == (void *) -1) {
            m_SelectedProfile = NULL;
            m_PasswordField.EnableWindow(FALSE);
        } else {
            pProfile = (CProfileManager::ProfDisplay *) m_SelectedProfile;
            if (pProfile->style == PROFINFO_GUEST) {
                m_PasswordField.EnableWindow(FALSE);
            } else {
                m_PasswordField.EnableWindow(TRUE);
            }
        }
    }
}


void CUserProfileDialog::OnEditProfile() 
{
    EndDialog(IDC_PROFILE_EDIT);
}

/////////////////////////////////////////////////////////////////////////////
// CProfileMgrDlg dialog


CProfileMgrDlg::CProfileMgrDlg(CProfileManager *pProfMgr, void *pItems, CWnd* pParent /*=NULL*/)
	: CDialog(CProfileMgrDlg::IDD, pParent)
{
    m_ProfMgr = pProfMgr;
    m_ProfileItems = pItems;

    //{{AFX_DATA_INIT(CProfileMgrDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CProfileMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProfileMgrDlg)
	DDX_Control(pDX, IDC_EDIT_PW, m_ChangePWBtn);
	DDX_Control(pDX, IDC_DELETE, m_DeleteBtn);
	DDX_Control(pDX, IDC_RENAME, m_RenameBtn);
	DDX_Control(pDX, IDC_LIST1, m_ProfileList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProfileMgrDlg, CDialog)
	//{{AFX_MSG_MAP(CProfileMgrDlg)
	ON_BN_CLICKED(IDC_RENAME, OnRenameProfile)
	ON_BN_CLICKED(IDC_DELETE, OnDeleteProfile)
	ON_BN_CLICKED(IDC_EDIT_PW, OnChangeProfilePW)
	ON_BN_CLICKED(IDC_NEW, OnNewProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfileMgrDlg message handlers

void CProfileMgrDlg::OnRenameProfile() 
{
	RenameProfile();

    m_ProfileList.ResetContent();
        PopulateList();
}

void CProfileMgrDlg::OnDeleteProfile() 
{
    int     result;
    CString profileName;

    result = m_ProfileList.GetCurSel();

    if (result != LB_ERR) {
        int     choice;

        choice = AfxMessageBox(IDS_DELETING_PROFILE, MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONEXCLAMATION, 0);

        if (choice != IDCANCEL) {
            m_ProfileList.GetText(result, profileName);
            m_ProfMgr->DeleteProfile(profileName, (choice == IDYES));

            m_ProfileList.DeleteString(result);
        }
    }
}

void CProfileMgrDlg::OnChangeProfilePW() 
{
	// TODO: Add your control notification handler code here
	
}

void CProfileMgrDlg::OnNewProfile() 
{
    EndDialog(IDC_NEW);

}

BOOL CProfileMgrDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    PopulateList();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProfileMgrDlg::PopulateList()
{
    CProfileManager::ProfDisplay  *pProfileItems = (CProfileManager::ProfDisplay *) m_ProfileItems;
    CProfileManager::ProfDisplay  *pCurrentItem = pProfileItems;

    while (pCurrentItem) {
        AddEntry(pCurrentItem->profileName, pCurrentItem->style, (void *) pCurrentItem);
        pCurrentItem = pCurrentItem->next;
    }

}

void CProfileMgrDlg::AddEntry(const char *name, int style, void *pData)
{
    if ((style != PROFINFO_SEPARATOR) && (style != PROFINFO_GUEST))
	    m_ProfileList.AddEntry(name, style, pData);

	return;
}

void CProfileMgrDlg::DeleteProfile(int nIndex)
{
    CProfileManager::ProfDisplay  *pProfileItems = (CProfileManager::ProfDisplay *) m_ProfileItems;
    CProfileManager::ProfDisplay  *pCurrentItem = pProfileItems;
    EntryInfo       *pItem;

    pItem = (EntryInfo *) m_ProfileList.GetProfileData(nIndex);

    if (pItem) {
        if (pProfileItems == (CProfileManager::ProfDisplay *) pItem->pData) {
            m_ProfileItems = ((CProfileManager::ProfDisplay *) pItem->pData)->next;
        } else {
            while (pCurrentItem && (pCurrentItem->next != (CProfileManager::ProfDisplay *) pItem->pData)) {
                pCurrentItem = pCurrentItem->next;
            }

            if (pCurrentItem) {
                pCurrentItem->next = ((CProfileManager::ProfDisplay *) pItem->pData)->next;
            }
        }
    }

    m_ProfileList.ResetContent();
    PopulateList();
}

void CProfileMgrDlg::RenameProfile()
{

}

