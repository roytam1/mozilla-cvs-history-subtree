/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsIContentHandler_h__
#define nsIContentHandler_h__

#include "nsISupports.h"
#include "net.h"

/////////////////////////////////////////////////////////////////////////
//
// XXX This is some level of abstraction higher than just a plain old
// nsIPluginClass. I'm not at all sure what this interface will look like,
// but I wanted to make sure it was separated out from the plugin class
// itself.
//

/**
 * An object capable of handling net content
 */
class nsIContentHandler : public nsISupports
{
public:
    NS_IMETHOD_(NET_StreamClass*)
    CreateStream(FO_Present_Types format_out,
                 URL_Struct* urls,
                 MWContext* cx) = 0;
};



// XXX Remember to get a GUID for this...
#define NS_ICONTENTHANDLER_IID                       \
{ /* 5d852ef0-eebc-11d1-85b1-00805f2e4dff */         \
    0x5d852ef0,                                      \
    0xeebc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x2e, 0x4d, 0xff} \
}



#endif // nsIContentHandler_h__
