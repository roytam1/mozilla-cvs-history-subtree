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
#include <stdlib.h>
#include "plstr.h"

static const PRInt32 DEFAULT_PORT = -1;

#define SET_POSITION(__part,__start,__length)   \
{                                               \
    m_Position[__part][0] = __start;            \
    m_Position[__part][1] = __length;           \
}


NS_NET
nsICoolURL* CreateURL(const char* i_URL)
{
    return new nsURL(i_URL);
}

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

    for (int i=0; i<TOTAL_PARTS; ++i) 
    {
        for (int j=0; j<2; ++j)
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
nsURL::Equals(const nsIURI* i_URI) const
{
    return PR_FALSE;
}

nsresult
nsURL::Extract(const char* *o_OutputString, nsURL::Part i_id) const
{
	int i = (int) i_id;
	if (o_OutputString)
	{
		*o_OutputString = new char(m_Position[i][1]+1);
        //if (!*o_OutputString)
        //    return NS_ERROR; //TODO

        char* dest = (char*) *o_OutputString;
        char* src = m_URL + m_Position[i][0];

		for (int j=0; j<m_Position[i][1]; ++j)
		{
            *dest++ = *src++; 
		}
        *dest = '\0';
        return NS_OK;
	}
    return NS_OK; //TODO error
}

nsresult
nsURL::GetDocument(const char* *o_Data)
{
    NS_PRECONDITION( (0 != m_URL), "GetDocument called on empty url!");
    *o_Data = "Here is your data";
    return NS_OK;
}


nsresult nsURL::OpenStream(nsIInputStream* *o_InputStream)
{
    NS_PRECONDITION( (0 != m_URL), "OpenStream called on empty url!");
    return NS_OK;
}

nsresult nsURL::Parse(void)
{
    NS_PRECONDITION( (0 != m_URL), "Parse called on empty url!");
    if (!m_URL)
        return NS_OK; //TODO change to error

    int len = PL_strlen(m_URL);
    char* colon = PL_strchr(m_URL, ':');
    char* at = PL_strchr(m_URL, '@');
    char* slash = PL_strchr (m_URL, '/');

    if (colon)
    {
        // If the first colon is followed by // then its definitely a spec
        if ((*++colon == '/') && (*++colon == '/'))
        {
            SET_POSITION(SCHEME, 0, (colon -2 - m_URL));
            ++colon; // Now points to start of anything after ://
            slash = PL_strchr(colon, '/');

            if (at)
            {
                SET_POSITION(PREHOST, (colon - m_URL), (at - colon));
                ++at; // at now points to start of anything after @
                colon = PL_strchr(at, ':'); // possible port number
                if (colon)
                {
                    SET_POSITION(HOST, (at - m_URL), (colon - at));
                    if (slash)
                    {
                        ++colon;
                        int portLen = slash - colon;
                        char* port = new char[portLen +1];
                        if (!port)
                            return NS_OK; //TODO error.
                        PL_strncpy(port, colon, portLen);
                        *(port + portLen) = '\0';
                        m_Port = atoi(port);
                        delete[] port;

                        SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                    }
                }
                else 
                {
                    if (slash)
                    {
                        SET_POSITION(HOST, (at - m_URL), (slash - at));
                        SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                    }
                    else //everything past the @ is the host
                    {
                        SET_POSITION(HOST, (at - m_URL), len - (at - m_URL));
                    }
                }
            }

        }
        else 
        {
            // If an @ is present then there has to be a username with an optional password
            // before the colon. 
            if (at > colon)
            {
                SET_POSITION(SCHEME, 0, (colon - m_URL));
            }
            // If there is a . before the colon than this might be the port colon
            // and hence all the stuff in the beginning is host (with maybe prehost)
            // I am thinking foo.com:80/path or user@foo.com:80/path
            char* dot = PL_strchr(m_URL, '.');
            if ((dot) && (dot < colon))
            {
                //Check for prehost stuff
                char* at = PL_strchr(m_URL, '@');
                if ((at) && (at < dot))
                {
                    SET_POSITION(PREHOST, 0, (at - m_URL));
                }
            }
            else // its just the host
            {
                SET_POSITION(SCHEME, 0, (colon - m_URL));
            }
        }
    }
    else // No colons, so its probably just a prehost,host/path format
    {
        if (at) 
        {
            SET_POSITION(PREHOST, 0, (at - m_URL));
        }
        else
        {
            if (slash)
            {
                SET_POSITION(HOST, 0, (slash - m_URL));
                SET_POSITION(PATH, (slash - m_URL), (len - (slash - m_URL)));
            }
        }
    }
    return NS_OK;
}


nsresult
nsURL::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr)
        return NS_ERROR_NULL_POINTER;

    *aInstancePtr = NULL;
    
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

    if (aIID.Equals(NS_ICOOLURL_IID)) {
        *aInstancePtr = (void*) ((nsICoolURL*)this);
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(NS_IURI_IID)) {
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
    return NS_OK;
}

nsresult
nsURL::SetPath(const char* i_Path)
{
    return NS_OK;
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
    return NS_OK;
}

nsresult
nsURL::SetPreHost(const char* i_PreHost)
{
    return NS_OK;
}

nsresult
nsURL::SetScheme(const char* i_Scheme)
{
    return NS_OK;
}
