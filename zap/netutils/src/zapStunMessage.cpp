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

#include "zapStunMessage.h"
#include "prnetdb.h"
#include "prprf.h"
#include <stdlib.h>
#include "zapICryptoUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"

////////////////////////////////////////////////////////////////////////
// helpers

// maps getATTR()/setATTR() to a PRBool member 'mATTR'
#define BOOL_GETTER_SETTER(attrib)                \
  NS_IMETHODIMP                                   \
  zapStunMessage::Get##attrib(PRBool *a##attrib)  \
  {                                               \
    *a##attrib = m##attrib;                       \
    return NS_OK;                                 \
  }                                               \
  NS_IMETHODIMP                                   \
  zapStunMessage::Set##attrib(PRBool a##attrib)   \
  {                                               \
    m##attrib = a##attrib;                        \
    return NS_OK;                                 \
  }

// maps getATTR()/setATTR()/getATTRPort()/setATTRPort() to a PRNetAddr
// member 'mATTR'
#define ADDRESS_GETTER_SETTERS(attrib)                            \
  NS_IMETHODIMP                                                   \
  zapStunMessage::Get##attrib##Port(PRUint16 *a##attrib##Port)    \
  {                                                               \
    *a##attrib##Port = PR_ntohs(PR_NetAddrInetPort(&m##attrib));  \
    return NS_OK;                                                 \
  }                                                               \
  NS_IMETHODIMP                                                   \
  zapStunMessage::Set##attrib##Port(PRUint16 a##attrib##Port)     \
  {                                                               \
    NS_ASSERTION(&m##attrib.inet.port == &m##attrib.ipv6.port,    \
                 "oops - strange union layout");                  \
    m##attrib.inet.port = PR_htons(a##attrib##Port);              \
    return NS_OK;                                                 \
  }                                                               \
                                                                  \
  NS_IMETHODIMP                                                   \
  zapStunMessage::Get##attrib(nsACString & a##attrib)             \
  {                                                               \
    char buf[64];                                                 \
    if (PR_NetAddrToString(&m##attrib, buf, 64) != PR_SUCCESS)    \
      return NS_ERROR_FAILURE;                                    \
    a##attrib = buf;                                              \
    return NS_OK;                                                 \
  }                                                               \
  NS_IMETHODIMP                                                   \
  zapStunMessage::Set##attrib(const nsACString & a##attrib)       \
  {                                                               \
    if (PR_StringToNetAddr(                                       \
          PromiseFlatCString(a##attrib).get(),                    \
          &m##attrib) != PR_SUCCESS)                              \
      return NS_ERROR_FAILURE;                                    \
    return NS_OK;                                                 \
  }

// append a PRUint16 value to the given buffer:
void AppendUint16(nsCString& buf, PRUint16 val)
{
  PRUint32 l = buf.Length();
  buf.SetLength(l + 2);
  PRUint16* p = (PRUint16*)(buf.BeginWriting() + l);
  *p = val;
}

// append a PRUint32 value to the given buffer:
void AppendUint32(nsCString& buf, PRUint32 val)
{
  PRUint32 l = buf.Length();
  buf.SetLength(l + 4);
  PRUint32* p = (PRUint32*)(buf.BeginWriting() + l);
  *p = val;
}

// append a type and length message attribute header to given buffer:
void AppendAttributeHeader(nsCString& buf, PRUint16 type, PRUint16 length)
{
  PRUint32 l = buf.Length();
  buf.SetLength(l + 4);
  PRUint16* p = (PRUint16*)(buf.BeginWriting() + l);
  *p++ = PR_htons(type);
  *p = PR_htons(length);
}

// append an address attribute to the given buffer:
nsresult AppendAddressAttrib(nsCString& buf, PRUint16 type, const PRNetAddr& addr)
{
  if (addr.raw.family == PR_AF_INET) {
    AppendAttributeHeader(buf, type, 8);
    AppendUint16(buf, PR_htons(0x01)); // family = IPv4
    AppendUint16(buf, addr.inet.port); // port (already in net order)
    AppendUint32(buf, addr.inet.ip); // ip address (already in net order)
  }
  else if (addr.raw.family == PR_AF_INET6) {
    AppendAttributeHeader(buf, 0x0001, 20);
    AppendUint16(buf, PR_htons(0x02)); // family = IPv6
    AppendUint16(buf, addr.ipv6.port); // port (already in net order)
    buf.Append((char*)addr.ipv6.ip.pr_s6_addr, 16); // ip address (already in net order)
  }
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

// Deserialize an address attrib into a PRNetAddr structure:
nsresult ParseAddressAttrib(PRNetAddr& addr, const PRUint16* p, PRUint16 length)
{
  PRUint16 family = 0x00FF & PR_ntohs(p[0]);
  NS_ASSERTION(&addr.inet.port == &addr.ipv6.port,
               "oops - strange union layout");
  addr.inet.port = p[1]; // stored in network order
  if (family == 0x01) { // IPv4
    if (length != 8) return NS_ERROR_FAILURE; // XXX ZAP_STUN_ATTRIBUTE_ERROR
    addr.raw.family = PR_AF_INET;
    addr.inet.ip = *(PRUint32*)(p+2);
  }
  else if (family == 0x02) { // IPv6
    if (length != 20) return NS_ERROR_FAILURE; // XXX ZAP_STUN_ATTRIBUTE_ERROR
    addr.raw.family = PR_AF_INET6;
    memcpy(addr.ipv6.ip.pr_s6_addr, p+2, 16);          
  }
  else {
    #ifdef DEBUG
      printf("zapStunMessage::ParseAddressAttrib: parsing error\n");
#endif
    return NS_ERROR_FAILURE; // XXX ZAP_STUN_ATTRIBUTE_ERROR
  }
  
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapStunMessage

zapStunMessage::zapStunMessage(nsISupports* outer)
    : mHasMappedAddressAttrib(PR_FALSE),
      mHasResponseAddressAttrib(PR_FALSE),
      mHasChangeRequestAttrib(PR_FALSE),
      mHasSourceAddressAttrib(PR_FALSE),
      mHasChangedAddressAttrib(PR_FALSE),
      mHasUsernameAttrib(PR_FALSE),
      mHasPasswordAttrib(PR_FALSE),
      mHasMessageIntegrityAttrib(PR_FALSE),
      mHasErrorCodeAttrib(PR_FALSE),
      mHasUnknownAttributesAttrib(PR_FALSE),
      mHasReflectedFromAttrib(PR_FALSE),
      mHasXORMappedAddressAttrib(PR_FALSE),
      mHasXOROnlyAttrib(PR_FALSE),
      mHasServerAttrib(PR_FALSE)
{
#ifdef DEBUG
  //printf("zapStunMessage@%p::zapStunMessage(%p)\n", this, outer);
#endif
  NS_INIT_AGGREGATED(outer);
}

zapStunMessage::~zapStunMessage()
{
#ifdef DEBUG
  //printf("zapStunMessage@%p::~zapStunMessage()\n", this);
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_AGGREGATED(zapStunMessage)

NS_INTERFACE_MAP_BEGIN_AGGREGATED(zapStunMessage)
  NS_INTERFACE_MAP_ENTRY(zapIStunMessage)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// zapIStunMessage implementation:

/* ACString serialize (); */
NS_IMETHODIMP
zapStunMessage::Serialize(nsACString & _retval)
{
  nsCString message;
  
  // allocate header; we write it later when we know the total message
  // length:
  message.SetLength(20);

  // write attributes:
  if (mHasMappedAddressAttrib) {
    AppendAddressAttrib(message, 0x0001, mMappedAddress);
  }

  if (mHasResponseAddressAttrib) {
    AppendAddressAttrib(message, 0x0002, mResponseAddress);
  }
  
  if (mHasChangeRequestAttrib) {
    AppendAttributeHeader(message, 0x0003, 4);
    PRUint16 flags = 0;
    if (mChangeRequestChangePort) flags |= 0x0002;
    if (mChangeRequestChangeIP)   flags |= 0x0004;
    AppendUint16(message, 0);
    AppendUint16(message, PR_htons(flags));
  }
  
  if (mHasSourceAddressAttrib) {
    AppendAddressAttrib(message, 0x0004, mSourceAddress);
  }
  
  if (mHasChangedAddressAttrib) {
    AppendAddressAttrib(message, 0x0005, mChangedAddress);
  }
  
  if (mHasUsernameAttrib) {
    AppendAttributeHeader(message, 0x0006, mUsername.Length());
    message += mUsername;
  }
  
  if (mHasPasswordAttrib) {
    AppendAttributeHeader(message, 0x0007, mPassword.Length());
    message += mPassword;
  }
  
  if (mHasErrorCodeAttrib) {
    AppendAttributeHeader(message, 0x0009, 4 + mErrorCodeReasonPhrase.Length());
    AppendUint16(message, 0);
    AppendUint16(message, PR_htons(mErrorCode));
    message += mErrorCodeReasonPhrase;
  }
  
  if (mHasUnknownAttributesAttrib) {
    //XXX
  }
  
  if (mHasReflectedFromAttrib) {
    AppendAddressAttrib(message, 0x000b, mReflectedFrom);
  }
  
  if (mHasXORMappedAddressAttrib) {
    if (mXORMappedAddress.raw.family == PR_AF_INET) {
      AppendAttributeHeader(message, 0x8020, 8);
      AppendUint16(message, PR_htons(0x01)); // family = IPv4
      AppendUint16(message,
                   mXORMappedAddress.inet.port ^ *(PRUint16*)mTransactionID);
      AppendUint32(message,
                   mXORMappedAddress.inet.ip ^ *(PRUint32*)mTransactionID);
    }
    else if (mXORMappedAddress.raw.family == PR_AF_INET6) {
      AppendAttributeHeader(message, 0x8020, 20);
      AppendUint16(message, PR_htons(0x02)); // family = IPv6
      AppendUint16(message,
                   mXORMappedAddress.ipv6.port ^ *(PRUint16*)mTransactionID);
      AppendUint16(message,
                   mXORMappedAddress.ipv6.ip.pr_s6_addr32[0] ^
                   ((PRUint32*)mTransactionID)[0]);
      AppendUint16(message,
                   mXORMappedAddress.ipv6.ip.pr_s6_addr32[1] ^
                   ((PRUint32*)mTransactionID)[1]);
      AppendUint16(message,
                   mXORMappedAddress.ipv6.ip.pr_s6_addr32[2] ^
                   ((PRUint32*)mTransactionID)[2]);
      AppendUint16(message,
                   mXORMappedAddress.ipv6.ip.pr_s6_addr32[3] ^
                   ((PRUint32*)mTransactionID)[3]);
    }
    else return NS_ERROR_FAILURE;
  }
  
  if (mHasXOROnlyAttrib) {
    AppendAttributeHeader(message, 0x0021, 0);
  }
  
  if (mHasServerAttrib) {
    AppendAttributeHeader(message, 0x8022, mServer.Length());
    message += mServer;
  }

  // write header:
  PRUint16 *p = (PRUint16*)message.BeginWriting();
  p[0] = PR_htons(mMessageType);
  // take into account message-integrity attrib for length:
  p[1] = PR_htons(mHasMessageIntegrityAttrib ?
                  message.Length()-20+24 :
                  message.Length()-20);
  memcpy(p+2, mTransactionID, 16);

  // MESSAGE-INTEGRITY NEEDS TO BE LAST:
  if (mHasMessageIntegrityAttrib) {
    AppendAttributeHeader(message, 0x0008, 20);
    message += Substring((char*)mMessageIntegrity,
                         (char*)mMessageIntegrity+20);
  }  
  
  _retval = message;
  
  return NS_OK;
}

/* void deserialize (in ACString packet, [array, size_is (count)] out unsigned short unknownAttribs, out unsigned long count); */
nsresult
zapStunMessage::Deserialize(const nsACString& packet,
                            PRUint16** unknownAttribs,
                            PRUint32* count)
{
  if (count)
    *count = 0;
  if (unknownAttribs)
    *unknownAttribs = nsnull;
  
  PRUint32 l = packet.Length();
  // we need at least a header:
  if (l < 20) {
#ifdef DEBUG
    printf("zapStunMessage::Deserialize: message too short (%d)\n", l);
#endif
    return NS_ERROR_FAILURE;
  }
  // the length must be a multiple of 4:
  if (l % 4) {
#ifdef DEBUG
    printf("zapStunMessage::Deserialize: message length (%d) not a multiple of 4\n", l);
#endif
    return NS_ERROR_FAILURE;
  }
  
  const PRUint16* p = (const PRUint16*)packet.BeginReading();
  const PRUint16* endp = p + packet.Length();

  nsresult rv = NS_OK;
  
  // Parse header:

  // message type
  rv = SetMessageType(PR_ntohs(p[0]));
  NS_ENSURE_SUCCESS(rv, rv);
  // check framing
  if (l - 20 != PR_ntohs(p[1])) return NS_ERROR_FAILURE; 
  // transaction id
  memcpy(mTransactionID, p+2, 16);

  // skip to end of 20 byte header:
  p += 10;

  // Parse attributes:
  while (p < endp) {
    PRUint16 type = PR_ntohs(*p++);
    PRUint16 length = PR_ntohs(*p++);
    if (length % 4) {
#ifdef DEBUG
      printf("zapStunMessage::Deserialize: attribute %d framing error (%d)\n", type, length);
#endif
      // attribute framing error
      return NS_ERROR_FAILURE;
    }
    if (p+(length>>1) > endp) {
#ifdef DEBUG
      printf("zapStunMessage::Deserialize: message framing error\n");
#endif
      // message framing error
      return NS_ERROR_FAILURE;
    }
    
    switch (type) {
      case 0x0001: // MAPPED-ADDRESS
        mHasMappedAddressAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mMappedAddress, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case 0x0002: // RESPONSE-ADDRESS
        mHasResponseAddressAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mResponseAddress, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case 0x0003: // CHANGE-REQUEST
        if (length != 4) return NS_ERROR_FAILURE;
        mHasChangeRequestAttrib = PR_TRUE;
        {
          PRUint16 flags = PR_ntohs(p[1]);
          mChangeRequestChangeIP   = ((flags & 0x0004) == 0x0004);
          mChangeRequestChangePort = ((flags & 0x0002) == 0x0002);
        }
        break;
      case 0x0004: // SOURCE-ADDRESS
        mHasSourceAddressAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mSourceAddress, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case 0x0005: // CHANGED-ADDRESS
        mHasChangedAddressAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mChangedAddress, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case 0x0006: // USERNAME
        mHasUsernameAttrib = PR_TRUE;
        mUsername.Assign((char*)p, length);
        break;
      case 0x0007: // PASSWORD
        mHasPasswordAttrib = PR_TRUE;
        mPassword.Assign((char*)p, length);
        break;
      case 0x0008: // MESSAGE-INTEGRITY
        if (length != 20) return NS_ERROR_FAILURE;
        mHasMessageIntegrityAttrib = PR_TRUE;
        memcpy(mMessageIntegrity, p, 20);
        break;
      case 0x0009: // ERROR-CODE
        mHasErrorCodeAttrib = PR_TRUE;
        if (length < 4) return NS_ERROR_FAILURE;
        mErrorCode = PR_ntohs(p[1]);
        if (mErrorCode < 100 || mErrorCode > 699) return NS_ERROR_FAILURE;
        mErrorCodeReasonPhrase.Assign((char*)(p+2), length-4);
        break;
      case 0x000a: // UNKNOWN-ATTRIBUTES
        //XXX
        break;
      case 0x000b: // REFLECTED-FROM
        mHasReflectedFromAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mReflectedFrom, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      case 0x8020: // XOR-MAPPED-ADDRESS
        mHasXORMappedAddressAttrib = PR_TRUE;
        rv = ParseAddressAttrib(mXORMappedAddress, p, length);
        NS_ENSURE_SUCCESS(rv, rv);
        // port/ip & transactionID are stored in net order, so it's
        // ok to xor on words:
        mXORMappedAddress.inet.port ^= *(PRUint16*)mTransactionID;
        if (mXORMappedAddress.raw.family == PR_AF_INET) {
          // see comment above
          mXORMappedAddress.inet.ip ^= *(PRUint32*)mTransactionID;
        }
        else {
          // PR_AF_INET6
          // see comment above
          mXORMappedAddress.ipv6.ip.pr_s6_addr32[0] ^= ((PRUint32*)mTransactionID)[0];
          mXORMappedAddress.ipv6.ip.pr_s6_addr32[1] ^= ((PRUint32*)mTransactionID)[1];
          mXORMappedAddress.ipv6.ip.pr_s6_addr32[2] ^= ((PRUint32*)mTransactionID)[2];
          mXORMappedAddress.ipv6.ip.pr_s6_addr32[3] ^= ((PRUint32*)mTransactionID)[3];
        }
        break;
      case 0x0021: // XOR-ONLY
        if (length != 0) return NS_ERROR_FAILURE;
        mHasXOROnlyAttrib = PR_TRUE;
        break;
      case 0x8022: // SERVER
        mHasServerAttrib = PR_TRUE;
        mServer.Assign((char*)p, length);
        break;
      default:
        if (type < 0x8000) {
          // this is a mandatory attribute
          // XXX enter type into unknownAttribs array
        }
        // otherwise -> ignore
        break;
    }
    // skip on to next attrib:
    p += (length >> 1);
  }

  return rv;
}

/* attribute unsigned short messageType; */
NS_IMETHODIMP
zapStunMessage::GetMessageType(PRUint16 *aMessageType)
{
  *aMessageType = mMessageType;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetMessageType(PRUint16 aMessageType)
{
  switch (aMessageType) {
    case BINDING_REQUEST_MESSAGE:
    case BINDING_RESPONSE_MESSAGE:
    case BINDING_ERROR_RESPONSE_MESSAGE:
    case SHARED_SECRET_REQUEST_MESSAGE:
    case SHARED_SECRET_RESPONSE_MESSAGE:
    case SHARED_SECRET_ERROR_RESPONSE_MESSAGE:
      mMessageType = aMessageType;
      break;
    default:
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/* attribute ACString transactionID; */
NS_IMETHODIMP
zapStunMessage::GetTransactionID(nsACString & aTransactionID)
{
  aTransactionID = Substring((char*)mTransactionID,
                             (char*)mTransactionID+16);
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetTransactionID(const nsACString & aTransactionID)
{
  if (aTransactionID.Length() != 16)
    return NS_ERROR_FAILURE;
  memcpy(mTransactionID, aTransactionID.BeginReading(), 16);
  return NS_OK;
}

/* void initTransactionID (); */
NS_IMETHODIMP
zapStunMessage::InitTransactionID()
{
  for (int i=0; i<8; ++i)
    ((PRUint16*)mTransactionID)[i] = rand();
  return NS_OK;
}



/* attribute boolean hasMappedAddressAttrib; */
BOOL_GETTER_SETTER(HasMappedAddressAttrib)

/* attribute unsigned short mappedAddressPort; */
/* attribute ACString mappedAddress; */
ADDRESS_GETTER_SETTERS(MappedAddress)

/* attribute boolean hasResponseAddressAttrib; */
BOOL_GETTER_SETTER(HasResponseAddressAttrib)
  
/* attribute unsigned short responseAddressPort; */
/* attribute ACString responseAddress; */
ADDRESS_GETTER_SETTERS(ResponseAddress)

/* attribute boolean hasChangeRequestAttrib; */
BOOL_GETTER_SETTER(HasChangeRequestAttrib)

/* attribute boolean changeRequestChangeIP; */
BOOL_GETTER_SETTER(ChangeRequestChangeIP)

/* attribute boolean changeRequestChangePort; */
BOOL_GETTER_SETTER(ChangeRequestChangePort)

/* attribute boolean hasSourceAddressAttrib; */
BOOL_GETTER_SETTER(HasSourceAddressAttrib)

/* attribute unsigned short sourceAddressPort; */
/* attribute ACString sourceAddress; */
ADDRESS_GETTER_SETTERS(SourceAddress)

/* attribute boolean hasChangedAddressAttrib; */
BOOL_GETTER_SETTER(HasChangedAddressAttrib)

/* attribute unsigned short changedAddressPort; */
/* attribute ACString changedAddress; */
ADDRESS_GETTER_SETTERS(ChangedAddress)

/* attribute boolean hasUsernameAttrib; */
BOOL_GETTER_SETTER(HasUsernameAttrib)
  
/* attribute ACString username; */
NS_IMETHODIMP
zapStunMessage::GetUsername(nsACString & aUsername)
{
  aUsername = mUsername;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetUsername(const nsACString & aUsername)
{
  // make sure username length is a multiple of 4:
  if (aUsername.Length() % 4)
    return NS_ERROR_FAILURE;
  mUsername = aUsername;
  return NS_OK;
}

/* attribute boolean hasPasswordAttrib; */
BOOL_GETTER_SETTER(HasPasswordAttrib)

/* attribute ACString password; */
NS_IMETHODIMP
zapStunMessage::GetPassword(nsACString & aPassword)
{
  aPassword = mPassword;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetPassword(const nsACString & aPassword)
{
  // make sure password length is a multiple of 4:
  if (aPassword.Length() % 4)
    return NS_ERROR_FAILURE;
  mPassword = aPassword;
  return NS_OK;
}

/* attribute boolean hasMessageIntegrityAttrib; */
BOOL_GETTER_SETTER(HasMessageIntegrityAttrib)

/* attribute ACString messageIntegrity; */
NS_IMETHODIMP
zapStunMessage::GetMessageIntegrity(nsACString & aMessageIntegrity)
{
  aMessageIntegrity = Substring((char*)mMessageIntegrity,
                                (char*)mMessageIntegrity+20);
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetMessageIntegrity(const nsACString & aMessageIntegrity)
{
  if (aMessageIntegrity.Length() != 20)
    return NS_ERROR_FAILURE;
  memcpy(mMessageIntegrity, aMessageIntegrity.BeginReading(), 20);
  return NS_OK;
}

/* void generateMessageIntegrityAttrib (in ACString key); */
NS_IMETHODIMP
zapStunMessage::GenerateMessageIntegrityAttrib(const nsACString & key)
{
  mHasMessageIntegrityAttrib = PR_TRUE;
  // serialize message (including old message integrity attrib, so
  // that the header records the correct size):
  nsCString message;
  Serialize(message);
  // now strip the old message integrity attrib:
  message.SetLength(message.Length() - 24);

  // compute new message integrity attrib:
  nsCOMPtr<zapICryptoUtils> crypto = do_GetService("@mozilla.org/zap/cryptoutils;1");
  nsCString mi;
  crypto->ComputeSHA1HMAC(message, key, mi);
  memcpy(mMessageIntegrity, mi.get(), 20);
         
  return NS_OK;
}

/* boolean checkMessageIntegrity (in ACString key); */
NS_IMETHODIMP
zapStunMessage::CheckMessageIntegrity(const nsACString & key, PRBool *_retval)
{
  *_retval = PR_FALSE;
  
  if (!mHasMessageIntegrityAttrib)
    return NS_ERROR_FAILURE;

  nsCString mi;
  GetMessageIntegrity(mi);

  // verify:
  nsCString message;
  Serialize(message);
  message.SetLength(message.Length() - 24);
  
  // compute new message integrity attrib:
  nsCOMPtr<zapICryptoUtils> crypto = do_GetService("@mozilla.org/zap/cryptoutils;1");
  nsCString my_mi;
  crypto->ComputeSHA1HMAC(message, key, my_mi);
  if (mi == my_mi)
    *_retval = PR_TRUE;
  
  return NS_OK;
}

/* attribute boolean hasErrorCodeAttrib; */
BOOL_GETTER_SETTER(HasErrorCodeAttrib)

/* attribute ACString errorCode; */
NS_IMETHODIMP
zapStunMessage::GetErrorCode(nsACString & aErrorCode)
{
  char buf[4];
  PR_snprintf(buf, 4, "%d", mErrorCode);
  aErrorCode = buf;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetErrorCode(const nsACString & aErrorCode)
{
  long code = atoi(PromiseFlatCString(aErrorCode).get());
  if (code < 100 || code > 699) return NS_ERROR_FAILURE;
  mErrorCode = (PRUint16)code;
  return NS_OK;
}

/* attribute ACString errorCodeReasonPhrase; */
NS_IMETHODIMP
zapStunMessage::GetErrorCodeReasonPhrase(nsACString & aErrorCodeReasonPhrase)
{
  aErrorCodeReasonPhrase = mErrorCodeReasonPhrase;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetErrorCodeReasonPhrase(const nsACString & aErrorCodeReasonPhrase)
{
  PRUint32 padding = (4 - aErrorCodeReasonPhrase.Length() % 4) % 4;
  mErrorCodeReasonPhrase = aErrorCodeReasonPhrase;
  mErrorCodeReasonPhrase.Append("    ", padding);
  return NS_OK;
}

/* attribute boolean hasUnknownAttributesAttrib; */
BOOL_GETTER_SETTER(HasUnknownAttributesAttrib)

/* void getUnknownAttributes (out unsigned long count, [array, size_is (count), retval] out unsigned short attribs); */
NS_IMETHODIMP
zapStunMessage::GetUnknownAttributes(PRUint32 *count, PRUint16 **attribs)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setUnknownAttributes ([array, size_is (count)] in unsigned short attribs, in unsigned long count); */
NS_IMETHODIMP
zapStunMessage::SetUnknownAttributes(PRUint16 *attribs, PRUint32 count)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean hasReflectedFromAttrib; */
BOOL_GETTER_SETTER(HasReflectedFromAttrib)
  
/* attribute unsigned short reflectedFromPort; */
/* attribute ACString reflectedFrom; */
ADDRESS_GETTER_SETTERS(ReflectedFrom)

/* attribute boolean hasXORMappedAddressAttrib; */
BOOL_GETTER_SETTER(HasXORMappedAddressAttrib)
  
/* attribute unsigned short XORMappedAddressPort; */
/* attribute ACString XORMappedAddress; */
ADDRESS_GETTER_SETTERS(XORMappedAddress)

/* attribute boolean hasXOROnlyAttrib; */
BOOL_GETTER_SETTER(HasXOROnlyAttrib)
  
/* attribute boolean hasServerAttrib; */
BOOL_GETTER_SETTER(HasServerAttrib)
  
/* attribute ACString server; */
NS_IMETHODIMP
zapStunMessage::GetServer(nsACString & aServer)
{
  aServer = mServer;
  return NS_OK;
}
NS_IMETHODIMP
zapStunMessage::SetServer(const nsACString & aServer)
{
  PRUint32 padding = (4 - aServer.Length() % 4) % 4;
  mServer = aServer;
  mServer.Append("    ", padding);
  return NS_OK;
}
