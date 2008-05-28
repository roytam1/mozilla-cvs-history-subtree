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

#include "zapRTPFrame.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "prnetdb.h"

////////////////////////////////////////////////////////////////////////
// zapRTPFrame implementation

zapRTPFrame::zapRTPFrame()
{
}

zapRTPFrame::~zapRTPFrame()
{
}

PRBool
zapRTPFrame::Init(const nsACString& octets, nsIPropertyBag2* streamInfo)
{
  //XXX validate packet more
  if (octets.Length() < 12) return PR_FALSE;
  
  mData = octets;
  mStreamInfo = streamInfo;
  
  return PR_TRUE;
}

PRBool
zapRTPFrame::Init()
{
  mData.SetLength(12);
  memset(mData.BeginWriting(), '\0', 12);
  SetVersion(2);
  
  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRTPFrame)
NS_IMPL_RELEASE(zapRTPFrame)

NS_INTERFACE_MAP_BEGIN(zapRTPFrame)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIRTPFrame)
  NS_INTERFACE_MAP_ENTRY(zapIRTPFrame)
  NS_INTERFACE_MAP_ENTRY(zapIMediaFrame)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIRTPFrame methods:


/* attribute unsigned short version; */
NS_IMETHODIMP
zapRTPFrame::GetVersion(PRUint16 *aVersion)
{
  *aVersion = (mData.BeginReading()[0] & 0xC0) >> 6;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetVersion(PRUint16 aVersion)
{
  mData.BeginWriting()[0] |= ((aVersion << 6) & 0xC0);
  return NS_OK;
}

/* attribute boolean padding; */
NS_IMETHODIMP
zapRTPFrame::GetPadding(PRBool *aPadding)
{
  *aPadding = (mData.BeginReading()[0] & 0x20) >> 5;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetPadding(PRBool aPadding)
{
  mData.BeginWriting()[0] |= ((aPadding << 5) & 0x20);
  return NS_OK;
}

/* attribute boolean extension; */
NS_IMETHODIMP
zapRTPFrame::GetExtension(PRBool *aExtension)
{
  *aExtension = (mData.BeginReading()[0] & 0x10) >> 4;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetExtension(PRBool aExtension)
{
  mData.BeginWriting()[0] |= ((aExtension << 4) & 0x10);
  return NS_OK;
}

/* attribute unsigned short CSRCCount; */
NS_IMETHODIMP
zapRTPFrame::GetCSRCCount(PRUint16 *aCSRCCount)
{
  *aCSRCCount = mData.BeginReading()[0] & 0x0F;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetCSRCCount(PRUint16 aCSRCCount)
{
  mData.BeginWriting()[0] |= (aCSRCCount & 0x0F);
  return NS_OK;
}

/* attribute unsigned short marker; */
NS_IMETHODIMP
zapRTPFrame::GetMarker(PRUint16 *aMarker)
{
  *aMarker = (mData.BeginReading()[1] & 0x80) >> 7;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetMarker(PRUint16 aMarker)
{
  mData.BeginWriting()[1] |= ((aMarker << 7) & 0x80);
  return NS_OK;
}

/* attribute unsigned short payloadType; */
NS_IMETHODIMP
zapRTPFrame::GetPayloadType(PRUint16 *aPayloadType)
{
  *aPayloadType = mData.BeginReading()[1] & 0x7F;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetPayloadType(PRUint16 aPayloadType)
{
  mData.BeginWriting()[1] |= (aPayloadType & 0x7F);
  return NS_OK;
}

/* attribute unsigned short sequenceNumber; */
NS_IMETHODIMP
zapRTPFrame::GetSequenceNumber(PRUint16 *aSequenceNumber)
{
  *aSequenceNumber = PR_ntohs(((PRUint16*)mData.BeginReading())[1]);
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetSequenceNumber(PRUint16 aSequenceNumber)
{
  ((PRUint16*)mData.BeginWriting())[1] = PR_htons(aSequenceNumber);
  return NS_OK;
}

/* attribute unsigned long rtpTimestamp; */
NS_IMETHODIMP
zapRTPFrame::GetRtpTimestamp(PRUint32 *aTimestamp)
{
  *aTimestamp = PR_ntohl(((PRUint32*)mData.BeginReading())[1]);
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetRtpTimestamp(PRUint32 aTimestamp)
{
  ((PRUint32*)mData.BeginWriting())[1] = PR_htonl(aTimestamp);
  return NS_OK;
}

/* attribute unsigned long SSRC; */
NS_IMETHODIMP
zapRTPFrame::GetSSRC(PRUint32 *aSSRC)
{
  *aSSRC = PR_ntohl(((PRUint32*)mData.BeginReading())[2]);
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetSSRC(PRUint32 aSSRC)
{
  ((PRUint32*)mData.BeginWriting())[2] = PR_htonl(aSSRC);
  return NS_OK;
}

/* unsigned long getCSRC (in unsigned short index); */
NS_IMETHODIMP
zapRTPFrame::GetCSRC(PRUint16 index, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setCSRC (in unsigned short index, in unsigned long CSRC); */
NS_IMETHODIMP
zapRTPFrame::SetCSRC(PRUint16 index, PRUint32 CSRC)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute ACString payload; */
NS_IMETHODIMP
zapRTPFrame::GetPayload(nsACString & aPayload)
{
  // RTP header is 12 bytes + 4*CSRC:
  PRUint16 csrcCount;
  GetCSRCCount(&csrcCount);
  PRUint16 offset = 12 + csrcCount*4;
  aPayload = Substring(mData, offset);
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetPayload(const nsACString & aPayload)
{
  PRUint16 csrcCount;
  GetCSRCCount(&csrcCount);
  PRUint16 length = 12 + csrcCount*4;
  mData = Substring(mData, 0, length);
  NS_ASSERTION(mData.Length()==length, "Unexpected!");
  mData.Append(aPayload);
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIMediaFrame methods:

/* attribute nsIPropertyBag2 streamInfo; */
NS_IMETHODIMP
zapRTPFrame::GetStreamInfo(nsIPropertyBag2** aStreamInfo)
{
  *aStreamInfo = mStreamInfo.get();
  NS_IF_ADDREF(*aStreamInfo);
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetStreamInfo(nsIPropertyBag2 * aStreamInfo)
{
  mStreamInfo = aStreamInfo;
  return NS_OK;
}

/* attribute unsigned long long timestamp; */
NS_IMETHODIMP
zapRTPFrame::GetTimestamp(PRUint64 *aTimestamp)
{
  *aTimestamp = 0;
  return NS_OK;
}
NS_IMETHODIMP
zapRTPFrame::SetTimestamp(PRUint64 aTimestamp)
{
  NS_ERROR("RTP frames currently only have 32 bit RTP timestamps");
  return NS_ERROR_FAILURE;
}

/* readonly attribute ACString data; */
NS_IMETHODIMP
zapRTPFrame::GetData(nsACString & aData)
{
  aData = mData;
  return NS_OK;
}


//----------------------------------------------------------------------

zapIRTPFrame *
CreateRTPFrame(const nsACString & octets, nsIPropertyBag2* streamInfo)
{
  zapRTPFrame* frame = new zapRTPFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);

  if (!frame->Init(octets, streamInfo)) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}

// Create a blank rtp frame:
zapRTPFrame*
CreateRTPFrame()
{
  zapRTPFrame* frame = new zapRTPFrame();
  if (!frame) return nsnull;
  NS_ADDREF(frame);
  
  if (!frame->Init()) {
    NS_RELEASE(frame);
    return nsnull;
  }
  return frame;
}
