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
#include "prtypes.h"
#ifdef NSPR20
#include "prio.h"
#else
#include "prfile.h"
#endif

#include "prefapi.h"
#include "cprofmgr.h"

#include "assert.h"

#include "afxwin.h"

#include "xp_mcom.h"
#include "xp_str.h"

#include "assert.h"

#include <direct.h>

#include "winupgrd.h"

#define NOT_NULL(X)	X
#ifdef XP_ASSERT
#undef XP_ASSERT
#endif

#define XP_ASSERT(X) assert(X)
#define LINEBREAK "\n"

struct update_PrefTable { char *xp_name; char *section; char *name; int type;};

int profmgr_NativeCopyStarterFiles(const char *userDir);

int WFEU_UpdaterMoveDirectory(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category);
int WFEU_UpdaterCopyDirectory(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category) ;
int WFEU_UpdateNewsFatFile(CString src,CString dst);
int WFEU_MoveDirectoryRecursiveDiffDrive(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category,XP_Bool bCopyDontMove);
int WFEU_MoveDirectorySameDrive(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category);
int WFEU_CheckDiskSpaceForMove(CString src,CString dst,uint32 * iSpaceNeeded);


int     login_UpdateFilesToNewLocation(const char * path,CWnd *pParent,BOOL bCopyDontMove);
int     login_UpdatePreferencesToJavaScript(const char * path);


// CUpdateFileDlg dialog


CUpdateFileDlg::CUpdateFileDlg(CWnd *pParent, BOOL bCopyDontMove)
	: CDialog(CUpdateFileDlg::IDD, pParent),
      m_pParent(pParent)
{
	//{{AFX_DATA_INIT(CSaveFileDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    
    // Suppress user interaction while we are active!
    if(pParent){ 
	pParent->EnableWindow(FALSE);
    }
    
    // For simplicity, create right here
    if (!CDialog::Create(CUpdateFileDlg::IDD, pParent))
    {
	TRACE0("Warning: creation of CUpdateFileDlg dialog failed\n");
	return;
    }

	m_bCopyDontMove = bCopyDontMove;

    // Clear our place-holder string
    GetDlgItem(IDC_CATEGORY)->SetWindowText("");
	GetDlgItem(IDC_TEXT1)->SetWindowText("Note:  If you have large mail or news folders, some of these operations may take a while.  Please be patient.");
    // Why doesn't this center on parent window automatically!
    CRect cRectParent;
    CRect cRectDlg;
    if( pParent ){
	pParent->GetWindowRect(&cRectParent);
	GetWindowRect(&cRectDlg);
	int iTop = 200;
	int iLeft = 200;
	if ( cRectParent.Height() > cRectDlg.Height() ){
	    iTop = cRectParent.top + ( (cRectParent.Height() - cRectDlg.Height()) / 2 );
	}
	if ( cRectParent.Width() > cRectDlg.Width() ){
	    iLeft = cRectParent.left + ( (cRectParent.Width() - cRectDlg.Width()) / 2 );
	}
	SetWindowPos(&wndTopMost, iLeft, iTop, 0, 0, SWP_NOSIZE);
    }
	ShowWindow(SW_SHOW);
}

void CUpdateFileDlg::StartFileUpdate(const char *category, const char * pFilename)
{
	GetDlgItem(IDC_CATEGORY)->SetWindowText(category);
	CString csTmp;
	
	if (m_bCopyDontMove)
		csTmp.LoadString(IDS_COPYING_FILE);
	else
		csTmp.LoadString(IDS_MOVING_FILE);
    csTmp+=pFilename;
    GetDlgItem(IDC_FILENAME_AREA)->SetWindowText(csTmp);
    // Report file number if we have more than 1 total
}

void CUpdateFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveFileDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUpdateFileDlg, CDialog)
	//{{AFX_MSG_MAP(CSaveFileDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CUpdateFileDlg::PostNcDestroy() 
{
	CDialog::PostNcDestroy();
    if( m_pParent && ::IsWindow(m_pParent->m_hWnd) ){
		m_pParent->EnableWindow(TRUE);
		// Return focus to parent window
		m_pParent->SetActiveWindow();
		m_pParent->SetFocus();
    }
}


#define MY_FINDFIRST(a,b) FindFirstFile(a,b) 
#define MY_FINDNEXT(a,b) FindNextFile(a,b) 
#define ISDIR(a) (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
#define MY_FINDCLOSE(a) FindClose(a) 
#define MY_FILENAME(a) a.cFileName
#define MY_FILESIZE(a) (a.nFileSizeHigh * MAXDWORD) + a.nFileSizeLow


int WFEU_CheckDiskSpaceForMove(CString src,CString dst,uint32 * iSpaceNeeded)
{
#ifdef XP_WIN16
	struct _find_t data_ptr;
	unsigned find_handle;
#else
	WIN32_FIND_DATA data_ptr;
	HANDLE find_handle;
#endif

	// Append slash to the end of the directory names if not there
	if (dst.Right(1) != "\\")
		dst += "\\";

	if (src.Right(1) != "\\")
		src += "\\";

	find_handle = MY_FINDFIRST(src+"*.*", &data_ptr);

	if (find_handle != INVALID_HANDLE_VALUE) {
		do  {

			if (ISDIR(data_ptr)
				&& (strcmpi(MY_FILENAME(data_ptr),"."))
				&& (strcmpi(MY_FILENAME(data_ptr),".."))) {
					WFEU_CheckDiskSpaceForMove(
						src + MY_FILENAME(data_ptr),
						dst + MY_FILENAME(data_ptr),
						iSpaceNeeded);
			}
			else if (!ISDIR(data_ptr)) {
				*iSpaceNeeded+= MY_FILESIZE(data_ptr);
			}
		} while(MY_FINDNEXT(find_handle,&data_ptr));
		MY_FINDCLOSE(find_handle);
	}
	return TRUE;
}

#ifndef XP_WIN16
// doesn't work in win16...copy individual files
int WFEU_MoveDirectorySameDrive(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category) 
{
	// Append slash to the end of the directory names if not there
	if (dst.Right(1) != "\\")
		dst += "\\";

	if (src.Right(1) != "\\")
		src += "\\";

	// update status dialog
	if (pDlg) pDlg->StartFileUpdate(category,(char *)(const char *)src);

	BOOL bRet = MoveFile(src,dst);
	return (bRet);
}
#endif

// 
int WFEU_MoveDirectoryRecursiveDiffDrive(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category,XP_Bool bCopyDontMove) 
{
#ifdef XP_WIN16
	struct _find_t data_ptr;
	unsigned find_handle;
#else
	WIN32_FIND_DATA data_ptr;
	HANDLE find_handle;
#endif

#ifdef XP_WIN16
	// before the extra slash
	_mkdir(dst);
#endif
	// Append slash to the end of the directory names if not there
	if (dst.Right(1) != "\\")
		dst += "\\";

	if (src.Right(1) != "\\")
		src += "\\";

#ifdef XP_WIN32
	// this wants (or at least tolerates the extra slash)
	CreateDirectory(dst, NULL);
#endif
	find_handle = MY_FINDFIRST(src+"*.*", &data_ptr);
	if (find_handle != INVALID_HANDLE_VALUE) {
		do  {
			if (ISDIR(data_ptr) 
				&& (strcmpi(MY_FILENAME(data_ptr),"."))
				&& (strcmpi(MY_FILENAME(data_ptr),".."))) {
					WFEU_MoveDirectoryRecursiveDiffDrive(
						src + MY_FILENAME(data_ptr), 
						dst + MY_FILENAME(data_ptr),
						pDlg,
						category,
						bCopyDontMove);
			}
			else if (!ISDIR(data_ptr)) {
				if (pDlg) pDlg->StartFileUpdate(category,MY_FILENAME(data_ptr));
				if (bCopyDontMove)
					CopyFile(src + MY_FILENAME(data_ptr), dst + MY_FILENAME(data_ptr), FALSE);
				else
					MoveFile(src + MY_FILENAME(data_ptr), dst + MY_FILENAME(data_ptr));
			}
		} while(MY_FINDNEXT(find_handle,&data_ptr));
		MY_FINDCLOSE(find_handle);
		return TRUE;
	}
	return FALSE;
}

// duplicate of line #1323 in mknewsgr.c -- Bad but don't wanna mess with header files
#define NEWSRC_MAP_FILE_COOKIE "netscape-newsrc-map-file"

int     WFEU_UpdateNewsFatFile(CString src,CString dst)
{
	XP_File src_fp,dst_fp;
	XP_StatStruct stats;
	long fileLength;
	CString csSrcFat,csDstFat;

		// Append slash to the end of the directory names if not there
	if (dst.Right(1) != "\\")
		dst += "\\";

	if (src.Right(1) != "\\")
		src += "\\";

	csSrcFat = src + "fat";
	csDstFat = dst + "fat.new";

    if (_stat(csSrcFat, &stats) == -1)
		return FALSE;

	fileLength = stats.st_size;
	if (fileLength <= 1)
		return FALSE;
	
	src_fp = fopen(csSrcFat, "r");
	dst_fp = fopen(csDstFat, "wb");

	if (src_fp && dst_fp) { 
		char buffer[512];
		char psuedo_name[512];
		char filename[512];
		char is_newsgroup[512];

		// This code is all stolen from mknewsgr.c -- written by JRE

		/* get the cookie and ignore */
		XP_FileReadLine(buffer, sizeof(buffer), src_fp);

		XP_FileWrite(NEWSRC_MAP_FILE_COOKIE, XP_STRLEN(NEWSRC_MAP_FILE_COOKIE), dst_fp);
		XP_FileWrite(LINEBREAK, XP_STRLEN(LINEBREAK), dst_fp);

		while(XP_FileReadLine(buffer, sizeof(buffer), src_fp))
		  {
			char * p;
			int i;
			
			filename[0] = '\0';
			is_newsgroup[0]='\0';

			for (i = 0, p = buffer; *p && *p != '\t' && i < 500; p++, i++)
				psuedo_name[i] = *p;
			psuedo_name[i] = '\0';
			if (*p) 
			  {
				for (i = 0, p++; *p && *p != '\t' && i < 500; p++, i++)
					filename[i] = *p;
				filename[i]='\0';
				if (*p) 
				  {
					for (i = 0, p++; *p && *p != '\r' && *p != '\n' && i < 500; p++, i++)
						is_newsgroup[i] = *p;
					is_newsgroup[i]='\0';
				  }
			  }

			CString csFilename = filename;
			CString csDestFilename = dst;
			int iLastSlash = csFilename.ReverseFind('\\');

			if (iLastSlash != -1) {
				csDestFilename += csFilename.Right(csFilename.GetLength()-iLastSlash-1);
			} else 
				csDestFilename += filename;

			// write routines stolen from mknewsgr.c -- line #1348
			XP_FileWrite(psuedo_name, XP_STRLEN(psuedo_name),dst_fp);
			
			XP_FileWrite("\t", 1, dst_fp);

			XP_FileWrite((const char *)csDestFilename, csDestFilename.GetLength() , dst_fp);
			
			XP_FileWrite("\t", 1, dst_fp);

			XP_FileWrite(is_newsgroup, XP_STRLEN(is_newsgroup), dst_fp);

			XP_FileWrite(LINEBREAK, XP_STRLEN(LINEBREAK), dst_fp);

		  }
		XP_FileClose(src_fp);
		XP_FileClose(dst_fp);
	} else
		return FALSE;
	
	return TRUE;
}

int WFEU_UpdaterCopyDirectory(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category) 
{
	uint32 iSpaceNeeded = 0;
	uint32 iSpaceAvailable = 0;
	DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumFreeClusters, dwTotalClusters;
	
	// assumes properly formated destination string
	GetDiskFreeSpace(dst.Left(3),&dwSectorsPerCluster,&dwBytesPerSector,
		&dwNumFreeClusters,&dwTotalClusters);

	iSpaceAvailable = (dwNumFreeClusters * dwSectorsPerCluster * dwBytesPerSector);
	WFEU_CheckDiskSpaceForMove(src,dst,&iSpaceNeeded);
	while (iSpaceNeeded > iSpaceAvailable) {
        CString     stringTemplate;
		char        szLen[64];
		char        szMsg[1024];

		_ltoa(((iSpaceNeeded-iSpaceAvailable)/1024)/1024,szLen,10);

        stringTemplate.LoadString(IDS_INSUFFICIENT_DISKSPACE_COPY);
		sprintf(szMsg, stringTemplate, category, src, dst, szLen);

		if (AfxMessageBox(szMsg,MB_OKCANCEL) == IDCANCEL)
			return FALSE;
		GetDiskFreeSpace(dst.Left(3),&dwSectorsPerCluster,&dwBytesPerSector,
			&dwNumFreeClusters,&dwTotalClusters);
		iSpaceAvailable = (dwNumFreeClusters * dwSectorsPerCluster * dwBytesPerSector);
	}
	return WFEU_MoveDirectoryRecursiveDiffDrive(src,dst,pDlg,category,TRUE);
}

int WFEU_UpdaterMoveDirectory(CString src,CString dst,CUpdateFileDlg * pDlg,const char *category) 
{
#ifndef XP_WIN16
	// directory rename doesn't work under win16
	int iDiffDrive = src.Left(2).CompareNoCase(dst.Left(2));

	if (!iDiffDrive) {
		int iRet = WFEU_MoveDirectorySameDrive(src,dst,pDlg,category);
		if (iRet)
			return iRet;
	}
#else
	int iDiffDrive = TRUE;
#endif
	// else if DiffDrive or if move same drive failed...

	if (iDiffDrive) {
		uint32 iSpaceNeeded = 0;
		uint32 iSpaceAvailable = 0;
		DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumFreeClusters, dwTotalClusters;
		
		// assumes properly formated destination string
		GetDiskFreeSpace(dst.Left(3),&dwSectorsPerCluster,&dwBytesPerSector,
			&dwNumFreeClusters,&dwTotalClusters);

		iSpaceAvailable = (dwNumFreeClusters * dwSectorsPerCluster * dwBytesPerSector);
		WFEU_CheckDiskSpaceForMove(src,dst,&iSpaceNeeded);
		while (iSpaceNeeded > iSpaceAvailable) {
            CString     stringTemplate;
			char szLen[64];
			char szMsg[1024];

            _ltoa(((iSpaceNeeded-iSpaceAvailable)/1024)/1024,szLen,10);

            stringTemplate.LoadString(IDS_INSUFFICIENT_DISKSPACE_MOVE);
	        sprintf(szMsg, stringTemplate, category, src, dst, szLen);

			if (AfxMessageBox(szMsg,MB_OKCANCEL) == IDCANCEL)
				return FALSE;
			GetDiskFreeSpace(dst.Left(3),&dwSectorsPerCluster,&dwBytesPerSector,
				&dwNumFreeClusters,&dwTotalClusters);
			iSpaceAvailable = (dwNumFreeClusters * dwSectorsPerCluster * dwBytesPerSector);
		}
	}
	return WFEU_MoveDirectoryRecursiveDiffDrive(src,dst,pDlg,category,FALSE);
}


static struct update_PrefTable prefUpdater[] =
{
  {"browser.cache.disk_cache_ssl","Cache","Disk Cache SSL",PREF_BOOL},
  {"browser.cache.disk_cache_size","Cache","Disk Cache Size",PREF_INT},
  {"browser.cache.memory_cache_size","Cache","Memory Cache Size",PREF_INT},
//
  {"security.enable_java","Java","Enable Java",PREF_BOOL},
  {"security.enable_javascript","Java","Enable JavaScript",PREF_BOOL},
//  
  {"mail.check_time","Mail","Check Time",PREF_INT},
  {"mail.max_size","Mail","Max Size",PREF_INT},
  {"mail.pop_password","Mail","POP Password",PREF_STRING},
  {"mail.pop_name","Mail","POP Name",PREF_STRING},
  {"mail.leave_on_server","Mail","Leave on server",PREF_BOOL},
  {"mail.auto_quote","Mail","Auto quote",PREF_BOOL},
  {"mail.remember_password","Mail","Remember Password",PREF_BOOL},
//
  {"browser.underline_anchors","Main","Anchor Underline",PREF_BOOL},
  {"browser.cache.check_doc_frequency","Main","Check Server",PREF_INT},
  {"browser.download_directory","Main","Default Save Dir",PREF_STRING},
  {"browser.wfe.ignore_def_check","Main","Ignore DefCheck",PREF_BOOL},
  {"browser.startup.homepage","Main","Home Page",PREF_STRING},
//
  {"security.submit_email_forms","Network","Warn Submit Email Form",PREF_BOOL},
  {"security.email_as_ftp_password","Network","Use Email For FTP",PREF_BOOL},
  {"network.cookie.warnAboutCookies","Network","Warn Accepting Cookie",PREF_BOOL},
  {"network.max_connections","Network","Max Connections",PREF_INT},
  {"network.tcpbufsize","Network","TCP Buffer Size",PREF_INT},
//
  {"news.max_articles","News","News Chunk Size",PREF_INT},
  {"news.show_headers","News","Show Headers",PREF_INT},
  {"news.sort_by","News","Sort News",PREF_INT},
  {"news.thread_news","News","Thread News",PREF_BOOL},
//
  {"network.proxy.autoconfig_url","Proxy Information","Auto Config Url",PREF_STRING},
  {"network.proxy.type","Proxy Information","Proxy Type",PREF_INT},
  {"network.proxy.wais_port","Proxy Information","Wais_ProxyPort",PREF_INT},
  {"network.proxy.ftp_port","Proxy Information","Ftp_ProxyPort",PREF_INT},
  {"network.proxy.ssl_port","Proxy Information","HTTPS_ProxyPort",PREF_INT},
  {"network.proxy.gopher_port","Proxy Information","Gopher_ProxyPort",PREF_INT},
  {"network.proxy.http_port","Proxy Information","Http_ProxyPort",PREF_INT},
  {"network.proxy.no_proxies_on","Proxy Information","No_Proxy",PREF_STRING},
  {"network.proxy.wais","Proxy Information","Wais_Proxy",PREF_STRING},
  {"network.proxy.ssl","Proxy Information","HTTPS_Proxy",PREF_STRING},
  {"network.proxy.gopher","Proxy Information","Gopher_Proxy",PREF_STRING},
  {"network.proxy.ftp","Proxy Information","FTP_Proxy",PREF_STRING},
  {"network.proxy.http","Proxy Information","HTTP_Proxy",PREF_STRING},
//
  {"security.warn_submit_insecure","Security","Warn Insecure Forms",PREF_BOOL},
  {"security.default_personal_cert","Security","Default User Cert",PREF_STRING},
  {"security.enable_ssl3","Security","Enable SSL v3",PREF_BOOL},
  {"security.enable_ssl2","Security","Enable SSL v2",PREF_BOOL},
  {"security.ask_for_password","Security","Ask for password",PREF_INT},
  {"security.password_lifetime","Security","Password Lifetime",PREF_INT},
  {"security.warn_entering_secure","Security","Warn Entering",PREF_BOOL},
  {"security.warn_leaving_secure","Security","Warn Leaving",PREF_BOOL},
  {"security.warn_viewing_mixed","Security","Warn Mixed",PREF_BOOL},
  {"security.ciphers","Security","Cipher Prefs",PREF_STRING},
//
  {"mail.use_exchange","Services","Mapi",PREF_BOOL},
  {"network.hosts.socks_serverport","Services","SOCKS_ServerPort",PREF_INT},
  {"network.hosts.socks_conf","Services","Socks Conf",PREF_STRING},
  {"network.hosts.pop_server","Services","POP_Server",PREF_STRING},
  {"network.hosts.nntp_server","Services","NNTP_Server",PREF_STRING},
  {"network.hosts.smtp_server","Services","SMTP_Server",PREF_STRING},
  {"network.hosts.socks_server","Services","SOCKS_Server",PREF_STRING},
//
  {"mail.fixed_width_messages","Settings","Fixed Width Messages",PREF_BOOL},
  {"mail.quoted_style","Settings","Quoted Style",PREF_INT},
  {"mail.quoted_size","Settings","Quoted Size",PREF_INT},
  {"browser.blink_allowed","Settings","Blinking",PREF_BOOL},
  {"browser.link_expiration","Settings","History Expiration",PREF_INT},
//
  {"mail.identity.reply_to","User","Reply_To",PREF_STRING},
  {"mail.identity.organization","User","User_Organization",PREF_STRING},
  {"mail.signature_file","User","Sig_File",PREF_STRING},
//
  {"editor.publish_location", "Publish", "Default Location", PREF_STRING},
  {"editor.publish_browse_location", "Publish", "Default Browse Location", PREF_STRING},
//
  {NULL,NULL,NULL,NULL}
};

    int
login_UpdateFilesToNewLocation(const char * path,CWnd *pParent,BOOL bCopyDontMove)
{
	CString     csTmp;
    CWinApp     *pApp = AfxGetApp();
    CString     csCurrentCategory;

	if (!path) return FALSE;

	CUpdateFileDlg * pDlg = new CUpdateFileDlg(pParent,bCopyDontMove);

	CString csMain = pApp->GetProfileString("Main","Install Directory","");

    csCurrentCategory.LoadString(IDS_GENERAL_FILES);

	CString csBookmarks = pApp->GetProfileString("Bookmark List","File Location","");
	if (!csBookmarks.IsEmpty()) {
		csTmp = path;
		csTmp += "\\bookmark.htm";
		pDlg->StartFileUpdate(csCurrentCategory,"bookmark.htm");
		if (bCopyDontMove) {
			CopyFile(csBookmarks,csTmp, FALSE);
		} else {
			if (MoveFile(csBookmarks,csTmp))
				pApp->WriteProfileString("Bookmark List","File Location",csTmp);
		}
	}

	CString csABook = pApp->GetProfileString("Address Book","File Location","");
	if (!csABook.IsEmpty()) {
		csTmp = path;
		csTmp += "\\address.htm";
		pDlg->StartFileUpdate(csCurrentCategory,"address.htm");
		if (bCopyDontMove) {
			CopyFile(csABook,csTmp, FALSE);
		} else {
			if (MoveFile(csABook,csTmp))
				pApp->WriteProfileString("Address Book","File Location",csTmp);
		}
	} else {
		if (!csMain.IsEmpty()) {
			csTmp = path;
			csTmp += "\\address.htm";
			pDlg->StartFileUpdate(csCurrentCategory,"address.htm");
			CopyFile(csMain + "\\address.htm" ,csTmp, FALSE);
		}
	}

	CString csHist = pApp->GetProfileString("History","History File","");
	if (!csHist.IsEmpty()) {
		csTmp = path;
		csTmp += "\\netscape.hst";
		pDlg->StartFileUpdate(csCurrentCategory,"netscape.hst");
		if (bCopyDontMove) {
			CopyFile(csHist,csTmp, FALSE);
		} else {
			if (MoveFile(csHist,csTmp))
				pApp->WriteProfileString("History","History File",csTmp);
		}
	} else {
		if (!csMain.IsEmpty()) {
			csTmp = path;
			csTmp += "\\netscape.hst";
			pDlg->StartFileUpdate(csCurrentCategory,"netscape.hst");
			CopyFile(csMain + "\\netscape.hst" ,csTmp, FALSE);
		}
	}

	CString csNewsRC = pApp->GetProfileString("Main","News RC","");
	if (!csNewsRC.IsEmpty()) {
		csTmp = path;
		csTmp += "\\Newsrc";
		pDlg->StartFileUpdate(csCurrentCategory,"Newsrc");
		if (bCopyDontMove) {
			CopyFile(csNewsRC,csTmp, FALSE);
		} else {
			if (MoveFile(csNewsRC,csTmp))
				pApp->WriteProfileString("Main","News RC",csTmp);
		}
	} 

	if (!csMain.IsEmpty()) {
		// these files are always copied since we don't store pointers
		csTmp = path;
		csTmp += "\\socks.cnf";
		pDlg->StartFileUpdate(csCurrentCategory,"socks.cnf");
		CopyFile(csMain + "\\socks.cnf" ,csTmp, FALSE);

		csTmp = path;
		csTmp += "\\cookies.txt";
		pDlg->StartFileUpdate(csCurrentCategory,"cookies.txt");
		CopyFile(csMain + "\\cookies.txt",csTmp, FALSE);

        csCurrentCategory.LoadString(IDS_SECURITY_FILES);

        csTmp = path;
		csTmp += "\\key.db";
		pDlg->StartFileUpdate(csCurrentCategory,"key.db");
		CopyFile(csMain + "\\key.db",csTmp, FALSE);

		csTmp = path;
		csTmp += "\\cert5.db";
		pDlg->StartFileUpdate(csCurrentCategory,"cert5.db");
		CopyFile(csMain + "\\cert5.db",csTmp, FALSE);

		csTmp = path;
		csTmp += "\\certni.db";
		pDlg->StartFileUpdate(csCurrentCategory,"certni.db");
		CopyFile(csMain + "\\certni.db",csTmp, FALSE);

        csCurrentCategory.LoadString(IDS_NETWORK_FILES);

		csTmp = path;
		csTmp += "\\proxy.cfg";
		pDlg->StartFileUpdate(csCurrentCategory,"proxy.cfg");
		CopyFile(csMain + "\\proxy.cfg",csTmp, FALSE);

		csTmp = path;
		csTmp += "\\abook.nab";
		if (bCopyDontMove)
			CopyFile(csMain + "\\abook.nab",csTmp, FALSE);
		else
			MoveFile(csMain + "\\abook.nab",csTmp);
	}

    csCurrentCategory.LoadString(IDS_MAIL_DIR);

	csTmp = path;
	csTmp += "\\Mail";
	CString csMail = pApp->GetProfileString("Mail","Mail Directory","");

	if (!csMail.IsEmpty()) {
	    if (!strnicmp(csMail,csTmp,strlen(csMail))) {
            CString     stringTemplate;
    	    char szMsg[256];

            stringTemplate.LoadString(IDS_UNABLETRANSFER_SUBDIR);
		    _snprintf(szMsg, 256, stringTemplate, csCurrentCategory);
		    AfxMessageBox(szMsg);
	    }
	    else {
		    if (bCopyDontMove) {
				WFEU_UpdaterCopyDirectory(csMail,csTmp,pDlg,csCurrentCategory);
		    } else {
			    if (WFEU_UpdaterMoveDirectory(csMail,csTmp,pDlg,csCurrentCategory)) {
				    pApp->WriteProfileString("Mail","Mail Directory",csTmp);
				    CString csFCC = pApp->GetProfileString("Mail","Default Fcc","");
				    int iSlash = csFCC.ReverseFind('\\');
				    if (iSlash != -1) {
					    CString csFolder = csFCC.Right(csFCC.GetLength()-iSlash);
					    pApp->WriteProfileString("Mail","Default Fcc",csTmp+csFolder);
				    }
			    }
		    }
        }
	}

    csCurrentCategory.LoadString(IDS_NEWS_DIR);

	csTmp = path;
	csTmp += "\\News";
	CString csNews = pApp->GetProfileString("News","News Directory","");

	if (!csNews.IsEmpty()) {
	    if (!strnicmp(csNews,csTmp,strlen(csNews))) {
            CString     stringTemplate;
    	    char szMsg[256];

            stringTemplate.LoadString(IDS_UNABLETRANSFER_SUBDIR);
		    _snprintf(szMsg, 256, stringTemplate,csCurrentCategory);
		    AfxMessageBox(szMsg);
	    }
	    else {
		    if (bCopyDontMove) {
				WFEU_UpdaterCopyDirectory(csNews,csTmp,pDlg,csCurrentCategory);
		    } else {
			    if (WFEU_UpdaterMoveDirectory(csNews,csTmp,pDlg,csCurrentCategory))
				    pApp->WriteProfileString("News","News Directory",csTmp);               
		    }
		    // update news rc file either way to new directory -- creates fat,new
		    int iRet = WFEU_UpdateNewsFatFile(csTmp,csTmp);
		    // now move fat.new over the old fat file
		    if (iRet) {
			    CString csOldFat = csTmp + "\\fat";
			    CString csNewFat = csTmp + "\\fat.new";
			    rename(csOldFat,csTmp + "\\fat.old");
			    rename(csNewFat,csTmp + "\\fat");
		    }
        }
    }

    csCurrentCategory.LoadString(IDS_CACHE_DIR);

	csTmp = path;
	csTmp += "\\cache";
	CString csCache = pApp->GetProfileString("Cache","Cache Dir","");

	if (!csCache.IsEmpty()) {
	    if (!strnicmp(csCache,csTmp,strlen(csCache))) {
            CString     stringTemplate;
    	    char szMsg[256];

            stringTemplate.LoadString(IDS_UNABLETRANSFER_SUBDIR);
		    _snprintf(szMsg, 256, stringTemplate, csCurrentCategory);
		    AfxMessageBox(szMsg);
	    }
	    else {
    		if (bCopyDontMove) {
				WFEU_UpdaterCopyDirectory(csCache,csTmp,pDlg,csCurrentCategory);
		    } else {
			    if (WFEU_UpdaterMoveDirectory(csCache,csTmp,pDlg,csCurrentCategory))
				    pApp->WriteProfileString("Cache","Cache Dir",csTmp);   
		    }
        }
	}

    if (pDlg) {
        pDlg->DestroyWindow();
        delete pDlg;
    }

	return TRUE;
}


int     login_UpdatePreferencesToJavaScript(const char * path)
{
    CWinApp     *pApp = AfxGetApp();
    int idx=0;

	while (prefUpdater[idx].xp_name) {
		if (prefUpdater[idx].type == PREF_STRING) {// char pref 
			CString csPref= pApp->GetProfileString(prefUpdater[idx].section,prefUpdater[idx].name,"");
			if (!csPref.IsEmpty())
				PREF_SetCharPref(prefUpdater[idx].xp_name,(char *)(const char *)csPref);
		} else if (prefUpdater[idx].type == PREF_INT) {// char pref 
			int iPref= pApp->GetProfileInt(prefUpdater[idx].section,prefUpdater[idx].name,-1);
			if (iPref != -1)
				PREF_SetIntPref(prefUpdater[idx].xp_name,iPref);
		} else if (prefUpdater[idx].type == PREF_BOOL) {// char pref 
			CString csPref= pApp->GetProfileString(prefUpdater[idx].section,prefUpdater[idx].name,"");
			if (!csPref.IsEmpty()) {
				int iRet = csPref.CompareNoCase("yes");
				if (iRet ==0)
					PREF_SetBoolPref(prefUpdater[idx].xp_name,TRUE);
				else
					PREF_SetBoolPref(prefUpdater[idx].xp_name,FALSE);
			}
		}
		idx++;
	}

	// Do some special hacking on "browser.cache.check_doc_frequency" since the values changed
	// between 3.0 and 4.0
	int32 iExpr;

	PREF_GetIntPref("browser.cache.check_doc_frequency",&iExpr);
	if (iExpr == 1)
		PREF_SetIntPref("browser.cache.check_doc_frequency",0);
	else if (iExpr == 0)
		PREF_SetIntPref("browser.cache.check_doc_frequency",1);

	// Do some special processing for history link expiration. It used to be that there
	// was an option that history never expired. We no longer support that, in the UI
	// anyway
	int32   iExp;

	PREF_GetIntPref("browser.link_expiration", &iExp);
	if (iExp == -1) {
		// Set the date to something safe like 6 months
		PREF_SetIntPref("browser.link_expiration", 180);
	}

	CString csPref= pApp->GetProfileString("Main","Autoload Home Page","");
	if (!csPref.IsEmpty()) {
		if (!csPref.CompareNoCase("yes"))
			PREF_SetIntPref("browser.startup.page", 1);  // set to load homepage
		else
			PREF_SetIntPref("browser.startup.page", 0);  // set to blank
	}
	return TRUE;
}

int
profmgr_NativeCopyStarterFiles(const char *userDir)
{
	CString         csDst = userDir;
	CString         csSrc;
    CString         csCategory;
	char            appFullPath[_MAX_PATH];
    char            appPath[_MAX_PATH];
    CWinApp         *pApp = AfxGetApp();
	CUpdateFileDlg  *pDlg = new CUpdateFileDlg(NULL,TRUE);
    char            *filePart;

	::GetModuleFileName(pApp->m_hInstance, appFullPath, _MAX_PATH);

    GetFullPathName(appFullPath, _MAX_PATH, appPath, &filePart);
    *filePart = '\0';

	if (appFullPath[0] == '\0') return PREF_ERROR;
	if (!userDir) return PREF_BAD_PARAMETER;
	if (!pDlg) return PREF_OUT_OF_MEMORY;

	csSrc = appPath;
	csSrc += "\\defaults";
	csDst = userDir;

    csCategory.LoadString(IDS_DEFAULT_FILES);

    WFEU_UpdaterCopyDirectory(csSrc,csDst,pDlg,csCategory);

    pDlg->DestroyWindow();
    delete pDlg;

    return PREF_OK;
}
