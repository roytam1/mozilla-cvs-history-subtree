/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Transaction Manager.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   John Gaunt <jgaunt@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "plstr.h"
#include "tmTransaction.h"
#include "tmTransactionService.h"
#include "tmUtils.h"

struct waiting_msg {
  tmTransaction* trans;   // a transaction waiting to be sent to a queue
  char* queueName;        // the short queue name
};

struct queue_mapping {
  PRUint32 queueID;       // the ID in the Transaction Module -- returned to us after attaching
  char* queueName;        // used by the consumers of this service
  char* joinedQueueName;  // used by the service -- profile + queue names
};

//////////////////////////////////////////////////////////////////////////////
// Constructor and Destructor

tmTransactionService::~tmTransactionService() {

  // just destroy this, it contains 2 pointers it doesn't own.
  PL_HashTableDestroy(mObservers);

  mWaitingMessages.Iterate();
  waiting_msg* msg = nsnull;
  while ((msg = (waiting_msg *)mWaitingMessages.Next())) {
    // clean up the struct
    PL_strfree(msg->queueName);
    delete msg->trans;
    delete msg;
  }

  mQueueMaps.Iterate();
  queue_mapping *qmap = nsnull;
  while ((qmap = (queue_mapping *)mQueueMaps.Next())) {
    // clean up the struct
    PL_strfree(qmap->queueName);
    PL_strfree(qmap->joinedQueueName);
    delete qmap;
  }
}

//////////////////////////////////////////////////////////////////////////////
// ISupports

NS_IMPL_ISUPPORTS1(tmTransactionService, tmITransactionService)

//////////////////////////////////////////////////////////////////////////////
// tmITransactionService

NS_IMETHODIMP
tmTransactionService::Init(const nsACString & aProfileName) {

  // register with the IPC service
  nsCOMPtr<ipcIService> ipcService(do_GetService("@mozilla.org/ipc/service;1"));
  if (!ipcService)
    return NS_ERROR_FAILURE;
  ipcService->SetMessageObserver(kTransModuleID, this);

  // create some internal storage
  mObservers = PL_NewHashTable(20, 
                               PL_HashString, 
                               PL_CompareStrings, 
                               PL_CompareValues, 0, 0);
  if (!mObservers)
    return NS_ERROR_FAILURE;

  // init some internal storage
  mQueueMaps.Init();
  mWaitingMessages.Init();

  // store the profile name
  mProfileName.Assign(aProfileName);
  return NS_OK;
}

NS_IMETHODIMP
tmTransactionService::Attach(const nsACString & aQueueName, 
                             tmITransactionObserver *aObserver) {

  nsresult rv = NS_ERROR_OUT_OF_MEMORY;  // prime the return value

  // if the queue already exists, then someone else is attached to it. must
  //   return an error here. Only one module attached to a queue per app.
  if (NS_SUCCEEDED(GetQueueID(aQueueName)))
    return TM_ERROR_QUEUE_EXISTS;

  // create the full queue name: profile + queue
  nsCString jQName;
  jQName.Assign(mProfileName);
  jQName.Append(aQueueName);

  // this char* has two homes, make sure it gets PL_free()ed properly
  char* joinedQueueName = PL_strdup(PromiseFlatCString(jQName).get());

  // link the observer to the queuename.  home #1 for joinedQueueName
  // these currently don't get removed until the destructor on this is called.
  PL_HashTableAdd(mObservers, joinedQueueName, (void*)aObserver);

  // store the queuename, create a place to store the ID
  queue_mapping* qm = new queue_mapping();
  if (!qm)
    return NS_ERROR_OUT_OF_MEMORY;
  qm->queueID = TM_INVALID_ID;              // initially no ID for the queue
  qm->joinedQueueName = joinedQueueName;    // home #2 for joinedQueueName
  qm->queueName = PL_strdup(PromiseFlatCString(aQueueName).get());
  mQueueMaps.Add((void *)qm);

  tmTransaction *trans = new tmTransaction();
  if (trans) {
    rv = trans->Init(0,                             // no IPC client
                     TM_INVALID_ID,                 // qID gets returned to us
                     TM_ATTACH,                     // action
                     NS_OK,                         // default status
                     (PRUint8 *)joinedQueueName,    // qName gets copied
                     PL_strlen(joinedQueueName)+1); // message length

    // send the attach msg
    if (NS_SUCCEEDED(rv))
      SendMessage(trans, PR_TRUE);
    delete trans;  
  }
  return rv;
}

// actual removal of the observer takes place when we get the detach reply
NS_IMETHODIMP
tmTransactionService::Detach(const nsACString & aQueueName) {
  nsresult rv = NS_ERROR_OUT_OF_MEMORY; // prime the return value

  tmTransaction *trans = new tmTransaction();
  if (trans) {
    rv = trans->Init(0,                      // no IPC client
                     GetQueueID(aQueueName), // qID to detach from
                     TM_DETACH,              // action
                     NS_OK);                 // default status

    if (NS_SUCCEEDED(rv)) {
      if (trans->GetQueueID() == TM_INVALID_ID){
        // stack it and pack it
        waiting_msg *msg = new waiting_msg();
        if (!msg)
          return NS_ERROR_FAILURE;
        msg->trans = trans;
        msg->queueName = PL_strdup(PromiseFlatCString(aQueueName).get());
        mWaitingMessages.Add((void *)msg);
      }
      else {
        // send it
        SendMessage(trans, PR_FALSE);
      }
    }
    delete trans;
  }
  return rv;
}

NS_IMETHODIMP
tmTransactionService::Flush(const nsACString & aQueueName) {
  nsresult rv = NS_ERROR_OUT_OF_MEMORY; // prime the return value

  tmTransaction *trans = new tmTransaction();
  if (trans) {
    rv = trans->Init(0,                      // no IPC client
                     GetQueueID(aQueueName), // qID to detach from
                     TM_FLUSH,               // action
                     NS_OK);                 // default status

    if (NS_SUCCEEDED(rv)) {
      if (trans->GetQueueID() == TM_INVALID_ID){
        // stack it and pack it
        waiting_msg *msg = new waiting_msg();
        if (!msg)
          return NS_ERROR_FAILURE;
        msg->trans = trans;
        msg->queueName = PL_strdup(PromiseFlatCString(aQueueName).get());
        mWaitingMessages.Add((void *)msg);
      }
      else {
        // send it
        SendMessage(trans, PR_TRUE);
      }
    }
    delete trans;
  }
  return rv;
}

NS_IMETHODIMP
tmTransactionService::PostTransaction(const nsACString & aQueueName, 
                                      const PRUint8 *aData, 
                                      PRUint32 aDataLen) {

  nsresult rv = NS_ERROR_OUT_OF_MEMORY; // prime the return value

  tmTransaction *trans = new tmTransaction();

  if (trans) {
    rv = trans->Init(0,                 // no IPC client
                GetQueueID(aQueueName), // qID returned to us
                TM_POST,                // action
                NS_OK,                  // default status
                aData,                  // message data
                aDataLen);              // message length

    if (NS_SUCCEEDED(rv)) {
      if (trans->GetQueueID() == TM_INVALID_ID){
        printf("PostTransaction - stack and pack\n");
        // stack it and pack it
        waiting_msg *msg = new waiting_msg(); 
        if (!msg)
          return NS_ERROR_FAILURE;
        msg->trans = trans;
        msg->queueName = PL_strdup(PromiseFlatCString(aQueueName).get());
        mWaitingMessages.Add((void *)msg);
      }
      else {
        // send it
        printf("PostTransaction - sending it right away\n");
        SendMessage(trans, PR_FALSE);
        delete trans;
      }
    }
  }
  return rv;
}

//////////////////////////////////////////////////////////////////////////////
// ipcIMessageObserver

NS_IMETHODIMP
tmTransactionService::OnMessageAvailable(const nsID & aTarget, 
                                         const PRUint8 *aData, 
                                         PRUint32 aDataLength) {

  nsresult rv = NS_ERROR_OUT_OF_MEMORY; // prime the return value

  tmTransaction *trans = new tmTransaction();
  if (trans) {
    rv = trans->Init(0,                      // no IPC client ID
                     aData,                  // message data
                     aDataLength);           // message length

    if (NS_SUCCEEDED(rv)) {
      switch(trans->GetAction()) {
      case TM_ATTACH_REPLY:
        OnAttachReply(trans);
        break;
      case TM_POST_REPLY:
        // OnPostReply() would be called here
        //   isn't neccessary at the current time 2/19/03
        break;
      case TM_DETACH_REPLY:
        OnDetachReply(trans);
        break;
      case TM_FLUSH_REPLY:
        OnFlushReply(trans);
        break;
      case TM_POST:
        OnPost(trans);
        break;
      default: // error, should not happen
        NS_ASSERTION(0, "Recieved a TM reply outside of mapped messages");
        break;
      }
    }
    delete trans;
  }
  return rv;
}

//////////////////////////////////////////////////////////////////////////////
// Protected Member Functions

void
tmTransactionService::SendMessage(tmTransaction *aTrans, PRBool aSync) {

  if (!aTrans)
    return;

  nsCOMPtr<ipcIService> ipcService(do_GetService(
                                   "@mozilla.org/ipc/service;1"));
  NS_ASSERTION(ipcService, "Failed to get the ipcService");

  if (ipcService)
    ipcService->SendMessage(0, 
                            kTransModuleID, 
                            aTrans->GetRawMessage(), 
                            aTrans->GetRawMessageLength(),
                            aSync);
}

nsresult
tmTransactionService::OnAttachReply(tmTransaction *aTrans) {

  // if we attached, store the queue's ID
  if (NS_SUCCEEDED(aTrans->GetStatus())) {

    mQueueMaps.Iterate();
    queue_mapping* qmap = nsnull;
    while ((qmap = (queue_mapping *)mQueueMaps.Next())) {
      if (qmap && 
          PL_strcmp(qmap->joinedQueueName, (char*)aTrans->GetMessage()) == 0) {

        // set the ID in the mapping
        qmap->queueID = aTrans->GetQueueID();
        // send any stored messges to the queue
        DispatchStoredMessages(qmap);
      }
    }
  }

  // notify the observer we have attached
  tmITransactionObserver *observer = 
    (tmITransactionObserver *)PL_HashTableLookup(mObservers, 
                                                 (char*)aTrans->GetMessage());
  if (observer)
    observer->OnAttachReply(aTrans->GetQueueID(), aTrans->GetStatus());
  return NS_OK;
}

nsresult
tmTransactionService::OnDetachReply(tmTransaction *aTrans) {

  queue_mapping *qmap = GetQueueMap(aTrans->GetQueueID());

  // get the observer before we release the hashtable entry
  tmITransactionObserver *observer = 
    (tmITransactionObserver *)PL_HashTableLookup(mObservers, 
                                                 qmap->joinedQueueName);

  // if it was removed, clean up
  if (NS_SUCCEEDED(aTrans->GetStatus())) {

    // remove the link between observer and queue
    PL_HashTableRemove(mObservers, qmap->joinedQueueName);

    // remove the mapping of queue names and id
    mQueueMaps.Remove(qmap);
    PL_strfree(qmap->queueName);
    PL_strfree(qmap->joinedQueueName);
    delete qmap;
  }


  // notify the observer -- could be didn't detach
  if (observer)
    observer->OnDetachReply(aTrans->GetQueueID(), aTrans->GetStatus());
  return NS_OK;
}

nsresult
tmTransactionService::OnFlushReply(tmTransaction *aTrans) {

  tmITransactionObserver *observer = 
    (tmITransactionObserver *)PL_HashTableLookup(mObservers, 
                              GetJoinedQueueName(aTrans->GetQueueID()));
  if (observer)
    observer->OnFlushReply(aTrans->GetQueueID(), aTrans->GetStatus());
  return NS_OK;
}

nsresult
tmTransactionService::OnPost(tmTransaction *aTrans) {

  tmITransactionObserver *observer = 
    (tmITransactionObserver *)PL_HashTableLookup(mObservers, 
                              GetJoinedQueueName(aTrans->GetQueueID()));
  if (observer)
    observer->OnTransactionAvailable(aTrans->GetQueueID(), 
                                     aTrans->GetMessage(), 
                                     aTrans->GetMessageLength());
  return NS_OK;
}

nsresult
tmTransactionService::DispatchStoredMessages(queue_mapping *aQMapping) {
  printf("DispatchStoredMessages\n");

  if (!aQMapping)
    return NS_ERROR_INVALID_ARG;

  mWaitingMessages.Iterate();
  waiting_msg *msg = nsnull;
  while ((msg = (waiting_msg *)mWaitingMessages.Next())) {
    // if the message is waiting on the queue passed in
    if (PL_strcmp(aQMapping->queueName, msg->queueName) == 0) {

      // found a match, send it and remove
      msg->trans->SetQueueID(aQMapping->queueID);
      SendMessage(msg->trans, PR_FALSE);
      mWaitingMessages.Remove((void *)msg);

      // clean up the struct
      PL_strfree(msg->queueName);
      delete msg->trans;
      delete msg;
    }
  }
  return NS_OK;
}

// searches against the short queue name
PRUint32
tmTransactionService::GetQueueID(const nsACString & aQueueName) {

  mQueueMaps.Iterate();
  queue_mapping* qmap = nsnull;
  while ((qmap = (queue_mapping *)mQueueMaps.Next())) {
    if (qmap && 
        PL_strcmp(qmap->queueName, PromiseFlatCString(aQueueName).get()) == 0)
      return qmap->queueID;
  }
  return TM_INVALID_ID;
}

char*
tmTransactionService::GetJoinedQueueName(PRUint32 aQueueID) {

  mQueueMaps.Iterate();
  queue_mapping* qmap = nsnull;
  while ((qmap = (queue_mapping *)mQueueMaps.Next())) {
    if (qmap && qmap->queueID == aQueueID)
      return qmap->joinedQueueName;
  }
  return nsnull;
}

queue_mapping*
tmTransactionService::GetQueueMap(PRUint32 aQueueID) {

  mQueueMaps.Iterate();
  queue_mapping* qmap = nsnull;
  while ((qmap = (queue_mapping *)mQueueMaps.Next())) {
    if (qmap && qmap->queueID == aQueueID)
      return qmap;
  }
  return nsnull;
}

