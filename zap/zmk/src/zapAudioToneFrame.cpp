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

#include "zapAudioToneFrame.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "prnetdb.h"

////////////////////////////////////////////////////////////////////////
// zapAudioTone implementation

zapAudioTone::zapAudioTone() {
  data.SetLength(4);
  memset(data.BeginWriting(), '\0', 4);
}

#define GET_BYTE(n) (data.BeginReading()[n])
#define SET_BYTE(n, val) data.BeginWriting()[n] = val
#define GET_UINT16(n) PR_ntohs(((PRUint16*)data.BeginReading())[n])
#define SET_UINT16(n, val) ((PRUint16*)data.BeginWriting())[n] = PR_htons(val)

PRUint16 zapAudioTone::GetModulation()
{
  return (GET_UINT16(0) & 0xFF80) >> 7;
}

void zapAudioTone::SetModulation(PRUint16 modulation)
{
  SET_UINT16(0, (modulation << 7) | (GET_UINT16(0) & 0x007F));
}

PRBool zapAudioTone::GetT()
{
  return (GET_BYTE(1) & 0x40) >> 6;
}

void zapAudioTone::SetT(PRBool T)
{
  SET_BYTE(1, ((T << 6) & 0x40) | (GET_BYTE(1) & 0xAF));
}

PRUint16 zapAudioTone::GetVolume()
{
  return GET_BYTE(1) & 0x3F;
}

void zapAudioTone::SetVolume(PRUint16 volume)
{
  SET_BYTE(1, (volume & 0x3F) | (GET_BYTE(1) & 0xC0));
}

PRUint16 zapAudioTone::GetDuration()
{
  return GET_UINT16(1);
}

void zapAudioTone::SetDuration(PRUint16 duration)
{
  SET_UINT16(1, duration);
}

PRUint16 zapAudioTone::GetFrequencyCount()
{
  return (data.Length() - 4)/2;
}

PRUint16 zapAudioTone::GetFrequencyAt(PRUint16 n)
{
  NS_ASSERTION(data.Length()/2 - 2 > n, "out of range");
  return GET_UINT16(n+2) & 0x0FFF;
}

void zapAudioTone::AddFrequency(PRUint16 frequency)
{
  PRUint32 l = data.Length();
  data.SetLength(l + 2);
  SET_UINT16(l/2, frequency & 0x0FFF);
}


////////////////////////////////////////////////////////////////////////
// zapAudioToneFrame implementation

zapAudioToneFrame::zapAudioToneFrame()
    : mMarkerBit(PR_FALSE)
{
}

zapAudioToneFrame::~zapAudioToneFrame()
{
}

PRBool
zapAudioToneFrame::Init(const nsACString& octets, nsIPropertyBag2* streamInfo)
{
  mStreamInfo = streamInfo;
 
  //XXX validate packet more
  if (octets.Length() < 4) return PR_FALSE;
  
  mToneData.data = octets;
  return PR_TRUE;
}

PRBool
zapAudioToneFrame::Init(const zapAudioTone& tone, nsIPropertyBag2* streamInfo)
{
  mStreamInfo = streamInfo;

  mToneData = tone;
  return PR_TRUE;
}

PRBool
zapAudioToneFrame::Init()
{
  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioToneFrame)
NS_IMPL_THREADSAFE_RELEASE(zapAudioToneFrame)

NS_INTERFACE_MAP_BEGIN(zapAudioToneFrame)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIAudioToneFrame)
  NS_INTERFACE_MAP_ENTRY(zapIAudioToneFrame)
  NS_INTERFACE_MAP_ENTRY(zapIMediaFrame)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIAudioToneFrame methods:

/* attribute boolean M; */
NS_IMETHODIMP
zapAudioToneFrame::GetM(PRBool *aM)
{
  *aM = mMarkerBit;
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetM(PRBool aM)
{
  mMarkerBit = aM;
  return NS_OK;
}

/* attribute unsigned short modulation; */
NS_IMETHODIMP
zapAudioToneFrame::GetModulation(PRUint16 *aModulation)
{
  *aModulation = mToneData.GetModulation();
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetModulation(PRUint16 aModulation)
{
  mToneData.SetModulation(aModulation);
  return NS_OK;
}

/* attribute boolean T; */
NS_IMETHODIMP
zapAudioToneFrame::GetT(PRBool *aT)
{
  *aT = mToneData.GetT();
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetT(PRBool aT)
{
  mToneData.SetT(aT);
  return NS_OK;
}

/* attribute unsigned short volume; */
NS_IMETHODIMP
zapAudioToneFrame::GetVolume(PRUint16 *aVolume)
{
  *aVolume = mToneData.GetVolume();
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetVolume(PRUint16 aVolume)
{
  mToneData.SetVolume(aVolume);
  return NS_OK;
}

/* attribute unsigned short duration; */
NS_IMETHODIMP
zapAudioToneFrame::GetDuration(PRUint16 *aDuration)
{
  *aDuration = mToneData.GetDuration();
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetDuration(PRUint16 aDuration)
{
  mToneData.SetDuration(aDuration);
  return NS_OK;
}

/* readonly attribute unsigned short frequencyCount; */
NS_IMETHODIMP
zapAudioToneFrame::GetFrequencyCount(PRUint16 *aFrequencyCount)
{
  *aFrequencyCount = mToneData.GetFrequencyCount();
  return NS_OK;
}

/* unsigned short getFrequencyAt (in unsigned short n); */
NS_IMETHODIMP
zapAudioToneFrame::GetFrequencyAt(PRUint16 n, PRUint16 *_retval)
{
  *_retval = mToneData.GetFrequencyAt(n);
  return NS_OK;
}

/* void addFrequency (in unsigned short frequency); */
NS_IMETHODIMP
zapAudioToneFrame::AddFrequency(PRUint16 frequency)
{
  mToneData.AddFrequency(frequency);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaFrame methods:

/* attribute nsIPropertyBag2 streamInfo; */
NS_IMETHODIMP
zapAudioToneFrame::GetStreamInfo(nsIPropertyBag2** aStreamInfo)
{
  *aStreamInfo = mStreamInfo.get();
  NS_IF_ADDREF(*aStreamInfo);
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetStreamInfo(nsIPropertyBag2 * aStreamInfo)
{
  mStreamInfo = aStreamInfo;
  return NS_OK;
}

/* attribute unsigned long long timestamp; */
NS_IMETHODIMP
zapAudioToneFrame::GetTimestamp(PRUint64 *aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}
NS_IMETHODIMP
zapAudioToneFrame::SetTimestamp(PRUint64 aTimestamp)
{
  mTimestamp = aTimestamp;
  return NS_OK;
}

/* readonly attribute ACString data; */
NS_IMETHODIMP
zapAudioToneFrame::GetData(nsACString & aData)
{
  aData = mToneData.data;
  return NS_OK;
}

//----------------------------------------------------------------------

zapIAudioToneFrame *
CreateAudioToneFrame(const nsACString & octets, nsIPropertyBag2* streamInfo)
{
  zapAudioToneFrame* frame = new zapAudioToneFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);

  if (!frame->Init(octets, streamInfo)) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}

zapAudioToneFrame *
CreateAudioToneFrame(const zapAudioTone& tone, nsIPropertyBag2* streamInfo)
{
  zapAudioToneFrame* frame = new zapAudioToneFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);

  if (!frame->Init(tone, streamInfo)) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}

// Create a blank audiotone frame:
zapAudioToneFrame*
CreateAudioToneFrame()
{
  zapAudioToneFrame* frame = new zapAudioToneFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);
  
  if (!frame->Init()) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}
