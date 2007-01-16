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

#ifndef __ZAP_TIMESTAMPTRIGGER_H__
#define __ZAP_TIMESTAMPTRIGGER_H__

#include "zapITimestampTrigger.h"
#include "zapFilterNode.h"
#include "nsIWritablePropertyBag2.h"

////////////////////////////////////////////////////////////////////////
// zapTimestampTrigger

// {4F41AE42-7925-4953-BEE4-571BF3958972}
#define ZAP_TIMESTAMPTRIGGER_CID                             \
  { 0x4f41ae42, 0x7925, 0x4953, { 0xbe, 0xe4, 0x57, 0x1b, 0xf3, 0x95, 0x89, 0x72 } }

#define ZAP_TIMESTAMPTRIGGER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "timestamp-trigger"

class zapTimestampTriggerNode;

class zapTimestampTrigger : public zapFilterNode,
                            public zapITimestampTrigger
{
public:
  zapTimestampTrigger();
  virtual ~zapTimestampTrigger();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_ZAPITIMESTAMPTRIGGER
  
  NS_IMETHOD AddedToGraph(zapIMediaGraph *graph,
                          const nsACString & id,
                          nsIPropertyBag2 *node_pars);
  NS_IMETHOD RemovedFromGraph(zapIMediaGraph *graph);
  virtual nsresult ValidateNewStream(nsIPropertyBag2* streamInfo);
  virtual nsresult Filter(zapIMediaFrame* input, zapIMediaFrame** output);

private:
  PRUint64 mLastTimestamp;
  PRUint32 mTriggerIdCount; // counter for generating ids
  zapTimestampTriggerNode* mTriggers;
  PRBool mTriggerListLocked;
};

#endif // __ZAP_TIMESTAMPTRIGGER_H__
