/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsURL.h"

static const PRInt32 DEFAULT_PORT = -1;

nsURL::nsURL(const char* i_URL):m_Port(DEFAULT_PORT),mRefCnt(0)
{
    NS_PRECONDITION( (0 != i_URL), "No url string specified!");

    if (i_URL)
    {
        m_URL = new char[PL_strlen(i_URL) + 1];
        if (m_URL)
        {
            PL_strcpy(m_URL, i_URL);
        }
    }
    else
    {
        m_URL = new char[1];
        *m_URL = '\0';
    }

    for (int i=0, i<TOTAL_PARTS, ++i) 
    {
        for (int j=0, j<TOTAL_PARTS, ++j)
        {
            m_Position[i][j] = -1;
        }
    }

    Parse();
}

nsURL::~nsURL()
{
    if (m_URL)
    {
        delete[] m_URL;
        m_URL = 0;
    }
}


NS_IMPL_ADDREF(nsURL);

PRBool
nsURL::Equals(const nsICoolURL* i_URL) const
{
    return PR_FALSE;
}

nsresult
nsURL::Extract(const char* o_OutputString, nsURL::Part i_id)
{
}

nsresult
nsURL::GetDocument(const char* *o_Data)
{
}


nsresult nsURL::OpenStream(nsIInputStream* *o_InputStream)
{
}

nsresult nsURL::Parse(void)
{
}


nsresult
nsURL::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 \

    if (aIID.Equals(NS_IIURL_IID)) {
        *aInstancePtr = (void*) ((nsIURL*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(NS_IIURI_IID)) {
        *aInstancePtr = (void*) ((nsIURI*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}
 
NS_IMPL_RELEASE(nsURL);

nsresult
nsURL::SetHost(const char* i_Host)
{
}

nsresult
nsURL::SetPort(PRInt32 i_Port)
{
    if (m_Port != i_Port)
    {
        m_Port = i_Port;
        if (DEFAULT_PORT == m_Port)
        {
            //Just REMOVE the earliar one. 
            NS_NOTYETIMPLEMENTED("Eeeks!");
        }
        else
        {
            //Insert the string equivalent 
            NS_NOTYETIMPLEMENTED("Eeeks!");
        }
        Parse();
    }
}

nsresult
nsURL::SetPreHost(const char* i_PreHost)
{
}

nsresult
nsURL::SetScheme(const char* i_Scheme)
{
}

nsresult
nsURL::ToString(const char* *o_URLString)
{
}
