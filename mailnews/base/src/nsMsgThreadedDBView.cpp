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
}

nsMsgThreadedDBView::~nsMsgThreadedDBView()
{
  /* destructor code */
}

NS_IMETHODIMP nsMsgThreadedDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount)
{
	nsresult rv;
	rv = nsMsgDBView::Open(folder, sortType, sortOrder, viewFlags, pCount);
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

	// list all the ids into m_keys.
	nsMsgKey startMsg = 0; 
	do
	{
		const PRInt32 kIdChunkSize = 200;
		PRInt32			numListed = 0;
		nsMsgKey	idArray[kIdChunkSize];
		PRInt32		flagArray[kIdChunkSize];
		char		levelArray[kIdChunkSize];

    rv = ListThreadIds(&startMsg, (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) != 0, idArray, flagArray, 
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
		if ((threadFlag & MSG_FLAG_IGNORED) && !(m_viewFlags & nsMsgViewFlagsType::kShowIgnored))
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
		if ((/*m_viewFlags & nsMsgViewFlagsType::kUnreadOnly || */(sortType != nsMsgViewSortType::byThread)) && flag & MSG_FLAG_ELIDED)
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
    if ((m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) == 0)
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
					// try adding in MSG_VIEW_FLAG_ISTHREAD flag for unreadonly view.
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

void	nsMsgThreadedDBView::OnExtraFlagChanged(nsMsgViewIndex index, PRUint32 extraFlag)
{
	if (IsValidIndex(index) && m_havePrevView)
	{
		nsMsgKey keyChanged = m_keys[index];
		nsMsgViewIndex prevViewIndex = m_prevIdArray.FindIndex(keyChanged);
		if (prevViewIndex != nsMsgViewIndex_None)
		{
			PRUint32 prevFlag = m_prevFlags.GetAt(prevViewIndex);
			// don't want to change the elided bit, or has children or is thread
			if (prevFlag & MSG_FLAG_ELIDED)
				extraFlag |= MSG_FLAG_ELIDED;
			else
				extraFlag &= ~MSG_FLAG_ELIDED;
			if (prevFlag & MSG_VIEW_FLAG_ISTHREAD)
				extraFlag |= MSG_VIEW_FLAG_ISTHREAD;
			else
				extraFlag &= ~MSG_VIEW_FLAG_ISTHREAD;
			if (prevFlag & MSG_VIEW_FLAG_HASCHILDREN)
				extraFlag |= MSG_VIEW_FLAG_HASCHILDREN;
			else
				extraFlag &= ~MSG_VIEW_FLAG_HASCHILDREN;
			m_prevFlags.SetAt(prevViewIndex, extraFlag);	// will this be right?
		}
	}
	// we don't really know what's changed, but to be on the safe side, set the sort invalid
	// so that reverse sort will pick it up.
	if (m_sortType == nsMsgViewSortType::byStatus || m_sortType == nsMsgViewSortType::byFlagged || 
      m_sortType == nsMsgViewSortType::byUnread || m_sortType == nsMsgViewSortType::byPriority)
		m_sortValid = PR_FALSE;
}

void nsMsgThreadedDBView::OnHeaderAddedOrDeleted()
{
	ClearPrevIdArray();
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
	if ((m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) && m_sortType == nsMsgViewSortType::byThread)
		ExpandAll();
	m_sortValid = PR_TRUE;
  if (sortType != nsMsgViewSortType::byThread)
    ExpandAll(); // for now, expand all and do a flat sort.

	Sort(sortType, sortOrder);
	if (sortType != nsMsgViewSortType::byThread)	// forget prev view, since it has everything expanded.
		ClearPrevIdArray();
	return NS_OK;
}

nsresult nsMsgThreadedDBView::OnNewHeader(nsMsgKey newKey, PRBool ensureListed)
{
	nsresult	rv = NS_OK;
	// views can override this behaviour, which is to append to view.
	// This is the mail behaviour, but threaded views want
	// to insert in order...
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(newKey, getter_AddRefs(msgHdr));
	if (NS_SUCCEEDED(rv) && msgHdr != nsnull)
	{
    PRUint32 msgFlags;
    msgHdr->GetFlags(&msgFlags);
    if ((m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) && !ensureListed && (msgFlags & MSG_FLAG_READ))
			return NS_OK;
		// Currently, we only add the header in a threaded view if it's a thread.
		// We used to check if this was the first header in the thread, but that's
		// a bit harder in the unreadOnly view. But we'll catch it below.
    if (! (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))// || msgHdr->GetMessageKey() == m_messageDB->GetKeyOfFirstMsgInThread(msgHdr->GetMessageKey()))
			rv = AddHdr(msgHdr);
		else	// need to find the thread we added this to so we can change the hasnew flag
				// added message to existing thread, but not to view
		{		// Fix flags on thread header.
			PRInt32 threadCount;
			PRUint32 threadFlags;
			nsMsgViewIndex threadIndex = ThreadIndexOfMsg(newKey, nsMsgViewIndex_None, &threadCount, &threadFlags);
			if (threadIndex != nsMsgViewIndex_None)
			{
				// check if this is now the new thread hdr
				PRUint32	flags = m_flags[threadIndex];
				// if we have a collapsed thread which just got a new
				// top of thread, change the keys array.
        char level = 0; // ### TODO
				if ((flags & MSG_FLAG_ELIDED) && level == 0 
					&& (!(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) || !(msgFlags & MSG_FLAG_READ)))
				{
          nsMsgKey msgKey;
          msgHdr->GetMessageKey(&msgKey);
					m_keys.SetAt(threadIndex, msgKey);
					NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
				}
				if (! (flags & MSG_VIEW_FLAG_HASCHILDREN))
				{
					flags |= MSG_VIEW_FLAG_HASCHILDREN | MSG_VIEW_FLAG_ISTHREAD;
          if (!(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly))
						flags |= MSG_FLAG_ELIDED;
					m_flags[threadIndex] = flags;
					NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
				}
				if (! (flags & MSG_FLAG_ELIDED))	// thread is expanded
				{								// insert child into thread
					PRUint8 level = 0;					// levels of other hdrs may have changed!
					PRUint32	newFlags = msgFlags;
          nsMsgViewIndex insertIndex = 0;
#ifdef HAVE_PORT
					 insertIndex = GetInsertInfoForNewHdr(newKey, threadIndex, &level);
#endif
					// this header is the new king! try collapsing the existing thread,
					// removing it, installing this header as king, and expanding it.
					if (level == 0)	
					{
						CollapseByIndex(threadIndex, nsnull);
						// call base class, so child won't get promoted.
						nsMsgDBView::RemoveByIndex(threadIndex);	
						newFlags |= MSG_VIEW_FLAG_ISTHREAD | MSG_VIEW_FLAG_HASCHILDREN | MSG_FLAG_ELIDED;
					}
					m_keys.InsertAt(insertIndex, newKey);
					m_flags.InsertAt(insertIndex, newFlags, 1);
					m_levels.InsertAt(insertIndex, level);
					NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
					NoteChange(insertIndex, 1, nsMsgViewNotificationCode::insertOrDelete);
					if (level == 0)
						ExpandByIndex(threadIndex, nsnull);
				}
			}
			else // adding msg to thread that's not in view.
			{
				nsCOMPtr <nsIMsgThread> threadHdr;
        m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
				if (threadHdr)
				{
          AddMsgToThreadNotInView(threadHdr, msgHdr, ensureListed);
				}
			}
		}
	}
	else
		rv = NS_MSG_MESSAGE_NOT_FOUND;
	return rv;
}

nsresult nsMsgThreadedDBView::AddMsgToThreadNotInView(nsIMsgThread *threadHdr, nsIMsgDBHdr *msgHdr, PRBool ensureListed)
{
  nsresult rv = NS_OK;
  PRUint32 threadFlags;
  threadHdr->GetFlags(&threadFlags);
	if (!(threadFlags & MSG_FLAG_IGNORED))
		rv = AddHdr(msgHdr);
  return rv;
}

// This method just removes the specified line from the view. It does
// NOT delete it from the database.
nsresult nsMsgThreadedDBView::RemoveByIndex(nsMsgViewIndex index)
{
	nsresult rv = NS_OK;
	PRInt32	flags;

	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;

	flags = m_flags[index];

  if (! (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
		return nsMsgDBView::RemoveByIndex(index);

#ifdef HAVE_PORT
  if ((flags & kIsThread) && !(flags & kElided) && (flags & kHasChildren))
	{
		// fix flags on thread header...Newly promoted message 
		// should have flags set correctly
		nsCOMPtr <nsIMsgThread> threadHdr; = m_db->GetThreadHdrForMsgID(m_idArray[index]);
		if (threadHdr)
		{
			MessageDBView::RemoveByIndex(index);
			nsCOMPtr <nsIMsgThread> nextThreadHdr = (index < GetSize()) 
				? m_messageDB->GetNeoThreadHdrForMsgID(m_idArray[index]) : 0;
			// make sure id of next message is really in the same thread
			// it might have been deleted from the view but not the db yet.
			if (threadHdr == nextThreadHdr && threadHdr->GetNumChildren() > 1)
			{
				// unreadOnly
				nsCOMPtr <nsIMsgDBHdr> msgHdr = threadHdr->GetChildHdrAt(1);
				if (msgHdr != NULL)
				{
					char flag = 0;
					CopyDBFlagsToExtraFlags(msgHdr->GetFlags(), &flag);
					if (threadHdr->GetNumChildren() > 2)
						flag |= kIsThread | kHasChildren;
					m_flags.SetAtGrow(index, (uint8) flag);
					m_levels.SetAtGrow(index, 0);
				}
			}
		}
		return err;
	}
	else if (!(flags & kIsThread))
	{
		return MessageDBView::RemoveByIndex(index);
	}
	// deleting collapsed thread header is special case. Child will be promoted,
	// so just tell FE that line changed, not that it was deleted
	nsCOMPtr <nsIMsgThread> threadHdr = m_messageDB->GetNeoThreadHdrForMsgID(m_idArray[index]);
	if (threadHdr && threadHdr->GetNumChildren() > 1)
	{
		// change the id array and flags array to reflect the child header.
		// If we're not deleting the header, we want the second header,
		// Otherwise, the first one (which just got promoted).
		nsCOMPtr <nsIMsgDBHdr> msgHdr = threadHdr->GetChildHdrAt(1);
		if (msgHdr != NULL)
		{
			m_keys.SetAt(index, msgHdr->GetMessageKey());

			char flag = 0;
			CopyDBFlagsToExtraFlags(msgHdr->GetFlags(), &flag);
//			if (msgHdr->GetArticleNum() == msgHdr->GetThreadId())
				flag |= kIsThread;

			if (threadHdr->GetNumChildren() == 2)	// if only hdr in thread (with one about to be deleted)
													// adjust flags.
			{
				flag &=  ~kHasChildren;
				flag &= ~kElided;
				// tell FE that thread header needs to be repainted.
				if (index > 0)
					NoteChange(index - 1, 0, MSG_NotifyInsertOrDelete);
			}
			else
			{
				flag |= kHasChildren;
				flag |= kElided;
			}
			m_flags[index] = flag;
		}
		else
			XP_ASSERT(FALSE);	
		NoteChange(index, 0, MSG_NotifyInsertOrDelete);	// horrible hack to tell fe that the key has changed
	}
	else
		err = MessageDBView::RemoveByIndex(index);
#endif
	return rv;
}

