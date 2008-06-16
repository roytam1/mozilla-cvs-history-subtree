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
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alexander Fritze <alex@croczilla.com> (original author)
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

#include "zapNetHelpersWin.h"
#include <iphlpapi.h>
#include "prlink.h"
#include "nsDebug.h"
#include <stdlib.h>

typedef DWORD 
(WINAPI *GetAdaptersAddressesFunc)(ULONG family, 
				   DWORD flags, 
				   PVOID reserved,
				   PIP_ADAPTER_ADDRESSES aaddrs,
				   PULONG size);

static PRLibrary* sPHLPAPI = NULL;
static GetAdaptersAddressesFunc sGetAdaptersAddresses = NULL;

void InitNetHelpersWin()
{
  sPHLPAPI = PR_LoadLibrary("iphlpapi.dll");
  if (!sPHLPAPI) 
    return;

  sGetAdaptersAddresses = 
    (GetAdaptersAddressesFunc)PR_FindSymbol(sPHLPAPI,
					    "GetAdaptersAddresses");

  if (!sGetAdaptersAddresses)
    FreeNetHelpersWin();
}

void FreeNetHelpersWin()
{
  if (!sPHLPAPI) return;
  PR_UnloadLibrary(sPHLPAPI);
  sPHLPAPI = NULL;
  sGetAdaptersAddresses = NULL;
}
 
// These implementations of getifaddrs and freeifaddrs are (very
// loosely) adapted from
// http://www.opensource.apple.com/darwinsource/tarballs/other/mDNSResponder-107.6.tar.gz,
// Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
// (Apache License, http://www.apache.org/licenses/LICENSE-2.0)

int getifaddrs(struct ifaddrs **ifap)
{
  if (!sGetAdaptersAddresses) {
    NS_WARNING("zapNetHelpersWin lib not init'ed or unsupported version of Windows");
    return -1;
  }

  DWORD gaa_flags = /* GAA_FLAG_INCLUDE_PREFIX | */
                    GAA_FLAG_SKIP_ANYCAST | 
                    GAA_FLAG_SKIP_MULTICAST |
                    GAA_FLAG_SKIP_DNS_SERVER |
                    GAA_FLAG_SKIP_FRIENDLY_NAME;

  ULONG aa_list_size;
  IP_ADAPTER_ADDRESSES *aa_list;

  int retries = 0;
  while (1) {
    aa_list_size = 0;
    if (sGetAdaptersAddresses(AF_UNSPEC, gaa_flags, NULL, NULL, &aa_list_size) != 
	ERROR_BUFFER_OVERFLOW) {
      NS_WARNING("Unexpected return value from GetAdaptersAddresses");
      return -1;
    }

    aa_list = (IP_ADAPTER_ADDRESSES*)malloc(aa_list_size);
    if (!aa_list) {
      NS_ERROR("Out of memory");
      return -1;
    }

    if (sGetAdaptersAddresses(AF_UNSPEC, gaa_flags, NULL, 
			      aa_list, &aa_list_size) ==
	ERROR_SUCCESS)
      break;

    // There might have been interfaces added/removed between the two
    // call to GetAdaptersAddresses, making the second call
    // fail. We'll retry up to 10 times.
    free(aa_list);
    if (++retries >= 10) {
      NS_ERROR("Too many retries for GetAdaptersAddresses");
      return -1;
    }
  }

  // At this point the aa_list datastructure has been filled. Now pick
  // it apart and reassemble the information into our output
  // datastructure:
  int err = -1;
  struct ifaddrs *head = NULL;
  struct ifaddrs **next = &head;

  // On Windows XP pre-SP1, GetAdaptersAddresses returns an
  // incompatible structure, which we can identify based on its size
  // being smaller than what we expect. We bail under these
  // conditions.  
  // XXX As long as we don't try to parse the prefix list (which has
  // some dodgy semantics anyway), we're safe to continue.
  //   if (aa_list && aa_list->Length < sizeof(IP_ADAPTER_ADDRESSES)) {
  //     NS_WARNING("GetAdaptersAddresses returned incompatible struct");
  //     goto bail;
  //   }

  for (IP_ADAPTER_ADDRESSES *aa = aa_list; aa; aa = aa->Next) {
    // Skip tunnel interfaces. XXX Do we really want to do that?
    if (aa->IfType == IF_TYPE_TUNNEL)
      continue;

    // Add each address as a separate interface to emulate the way
    // getifaddrs works.
    IP_ADAPTER_UNICAST_ADDRESS *addr;
    for (addr = aa->FirstUnicastAddress; addr; addr = addr->Next) {
      int family = addr->Address.lpSockaddr->sa_family;
      if (family != AF_INET && family != AF_INET6) continue;

      struct ifaddrs *ifa = (struct ifaddrs *)calloc(1, sizeof(struct ifaddrs));
      if (!ifa) goto bail;
      *next = ifa;
      next = &ifa->ifa_next;

      // Interface name
      ifa->ifa_name = strdup(aa->AdapterName);

      // Interface flags
      ifa->ifa_flags = 0;
      if (aa->OperStatus == IfOperStatusUp) 
	ifa->ifa_flags |= IFF_UP;
      if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) 
	ifa->ifa_flags |= IFF_LOOPBACK;
      if (!(aa->Flags & IP_ADAPTER_NO_MULTICAST)) 
	ifa->ifa_flags |= IFF_MULTICAST;
      // XXX IFF_POINTTOPOINT

      // Address
      ifa->ifa_addr = (struct sockaddr*)calloc(1, addr->Address.iSockaddrLength);
      if (!ifa->ifa_addr) goto bail;
      memcpy(ifa->ifa_addr, addr->Address.lpSockaddr, addr->Address.iSockaddrLength);

      // XXX NETMASK

      // XXX BROADCAST ADDRESS
    }
  }

  // Only if we end up at this line have we been successful. Clear
  // error flag and fall through to cleanup code.
  *ifap = head;
  head = NULL;
  err = 0;

 bail:
  if (head)
    freeifaddrs(head);
  if (aa_list)
    free(aa_list);

  return err;
}

void freeifaddrs(struct ifaddrs *ifp)
{
  while (ifp) {
    struct ifaddrs *p = ifp;
    ifp = p->ifa_next;
    
    if (p->ifa_name) free(p->ifa_name);
    if (p->ifa_addr) free(p->ifa_addr);
    if (p->ifa_netmask) free(p->ifa_netmask);
    if (p->ifa_dstaddr) free(p->ifa_dstaddr);
    if (p->ifa_data) free(p->ifa_data);
    free(p);
  }
}
