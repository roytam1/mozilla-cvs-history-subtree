/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef MOZ_LOGGING
// sorry, this has to be before the pre-compiled header
#define FORCE_PR_LOG /* Allow logging in the release build */
#endif

#include "nsAbSync.h"
#include "prmem.h"
#include "nsAbSyncCID.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "prprf.h"
#include "nsIAddrBookSession.h"
#include "nsAbBaseCID.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsEscape.h"
#include "nsSyncDecoderRing.h"
#include "plstr.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsTextFormatter.h"
#include "nsIStringBundle.h"
#include "nsMsgI18N.h"
#include "nsIScriptGlobalObject.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIAbMDBDirectory.h" 
#include "nsAbCardProperty.h"

extern PRLogModuleInfo *ABSYNC;

/* Implementation file */

//
// Each list is stored as a single line in the history file in the following sequence: 
//
//  CRC_of_email_members/list1_key/list1_server_id,
//  list1_member1_key/list1_member1_server_id,
//  list1_member2_key/list1_member2_server_id,
//  etc.
//
// Examples: (list whose local key is 52 does not have any members):
//
// 0/52/73
// 3710889628/57/91,55/88,56/89 (2 members:55/88 & 56/89 and member 56 is type 'email')
//
// The file is loaded into memory in the following structure:
//
//  typedef struct {
//    PRInt32       serverID;
//    PRInt32       localID;
//    ulong         CRC;
//    nsUInt32Array memServerID;
//    nsUInt32Array memLocalID;
//    nsUInt32Array memFlags;
//  } syncListMappingRecord;
//

nsresult nsAbSync::LoadListsFromHistoryFile()
{
  nsresult rv = NS_OK;
  PRUint32 fileSize;
  if (NS_FAILED(mListHistoryFile->GetFileSize(&fileSize)))
    return NS_ERROR_FAILURE;

  if (NS_FAILED(mListHistoryFile->OpenStreamForReading()))
    return(NS_ERROR_FAILURE);

  mListOldTableSize = 0;
  char *listBuf = (char *)PR_Malloc(fileSize+1);
  if (!listBuf)
    return(NS_ERROR_OUT_OF_MEMORY);

  PRInt32 len = 0;
  if (NS_SUCCEEDED(mListHistoryFile->Read(&listBuf, fileSize, &len)) && len > 0)
  {
    // Count the number of lists in the buffer.
    *(listBuf+fileSize) = 0; // end buffer with '\0'.
    PRUint32 listCount = CountListLines(listBuf, listBuf+fileSize);
    if (listCount)
    {
      // Allocate the history table first.
      mListOldSyncMapingTable = (syncListMappingRecord *) PR_MALLOC(listCount * sizeof(syncListMappingRecord));
      if (!mListOldSyncMapingTable)
      {
        PR_FREEIF(listBuf);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      // Init memory & remember the list count in the history file
      memset(mListOldSyncMapingTable, 0, listCount * sizeof(syncListMappingRecord));
      mListOldTableSize = listCount;

      char *pChar, *start, *end;
      PRUint32 listNum = 0;

      pChar = start = listBuf;
      end = listBuf + fileSize;
      while (start < end)
      {
        while ((pChar < end) && (*pChar != nsCRT::CR) && (*(pChar+1) != nsCRT::LF))
          pChar++;

        if (pChar < end)
        {
          // Found a line so parse out the list and member ids.
          *pChar = 0;
          ParseListMappingEntry(start, pChar, listNum);
          pChar += 2;
          start = pChar;
        }
        else if (start < end)
        {
          // Check the last line and we're done.
          *pChar = 0;
          ParseListMappingEntry(start, pChar, listNum);
          break;
        }
        listNum++;
      }

      // Log list history table to file (can be removed later).
      for (PRUint32 i = 0; i < listNum; i++)
      {
        if (mListOldSyncMapingTable[i].serverID != 0)
        {
          nsXPIDLCString str;
          ConvertListMappingEntryToString(mListOldSyncMapingTable[i], getter_Copies(str));
          PR_LOG(ABSYNC, PR_LOG_DEBUG, ("%s[%d]: %s", "  Loading history table", i, str.get()));
        }
      }
    }
  }
  PR_FREEIF(listBuf);

  return NS_OK;
}

PRUint32 nsAbSync::CountListLines(const char *start, const char *end)
{
  PRUint32 cnt = 0;
  char *pChar, *pCurPos = (char *)start;
  while (pChar = PL_strstr(pCurPos, CRLF))
  {
    pCurPos = pChar+2;
    cnt++;
    if (pCurPos >= end)
      break;
  }
  return cnt;
}

void nsAbSync::ParseListMappingEntry(const char *start, const char *end, PRUint32 listNum)
{
  char *pChar, *PCurPos = (char *)start;
  PRInt32 cnt=0, localId, serverId;
  ulong crc;

  while (pChar < end)
  {
    // Get list id first.
    pChar = PL_strchr(PCurPos, ',');
    if (pChar)
      *pChar = 0;

    if (cnt == 0)
    {
      ExtractThreeIDs(PCurPos, '/', &crc, &localId, &serverId);
      mListOldSyncMapingTable[listNum].localID = localId;
      mListOldSyncMapingTable[listNum].serverID = serverId;
      mListOldSyncMapingTable[listNum].CRC = crc;
    }
    else
    {
      ExtractTwoIDs(PCurPos, '/', &localId, &serverId);
      mListOldSyncMapingTable[listNum].memLocalID.Add(localId);
      mListOldSyncMapingTable[listNum].memServerID.Add(serverId);
      mListOldSyncMapingTable[listNum].memFlags.Add(0);  // may not need this
    }

    if (!pChar)
      break;  // we are done

    // continue to the next one
    *pChar = ',';
    pChar++;
    PCurPos = pChar;
    cnt++;
  }
}

void nsAbSync::ExtractTwoIDs(const char *str, const char delim, PRInt32 *localId, PRInt32 *serverId)
{
  *localId = *serverId = 0;
  char *pChar = (char *)str;
  while ( (*pChar) && (*pChar != delim) )
    pChar++;

  // Terminate the string temporarily...
  if (*pChar)
    *pChar = 0;

  // Now extract the ids off the form "-106=164" or "12/34".
  *localId = atoi(str);
  *pChar = delim;
  if (*(pChar+1))
    *serverId = atoi(pChar+1);
}

void nsAbSync::ExtractThreeIDs(const char *str, const char delim, ulong *crc, PRInt32 *localId, PRInt32 *serverId)
{
  *crc = 0;
  char *pChar = (char *)str;
  while ( (*pChar) && (*pChar != delim) )
    pChar++;

  // Terminate the string temporarily...
  if (*pChar)
    *pChar = 0;

  // Extract the crc first.
  *crc = atol(str);
  *pChar = delim;
  if (*(pChar+1))
    ExtractTwoIDs(pChar+1, delim, localId, serverId);
}

nsresult nsAbSync::InitListSyncTable(nsIEnumerator *cardEnum)
{
  nsresult rv;
  nsCOMPtr<nsIAbCard> card;
  nsCOMPtr<nsISupports> obj;

  cardEnum->First();
  do
  {
    if (NS_FAILED(cardEnum->CurrentItem(getter_AddRefs(obj))))
      break;
    else
    {
      card = do_QueryInterface(obj, &rv);
      if (NS_SUCCEEDED(rv) && (card))
      {
        // If the card is not a list then ignore it.
        PRBool isMailList = PR_FALSE;
        rv = card->GetIsMailList(&isMailList);
        if (isMailList)
          mListNewTableSize++;
      }
    }
  } while (NS_SUCCEEDED(cardEnum->Next()));

  if (! mListNewTableSize)
    return NS_OK;

  mListNewSyncMapingTable = (syncListMappingRecord *) PR_MALLOC(mListNewTableSize * sizeof(syncListMappingRecord));
  if (!mListNewSyncMapingTable)
    return NS_ERROR_OUT_OF_MEMORY;

  // Init the memory!
  memset(mListNewSyncMapingTable, 0, (mListNewTableSize * sizeof(syncListMappingRecord)));
  return NS_OK;
}

nsresult nsAbSync::PatchListHistoryTableWithNewID(PRInt32 listLocalID, PRInt32 listServerID, PRInt32 aMultiplier)
{
  for (PRUint32 i = 0; i < mListNewTableSize; i++)
  {
    if (mListNewSyncMapingTable[i].localID == (listLocalID * aMultiplier))
    {
      mListNewSyncMapingTable[i].serverID = listServerID;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult nsAbSync::PatchListHistoryTableWithNewMemberID(PRInt32 listServerID, PRInt32 memLocalID, PRInt32 memServerID, PRInt32 aMultiplier)
{
  for (PRUint32 i = 0; i < mListNewTableSize; i++)
  {
    if (mListNewSyncMapingTable[i].serverID == listServerID)
    {
      PRUint32 cnt = mListNewSyncMapingTable[i].memLocalID.GetSize();
      for (PRUint32 j = 0; j < cnt; j++)
        if (mListNewSyncMapingTable[i].memLocalID.GetAt(j) == (PRUint32)(memLocalID * aMultiplier))
        {
          mListNewSyncMapingTable[i].memServerID.SetAt(j, memServerID);
          return NS_OK;
        }
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult nsAbSync::SaveCurrentListsToHistoryFile()
{
  if (!mListHistoryFile)
    return NS_ERROR_FAILURE;

  if (NS_FAILED(mListHistoryFile->OpenStreamForWriting()))
  	return NS_ERROR_FAILURE;

  // Ok, these are the lists that exist when we started the sync op.
  PRInt32 writeSize;
  PRUint32 writeCount = 0;
  while (writeCount < mListNewTableSize)
  {
    // If server id is not 0 then it's a valid record, so save it to file.
    if (mListNewSyncMapingTable[writeCount].serverID != 0)
    {
      nsXPIDLCString str;
      ConvertListMappingEntryToString(mListNewSyncMapingTable[writeCount], getter_Copies(str));
      PR_LOG(ABSYNC, PR_LOG_DEBUG, ("%s[%d]: %s", "  Saving existing list", writeCount, str.get()));
      if (NS_FAILED(mListHistoryFile->Write(str.get(),  str.Length(), &writeSize))
                                       || (writeSize != (PRInt32)str.Length()))
        return NS_ERROR_FAILURE;
    }
    writeCount++;
  }

  // These are the lists that we got back from the server and are new to us now!
  writeCount = 0;
  if (mListNewServerTable)
  {
    while (writeCount < (PRUint32) mListNewServerTable->Count())
    {
      syncListMappingRecord *tRec = (syncListMappingRecord *)mListNewServerTable->ElementAt(writeCount);
      if (!tRec)
        continue;

      nsXPIDLCString str;
      ConvertListMappingEntryToString(*tRec, getter_Copies(str));
      PR_LOG(ABSYNC, PR_LOG_DEBUG, ("%s[%d]: %s", "  Saving new list", writeCount, str.get()));
      if (NS_FAILED(mListHistoryFile->Write(str.get(),  str.Length(), &writeSize))
                                       || (writeSize != (PRInt32)str.Length()))
        return NS_ERROR_FAILURE;

      writeCount++;
    }
  }

  if (mListHistoryFile)
    mListHistoryFile->CloseStream();
  return NS_OK;
}

//
// For a given syncListMappingRecord generate one of the following output string:
//   CRC/52/73
//   CRC/57/91,55/88,56/89
//
// Notet that 'crc' is the check sum of all the email addresses from the 'eamil
// adress' type of members and is used to tell if we need to send servers a list of
// new email addresses. This happens when cards of type 'email address' are edited
// and crc seems to be the best way to tell if something has changed (instead storing
// all email addresses in the history file).
//
void nsAbSync::ConvertListMappingEntryToString(syncListMappingRecord &listRecord, char **result)
{
  nsCAutoString idString;
  char buf[24]; // enough for a ulong value.

  // Can't use AppendInt() call for an ulong since it may become negative.
  sprintf(buf,"%lu", listRecord.CRC);
  idString.Append(buf);
  idString.Append("/");
  idString.AppendInt(listRecord.localID,  10 /* base 10 */);
  idString.Append("/");
  idString.AppendInt(listRecord.serverID, 10 /* base 10 */);

  PRUint32 cnt = listRecord.memLocalID.GetSize();
  for (PRUint32 i = 0; i < cnt; i++)
    if (listRecord.memServerID.GetAt(i) != 0)
    {
      idString.Append(",");
      idString.AppendInt(listRecord.memLocalID.GetAt(i), 10 /* base 10 */);
      idString.Append("/");
      idString.AppendInt(listRecord.memServerID.GetAt(i), 10 /* base 10 */);
    }

  idString.Append(CRLF);  // end with CRLF
  *result = ToNewCString(idString);
}

// For a given new list, generate protocol string for all new members.
// Also add it to the history table.
nsresult nsAbSync::GenerateMemberProtocolForNewList(nsIAbCard *listCard, PRUint32 listIndex, nsString &protLine)
{
  nsresult rv;
  nsCOMPtr<nsISupportsArray> members;

  protLine.Truncate(); // init string

  nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(listCard, &rv)); 
  if (NS_FAILED(rv) || !dbcard)
    return NS_ERROR_FAILURE;

  PRUint32 listKey;
  if (NS_FAILED(dbcard->GetKey(&listKey)) || (listKey <=0))
    return NS_ERROR_FAILURE;

  // Now fill in the list mapping table for this list
  mListNewSyncMapingTable[listIndex].localID = listKey;
  mListNewSyncMapingTable[listIndex].CRC = 0; // init crc

  rv = GetMemberListByCard(listCard, getter_AddRefs(members));
  if (NS_SUCCEEDED(rv) && members)
  {
    PRUint32 total = 0;
    members->Count(&total);
    if (total)
    {
      nsString emailString;
      PRUint32 i;
      for (i = 0; i < total; i++)
      {
        nsCOMPtr <nsISupports> item = getter_AddRefs(members->ElementAt(i));
        nsCOMPtr<nsIAbMDBCard>dbCard = do_QueryInterface(item, &rv);
        if (NS_FAILED(rv) || !dbCard)
          break;

        PRUint32 aKey;
        if (NS_FAILED(dbCard->GetKey(&aKey)) || (aKey <=0))
          continue;

        // This member may already has the server id so check it.
        PRInt32 serverID;
        if (NS_FAILED(LocateServerIDFromClientID(aKey, &serverID)))
          continue;

        mListNewSyncMapingTable[listIndex].memLocalID.Add(aKey);
        mListNewSyncMapingTable[listIndex].memServerID.Add(serverID);
        mListNewSyncMapingTable[listIndex].memFlags.Add(SYNC_ADD);  // may not need this

        // If it's an 'email address' card (type) then store the email address as
        // we need to send servers a separate cmd for these email string members.
        nsXPIDLString email;
        nsCOMPtr<nsIAbCard> card = do_QueryInterface(dbCard, &rv);
        if (WhichCardType(card) == SYNC_IS_AOL_ADDITIONAL_EMAIL)
        {
          card->GetPrimaryEmail(getter_Copies(email));
          if (email && !email.IsEmpty())
          {
            if (!emailString.IsEmpty())
              emailString.Append(NS_LITERAL_STRING(","));
            emailString.Append(email);
          }
        }
        else
          AddAListMememerToProtocolLine(listKey, aKey, i, protLine);
      }

      // If we have 'email addresss' members then add 'emailstringUpdate' cmd to the protocol.
      if (! emailString.IsEmpty())
      {
        GenerateEmailStringUpdateProtocol(listKey, emailString, protLine);
        // Now store the member CRC.
        char *tVal = ToNewCString(emailString);
        mListNewSyncMapingTable[listIndex].CRC = GetCRC(tVal);
        nsCRT::free(tVal);
      }
    }
    // If fails for any reason, rest protocol string so we don't send something wrong to the server.
    if (NS_FAILED(rv))
      protLine.Truncate();
  }
  return rv;
}

void nsAbSync::AppendProtocolCmdHeader(nsString &protLine, const char *cmd)
{
  if (!protLine.IsEmpty())
    protLine.Append(NS_LITERAL_STRING("&"));
  char *tVal = PR_smprintf("%d", mCurrentPostRecord);
  protLine.Append(NS_ConvertASCIItoUCS2(tVal) + NS_LITERAL_STRING("="));
  protLine.Append(NS_ConvertASCIItoUCS2(cmd));
  mCurrentPostRecord++;
  PR_FREEIF(tVal);
}

void nsAbSync::GenerateEmailStringUpdateProtocol(PRInt32 listID, nsString emailString, nsString &protLine)
{
  // Construct protocol cmd header for updating member email strings,
  AppendProtocolCmdHeader(protLine, SYNC_ESCAPE_MOD_LIST_EMAIL);

  PRInt32 listServerID;
  if (NS_FAILED(LocateServerIDFromClientID(listID, &listServerID)) || (listServerID == 0))
    listServerID = listID * -1;

  NS_ASSERTION((listServerID>0),"ab sync:  GenerateEmailStringUpdateProtocol(), serer id should be positive.");

  char *tVal = PR_smprintf("%d", listServerID);
  if (tVal)
  {
    protLine.Append(NS_LITERAL_STRING("%26list_id%3D") + NS_ConvertASCIItoUCS2(tVal));
    nsCRT::free(tVal);
  }

  protLine.Append(NS_LITERAL_STRING("%26email_string%3D") + emailString);
}

void nsAbSync::AddAListMememerToProtocolLine(PRUint32 listKey, PRUint32 memberKey, PRUint32 cid, nsString &protLine)
{
  // Construct protocol cmd header for adding list members,
  AppendProtocolCmdHeader(protLine, SYNC_ESCAPE_ADD_LIST_CONTACT);

  // If the id represents an existing local record then use the
  // correspoding server id, otherwise, use the negative local id.
  PRInt32 listID, recordID;
  if (NS_FAILED(LocateServerIDFromClientID(listKey, &listID)) || (listID == 0))
    listID = listKey * -1;

  if (NS_FAILED(LocateServerIDFromClientID(memberKey, &recordID)) || (recordID == 0))
    recordID = memberKey * -1;

  // Now build the protocol line.
  char *tVal = PR_smprintf("%d", recordID);
  if (tVal)
  {
    protLine.Append(NS_LITERAL_STRING("%26contact_record_id%3D") + NS_ConvertASCIItoUCS2(tVal));
    nsCRT::free(tVal);
  }
  tVal = PR_smprintf("%d", listID);
  if (tVal)
  {
    protLine.Append(NS_LITERAL_STRING("%26list_id%3D") + NS_ConvertASCIItoUCS2(tVal));
    nsCRT::free(tVal);
  }
  // cid for member list is not really used by us so we just use the member seq number here.
  tVal = PR_smprintf("%d", ((cid+1) * -1));
  if (tVal)
  {
    protLine.Append(NS_LITERAL_STRING("%26cid%3D") + NS_ConvertASCIItoUCS2(tVal));
    nsCRT::free(tVal);
  }
}

nsresult nsAbSync::GetMemberListByCard(nsIAbCard *aCard, nsISupportsArray **aList)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsXPIDLCString mailListURI;
  rv = aCard->GetMailListURI(getter_Copies(mailListURI));
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(mailListURI.get(), getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv,rv);
    
  nsCOMPtr <nsIAbDirectory> mailList = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
    
  return(mailList->GetAddressLists(aList));
}

// For a given list card, generate protocol string for all changed members, if any.
nsresult nsAbSync::CheckCurrentListForChangedMember(nsIAbCard *listCard, PRUint32 listIndex, nsString &protLine)
{
  nsresult rv;
  nsCOMPtr<nsISupportsArray> members;

  protLine.Truncate(); // init string

  nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(listCard, &rv)); 
  if (NS_FAILED(rv) || !dbcard)
    return NS_ERROR_FAILURE;

  PRUint32 listKey;
  if (NS_FAILED(dbcard->GetKey(&listKey)) || (listKey <=0))
    return NS_ERROR_FAILURE;

  PRInt32 listServerID;
  if (NS_FAILED(LocateServerIDFromClientID(listKey, &listServerID)))
    return NS_ERROR_FAILURE;

  // Now fill in the list mapping table for this list
  mListNewSyncMapingTable[listIndex].localID = listKey;
  mListNewSyncMapingTable[listIndex].serverID = listServerID;

  rv = GetMemberListByCard(listCard, getter_AddRefs(members));
  if (NS_SUCCEEDED(rv) && members)
  {
    // Locate the list history entry.
    syncListMappingRecord *historyRecord;
    rv = LocateHistoryListRecord(listKey, &historyRecord);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString emailString;
    PRUint32 total = 0;
    members->Count(&total);
    if (total)
    {
      // Get the member info from the history file so we can tell the difference.
      // Note that the changed member is implemented by adding and deleting members.
      PRUint32 i;
      PRUint32 aKey;
      PRInt32 serverID;
      for (i = 0; i < total; i++)
      {
        nsCOMPtr <nsISupports> item = getter_AddRefs(members->ElementAt(i));
        nsCOMPtr<nsIAbMDBCard> dbCard = do_QueryInterface(item, &rv);
        if (NS_FAILED(rv) || !dbCard)
          continue; // should we continue here?

        if (NS_FAILED(dbCard->GetKey(&aKey)) || (aKey <=0))
          continue; // should we continue here?

        if (NS_FAILED(LocateServerIDFromClientID(aKey, &serverID)))
          continue; // should we continue here?

        // If it's an 'email address' card (type) then store the email address for later use.
        nsXPIDLString email;
        nsCOMPtr<nsIAbCard> card = do_QueryInterface(dbCard, &rv);
        if (WhichCardType(card) == SYNC_IS_AOL_ADDITIONAL_EMAIL)
        {
          card->GetPrimaryEmail(getter_Copies(email));
          if (email && !email.IsEmpty())
          {
            if (!emailString.IsEmpty())
              emailString.Append(NS_LITERAL_STRING(","));
            emailString.Append(email);
          }
        }

        mListNewSyncMapingTable[listIndex].memLocalID.Add(aKey);
        mListNewSyncMapingTable[listIndex].memServerID.Add(serverID);
        mListNewSyncMapingTable[listIndex].memFlags.Add(SYNC_MODIFIED);  // may not need this

        // If this member not found in the list then add it to the protocol string.
        if (MemberNotFoundInHistory(historyRecord, aKey))
          AddAListMememerToProtocolLine(listKey, aKey, i, protLine);
      }
    }

    // Now, check non 'email address' members that have been deleted.
    CheckDeletedMembers(historyRecord, protLine);

    // If email members have been altered then add 'emailstringUpdate' cmd to the protocol.
    // We do this by comparing the new CRC against the one in the list history entry.
    ulong newCRC = 0;
    if (! emailString.IsEmpty())
    {
      char *tVal = ToNewCString(emailString);
      newCRC = GetCRC(tVal);
      nsCRT::free(tVal);
    }

    if (historyRecord->CRC != newCRC)
      GenerateEmailStringUpdateProtocol(listKey, emailString, protLine);

    // Finally store the CRC in the list table.
    mListNewSyncMapingTable[listIndex].CRC = newCRC;
  }
  return rv;
}

void nsAbSync::CheckDeletedMembers(syncListMappingRecord *listRecord, nsString &protLine)
{
  PRUint32 cnt = listRecord->memLocalID.GetSize();

  for (PRUint32 i = 0; i < cnt; i++)
    if (!(listRecord->memFlags.GetAt(i) & SYNC_PROCESSED))
    {
      // This member has been removed and if it's an "email address" card (type)
      // then set emailMembersHaveChanged to true and continue. This is because
      // we need a different protocol for this kind of card deletion. We also need
      // to remove the card itself.
      if (GetCardTypeByMemberId(listRecord->memLocalID.GetAt(i)) & SYNC_IS_AOL_ADDITIONAL_EMAIL)
      {
        nsresult rv = DeleteCardByServerID(listRecord->memServerID.GetAt(i));
        continue;
      }

      // Construct protocol cmd header for deleting list members,
      AppendProtocolCmdHeader(protLine, SYNC_ESCAPE_DEL_LIST_CONTACT);

      char *tVal = PR_smprintf("%d", listRecord->serverID);
      if (tVal)
      {
        protLine.Append(NS_LITERAL_STRING("%26list_id%3D") + NS_ConvertASCIItoUCS2(tVal));
        nsCRT::free(tVal);
      }
      tVal = PR_smprintf("%d", listRecord->memServerID.GetAt(i));
      if (tVal)
      {
        protLine.Append(NS_LITERAL_STRING("%26contact_record_id%3D") + NS_ConvertASCIItoUCS2(tVal));
        nsCRT::free(tVal);
      }
    }
}

nsresult nsAbSync::LocateExistingListRecord(PRUint32 listID, syncListMappingRecord **result)
{
  NS_ENSURE_ARG_POINTER(result);

  for (PRUint32 i = 0; i < mListNewTableSize; i++)
  {
    if (mListNewSyncMapingTable[i].localID == (PRInt32)listID)
    {
      *result = &mListNewSyncMapingTable[i];
      return NS_OK;
    }
  }

  if (mListNewServerTable)
  {
    PRUint32 writeCount = 0;
    while (writeCount < (PRUint32) mListNewServerTable->Count())
    {
      syncListMappingRecord *tRec = (syncListMappingRecord *)mListNewServerTable->ElementAt(writeCount);
      if (!tRec)
        continue;

      if (tRec->localID == (PRInt32)listID)
      {
        *result = tRec;
        return NS_OK;
      }

      writeCount++;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult nsAbSync::LocateHistoryListRecord(PRUint32 listID, syncListMappingRecord **result)
{
  NS_ENSURE_ARG_POINTER(result);

  for (PRUint32 i = 0; i < mListOldTableSize; i++)
  {
    if (mListOldSyncMapingTable[i].localID == (PRInt32)listID)
    {
      *result = &mListOldSyncMapingTable[i];
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

PRBool nsAbSync::MemberNotFoundInHistory(syncListMappingRecord *listRecord, PRUint32 memberID)
{
  PRUint32 cnt = listRecord->memLocalID.GetSize();
  for (PRUint32 i = 0; i < cnt; i++)
    if (listRecord->memLocalID.GetAt(i) == memberID)
    {
      listRecord->memFlags.SetAt(i, SYNC_PROCESSED);
      return PR_FALSE;
    }
  return PR_TRUE;
}

nsresult nsAbSync::InitNewListTablesAndOpenDB(nsIAddrDatabase **aDatabase, nsIAbDirectory **aDirectory)
{
  NS_ENSURE_ARG_POINTER(aDatabase);
  NS_ENSURE_ARG_POINTER(aDirectory);

  nsresult rv;
  // Get the RDF service...
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // this should not be hardcoded to abook.mab
  // this works for any address book...not sure why
  // absync on go againt abook.mab - candice
  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // query interface 
  nsCOMPtr <nsIAbDirectory> directory;
  directory = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  *aDirectory = directory;

  // Create the history array for users if it doesn't already exist.
  if (!mNewServerTable)
  {
    mNewServerTable = new nsVoidArray();
    if (!mNewServerTable)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  // Create the history array for lists if it doesn't already exist.
  if (!mListNewServerTable)
  {
    mListNewServerTable = new nsVoidArray();
    if (!mListNewServerTable)
    {
      delete mNewServerTable;
      mNewServerTable = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  // Now, open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, aDatabase);
  return rv;
}

nsresult nsAbSync::AddNewMailingLists()
{
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase;
  PRInt32         addCount = 0;
  PRInt32         i,j;
  PRInt32         serverID;
  PRInt32         localID;
  nsCOMPtr<nsIAbCard> newCard;
  nsIAbCard       *tCard = nsnull;
  nsString        tempProtocolLine;
  PRBool          isNewCard = PR_TRUE;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mNewRecordValues->Count() % mNewRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the add count.
  addCount = mNewRecordValues->Count() / mNewRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  // 
  // Create the new card that we will eventually add to the 
  // database...
  //
  for (i = 0; i < addCount; i++)
  {    
    serverID = 0;     // for safety

    // Check to see if this is a new or existing card.
    localID = HuntForExistingABEntryInServerRecord(i, aDatabase, directory, PR_FALSE, &serverID, &tCard);
    if ( (localID > 0) && (nsnull != tCard))
    {
      // This is an existing list in the local address book
      newCard = tCard;
      isNewCard = PR_FALSE;
    }
    else
    {
      // This is a new entry!
      rv = nsComponentManager::CreateInstance(NS_ABCARDPROPERTY_CONTRACTID, nsnull, NS_GET_IID(nsIAbCard), 
                                              getter_AddRefs(newCard));
      isNewCard = PR_TRUE;
    }

    if (NS_FAILED(rv))
    {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto EarlyExit;
    }

    //char *listName = nsnull;
    nsString listName;
    PRInt32   listID;
    for (j = 0; j < mNewRecordTags->Count(); j++)
    {
      nsString *val = mNewRecordValues->StringAt((i*(mNewRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        // See if this is the record_id, keep it around for later...
        nsString *tagVal = mNewRecordTags->StringAt(j);

        // Ok, "val" could still be URL Encoded, so we need to decode
        // first and then pass into the call...
        //
        char *myTStr = ToNewCString(*val);
        if (myTStr)
        {
          char *ret = nsUnescape(myTStr);
          if (ret)
          {
            val->AssignWithConversion(ret);
            PR_FREEIF(ret);
          }
        }
        if (tagVal->Equals(NS_LITERAL_STRING("list_id")))
        {
          PRInt32 errorCode;
          listID = val->ToInteger(&errorCode);
          //continue;
        }
        else if (tagVal->Equals(NS_LITERAL_STRING("listname")))
          listName.Assign(val->get());
      }
    }

    // Ok, now that we are here, we should check if this is a recover from a crash situation.
    // If it is, then we should try to find the CRC for this entry in the local address book
    // and tweak a new flag if it is already there.
    //
    PRBool    cardAlreadyThere = PR_FALSE;

    // See if last sync failed.
    ulong     tempCRC;
    if (mLastSyncFailed)
      cardAlreadyThere = CardAlreadyInAddressBook(newCard, &localID, &tempCRC);

    // Ok, now we need to modify or add the card! ONLY IF ITS NOT THERE ALREADY
    if (!cardAlreadyThere)
    {
      // Now update the list info.
      newCard->SetDisplayName(listName.get());
      newCard->SetLastName(listName.get());
      if (!isNewCard)
      {
        ChangeMailingListName(directory, listName, newCard);
      }
      else
      {
        // Create the new list and set the display name (for CRC purpose).
        CreateANewMailingList(listName, aDatabase, directory, &localID);
      }
    }

    //
    // Now, calculate the NEW CRC for the new or updated list...
    //
    if (NS_FAILED(GenerateProtocolForList(newCard, PR_FALSE, tempProtocolLine)))
      continue;

    // Get the CRC for this temp entry line...
    char *tLine = ToNewCString(tempProtocolLine);
    if (!tLine)
      continue;

    if (!isNewCard)
    {
      // First try to patch the old table if this is an old card...if that
      // fails, then flip the flag to TRUE and have a new record created
      // in newSyncRecord
      //
      if (NS_FAILED(PatchHistoryTableWithNewID(localID, serverID, 1, GetCRC(tLine))))
        isNewCard = PR_TRUE;
    }

    if (isNewCard)
    {
      syncMappingRecord     *newSyncRecord = nsnull;
      syncListMappingRecord *newListSyncRecord = nsnull;

      newListSyncRecord = (syncListMappingRecord *)PR_Malloc(sizeof(syncListMappingRecord));
      newSyncRecord = (syncMappingRecord *)PR_Malloc(sizeof(syncMappingRecord));
      if (newListSyncRecord && newSyncRecord)
      {
        // Add ids to both user and list history arrays.
        memset(newSyncRecord, 0, sizeof(syncMappingRecord));
        newSyncRecord->CRC = GetCRC(tLine);
        newSyncRecord->serverID = serverID;
        newSyncRecord->localID = localID;
        newSyncRecord->flags |= SYNC_IS_LIST;
        mNewServerTable->AppendElement((void *)newSyncRecord);

        memset(newListSyncRecord, 0, sizeof(syncListMappingRecord));
        newListSyncRecord->serverID = serverID;
        newListSyncRecord->localID = localID;
        mListNewServerTable->AppendElement((void *)newListSyncRecord);
      }
      else
      {
        PR_FREEIF(newSyncRecord);
        PR_FREEIF(newListSyncRecord);
      }
    }


    nsCRT::free(tLine);

    newCard = nsnull;
  }

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase); 
  
  return rv;
}

// Used to create a new list
nsresult nsAbSync::CreateANewMailingList(nsString listName, nsIAddrDatabase *aDatabase, nsIAbDirectory *parentDir, PRInt32* localID)
{
  NS_ENSURE_ARG_POINTER(localID);
  nsresult rv;

  nsCOMPtr <nsIAbDirectory> newList;
  rv = nsComponentManager::CreateInstance(NS_ABDIRPROPERTY_CONTRACTID, nsnull, NS_GET_IID(nsIAbDirectory), 
                                              getter_AddRefs(newList));
  NS_ENSURE_SUCCESS(rv, rv);
  newList->SetDirName(listName.get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 key;
  rv = parentDir->AddMailListWithKey(newList, &key);
  NS_ENSURE_SUCCESS(rv, rv);
  *localID = (PRUint32) key;

  return rv;
}

PRBool nsAbSync::MemberAlreadyExists(syncListMappingRecord *listRecord, PRInt32 localID, PRInt32 serverID)
{
  PRUint32 cnt = listRecord->memLocalID.GetSize();
  for (PRUint32 i = 0; i < cnt; i++)
    if ( (listRecord->memLocalID.GetAt(i) == (PRUint32)localID) &&
         (listRecord->memServerID.GetAt(i) == (PRUint32)serverID) )
      return PR_TRUE;
    //else
    //  return PR_FALSE;
  return PR_FALSE;
}

nsresult nsAbSync::AddNewMailingListMembers()
{
  nsresult rv = NS_OK;
  nsIAddrDatabase *aDatabase = nsnull;
  PRInt32         addCount = 0;
  nsIAbCard       *tCard = nsnull;
  nsString        tempProtocolLine;
  PRBool          isNewCard = PR_TRUE;
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mNewRecordValues->Count() % mNewRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the add count.
  addCount = mNewRecordValues->Count() / mNewRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  // Find the list and member local ids (keys), get their 
  // cards from database and add the members to the list.
  PRInt32 i, j;
  for (i = 0; i < addCount; i++)
  {
    nsString listName;
    PRInt32  listID=0, contactID=0, listLocalID=0, localID=0, errorCode;
    for (j = 0; j < mNewRecordTags->Count(); j++)
    {
      nsString *val = mNewRecordValues->StringAt((i*(mNewRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        // Look for list_id and contact_record_id.
        nsString *tagVal = mNewRecordTags->StringAt(j);

        // Ok, "val" could still be URL Encoded, so we need to decode first.
        char *myTStr = ToNewCString(*val);
        if (myTStr)
        {
          char *ret = nsUnescape(myTStr);
          if (ret)
          {
            val->AssignWithConversion(ret);
            PR_FREEIF(ret);
          }
        }

        if (tagVal->Equals(NS_LITERAL_STRING("list_id")))
          listID = val->ToInteger(&errorCode);
        else if (tagVal->Equals(NS_LITERAL_STRING("contact_record_id")))
          contactID = val->ToInteger(&errorCode);
      }
    }

    // Find the local list id (which is our db key) from server id.
    if (NS_FAILED(LocateClientIDFromServerID(listID, &listLocalID)))
      continue; // continue since the record has already been removed.

    // Find the local member id (which is our db key) from server id.
    if (NS_FAILED(LocateClientIDFromServerID(contactID, &localID)))
      continue; // should we continue?

    // See if the member already exists (in case servers send us one such for some reason).
    syncListMappingRecord *listRecord;
    if (NS_FAILED(LocateExistingListRecord(listLocalID, &listRecord)))
      continue;

    if (MemberAlreadyExists(listRecord, localID, contactID))
      continue; // continue since the member already exists.

    // Now, we have the member local key, get the card in the address book.
    nsCOMPtr<nsIAbCard> newCard;
    if (NS_FAILED(FindCardByClientID(localID, aDatabase, directory, getter_AddRefs(newCard))))
      continue; // continue since the record has already been removed.

    // Now, we have the list local key, get the card in the address book.
    nsCOMPtr<nsIAbCard> listCard;
    if (NS_FAILED(FindCardByClientID(listLocalID, aDatabase, directory, getter_AddRefs(listCard))))
      continue; // should we continue?

    // Now, add the member to the list.
    rv = AddMemberToList(listCard, newCard);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDatabase->Commit(nsAddrDBCommitType::kLargeCommit);

    // Seems like verything is fine so add the member to the sync table.
    if (NS_SUCCEEDED(rv))
    {
      listRecord->memLocalID.Add(localID);
      listRecord->memServerID.Add(contactID);
      listRecord->memFlags.Add(SYNC_PROCESSED); // mark processed so no one can remove it.
    }
  }

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase);

  return rv;
}

nsresult nsAbSync::ChangeMailingListName(nsIAbDirectory *parentDir, nsString listName, nsIAbCard *listCard)
{
  if (!parentDir || !listCard)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  // Get the parent directory uri.
  nsCOMPtr<nsIAbMDBDirectory> dbdirectory(do_QueryInterface(parentDir, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString parentUri;
  rv = dbdirectory->GetDirUri(getter_Copies(parentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  // Now get the nsIAbDirectory ptr for the list itself.
  nsXPIDLCString mailListURI;
  rv = listCard->GetMailListURI(getter_Copies(mailListURI));
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(mailListURI.get(), getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv,rv);
    
  nsCOMPtr <nsIAbDirectory> mailList = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  mailList->SetDirName(listName.get());

  rv = mailList->EditMailListToDatabase(parentUri.get(), listCard);
  return rv;
}

nsresult nsAbSync::AddMemberToList(nsIAbCard *listCard, nsIAbCard *newCard)
{
  if (!listCard)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsXPIDLCString mailListURI;
  rv = listCard->GetMailListURI(getter_Copies(mailListURI));
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(mailListURI.get(), getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv,rv);
    
  nsCOMPtr <nsIAbDirectory> mailList = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // Add the new member to list.
  nsCOMPtr<nsIAbMDBDirectory> dbList(do_QueryInterface(mailList, &rv));
  NS_ENSURE_SUCCESS(rv,rv);
  dbList->AddAddressToList(newCard);
  NS_ENSURE_SUCCESS(rv,rv);

  // Update the list.
  rv = mailList->EditMailListToDatabase(mailListURI.get(), listCard);
  return rv;
}

nsresult nsAbSync::DeleteMemberFromList(nsIAddrDatabase *aDatabase, nsIAbCard *listCard, PRInt32 memberLocalID)
{
  if (!listCard)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsXPIDLCString mailListURI;
  rv = listCard->GetMailListURI(getter_Copies(mailListURI));
  NS_ENSURE_SUCCESS(rv,rv);
  
  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(mailListURI.get(), getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv,rv);
    
  nsCOMPtr <nsIAbDirectory> mailList = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsISupportsArray> members;
  rv = mailList->GetAddressLists(getter_AddRefs(members));
  NS_ENSURE_SUCCESS(rv,rv);

  // Remove the member from the list's address list and then edit the list itself.
  PRUint32 total = 0;
  members->Count(&total);
  if (total)
  {
    for (PRUint32 i = 0; i < total; i++)
    {
      nsCOMPtr <nsISupports> item = getter_AddRefs(members->ElementAt(i));
      nsCOMPtr<nsIAbMDBCard> dbcard = do_QueryInterface(item, &rv);
      if (NS_FAILED(rv) || !dbcard)
        continue;
      PRUint32 aKey;
      if (NS_FAILED(dbcard->GetKey(&aKey)) || (aKey <=0))
        continue;
      if (aKey == (PRUint32)memberLocalID)
      {
        members->DeleteElementAt(i);
        rv = mailList->EditMailListToDatabase(mailListURI.get(), listCard);
        break;
      }
    }
  }
  return rv;
}

nsresult nsAbSync::DeleteMailingListMembers()
{
  // DeleteCardFromMailList(nsIAbDirectory *mailList, nsIAbCard *card, PRBool aNotify)
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr <nsIRDFResource> resource;
  nsCOMPtr <nsIAbDirectory> directory;
  nsIAddrDatabase *aDatabase = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mDeletedRecordValues->Count() % mDeletedRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the delete count.
  PRInt32 delCount = mDeletedRecordValues->Count() / mDeletedRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  // 
  // Find the list and member local ids (keys) and get their cards from database.
  //
  PRInt32 i, j;
  for (i = 0; i < delCount; i++)
  {
    nsString listName;
    PRInt32  listID=0, contactID=0, listLocalID=0, memberLocalID=0, errorCode;
    for (j = 0; j < mDeletedRecordTags->Count(); j++)
    {
      nsString *val = mDeletedRecordValues->StringAt((i*(mDeletedRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        nsString *tagVal = mDeletedRecordTags->StringAt(j);
        // Look for list_id .
        if (tagVal->Equals(NS_LITERAL_STRING("list_id")))
          listID = val->ToInteger(&errorCode);
        else if (tagVal->Equals(NS_LITERAL_STRING("contact_record_id")))
          contactID = val->ToInteger(&errorCode);
      }
    }
  
    if (NS_FAILED(LocateClientIDFromServerID(listID, &listLocalID)))
      continue;
    if (NS_FAILED(LocateClientIDFromServerID(contactID, &memberLocalID)))
      continue;
    nsCOMPtr<nsIAbCard> listCard;
    if (NS_FAILED(FindCardByClientID(listLocalID, aDatabase, directory, getter_AddRefs(listCard))))
      continue;
    //nsCOMPtr<nsIAbCard> memberCard;
    //if (NS_FAILED(FindCardByClientID(memberLocalID, aDatabase, directory, getter_AddRefs(memberCard))))
    //  continue;

    // Now delete the member.
    rv = DeleteMemberFromList(aDatabase, listCard, memberLocalID);

    if (NS_SUCCEEDED(rv))
    {
      syncListMappingRecord *listRecord;
      // reset server id so we don't save it to history table later.
      rv = LocateExistingListRecord(listLocalID, &listRecord);
      if (NS_SUCCEEDED(rv))
      {
        PRUint32 cnt = listRecord->memLocalID.GetSize();
        for (PRUint32 k = 0; k < cnt; k++)
          if (listRecord->memLocalID.GetAt(k) == (PRUint32)memberLocalID)
            listRecord->memServerID.SetAt(k, 0);
      }
    }
  }  

EarlyExit:
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase);

  return rv;
}

nsresult        
nsAbSync::DeleteMailingLists()
{
  nsresult    rv = NS_ERROR_FAILURE;
  nsCOMPtr <nsIRDFResource> resource;
  nsCOMPtr <nsIAbDirectory> directory;
  nsIAddrDatabase *aDatabase = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mDeletedRecordValues->Count() % mDeletedRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the delete count.
  PRInt32 delCount = mDeletedRecordValues->Count() / mDeletedRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  // Find the list local ids (keys) and remove the cards from database.
  PRInt32 i, j;
  for (i = 0; i < delCount; i++)
  {
    nsString listName;
    PRInt32  listID=0, errorCode;
    for (j = 0; j < mDeletedRecordTags->Count(); j++)
    {
      nsString *val = mDeletedRecordValues->StringAt((i*(mDeletedRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        nsString *tagVal = mDeletedRecordTags->StringAt(j);
        // Look for list_id .
        if (!tagVal->Equals(NS_LITERAL_STRING("list_id")))
          continue;

        listID = val->ToInteger(&errorCode);
        if ((listID == 0) || NS_FAILED(errorCode))
          continue;

        // Now delete the list card.
        rv = DeleteCardByServerID(listID);
        if (NS_SUCCEEDED(rv))
        {
          PRInt32 listLocalID;
          syncListMappingRecord *listRecord;
          if (NS_SUCCEEDED(LocateClientIDFromServerID(listID, &listLocalID)))
          { 
            // Reset server id so we don't save it to history table later.
            rv = LocateExistingListRecord(listLocalID, &listRecord);
            listRecord->serverID = 0;
            // Now delete cards associated with email members.
            DeleteAllEmailMemberCards(listRecord);
          }
        }
      }
    }
  }  

EarlyExit:
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase);

  return rv;
}

nsresult nsAbSync::DeleteGroups()
{
  // Since we map a group to a sinlge card in the db AND the
  // server respnose to delete a group is the same as deleting
  // a user (ie, just record_id) so we simply call DeleteUsers()
  // to delete the card for the group.
  return (DeleteUsers());
}

nsresult nsAbSync::AddNewGroups()
{
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase;
  PRInt32         addCount = 0;
  PRInt32         i,j;
  PRInt32         serverID;
  PRInt32         localID;
  nsCOMPtr<nsIAbCard> newCard;
  nsIAbCard       *tCard = nsnull;
  nsString        tempProtocolLine;
  PRBool          isNewCard = PR_TRUE;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mNewRecordValues->Count() % mNewRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the add count.
  addCount = mNewRecordValues->Count() / mNewRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  //
  // Create the new card that we will eventually add to the database.
  //
  for (i = 0; i < addCount; i++)
  {    
    serverID = 0;     // for safety

    // Check to see if this is a new or existing card.
    localID = HuntForExistingABEntryInServerRecord(i, aDatabase, directory, PR_TRUE, &serverID, &tCard);
    if ( (localID > 0) && (nsnull != tCard))
    {
      // This is an existing card in the local address book
      newCard = tCard;
      isNewCard = PR_FALSE;
    }
    else
    {
      // This is a new entry!
      rv = nsComponentManager::CreateInstance(NS_ABCARDPROPERTY_CONTRACTID, nsnull, NS_GET_IID(nsIAbCard), 
                                              getter_AddRefs(newCard));
      isNewCard = PR_TRUE;
    }

    if (NS_FAILED(rv) || !newCard)
    {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto EarlyExit;
    }

    // Parse info associated with the card type and store it in the new card.
    for (j = 0; j < mNewRecordTags->Count(); j++)
    {
      nsString *val = mNewRecordValues->StringAt((i*(mNewRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        // See if this is the record_id, keep it around for later...
        nsString *tagVal = mNewRecordTags->StringAt(j);
        if (tagVal->Equals(NS_LITERAL_STRING("record_id")))
        {
          PRInt32 errorCode;
          serverID = val->ToInteger(&errorCode);
        }

        // Ok, "val" could still be URL Encoded, so we need to decode it first.
        char *myTStr = ToNewCString(*val);
        if (myTStr)
        {
          char *ret = nsUnescape(myTStr);
          if (ret)
          {
            val->AssignWithConversion(ret);
            PR_FREEIF(ret);
          }
        }
        AddGroupsValueToNewCard(newCard, mNewRecordTags->StringAt(j), val);
      }
    }

    // Set default email to other email #1.
    nsString defaulEmail;
    defaulEmail.AssignWithConversion(AB_DEFAULT_EMAIL_IS_EMAIL_1);
    newCard->SetDefaultEmail(defaulEmail.get());

    // Set card type to AOL groups.
    nsString cardType;
    cardType.AssignWithConversion(AB_CARD_IS_AOL_GROUPS);
    newCard->SetCardType(cardType.get());

    // Ok, now that we are here, we should check if this is a recover from a crash situation.
    // If it is, then we should try to find the CRC for this entry in the local address book
    // and tweak a new flag if it is already there.
    //
    PRBool    cardAlreadyThere = PR_FALSE;

    // See if last sync failed.
    ulong     tempCRC;
    if (mLastSyncFailed)
      cardAlreadyThere = CardAlreadyInAddressBook(newCard, &localID, &tempCRC);

    // Ok, now we need to modify or add the card! ONLY IF ITS NOT THERE ALREADY
    if (!cardAlreadyThere)
    {
      if (!isNewCard)
        rv = aDatabase->EditCard(newCard, PR_TRUE);
      else
      {
        // hack
        // there is no way (currently) to set generic attributes on simple abcardproperties
        // so we have to use a mdbcard instead. We can remove this once #128567 is fixed.
        nsCOMPtr<nsIAbMDBCard> dbcard = do_CreateInstance(NS_ABMDBCARD_CONTRACTID, &rv);
        if (NS_FAILED(rv))
          continue;
        
        nsCOMPtr<nsIAbCard> card = do_QueryInterface(dbcard, &rv);
        if (NS_FAILED(rv))
          continue;
        
        rv = card->Copy(newCard);
        if (NS_FAILED(rv))
          continue;
        
        dbcard->SetAbDatabase(aDatabase);
        PRUint32  tID;
        // card has to be a mdbcard, or tID will come back as 0
        rv = aDatabase->CreateNewCardAndAddToDBWithKey(card, PR_TRUE, &tID);
        localID = (PRInt32) tID;
        newCard = card;
      }
    }

    // Now, calculate the CRC for the new group. Since it's mapped to a card so treat it that way.
    if (NS_FAILED(GenerateProtocolForCard(newCard, PR_FALSE, tempProtocolLine)))
      continue;

    // Get the CRC for this temp entry line...
    char *tLine = ToNewCString(tempProtocolLine);
    if (!tLine)
      continue;

    if (!isNewCard)
    {
      // Try to patch the old table if this is an old card. If that fails, then 
      // flip the flag to TRUE and have a new record created in newSyncRecord
      //
      if (NS_FAILED(PatchHistoryTableWithNewID(localID, serverID, 1, GetCRC(tLine))))
        isNewCard = PR_TRUE;
    }

    if (isNewCard)
    {
      syncMappingRecord *newSyncRecord = (syncMappingRecord *)PR_Malloc(sizeof(syncMappingRecord));
      if (newSyncRecord)
      {
        memset(newSyncRecord, 0, sizeof(syncMappingRecord));
        newSyncRecord->CRC = GetCRC(tLine);
        newSyncRecord->serverID = serverID;
        newSyncRecord->localID = localID;
        newSyncRecord->flags |= SYNC_IS_AOL_GROUPS;
        mNewServerTable->AppendElement((void *)newSyncRecord);
      }
    }

    nsCRT::free(tLine);
    newCard = nsnull;
  }

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase);  

  return rv;
}

nsresult nsAbSync::AddGroupsValueToNewCard(nsIAbCard *aCard, nsString *aTagName, nsString *aTagValue)
{
  nsresult  rv = NS_OK;

  nsString  outValue;
  char      *tValue = nsnull;

  tValue = ToNewCString(*aTagValue);
  if (tValue)
  {
    rv = nsMsgI18NConvertToUnicode(nsCAutoString("UTF-8"), nsCAutoString(tValue), outValue);
    if (NS_SUCCEEDED(rv))
      aTagValue->Assign(outValue);
    PR_FREEIF(tValue);
  }

  if (aTagName->Equals(NS_LITERAL_STRING("group_name")))
  { // Map group name to both last and display names.
    aCard->SetLastName(aTagValue->get());
    aCard->SetDisplayName(aTagValue->get());
  }
  else if (aTagName->Equals(NS_LITERAL_STRING("group_email")))
    aCard->SetPrimaryEmail(aTagValue->get());
  else if (aTagName->Equals(NS_LITERAL_STRING("group_url")))
    aCard->SetWebPage2(aTagValue->get());

  return rv;
}

nsresult nsAbSync::InitNewTables()
{
   // Create the table for new users from servers if it doesn't already exist.
  if (!mNewServerTable)
  {
    mNewServerTable = new nsVoidArray();
    if (!mNewServerTable)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  // Create the table for new lists from servers if it doesn't already exist.
  if (!mListNewServerTable)
  {
    mListNewServerTable = new nsVoidArray();
    if (!mListNewServerTable)
    {
      delete mNewServerTable;
      mNewServerTable = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

nsresult nsAbSync::AddNewMailingListEmailMembers()
{
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase;
  PRInt32         addCount = 0;
  PRInt32         i, j, cnt;
  PRInt32         serverID, listServerID, listLocalID;
  PRInt32         localID;
  nsCOMPtr<nsIAbCard> newCard;
  nsIAbCard       *tCard = nsnull;
  nsString        tempProtocolLine;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Sanity check. If the numbers don't add up, then return an error.
  if ((mNewRecordValues->Count() % mNewRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the add count.
  addCount = mNewRecordValues->Count() / mNewRecordTags->Count();

  // First open the database.
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the addrbook directory.
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  if (NS_FAILED(rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource))))
    goto EarlyExit;
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Init new tables for new users/lists.
  if (NS_FAILED(InitNewTables()))
    goto EarlyExit;

  // For each email address, create a card of type 'email address'.
  for (i = 0; i < addCount; i++)
  {    
    // Parse info associated with the card type and store it in the new card.
    nsVoidArray   memberAddresses;
    nsUInt32Array memberServerIDs;
    for (j = 0; j < mNewRecordTags->Count(); j++)
    {
      PRInt32 errorCode;
      nsString *val = mNewRecordValues->StringAt((i*(mNewRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        // There may be multiple ids for 'id_string' so parse
        // and store them in an array. Same for 'email_string'.
        nsString *tagVal = mNewRecordTags->StringAt(j);
        if (tagVal->Equals(NS_LITERAL_STRING("list_id")))
          listServerID = val->ToInteger(&errorCode);
        else if (tagVal->Equals(NS_LITERAL_STRING("id_string")))
          ParseEmailMemberIds(val, memberServerIDs);
        else if (tagVal->Equals(NS_LITERAL_STRING("email_string")))
        {
          // Ok, "val" could still be URL Encoded, so we need to decode it first.
          char *myTStr = ToNewCString(*val);
          if (myTStr)
          {
            char *ret = nsUnescape(myTStr);
            if (ret)
            {
              val->AssignWithConversion(ret);
              PR_FREEIF(ret);
            }
          }
          ParseEmailMemberAddresses(val, memberAddresses);
        }
      }
    }

    // Sanity check. If the number of email string ids != number 
    // of addresses it's most milkely a server protocol error.
    if (memberAddresses.Count() != (PRInt32) memberServerIDs.GetSize())
      continue;

    // Find the list local id (which is our db key) from server id.
    if (NS_FAILED(LocateClientIDFromServerID(listServerID, &listLocalID)))
      continue; // continue since the record has already been removed.

    // Locate the list mapping entry so we can examine its members.
    syncListMappingRecord *listRecord;
    if (NS_FAILED(LocateExistingListRecord(listLocalID, &listRecord)))
      continue;

    // Locate the card for the list in the address book.
    nsCOMPtr<nsIAbCard> listCard;
    if (NS_FAILED(FindCardByClientID(listLocalID, aDatabase, directory, getter_AddRefs(listCard))))
      continue;

    // The following two server protocol responses show that a2@abc.com (id=113)
    // is removed from list id 110:
    //
    //   110
    //   a1@abc.com,a2@abc.com,a3@abc.com
    //   112 113 114
    //
    // <2nd server response>
    //   110
    //   a1@abc.com,a3@abc.com
    //   112 114
    //
    // So the sync servers give you all the existing "additional email addresses" in
    // the protocol instead of separate add and delete cmds. This is quite different
    // from normal add/delete contacts and lists where servers give you one by one.

    // Delete all the existing email address type of cards associated with the list
    // first and then add the new ones.
    cnt = listRecord->memLocalID.GetSize();
    for (j = 0; j < cnt; j++)
      if ( (GetCardTypeByMemberId(listRecord->memLocalID.GetAt(j)) & SYNC_IS_AOL_ADDITIONAL_EMAIL) )
      {
        // Remove the member from the list first and then remove the card itself.
        // Need to reset the member server id to 0 so it's not saved to history file.
        rv = DeleteMemberFromList(aDatabase, listCard, listRecord->memLocalID.GetAt(j));
        if (NS_SUCCEEDED(rv))
        {
          rv = DeleteCardByServerID(listRecord->memServerID.GetAt(j));
          listRecord->memServerID.SetAt(j, 0);
        }
      }

    nsString emailString;
    // All we have to do here is create new cards for email addresses.
    for (j=0; j < memberAddresses.Count(); j++)
    {
      serverID = memberServerIDs.GetAt(j);
      rv = nsComponentManager::CreateInstance(NS_ABCARDPROPERTY_CONTRACTID, nsnull, NS_GET_IID(nsIAbCard), 
                                              getter_AddRefs(newCard));
      if (NS_FAILED(rv) || !newCard)
        continue;

      // Set email address. Save it for member CRC as well.
      nsString *email = (nsString *) memberAddresses.ElementAt(j);
      newCard->SetPrimaryEmail((*email).get());

      if (!emailString.IsEmpty())
        emailString.Append(NS_LITERAL_STRING(","));
      emailString.Append(*email);

      // Set default email to other email #1.
      nsString defaulEmail;
      defaulEmail.AssignWithConversion(AB_DEFAULT_EMAIL_IS_EMAIL_1);
      newCard->SetDefaultEmail(defaulEmail.get());

      // Set card type to AOL additional email address.
      nsString cardType;
      cardType.AssignWithConversion(AB_CARD_IS_AOL_ADDITIONAL_EMAIL);
      newCard->SetCardType(cardType.get());


      // hack
      // there is no way (currently) to set generic attributes on simple abcardproperties
      // so we have to use a mdbcard instead. We can remove this once #128567 is fixed.
      nsCOMPtr<nsIAbMDBCard> dbcard = do_CreateInstance(NS_ABMDBCARD_CONTRACTID, &rv);
      if (NS_FAILED(rv))
        continue;
    
      nsCOMPtr<nsIAbCard> card = do_QueryInterface(dbcard, &rv);
      if (NS_FAILED(rv))
        continue;
    
      rv = card->Copy(newCard);
      if (NS_FAILED(rv))
        continue;
    
      dbcard->SetAbDatabase(aDatabase);
      PRUint32  tID;
      // card has to be a mdbcard, or tID will come back as 0
      rv = aDatabase->CreateNewCardAndAddToDBWithKey(card, PR_TRUE, &tID);
      localID = (PRInt32) tID;
      newCard = card;

      // Now, calculate the NEW CRC for the new card.
      if (NS_FAILED(GenerateProtocolForCard(newCard, PR_FALSE, tempProtocolLine)))
        continue;

      // Get the CRC for this temp entry line...
      char *tLine = ToNewCString(tempProtocolLine);
      if (!tLine)
        continue;

      // Create a new sync mapping record for this card.
      syncMappingRecord *newSyncRecord = (syncMappingRecord *)PR_Malloc(sizeof(syncMappingRecord));
      if (newSyncRecord)
      {
        memset(newSyncRecord, 0, sizeof(syncMappingRecord));
        newSyncRecord->CRC = GetCRC(tLine);
        newSyncRecord->serverID = serverID;
        newSyncRecord->localID = localID;
        newSyncRecord->flags |= SYNC_IS_AOL_ADDITIONAL_EMAIL;
        mNewServerTable->AppendElement((void *)newSyncRecord);
      }

      // Now, add the member to the list.
      rv = AddMemberToList(listCard, newCard);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aDatabase->Commit(nsAddrDBCommitType::kLargeCommit);

      // Seems like verything is fine so add the member to the list sync table.
      if (NS_SUCCEEDED(rv))
      {
        listRecord->memLocalID.Add(localID);
        listRecord->memServerID.Add(serverID);
        listRecord->memFlags.Add(SYNC_PROCESSED);
      }
      nsCRT::free(tLine);
      newCard = nsnull;
    }

    // Now store the new CRC.
    if (emailString.IsEmpty())
      listRecord->CRC = 0;
    else
    {
      char *tVal = ToNewCString(emailString);
      listRecord->CRC = GetCRC(tVal);
      nsCRT::free(tVal);
    }

    // Free email address array
    nsString *email;
    for (j=0; j < memberAddresses.Count(); j++)
    {
      email = (nsString *) memberAddresses.ElementAt(j);
      delete email;
    }
  }

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
    aDatabase->Close(PR_TRUE);
  NS_IF_RELEASE(aDatabase);  

  return rv;
}

nsresult nsAbSync::DeleteAllEmailMemberCards(syncListMappingRecord *listRecord)
{
  nsresult rv = NS_OK;
  PRUint32 cnt = listRecord->memLocalID.GetSize();
  for (PRUint32 i = 0; i < cnt; i++)
    if ( (GetCardTypeByMemberId(listRecord->memLocalID.GetAt(i)) & SYNC_IS_AOL_ADDITIONAL_EMAIL) )
    {
      rv |= DeleteCardByServerID(listRecord->memServerID.GetAt(i));
      listRecord->memServerID.SetAt(i, 0);
    }
  return rv;
}

// Input id string is separated by a single space like:
//   112 113 114
nsresult nsAbSync::ParseEmailMemberIds(nsString *idString, nsUInt32Array &memberServerIDs)
{
  if (idString->IsEmpty())
    return NS_OK;

  // Strip off while space (especailly leading & trailing ones)
  idString->Trim("\b\t\r\n ");

  char *str = ToNewCString(*idString);
  if (str)
  {
    char *pChar, *PCurPos, *end;

    pChar = PCurPos = str;
    end = str + idString->Length();
    while (pChar < end)
    {
      // Find separated ids.
      pChar = PL_strchr(PCurPos, ' ');
      if (pChar)
        *pChar = 0;
      memberServerIDs.Add(atoi(PCurPos));
      if (! pChar)
        break;  // we're done
      pChar++;
      PCurPos = pChar;
    }
    PR_FREEIF(str);
  }
  return NS_OK;
}

// Input email address string is separated by a single comma like:
//   a1@abc.com,a2@abc.com,a3@abc.com
nsresult nsAbSync::ParseEmailMemberAddresses(nsString *addrString, nsVoidArray &memberAddresses)
{
  if (addrString->IsEmpty())
    return NS_OK;

  // Strip off while space (especailly leading & trailing ones)
  addrString->Trim("\b\t\r\n ");

  nsString  outValue;
  char *tValue = ToNewCString(*addrString);
  if (tValue)
  {
    nsresult rv = nsMsgI18NConvertToUnicode(nsCAutoString("UTF-8"), nsCAutoString(tValue), outValue);
    NS_ENSURE_SUCCESS(rv,rv);

    addrString->Assign(outValue);

    nsString *email;
    PRInt32 atPos;
    while ((atPos = addrString->FindChar(',')) != -1)
    {
      email = new nsString();
		  addrString->Left(*email, atPos);
      memberAddresses.AppendElement((void *)email);
		  addrString->Cut(0, atPos+1);
	  }

    if (!addrString->IsEmpty())
    {
      email = new nsString();
      *email = *addrString;
      memberAddresses.AppendElement((void *)email);
    }

    PR_FREEIF(tValue);
  }
  return NS_OK;
}

