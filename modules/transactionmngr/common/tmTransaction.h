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

#ifndef _tmTransaction_H_
#define _tmTransaction_H_

#include "tmUtils.h"

//////////////////////////////////////////////////////////////////////////////
//
// Message format
//
// |------------------------------------|--
// |Action - Post/Flush/Attach etc      |  |
// |------------------------------------|  |
// |QueueID                             |  |- this is the tmMsg
// |------------------------------------|  |
// |Status                              |  | 
// |------------------------------------|--
// |Message Data                        |
// |------------------------------------|
//
// The Attach call is a special case in that it doesn't have a QueueID yet. A
//   QueueID will be 0's. The message Data will be the Queue Name String which
//   will be the profile name with a domain attached, a domain being 
//   [prefs|cookies|etc]
//
//////////////////////////////////////////////////////////////////////////////

struct tmMsg {
  PRUint32 queueID;
  PRUint32 action;
  nsresult status;
};

const PRUint32 TM_INVALID_ID = 0xFFFFFFFF;

class tmTransaction
{

public:

  ////////////////////////////////////////////////////////////////////////////
  // Constructor(s) & Destructor

  tmTransaction(): mData(), mMessage(nsnull), mMessageLength(0),
                   mOwnerID(0) { ;}

  virtual ~tmTransaction();

  ////////////////////////////////////////////////////////////////////////////
  // Public Member Functions

  // Initializers

  /**
    * Initializer to use when the tmMsg and the message information
    *   have already been combined. This initializer seperates the
    *   raw message into the tmMsg and the message. 
    *   
    * Used by the tmIPCModule & tmTransactionService as they only receive
    *   a void* as data and not seperate tmMsg and possible message data
    *
    * @post on failure of setting message, all data members are cleared
    *
    * @returns success of setting the message in the transaction
    */
  nsresult Init(PRUint32 aClientIPCID,
                const PRUint8 *aRawMessage,
                PRUint32 aLength);

  /**
    * Initializer for transactions with only tmMsg data to pass on.
    *   Examples include most replies, detach, flush...
    *
    * @returns NS_OK
    */
  nsresult Init(PRUint32 aClientIPCID,
                PRUint32 aID,
                PRUint32 aAction,
                nsresult aStatus);

  /**
    * Initializer for transactions that have tmMsg information and
    *   additional message information. An example is the attach_reply
    *   message that must pass back the name of the queue so the
    *   client side tmTransactionService can associate the name with
    *   the ID returned in the tmMsg.
    *
    * @post on failure of setting message, all data members are cleared
    *
    * @returns success of setting the message in the transaction
    */
  nsresult Init(PRUint32 aClientIPCID,
                PRUint32 aID,
                PRUint32 aAction,
                nsresult aStatus,
                const PRUint8 *aMessage,
                PRUint32 aLength);

  // accessors

  /**
    * @returns a byte pointer to the message carried by this transaction
    */
  const PRUint8* GetMessage() const { return mMessage; }

  /**
    * @returns the length of the message carried by this transaction
    */
  PRUint32 GetMessageLength() const { return mMessageLength; }

  // mData accessors

  /**
    * @returns the id of the destination or sending queue
    */
  PRUint32 GetQueueID() const { return mData.queueID; }

  /**
    * @returns the action represented by this transaction
    */
  PRUint32 GetAction() const { return mData.action; }

  /**
    * @returns the success state, if applicable of the action leading
    *          up to this message
    */
  nsresult GetStatus() const { return mData.status; }

  /**
    * Creates a copy of both the mData and mMessage variables. Uses
    *   malloc to allocate memory. The caller must assume ownership of
    *   the allocated space.
    *
    * @returns a pointer to the newly malloced memory containing the
    *          mData information followed immediately by the mMessage
    *          data.
    */
  PRUint8* GetRawMessage() const;

  /**
    * @returns the length of the flags (mData) and message combined
    */
  PRUint32 GetRawMessageLength() const { 
    return (sizeof(tmMsg) + mMessageLength); 
  }

  /**
    * @returns the client ID (in IPC daemon terms) of the client who initiated
    *          the exchange that generated this transaction.
    */
  PRUint32 GetOwnerID() const { return mOwnerID; }

  /**
    * @returns sets the ID of the destination or source queue
    */
  void SetQueueID(PRUint32 aID) { mData.queueID = aID; }

protected:

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Functions

  /**
    * This sets the data member mMessage. Ownership of the pointer passed
    *   in is not taken. The message is copied into the transaction.
    *
    * @returns NS_OK if everything was successful
    * @returns NS_ERROR_INVALID_ARG if the pointer passed in was null
    * @returns NS_ERROR_OUT_OF_MEMORY if allocation of space for the
    *          copy of the message fails.
    */
  nsresult SetMessage(const PRUint8 *aMessage, PRUint32 aLength);

  /**
    * This sets the value mData and if additional data is present sets
    *   the value of the data member mMessage. Ownership of the pointer
    *   passed in is not taken. The message is copied into the transaction.
    *
    * @returns NS_OK if everything was successful
    * @returns NS_ERROR_INVALID_ARG if the pointer passed in was null
    * @returns NS_ERROR_OUT_OF_MEMORY if allocation of space for the
    *          copy of the message fails.
    */
  nsresult SetRawMessage(const PRUint8 *aRawMessage, PRUint32 aLength);

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Variables

  tmMsg    mData;              // struct containing type,id,status flags
  PRUint8 *mMessage;           // actual data to be processed by the clients
  PRUint32 mMessageLength;     // length of mMessage
  PRUint32 mOwnerID;           // client who sent this trans. - a IPC ClientID

};

#endif
