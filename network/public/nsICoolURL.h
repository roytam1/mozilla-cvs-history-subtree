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

#ifndef _nsICoolURL_h_
#define _nsICoolURL_h_

/* 
    The nsICoolURL class is an interface to the URL behaviour. 
    This follows the URL spec at-
    
         http://www.w3.org/Addressing/URL/url-spec.txt
    
    For the purpose of this class, here is the most elaborate form of a URL
    and its corresponding parts-
    
         ftp://username:password@hostname:portnumber/pathname
         \ /   \               / \      / \        /\       /
          -     ---------------   ------   --------  -------
          |            |             |        |         |
          |            |             |        |        Path
          |            |             |       Port         
          |            |            Host
          |         PreHost            
        Scheme

    The URL class extends the URI behaviour by providing a mechanism 
    for consumers to retreive (and on protocol specific levels put) documents
    defined by the URI.
 */

#include "nsIURI.h"
#include "nsIInputStream.h"
//#include "nsIConnection.h"

// {6DA32F00-BD64-11d2-B00F-006097BFC036}
static const nsIID NS_ICOOLURL_IID = 
{ 0x6da32f00, 0xbd64, 0x11d2, { 0xb0, 0xf, 0x0, 0x60, 0x97, 0xbf, 0xc0, 0x36 } };

class nsICoolURL : public nsIURI
{

public:
    
    //Core action functions
    /* 
        Note: The OpenStream function also opens a connection using 
        the available information in the URL. This is the same as 
        calling OpenInputStream on the connection returned from 
        OpenConnection. Note that this stream doesn't contain any 
        header information either. 
    */
    NS_IMETHOD          OpenStream(nsIInputStream* *o_InputStream) = 0;

    /*
        The GetDocument function BLOCKS till the data is made available
        or an error condition is encountered. The return type is the overall
        success status which depends on the protocol implementation. 
        Its just a convenience function that internally sets up a temp 
		stream in memory and buffers everything. Note that this 
		mechanism strips off the headers and only the raw data is 
		copied to the passed string.

        TODO - return status? 
    */
    NS_IMETHOD          GetDocument(const char* *o_Data) = 0;

    /* 
        The OpenConnection method sets up the connection as decided by the 
        protocol implementation. This may then be used to get various 
        connection specific details like the input and the output streams 
        associated with the connection, or the header information by querying
        on the connection type which will be protocol specific.

		TODO- Should this be the same as the protocol instance, since 
		some protocols may decide to reuse a "connection" per se?
    */
    //NS_IMETHOD          OpenConnection(nsIConnection* o_Connection) = 0;

    static const nsIID& IID() { return NS_ICOOLURL_IID; };

};


extern NS_NET
    nsICoolURL* CreateURL(const char* i_URL);


#endif /* _nsICoolURL_h_ */
