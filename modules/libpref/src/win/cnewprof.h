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
 
// cnewprof.h : header file
//

#include "afxwin.h"
#include <afxdlgs.h>
#include "res/resource.h"

#include "cprofile.h"
#include "xp_mcom.h"


/////////////////////////////////////////////////////////////////////////////
// CNewProfileSheet propery sheet

class CNewProfileSheet : public CPropertySheet
{
	DECLARE_DYNCREATE(CNewProfileSheet)

protected:
    XP_Bool             m_bUpgrade;
    int                 m_Style;
    CProfile            *m_Profile;
	CString	            m_Username;
	CString	            m_UserEmail;
	CString	            m_ProfileName;
	CString	            m_ProfileDirectory;
	CString	            m_MailServer;
	CString	            m_MailUsername;
    int                 m_MailServerType;
	CString	            m_SMTPServer;
	CString	            m_NNTPServer;
	UINT	            m_NNTPPort;
    XP_Bool             m_NNTPIsSecure;
    uint16              m_UpgradeInfo;
    int                 m_ReturnCode;

// Construction
public:
    CNewProfileSheet();
    CNewProfileSheet(CProfile *pWorkingProfile, int style=0, XP_Bool upgradeUser = FALSE);
    virtual ~CNewProfileSheet();

// Implementation
public:
    XP_Bool             IsUpgrading(void)          {return m_bUpgrade;}
    XP_Bool             CopyUpgradedFiles(void)    {return (m_UpgradeInfo & PROFILE_COPY_UPGRADED_FILES);} 

    int                 GetStyle(void)             {return m_Style;}
    CProfile            *GetProfile(void)          {return m_Profile;}
    CString             GetUserName(void)          {return m_Username;}
    CString             GetUserEmail(void)         {return m_UserEmail;}
    CString             GetProfileName(void)       {return m_ProfileName;}
    CString             GetProfileDirectory(void)  {return m_ProfileDirectory;}
    CString             GetMailServer(void)        {return m_MailServer;}
    CString             GetMailUsername(void)      {return m_MailUsername;}
    int                 GetMailServerType(void)    {return m_MailServerType;}
    CString             GetSMTPServer(void)        {return m_SMTPServer;}
    CString             GetNNTPServer(void)        {return m_NNTPServer;}
    int                 GetNNTPPort(void)          {return m_NNTPPort;}
    XP_Bool             GetNewsIsSecure(void)      {return m_NNTPIsSecure;}

    PROFILE_ERROR       DoDialog(void);

protected:    
    void                SetUserName(CString username)       {m_Username = username;}
    void                SetUserEmail(CString email)         {m_UserEmail = email;}
    void                SetProfileName(CString profileName) {m_ProfileName = profileName;}
    void                SetProfileDirectory(CString profileDir) {m_ProfileDirectory = profileDir;}
    void                SetMailServer(CString serverName)   {m_MailServer = serverName;}
    void                SetMailUsername(CString name)       {m_MailUsername = name;}
    void                SetMailServerType(int type)         {m_MailServerType = type;}
    void                SetSMTPServer(CString serverName)   {m_SMTPServer = serverName;}
    void                SetNNTPServer(CString serverName)   {m_NNTPServer = serverName;}
    void                SetNNTPIsSecure(XP_Bool secure)     {m_NNTPIsSecure = secure;}
    void                SetNNTPPort(int port)               {m_NNTPPort = port;}
    void                SetUpgradeFlag(XP_Bool upgrading)   {m_bUpgrade = upgrading;}
    void                SetUpgradeInfo(int upgradeInfo)     {m_UpgradeInfo = upgradeInfo;}
    void                SetReturnCode(int returnCode)       {m_ReturnCode = returnCode;}


    friend class CNewProfileIdentityPage;
    friend class CNewProfileIntroPage;
    friend class CNewProfileNetIntroPage;
    friend class CNewProfileDirectoryPage;
    friend class CNewProfileMailPage;
    friend class CNewProfileNNTPPage;
    friend class CNewProfileSMTPPage;
    friend class CNewProfileNetIdentityPage;
    friend class CNewProfileRemoteIntroPage;
    friend class CNewProfileCountryPage;
    friend class CNewProfileUpgradePage;
};

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIdentityPage dialog

class CNewProfileIdentityPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileIdentityPage)

// Construction
public:
	CNewProfileIdentityPage();
	~CNewProfileIdentityPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileIdentityPage)
	enum { IDD = IDD_NEWPROF_NAME };
	CString	m_Username;
	CString	m_UserEmail;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileIdentityPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileIdentityPage)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIntroPage dialog

class CNewProfileIntroPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileIntroPage)

// Construction
public:
	CNewProfileIntroPage();
	~CNewProfileIntroPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileIntroPage)
	enum { IDD = IDD_NEWPROF_INTRO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileIntroPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileIntroPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIntroPage dialog

class CNewProfileNetIntroPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileNetIntroPage)

// Construction
public:
	CNewProfileNetIntroPage();
	~CNewProfileNetIntroPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileNetIntroPage)
	enum { IDD = IDD_NEWPROF_NETINTRO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileNetIntroPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileNetIntroPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CNewProfileDirectoryPage dialog

class CNewProfileDirectoryPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileDirectoryPage)
protected:


// Construction
public:
	CNewProfileDirectoryPage();
	~CNewProfileDirectoryPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileDirectoryPage)
	enum { IDD = IDD_NEWPROF_DIRS };
	CEdit	m_NameControl;
	CEdit	m_DirControl;
	CString	m_ProfileName;
	CString	m_ProfileDirectory;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileDirectoryPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileDirectoryPage)
	afx_msg void OnUpdateProfileName();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileMailPage dialog

class CNewProfileMailPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileMailPage)

// Construction
public:
	CNewProfileMailPage();
	~CNewProfileMailPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileMailPage)
	enum { IDD = IDD_NEWPROF_MSERVER };
	CString	m_MailServer;
	CString	m_MailUsername;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileMailPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileMailPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileNNTPPage dialog

class CNewProfileNNTPPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileNNTPPage)

// Construction
public:
	CNewProfileNNTPPage();
	~CNewProfileNNTPPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileNNTPPage)
	enum { IDD = IDD_NEWPROF_NNTP };
	CEdit	m_NNTPPortEditField;
	CButton	m_SecureCheckbox;
	UINT	m_NNTPPort;
	CString	m_NNTPServer;
	BOOL	m_IsSecure;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileNNTPPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileNNTPPage)
	afx_msg void OnCheckSecure();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileSMTPPage dialog

class CNewProfileSMTPPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileSMTPPage)

// Construction
public:
	CNewProfileSMTPPage();
	~CNewProfileSMTPPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileSMTPPage)
	enum { IDD = IDD_NEWPROF_SMTP };
	CString	m_SMTPServer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileSMTPPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileSMTPPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIdentityPage dialog

class CNewProfileNetIdentityPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileNetIdentityPage)

// Construction
public:
	CNewProfileNetIdentityPage();
	~CNewProfileNetIdentityPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileNetIdentityPage)
	enum { IDD = IDD_NEWPROF_NETNAME };
	CString	m_UserEmail;
	CString	m_Username;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileNetIdentityPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileNetIdentityPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileRemoteIntroPage dialog

class CNewProfileRemoteIntroPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileRemoteIntroPage)

// Construction
public:
	CNewProfileRemoteIntroPage();
	~CNewProfileRemoteIntroPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileRemoteIntroPage)
	enum { IDD = IDD_NEWPROF_REMOTEINTRO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileRemoteIntroPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileRemoteIntroPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileCountryPage dialog

class CNewProfileCountryPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileCountryPage)

// Construction
public:
	CNewProfileCountryPage();
	~CNewProfileCountryPage();

// Dialog Data
	//{{AFX_DATA(CNewProfileCountryPage)
	enum { IDD = IDD_NEWPROF_COUNTRY };
	CString	m_SelectedCountry;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileCountryPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileCountryPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CNewProfPasswordDlg dialog

class CNewProfPasswordDlg : public CDialog
{
// Construction
public:
	CNewProfPasswordDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewProfPasswordDlg)
	enum { IDD = IDD_PROF_PWONLY };
	CString	m_Password;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewProfPasswordDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    char    *GetSuppliedPassword(void);

protected:

	// Generated message map functions
	//{{AFX_MSG(CNewProfPasswordDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CNewProfileUpgradePage dialog

class CNewProfileUpgradePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewProfileUpgradePage)

// Construction
public:
	CNewProfileUpgradePage();
	~CNewProfileUpgradePage();

// Dialog Data
	//{{AFX_DATA(CNewProfileUpgradePage)
	enum { IDD = IDD_NEWPROF_UPGRADE };
	CButton	m_RadioMove;
	CButton	m_RadioCopy;
	CButton	m_RadioNew;
	CStatic	m_NextText;
	CStatic	m_FinishText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewProfileUpgradePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewProfileUpgradePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioCopy();
	afx_msg void OnRadioMove();
	afx_msg void OnRadioNew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
