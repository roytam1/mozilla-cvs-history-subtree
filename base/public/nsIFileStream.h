/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsIFileStream_h___
#define nsIFileStream_h___

#include "nsIInputStream.h"
#include "nsIOutputStream.h"

class nsFileSpec;

/* a6cf90e6-15b3-11d2-932e-00805f8add32 */
#define NS_IFILEINPUTSTREAM_IID \
{ 0xa6cf90e6, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }
    
/* a6cf90e7-15b3-11d2-932e-00805f8add32 */
#define NS_IFILEOUTPUTSTREAM_IID \
{ 0xa6cf90e7, 0x15b3, 0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

//========================================================================================
class nsIFileInputStream : public nsIInputStream
//========================================================================================
{
	// File-specific methods need to be added here - that's why I didn't just
	// provide special factory methods for file streams that
	// would merely return nsIInputStream.
}; // class nsIFileInputStream

//========================================================================================
class nsIFileOutputStream : public nsIOutputStream
//========================================================================================
{
	// File-specific methods need to be added here - that's why I didn't just
	// provide special factory methods for file streams that
	// would merely return nsIOutputStream.
}; // class nsIFileOutputStream


//----------------------------------------------------------------------------------------
extern "C" NS_BASE nsresult NS_NewInputConsoleStream(
    nsIFileInputStream** aInstancePtrResult);
    // Factory method to get an nsInputStream from the console.

//----------------------------------------------------------------------------------------
extern "C" NS_BASE nsresult NS_NewInputFileStream(
    nsIFileInputStream** aInstancePtrResult,
    const nsFileSpec& inFile,
    PRInt32 nsprMode /*Default = PR_RDONLY*/,
    PRInt32 accessMode /*Default = 0700 (octal)*/);
    // Factory method to get an nsInputStream from a file
    // (see nsFileStream.h for more info on the parameters).

//----------------------------------------------------------------------------------------
extern "C" NS_BASE nsresult NS_NewOutputConsoleStream(
    nsIFileOutputStream** aInstancePtrResult);
    // Factory method to get an nsOutputStream to the console.

//----------------------------------------------------------------------------------------
extern "C" NS_BASE nsresult NS_NewOutputFileStream(
    nsIFileOutputStream** aInstancePtrResult,
    const nsFileSpec& inFile,
    PRInt32 nsprMode /*default = (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE)*/,
    PRInt32 accessMode /*Default = 0700 (octal)*/);
    // Factory method to get an nsOutputStream to a file.
    // (see nsFileStream.h for more info on the parameters).

#endif /* nsIFileStream_h___ */
