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

NS_IMPL_ADDREF(nsMsgDBView)
NS_IMPL_RELEASE(nsMsgDBView)

NS_INTERFACE_MAP_BEGIN(nsMsgDBView)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIDBChangeListener)
   NS_INTERFACE_MAP_ENTRY(nsIOutlinerView)
NS_INTERFACE_MAP_END

nsMsgDBView::nsMsgDBView()
{
  NS_INIT_ISUPPORTS();
  m_viewType = nsMsgDBViewType::anyView;
  /* member initializers and constructor code */
  m_sortValid = PR_TRUE;
  m_sortOrder = nsMsgViewSortOrder::none;
  m_viewFlags = nsMsgViewFlagsType::kNone;
  m_cachedMsgKey = nsMsgKey_None;
}

nsMsgDBView::~nsMsgDBView()
{
  if (m_db)
	  m_db->RemoveListener(this);
}

///////////////////////////////////////////////////////////////////////////
// nsIOutlinerView Implementation Methods
///////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMsgDBView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = GetSize();
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSelection(nsIOutlinerSelection * *aSelection)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSelection(nsIOutlinerSelection * aSelection)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetCellProperties(PRInt32 row, const PRUnichar *colID, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainer(PRInt32 index, PRBool *_retval)
{
  PRUint32 flags = m_flags[index];
  *_retval = (flags & MSG_VIEW_FLAG_HASCHILDREN) != 0;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  PRUint32 flags = m_flags[index];
  *_retval = (flags & MSG_FLAG_ELIDED) == 0;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  *_retval = m_levels[index];
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetCellText(PRInt32 aRow, const PRUnichar * aColID, PRUnichar ** aValue)
{
  nsresult rv = NS_OK;
  nsMsgKey key = m_keys.GetAt(aRow);
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  if (key == m_cachedMsgKey)
    msgHdr = m_cachedHdr;
  else
  {
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv))
    {
      m_cachedHdr = msgHdr;
      m_cachedMsgKey = key;
    }
    else
      return rv;
  }

  // just a hack
  nsXPIDLCString dbString;

  switch (aColID[0])
  {
  case 's':
    if (aColID[1] == 'u') // subject
      rv = msgHdr->GetMime2DecodedSubject(aValue);
    else
      rv = msgHdr->GetMime2DecodedAuthor(aValue);
    break;
  case 'd':  // date
    break;
  default:
    break;
  }

  // mscott: to do, we need to use a msgHdrParser to break down the author field and extract just the "pretty name"
  // but this requires a bunch of string conversions I'd like to avoid on every paint! 
#if 0
  if(mHeaderParser)
	{

		nsXPIDLCString name;

    rv = mHeaderParser->ExtractHeaderAddressName("UTF-8", NS_ConvertUCS2toUTF8(sender), getter_Copies(name));
    if (NS_SUCCEEDED(rv) && (const char*)name)
      senderUserName.Assign(NS_ConvertUTF8toUCS2(name));
	}
#endif
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetOutliner(nsIOutlinerBoxObject *outliner)
{
  mOutliner = outliner;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::ToggleOpenState(PRInt32 index)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::CycleHeader(nsIDOMElement *elt)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::CycleCell(PRInt32 row, const PRUnichar *colID)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, const PRUnichar *colID)
{
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////
// end nsIOutlinerView Implementation Methods
///////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount)
{
  NS_ENSURE_ARG(folder);
  nsCOMPtr <nsIDBFolderInfo> folderInfo;

  m_viewFlags = viewFlags;
  m_sortOrder = sortOrder;
  m_sortType = sortType;

  nsresult rv = folder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_db));
  NS_ENSURE_SUCCESS(rv,rv);
	m_db->AddListener(this);
#ifdef HAVE_PORT
	CacheAdd ();
#endif

	return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Close()
{
  if (m_db)
  {
  	m_db->RemoveListener(this);
    m_db = nsnull;
  }
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Init(PRInt32 *pCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void doCommand (in nsMsgViewCommandTypeValue command, out nsMsgViewIndex indices, in long numindices); */
NS_IMETHODIMP nsMsgDBView::DoCommand(nsMsgViewCommandTypeValue command, nsMsgViewIndex *indices, PRInt32 numindices)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getCommandStatus (in nsMsgViewCommandTypeValue command, out nsMsgViewIndex indices, in long numindices, out boolean selectable_p, out nsMsgViewCommandCheckStateValue selected_p); */
NS_IMETHODIMP nsMsgDBView::GetCommandStatus(nsMsgViewCommandTypeValue command, nsMsgViewIndex *indices, PRInt32 numindices, PRBool *selectable_p, nsMsgViewCommandCheckStateValue *selected_p)
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
        if (m_viewFlags == nsMsgViewFlagsType::kOutlineDisplay) {
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
            // we just reversed the sort order...we still need to invalidate the view
            mOutliner->Invalidate();
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

    PRUint32 maxBlockSize = (PRUint32) 0xf000L;
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
    //m_db->SetSortInfo(sortType, sortOrder);

    // last but not least, invalidate the entire view
    if (mOutliner)
      mOutliner->Invalidate();

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
//	(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) 
//		? threadHdr->GetFirstUnreadKey(m_db) : threadHdr->GetChildAt(0);
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
			//nsMsgKey		threadKey = (msgIndex == nsMsgViewIndex_None) ? nsMsgKey_None : GetAt(msgIndex);
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
    rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
    NS_ENSURE_SUCCESS(rv, rv);
	nsMsgKey	firstKeyInThread = nsMsgKey_None;

	NS_ASSERTION(pThread, "error getting msg from thread");
	if (!pThread)
	{
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

nsresult nsMsgDBView::GetThreadCount(nsMsgKey messageKey, PRUint32 *pThreadCount)
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
	if ( index > ((nsMsgViewIndex) m_keys.GetSize()))
		return NS_MSG_MESSAGE_NOT_FOUND;
	char	flags = m_flags[index];

	if (m_sortType != nsMsgViewSortType::byThread)
		return NS_OK;

	// The client can pass in the key of any message
	// in a thread and get the expansion delta for the thread.

	if (!(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly))
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
	nsMsgKey		firstIdInThread;
#ifdef HAVE_PORT
    nsMsgKey        startMsg = nsMsgKey_None;
#endif
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
	if (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly)
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
  m_db = nsnull;
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

NS_IMETHODIMP nsMsgDBView::GetViewFlags(nsMsgViewFlagsTypeValue *aViewFlags)
{
    NS_ENSURE_ARG_POINTER(aViewFlags);
    *aViewFlags = m_viewFlags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetViewFlags(nsMsgViewFlagsTypeValue aViewFlags)
{
    m_viewFlags = aViewFlags;
    return NS_OK;
}

nsresult nsMsgDBView::MarkThreadOfMsgRead(nsMsgKey msgId, nsMsgViewIndex msgIndex, nsMsgKeyArray &idsMarkedRead, PRBool bRead)
{
    nsCOMPtr <nsIMsgThread> threadHdr;
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    nsresult rv;

    rv = m_db->GetMsgHdrForKey(msgId, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMsgViewIndex threadIndex;

    NS_ASSERTION(threadHdr, "threadHdr is null");
    if (!threadHdr) {
        return NS_MSG_MESSAGE_NOT_FOUND;
    }
    nsCOMPtr <nsIMsgDBHdr> firstHdr;
    threadHdr->GetChildAt(0, getter_AddRefs(firstHdr));
    nsMsgKey firstHdrId;
    firstHdr->GetMessageKey(&firstHdrId);
    if (msgId != firstHdrId)
        threadIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
    else
        threadIndex = msgIndex;
    rv = MarkThreadRead(threadHdr, threadIndex, idsMarkedRead, bRead);
    return rv;
}

nsresult nsMsgDBView::MarkThreadRead(nsIMsgThread *threadHdr, nsMsgViewIndex threadIndex, nsMsgKeyArray &idsMarkedRead, PRBool bRead)
{
    PRBool threadElided = PR_TRUE;
    if (threadIndex != nsMsgViewIndex_None)
        threadElided = (m_flags.GetAt(threadIndex) & MSG_FLAG_ELIDED);

    PRUint32 numChildren;
    threadHdr->GetNumChildren(&numChildren);
    for (PRInt32 childIndex = 0; childIndex < (PRInt32) numChildren ; childIndex++) {
        nsCOMPtr <nsIMsgDBHdr> msgHdr;
        threadHdr->GetChildHdrAt(childIndex, getter_AddRefs(msgHdr));
        NS_ASSERTION(msgHdr, "msgHdr is null");
        if (!msgHdr) {
            continue;
        }

        PRBool isRead;

        nsMsgKey hdrMsgId;
        msgHdr->GetMessageKey(&hdrMsgId);
        m_db->IsRead(hdrMsgId, &isRead);

        if (isRead != bRead) {
            m_db->MarkHdrRead(msgHdr, bRead, nsnull);
            // insert at the front.  should we insert at the end?
            idsMarkedRead.InsertAt(0, hdrMsgId);
        }
    }

    if (bRead) {
        printf("fix this\n");
        //threadHdr->SetNumNewChildren(0);
    }
    else {
        PRUint32 numChildren;
        threadHdr->GetNumChildren(&numChildren);
        printf("fix this\n");
        //threadHdr->SetNumNewChildren(numChildren);
    }
    return NS_OK;
}

// Starting from startIndex, performs the passed in navigation, including
// any marking read needed, and returns the resultId and resultIndex of the
// destination of the navigation. If there are no more unread in the view,
// it returns a resultId of nsMsgKey_None and an resultIndex of nsMsgViewIndex_None.
NS_IMETHODIMP nsMsgDBView::ViewNavigate(nsMsgNavigationTypeValue motion, nsMsgViewIndex startIndex, nsMsgKey *selection, PRUint32 numSelected, nsMsgKey *pResultKey, nsMsgViewIndex *pResultIndex, nsMsgViewIndex *pThreadIndex, PRBool wrap, nsIMsgFolder **resultFolderInfo)
{
    NS_ENSURE_ARG_POINTER(selection);
    NS_ENSURE_ARG_POINTER(pResultKey);
    NS_ENSURE_ARG_POINTER(pResultIndex);
    NS_ENSURE_ARG_POINTER(pThreadIndex);
    NS_ENSURE_ARG_POINTER(resultFolderInfo);

    nsresult rv = NS_OK;
    nsMsgKey resultThreadKey;
    nsMsgViewIndex curIndex;
    nsMsgViewIndex lastIndex = (GetSize() > 0) ? (nsMsgViewIndex) GetSize() - 1 : nsMsgViewIndex_None;
    nsMsgViewIndex threadIndex = nsMsgViewIndex_None;

    switch (motion) {
        case nsMsgNavigationType::firstMessage:
            if (GetSize() > 0) {
                *pResultIndex = 0;
                *pResultKey = m_keys.GetAt(0);
            }
            else {
                *pResultIndex = nsMsgViewIndex_None;
                *pResultKey = nsMsgKey_None;
            }
            break;
        case nsMsgNavigationType::nextMessage:
            // return same index and id on next on last message
            *pResultIndex = PR_MIN(startIndex + 1, lastIndex);
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::previousMessage:
            *pResultIndex = (startIndex > 0) ? startIndex - 1 : 0;
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::lastMessage:
            *pResultIndex = lastIndex;
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstFlagged:
            rv = FindFirstFlagged(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::nextFlagged:
            rv = FindNextFlagged(startIndex + 1, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::previousFlagged:
            rv = FindPrevFlagged(startIndex, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstNew:
            rv = FindFirstNew(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstUnreadMessage:
            startIndex = nsMsgViewIndex_None;        // note fall thru - is this motion ever used?
        case nsMsgNavigationType::nextUnreadMessage:
            for (curIndex = (startIndex == nsMsgViewIndex_None) ? 0 : startIndex; curIndex <= lastIndex && lastIndex != nsMsgViewIndex_None; curIndex++) {
                PRUint32 flags = m_flags.GetAt(curIndex);

                // don't return start index since navigate should move
                if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
                    *pResultIndex = curIndex;
                    *pResultKey = m_keys.GetAt(*pResultIndex);
                    break;
                }
                // check for collapsed thread with new children
                if (m_sortType == nsMsgViewSortType::byThread && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
                    nsCOMPtr <nsIMsgThread> threadHdr;
                    nsCOMPtr <nsIMsgDBHdr> msgHdr;
                    rv = m_db->GetMsgHdrForKey(m_keys.GetAt(curIndex), getter_AddRefs(msgHdr));
                    NS_ENSURE_SUCCESS(rv, rv);
                    rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
                    NS_ENSURE_SUCCESS(rv, rv);

                    NS_ASSERTION(threadHdr, "threadHdr is null");
                    if (!threadHdr) {
                        continue;
                    }
                    PRUint32 numUnreadChildren;
                    threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                    if (numUnreadChildren > 0) {
                        PRUint32 numExpanded;
                        ExpandByIndex(curIndex, &numExpanded);
                        lastIndex += numExpanded;
                        if (pThreadIndex)
                            *pThreadIndex = curIndex;
                    }
                }
            }
            if (curIndex > lastIndex) {
                // wrap around by starting at index 0.
                if (wrap) {
                    nsMsgKey startKey = GetAt(startIndex);

                    rv = ViewNavigate(nsMsgNavigationType::nextUnreadMessage, nsMsgViewIndex_None, selection, numSelected, pResultKey, pResultIndex, pThreadIndex, PR_FALSE, resultFolderInfo);
                    if (*pResultKey == startKey) {   
                        // wrapped around and found start message!
                        *pResultIndex = nsMsgViewIndex_None;
                        *pResultKey = nsMsgKey_None;
                    }
                }
                else {
                    *pResultIndex = nsMsgViewIndex_None;
                    *pResultKey = nsMsgKey_None;
                }
            }
            break;
        case nsMsgNavigationType::previousUnreadMessage:
            rv = FindPrevUnread(m_keys.GetAt(startIndex), pResultKey,
                                &resultThreadKey);
            if (NS_SUCCEEDED(rv)) {
                *pResultIndex = FindViewIndex(*pResultKey);
                if (*pResultKey != resultThreadKey && m_sortType == nsMsgViewSortType::byThread) {
                    threadIndex  = ThreadIndexOfMsg(*pResultKey, nsMsgViewIndex_None);
                    if (*pResultIndex == nsMsgViewIndex_None) {
                        nsCOMPtr <nsIMsgThread> threadHdr;
                        nsCOMPtr <nsIMsgDBHdr> msgHdr;
                        rv = m_db->GetMsgHdrForKey(*pResultKey, getter_AddRefs(msgHdr));
                        NS_ENSURE_SUCCESS(rv, rv);
                        rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
                        NS_ENSURE_SUCCESS(rv, rv);

                        NS_ASSERTION(threadHdr, "threadHdr is null");
                        if (threadHdr) {
                            break;
                        }
                        PRUint32 numUnreadChildren;
                        threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                        if (numUnreadChildren > 0) {
                            PRUint32 numExpanded;
                            ExpandByIndex(threadIndex, &numExpanded);
                        }
                        *pResultIndex = FindViewIndex(*pResultKey);
                    }
                }
                if (pThreadIndex)
                    *pThreadIndex = threadIndex;
            }
            break;
        case nsMsgNavigationType::lastUnreadMessage:
            break;
        case nsMsgNavigationType::nextUnreadThread:
            {
                nsMsgKeyArray idsMarkedRead;

                if (startIndex == nsMsgViewIndex_None) {
                    NS_ASSERTION(0,"startIndex == nsMsgViewIndex_None");
                    break;
                }
                rv = MarkThreadOfMsgRead(m_keys.GetAt(startIndex), startIndex, idsMarkedRead, PR_TRUE);
                if (NS_SUCCEEDED(rv)) 
                    return ViewNavigate(nsMsgNavigationType::nextUnreadMessage, startIndex, selection, numSelected, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
                break;
            }
        case nsMsgNavigationType::toggleThreadKilled:
            {
                PRBool resultKilled;

                if (startIndex == nsMsgViewIndex_None) {
                    NS_ASSERTION(0,"startIndex == nsMsgViewIndex_None");
                    break;
                }
                threadIndex = ThreadIndexOfMsg(GetAt(startIndex), startIndex);
                ToggleIgnored(&startIndex, 1, &resultKilled);
                if (resultKilled) {
                    if (threadIndex != nsMsgViewIndex_None)
                        CollapseByIndex(threadIndex, nsnull);
                    return ViewNavigate(nsMsgNavigationType::nextUnreadThread, threadIndex, selection, numSelected, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
                }
                else {
                    *pResultIndex = startIndex;
                    *pResultKey = m_keys.GetAt(*pResultIndex);
                    return NS_OK;
                }
            }
        case nsMsgNavigationType::laterMessage:
            if (startIndex == nsMsgViewIndex_None) {
                NS_ASSERTION(0, "unexpected");
                break;
            }
            m_db->MarkLater(m_keys.GetAt(startIndex), 0);
            return ViewNavigate(nsMsgNavigationType::nextUnreadMessage, startIndex, selection, numSelected, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
        default:
            NS_ASSERTION(0, "unsupported motion");
            break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::NavigateStatus(nsMsgNavigationTypeValue motion, nsMsgViewIndex index, nsMsgKey *selection, PRUint32 numSelected, PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(selection);
    NS_ENSURE_ARG_POINTER(_retval);

    PRBool enable = PR_FALSE;
    nsresult rv = NS_ERROR_FAILURE;
    nsMsgKey resultKey = nsMsgKey_None;
    nsMsgViewIndex resultIndex = nsMsgViewIndex_None;

    // warning - we no longer validate index up front because fe passes in -1 for no
    // selection, so if you use index, be sure to validate it before using it
    // as an array index.
    switch (motion) {
        case nsMsgNavigationType::firstMessage:
        case nsMsgNavigationType::lastMessage:
            if (GetSize() > 0)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::nextMessage:
            if (IsValidIndex(index) && index < (nsMsgViewIndex) GetSize() - 1)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::previousMessage:
            if (IsValidIndex(index) && index != 0 && GetSize() > 1)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::firstFlagged:
            rv = FindFirstFlagged(&resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::nextFlagged:
            rv = FindNextFlagged(index + 1, &resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::previousFlagged:
            if (IsValidIndex(index) && index != 0)
                rv = FindPrevFlagged(index, &resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::firstNew:
            rv = FindFirstNew(&resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::laterMessage:
            enable = GetSize() > 0;
            break;
        case nsMsgNavigationType::readMore:
            enable = PR_TRUE;  // for now, always true.
            break;
        case nsMsgNavigationType::nextFolder:
        case nsMsgNavigationType::nextUnreadThread:
        case nsMsgNavigationType::nextUnreadMessage:
        case nsMsgNavigationType::toggleThreadKilled:
            enable = PR_TRUE;  // always enabled
            break;
        case nsMsgNavigationType::previousUnreadMessage:
            if (IsValidIndex(index)) {
                nsMsgKey threadId;
                rv = FindPrevUnread(m_keys.GetAt(index), &resultKey, &threadId);
                enable = (resultKey != nsMsgKey_None);
            }
            break;
        default:
            NS_ASSERTION(0,"unexpected");
            break;
    }

    *_retval = enable;
    return NS_OK;
}

// Note that these routines do NOT expand collapsed threads! This mimics the old behaviour,
// but it's also because we don't remember whether a thread contains a flagged message the
// same way we remember if a thread contains new messages. It would be painful to dive down
// into each collapsed thread to update navigate status.
// We could cache this info, but it would still be expensive the first time this status needs
// to get updated.
nsresult nsMsgDBView::FindNextFlagged(nsMsgViewIndex startIndex, nsMsgViewIndex *pResultIndex)
{
    nsMsgViewIndex lastIndex = (nsMsgViewIndex) GetSize() - 1;
    nsMsgViewIndex curIndex;

    *pResultIndex = nsMsgViewIndex_None;

    if (GetSize() > 0) {
        for (curIndex = startIndex; curIndex <= lastIndex; curIndex++) {
            PRUint32 flags = m_flags.GetAt(curIndex);
            if (flags & MSG_FLAG_MARKED) {
                *pResultIndex = curIndex;
                break;
            }
        }
    }

    return NS_OK;
}

nsresult nsMsgDBView::FindFirstNew(nsMsgViewIndex *pResultIndex)
{
    if (m_db) {
        nsMsgKey firstNewKey;
        m_db->GetFirstNew(&firstNewKey);
        if (pResultIndex)
            *pResultIndex = FindKey(firstNewKey, PR_TRUE);
    }
    return NS_OK;
}

// Generic routine to find next unread id. It doesn't do an expand of a
// thread with new messages, so it can't return a view index.
nsresult nsMsgDBView::FindNextUnread(nsMsgKey startId, nsMsgKey *pResultKey,
                                     nsMsgKey *resultThreadId)
{
    nsMsgViewIndex startIndex = FindViewIndex(startId);
    nsMsgViewIndex curIndex = startIndex;
    nsMsgViewIndex lastIndex = (nsMsgViewIndex) GetSize() - 1;
    nsresult rv = NS_OK;

    if (startIndex == nsMsgViewIndex_None)
        return NS_MSG_MESSAGE_NOT_FOUND;

    *pResultKey = nsMsgKey_None;
    if (resultThreadId)
        *resultThreadId = nsMsgKey_None;

    for (; curIndex <= lastIndex && (*pResultKey == nsMsgKey_None); curIndex++) {
        char    flags = m_flags.GetAt(curIndex);

        if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
            *pResultKey = m_keys.GetAt(curIndex);
            break;
        }
        // check for collapsed thread with new children
        if (m_sortType == nsMsgViewSortType::byThread && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
            nsMsgKey threadId = m_keys.GetAt(curIndex);
            printf("fix this\n");
            //rv = m_db->GetUnreadKeyInThread(threadId, pResultKey, resultThreadId);
            if (NS_SUCCEEDED(rv) && (*pResultKey != nsMsgKey_None))
                break;
        }
    }
    // found unread message but we don't know the thread
    if (*pResultKey != nsMsgKey_None && resultThreadId && *resultThreadId == nsMsgKey_None) {
        printf("fix this\n");
        //*resultThreadId = m_db->GetThreadIdForMsgId(*pResultKey);
    }
    return rv;
}


nsresult nsMsgDBView::FindPrevUnread(nsMsgKey startKey, nsMsgKey *pResultKey,
                                     nsMsgKey *resultThreadId)
{
    nsMsgViewIndex startIndex = FindViewIndex(startKey);
    nsMsgViewIndex curIndex = startIndex;
    nsresult rv = NS_MSG_MESSAGE_NOT_FOUND;

    if (startIndex == nsMsgViewIndex_None)
        return NS_MSG_MESSAGE_NOT_FOUND;

    *pResultKey = nsMsgKey_None;
    if (resultThreadId)
        *resultThreadId = nsMsgKey_None;

    for (; (int) curIndex >= 0 && (*pResultKey == nsMsgKey_None); curIndex--) {
        PRUint32 flags = m_flags.GetAt(curIndex);

        if (curIndex != startIndex && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
            nsMsgKey threadId = m_keys.GetAt(curIndex);
            printf("fix this\n");
            //rv = m_db->GetUnreadKeyInThread(threadId, pResultKey, resultThreadId);
            if (NS_SUCCEEDED(rv) && (*pResultKey != nsMsgKey_None))
                break;
        }
        if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
            *pResultKey = m_keys.GetAt(curIndex);
            rv = NS_OK;
            break;
        }
    }
    // found unread message but we don't know the thread
    if (*pResultKey != nsMsgKey_None && resultThreadId && *resultThreadId == nsMsgKey_None) {
        printf("fix this\n");
        //*resultThreadId = m_db->GetThreadIdForMsgId(*pResultKey);
    }
    return rv;
}

nsresult nsMsgDBView::FindFirstFlagged(nsMsgViewIndex *pResultIndex)
{
    return FindNextFlagged(0, pResultIndex);
}

nsresult nsMsgDBView::FindPrevFlagged(nsMsgViewIndex startIndex, nsMsgViewIndex *pResultIndex)
{
    nsMsgViewIndex curIndex;

    *pResultIndex = nsMsgViewIndex_None;

    if (GetSize() > 0 && IsValidIndex(startIndex)) {
        curIndex = startIndex;
        do {
            if (curIndex != 0)
                curIndex--;

            PRUint32 flags = m_flags.GetAt(curIndex);
            if (flags & MSG_FLAG_MARKED) {
                *pResultIndex = curIndex;
                break;
            }
        }
        while (curIndex != 0);
    }
    return NS_OK;
}

PRBool nsMsgDBView::IsValidIndex(nsMsgViewIndex index)
{
    return (index < (nsMsgViewIndex) m_keys.GetSize());
}

nsresult nsMsgDBView::ToggleIgnored(nsMsgViewIndex * indices, PRInt32 numIndices, PRBool *resultToggleState)
{
#if 0
    MsgERR      err;
    NeoThreadMessageHdr *thread = NULL;

    if (numIndices == 1)
    {
        MsgViewIndex    threadIndex = GetThreadFromMsgIndex(*indices, &thread);
        if (thread)
        {
            err = ToggleThreadIgnored(thread, threadIndex);
            if (resultToggleState)
                *resultToggleState = (thread->GetFlags() & kIgnored) ? TRUE : FALSE;
            thread->unrefer();
        }
    }
    else
    {
        if (numIndices > 1)
            XP_QSORT (indices, numIndices, sizeof(MSG_ViewIndex), MSG_Pane::CompareViewIndices);
        for (int curIndex = numIndices - 1; curIndex >= 0; curIndex--)
        {
            MsgViewIndex    threadIndex = GetThreadFromMsgIndex(*indices, &thread);
        }
    }
    return NS_OK;
#endif
    return NS_ERROR_NOT_IMPLEMENTED;
}
