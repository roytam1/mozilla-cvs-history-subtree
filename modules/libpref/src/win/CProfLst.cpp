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
 
#define _NSPR_NO_WINDOWS_H
#include <afx.h>
#include "resource.h"
#include "CProfLst.h"
#include "cprofmgr.h"
#include "..\prefpriv.h"

/////////////////////////////////////////////////////////////////////////////
// CProfilesComboBox

CProfilesComboBox::CProfilesComboBox()
{
}

CProfilesComboBox::~CProfilesComboBox()
{
    /* remember to free all the EntryInfo structs */
}


BEGIN_MESSAGE_MAP(CProfilesComboBox, CComboBox)
	//{{AFX_MSG_MAP(CProfilesComboBox)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfilesComboBox message handlers

void CProfilesComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
    EntryInfo  *itemInfo;
    BOOL        isSeparator;
    CString     lbText;

    itemInfo = (EntryInfo *) GetItemDataPtr(lpMIS->itemID);

    isSeparator = itemInfo && (itemInfo->style == PROFINFO_SEPARATOR);

    if (!isSeparator) {
        GetLBText(lpMIS->itemID, lbText );

        if (lbText.IsEmpty())
            isSeparator = TRUE;
    }

    if (isSeparator) {
    	lpMIS->itemHeight = 5;
    } else {
    	lpMIS->itemHeight = 20;
    }
}

void CProfilesComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	{
		COLORREF crHilite;
		CBitmap		headBitmap;
		CBitmap*	pbmOld = NULL;
		CDC			dcMem;
		CString		itemName;
        EntryInfo   *itemInfo;

        itemInfo = (EntryInfo *) GetItemDataPtr(lpDIS->itemID);

        if ((itemInfo != (EntryInfo *) -1) && (itemInfo->style == PROFINFO_SEPARATOR)) {
            pDC->MoveTo(lpDIS->rcItem.left, lpDIS->rcItem.top + 2);
            pDC->LineTo(lpDIS->rcItem.right, lpDIS->rcItem.top + 2);
        } else {

		    if (lpDIS->itemState & ODS_SELECTED) {
			    crHilite = GetSysColor(COLOR_HIGHLIGHT);
		    } else {
			    crHilite = GetSysColor(COLOR_WINDOW);
		    }

		    pDC-> FillSolidRect(&lpDIS->rcItem, crHilite);

            if ((itemInfo->style == PROFINFO_LOCAL) || (itemInfo->style == PROFINFO_NATIVE)) {
                headBitmap.LoadBitmap(IDB_HEAD_BITMAP);
            } else if (itemInfo->style == PROFINFO_REMOTE) {
                headBitmap.LoadBitmap(IDB_HEAD_BITMAP);
            }

            dcMem.CreateCompatibleDC(pDC);
		    pbmOld = (CBitmap *) dcMem.SelectObject(headBitmap);

		    pDC->BitBlt(lpDIS->rcItem.left + 2, lpDIS->rcItem.top + 2, 12, 12, &dcMem, 0, 0, SRCCOPY);

		    if ((LONG) lpDIS->itemID >= 0) {
			    GetLBText(lpDIS->itemID, itemName);
			    pDC->TextOut(lpDIS->rcItem.left + 18, lpDIS->rcItem.top + 2, itemName);
		    }
        }
	}

}

int CProfilesComboBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS) 
{
    CString     item1, item2;
    EntryInfo   *itemInfo1, *itemInfo2;

    GetLBText(lpCIS->itemID1, item1);
    GetLBText(lpCIS->itemID2, item2);

    return item1.CompareNoCase(item2);
}

void *CProfilesComboBox::GetProfileData(int nIndex)
{
    EntryInfo   *itemInfo;

    itemInfo =  (EntryInfo *) GetItemDataPtr(nIndex);

    return itemInfo->pData;
}

int CProfilesComboBox::AddEntry(const char *name, int style, void *pData)
{
    int         index, err;
    char        nullStr[1] = "";
    EntryInfo   *pInfo;

    if (name) {
        index = AddString(name);
    } else {
        index = AddString(nullStr);
    }
    if ((index != CB_ERR) && (index != CB_ERRSPACE)) {
        pInfo = (EntryInfo *) malloc(sizeof(EntryInfo));

        pInfo->style = style;
        pInfo->pData = pData;
        err = this->SetItemDataPtr(index, pInfo);
    }

    return index;
}

/////////////////////////////////////////////////////////////////////////////
// CProfilesListBox

CProfilesListBox::CProfilesListBox()
{
}

CProfilesListBox::~CProfilesListBox()
{
}


BEGIN_MESSAGE_MAP(CProfilesListBox, CListBox)
	//{{AFX_MSG_MAP(CProfilesListBox)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfilesListBox message handlers

void CProfilesListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

    if ((int) lpDIS->itemID >= 0) {
		COLORREF crHilite;
		CBitmap		headBitmap;
		CBitmap*	pbmOld = NULL;
		CDC			dcMem;
		CString		itemName;
        EntryInfo   *itemInfo;

        itemInfo = (EntryInfo *) GetItemDataPtr(lpDIS->itemID);

        if ((itemInfo != (EntryInfo *) -1) && (itemInfo->style == PROFINFO_SEPARATOR)) {
            pDC->MoveTo(lpDIS->rcItem.left, lpDIS->rcItem.top + 2);
            pDC->LineTo(lpDIS->rcItem.right, lpDIS->rcItem.top + 2);
        } else {

		    if (lpDIS->itemState & ODS_SELECTED) {
			    crHilite = GetSysColor(COLOR_HIGHLIGHT);
		    } else {
			    crHilite = GetSysColor(COLOR_WINDOW);
		    }

		    pDC-> FillSolidRect(&lpDIS->rcItem, crHilite);

            if ((itemInfo->style == PROFINFO_LOCAL) || (itemInfo->style == PROFINFO_NATIVE)) {
                headBitmap.LoadBitmap(IDB_HEAD_BITMAP);
            } else if (itemInfo->style == PROFINFO_REMOTE) {
                headBitmap.LoadBitmap(IDB_HEAD_BITMAP);
            }

            dcMem.CreateCompatibleDC(pDC);
		    pbmOld = (CBitmap *) dcMem.SelectObject(headBitmap);

		    pDC->BitBlt(lpDIS->rcItem.left + 2, lpDIS->rcItem.top + 2, 12, 12, &dcMem, 0, 0, SRCCOPY);

		    if ((LONG) lpDIS->itemID >= 0) {
			    GetText(lpDIS->itemID, itemName);
			    pDC->TextOut(lpDIS->rcItem.left + 18, lpDIS->rcItem.top + 2, itemName);
		    }
        }
	}
}

void CProfilesListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS) 
{
    	lpMIS->itemHeight = 20;
}

void *CProfilesListBox::GetProfileData(int nIndex)
{
    EntryInfo   *itemInfo;

    itemInfo =  (EntryInfo *) GetItemDataPtr(nIndex);

    return itemInfo->pData;
}

int CProfilesListBox::AddEntry(const char *name, int style, void *pData)
{
    int         index, err;
    char        nullStr[1] = "";
    EntryInfo   *pInfo;

    if (name) {
        index = AddString(name);
    } else {
        index = AddString(nullStr);
    }
    if ((index != CB_ERR) && (index != CB_ERRSPACE)) {
        pInfo = (EntryInfo *) malloc(sizeof(EntryInfo));

        pInfo->style = style;
        pInfo->pData = pData;
        err = this->SetItemDataPtr(index, pInfo);
    }

    return index;
}


int CProfilesListBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS) 
{
    CString     item1, item2;

    GetText(lpCIS->itemID1, item1);
    GetText(lpCIS->itemID2, item2);

    return item1.CompareNoCase(item2);
}
