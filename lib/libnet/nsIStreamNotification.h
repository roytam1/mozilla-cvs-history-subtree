/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIStreamNotification_h___
#define nsIStreamNotification_h___

#include "prtypes.h"
#include "nsISupports.h"

/* forward declaration */
class nsIInputStream;


/* 45d234d0-c6c9-11d1-bea2-00805f8a66dc */
#define NS_ISTREAMNOTIFICATION_IID   \
{ 0x45d234d0, 0xc6c9, 0x11d1, \
  {0xbe, 0xa2, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }


struct nsIStreamNotification : public nsISupports 
{
    NS_IMETHOD GetBindInfo(void)                      = 0;
    NS_IMETHOD OnProgress(void)                       = 0;
    NS_IMETHOD OnStartBinding(void)                   = 0;
    NS_IMETHOD OnDataAvailable(nsIInputStream *pIStream)   = 0;
    NS_IMETHOD OnStopBinding(void)                    = 0;
};


#endif /* nsIStreamNotification_h___ */
