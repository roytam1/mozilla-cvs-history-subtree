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
#include "tmTransaction.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor(s) & Destructor

tmTransaction::~tmTransaction() {
  if (mMessage)
    free(mMessage);
}

///////////////////////////////////////////////////////////////////////////////
// Protected Methods

nsresult
tmTransaction::Init(PRUint32 aClientIPCID, 
                    const PRUint8 *aRawMessage, 
                    PRUint32 aLength) {

  mOwnerID = aClientIPCID;
  nsresult rv = SetRawMessage(aRawMessage, aLength);
  if (NS_FAILED(rv)) {
    mMessage = 0;
    mMessageLength = 0;
    mData.queueID = 0;
    mData.action = 0;
    mData.status = rv;
  }
  return rv;
}

nsresult
tmTransaction::Init(PRUint32 aClientIPCID, 
                    PRUint32 aID, 
                    PRUint32 aAction, 
                    nsresult aStatus) {

  mOwnerID = aClientIPCID;
  mData.queueID = aID;
  mData.action = aAction;
  mData.status = aStatus;
  return NS_OK;
}

nsresult
tmTransaction::Init(PRUint32 aClientIPCID, 
                    PRUint32 aID, 
                    PRUint32 aAction, 
                    nsresult aStatus, 
                    const PRUint8 *aMessage, 
                    PRUint32 aLength) {

  mOwnerID = aClientIPCID;
  nsresult rv = SetMessage(aMessage, aLength);
  if (NS_SUCCEEDED(rv)) {
    mData.queueID = aID;
    mData.action = aAction;
    mData.status = aStatus;
  }
  else {
    mMessage = 0;
    mMessageLength = 0;
    mData.queueID = 0;
    mData.action = 0;
    mData.status = rv;
  }
  return rv;
}

// Does not take ownwership of the message passed in
// sets mMessage ONLY
nsresult
tmTransaction::SetMessage(const PRUint8 *aMessage, PRUint32 aLength) {

  if (!aMessage)
    return NS_ERROR_INVALID_ARG;

  // clean up the old message
  if (mMessage)
    free(mMessage);

  // allocate room for the new message
  mMessage = (PRUint8 *)malloc(aLength);
  if (!mMessage)
    return NS_ERROR_OUT_OF_MEMORY;

  // set the new message and length
  memcpy(mMessage, aMessage, aLength);
  mMessageLength = aLength;
  return NS_OK;
}

// Does not take ownwership of the RawMessage passed in
// Sets mData and if more data, mMessage
nsresult
tmTransaction::SetRawMessage(const PRUint8 *aRawMessage, PRUint32 aLength) {

  if (!aRawMessage || aLength < sizeof(tmMsg))
    return NS_ERROR_INVALID_ARG;

  // clean up the old message
  if (mMessage)
    free(mMessage);

  // set the members of the tmMsg
  PRUint32 *index = (PRUint32 *)aRawMessage;
  mData.queueID = *index;
  mData.action = *(++index);   // pointer arithmetic
  mData.status = *(++index);   // pointer arithmetic

  // see if there is additional message data
  PRUint32 headersize = sizeof(tmMsg);
  if (aLength > headersize) {
    // allocate
    mMessageLength = aLength - headersize;
    mMessage = (PRUint8 *)malloc(mMessageLength);
    if (!mMessage)
      return NS_ERROR_OUT_OF_MEMORY;

    // copy
    memcpy(mMessage, (PRUint8 *)++index, mMessageLength);
  }
  return NS_OK;
}

// returns a copy of the message or nsnull if allocs fail
PRUint8 *
tmTransaction::GetRawMessage() const {

  PRUint8 *data = nsnull;
  PRUint8 *index = nsnull;
  PRUint32 msgSize = sizeof(tmMsg);

  if (mMessageLength == 0) {      // mData only
    // allocate
    data = (PRUint8 *)malloc(msgSize);

    // copy
    if (data)
      memcpy(data, &mData, msgSize);
  }
  else {                          // mData + mMessage
    // allocate
    data = (PRUint8 *)malloc(msgSize + mMessageLength);

    // copy
    if (data) {
      memcpy(data, &mData, msgSize);
      index = data + msgSize;
      memcpy(index, mMessage, mMessageLength);
    }
  }
  return (PRUint8 *)data;
}
