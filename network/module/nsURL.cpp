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
    nsICoolURL* pReturn = new nsURL(i_URL);
    if (pReturn)
        pReturn->AddRef();
    return pReturn;
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

void
nsURL::ExtractPortFrom(int start, int length)
{
    char* port = new char[length +1];
    if (!port)
    {
        //TODO error.
    }
    PL_strncpy(port, m_URL+start, length);
    *(port + length) = '\0';
    m_Port = atoi(port);
    delete[] port;
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

    //Remember to remove leading/trailing spaces, etc. TODO

    int len = PL_strlen(m_URL);
    char* colon = PL_strchr(m_URL, ':');
    char* at = PL_strchr(m_URL, '@');
    char* slash = PL_strchr (m_URL, '/');

    if (colon)
    {
        // If the first colon is followed by // then its definitely a spec
        if ((*(colon+1) == '/') && (*(colon+2) == '/')) // e.g. http://
        {
            SET_POSITION(SCHEME, 0, (colon - m_URL));
            colon += 3; // Now points to start of anything after ://
            slash = PL_strchr(colon, '/');
            
            if (at) // http://username@host...
            {
                SET_POSITION(PREHOST, (colon - m_URL), (at - colon));
                ++at; // at now points to start of anything after @
    
                colon = PL_strchr(colon, ':'); //Find the next colon
                if (colon) // Could be http://user:pass@hostname.com: or http://user@hostname:80/path...
                {
                    if (colon < at) // Its a password : and we don't parse that so skip it...
                        colon = PL_strchr(at, ':'); // Assumption-more colons before @ is anyway sick.

                    if (colon>at) // This is a colon after the @ sign so cant be password.
                                // Its of port. http://username@host:80...
                    {
                        SET_POSITION(HOST, (at - m_URL), (colon - at));
                        int portLen;
                        ++colon; //Moved to start of port now

                        if (slash) // http://username@host:80/path
                        {
                            portLen = slash - colon;
                            SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                        }
                        else // Everything else is just port http://username@host:80
                        {
                            portLen = len-(colon - m_URL);
                        }
                        ExtractPortFrom((colon-m_URL), portLen);
                    }
                }
                else //No colons found 
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
        else // no construct like http:// so it probably just starts with username, or hostname or just path...
        {
            if (at > colon) // http:user@hostname - note that this can never be user:pass@hostname 
                            // as it clashes with the first and a more likely candidate form.
                            // This is very debatable an issue and I will revisit it after I get some 
                            // Usability guys' feedback. I am more inclined at making it the latter though...
            {
                SET_POSITION(SCHEME, 0, (colon - m_URL));
                SET_POSITION(PREHOST, (colon+1 - m_URL), (at - colon-1));
                //There is still a possibility of a port here so lets check
                //TODO change to PL_stnbrk or whatever that is...
                colon = PL_strchr(at, ':');
                slash = PL_strchr(at, '/');
                if (colon) 
                {
                    if ((slash) && (colon < slash)) // http:username@hostname:80/path
                    {
                        SET_POSITION(HOST, (at+1 - m_URL), colon - (at+1));
                        ExtractPortFrom(colon+1-m_URL, slash-colon-1);
                        SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                    }
                    else //There is a colon but no slash so everything after the colon is port.
                        // // http:username@hostname:80
                    {
                        SET_POSITION(HOST, (at+1 - m_URL), colon - (at+1));
                        ExtractPortFrom(colon+1 - m_URL, len - (colon+1 - m_URL));
                    }
                }
                else
                {
                    if (slash) // http:user@hostname/path
                    {
                        SET_POSITION(HOST, (at +1 - m_URL), (slash - at));
                        SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                    }
                    else // http:user@hostname
                    {
                        SET_POSITION(HOST, (at+1 - m_URL), len - (at+1 - m_URL));
                    }
                }
            }
            else    // user@host:80/something
            {
                SET_POSITION(PREHOST, 0, (at - m_URL));
                SET_POSITION(HOST, (at-m_URL+1), (colon - at-1));
                slash = PL_strchr(colon, '/');
                if (slash)
                {
                    ExtractPortFrom(colon+1 -m_URL, (slash - colon));
                    SET_POSITION(PATH, (slash - m_URL), len - (slash - m_URL));
                }
                else // user@host:81
                {
                    ExtractPortFrom(colon+1 -m_URL, len - (colon + 1 - m_URL));
                }
            }
        }
    }
    else // No colons, so its probably just a prehost,host/path format like username@host/path
    {
        if (at) //username@host/path
        {
            SET_POSITION(PREHOST, 0, (at - m_URL));
            slash = PL_strchr(at, '/');
            if (slash) //username@host/path 
            {
                SET_POSITION(HOST, (at +1 - m_URL), (slash - at-1));
                SET_POSITION(PATH, (slash - m_URL), (len - (slash - m_URL)));
            }
            else    //username@host
            {
                SET_POSITION(HOST, (at+1 - m_URL), (len - (at+1 - m_URL)));
            }
        }
        else //host/path
        {
            if (slash) //host/path or even /path
            {
                SET_POSITION(HOST, 0, (slash - m_URL));
                SET_POSITION(PATH, (slash - m_URL), (len - (slash - m_URL)));
            }
            else    //host
            {
                SET_POSITION(HOST, 0, len);
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

#ifdef DEBUG
nsresult 
nsURL::DebugString(const char* *o_String) const
{
    if (!m_URL)
        return NS_OK;
    const char* tempBuff = 0;
    char* tmp = new char[PL_strlen(m_URL) + 64];
    char* tmp2 = tmp;
    for (int i=PL_strlen(m_URL)+64; i>0 ; --i)
        *tmp2++ = '\0';
/*///
    PL_strcpy(tmp, "m_URL= ");
    PL_strcat(tmp, m_URL);
    
    GetScheme(&tempBuff);
    PL_strcat(tmp, "\nScheme= ");
    PL_strcat(tmp, tempBuff);
    
    GetPreHost(&tempBuff);
    PL_strcat(tmp, "\nPreHost= ");
    PL_strcat(tmp, tempBuff);
    
    GetHost(&tempBuff);
    PL_strcat(tmp, "\nHost= ");
    PL_strcat(tmp, tempBuff);
    
    char* tempPort = new char[6];
    if (tempPort)
    {
        itoa(m_Port, tempPort, 10);
        PL_strcat(tmp, "\nPort= ");
        PL_strcat(tmp, tempPort);
        delete[] tempPort;
    }
    else
        PL_strcat(tmp, "\nPort couldn't be conv'd to string!");

    GetPath(&tempBuff);
    PL_strcat(tmp, "\nPath= ");
    PL_strcat(tmp, tempBuff);
    PL_strcat(tmp, "\n");
/*///
    char delimiter[] = ",";
#define TEMP_AND_DELIMITER PL_strcat(tmp, tempBuff); \
    PL_strcat(tmp, delimiter)

    PL_strcpy(tmp, m_URL);
    PL_strcat(tmp, "\n");

    GetScheme(&tempBuff);
    TEMP_AND_DELIMITER;

    GetPreHost(&tempBuff);
    TEMP_AND_DELIMITER;

    GetHost(&tempBuff);
    TEMP_AND_DELIMITER;

    char* tempPort = new char[6];
    if (tempPort)
    {
        itoa(m_Port, tempPort, 10);
        PL_strcat(tmp, tempPort);
        PL_strcat(tmp, delimiter);
        delete[] tempPort;
    }
    else
        PL_strcat(tmp, "\nPort couldn't be conv'd to string!");

    GetPath(&tempBuff);
    PL_strcat(tmp, tempBuff);
    PL_strcat(tmp, "\n");

//*/// 
    *o_String = tmp;
    return NS_OK;
}
#endif // DEBUG