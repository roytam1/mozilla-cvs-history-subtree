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
#include "nsMsgDBView.h"
#include "nsISupports.h"
#include "nsIMsgFolder.h"
#include "nsIDBFolderInfo.h"
#include "nsIDBFolderInfo.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgFolder.h"
#include "MailNewsTypes2.h"
#include "nsMsgUtils.h"
#include "nsXPIDLString.h"
#include "nsQuickSort.h"

/* Implementation file */

NS_IMPL_ISUPPORTS2(nsMsgDBView, nsIMsgDBView, nsIDBChangeListener)

nsMsgDBView::nsMsgDBView()
{
  NS_INIT_ISUPPORTS();
  m_viewType = nsMsgDBViewType::anyView;
  /* member initializers and constructor code */
  m_sortValid = PR_TRUE;
  m_sortOrder = nsMsgViewSortOrder::none;
  m_viewFlags = (nsMsgDBViewFlags) 0;
}

nsMsgDBView::~nsMsgDBView()
{
  /* destructor code */
}

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue viewType, PRInt32 *pCount)
{
  NS_ENSURE_ARG(folder);
  nsCOMPtr <nsIDBFolderInfo> folderInfo;

  nsresult rv = folder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_db));
  NS_ENSURE_SUCCESS(rv,rv);
	m_db->AddListener(this);
#ifdef HAVE_PORT
	if (viewType == ViewAny)
		viewType = ViewAllThreads;
	m_viewType = viewType;
	if (messageDB && messageDB->GetNeoFolderInfo()->GetFlags() & MSG_FOLDER_PREF_SHOWIGNORED)
		m_viewFlags = (ViewFlags) ((int32) kShowIgnored | (int32) m_viewFlags);
	if (messageDB && messageDB->GetNeoFolderInfo() 
		&& (viewType == ViewOnlyNewHeaders))
		m_viewFlags = (ViewFlags) ((int32) kUnreadOnly | (int32) m_viewFlags);

	CacheAdd ();
#endif
	return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Close()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::Init(PRInt32 *pCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

// reversing threads involves reversing the threads but leaving the
// expanded messages ordered relative to the thread, so we
// make a copy of each array and copy them over.
nsresult nsMsgDBView::ReverseThreads()
{
    nsUInt32Array *newFlagArray = new nsUInt32Array;
    if (!newFlagArray) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsMsgKeyArray *newKeyArray = new nsMsgKeyArray;
    if (!newKeyArray) {
        delete newFlagArray;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsUint8Array *newLevelArray = new nsUint8Array;
    if (!newLevelArray) {
        delete newFlagArray;
        delete newKeyArray;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    PRInt32 sourceIndex, destIndex;
    PRInt32 viewSize = GetSize();

    newKeyArray->SetSize(m_keys.GetSize());
    newFlagArray->SetSize(m_flags.GetSize());
    newLevelArray->SetSize(m_levels.GetSize());

    for (sourceIndex = 0, destIndex = viewSize - 1; sourceIndex < viewSize;) {
        PRInt32 endThread;  // find end of current thread.
        PRBool inExpandedThread = PR_FALSE;
        for (endThread = sourceIndex; endThread < viewSize; endThread++) {
            PRUint32 flags = m_flags.GetAt(endThread);
            if (!inExpandedThread && (flags & (MSG_VIEW_FLAG_ISTHREAD|MSG_VIEW_FLAG_HASCHILDREN)) && !(flags & MSG_FLAG_ELIDED))
                inExpandedThread = PR_TRUE;
            else if (flags & MSG_VIEW_FLAG_ISTHREAD) {
                if (inExpandedThread)
                    endThread--;
                break;
            }
        }

        if (endThread == viewSize)
            endThread--;
        PRInt32 saveEndThread = endThread;
        while (endThread >= sourceIndex)
        {
            newKeyArray->SetAt(destIndex, m_keys.GetAt(endThread));
            newFlagArray->SetAt(destIndex, m_flags.GetAt(endThread));
            newLevelArray->SetAt(destIndex, m_levels.GetAt(endThread));
            endThread--;
            destIndex--;
        }
        sourceIndex = saveEndThread + 1;
    }
    // this copies the contents of both arrays - it would be cheaper to
    // just assign the new data ptrs to the old arrays and "forget" the new
    // arrays' data ptrs, so they won't be freed when the arrays are deleted.
    m_keys.RemoveAll();
    m_flags.RemoveAll();
    m_levels.RemoveAll();
    m_keys.InsertAt(0, newKeyArray);
    m_flags.InsertAt(0, newFlagArray);
    m_levels.InsertAt(0, newLevelArray);

    // if we swizzle data pointers for these arrays, this won't be right.
    delete newFlagArray;
    delete newKeyArray;
    delete newLevelArray;

    return NS_OK;
}

nsresult nsMsgDBView::ReverseSort()
{
    PRUint32 num = GetSize();

    // go up half the array swapping values
    for (PRUint32 i = 0; i < (num / 2); i++) {
        // swap flags
        PRUint32 end = num - i - 1;
        PRUint32 tempFlags = m_flags.GetAt(i);
        m_flags.SetAt(i, m_flags.GetAt(end));
        m_flags.SetAt(end, tempFlags);

        // swap keys
        nsMsgKey tempKey = m_keys.GetAt(i);
        m_keys.SetAt(i, m_keys.GetAt(end));
        m_keys.SetAt(end, tempKey);

        // no need to swap elements in m_levels, 
        // since we won't call ReverseSort() if we
        // are in threaded mode, so m_levels are all the same.
    }

    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::DumpView()
{
    PRUint32 i,j;
    nsresult rv;
    PRUint32 num = GetSize();
    printf("[row]\t(key,flag,level,subject)\n");
    for (i = 0; i < num; i++) {
        PRUint32 flags = m_flags.GetAt(i);
        PRUint32 level = m_levels.GetAt(i);
        printf("[%d]\t",i);
        if (m_sortType == nsMsgViewSortType::byThread) {
            if (flags | MSG_FLAG_ELIDED) {
                printf("+");
            }
            else {
                printf("-");
            }
            for (j=0;j<level;j++) {
                printf(".");
            }
        }
        else {
            printf(" ");
        }
        
        nsMsgKey key = m_keys.GetAt(i);
        nsCOMPtr <nsIMsgDBHdr> msgHdr;
        rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
        NS_ENSURE_SUCCESS(rv,rv);

        nsXPIDLCString subject;
        rv = msgHdr->GetSubject(getter_Copies(subject));
        NS_ENSURE_SUCCESS(rv,rv);
        printf("(%d,%d,%d,%s)\n",key,flags,level,(const char *)subject);
    }
    printf("\n");
    return NS_OK;
}

typedef struct entryInfo
{
    nsMsgKey    id;
    PRUint32    bits;
} EntryInfo;

typedef struct tagIdStr{
    EntryInfo   info;
    char        str[1];
} IdStr;

int PR_CALLBACK
FnSortIdStr(const void *pItem1, const void *pItem2, void *privateData)
{
    IdStr** p1 = (IdStr**)pItem1;
    IdStr** p2 = (IdStr**)pItem2;
    int retVal = nsCRT::strcmp((*p1)->str, (*p2)->str); // used to be strcasecmp, but INTL sorting routine lower cases it.
    if (retVal != 0)
        return(retVal);
    if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

typedef struct tagIdDWord{
    EntryInfo   info;
    PRUint32 dword;
} IdDWord;

int PR_CALLBACK
FnSortIdDWord(const void *pItem1, const void *pItem2, void *privateData)
{
    IdDWord** p1 = (IdDWord**)pItem1;
    IdDWord** p2 = (IdDWord**)pItem2;
    if ((*p1)->dword > (*p2)->dword)
        return(1);
    else if ((*p1)->dword < (*p2)->dword)
        return(-1);
    else if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

typedef struct tagIdPRTime{
    EntryInfo   info;
    PRTime prtime;
} IdPRTime;

int PR_CALLBACK
FnSortIdPRTime(const void *pItem1, const void *pItem2, void *privateData)
{
    IdPRTime ** p1 = (IdPRTime**)pItem1;
    IdPRTime ** p2 = (IdPRTime**)pItem2;

    if (LL_CMP((*p1)->prtime, >, (*p2)->prtime)) 
        return(1);
    else if (LL_CMP((*p1)->prtime, <, (*p2)->prtime))  
        return(-1);
    else if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

/* better place for these? */
const int kMaxSubject = 160;
const int kMaxAuthor = 160;
const int kMaxRecipient = 80;
const int kMaxMsgIdLen = 80;
const int kMaxReferenceLen = 10 * kMaxMsgIdLen;

nsresult nsMsgDBView::GetFieldTypeAndLenForSort(nsMsgViewSortTypeValue sortType, PRUint16 *pMaxLen, eFieldType *pFieldType)
{
    NS_ENSURE_ARG_POINTER(pMaxLen);
    NS_ENSURE_ARG_POINTER(pFieldType);

    switch (sortType) {
        case nsMsgViewSortType::bySubject:
            *pFieldType = kString;
            *pMaxLen = kMaxSubject;
            break;
        case nsMsgViewSortType::byRecipient:
            *pFieldType = kString;
            *pMaxLen = kMaxRecipient;
            break;
        case nsMsgViewSortType::byAuthor:
            *pFieldType = kString;
            *pMaxLen = kMaxAuthor;
            break;
        case nsMsgViewSortType::byDate:
            *pFieldType = kPRTime;
            *pMaxLen = sizeof(PRTime);
            break;
        case nsMsgViewSortType::byPriority:
        case nsMsgViewSortType::byThread:
        case nsMsgViewSortType::byId:
        case nsMsgViewSortType::bySize:
        case nsMsgViewSortType::byFlagged:
        case nsMsgViewSortType::byUnread:
        case nsMsgViewSortType::byStatus:
            *pFieldType = kU32;
            *pMaxLen = sizeof(PRUint32);
            break;
        default:
            return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}

nsresult nsMsgDBView::GetPRTimeField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRTime *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  switch (sortType) {
    case nsMsgViewSortType::byDate:
        rv = msgHdr->GetDate(result);
        break;
    default:
        NS_ASSERTION(0,"should not be here");
        rv = NS_ERROR_UNEXPECTED;
    }

    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}

nsresult nsMsgDBView::GetLongField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  nsMsgKey key;
  PRBool isRead;
  PRUint32 bits;

  switch (sortType) {
    case nsMsgViewSortType::bySize:
        rv = msgHdr->GetMessageSize(result);
        break;
    case nsMsgViewSortType::byPriority: 
        // want highest priority to have lowest value
        // so ascending sort will have highest priority first.
        nsMsgPriorityValue priority;
        rv = msgHdr->GetPriority(&priority);
        *result = nsMsgPriority::highest - priority;
        break;
    case nsMsgViewSortType::byStatus:
        rv = msgHdr->GetStatusOffset(result);
        break;
    case nsMsgViewSortType::byFlagged:
        bits = 0;
        rv = msgHdr->GetFlags(&bits);
        *result = !(bits & MSG_FLAG_MARKED);  //make flagged come out on top.
        break;
    case nsMsgViewSortType::byUnread:
        rv = msgHdr->GetMessageKey(&key);
        if (NS_SUCCEEDED(rv)) {
            isRead = PR_FALSE;
            rv = m_db->IsRead(key, &isRead);
            *result = !isRead;
        }
        break;
    case nsMsgViewSortType::byId:
        // handled by caller, since caller knows the key
    default:
        NS_ASSERTION(0,"should not be here");
        rv = NS_ERROR_UNEXPECTED;
        break;
    }
    
    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}


nsresult nsMsgDBView::GetStringField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUnichar **result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  switch (sortType) {
    case nsMsgViewSortType::bySubject:
        rv = msgHdr->GetSubjectCollationKey(result);
        break;
    case nsMsgViewSortType::byRecipient:
        rv = msgHdr->GetRecipientsCollationKey(result);
        break;
    case nsMsgViewSortType::byAuthor:
        rv = msgHdr->GetAuthorCollationKey(result);
        break;
    default:
        rv = NS_ERROR_UNEXPECTED;
        break;
    }

    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
    nsresult rv;

    NS_ASSERTION(m_db, "no db");
    if (!m_db) return NS_ERROR_FAILURE;

    printf("XXX nsMsgDBView::Sort(%d,%d)\n",(int)sortType,(int)sortOrder);
    if (m_sortType == sortType && m_sortValid) {
        if (m_sortOrder == sortOrder) {
            printf("XXX same as it ever was.  do nothing\n");
            return NS_OK;
        }   
        else {
            if (m_sortType != nsMsgViewSortType::byThread) {
                rv = ReverseSort();
                NS_ENSURE_SUCCESS(rv,rv);
            }
            else {
                rv = ReverseThreads();
                NS_ENSURE_SUCCESS(rv,rv);
            }

            m_sortOrder = sortOrder;
            return NS_OK;
        }
    }

    if (sortType == nsMsgViewSortType::byThread) {
        return NS_OK;
    }

    // figure out how much memory we'll need, and the malloc it
    PRUint16 maxLen;
    eFieldType fieldType;

    rv = GetFieldTypeAndLenForSort(sortType, &maxLen, &fieldType);
    NS_ENSURE_SUCCESS(rv,rv);

    nsVoidArray ptrs;
    PRUint32 arraySize = GetSize();
    IdStr** pPtrBase = (IdStr**)PR_Malloc(arraySize * sizeof(IdStr*));
    NS_ASSERTION(pPtrBase, "out of memory, can't sort");
    if (!pPtrBase) return NS_ERROR_OUT_OF_MEMORY;
    ptrs.AppendElement((void *)pPtrBase); // remember this pointer so we can free it later
    
    // build up the beast, so we can sort it.
    PRUint32 numSoFar = 0;
    // calc max possible size needed for all the rest
    PRUint32 maxSize = (PRUint32)(maxLen + sizeof(EntryInfo) + 1) * (PRUint32)(arraySize - numSoFar);

    PRUint32 maxBlockSize = (uint32) 0xf000L;
    PRUint32 allocSize = PR_MIN(maxBlockSize, maxSize);
    char *pTemp = (char *) PR_Malloc(allocSize);
    NS_ASSERTION(pTemp, "out of memory, can't sort");
    if (!pTemp) {   
        FreeAll(&ptrs);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    ptrs.AppendElement((void *)pTemp); // remember this pointer so we can free it later

    char * pBase = pTemp;
    PRBool more = PR_TRUE;

    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    PRUint32 longValue;
    PRTime timeValue;
    while (more && numSoFar < arraySize) {
      nsMsgKey thisKey = m_keys.GetAt(numSoFar);
      if (sortType != nsMsgViewSortType::byId) {
        rv = m_db->GetMsgHdrForKey(thisKey, getter_AddRefs(msgHdr));
        NS_ASSERTION(NS_SUCCEEDED(rv) && msgHdr, "header not found");
        if (NS_FAILED(rv) || !msgHdr) {
          FreeAll(&ptrs);
          return NS_ERROR_UNEXPECTED;
        }
      }
      else {
        msgHdr = nsnull;
      }

      // could be a problem here if the ones that appear here are different than the ones already in the array
      const char* pField = nsnull;
      nsXPIDLString intlString;
      PRUint32 paddedFieldLen = 0;
      PRUint32 actualFieldLen = 0;
      if (fieldType == kString) {
        rv = GetStringField(msgHdr, sortType, getter_Copies(intlString));
        NS_ENSURE_SUCCESS(rv,rv);
        pField = (const char *)((const PRUnichar *)intlString);
        //"Re:" might be encoded inside subject field using MIMEII encoding,
        //It should be stripped before sorting
        NS_MsgStripRE(&pField, 0);
        actualFieldLen = (pField) ? nsCRT::strlen(pField) + 1 : 1;
        paddedFieldLen = actualFieldLen;
        PRUint32 mod4 = actualFieldLen % 4;
        if (mod4 > 0) {
          paddedFieldLen += 4 - mod4;
        }
      }
      else if (fieldType == kPRTime) {
        rv = GetPRTimeField(msgHdr, sortType, &timeValue);
        NS_ENSURE_SUCCESS(rv,rv);

        pField = (const char *) &timeValue;
        actualFieldLen = paddedFieldLen = maxLen;
      }
      else {
        if (sortType == nsMsgViewSortType::byId) {
            longValue = thisKey;
        }
        else {
            rv = GetLongField(msgHdr, sortType, &longValue);
            NS_ENSURE_SUCCESS(rv,rv);
        }
        pField = (const char *) &longValue;
        actualFieldLen = paddedFieldLen = maxLen;
      }

      // check to see if this entry fits into the block we have allocated so far
      // pTemp - pBase = the space we have used so far
      // sizeof(EntryInfo) + fieldLen = space we need for this entry
      // allocSize = size of the current block
      if ((PRUint32)(pTemp - pBase) + (PRUint32)sizeof(EntryInfo) + (PRUint32)paddedFieldLen >= allocSize) {
        maxSize = (PRUint32)(maxLen + sizeof(EntryInfo) + 1) * (PRUint32)(arraySize - numSoFar);
        maxBlockSize = (PRUint32) 0xf000L;
        allocSize = PR_MIN(maxBlockSize, maxSize);
        pTemp = (char*)PR_Malloc(allocSize);
        NS_ASSERTION(pTemp, "out of memory, can't sort");
        if (!pTemp) {
          FreeAll(&ptrs);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        pBase = pTemp;
        ptrs.AppendElement((void *)pTemp); // remember this pointer so we can free it later
      }

      // make sure there aren't more IDs than we allocated space for
      NS_ASSERTION(numSoFar < arraySize, "out of memory");
      if (numSoFar >= arraySize) {
        FreeAll(&ptrs);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      // now store this entry away in the allocated memory
      pPtrBase[numSoFar] = (IdStr*)pTemp;
      EntryInfo *info = (EntryInfo *)pTemp;
      info->id = thisKey;
      PRUint32 bits= 0;
      bits = m_flags.GetAt(numSoFar);
      info->bits = bits;
      pTemp += sizeof(EntryInfo);
      PRInt32 bytesLeft = allocSize - (PRInt32)(pTemp - pBase);
      PRInt32 bytesToCopy = PR_MIN(bytesLeft, (PRInt32)actualFieldLen);
      if (pField && bytesToCopy > 0) {
        nsCRT::memcpy((char *)pTemp, pField, bytesToCopy);
        if (bytesToCopy < (PRInt32)actualFieldLen) {
          NS_ASSERTION(0, "wow, big block");
          *(pTemp + bytesToCopy - 1) = '\0';
        }
      }
      else {
        *pTemp = 0;
      }
      pTemp += paddedFieldLen;
      ++numSoFar;
    }

    // do the sort
    switch (fieldType) {
        case kString:
            NS_QuickSort(pPtrBase, numSoFar, sizeof(IdStr*), FnSortIdStr, nsnull);
            break;
        case kU32:
            NS_QuickSort(pPtrBase, numSoFar, sizeof(IdDWord*), FnSortIdDWord, nsnull);
            break;
        case kPRTime:
            NS_QuickSort(pPtrBase, numSoFar, sizeof(IdPRTime*), FnSortIdPRTime, nsnull);
            break;
        default:
            NS_ASSERTION(0, "not supposed to get here");
            break;
    }

    // now put the IDs into the array in proper order
    for (PRUint32 i = 0; i < numSoFar; i++) {
        m_keys.SetAt(i, pPtrBase[i]->info.id);
        m_flags.SetAt(i, pPtrBase[i]->info.bits);
    }

    m_sortType = sortType;
    m_sortOrder = sortOrder;

    if (sortOrder == nsMsgViewSortOrder::descending) {
        rv = ReverseSort();
        NS_ASSERTION(NS_SUCCEEDED(rv),"failed to reverse sort");
    }

    // free all the memory we allocated
    FreeAll(&ptrs);

    m_sortValid = PR_TRUE;
    //m_messageDB->SetSortInfo(sortType, sortOrder);

    return NS_OK;
}

void nsMsgDBView::FreeAll(nsVoidArray *ptrs)
{
    PRInt32 i;
    PRInt32 count = (PRInt32) ptrs->Count();
    if (count == 0) return;

    for (i=(count - 1);i>=0;i--) {
        void *ptr = (void *) ptrs->ElementAt(i);
        PR_FREEIF(ptr);
        ptrs->RemoveElementAt(i);
    }
}

nsMsgViewIndex nsMsgDBView::GetIndexOfFirstDisplayedKeyInThread(nsIMsgThread *threadHdr)
{
	nsMsgViewIndex	retIndex = nsMsgViewIndex_None;
	PRUint32	childIndex = 0;
	// We could speed up the unreadOnly view by starting our search with the first
	// unread message in the thread. Sometimes, that will be wrong, however, so
	// let's skip it until we're sure it's neccessary.
//	(m_viewFlags & kUnreadOnly) 
//		? threadHdr->GetFirstUnreadKey(m_messageDB) : threadHdr->GetChildAt(0);
  PRUint32 numThreadChildren;
  threadHdr->GetNumChildren(&numThreadChildren);
	while (retIndex == nsMsgViewIndex_None && childIndex < numThreadChildren)
	{
		nsMsgKey childKey;
    threadHdr->GetChildKeyAt(childIndex++, &childKey);
		retIndex = FindViewIndex(childKey);
	}
	return retIndex;
}


// Find the view index of the thread containing the passed msgKey, if
// the thread is in the view. MsgIndex is passed in as a shortcut if
// it turns out the msgKey is the first message in the thread,
// then we can avoid looking for the msgKey.
nsMsgViewIndex nsMsgDBView::ThreadIndexOfMsg(nsMsgKey msgKey, 
											  nsMsgViewIndex msgIndex /* = nsMsgViewIndex_None */,
											  PRInt32 *pThreadCount /* = NULL */,
											  PRUint32 *pFlags /* = NULL */)
{
	if (m_sortType != nsMsgViewSortType::byThread)
		return nsMsgViewIndex_None;
	nsCOMPtr <nsIMsgThread> threadHdr;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = m_db->GetMsgHdrForKey(msgKey, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);
  rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);

	nsMsgViewIndex retIndex = nsMsgViewIndex_None;

	if (threadHdr != nsnull)
	{
		if (msgIndex == nsMsgViewIndex_None)
			msgIndex = FindViewIndex(msgKey);

		if (msgIndex == nsMsgViewIndex_None)	// key is not in view, need to find by thread
		{
			msgIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
			nsMsgKey		threadKey = (msgIndex == nsMsgViewIndex_None) ? nsMsgKey_None : GetAt(msgIndex);
			if (pFlags)
				threadHdr->GetFlags(pFlags);
		}
		nsMsgViewIndex startOfThread = msgIndex;
		while ((PRInt32) startOfThread >= 0 && m_levels[startOfThread] != 0)
			startOfThread--;
		retIndex = startOfThread;
		if (pThreadCount)
		{
			PRInt32 numChildren = 0;
			nsMsgViewIndex threadIndex = startOfThread;
			do
			{
				threadIndex++;
				numChildren++;
			}
			while ((int32) threadIndex < m_levels.GetSize() && m_levels[threadIndex] != 0);
			*pThreadCount = numChildren;
		}
	}
	return retIndex;
}

nsMsgKey nsMsgDBView::GetKeyOfFirstMsgInThread(nsMsgKey key)
{
	nsCOMPtr <nsIMsgThread> pThread;
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, rv);
  m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
	nsMsgKey	firstKeyInThread = nsMsgKey_None;

	if (pThread == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "error getting msg from thread");
		return firstKeyInThread;
	}
	// ### dmb UnreadOnly - this is wrong.
	pThread->GetChildKeyAt(0, &firstKeyInThread);
	return firstKeyInThread;
}

nsMsgKey nsMsgDBView::GetAt(nsMsgViewIndex index) 
{
	if (index >= m_keys.GetSize() || index == nsMsgViewIndex_None)
		return nsMsgKey_None;
	else
		return(m_keys.GetAt(index));
}

nsMsgViewIndex	nsMsgDBView::FindKey(nsMsgKey key, PRBool expand)
{
	nsMsgViewIndex retIndex = nsMsgViewIndex_None;
	retIndex = (nsMsgViewIndex) (m_keys.FindIndex(key));
	if (key != nsMsgKey_None && retIndex == nsMsgViewIndex_None && expand && m_db)
	{
		nsMsgKey threadKey = GetKeyOfFirstMsgInThread(key);
		if (threadKey != nsMsgKey_None)
		{
			nsMsgViewIndex threadIndex = FindKey(threadKey, PR_FALSE);
			if (threadIndex != nsMsgViewIndex_None)
			{
				PRUint32 flags = m_flags[threadIndex];
				if ((flags & MSG_FLAG_ELIDED) && NS_SUCCEEDED(ExpandByIndex(threadIndex, nsnull)))
					retIndex = FindKey(key, PR_FALSE);
			}
		}
	}
	return retIndex;
}

nsresult		nsMsgDBView::GetThreadCount(nsMsgKey messageKey, PRUint32 *pThreadCount)
{
	nsresult rv = NS_MSG_MESSAGE_NOT_FOUND;
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(messageKey, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr <nsIMsgThread> pThread;
  rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
	if (NS_SUCCEEDED(rv) && pThread != nsnull)
    rv = pThread->GetNumChildren(pThreadCount);
	return rv;
}


// This counts the number of messages in an expanded thread, given the
// index of the first message in the thread.
PRInt32 nsMsgDBView::CountExpandedThread(nsMsgViewIndex index)
{
	PRInt32 numInThread = 0;
	nsMsgViewIndex startOfThread = index;
	while ((PRInt32) startOfThread >= 0 && m_levels[startOfThread] != 0)
		startOfThread--;
	nsMsgViewIndex threadIndex = startOfThread;
	do
	{
		threadIndex++;
		numInThread++;
	}
	while ((PRInt32) threadIndex < m_levels.GetSize() && m_levels[threadIndex] != 0);

	return numInThread;
}

// returns the number of lines that would be added (> 0) or removed (< 0) 
// if we were to try to expand/collapse the passed index.
nsresult nsMsgDBView::ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta)
{
	PRUint32 numChildren;
	nsresult	rv;

	*expansionDelta = 0;
	if ((int) index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;
	char	flags = m_flags[index];

	if (m_sortType != nsMsgViewSortType::byThread)
		return NS_OK;

	// The client can pass in the key of any message
	// in a thread and get the expansion delta for the thread.

	if (!(m_viewFlags & kUnreadOnly))
	{
		rv = GetThreadCount(m_keys[index], &numChildren);
		NS_ENSURE_SUCCESS(rv, rv);
	}
	else
	{
		numChildren = CountExpandedThread(index);
	}

	if (flags & MSG_FLAG_ELIDED)
		*expansionDelta = numChildren - 1;
	else
		*expansionDelta = - (PRInt32) (numChildren - 1);

	return NS_OK;
}

nsresult nsMsgDBView::ToggleExpansion(nsMsgViewIndex index, PRUint32 *numChanged)
{
  NS_ENSURE_ARG(numChanged);
  *numChanged = 0;
	nsMsgViewIndex threadIndex = ThreadIndexOfMsg(GetAt(index), index);
	if (threadIndex == nsMsgViewIndex_None)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find thread");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
	PRInt32	flags = m_flags[threadIndex];

	// if not a thread, or doesn't have children, no expand/collapse
	// If we add sub-thread expand collapse, this will need to be relaxed
	if (!(flags & MSG_VIEW_FLAG_ISTHREAD) || !(flags && MSG_VIEW_FLAG_HASCHILDREN))
		return NS_MSG_MESSAGE_NOT_FOUND;
	if (flags & MSG_FLAG_ELIDED)
		return ExpandByIndex(threadIndex, numChanged);
	else
		return CollapseByIndex(threadIndex, numChanged);

}

nsresult nsMsgDBView::ExpandAll()
{
	for (PRInt32 i = GetSize() - 1; i >= 0; i--) 
	{
		PRUint32 numExpanded;
		PRUint32 flags = m_flags[i];
		if (flags & MSG_FLAG_ELIDED)
			ExpandByIndex(i, &numExpanded);
	}
	return NS_OK;
}

nsresult nsMsgDBView::ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded)
{
	char			flags = m_flags[index];
	nsMsgKey		firstIdInThread, startMsg = nsMsgKey_None;
	nsresult		rv = NS_OK;
	PRUint32			numExpanded = 0;

	NS_ASSERTION(flags & MSG_FLAG_ELIDED, "can't expand an already expanded thread");
	flags &= ~MSG_FLAG_ELIDED;

	if ((PRUint32) index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsCOMPtr <nsIMsgThread> pThread;
    m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find message to expand");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
  rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
	m_flags[index] = flags;
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	if (m_viewFlags & kUnreadOnly)
	{
//		if (flags & MSG_FLAG_READ)
//			unreadLevelArray.Add(0);	// keep top level hdr in thread, even though read.
    nsMsgKey threadId;
    msgHdr->GetThreadId(&threadId);
#ifdef HAVE_PORT
		rv = m_db->ListUnreadIdsInThread(pThread,  &startMsg, unreadLevelArray, 
										listChunk, listIDs, listFlags, listLevels, &numExpanded);
#endif
	}
	else
		rv = ListIdsInThread(pThread,  index, &numExpanded);

	NoteStartChange(index, numExpanded, nsMsgViewNotificationCode::insertOrDelete);

	NoteEndChange(index, numExpanded, nsMsgViewNotificationCode::insertOrDelete);
	if (pNumExpanded != nsnull)
		*pNumExpanded = numExpanded;
	return rv;
}

nsresult nsMsgDBView::CollapseByIndex(nsMsgViewIndex index, PRUint32 *pNumCollapsed)
{
	nsMsgKey		firstIdInThread;
	nsresult	rv;
	PRInt32	flags = m_flags[index];
	PRInt32	threadCount = 0;

	if (flags & MSG_FLAG_ELIDED || m_sortType != nsMsgViewSortType::byThread)
		return NS_OK;
	flags  |= MSG_FLAG_ELIDED;

	if (index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (!NS_SUCCEEDED(rv) || msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "error collapsing thread");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}

	m_flags[index] = flags;
	NoteChange(index, 1, nsMsgViewNotificationCode::changed);

	rv = ExpansionDelta(index, &threadCount);
	if (NS_SUCCEEDED(rv))
	{
		PRInt32 numRemoved = threadCount; // don't count first header in thread
    NoteStartChange(index + 1, -numRemoved, nsMsgViewNotificationCode::insertOrDelete);
		// start at first id after thread.
		for (int i = 1; i <= threadCount && index + 1 < m_keys.GetSize(); i++)
		{
			m_keys.RemoveAt(index + 1);
			m_flags.RemoveAt(index + 1);
			m_levels.RemoveAt(index + 1);
		}
		if (pNumCollapsed != nsnull)
			*pNumCollapsed = numRemoved;	
		NoteEndChange(index + 1, -numRemoved, nsMsgViewNotificationCode::insertOrDelete);
	}
	return rv;
}

PRBool nsMsgDBView::WantsThisThread(nsIMsgThread * /*threadHdr*/)
{
  return PR_TRUE; // default is to want all threads.
}

PRInt32 nsMsgDBView::FindLevelInThread(nsIMsgDBHdr *msgHdr, nsMsgKey msgKey, nsMsgViewIndex startOfThreadViewIndex)
{
  nsMsgKey threadParent;
  msgHdr->GetThreadParent(&threadParent);
  nsMsgViewIndex parentIndex = m_keys.FindIndex(threadParent, startOfThreadViewIndex);
  if (parentIndex != nsMsgViewIndex_None)
    return m_levels[parentIndex] + 1;
  else
  {
    NS_ASSERTION(PR_FALSE, "couldn't find parent of msg");
    return 1; // well, return level 1.
  }
}

nsresult	nsMsgDBView::ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  NS_ENSURE_ARG(threadHdr);
	// these children ids should be in thread order.
	PRUint32 i;
  nsMsgViewIndex viewIndex = startOfThreadViewIndex + 1;
	*pNumListed = 0;

  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
	for (i = 1; i < numChildren; i++)
	{
		nsCOMPtr <nsIMsgDBHdr> msgHdr;
    threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));
		if (msgHdr != nsnull)
		{
      nsMsgKey msgKey;
      PRUint32 msgFlags, newFlags;
      msgHdr->GetMessageKey(&msgKey);
      msgHdr->GetFlags(&msgFlags);
			PRBool isRead = PR_FALSE;
			m_db->IsRead(msgKey, &isRead);
			// just make sure flag is right in db.
			m_db->MarkHdrRead(msgHdr, isRead, nsnull);
//				if (isRead)
//					msgHdr->m_flags |= kIsRead;
//				else
//					msgHdr->m_flags &= ~kIsRead;
			m_keys.InsertAt(viewIndex, msgKey);
      // ### TODO - how about hasChildren flag?
			m_flags.InsertAt(viewIndex, msgFlags & ~MSG_VIEW_FLAGS);
      // ### TODO this is going to be tricky - might use enumerators
      PRInt32 level = FindLevelInThread(msgHdr, msgKey, startOfThreadViewIndex);
			m_levels.InsertAt(viewIndex, level); 
			// turn off thread or elided bit if they got turned on (maybe from new only view?)
			if (i > 0)	
				msgHdr->AndFlags(~(MSG_VIEW_FLAG_ISTHREAD | MSG_FLAG_ELIDED), &newFlags);
			(*pNumListed)++;
      viewIndex++;
		}
	}
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnKeyChange(nsMsgKey aKeyChanged, PRUint32 aOldFlags, 
                                       PRUint32, nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}
NS_IMETHODIMP nsMsgDBView::OnKeyDeleted(nsMsgKey aKeyChanged, nsMsgKey aParentKey, PRInt32 aFlags, 
                            nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnKeyAdded(nsMsgKey aKeyChanged, nsMsgKey aParentKey, PRInt32 aFlags, 
                          nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}
                          
NS_IMETHODIMP nsMsgDBView::OnParentChanged (nsMsgKey aKeyChanged, nsMsgKey oldParent, nsMsgKey newParent, nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnAnnouncerGoingAway(nsIDBChangeAnnouncer *instigator)
{
  return NS_OK;
}

    
NS_IMETHODIMP nsMsgDBView::OnReadChanged(nsIDBChangeListener *instigator)
{
  return NS_OK;
}


void	nsMsgDBView::EnableChangeUpdates()
{
}
void	nsMsgDBView::DisableChangeUpdates()
{
}
void	nsMsgDBView::NoteChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}

void	nsMsgDBView::NoteStartChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}
void	nsMsgDBView::NoteEndChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}
