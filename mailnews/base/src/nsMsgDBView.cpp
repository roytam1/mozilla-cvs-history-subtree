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

/* Implementation file */

NS_IMPL_ISUPPORTS1(nsMsgDBView, nsIMsgDBView)

nsMsgDBView::nsMsgDBView()
{
  NS_INIT_ISUPPORTS();
  m_viewType = nsMsgDBViewType::anyView;
  /* member initializers and constructor code */
  m_sortValid = PR_TRUE;
  m_sortOrder = nsMsgViewSortOrder::none;
  //m_viewFlags = (ViewFlags) 0;
}

nsMsgDBView::~nsMsgDBView()
{
  /* destructor code */
}

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgDatabase *msgDB, nsMsgViewSortTypeValue viewType, PRInt32 *pCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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

nsresult nsMsgDBView::ReverseThreads()
{
    printf("XXX same sort type, just different sort order.  just reverse threads\n");
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

#include "nsIDBFolderInfo.h"
#include "nsIMsgDatabase.h"
#include "nsIRDFService.h"
#include "nsIMsgFolder.h"
#include "nsMsgRDFUtils.h"

NS_IMETHODIMP nsMsgDBView::PopulateView()
{
  nsresult rv;
  nsCOMPtr<nsIRDFService> rdf = do_GetService(NS_RDF_CONTRACTID "/rdf-service;1",&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIRDFResource> resource;
  rv = rdf->GetResource("mailbox://nobody@Local%20Folders/Trash", getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIMsgFolder> folder;
  folder = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIDBFolderInfo> folderInfo;
  rv = folder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_db));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIMsgDBHdr> msgHdr;
  nsMsgKeyArray   keys;
  PRUint32 flags;
  m_db->ListAllKeys(keys);
  PRUint32 size = keys.GetSize();
  for (PRUint32 i=0;i<size;i++) {
    m_keys.InsertAt(i,keys[i]);
    rv = m_db->GetMsgHdrForKey(keys[i], getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = msgHdr->GetFlags(&flags);
    NS_ENSURE_SUCCESS(rv,rv);
    m_flags.InsertAt(i,flags);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::DumpView()
{
    PRUint32 i;
    PRUint32 num = GetSize();
    printf("#:  (key,flag)\n");
    for (i = 0; i < num; i++) {
        printf("%d:  (%d,%d)\n",i,m_keys.GetAt(i),m_flags.GetAt(i));
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
            *pFieldType = kU64;
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

nsresult nsMsgDBView::GetLongField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);
  *result = 1;
  return NS_OK;
}


nsresult nsMsgDBView::GetStringField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, char **result)
{
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);
#if 0
  const char *pField;

  switch (sortType) {
    case nsMsgViewSortType::bySubject:
        if (msgHdr->GetSubject(string))
            pField = string;
        else
            pField = "";
        break;
    case SortByRecipient:
        msgHdr->GetNameOfRecipient(string, 0, m_messageDB->GetDB());
        pField = string;
        break;
    case SortByAuthor:
        msgHdr->GetRFC822Author(string);
        pField = string;
        break;
    default:
//      XP_ASSERT(FALSE);
        return(0);
    }
    return INTL_DecodeMimePartIIAndCreateCollationKey(pField, csid, 0);
#endif
    *result = nsnull;
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

    PRUint32 arraySize = GetSize();
    IdStr** pPtrBase = (IdStr**)PR_Malloc(arraySize * sizeof(IdStr*));
    NS_ASSERTION(pPtrBase, "out of memory, can't sort");
    if (!pPtrBase) return NS_ERROR_OUT_OF_MEMORY;
    
    // build up the beast, so we can sort it.
    PRUint32 numSoFar = 0;
    // calc max possible size needed for all the rest
    PRUint32 maxSize = (PRUint32)(maxLen + sizeof(EntryInfo) + 1) * (PRUint32)(arraySize - numSoFar);

    PRUint32 maxBlockSize = (uint32) 0xf000L;
    PRUint32 allocSize = PR_MIN(maxBlockSize, maxSize);
    char *pTemp = (char *) PR_Malloc(allocSize);
    NS_ASSERTION(pTemp, "out of memory, can't sort");
    if (!pTemp) {   
        PR_FREEIF(pPtrBase);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    char * pBase = pTemp;
    PRBool more = PR_TRUE;

    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    PRUint32 longValue;
    while (more && numSoFar < arraySize) {
      nsMsgKey thisKey = m_keys.GetAt(numSoFar);
      if (sortType != nsMsgViewSortType::byId) {
        rv = m_db->GetMsgHdrForKey(thisKey, getter_AddRefs(msgHdr));
        NS_ASSERTION(NS_SUCCEEDED(rv) && msgHdr, "header not found");
        if (NS_FAILED(rv) || !msgHdr) {
          PR_FREEIF(pPtrBase);
          PR_FREEIF(pTemp);
          return NS_ERROR_UNEXPECTED;
        }
      }
      else {
        msgHdr = nsnull;
      }

      // could be a problem here if the ones that appear here are different than the ones already in the array
      const char* pField = nsnull;
      char *intlString = nsnull;
      PRUint32 paddedFieldLen = 0;
      PRUint32 actualFieldLen = 0;
      if (fieldType == kString) {
        rv = GetStringField(msgHdr, sortType, &intlString);
        NS_ENSURE_SUCCESS(rv,rv);
        pField = intlString;
        //"Re:" might be encoded inside subject field using MIMEII encoding,
        //It should be stripped before sorting
        printf("msg_StripRE(&pField, 0);\n");
        actualFieldLen = (pField) ? nsCRT::strlen(pField) + 1 : 1;
        paddedFieldLen = actualFieldLen;
        PRUint32 mod4 = actualFieldLen % 4;
        if (mod4 > 0) {
          paddedFieldLen += 4 - mod4;
        }
      }
      else if (fieldType == kU64) {
        printf("not implemented yet\n");
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
          PR_FREEIF(pPtrBase);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        pBase = pTemp;
      }

      // make sure there aren't more IDs than we allocated space for
      NS_ASSERTION(numSoFar >= arraySize, "out of memory");
      if (numSoFar >= arraySize) {
        PR_FREEIF(pPtrBase);
        PR_FREEIF(pTemp);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      // now store this entry away in the allocated memory
      pPtrBase[numSoFar] = (IdStr*)pTemp;
      EntryInfo *info = (EntryInfo *)  pTemp;
      info->id = thisKey;
      PRUint32 bits= 0;
      bits = m_flags.GetAt(numSoFar);
      info->bits = bits;
      pTemp += sizeof(EntryInfo);
      PRInt32 bytesLeft = allocSize - (PRInt32)(pTemp - pBase);
      PRInt32 bytesToCopy = PR_MIN(bytesLeft, (PRInt32)actualFieldLen);
      if (pField && bytesToCopy > 0) {
        memcpy((char *)pTemp, pField, bytesToCopy);
        if (bytesToCopy < (PRInt32)actualFieldLen) {
          NS_ASSERTION(0, "wow, big block");
          *(pTemp + bytesToCopy - 1) = '\0';
        }
        PR_FREEIF(intlString); // free intl'ized string
      }
      else {
        *pTemp = 0;
      }
      pTemp += paddedFieldLen;
      ++numSoFar;
    }

    // do the sort
    // use new array to shuffle m_keys

    m_sortType = sortType;
    m_sortOrder = sortOrder;

    if (sortOrder == nsMsgViewSortOrder::descending) {
        rv = ReverseSort();
        NS_ASSERTION(NS_SUCCEEDED(rv),"failed to reverse sort");
    }

    // free new array

    return NS_OK;
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
	int				numListed = 0;
	char			flags = m_flags[index];
	nsMsgKey		firstIdInThread, startMsg = nsMsgKey_None;
	nsresult		rv = NS_OK;
	nsMsgViewIndex	firstInsertIndex = index + 1;
	nsMsgViewIndex	insertIndex = firstInsertIndex;
	uint32			numExpanded = 0;
	nsMsgKeyArray			tempIDArray;
	nsUInt32Array		tempFlagArray;
	nsUint8Array		tempLevelArray;
	nsUint8Array		unreadLevelArray;

	NS_ASSERTION(flags & MSG_FLAG_ELIDED, "can't expand an already expanded thread");
	flags &= ~MSG_FLAG_ELIDED;

	if ((PRUint32) index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
    m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find message to expand");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
	m_flags[index] = flags;
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	do
	{
		const int listChunk = 200;
		nsMsgKey	listIDs[listChunk];
		char		listFlags[listChunk];
		char		listLevels[listChunk];
#ifdef HAVE_PORT
		if (m_viewFlags & kUnreadOnly)
		{
			if (flags & MSG_FLAG_READ)
				unreadLevelArray.Add(0);	// keep top level hdr in thread, even though read.
      nsMsgKey threadId;
      msgHdr->GetThreadId(&threadId);
			rv = m_db->ListUnreadIdsInThread(threadId,  &startMsg, unreadLevelArray, 
											listChunk, listIDs, listFlags, listLevels, &numListed);
		}
		else
			rv = m_db->ListIdsInThread(msgHdr,  &startMsg, listChunk, 
											listIDs, listFlags, listLevels, &numListed);
#endif
		// Don't add thread to view, it's already in.
		for (int i = 0; i < numListed; i++)
		{
			if (listIDs[i] != firstIdInThread)
			{
				tempIDArray.Add(listIDs[i]);
				tempFlagArray.Add(listFlags[i]);
				tempLevelArray.Add(listLevels[i]);
				insertIndex++;
			}
		}
		if (numListed < listChunk || startMsg == nsMsgKey_None)
			break;
	}
	while (NS_SUCCEEDED(rv));
	numExpanded = (insertIndex - firstInsertIndex);

	NoteStartChange(firstInsertIndex, numExpanded, nsMsgViewNotificationCode::insertOrDelete);

	m_keys.InsertAt(firstInsertIndex, &tempIDArray);
	m_flags.InsertAt(firstInsertIndex, &tempFlagArray);
  m_levels.InsertAt(firstInsertIndex, &tempLevelArray);
	NoteEndChange(firstInsertIndex, numExpanded, nsMsgViewNotificationCode::insertOrDelete);
	if (pNumExpanded != nsnull)
		*pNumExpanded = numExpanded;
	return rv;
}

PRBool nsMsgDBView::WantsThisThread(nsIMsgThread * /*threadHdr*/)
{
  return PR_TRUE; // default is to want all threads.
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
