/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsMsgThreadedDBView.h"
#include "nsIMsgHdr.h"
#include "nsIMsgThread.h"

// "imap://bienvenu@nsmail-1/INBOX"
// chrome://messenger/content/dbviewtest.xul
nsMsgThreadedDBView::nsMsgThreadedDBView()
{
  /* member initializers and constructor code */
	m_havePrevView = PR_FALSE;
  m_viewType = nsMsgDBViewType::allThreads; // by default
}

nsMsgThreadedDBView::~nsMsgThreadedDBView()
{
  /* destructor code */
}

NS_IMETHODIMP nsMsgThreadedDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue viewType, PRInt32 *pCount)
{
	nsresult rv;
  m_sortType = viewType;
  m_sortOrder = nsMsgViewSortOrder::ascending;
	rv = nsMsgDBView::Open(folder, viewType, pCount);
  NS_ENSURE_SUCCESS(rv, rv);

	if (pCount)
		*pCount = 0;
	return Init(pCount);
}

NS_IMETHODIMP nsMsgThreadedDBView::Close()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgThreadedDBView::Init(PRInt32 *pCount)
{
	nsresult rv;

	m_keys.RemoveAll();
	m_flags.RemoveAll();
	m_levels.RemoveAll(); 
	m_prevIdArray.RemoveAll();
	m_prevFlags.RemoveAll();
	m_prevLevels.RemoveAll();
	m_havePrevView = PR_FALSE;
	nsresult getSortrv = NS_OK; // ### TODO m_db->GetSortInfo(&sortType, &sortOrder);

	// list all the ids into m_idArray.
	nsMsgKey startMsg = 0; 
	do
	{
		const PRInt32 kIdChunkSize = 200;
		PRInt32			numListed = 0;
		nsMsgKey	idArray[kIdChunkSize];
		PRInt32		flagArray[kIdChunkSize];
		char		levelArray[kIdChunkSize];

    rv = ListThreadIds(&startMsg, m_viewType == nsMsgDBViewType::onlyUnreadHeaders, idArray, flagArray, 
                    levelArray, kIdChunkSize, &numListed, nsnull);
		if (NS_SUCCEEDED(rv))
		{
			PRInt32 numAdded = AddKeys(idArray, flagArray, levelArray, m_sortType, numListed);
			if (pCount)
				*pCount += numAdded;
		}

	} while (NS_SUCCEEDED(rv) && startMsg != nsMsgKey_None);

	if (NS_SUCCEEDED(getSortrv))
	{
		InitSort(m_sortType, m_sortOrder);
	}
	return rv;
}

NS_IMETHODIMP nsMsgThreadedDBView::AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd)

{
	PRInt32	numAdded = 0;
	for (PRInt32 i = 0; i < numKeysToAdd; i++)
	{
		PRInt32 threadFlag = pFlags[i];
		PRInt32 flag = threadFlag;

		// skip ignored threads.
		if ((threadFlag & MSG_FLAG_IGNORED) && !(m_viewFlags & kShowIgnored))
			continue;
		// by default, make threads collapsed, unless we're in only viewing new msgs

		if (flag & MSG_VIEW_FLAG_HASCHILDREN)
			flag |= MSG_FLAG_ELIDED;
		// should this be persistent? Doesn't seem to need to be.
		flag |= MSG_VIEW_FLAG_ISTHREAD;
		m_keys.Add(pKeys[i]);
		m_flags.Add(flag);
		m_levels.Add(pLevels[i]);
		numAdded++;
		if ((/*m_viewFlags & kUnreadOnly || */(sortType != nsMsgViewSortType::byThread)) && flag & MSG_FLAG_ELIDED)
		{
			ExpandByIndex(m_keys.GetSize() - 1, NULL);
		}
	}
	return numAdded;
}

NS_IMETHODIMP nsMsgThreadedDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
    nsresult rv;
    rv = nsMsgDBView::Sort(sortType,sortOrder);
    NS_ENSURE_SUCCESS(rv,rv);
    return rv;
}



// list the ids of the top-level thread ids starting at id == startMsg. This actually returns
// the ids of the first message in each thread.
nsresult nsMsgThreadedDBView::ListThreadIds(nsMsgKey *startMsg, PRBool unreadOnly, nsMsgKey *pOutput, PRInt32 *pFlags, char *pLevels, 
									 PRInt32 numToList, PRInt32 *pNumListed, PRInt32 *pTotalHeaders)
{
	nsresult rv = NS_OK;
	// N.B..don't ret before assigning numListed to *pNumListed
	PRInt32				numListed = 0;

	if (*startMsg > 0)
	{
		NS_ASSERTION(m_threadEnumerator != nsnull, "where's our iterator?");	// for now, we'll just have to rely on the caller leaving
									// the iterator in the right place.
	}
	else
	{
    PRUint32 viewType;
    if (m_viewType == nsMsgDBViewType::allThreads)
      viewType = nsMsgViewType::eShowAll;
    else
      viewType = nsMsgViewType::eShowUnread;

    rv = m_db->EnumerateThreads(viewType, getter_AddRefs(m_threadEnumerator));
    NS_ENSURE_SUCCESS(rv, rv);
	}

	PRBool hasMore = PR_FALSE;

  nsCOMPtr <nsIMsgThread> threadHdr ;
	while (NS_SUCCEEDED(rv = m_threadEnumerator->HasMoreElements(&hasMore)) && (hasMore == PR_TRUE)) 
	{
    nsCOMPtr <nsISupports> supports;
    rv = m_threadEnumerator->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv,rv);
    threadHdr = do_QueryInterface(supports);

		PRInt32	threadsRemoved = 0;
		for (PRInt32 i = 0; i < numToList && threadHdr != nsnull; i++)
		{
    	nsCOMPtr <nsIMsgDBHdr> msgHdr;
      PRUint32 numChildren;
      threadHdr->GetNumChildren(&numChildren);
			PRUint32 threadFlags;
      threadHdr->GetFlags(&threadFlags);
			if (numChildren != 0)	// not empty thread
			{
				if (pTotalHeaders)
					*pTotalHeaders += numChildren;
    		if (unreadOnly)
					rv = threadHdr->GetFirstUnreadChild(getter_AddRefs(msgHdr));
				else
					rv = threadHdr->GetChildAt(0, getter_AddRefs(msgHdr));
				if (NS_SUCCEEDED(rv) && msgHdr != nsnull && WantsThisThread(threadHdr))
				{
          PRUint32 msgFlags;
          PRUint32 newMsgFlags;
          nsMsgKey msgKey;
          msgHdr->GetMessageKey(&msgKey);
          msgHdr->GetFlags(&msgFlags);
          // turn off high byte of msg flags - used for view flags.
          msgFlags &= ~MSG_VIEW_FLAGS;
					pOutput[numListed] = msgKey;
					pLevels[numListed] = 0;
					// DMB TODO - This will do for now...Until we decide how to
					// handle thread flags vs. message flags, if we do decide
					// to make them different.
					msgHdr->OrFlags(threadFlags & (MSG_FLAG_WATCHED | MSG_FLAG_IGNORED), &newMsgFlags);
					PRBool	isRead = PR_FALSE;

					// make sure DB agrees with newsrc, if we're news.
					m_db->IsRead(msgKey, &isRead);
					m_db->MarkHdrRead(msgHdr, isRead, nsnull);
					// try adding in kIsThread flag for unreadonly view.
					pFlags[numListed] = msgFlags | MSG_VIEW_FLAG_ISTHREAD | threadFlags;
					if (numChildren > 1)
						pFlags[numListed] |= MSG_VIEW_FLAG_HASCHILDREN;

					numListed++;
				}
			}
			else if (threadsRemoved < 10 && !(threadFlags & (MSG_FLAG_WATCHED | MSG_FLAG_IGNORED)))
			{
				// ### remove thread.
				threadsRemoved++;	// don't want to remove all empty threads first time
									// around as it will choke preformance for upgrade.
#ifdef DEBUG_bienvenu
				printf("removing empty non-ignored non-watched thread\n");
#endif
			}
      rv = m_threadEnumerator->GetNext(getter_AddRefs(supports));
      threadHdr = do_QueryInterface(supports);
		}
    if (numListed >= numToList)
      break;
	}

	if (threadHdr)
	{
    threadHdr->GetThreadKey(startMsg);
	}
	else
	{
		*startMsg = nsMsgKey_None;
		m_threadEnumerator = nsnull;
	}
  *pNumListed = numListed;
	return rv;
}

nsresult	nsMsgThreadedDBView::ExpandAll()
{
	nsresult rv = NS_OK;
	// go through expanding in place 
	for (PRUint32 i = 0; i < m_keys.GetSize(); i++)
	{
		PRUint32	numExpanded;
		PRUint32	flags = m_flags[i];
		if (flags & MSG_VIEW_FLAG_HASCHILDREN && (flags & MSG_FLAG_ELIDED))
		{
			rv = ExpandByIndex(i, &numExpanded);
			i += numExpanded;
			NS_ENSURE_SUCCESS(rv, rv);
		}
	}
	return rv;
}

void nsMsgThreadedDBView::ClearPrevIdArray()
{
  m_prevIdArray.RemoveAll();
  m_prevFlags.RemoveAll();
  m_havePrevView = PR_FALSE;
}

nsresult nsMsgThreadedDBView::InitSort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
  if (sortType == nsMsgViewSortType::byThread)
	{
//		SortInternal(nsMsgViewSortType::byId, sortOrder); // sort top level threads by id.
		m_sortType = nsMsgViewSortType::byThread;
//		m_db->SetSortInfo(m_sortType, sortOrder);
	}
  // by default, the unread only view should have all threads expanded.
	if ((m_viewFlags & kUnreadOnly) && m_sortType == nsMsgViewSortType::byThread)
		ExpandAll();
	m_sortValid = PR_TRUE;
  if (sortType != nsMsgViewSortType::byThread)
    ExpandAll(); // for now, expand all and do a flat sort.

	Sort(sortType, sortOrder);
	if (sortType != nsMsgViewSortType::byThread)	// forget prev view, since it has everything expanded.
		ClearPrevIdArray();
	return NS_OK;
}

