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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "msgCore.h"
#include "nntpCore.h"
#include "nsXPIDLString.h"
#include "nsIMsgNewsFolder.h"
#include "nsIStringBundle.h"
#include "nsNewsDownloader.h"
#include "nsINntpService.h"
#include "nsMsgNewsCID.h"
#include "nsIMsgSearchSession.h"
#include "nsIMsgSearchTerm.h"

// This file contains the news article download state machine.

static NS_DEFINE_CID(kNntpServiceCID,	NS_NNTPSERVICE_CID);


// if pIds is not null, download the articles whose id's are passed in. Otherwise,
// which articles to download is determined by nsNewsDownloader object,
// or subclasses thereof. News can download marked objects, for example.
nsresult nsNewsDownloader::DownloadArticles(nsIMsgWindow *window, nsIMsgFolder *folder, nsMsgKeyArray *pIds)
{
	if (pIds != nsnull)
		m_keysToDownload.InsertAt(0, pIds);

	if (m_keysToDownload.GetSize() > 0)
		m_downloadFromKeys = PR_TRUE;

	m_folder = folder;
  m_window = window;

	PRBool headersToDownload = GetNextHdrToRetrieve();
  // should we have a special error code for failure here?
  return (headersToDownload) ? DownloadNext(PR_TRUE) : NS_ERROR_FAILURE;
}

/* Saving news messages
 */

NS_IMPL_ISUPPORTS2(nsNewsDownloader, nsIUrlListener, nsIMsgSearchNotify)

nsNewsDownloader::nsNewsDownloader(nsIMsgWindow *window, nsIMsgDatabase *msgDB, nsIUrlListener *listener)
{
	m_numwrote = 0;
	m_downloadFromKeys = PR_FALSE;
	m_newsDB = msgDB;
	m_abort = PR_FALSE;
  m_listener = listener;
  m_window = window;
  NS_INIT_REFCNT();
}

nsNewsDownloader::~nsNewsDownloader()
{
	if (m_listener)
	  m_listener->OnStopRunningUrl(/* don't have a url */nsnull, m_status);
	if (m_newsDB)
	{
		m_newsDB->Commit(nsMsgDBCommitType::kLargeCommit);
		m_newsDB = nsnull;
	}
}

NS_IMETHODIMP nsNewsDownloader::OnStartRunningUrl(nsIURI* url)
{
  return NS_OK;
}

NS_IMETHODIMP nsNewsDownloader::OnStopRunningUrl(nsIURI* url, nsresult exitCode)
{
  nsresult rv = exitCode;
  if (NS_SUCCEEDED(exitCode))
    rv = DownloadNext(PR_FALSE);

  return rv;
}

nsresult nsNewsDownloader::DownloadNext(PRBool firstTimeP)
{
  nsresult rv;
  PRBool moreHeaders = GetNextHdrToRetrieve();
  if (!moreHeaders)
    return NS_OK;
//		return 0;
//	char *url = m_folder->BuildUrl(m_newsDB, m_keyToDownload);
	m_numwrote++;

	StartDownload();
	m_wroteAnyP = PR_FALSE;
  NS_WITH_SERVICE(nsINntpService, nntpService, kNntpServiceCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return nntpService->FetchMessage(m_folder, m_keyToDownload, m_window, nsnull, this, nsnull);
#ifdef DEBUG_bienvenu
//	XP_Trace("downloading %s\n", url);
#endif

//	return NS_OK;
}

PRBool DownloadNewsArticlesToOfflineStore::GetNextHdrToRetrieve()
{
	nsresult rv;

	if (m_downloadFromKeys)
		return nsNewsDownloader::GetNextHdrToRetrieve();

	if (m_headerEnumerator == nsnull)
		rv = m_newsDB->EnumerateMessages(getter_AddRefs(m_headerEnumerator));

	PRBool hasMore = PR_FALSE;

	while (NS_SUCCEEDED(rv = m_headerEnumerator->HasMoreElements(&hasMore)) && (hasMore == PR_TRUE)) 
	{
    nsCOMPtr <nsISupports> supports;
    rv = m_headerEnumerator->GetNext(getter_AddRefs(supports));
    m_newsHeader = do_QueryInterface(supports);
    NS_ENSURE_SUCCESS(rv,rv);
    PRUint32 hdrFlags;
    m_newsHeader->GetFlags(&hdrFlags);
		if (hdrFlags & MSG_FLAG_MARKED)
		{
			m_newsHeader->GetMessageKey(&m_keyToDownload);
#ifdef HAVE_PORT
			char *statusTemplate = XP_GetString (MK_MSG_RETRIEVING_ARTICLE);
			char *statusString = PR_smprintf (statusTemplate,  m_numwrote);
			if (statusString)
			{
				FE_Progress (m_context, statusString);
				XP_FREE(statusString);
			}
#endif
			break;
		}
		else
		{
			m_newsHeader = nsnull;
		}
	}
#ifdef HAVE_PORT
	if (m_newsHeader && m_dbWriteDocument)
	{
		m_dbWriteDocument->SetMessageHdr(m_newsHeader, m_newsDB);
	}
#endif
	return hasMore;
}

void nsNewsDownloader::Abort() {}
void nsNewsDownloader::Complete() {}

PRBool nsNewsDownloader::GetNextHdrToRetrieve()
{
  nsresult rv;
	if (m_downloadFromKeys)
	{
		if (m_numwrote >= (PRInt32) m_keysToDownload.GetSize())
			return PR_FALSE;
		m_keyToDownload = m_keysToDownload.GetAt(m_numwrote);
#ifdef DEBUG_bienvenu
//		XP_Trace("downloading %ld index = %ld\n", m_keyToDownload, m_numwrote);
#endif
    nsCOMPtr <nsIMsgNewsFolder> newsFolder = do_QueryInterface(m_folder);
//		PRInt32 stringID = (newsFolder ? MK_MSG_RETRIEVING_ARTICLE_OF : MK_MSG_RETRIEVING_MESSAGE_OF);
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle(NEWS_MSGS_URL, nsnull, getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString firstStr;
    firstStr.AppendInt(m_numwrote);
    nsAutoString totalStr;
    totalStr.AppendInt(m_keysToDownload.GetSize());
    nsXPIDLString prettiestName;
    nsXPIDLString statusString;

    m_folder->GetPrettiestName(getter_Copies(prettiestName));

    const PRUnichar *formatStrings[3] = { firstStr.GetUnicode(), totalStr.GetUnicode(), (const PRUnichar *) prettiestName };
    rv = bundle->FormatStringFromName(NS_LITERAL_STRING("downloadingArticlesForOffline"), formatStrings, 3, getter_Copies(statusString));
    NS_ENSURE_SUCCESS(rv, rv);
    // ### TODO set status string on window?
		PRInt32 percent;
		percent = (100 * m_numwrote) / (PRInt32) m_keysToDownload.GetSize();
		// FE_SetProgressBarPercent (m_context, percent);
		return PR_TRUE;
	}
	NS_ASSERTION(PR_FALSE, "shouldn't get here if we're not downloading from keys.");
	return PR_FALSE;	// shouldn't get here if we're not downloading from keys.
}

NS_IMETHODIMP DownloadNewsArticlesToOfflineStore::OnStartRunningUrl(nsIURI* url)
{
  return NS_OK;
}


NS_IMETHODIMP DownloadNewsArticlesToOfflineStore::OnStopRunningUrl(nsIURI* url, nsresult exitCode)
{
	m_status = exitCode;
	if (m_newsHeader != nsnull)
	{
#ifdef DEBUG_bienvenu
//		XP_Trace("finished retrieving %ld\n", m_newsHeader->GetMessageKey());
#endif
		if (m_newsDB)
		{
      nsMsgKey msgKey;
      m_newsHeader->GetMessageKey(&msgKey);
			m_newsDB->MarkMarked(msgKey, PR_FALSE, nsnull);
		}
	}	
	m_newsHeader = nsnull;
	return nsNewsDownloader::OnStopRunningUrl(url, exitCode);
}

int DownloadNewsArticlesToOfflineStore::FinishDownload()
{
	return 0;
}


NS_IMETHODIMP nsNewsDownloader::OnSearchHit(nsIMsgDBHdr *header, nsIMsgFolder *folder)
{
  NS_ENSURE_ARG(header);

  
  PRUint32 msgFlags;
  header->GetFlags(&msgFlags);
	// only need to download articles we don't already have...
	if (! (msgFlags & MSG_FLAG_OFFLINE))
	{
    nsMsgKey key;
    header->GetMessageKey(&key);
		m_keysToDownload.Add(key);
#ifdef HAVE_PORT
		char *statusTemplate = XP_GetString (MK_MSG_ARTICLES_TO_RETRIEVE);
		char *statusString = PR_smprintf (statusTemplate,  (long) newsArticleState->m_keysToDownload.GetSize());
		if (statusString)
		{
			FE_Progress (newsArticleState->m_context, statusString);
			XP_FREE(statusString);
		}
#endif
	}
  return NS_OK;
}

NS_IMETHODIMP nsNewsDownloader::OnSearchDone(nsresult status)
{
  // kick off download process?
  return NS_OK;
}
NS_IMETHODIMP nsNewsDownloader::OnNewSearch()
{
  return NS_OK;
}

int DownloadNewsArticlesToOfflineStore::StartDownload()
{
	m_newsDB->GetMsgHdrForKey(m_keyToDownload, getter_AddRefs(m_newsHeader));
	return 0;
}

DownloadNewsArticlesToOfflineStore::DownloadNewsArticlesToOfflineStore(nsIMsgWindow *window, nsIMsgDatabase *db, nsIUrlListener *listener)
	: nsNewsDownloader(window, db, listener)
{
	m_newsDB = db;
}

DownloadNewsArticlesToOfflineStore::~DownloadNewsArticlesToOfflineStore()
{
}

/*static*/ nsresult DownloadMatchingNewsArticlesToNewsDB::SaveMatchingMessages(nsIMsgWindow *window, nsIMsgFolder *folder, nsIMsgDatabase *newsDB,
							nsISupportsArray *termArray)
{
  nsresult rv;
  NS_ENSURE_ARG(termArray);
  nsCOMPtr <nsIMsgSearchSession> searchSession = do_CreateInstance(NS_MSGSEARCHSESSION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
	DownloadMatchingNewsArticlesToNewsDB *downloadState = 
		new DownloadMatchingNewsArticlesToNewsDB(window, folder, newsDB, termArray);

  searchSession->RegisterListener(downloadState);
  searchSession->AddScopeTerm(nsMsgSearchScope::OfflineNewsgroup, folder);
  PRUint32 termCount;
  termArray->Count(&termCount);
	for (PRUint32 i = 0; i < termCount; i++)
	{
    nsCOMPtr<nsIMsgSearchTerm> term;
    termArray->QueryElementAt(i, NS_GET_IID(nsIMsgSearchTerm),
                               (void **)getter_AddRefs(term));
		searchSession->AppendTerm(term);
	}
	return searchSession->Search(window);
}

DownloadMatchingNewsArticlesToNewsDB::DownloadMatchingNewsArticlesToNewsDB
	(nsIMsgWindow *window, nsIMsgFolder *folder, nsIMsgDatabase *newsDB,
	 nsISupportsArray * /*termArray*/) :
	 DownloadNewsArticlesToOfflineStore(window, newsDB, nsnull /* url listener */)
{
	m_window = window;
	m_folder = folder;
	m_newsDB = newsDB;
	m_downloadFromKeys = PR_TRUE;	// search term matching means downloadFromKeys.
}

DownloadMatchingNewsArticlesToNewsDB::~DownloadMatchingNewsArticlesToNewsDB()
{
}


