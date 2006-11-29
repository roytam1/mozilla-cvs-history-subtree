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

#include "zapTimestampScaler.h"
#include "zapIMediaGraph.h"
#include "nsString.h"
#include "nsIPropertyBag2.h"
#include "nsHashPropertyBag.h"
#include "zapMediaUtils.h"
#include "zapMediaFrame.h"

////////////////////////////////////////////////////////////////////////
// zapTimestampScaler

NS_IMETHODIMP
zapTimestampScaler::AddedToGraph(zapIMediaGraph *graph, const nsACString & id,
                                 nsIPropertyBag2 *node_pars)
{
  mOffset = 0;
  mNumerator = 1;
  mDenominator = 1;
  
  // unpack node parameters:
  if (!node_pars) {
    return NS_OK;
  }

  node_pars->GetPropertyAsUint64(NS_LITERAL_STRING("offset"),
                                 &mOffset);
  node_pars->GetPropertyAsInt32(NS_LITERAL_STRING("numerator"),
                                &mNumerator);
  node_pars->GetPropertyAsInt32(NS_LITERAL_STRING("denominator"),
                                &mDenominator);

  if (mDenominator == 0) {
    NS_ERROR("denominator can't be 0");
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
zapTimestampScaler::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

nsresult
zapTimestampScaler::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  return NS_OK;
}

nsresult
zapTimestampScaler::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  PRUint64 ts_in;
  input->GetTimestamp(&ts_in);
  PRUint64 ts_out = mOffset + (ts_in*mNumerator)/mDenominator;
  input->SetTimestamp(ts_out);
  *output = input;
  NS_ADDREF(*output);
  return NS_OK;
}
