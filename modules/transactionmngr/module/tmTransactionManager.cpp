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

#include "plstr.h"
#include <stdlib.h>
#include "tmIPCModule.h"
#include "tmQueue.h"
#include "tmTransactionManager.h"
#include "tmTransaction.h"
#include "tmUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Constructors & Destructor & Initializer

tmTransactionManager::~tmTransactionManager() {

  mQueues.Iterate();
  tmQueue *queue = nsnull;
  while(queue = (tmQueue *)mQueues.Next()) {
    mQueues.RemoveAt(mQueues.GetIterator());
    delete queue;
  }
}

nsresult
tmTransactionManager::Init() {
  return mQueues.Init();
}

///////////////////////////////////////////////////////////////////////////////
// public transaction module methods

void
tmTransactionManager::HandleTransaction(tmTransaction *aTrans) {
  printf("TM::HandleTransaction\n");

  if (!aTrans)
    return;

  nsresult rv = NS_OK;
  PRUint32 action = aTrans->GetAction();
  PRUint32 ownerID = aTrans->GetOwnerID();
  tmQueue *queue = nsnull;

  // get the right queue -- attaches do it differently
  if (action == TM_ATTACH) {
    char *name = (char *)aTrans->GetMessage(); // is qName for Attaches
    queue = GetQueue(name);  
    if (!queue) {
      rv = AddQueue(name);
      if (NS_SUCCEEDED(rv))
        queue = GetQueue(NS_ERROR_GET_CODE(rv));
    }
  }
  else  // all other trans should have a queue ID already
    queue = GetQueue(aTrans->GetQueueID());

  if (!queue)    // Trying to post to a tmQueue that doesn't exist!!!
    return;

  // All possible actions should have a case, default is not valid
  //   delete trans when done with them, let the queue own the trans
  //   that are posted to them.
  switch (action) {
  case TM_ATTACH:
    queue->AttachClient(ownerID);
    delete aTrans;
    break;
  case TM_POST:
    queue->PostTransaction(aTrans);
    break;
  case TM_FLUSH:
    queue->FlushQueue(ownerID);
    delete aTrans;
    break;
  case TM_DETACH:
    if (queue->DetachClient(ownerID) == TM_SUCCESS_DELETE_QUEUE)
      RemoveQueue(aTrans->GetQueueID());
    delete aTrans;
    break;
  default:
    PR_ASSERT(0);               // Bad action in the transaction
  }
}

// called from tmQueue
void
tmTransactionManager::SendTransaction(PRUint32 aDestClientIPCID, 
                                      tmTransaction *aTrans) {
  if (aDestClientIPCID < 0 || !aTrans)
    return;

  printf("TM::SendTransaction(to client)\n");

  tmIPCModule::SendMsg(aDestClientIPCID, aTrans);
}

///////////////////////////////////////////////////////////////////////////////
// Protected member functions

//
// Queue Handling
//

// the queueID is the index in the vector where the queue is stored
tmQueue*
tmTransactionManager::GetQueue(PRUint32 aQueueID) {
  return (tmQueue *) (mQueues.ElementAt(aQueueID));
}

tmQueue*
tmTransactionManager::GetQueue(const char *aQueueName) {

  mQueues.Iterate();
  tmQueue *queue = nsnull;
  while(queue = (tmQueue *)mQueues.Next()) {
    if (PL_strcmp(queue->GetName(), aQueueName) == 0)
      return queue;
  }
  return nsnull;
}

// if successful the nsresult contains the index of the added queue
nsresult
tmTransactionManager::AddQueue(const char *aQueueType) {

  tmQueue* foo = new tmQueue();
  nsresult rv = mQueues.Add((void*)foo);  // null safe call
  if (foo && NS_FAILED(rv))
    delete foo;
  else if (NS_SUCCEEDED(rv)) // implicitly foo exists
    foo->Init(aQueueType, NS_ERROR_GET_CODE(rv), this);
  return rv;
}

nsresult
tmTransactionManager::RemoveQueue(PRUint32 aQueueID) {

  tmQueue *queue = (tmQueue *)mQueues.ElementAt(aQueueID);
  nsresult rv = mQueues.RemoveAt(aQueueID);
  if (NS_SUCCEEDED(rv))
    delete queue;
  return rv;
}
