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

#ifndef _tmQueue_H_
#define _tmQueue_H_

#include "tmUtils.h"
#include "tmVector.h"

class tmClient;
class tmTransaction;
class tmTransactionManager;

/**
  * This class isn't so much a queue as it is storage for transactions. It
  *   is set up to recieve and store transactions in a growing collection
  *   (using tmVectors). Different messages can be recieved from the 
  *   Transaction Manager(TM) the queue belongs to which can add and remove
  *   listeners, empty the queue (flush), and add messages to the queue.
  *
  * See the documentation in tmTransactionService.h for details on the
  *   messages you can send to and recieve from the queues in the TM
  */
class tmQueue
{

public:

  ////////////////////////////////////////////////////////////////////////////
  // Constructor & Destructor

  /**
    * Set the internal state to default values. Init() must be called 
    *   after construction to allocate the storage and set the name and ID.
    */
  tmQueue(): mID(0), mName(nsnull), mTM(nsnull) {;}

  /**
    * Reclaim the memory allocated in Init(). Destroys the transactions in
    *   the transaction storage and the ids in the listener storage
    */
  virtual ~tmQueue();

  ////////////////////////////////////////////////////////////////////////////
  // Public Member Functions

  /**
    * Initialize internal storage vectors and set the name of the queue
    *   and the pointer to the TM container.
    *
    * @returns NS_OK if everything succeeds
    * @returns NS_ERROR_FAILURE if the vector initializations fail or the
    *          allocation for the name fails.
    */
  nsresult Init(const char* aName, PRUint32 aID, tmTransactionManager *aTM);

  // Queue Operations

  /**
    * Adds the clientID to the list of queue listeners. A reply is created
    *   and sent to the client. The reply contains both the name of the
    *   queue and the id, so the client can match the id to the name and
    *   then use the id in all further communications to the queue.
    *
    * The reply is sent for all cases.
    *
    * @returns NS_ERRROR_INVALID_ARG if there were problems creating storage
    *          for the clientID
    * @returns NS_ERROR_OUT_OF_MEMORY if the collection needed to grow and
    *          the allocation of a new backing store failed.
    * @returns NS_ERROR_GENERATE_SUCCESS(index) if the add was successful.
    *          The calling method can get the index of the element added by
    *          using the macro NS_ERROR_GET_CODE() on the return value. But 
    *          for simple success test use NS_SUCCEEDED() on the return value.
    */
  nsresult AttachClient(PRUint32 aClientID);

  /**
    * Removes the client from the list of queue listeners. A reply is created
    *   and sent to the client to indicate the success of the removal.
    *
    * The reply is sent for all cases.
    *
    * @returns NS_OK on success
    * @returns NS_ERROR_INVALID_ARG if client didn't map to an ID that was
    *          in the range of the vector. This is bad news, indicates
    *          inconsistant state between the queue and it's storage.
    */
  nsresult DetachClient(PRUint32 aClientID);

  /**
    * Removes all the transactions being held in the queue, creating an empty
    *   queue. A reply is created and sent to the client to indicate the
    *   completion of the operation.
    *
    * The reply is sent for all cases.
    *
    * @returns NS_OK
    */
  nsresult FlushQueue(PRUint32 aClientID);

  /**
    * Places the transaction passed in on the queue. Takes ownership of the
    *   transaction, deletes it in the destructor. A reply is created and
    *   sent to the client to indicate the success of the posting of the
    *   transaction.
    *
    * The reply is sent for all cases except INVALID_ARG (no destination)
    *
    * @returns NS_ERRROR_INVALID_ARG if the transaction is null
    * @returns TM_ERROR_WRONG_QUEUE if the transaction has been sent to the
    *          wrong queue.
    * @returns NS_ERROR_OUT_OF_MEMORY if the collection needed to grow and
    *          the allocation of a new backing store failed.
    * @returns NS_ERROR_GENERATE_SUCCESS(index) if the add was successful.
    *          The calling method can get the index of the element added by
    *          using the macro NS_ERROR_GET_CODE() on the return value. But 
    *          for simple success test use NS_SUCCEEDED() on the return value.
    */
  nsresult PostTransaction(tmTransaction *aTrans);

  // Accessors

  /**
    * @returns the ID of the queue
    */
  PRUint32 GetID() const { return mID; }

  /**
    * @returns the name of the queue
    */
  const char* GetName() const { return mName; }

protected:

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Variables

  // storage
  tmVector mTransactions;     // transactions that have been posted
  tmVector mListeners;        // programs listening to this queue

  // bookkeeping
  PRUint32 mID;               // a number linked to the name in the mTM
  char *mName;                // format: [profilename][domainname(ie prefs)]
  tmTransactionManager *mTM;  // the container that holds the queue

};

#endif
