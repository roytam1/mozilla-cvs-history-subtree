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

#ifndef _tmTransactionService_H_
#define _tmTransactionService_H_

#include "ipcIService.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "plhash.h"
#include "tmITransactionService.h"
#include "tmTransaction.h"
#include "tmVector.h"

// from tmTransactionManager.h
//
// XXX documentation needs work
//////////////////////////////////////////////////////////////////////////////
//  Overview of TransactionManager IPC Module
//
// Classes:
//    tmIPCModule - From the tmTransactionManager's point of view, this
//        is a proxy for the IPC daemon itself. The reverse is true
//        from the daemon's point of view. This is an interface for the
//        Transaction system to work with the IPC daemon as its transport
//        layer.
//    tmTransactionManager (TM) - Manages the different queues. Maintains
//        the queues neccessary for different clients. Receives messages
//        from the tmIPCModule and passes message to the IPC daemon through
//        the tmIPCModule.
//    tmQueue - this class manages the transactions for the different areas
//        of the profiles being shared. Broken down by functional area there
//        will be  a queue for prefs, cookies etc, but not for profileA and 
//        profileB, and not for pref_delete, pref_create, pref_change etc...
//    tmTransaction - the actual transaction being shared with the different
//        tmClients. It contains the type of transaction, which will equate with
//        a type of queue in existance, the owner of the transaction (an IPC daemon ID)
//        and the actual text message to be shared. 
//
//////////////////////////////////////////////////////////////////////////////

/// XXX some docs that need to be put somewhere:
//
// from tmqueue.cpp
// Docs - note that the status of the TM_ATTACH_REPLY is only for checking
//            for TM_ERROR_FAILURE. Other numbers have no importance
// success of the status means the NS_ERROR_GET_CODE(status) will 
//     yield the index of the listener.
//
// move to documentation page - from tmqueue.h
//
// a queue is specific to profile
//
// UUID going out from the module is a handler in the client 
//          (will go to the XPCOM service impling that UUID)
//   -- does it make sense to have different UUIDs for cookies/prefs/etc
//


struct queue_mapping;

// This must match EXACTLY the ID in tmICPModule.h
// UUID used to identify the Transaction Module in both daemon and client
#define TRANSACTION_MODULE_ID                         \
{ /* c3dfbcd5-f51d-420b-abf4-3bae445b96a9 */          \
    0xc3dfbcd5,                                       \
    0xf51d,                                           \
    0x420b,                                           \
    {0xab, 0xf4, 0x3b, 0xae, 0x44, 0x5b, 0x96, 0xa9}  \
}

static const nsID kTransModuleID = TRANSACTION_MODULE_ID;


/**
  *
  */
class tmTransactionService : public tmITransactionService,
                             public ipcIMessageObserver
{

public:

  ////////////////////////////////////////////////////////////////////////////
  // Constructor & Destructor

  /**
    * Reclaim all the memory allocated: PL_hashtable, nsvoidarrays
    */
  virtual ~tmTransactionService();



  ////////////////////////////////////////////////////////////////////////////
  // Interface Declarations

  NS_DECL_ISUPPORTS

  // ipcIMessageObserver

  /**
    * The transaction is constructed from the raw data and passed on to the
    *   proper handler.
    *
    * @returns NS_OK if everything was successful
    * @returns NS_ERROR_INVALID_ARG if there was a problem creating the
    *          transaction
    * @returns NS_ERROR_OUT_OF_MEMORY if allocation of space for the
    *          copy of the message fails.
    */
  NS_IMETHOD OnMessageAvailable(const nsID & aTarget, 
                                const PRUint8 *aData, 
                                PRUint32 aDataLength);

  // tmITransactionService

  /**
    * This gets the IPC Service and registers as a listener for transaction
    *   IPC messages. Some internal storage is created and the profile name
    *   that we are interested in is cached.
    *
    * @returns NS_OK if all goes well
    * @returns NS_ERROR_FAILURE if we are unable to get the IPC Service or
    *          if we fail to initialize the hashtable
    */
  NS_IMETHOD Init(const nsACString & aProfileName);

  /**
    * Sends a message to attach to the queue named by the arg passed in and
    *   link those transactions to the observer passed in.
    *
    * @returns NS_OK if everything goes well and the attach message is sent
    * @returns TM_ERROR_QUEUE_EXISTS if the queue already exists which means
    *          someone has already attached to it.
    * @returns NS_ERROR_OUT_OF_MEMORY if any of the various allocations fail
    * @returns NS_ERROR_INVALID_ARG if there was a problem creating the
    *          transaction
    */
  NS_IMETHOD Attach(const nsACString & aQueueName, 
                    tmITransactionObserver *aObserver);

  /**
    * Sends a message to remove the listener from the queue named by the arg
    *   passed in. Actual detachment locally doesn't happen until the
    *   DETACH_REPLY message is recieved. If the queue named has not been
    *   attached to yet this message is queued until the ATTACH_REPLY is
    *   received for that queue.
    *
    * @returns NS_OK if everything goes well and the detach message is sent
    * @returns NS_ERROR_OUT_OF_MEMORY if any of the various allocations fail
    * @returns NS_ERROR_INVALID_ARG if there was a problem creating the
    *          transaction
    */
  NS_IMETHOD Detach(const nsACString & aQueueName);

  /**
    * Sends a message to flush the queue named by the arg, removing all
    *   transactions from that queue. A client should not assume the queue
    *   has been flushed until receiving the FLUSH_REPLY message. If the 
    *   queue named has not been attached to yet this message is queued
    *   until the ATTACH_REPLY is received for that queue.
    *
    * @returns NS_OK if everything goes well and the detach message is sent
    * @returns NS_ERROR_OUT_OF_MEMORY if any of the various allocations fail
    * @returns NS_ERROR_INVALID_ARG if there was a problem creating the
    *          transaction
   */
  NS_IMETHOD Flush(const nsACString & aQueueName);

  /**
    * A transaction is created to add to the queue named in the arg. The
    *   Transaction Manager will send a POST_REPLY message to indicate the
    *   success of posting the message to the queue. If the 
    *   queue named has not been attached to yet this message is queued
    *   until the ATTACH_REPLY is received for that queue.
    * 
    * @returns NS_OK if everything goes well and the post is sent.
    * @returns NS_ERROR_OUT_OF_MEMORY if any of the various allocations fail
    * @returns NS_ERROR_INVALID_ARG if there was a problem creating the
    *          transaction
    */
  NS_IMETHOD PostTransaction(const nsACString & aQueueName, 
                             const PRUint8 *aData, 
                             PRUint32 aDataLen);

protected:

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Functions

  /**
    * Pulls the raw message out of the transaction and sends it to the IPC
    *   service to be delivered to the TM.
    */
  void SendMessage(tmTransaction *aTrans);

  // handlers for reply messages from TransactionManager

  /**
    * Pulls the queueID out of the ATTACH_REPLY message and stores it in the
    *   proper queue_mapping object. Calls DispatchStoredMessages() to make
    *   sure we send any messages that have been waiting on the ATTACH_REPLY.
    *   Also calls the OnAttachReply() method for the observer of the queue.
    *
    * @returns NS_OK
    */
  nsresult OnAttachReply(tmTransaction *aTrans);

  /**
    * Removes the queue_mapping object and calls the OnDetachReply() method
    *   on the observer of the queue detached.
    *
    * @returns NS_OK
    */
  nsresult OnDetachReply(tmTransaction *aTrans);

  /**
    * Calls the OnFlushReply method of the observer of the queue.
    *
    * @returns NS_OK
    */
  nsresult OnFlushReply(tmTransaction *aTrans);

  /**
    * Calls the OnPost method of the observer of the queue.
    *
    * @returns NS_OK
    */
  nsresult OnPost(tmTransaction *aTrans);

  // other helper functions

  /**
    * Cycle through the collection of transactions waiting to go out and
    *   send any that are waiting on an ATTACH_REPLY from the queue
    *   specified by the queue_mapping passed in.
    *
    * @returns NS_OK
    * @returns NS_ERROR_INVALID_ARG if the queue_mapping passed in is null
    */
  nsresult DispatchStoredMessages(queue_mapping *aQMapping);

  // helper methods for accessing the void arrays

  /**
    * @returns the ID corresponding to the queue name passed in
    * @returns TM_INVALID_ID if the name is not found.
    */
  PRUint32 GetQueueID(const nsACString & aQueueName);

  /**
    * @returns the joined queue name - profile name + domain 
    *          (prefs, cookies etc) corresponding to the ID passed in.
    * @reutnrs nsnull if the ID is not found.
    */
  char* GetJoinedQueueName(PRUint32 aQueueID);

  /**
    * @returns the queue_mapping object that contains the ID passed in.
    * @returns nsnull if the ID is not found.
    */
  queue_mapping* GetQueueMap(PRUint32 aQueueID);

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Variables

  nsCString mProfileName;             // what profile are we dealing with
  PLHashTable *mObservers;            // maps qName -> tmITransactionObserver

  tmVector mQueueMaps;
  tmVector mWaitingMessages;

//  nsAutoVoidArray mQueueMaps;         // maps qName -> joinedQName -> qID
//  nsAutoVoidArray mWaitingMessages;   // trans waiting for an attachreply

private:

};

#endif
