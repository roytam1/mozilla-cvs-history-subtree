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
#include "tmQueue.h"
#include "tmTransaction.h"
#include "tmTransactionManager.h"
#include "tmUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Constructors & Destructor

tmQueue::~tmQueue() {

  // empty the vectors
  mTransactions.Iterate();
  tmTransaction *trans = nsnull;
  while(trans = (tmTransaction *)mTransactions.Next())
    delete trans;

  mListeners.Iterate();
  PRUint32 *id = nsnull;
  while(id = (PRUint32 *)mListeners.Next())
    delete id;

  mTM = nsnull;
  mID = 0;
  if (mName)
    PL_strfree(mName);
}

///////////////////////////////////////////////////////////////////////////////
// Public Methods

nsresult
tmQueue::Init(const char* aName, PRUint32 aID, tmTransactionManager *aTM) {

  if (NS_SUCCEEDED(mTransactions.Init()) &&
      NS_SUCCEEDED(mListeners.Init()) &&
      ((mName = PL_strdup(aName)) != nsnull) ) {
    mTM = aTM;
    mID = aID;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
tmQueue::AttachClient(PRUint32 aClientID) {

  nsresult status = NS_OK;                 // success of adding client
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;    // success of creating reply trans.

  // add the client to the listener list -- null safe call
  status = mListeners.Add((void *) new int(aClientID));

  // create a reply transaction
  tmTransaction *trans = new tmTransaction();
  if(trans) {
    rv = trans->Init(aClientID,          // ipc ID
                     mID,                // give the client our ID
                     TM_ATTACH_REPLY,    // action
                     status,             // success of the add
                     (PRUint8 *)mName,   // client needs name to match ID
                     PL_strlen(mName)+1);

    // send the reply
    if (NS_SUCCEEDED(rv))
      mTM->SendTransaction(aClientID, trans);
    delete trans;
  }

  // if we successfully added the client - send all current transactions
  if (NS_SUCCEEDED(status)) {
    mTransactions.Iterate();
    tmTransaction *trans = nsnull;
    while(trans = (tmTransaction *)mTransactions.Next())
      mTM->SendTransaction(aClientID, trans);
  }
  return status;
}

nsresult
tmQueue::DetachClient(PRUint32 aClientID) {

  nsresult status = NS_OK;

  mListeners.Iterate();
  PRUint32 *id = nsnull;
  while(id = (PRUint32 *)mListeners.Next()) {
    if(*id == aClientID) {
      status = mListeners.RemoveAt(mListeners.GetIterator());
      if (NS_SUCCEEDED(status))
        delete id;  // the storage in mListeners was created with new
      break;        // break even if we failed, we found the right id
    }
  }

  tmTransaction* trans = new tmTransaction();
  if (trans) {
    if (NS_SUCCEEDED(trans->Init(aClientID, mID, TM_DETACH_REPLY, status)))
      mTM->SendTransaction(aClientID, trans);
    delete trans;
  }

  // if we've removed all the listeners, remove the queue.
  if (mListeners.Size() == 0)
    status = TM_SUCCESS_DELETE_QUEUE;

  return status;
}

// XXX should we check to make sure the client id supplied is listening?
//     for some added security.
nsresult
tmQueue::FlushQueue(PRUint32 aClientID) {

  mTransactions.Iterate();
  tmTransaction *trans = nsnull;
  while(trans = (tmTransaction *)mTransactions.Next())
    delete trans;

  mTransactions.Clear();  // sets the vector's slots to nsnull

  trans = new tmTransaction();
  if (trans) {
    if (NS_SUCCEEDED(trans->Init(aClientID, mID, TM_FLUSH_REPLY, NS_OK)))
      mTM->SendTransaction(aClientID, trans);
    delete trans;
  }
  return NS_OK;
}

// XXX should we check to make sure the client id supplied is listening?
//     for some added security.
nsresult
tmQueue::PostTransaction(tmTransaction *aTrans) {

  if (!aTrans)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = NS_OK;
  nsresult status = NS_OK;
  PRUint32 ownerID = aTrans->GetOwnerID();

  // wrong queue
  if (aTrans->GetQueueID() != mID) {
    status = TM_ERROR_NOT_POSTED;
    rv = TM_ERROR_WRONG_QUEUE;
  }

  if (NS_SUCCEEDED(rv))
    status = mTransactions.Add((void *)aTrans);

  if (NS_SUCCEEDED(status)) {
    // send the transaction to all members of mListeners except the owner
    mListeners.Iterate();
    PRUint32 *id = nsnull;
    while(id = (PRUint32 *)mListeners.Next()) {
      printf("Q::PostTransaction - sending to others\n");
      if (ownerID != *id)
        mTM->SendTransaction(*id, aTrans);
    }
  }

  tmTransaction* trans = new tmTransaction();
  if (trans) {
    if (NS_SUCCEEDED(trans->Init(ownerID, mID, TM_POST_REPLY, status)))
      mTM->SendTransaction(ownerID, trans);
    delete trans;
  }
  return status;
}
