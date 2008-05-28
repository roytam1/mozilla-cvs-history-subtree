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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

#include "zapTimestampTrigger.h"
#include "zapIMediaFrame.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapTimestampTriggerNode

class zapTimestampTriggerNode
{
public:
  typedef enum { SENTINEL, TRIGGER } NodeType;

  zapTimestampTriggerNode() : type(SENTINEL)
  { prev = this; next = this; }
  
  virtual ~zapTimestampTriggerNode() {};

  NodeType type;

  zapTimestampTriggerNode* prev;
  zapTimestampTriggerNode* next;
};

class zapTriggerDescriptor : public zapTimestampTriggerNode
{
public:
  zapTriggerDescriptor(PRUint64 _triggertime,
                       zapITimestampTriggerCallback* _callback,
                       const nsACString &_id,
                       const nsACString &_opaque,
                       PRBool _oneshot,
                       zapTimestampTriggerNode* _prev,
                       zapTimestampTriggerNode* _next)
      : triggertime(_triggertime), callback(_callback),
        id(_id), opaque(_opaque), oneshot(_oneshot)
  {
    type = TRIGGER;
    prev = _prev;
    next = _next;
  };

  virtual ~zapTriggerDescriptor() {}

  PRUint64 triggertime;
  nsCOMPtr<zapITimestampTriggerCallback> callback;
  nsCString id;
  nsCString opaque;
  PRBool oneshot;  
};

////////////////////////////////////////////////////////////////////////
// zapTimestampTrigger

zapTimestampTrigger::zapTimestampTrigger()
    : mTriggerListLocked(PR_FALSE),
      mTriggerIdCount(0),
      mTriggers(nsnull)
{
}

zapTimestampTrigger::~zapTimestampTrigger()
{
  NS_ASSERTION(!mTriggers, "inconsitent state");
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF_INHERITED(zapTimestampTrigger, zapFilterNode)
NS_IMPL_RELEASE_INHERITED(zapTimestampTrigger, zapFilterNode)

NS_INTERFACE_MAP_BEGIN(zapTimestampTrigger)
  NS_INTERFACE_MAP_ENTRY(zapITimestampTrigger)
NS_INTERFACE_MAP_END_INHERITING(zapFilterNode)

//----------------------------------------------------------------------
// zapITimestampTrigger methods:

/* ACString addTrigger (in zapITimestampTriggerCallback callback, in unsigned long long triggertime, in ACString opaque, in boolean oneshot); */
NS_IMETHODIMP
zapTimestampTrigger::AddTrigger(zapITimestampTriggerCallback *callback,
                                PRUint64 triggertime,
                                const nsACString & opaque,
                                PRBool oneshot, nsACString & _retval)
{
  if (mTriggerListLocked) {
    NS_WARNING("Synchronous invocation of this method from callback is not allowed");
    return NS_ERROR_FAILURE;
  }
  
  // make a new id:
  nsCString id = NS_LITERAL_CSTRING("t");
  id.AppendInt(mTriggerIdCount++);
  
  zapTriggerDescriptor *descr =
    new zapTriggerDescriptor(triggertime, callback, id, opaque, oneshot,
                             mTriggers->prev, mTriggers);
  mTriggers->prev->next = descr;
  mTriggers->prev = descr;
  
  return NS_OK;
}

/* void removeTrigger (in ACString triggerID); */
NS_IMETHODIMP
zapTimestampTrigger::RemoveTrigger(const nsACString & triggerID)
{
  if (mTriggerListLocked) {
    NS_WARNING("Synchronous invocation of this method from callback is not allowed");
    return NS_ERROR_FAILURE;
  }
  
  zapTimestampTriggerNode* node = mTriggers->next;
  while (node != mTriggers) {
    NS_ASSERTION(node->type == zapTimestampTriggerNode::TRIGGER,
                 "uh-oh, corrupt trigger list");
    if (((zapTriggerDescriptor*)node)->id == triggerID) {
      node->prev->next = node->next;
      node->next->prev = node->prev;
      delete node;
      return NS_OK;
    }
    node = node->next;
  }
  
  NS_WARNING("Trigger not found");
  return NS_OK;
}

/* void removeAllTriggers (); */
NS_IMETHODIMP
zapTimestampTrigger::RemoveAllTriggers()
{
  if (mTriggerListLocked) {
    NS_WARNING("Synchronous invocation of this method from callback is not allowed");
    return NS_ERROR_FAILURE;
  }
  
  zapTimestampTriggerNode* node = mTriggers->next;
  while (node != mTriggers) {
    zapTimestampTriggerNode* current = node;
    node = node->next;
    delete current;
  }
  mTriggers->next = mTriggers;
  mTriggers->prev = mTriggers;
  
  return NS_OK;
}

//----------------------------------------------------------------------
// Implementation helpers:

NS_IMETHODIMP
zapTimestampTrigger::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                           nsIPropertyBag2 *node_pars)
{
  NS_ASSERTION(!mTriggers, "inconsistent state");
  mTriggers = new zapTimestampTriggerNode;
  mLastTimestamp = -1;
  return NS_OK;
}

NS_IMETHODIMP
zapTimestampTrigger::RemovedFromContainer()
{
  NS_ASSERTION(!mTriggerListLocked, "uh-oh, inconsitent state");
  RemoveAllTriggers();
  NS_ASSERTION(mTriggers &&
               mTriggers->prev == mTriggers &&
               mTriggers->next == mTriggers,
               "corrupt trigger list");
  delete mTriggers;
  mTriggers = nsnull;
  
  return NS_OK;
}

nsresult
zapTimestampTrigger::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  mLastTimestamp = -1;
  return NS_OK;
}

nsresult
zapTimestampTrigger::Filter(zapIMediaFrame* input,
                            zapIMediaFrame** output)
{
  *output = input;
  NS_ADDREF(*output);

  PRUint64 timestamp;
  input->GetTimestamp(&timestamp);
  
  NS_ASSERTION(!mTriggerListLocked, "inconsistent state!");
  mTriggerListLocked = PR_TRUE;
  zapTimestampTriggerNode* node = mTriggers->next;
  while (node != mTriggers) {
    NS_ASSERTION(node->type == zapTimestampTriggerNode::TRIGGER,
                 "uh-oh, corrupt trigger list");
    zapTriggerDescriptor* descr = (zapTriggerDescriptor*)node;
    node = node->next;
    if (descr->triggertime > mLastTimestamp &&
        timestamp >= descr->triggertime) {
      descr->callback->Trigger(descr->triggertime, descr->opaque, descr->id);
      if (descr->oneshot) {
        descr->prev->next = node;
        node->prev = descr->prev;
        delete descr;
      }
    }
  }
  mTriggerListLocked = PR_FALSE;
    
  mLastTimestamp = timestamp;
  
  return NS_OK;
}

