/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#define NS_IMPL_IDS
#include "nsID.h"

// {84B0F181-C6C7-11d2-B3B0-00805F8A6670}
NS_DECLARE_ID(kIPlatformCharsetIID, 
 0x84b0f181, 0xc6c7, 0x11d2, 0xb3, 0xb0, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 );

// Interface ID for our Unicode Decode Helper interface
// {9CC39FF0-DD5D-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kIUnicodeDecodeHelperIID,
  0x9cc39ff0, 0xdd5d, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our UnicodeDecoderHelper implementation
// {9CC39FF1-DD5D-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kUnicodeDecodeHelperCID, 
  0x9cc39ff1, 0xdd5d, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Interface ID for our Unicode Encode Helper interface
// {D8E6B700-CA9D-11d2-8AA9-00600811A836}
NS_DECLARE_ID(kIUnicodeEncodeHelperIID,
  0xd8e6b700, 0xca9d, 0x11d2, 0x8a, 0xa9, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our UnicodeEncoderHelper implementation
// {1767FC50-CAA4-11d2-8AA9-00600811A836}
NS_DECLARE_ID(kUnicodeEncodeHelperCID, 
  0x1767fc50, 0xcaa4, 0x11d2, 0x8a, 0xa9, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);
