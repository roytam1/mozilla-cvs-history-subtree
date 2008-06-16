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
 * Portions created by the Initial Developer are Copyright (C) 2005-2008
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

#include "zapNetUtils.h"
#include "zapStunMessage.h"
#include "prnetdb.h"
#include "zapIStunResolver.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "zapNetItfAddress.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"

#ifdef XP_WIN
#include "zapNetHelpersWin.h"
#else
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

////////////////////////////////////////////////////////////////////////
// helpers

// XXX something like this should be in NSPR
static PRBool sockaddr_to_prnetaddr(const struct sockaddr *sa, PRNetAddr *pa)
{
  if (!sa) {
    NS_WARNING("Null socket");
    return PR_FALSE;
  }
  
  switch (sa->sa_family) {
  case AF_INET:
    memcpy(pa, sa, sizeof(struct sockaddr_in));
    pa->raw.family = PR_AF_INET;
    break;
  case AF_INET6:
    memcpy(pa, sa, sizeof(struct sockaddr_in6));
    pa->raw.family = PR_AF_INET6;
    break;
  default:
    NS_WARNING("Unsupported address family");
    return PR_FALSE;
  }
  return PR_TRUE;
}


////////////////////////////////////////////////////////////////////////
// zapNetUtils

zapNetUtils::zapNetUtils()
{
#ifdef XP_WIN
  InitNetHelpersWin();
#endif
}

zapNetUtils::~zapNetUtils()
{
#ifdef XP_WIN
  FreeNetHelpersWin();
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapNetUtils)
NS_IMPL_THREADSAFE_RELEASE(zapNetUtils)

NS_INTERFACE_MAP_BEGIN(zapNetUtils)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapINetUtils)
  NS_INTERFACE_MAP_ENTRY(zapINetUtils)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapINetUtils methods:

/* zapIStunMessage createStunMessage (); */
NS_IMETHODIMP
zapNetUtils::CreateStunMessage(zapIStunMessage **_retval)
{
  zapStunMessage* message = new zapStunMessage(nsnull);
  if (!message) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(message);
  *_retval = message;
  
  return NS_OK;
}

/* zapIStunMessage deserializeStunPacket (in ACString packet, [array, size_is (count)] out unsigned short unknownAttribs, out unsigned long count); */
NS_IMETHODIMP
zapNetUtils::DeserializeStunPacket(const nsACString & packet,
                                      PRUint16 **unknownAttribs,
                                      PRUint32 *count,
                                      zapIStunMessage **_retval)
{
  *_retval = nsnull;
  zapStunMessage* message = new zapStunMessage(nsnull);
  if (!message) {
    return NS_ERROR_FAILURE;
  }
  NS_ADDREF(message);

  nsresult rv = message->Deserialize(packet, unknownAttribs, count);
  if (NS_FAILED(rv)) {
    NS_RELEASE(message);
    return rv;
  }

  *_retval = message;
  
  return NS_OK;
}

/* PRInt32 snoopStunPacket (in ACString buffer); */
NS_IMETHODIMP
zapNetUtils::SnoopStunPacket(const nsACString & buffer, PRInt32 *_retval)
{
  PRUint32 l = buffer.Length();
  if (l == 0) {
    *_retval = -1;
  }
  else if (l == 1) {
    const PRUint8* p = (const PRUint8*)buffer.BeginReading();
    if (p[0] == 1 || p[0] == 0)
      *_retval = -1;
    else
      *_retval = -2;
  }
  else {
    const PRUint16* p = (const PRUint16*)buffer.BeginReading();
    
    // check message type:
    PRUint16 type = PR_ntohs(p[0]);
    if (type != 0x0001 &&
        type != 0x0101 &&
        type != 0x0111 &&
        type != 0x0002 &&
        type != 0x0102 &&
        type != 0x0112) {
      *_retval = -2;
    }
    else if (l < 4) {
      *_retval = 0;
    }
    else {
      // determine length (including 20 byte header):
      *_retval = PR_ntohs(p[1]) + 20;
    }
  }
  
  return NS_OK;
}
  
/* void resolveMappedAddress (in zapIStunAddressResolveListener listener, in ACString stunServer); */
NS_IMETHODIMP
zapNetUtils::ResolveMappedAddress(zapIStunAddressResolveListener *listener, const nsACString & stunServer)
{
  // XXX THE STUN-RESOLVER IS NOT THREADSAFE
  
  nsCOMPtr<zapIStunResolver> resolver = do_CreateInstance("@mozilla.org/zap/stun-resolver;1");
  if (!resolver) return NS_ERROR_FAILURE;
  return resolver->ResolveMappedAddress(listener, stunServer);
}


/* ACString getPrimaryHostAddress(); */
NS_IMETHODIMP
zapNetUtils::GetPrimaryHostAddress(nsACString & retval)
{
  nsCOMPtr<nsIDNSService> dnsService = do_GetService("@mozilla.org/network/dns-service;1");
  nsCString hostName;
  dnsService->GetMyHostName(hostName);
  nsCOMPtr<nsIDNSRecord> dnsRecord;
  if (NS_SUCCEEDED(dnsService->Resolve(hostName, 0,
                                       getter_AddRefs(dnsRecord))) &&
      dnsRecord) {
    dnsRecord->GetNextAddrAsString(retval);
  }
  else {
    retval = NS_LITERAL_CSTRING("127.0.0.1");
  }

#ifdef XP_UNIX
  if (retval == NS_LITERAL_CSTRING("127.0.0.1")) {
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in*) &ifr.ifr_addr;
    memset(&ifr, 0, sizeof(ifr));
    int sfd;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) >= 0 ) {
      strcpy(ifr.ifr_name, "eth0");
      sin->sin_family = AF_INET;
      if (ioctl(sfd, SIOCGIFADDR, &ifr) == 0) {
        retval = inet_ntoa(sin->sin_addr);
      }
    }
  }
#endif
  
  return NS_OK;
}

/* void getNetItfAddresses (out unsigned long count, [array, size_is (count), retval] out zapINetItfAddress results); */
NS_IMETHODIMP
zapNetUtils::GetNetItfAddresses(PRUint32 *count, zapINetItfAddress ***results)
{
  *count = 0;
  *results = nsnull;
  
  nsCOMArray<zapINetItfAddress> addrs;
  nsresult rv = GetNetItfAddressesCOMArray(&addrs);
  NS_ENSURE_SUCCESS(rv, rv);
  if (addrs.Count() == 0) return NS_OK;

  *results = static_cast<zapINetItfAddress**>(nsMemory::Alloc(sizeof(zapINetItfAddress*) * addrs.Count()));
  NS_ENSURE_TRUE(*results, NS_ERROR_OUT_OF_MEMORY);

  *count = addrs.Count();
  for (int i=0; i<*count; ++i) {
    NS_ADDREF((*results)[i] = addrs[i]);
  }
  
  return NS_OK;
}

/* [noscript] void getNetItfAddressesCOMArray (in NetItfAddrArray results); */
NS_IMETHODIMP
zapNetUtils::GetNetItfAddressesCOMArray(nsCOMArray<zapINetItfAddress> * results)
{
  results->Clear();

  struct ifaddrs *addrs;
  if (getifaddrs(&addrs) < 0)
    return NS_ERROR_FAILURE;
 
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  struct ifaddrs *p = addrs;
  while (p) {
    struct ifaddrs *c = p;
    p = c->ifa_next;
    
    nsRefPtr<zapNetItfAddress> nia = new zapNetItfAddress();
    if (!nia) goto bail;
    if (!sockaddr_to_prnetaddr(c->ifa_addr, &nia->mNetAddr)) {
      // unsupported address family -> skip
      continue;
    }
    nia->mItfName = c->ifa_name;
    nia->mFlags = c->ifa_flags;

    if (!results->AppendObject(static_cast<zapINetItfAddress*>(nia))) goto bail;
  }

  // success!
  rv = NS_OK;
  // fall through to clean up addrs
  
 bail:
  freeifaddrs(addrs);
  
  return rv;
}
