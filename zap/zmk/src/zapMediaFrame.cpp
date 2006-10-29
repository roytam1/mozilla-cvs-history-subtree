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
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#include "zapMediaFrame.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapMediaFrame

zapMediaFrame::zapMediaFrame()
{
#ifdef DEBUG_afri_zmk
//  printf("/");
#endif
}

zapMediaFrame::~zapMediaFrame()
{
#ifdef DEBUG_afri_zmk
//  printf("\\");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapMediaFrame)
NS_IMPL_THREADSAFE_RELEASE(zapMediaFrame)

NS_INTERFACE_MAP_BEGIN(zapMediaFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(zapIMediaFrame)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaFrame methods:

/* readonly attribute nsIPropertyBag2 streamInfo; */
NS_IMETHODIMP
zapMediaFrame::GetStreamInfo(nsIPropertyBag2** aStreamInfo)
{
  *aStreamInfo = mStreamInfo.get();
  NS_IF_ADDREF(*aStreamInfo);
  return NS_OK;
}

/* attribute unsigned long long timestamp; */
NS_IMETHODIMP
zapMediaFrame::GetTimestamp(PRUint64 *aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}
NS_IMETHODIMP
zapMediaFrame::SetTimestamp(PRUint64 aTimestamp)
{
  mTimestamp = aTimestamp;
  return NS_OK;
}

/* readonly attribute ACString data; */
NS_IMETHODIMP
zapMediaFrame::GetData(nsACString & aData)
{
  aData = mData;
  return NS_OK;
}

