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

#ifndef __ZAP_NETHELPERSWIN__
#define __ZAP_NETHELPERSWIN__

#include "prtypes.h"

////////////////////////////////////////////////////////////////////////
//  Network compatibility functions for Windows


//----------------------------------------------------------------------
// Windows headers

// We need to force compilation for Windows XP SP1 or higher to pull
// in the definitions of IP_ADAPTER_UNICAST_ADDRESS and
// IP_ADAPTER_PREFIX. We have a check in getifaddrs() to ascertain
// that the structure returned from GetAdaptersAddresses is in fact
// a version we can process.
#if (_WIN32_WINNT < 0x501)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//----------------------------------------------------------------------
// Initialization

void InitNetHelpersWin();
void FreeNetHelpersWin();

//----------------------------------------------------------------------
// getifaddrs/freeifaddrs:
// Emulate the eponymous UNIX function.
// Shortcomings:
// - Only works on Windows XP and later. Fails gracefully on earlier
//   versions.
// - Only IPv4 and IPv6 sockets.
// - ifa_netmask/ifa_dstaddr/ifa_data are not filled in.
// - Only flags IFF_UP, IFF_LOOPBACK and IFF_MULTICAST are filled in.

struct ifaddrs
{
  struct ifaddrs *ifa_next;
  char *ifa_name;
  PRUint32 ifa_flags;
  struct sockaddr *ifa_addr;
  struct sockaddr *ifa_netmask;
  struct sockaddr *ifa_dstaddr;
  void *ifa_data;  
};

int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifp);

#endif // __ZAP_NETHELPERSWIN__
