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

#include "zapTelephoneEventFrame.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "prnetdb.h"

////////////////////////////////////////////////////////////////////////
// zapTelephoneEvent implementation

zapTelephoneEvent::zapTelephoneEvent() {
  data.SetLength(4);
  memset(data.BeginWriting(), '\0', 4);
}

#define GET_BYTE(n) (data.BeginReading()[n])
#define SET_BYTE(n, val) data.BeginWriting()[n] = val
#define GET_UINT16(n) PR_ntohs(((PRUint16*)data.BeginReading())[n])
#define SET_UINT16(n, val) ((PRUint16*)data.BeginWriting())[n] = PR_htons(val)

PRUint16 zapTelephoneEvent::GetEvent()
{
  return GET_BYTE(0);
}

void zapTelephoneEvent::SetEvent(PRUint16 tevent)
{
  SET_BYTE(0, (PRUint8)tevent);
}

PRBool zapTelephoneEvent::GetE()
{
  return (GET_BYTE(1) & 0x80) >> 7;
}

void zapTelephoneEvent::SetE(PRBool E)
{
  SET_BYTE(1, ((E << 7) & 0x80) | (GET_BYTE(1) & 0x7F));
}

PRUint16 zapTelephoneEvent::GetVolume()
{
  return GET_BYTE(1) & 0x3F;
}

void zapTelephoneEvent::SetVolume(PRUint16 volume)
{
  SET_BYTE(1, (volume & 0x3F) | (GET_BYTE(1) & 0xC0));
}

PRUint16 zapTelephoneEvent::GetDuration()
{
  return GET_UINT16(1);
}

void zapTelephoneEvent::SetDuration(PRUint16 duration)
{
  SET_UINT16(1, duration);
}

////////////////////////////////////////////////////////////////////////
// zapTelephoneEventFrame implementation

zapTelephoneEventFrame::zapTelephoneEventFrame()
    : mMarkerBit(PR_FALSE)
{
}

zapTelephoneEventFrame::~zapTelephoneEventFrame()
{
}

PRBool
zapTelephoneEventFrame::Init(const nsACString& octets, nsIPropertyBag2* streamInfo)
{
  mStreamInfo = streamInfo;
 
  //XXX validate packet more
  if (octets.Length() != 4) return PR_FALSE;
  
  mTEventData.data = octets;
  return PR_TRUE;
}

PRBool
zapTelephoneEventFrame::Init(const zapTelephoneEvent& tevent, nsIPropertyBag2* streamInfo)
{
  mStreamInfo = streamInfo;

  mTEventData = tevent;
  return PR_TRUE;
}

PRBool
zapTelephoneEventFrame::Init()
{
  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapTelephoneEventFrame)
NS_IMPL_THREADSAFE_RELEASE(zapTelephoneEventFrame)

NS_INTERFACE_MAP_BEGIN(zapTelephoneEventFrame)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapITelephoneEventFrame)
  NS_INTERFACE_MAP_ENTRY(zapITelephoneEventFrame)
  NS_INTERFACE_MAP_ENTRY(zapIMediaFrame)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapITelephoneEventFrame methods:

/* attribute boolean M; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetM(PRBool *aM)
{
  *aM = mMarkerBit;
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetM(PRBool aM)
{
  mMarkerBit = aM;
  return NS_OK;
}

/* attribute unsigned short event; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetEvent(PRUint16 *aEvent)
{
  *aEvent = mTEventData.GetEvent();
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetEvent(PRUint16 aEvent)
{
  mTEventData.SetEvent(aEvent);
  return NS_OK;
}

/* attribute boolean E; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetE(PRBool *aE)
{
  *aE = mTEventData.GetE();
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetE(PRBool aE)
{
  mTEventData.SetE(aE);
  return NS_OK;
}

/* attribute unsigned short volume; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetVolume(PRUint16 *aVolume)
{
  *aVolume = mTEventData.GetVolume();
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetVolume(PRUint16 aVolume)
{
  mTEventData.SetVolume(aVolume);
  return NS_OK;
}

/* attribute unsigned short duration; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetDuration(PRUint16 *aDuration)
{
  *aDuration = mTEventData.GetDuration();
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetDuration(PRUint16 aDuration)
{
  mTEventData.SetDuration(aDuration);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaFrame methods:

/* attribute nsIPropertyBag2 streamInfo; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetStreamInfo(nsIPropertyBag2** aStreamInfo)
{
  *aStreamInfo = mStreamInfo.get();
  NS_IF_ADDREF(*aStreamInfo);
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetStreamInfo(nsIPropertyBag2 * aStreamInfo)
{
  mStreamInfo = aStreamInfo;
  return NS_OK;
}

/* attribute unsigned long long timestamp; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetTimestamp(PRUint64 *aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}
NS_IMETHODIMP
zapTelephoneEventFrame::SetTimestamp(PRUint64 aTimestamp)
{
  mTimestamp = aTimestamp;
  return NS_OK;
}

/* readonly attribute ACString data; */
NS_IMETHODIMP
zapTelephoneEventFrame::GetData(nsACString & aData)
{
  aData = mTEventData.data;
  return NS_OK;
}

//----------------------------------------------------------------------

zapITelephoneEventFrame *
CreateTelephoneEventFrame(const nsACString & octets, nsIPropertyBag2* streamInfo)
{
  zapTelephoneEventFrame* frame = new zapTelephoneEventFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);

  if (!frame->Init(octets, streamInfo)) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}

zapTelephoneEventFrame *
CreateTelephoneEventFrame(const zapTelephoneEvent& tevent, nsIPropertyBag2* streamInfo)
{
  zapTelephoneEventFrame* frame = new zapTelephoneEventFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);

  if (!frame->Init(tevent, streamInfo)) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}

// Create a blank telephone-event frame:
zapTelephoneEventFrame*
CreateTelephoneEventFrame()
{
  zapTelephoneEventFrame* frame = new zapTelephoneEventFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);
  
  if (!frame->Init()) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}
