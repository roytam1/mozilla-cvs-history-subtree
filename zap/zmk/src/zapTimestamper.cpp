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
 * Portions created by the Initial Developer are Copyright (C) 2006
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

#include "zapTimestamper.h"
#include "nsStringAPI.h"
#include "nsIPropertyBag2.h"
#include "nsHashPropertyBag.h"
#include "zapMediaUtils.h"
#include "zapMediaFrame.h"

////////////////////////////////////////////////////////////////////////
// zapTimestamper

zapTimestamper::zapTimestamper()
{
}

zapTimestamper::~zapTimestamper()
{
}

NS_IMETHODIMP
zapTimestamper::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                      nsIPropertyBag2 *node_pars)
{
  mFrameCount = 0;
  mOffset = 0;
  mIncrNum = 1;
  mIncrDenom = 1;
  
  // unpack node parameters:
  if (!node_pars) {
    return NS_OK;
  }

  node_pars->GetPropertyAsUint64(NS_LITERAL_STRING("offset"),
                                 &mOffset);
  node_pars->GetPropertyAsInt32(NS_LITERAL_STRING("incr_num"),
                                &mIncrNum);
  node_pars->GetPropertyAsInt32(NS_LITERAL_STRING("incr_denom"),
                                &mIncrDenom);

  if (mIncrDenom == 0) {
    NS_ERROR("denominator can't be 0");
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
zapTimestamper::RemovedFromContainer()
{
  return NS_OK;
}

nsresult
zapTimestamper::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  // reset framer count:
  mFrameCount = 0;
  
  return NS_OK;
}

nsresult
zapTimestamper::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  PRUint64 ts = mOffset + (mIncrNum*mFrameCount)/mIncrDenom;
  input->SetTimestamp(ts);
  ++mFrameCount;
  *output = input;
  NS_ADDREF(*output);
  return NS_OK;
}
