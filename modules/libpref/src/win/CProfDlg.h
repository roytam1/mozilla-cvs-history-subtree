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
 
// UserProfileDialog.h : header file
//

#include "res/resource.h"
#include "CProfLst.h"

/////////////////////////////////////////////////////////////////////////////
// CUserProfileDialog dialog

class CUserProfileDialog : public CDialog
{
protected:
    char *m_DefaultSelection;
    void *m_ProfileItems;
    void *m_SelectedProfile;

// Construction
public:
	CUserProfileDialog(void *pItems, CWnd* pParent = NULL);   // standard constructor
    virtual ~CUserProfileDialog();

// Dialog Data
	//{{AFX_DATA(CUserProfileDialog)
	enum { IDD = IDD_LOGIN_DIALOG };
	CEdit	m_PasswordField;
	CProfilesComboBox	m_Profiles;
	CString	m_Password;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserProfileDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    virtual void        AddEntry(const char *name, int style, void *data);
    virtual void *      GetSelectedProfile(void);
    virtual char *      GetSuppliedPassword(void);

    virtual void        SetDefaultName(const char *name);
    virtual void        SelectProfileName(const char *name);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUserProfileDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSelchangeProfiles();
	afx_msg void OnEditProfile();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CProfileMgrDlg dialog

class CProfileMgrDlg : public CDialog
{
protected:
    void *m_ProfileItems;
    CProfileManager *m_ProfMgr;

// Construction
public:
	CProfileMgrDlg(CProfileManager *pProfMgr, void *pItems, CWnd* pParent = NULL);   // standard constructor

    virtual void        AddEntry(const char *name, int style, void *data);

// Dialog Data
	//{{AFX_DATA(CProfileMgrDlg)
	enum { IDD = IDD_LOGIN_PROFMGR };
	CButton	m_ChangePWBtn;
	CButton	m_DeleteBtn;
	CButton	m_RenameBtn;
	CProfilesListBox	m_ProfileList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProfileMgrDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProfileMgrDlg)
	afx_msg void OnRenameProfile();
	afx_msg void OnDeleteProfile();
	afx_msg void OnChangeProfilePW();
	afx_msg void OnNewProfile();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void    PopulateList(void);
    void    RenameProfile(void);
    void    DeleteProfile(int nIndex);
};
