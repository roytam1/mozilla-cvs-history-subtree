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
 
// cnewprof.cpp : implementation file
//

#define _NSPR_NO_WINDOWS_H
#include "cnewprof.h"
#include "cprofile.h"
#include "cprofmgr.h"
#include "../prefpriv.h"

// #include "msgcom.h"
#include "xp_mcom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewProfileSheet property sheet

IMPLEMENT_DYNCREATE(CNewProfileSheet, CPropertySheet)

CNewProfileSheet::CNewProfileSheet()
{
    m_bUpgrade = FALSE;
    m_Profile = NULL;
    m_Style = 0;
	m_Username = "";
	m_UserEmail = "";
	m_ProfileName = "";
    m_ProfileDirectory = "";
    m_MailServer = "";
    m_MailUsername = "";
    m_MailServerType = MSG_Imap4;
    m_SMTPServer = "";
    m_NNTPServer = "";
    m_NNTPPort = NEWS_PORT;
    m_NNTPIsSecure = FALSE;
    m_ReturnCode = PREF_OK;
}

CNewProfileSheet::CNewProfileSheet(CProfile *pWorkingProfile, int style, XP_Bool upgradeUser)
    : CPropertySheet()
{
    m_bUpgrade = upgradeUser;
    m_Profile = pWorkingProfile;
    m_Style = style;
	m_Username = "";
	m_UserEmail = "";
	m_ProfileName = "";
    m_ProfileDirectory = "";
    m_MailServer = "";
    m_MailUsername = "";
    m_MailServerType = MSG_Imap4;
    m_SMTPServer = "";
    m_NNTPServer = "";
    m_NNTPPort = NEWS_PORT;
    m_NNTPIsSecure = FALSE;

    m_ReturnCode = PREF_OK;
}

CNewProfileSheet::~CNewProfileSheet()
{
}

    PROFILE_ERROR
CNewProfileSheet::DoDialog()
{
    uint16                       style = (m_Style & PROFMGR_NEWPROF_TYPE_MASK);
    BOOL                         upgrade = (m_Style & PROFMGR_NEWPROF_UPGRADE);
    CPropertyPage                *pPageIntro = NULL;
    CPropertyPage                *pPageUpgrade = NULL;
    CPropertyPage                *pPageIdentity = NULL;
    CNewProfileIntroPage         pageIntro;
    CNewProfileIdentityPage      pageIdentity;
    CNewProfileDirectoryPage     pageDirectory;
    CNewProfileSMTPPage          pageSMTPInfo;
    CNewProfileMailPage          pageMailInfo;
    CNewProfileNNTPPage          pageNewsInfo;
    CNewProfileUpgradePage       pageUpgrade;
 // CNewProfileUpgradeDonePage   pageUpgradeDone;
    CNewProfileCountryPage       pageCountryPage;

    // Move member data from the view (or from the currently
    // selected object in the view, for example).
   
    switch(style) {
        case PROFMGR_NEWPROF_NETWORK:
           pPageIntro = new CNewProfileNetIntroPage();
           break;
        case PROFMGR_NEWPROF_REMOTE:
           pPageIntro = new CNewProfileRemoteIntroPage();
           break;
        default:
           pPageIntro = new CNewProfileIntroPage();
            break;
    }

    AddPage(pPageIntro);

    if (upgrade) {
        AddPage(&pageDirectory);
        AddPage(&pageUpgrade);
    }

   if (style == PROFMGR_NEWPROF_MINIMAL) {
       pPageIdentity = new CNewProfileNetIdentityPage();
       AddPage(pPageIdentity);
   } else if (style != PROFMGR_NEWPROF_REMOTE) {
       pPageIdentity = new CNewProfileIdentityPage();
   }

   if (pPageIdentity) AddPage(pPageIdentity);

   if (m_Style != PROFMGR_NEWPROF_MINIMAL) {
        if (!upgrade) {
            AddPage(&pageDirectory);
        }

       if (!upgrade) {
           if (m_Style != PROFMGR_NEWPROF_REMOTE) {
    #ifndef MOZ_LITE      
               AddPage(&pageSMTPInfo);
               AddPage(&pageMailInfo);
               AddPage(&pageNewsInfo);
    #endif
           }
       }
   }

//   AddPage(&pageCountryPage);

/*   if (upgrade)
       AddPage(&pageUpgradeDone);
*/
   SetWizardMode();

   if (DoModal() == ID_WIZFINISH)
   {
      return m_ReturnCode;
   }

   if (pPageIntro)
        delete pPageIntro;

   if (pPageIdentity)
        delete pPageIdentity;

   return PREF_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIntroPage property page

IMPLEMENT_DYNCREATE(CNewProfileIntroPage, CPropertyPage)

CNewProfileIntroPage::CNewProfileIntroPage() : CPropertyPage(CNewProfileIntroPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileIntroPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CNewProfileIntroPage::~CNewProfileIntroPage()
{
}

void CNewProfileIntroPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileIntroPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileIntroPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileIntroPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIntroPage message handlers

BOOL CNewProfileIntroPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIntroPage property page

IMPLEMENT_DYNCREATE(CNewProfileNetIntroPage, CPropertyPage)

CNewProfileNetIntroPage::CNewProfileNetIntroPage() : CPropertyPage(CNewProfileNetIntroPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileNetIntroPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CNewProfileNetIntroPage::~CNewProfileNetIntroPage()
{
}

void CNewProfileNetIntroPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileNetIntroPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileNetIntroPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileNetIntroPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIntroPage message handlers

BOOL CNewProfileNetIntroPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileRemoteIntroPage property page

IMPLEMENT_DYNCREATE(CNewProfileRemoteIntroPage, CPropertyPage)

CNewProfileRemoteIntroPage::CNewProfileRemoteIntroPage() : CPropertyPage(CNewProfileRemoteIntroPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileRemoteIntroPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CNewProfileRemoteIntroPage::~CNewProfileRemoteIntroPage()
{
}

void CNewProfileRemoteIntroPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileRemoteIntroPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileRemoteIntroPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileRemoteIntroPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileRemoteIntroPage message handlers

BOOL CNewProfileRemoteIntroPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIdentityPage property page

IMPLEMENT_DYNCREATE(CNewProfileIdentityPage, CPropertyPage)

CNewProfileIdentityPage::CNewProfileIdentityPage() : CPropertyPage(CNewProfileIdentityPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileIdentityPage)
	m_Username = _T("");
	m_UserEmail = _T("");
	//}}AFX_DATA_INIT
}

CNewProfileIdentityPage::~CNewProfileIdentityPage()
{
}

void CNewProfileIdentityPage::DoDataExchange(CDataExchange* pDX)
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileIdentityPage)
	DDX_Text(pDX, IDC_EDIT_NAME, m_Username);
	DDX_Text(pDX, IDC_EDIT_ADDRESS, m_UserEmail);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        pSheet->SetUserName(m_Username);
        pSheet->SetUserEmail(m_UserEmail);
    }

}


BEGIN_MESSAGE_MAP(CNewProfileIdentityPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileIdentityPage)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileIdentityPage message handlers

int CNewProfileIdentityPage::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());
	char             buffer[256];
	int              nLen = 255;

    if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

    if (pSheet->IsUpgrading()) {
        CWinApp     *pApp = AfxGetApp();

        CString csUserAddr = pApp->GetProfileString("User","User_Addr","DefaultUser");
		CString csFullName = pApp->GetProfileString("User","User_Name","");

		m_Username = csUserAddr;
		m_UserEmail = csFullName;

    } else {
        CProfile         *pProfile;

        pProfile = pSheet->GetProfile();
        ASSERT(pProfile);

        if (PREF_NOERROR == pProfile->GetCharPref("mail.identity.username", buffer, &nLen)) {
            m_Username = buffer;
        }

        if (PREF_NOERROR == pProfile->GetCharPref("mail.identity.useremail", buffer, &nLen)) {
            m_UserEmail = buffer;
        }
    }

	return 0;
}

BOOL CNewProfileIdentityPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIdentityPage property page

IMPLEMENT_DYNCREATE(CNewProfileNetIdentityPage, CPropertyPage)

CNewProfileNetIdentityPage::CNewProfileNetIdentityPage() : CPropertyPage(CNewProfileNetIdentityPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileNetIdentityPage)
	m_UserEmail = _T("");
	m_Username = _T("");
	//}}AFX_DATA_INIT
}

CNewProfileNetIdentityPage::~CNewProfileNetIdentityPage()
{
}

void CNewProfileNetIdentityPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileNetIdentityPage)
	DDX_Text(pDX, IDC_EDIT_ADDRESS, m_UserEmail);
	DDX_Text(pDX, IDC_EDIT_NAME, m_Username);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileNetIdentityPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileNetIdentityPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNetIdentityPage message handlers

BOOL CNewProfileNetIdentityPage::OnInitDialog() 
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());
	char             buffer[256];
	int              nLen = 255;

	CPropertyPage::OnInitDialog();
	
    if (pSheet->IsUpgrading()) {
        CWinApp     *pApp = AfxGetApp();

        CString csUserAddr = pApp->GetProfileString("User","User_Addr","DefaultUser");
		CString csFullName = pApp->GetProfileString("User","User_Name","");

		m_Username = csUserAddr;
		m_UserEmail = csFullName;

    } else {
        CProfile         *pProfile;

        pProfile = pSheet->GetProfile();
        ASSERT(pProfile);

        if (PREF_NOERROR == pProfile->GetCharPref("mail.identity.username", buffer, &nLen)) {
            m_Username = buffer;
        } else {
            /* Since this is a networked profile, try to default the user name to the
               current login */

            if (::GetUserName(buffer, (LPDWORD) &nLen)) {
                m_Username = buffer;
            }
        }

        if (PREF_NOERROR == pProfile->GetCharPref("mail.identity.useremail", buffer, &nLen)) {
            m_UserEmail = buffer;
        }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CNewProfileNetIdentityPage::OnSetActive() 
{
	CPropertySheet* pSheet = STATIC_DOWNCAST(CPropertySheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    return CPropertyPage::OnSetActive();
}
/////////////////////////////////////////////////////////////////////////////
// CNewProfileDirectoryPage property page

IMPLEMENT_DYNCREATE(CNewProfileDirectoryPage, CPropertyPage)

CNewProfileDirectoryPage::CNewProfileDirectoryPage() : CPropertyPage(CNewProfileDirectoryPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileDirectoryPage)
	m_ProfileName = _T("");
	m_ProfileDirectory = _T("");
	//}}AFX_DATA_INIT

}

CNewProfileDirectoryPage::~CNewProfileDirectoryPage()
{
}

void CNewProfileDirectoryPage::DoDataExchange(CDataExchange* pDX)
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileDirectoryPage)
	DDX_Control(pDX, IDC_EDIT_PROFILE, m_NameControl);
	DDX_Control(pDX, IDC_EDIT_PROFILE_DIR, m_DirControl);
	DDX_Text(pDX, IDC_EDIT_PROFILE, m_ProfileName);
	DDV_MaxChars(pDX, m_ProfileName, 30);
	DDX_Text(pDX, IDC_EDIT_PROFILE_DIR, m_ProfileDirectory);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        pSheet->SetProfileName(m_ProfileName);
        pSheet->SetProfileDirectory(m_ProfileDirectory);
    }
}


BEGIN_MESSAGE_MAP(CNewProfileDirectoryPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileDirectoryPage)
	ON_EN_UPDATE(IDC_EDIT_PROFILE, OnUpdateProfileName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileDirectoryPage message handlers

BOOL CNewProfileDirectoryPage::OnInitDialog() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());
    CString           csUserAddrShort = pSheet->GetUserEmail();
    char              *newProfileDir = NULL;
	
	CPropertyPage::OnInitDialog();
	
	int iAtSign = csUserAddrShort.Find('@');
	
	if (iAtSign != -1) 
		csUserAddrShort = csUserAddrShort.Left(iAtSign);

	if (csUserAddrShort.IsEmpty()) 
        csUserAddrShort = "default";        /* FIXME: Hardcoded string */

    m_NameControl.SetWindowText(csUserAddrShort);
    newProfileDir = CProfileManager::GenerateProfileDirectory(NULL, csUserAddrShort);

    if (newProfileDir) {
        m_DirControl.SetWindowText(newProfileDir);
    }

    XP_FREEIF(newProfileDir);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CNewProfileDirectoryPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    if (pSheet->GetStyle() == PROFMGR_NEWPROF_MINIMAL) {
        pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
    } else {
        pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
	
	return CPropertyPage::OnSetActive();
}


void CNewProfileDirectoryPage::OnUpdateProfileName() 
{
    char    *newProfileDir = NULL;
    CString profileName;

	/* Have the profile directory track the name, unless the user
       explicitly changed the directory name */

    if (!m_DirControl.GetModify()) {
        m_NameControl.GetWindowText(profileName);
        newProfileDir = CProfileManager::GenerateProfileDirectory(NULL, profileName);

        if (newProfileDir) {
            m_DirControl.SetWindowText(newProfileDir);
        }

        XP_FREEIF(newProfileDir);
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileUpgradePage property page

IMPLEMENT_DYNCREATE(CNewProfileUpgradePage, CPropertyPage)

CNewProfileUpgradePage::CNewProfileUpgradePage() : CPropertyPage(CNewProfileUpgradePage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileUpgradePage)
	//}}AFX_DATA_INIT
}

CNewProfileUpgradePage::~CNewProfileUpgradePage()
{
}

void CNewProfileUpgradePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileUpgradePage)
	DDX_Control(pDX, IDC_RADIO_MOVE, m_RadioMove);
	DDX_Control(pDX, IDC_RADIO_COPY, m_RadioCopy);
	DDX_Control(pDX, IDC_RADIO_NEWPROFILE, m_RadioNew);
	DDX_Control(pDX, IDC_NEXTTEXT, m_NextText);
	DDX_Control(pDX, IDC_FINISHTEXT, m_FinishText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileUpgradePage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileUpgradePage)
	ON_BN_CLICKED(IDC_RADIO_COPY, OnRadioCopy)
	ON_BN_CLICKED(IDC_RADIO_MOVE, OnRadioMove)
	ON_BN_CLICKED(IDC_RADIO_NEWPROFILE, OnRadioNew)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileUpgradePage message handlers

BOOL CNewProfileUpgradePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

    m_RadioMove.SetCheck(1);
    OnRadioMove();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewProfileUpgradePage::OnRadioCopy() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    m_NextText.ShowWindow(SW_HIDE);	
    m_FinishText.ShowWindow(SW_SHOW);
    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

    pSheet->SetUpgradeFlag(TRUE);
    pSheet->SetUpgradeInfo(PROFILE_COPY_UPGRADED_FILES);
    pSheet->SetReturnCode(PREF_PROFILE_UPGRADE);
}

void CNewProfileUpgradePage::OnRadioMove() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    m_NextText.ShowWindow(SW_HIDE);	
    m_FinishText.ShowWindow(SW_SHOW);	
    pSheet->SetUpgradeInfo(0);
    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

    pSheet->SetUpgradeFlag(TRUE);
    pSheet->SetReturnCode(PREF_PROFILE_UPGRADE);
}

void CNewProfileUpgradePage::OnRadioNew() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    m_NextText.ShowWindow(SW_SHOW);	
    m_FinishText.ShowWindow(SW_HIDE);
    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    pSheet->SetUpgradeFlag(FALSE);
    pSheet->SetReturnCode(PREF_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CNewProfileMailPage property page

IMPLEMENT_DYNCREATE(CNewProfileMailPage, CPropertyPage)

CNewProfileMailPage::CNewProfileMailPage() : CPropertyPage(CNewProfileMailPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileMailPage)
	m_MailServer = _T("");
	m_MailUsername = _T("");
	//}}AFX_DATA_INIT
}

CNewProfileMailPage::~CNewProfileMailPage()
{
}

void CNewProfileMailPage::DoDataExchange(CDataExchange* pDX)
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileMailPage)
	DDX_Text(pDX, IDC_EDIT_MAIL_SERVER, m_MailServer);
	DDX_Text(pDX, IDC_EDIT_MAIL_USER, m_MailUsername);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        CButton     *popButton;

        pSheet->SetMailServer(m_MailServer);
        pSheet->SetMailUsername(m_MailUsername);

        popButton = (CButton *) GetDlgItem(IDC_RADIO_POP);
        
        if ((popButton->GetState() & 0x003) == 1) {
            pSheet->SetMailServerType(MSG_Pop3);
        } else {
            pSheet->SetMailServerType(MSG_Imap4);
        }
        
    }
}


BEGIN_MESSAGE_MAP(CNewProfileMailPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileMailPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileMailPage message handlers

BOOL CNewProfileMailPage::OnInitDialog() 
{

    CheckDlgButton(IDC_RADIO_IMAP, TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CNewProfileMailPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNNTPPage property page

IMPLEMENT_DYNCREATE(CNewProfileNNTPPage, CPropertyPage)

CNewProfileNNTPPage::CNewProfileNNTPPage() : CPropertyPage(CNewProfileNNTPPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileNNTPPage)
	m_NNTPPort = 0;
	m_NNTPServer = _T("");
	m_IsSecure = FALSE;
	//}}AFX_DATA_INIT
}

CNewProfileNNTPPage::~CNewProfileNNTPPage()
{
}

void CNewProfileNNTPPage::DoDataExchange(CDataExchange* pDX)
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileNNTPPage)
	DDX_Control(pDX, IDC_EDIT_NEWS_PORT, m_NNTPPortEditField);
	DDX_Control(pDX, IDC_CHECK1, m_SecureCheckbox);
	DDX_Text(pDX, IDC_EDIT_NEWS_PORT, m_NNTPPort);
	DDX_Text(pDX, IDC_EDIT_NEWS_SERVER, m_NNTPServer);
	DDX_Check(pDX, IDC_CHECK1, m_IsSecure);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        pSheet->SetNNTPServer(m_NNTPServer);
        pSheet->SetNNTPPort(m_NNTPPort);
        pSheet->SetNNTPIsSecure(m_IsSecure);
    }
}


BEGIN_MESSAGE_MAP(CNewProfileNNTPPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileNNTPPage)
	ON_BN_CLICKED(IDC_CHECK1, OnCheckSecure)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileNNTPPage message handlers

BOOL CNewProfileNNTPPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    SetDlgItemInt(IDC_EDIT_NEWS_PORT, NEWS_PORT);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CNewProfileNNTPPage::OnSetActive() 
{
	CPropertySheet* pSheet = STATIC_DOWNCAST(CPropertySheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

    return CPropertyPage::OnSetActive();
}


void CNewProfileNNTPPage::OnCheckSecure() 
{
    CString     port;
    int32       lPortNum;

    m_NNTPPortEditField.GetWindowText(port);
    lPortNum = atol(port);

    if ((m_SecureCheckbox.GetState() & 0x003) == 1) {
        if (lPortNum == NEWS_PORT) {
            SetDlgItemInt(IDC_EDIT_NEWS_PORT, SECURE_NEWS_PORT);
        }
    } else {
        if (lPortNum == SECURE_NEWS_PORT) {
            SetDlgItemInt(IDC_EDIT_NEWS_PORT, NEWS_PORT);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CNewProfileSMTPPage property page

IMPLEMENT_DYNCREATE(CNewProfileSMTPPage, CPropertyPage)

CNewProfileSMTPPage::CNewProfileSMTPPage() : CPropertyPage(CNewProfileSMTPPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileSMTPPage)
	m_SMTPServer = _T("");
	//}}AFX_DATA_INIT
}

CNewProfileSMTPPage::~CNewProfileSMTPPage()
{
}

void CNewProfileSMTPPage::DoDataExchange(CDataExchange* pDX)
{
	CNewProfileSheet *pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileSMTPPage)
	DDX_Text(pDX, IDC_EDIT_SMTP_HOST, m_SMTPServer);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate) {
        pSheet->SetSMTPServer(m_SMTPServer);
    }
}


BEGIN_MESSAGE_MAP(CNewProfileSMTPPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileSMTPPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileSMTPPage message handlers

BOOL CNewProfileSMTPPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CNewProfileCountryPage property page

IMPLEMENT_DYNCREATE(CNewProfileCountryPage, CPropertyPage)

CNewProfileCountryPage::CNewProfileCountryPage() : CPropertyPage(CNewProfileCountryPage::IDD)
{
	//{{AFX_DATA_INIT(CNewProfileCountryPage)
	m_SelectedCountry = _T("");
	//}}AFX_DATA_INIT
}

CNewProfileCountryPage::~CNewProfileCountryPage()
{
}

void CNewProfileCountryPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfileCountryPage)
	DDX_CBString(pDX, IDC_COUNTRY, m_SelectedCountry);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfileCountryPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProfileCountryPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfileCountryPage message handlers

BOOL CNewProfileCountryPage::OnSetActive() 
{
	CNewProfileSheet* pSheet = STATIC_DOWNCAST(CNewProfileSheet, GetParent());

    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	
	return CPropertyPage::OnSetActive();
}
/////////////////////////////////////////////////////////////////////////////
// CNewProfPasswordDlg dialog


CNewProfPasswordDlg::CNewProfPasswordDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewProfPasswordDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewProfPasswordDlg)
	m_Password = _T("");
	//}}AFX_DATA_INIT
}


void CNewProfPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProfPasswordDlg)
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfPasswordDlg, CDialog)
	//{{AFX_MSG_MAP(CNewProfPasswordDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProfPasswordDlg message handlers

char *CNewProfPasswordDlg::GetSuppliedPassword()
{
    return (char *) (LPCTSTR) m_Password;
}

