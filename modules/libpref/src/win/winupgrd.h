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
 

#include "res/resource.h"

/////////////////////////////////////////////////////////////////////////////
// CUpdateFileDlg dialog
//
class CUpdateFileDlg : public CDialog
{
// Construction
public:
	CUpdateFileDlg(CWnd *pParent, BOOL bCopyDontMove);

    // Called at the start of each image saved
    void StartFileUpdate(const char *category,const char * pFilename);

	BOOL m_bCopyDontMove;
// Dialog Data
	//{{AFX_DATA(CUpdateFileDlg)
	enum { IDD = IDD_PROFILE_UPGRADE_STATUS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
    
private:
    CWnd      *m_pParent;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUpdateFileDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	
    // Generated message map functions
	//{{AFX_MSG(CUpdateFileDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

