/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _nsNewsDownloader_H_
#define _nsNewsDownloader_H_


#include "nsIMsgDatabase.h"
#include "nsIUrlListener.h"
#include "nsIMsgFolder.h"
#include "nsIMsgHdr.h"
#include "nsIMsgWindow.h"
#include "nsIMsgSearchNotify.h"

class nsNewsDownloader : public nsIUrlListener, public nsIMsgSearchNotify
{
public:
	nsNewsDownloader(nsIMsgWindow *window, nsIMsgDatabase *db, nsIUrlListener *listener);
	virtual ~nsNewsDownloader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLLISTENER
  NS_DECL_NSIMSGSEARCHNOTIFY

	virtual nsresult DownloadArticles(nsIMsgWindow *window, nsIMsgFolder *folder, nsMsgKeyArray *pKeyArray);
	
	PRBool			ShouldAbort() const { return m_abort; }
	
protected:
	virtual PRInt32 Write(const char * /*block*/, PRInt32 length) {return length;}
	virtual void Abort();
	virtual void Complete();
	virtual PRBool GetNextHdrToRetrieve();
	virtual nsresult DownloadNext(PRBool firstTimeP);
	virtual PRInt32 FinishDownload() {return 0;}
	virtual PRInt32	StartDownload() {return 0;}

	nsMsgKeyArray			m_keysToDownload;
	PRBool			m_downloadFromKeys;
	nsCOMPtr <nsIMsgFolder>	m_folder;
	nsCOMPtr <nsIMsgDatabase> m_newsDB;
  nsCOMPtr <nsIUrlListener> m_listener;
	PRBool			m_existedP;
	PRBool			m_wroteAnyP;
	PRBool			m_summaryValidP;
	PRInt32			m_numwrote;
	nsMsgKey    m_keyToDownload;
	nsCOMPtr <nsIMsgWindow>		m_window;
	nsresult				m_status;
	PRBool			m_abort;
};

typedef struct MSG_RetrieveArtInfo
{
	PRBool		m_useDefaults;
	PRBool		m_byReadness;
	PRBool		m_unreadOnly;
	PRBool		m_byDate;
	PRInt32		m_daysOld;
} MSG_RetrieveArtInfo;

class DownloadNewsArticlesToOfflineStore : public nsNewsDownloader
{
public:
	DownloadNewsArticlesToOfflineStore(nsIMsgWindow *window, nsIMsgDatabase *db, nsIUrlListener *listener);
	virtual ~DownloadNewsArticlesToOfflineStore();

  NS_IMETHOD OnStartRunningUrl(nsIURI* url);
  NS_IMETHOD OnStopRunningUrl(nsIURI* url, nsresult exitCode);
protected:
	virtual PRInt32	StartDownload();
	virtual PRInt32 FinishDownload();
	virtual PRBool GetNextHdrToRetrieve();

	nsCOMPtr <nsISimpleEnumerator>	m_headerEnumerator;
	nsCOMPtr <nsIMsgDBHdr>	m_newsHeader;
//	MsgDocument		*m_dbWriteDocument;
};

class DownloadMatchingNewsArticlesToNewsDB : public DownloadNewsArticlesToOfflineStore
{
public:
	DownloadMatchingNewsArticlesToNewsDB(nsIMsgWindow *window, nsIMsgFolder *folder, nsIMsgDatabase *newsDB,  nsISupportsArray *termArray);
	virtual ~DownloadMatchingNewsArticlesToNewsDB();
static nsresult	SaveMatchingMessages(nsIMsgWindow *window, nsIMsgFolder *folder, nsIMsgDatabase *newsDB, nsISupportsArray *terms);
protected:
};

#endif
