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

#include "zapRtpPacket.h"
#include "stdio.h"
#include "nsString.h"
#include "prnetdb.h"

////////////////////////////////////////////////////////////////////////
// zapRtpPacket implementation

zapRtpPacket::zapRtpPacket()
{
}

zapRtpPacket::~zapRtpPacket()
{
}

PRBool
zapRtpPacket::Init(const nsACString& octets)
{
  //XXX validate packet more
  if (octets.Length() < 12) return PR_FALSE;
  
  mData = octets;

  return PR_TRUE;
}

PRBool
zapRtpPacket::Init()
{
  // XXX we want mData.SetTo("\0", 12) or something like that
  mData.Assign("\0\0\0\0\0\0\0\0\0\0\0\0", 12);

  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRtpPacket)
NS_IMPL_RELEASE(zapRtpPacket)

NS_INTERFACE_MAP_BEGIN(zapRtpPacket)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(zapIRtpPacket)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIRtpPacket methods:


/* attribute unsigned short version; */
NS_IMETHODIMP
zapRtpPacket::GetVersion(PRUint16 *aVersion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
zapRtpPacket::SetVersion(PRUint16 aVersion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean padding; */
NS_IMETHODIMP
zapRtpPacket::GetPadding(PRBool *aPadding)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
zapRtpPacket::SetPadding(PRBool aPadding)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean extension; */
NS_IMETHODIMP
zapRtpPacket::GetExtension(PRBool *aExtension)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
zapRtpPacket::SetExtension(PRBool aExtension)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned short CSRCCount; */
NS_IMETHODIMP
zapRtpPacket::GetCSRCCount(PRUint16 *aCSRCCount)
{
  *aCSRCCount = mData.BeginReading()[0] & 0x0F;
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetCSRCCount(PRUint16 aCSRCCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned short marker; */
NS_IMETHODIMP
zapRtpPacket::GetMarker(PRUint16 *aMarker)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
zapRtpPacket::SetMarker(PRUint16 aMarker)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned short payloadType; */
NS_IMETHODIMP
zapRtpPacket::GetPayloadType(PRUint16 *aPayloadType)
{
  *aPayloadType = mData.BeginReading()[1] & 0x7F;
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetPayloadType(PRUint16 aPayloadType)
{
  mData.BeginWriting()[1] |= (aPayloadType & 0x7F);
  return NS_OK;
}

/* attribute unsigned short sequenceNumber; */
NS_IMETHODIMP
zapRtpPacket::GetSequenceNumber(PRUint16 *aSequenceNumber)
{
  *aSequenceNumber = PR_ntohs(((PRUint16*)mData.BeginReading())[1]);
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetSequenceNumber(PRUint16 aSequenceNumber)
{
  ((PRUint16*)mData.BeginWriting())[1] = PR_htons(aSequenceNumber);
  return NS_OK;
}

/* attribute unsigned long timestamp; */
NS_IMETHODIMP
zapRtpPacket::GetTimestamp(PRUint32 *aTimestamp)
{
  *aTimestamp = PR_ntohl(((PRUint32*)mData.BeginReading())[1]);
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetTimestamp(PRUint32 aTimestamp)
{
  ((PRUint32*)mData.BeginWriting())[1] = PR_htonl(aTimestamp);
  return NS_OK;
}

/* attribute unsigned long SSRC; */
NS_IMETHODIMP
zapRtpPacket::GetSSRC(PRUint32 *aSSRC)
{
  *aSSRC = PR_ntohl(((PRUint32*)mData.BeginReading())[2]);
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetSSRC(PRUint32 aSSRC)
{
  ((PRUint32*)mData.BeginWriting())[2] = PR_htonl(aSSRC);
  return NS_OK;
}

/* unsigned long getCSRC (in unsigned short index); */
NS_IMETHODIMP
zapRtpPacket::GetCSRC(PRUint16 index, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setCSRC (in unsigned short index, in unsigned long CSRC); */
NS_IMETHODIMP
zapRtpPacket::SetCSRC(PRUint16 index, PRUint32 CSRC)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute ACString payload; */
NS_IMETHODIMP
zapRtpPacket::GetPayload(nsACString & aPayload)
{
  // RTP header is 12 bytes + 4*CSRC:
  PRUint16 csrcCount;
  GetCSRCCount(&csrcCount);
  PRUint16 offset = 12 + csrcCount*4;
  aPayload = Substring(mData, offset);
  return NS_OK;
}
NS_IMETHODIMP
zapRtpPacket::SetPayload(const nsACString & aPayload)
{
  PRUint16 csrcCount;
  GetCSRCCount(&csrcCount);
  PRUint16 length = 12 + csrcCount*4;
  mData = Substring(mData, 0, length);
  NS_ASSERTION(mData.Length()==length, "Unexpected!");
  mData.Append(aPayload);
  return NS_OK;
}

/* readonly attribute ACString packet; */
NS_IMETHODIMP
zapRtpPacket::GetPacket(nsACString & aPacket)
{
  aPacket = mData;
  return NS_OK;
}

//----------------------------------------------------------------------

zapIRtpPacket *
CreateRtpPacket(const nsACString & octets)
{
  zapRtpPacket* packet = new zapRtpPacket();
  if (!packet) return nsnull;
  NS_ADDREF(packet);

  if (!packet->Init(octets)) {
    NS_RELEASE(packet);
    return nsnull;
  }
  return packet;
}

// Create a blank rtp packet:
zapIRtpPacket *
CreateRtpPacket()
{
  zapRtpPacket* packet = new zapRtpPacket();
  if (!packet) return nsnull;
  NS_ADDREF(packet);
  
  if (!packet->Init()) {
    NS_RELEASE(packet);
    return nsnull;
  }
  return packet;
}
