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

#include "nsIBinaryInputStream.h"
#include "nsIBinaryOutputStream.h"
#include "nsCOMPtr.h"

class nsBinaryOutputStream : public nsIBinaryOutputStream
{
public:
    nsBinaryOutputStream(nsIOutputStream *aStream);
    ~nsBinaryOutputStream() {};

private:
    
    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsIBaseStream methods
    NS_DECL_NSIBASESTREAM

    // nsIOutputStream methods
    NS_DECL_NSIOUTPUTSTREAM

    // nsIBinaryOutputStream methods
    NS_DECL_NSIBINARYOUTPUTSTREAM
    
protected:
    nsCOMPtr<nsIOutputStream> mOutputStream;
};

class nsBinaryInputStream : public nsIBinaryInputStream
{
public:
    nsBinaryInputStream(nsIInputStream *aStream);
    ~nsBinaryInputStream() {};

private:
    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsIBaseStream methods
    NS_DECL_NSIBASESTREAM

    // nsIInputStream methods
    NS_DECL_NSIINPUTSTREAM

    // nsIBinaryInputStream methods
    NS_DECL_NSIBINARYINPUTSTREAM
    
protected:
    nsCOMPtr<nsIInputStream> mInputStream;
};

