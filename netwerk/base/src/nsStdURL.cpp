/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

#include "nsStdURL.h"
#include "nscore.h"
#include "nsCRT.h"
#include "prmem.h"
#include "prprf.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"

static NS_DEFINE_CID(kStdURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kThisStdURLImplementationCID,
                     NS_THIS_STANDARDURL_IMPLEMENTATION_CID);

//----------------------------------------

// Helper function to extract the port # from a string
// PR_sscanf handles spaces and non-digits correctly
static PRInt32 ExtractPortFrom(char* src)
{
    PRInt32 returnValue = -1;
    return (0 < PR_sscanf(src, "%d", &returnValue)) ? returnValue : -1;
}

// Replace all /./ with a /
// Also changes all \ to /
// But only till #?;
static void ReplaceMess(char* io_Path)
{
    /* Stolen from the old netlib's mkparse.c.
     *
     * modifies a url of the form   /foo/../foo1  ->  /foo1
     *                       and    /foo/./foo1   ->  /foo/foo1
     *                       and    /foo/foo1/..  ->  /foo/
     */
    char *fwdPtr = io_Path;
    char *urlPtr = io_Path;

    for(; (*fwdPtr != '\0') &&
            (*fwdPtr != ';') &&
            (*fwdPtr != '?') &&
            (*fwdPtr != '#'); ++fwdPtr)
    {
        if (*fwdPtr == '\\')
            *fwdPtr = '/';
        if (*fwdPtr == '/' && *(fwdPtr+1) == '.' &&
            (*(fwdPtr+2) == '/' || *(fwdPtr+2) == '\\'))
        {
            // remove . followed by slash or a backslash
            fwdPtr += 1;
        }
        else if(*fwdPtr == '/' && *(fwdPtr+1) == '.' && *(fwdPtr+2) == '.' &&
                (*(fwdPtr+3) == '/' ||
                    *(fwdPtr+3) == '\0' ||
                    *(fwdPtr+3) == ';' ||   // This will take care of likes of
                    *(fwdPtr+3) == '?' ||   //    foo/bar/..#sometag
                    *(fwdPtr+3) == '#' ||
                    *(fwdPtr+3) == '\\'))
        {
            // remove foo/..
            // reverse the urlPtr to the previous slash
            if(urlPtr != io_Path)
                urlPtr--; // we must be going back at least by one
            for(;*urlPtr != '/' && urlPtr != io_Path; urlPtr--)
                ;  // null body

            // forward the fwd_prt past the ../
            fwdPtr += 2;
            // special case if we have reached the end to preserve the last /
            if (*fwdPtr == '.' && *(fwdPtr+1) == '\0')
                urlPtr +=1;
        }
        else
        {
            // copy the url incrementaly
            *urlPtr++ = *fwdPtr;
        }
    }
    // Copy remaining stuff past the #?;
    for (; *fwdPtr != '\0'; ++fwdPtr)
    {
        *urlPtr++ = *fwdPtr;
    }
    *urlPtr = '\0';  // terminate the url

    /*
     *  Now lets remove trailing . case
     *     /foo/foo1/.   ->  /foo/foo1/
     */

    if ((urlPtr > (io_Path+1)) && (*(urlPtr-1) == '.') && (*(urlPtr-2) == '/'))
        *(urlPtr-1) = '\0';
}

//----------------------------------------

nsStdURL::nsStdURL(const char* i_Spec, nsISupports* outer)
    : mScheme(nsnull),
      mPreHost(nsnull),
      mHost(nsnull),
      mPort(-1),
      mPath(nsnull),
      mDirectory(nsnull),
      mFileName(nsnull),
      mParam(nsnull),
      mQuery(nsnull),
      mRef(nsnull)
{
    // Skip leading spaces
    char* fwdPtr= (char*) i_Spec;
    while (fwdPtr && (*fwdPtr != '\0') && (*fwdPtr == ' '))
        fwdPtr++;
    // Remove trailing spaces
    if (fwdPtr) {
        char* bckPtr= (char*)fwdPtr + PL_strlen(fwdPtr) -1;
        if (*bckPtr == ' ') {
            while ((bckPtr-fwdPtr) >= 0 && (*bckPtr == ' ')) {
                bckPtr--;
            }
            *(bckPtr+1) = '\0';
        }
    }
    mSpec = fwdPtr ? nsCRT::strdup(fwdPtr) : nsnull;
    NS_INIT_AGGREGATED(outer);
    if (fwdPtr)
        Parse();
}

nsStdURL::nsStdURL(const nsStdURL& otherURL)
    : mPort(otherURL.mPort)
{
    mSpec = otherURL.mSpec ? nsCRT::strdup(otherURL.mSpec) : nsnull;
    mScheme = otherURL.mScheme ? nsCRT::strdup(otherURL.mScheme) : nsnull;
    mPreHost = otherURL.mPreHost ? nsCRT::strdup(otherURL.mPreHost) : nsnull;
    mHost = otherURL.mHost ? nsCRT::strdup(otherURL.mHost) : nsnull;
    mPath = otherURL.mPath ? nsCRT::strdup(otherURL.mPath) : nsnull;
    mDirectory = otherURL.mDirectory ? nsCRT::strdup(otherURL.mDirectory) : nsnull;
    mFileName = otherURL.mFileName ? nsCRT::strdup(otherURL.mFileName) : nsnull;
    mParam = otherURL.mParam ? nsCRT::strdup(otherURL.mParam) : nsnull;
    mQuery = otherURL.mQuery ? nsCRT::strdup(otherURL.mQuery) : nsnull;
    mRef= otherURL.mRef ? nsCRT::strdup(otherURL.mRef) : nsnull;
    NS_INIT_AGGREGATED(nsnull); // Todo! How?
}

nsStdURL&
nsStdURL::operator=(const nsStdURL& otherURL)
{
    mSpec = otherURL.mSpec ? nsCRT::strdup(otherURL.mSpec) : nsnull;
    mScheme = otherURL.mScheme ? nsCRT::strdup(otherURL.mScheme) : nsnull;
    mPreHost = otherURL.mPreHost ? nsCRT::strdup(otherURL.mPreHost) : nsnull;
    mHost = otherURL.mHost ? nsCRT::strdup(otherURL.mHost) : nsnull;
    mPath = otherURL.mPath ? nsCRT::strdup(otherURL.mPath) : nsnull;
    mDirectory = otherURL.mDirectory ? nsCRT::strdup(otherURL.mDirectory) : nsnull;
    mFileName = otherURL.mFileName ? nsCRT::strdup(otherURL.mFileName) : nsnull;
    mParam = otherURL.mParam ? nsCRT::strdup(otherURL.mParam) : nsnull;
    mQuery = otherURL.mQuery ? nsCRT::strdup(otherURL.mQuery) : nsnull;
    mRef= otherURL.mRef ? nsCRT::strdup(otherURL.mRef) : nsnull;
    NS_INIT_AGGREGATED(nsnull); // Todo! How?
    return *this;
}

PRBool
nsStdURL::operator==(const nsStdURL& otherURL) const
{
    PRBool retValue = PR_FALSE;
    ((nsStdURL*)(this))->Equals((nsIURI*)&otherURL,&retValue);
    return retValue;
}

nsStdURL::~nsStdURL()
{
    CRTFREEIF(mScheme);
    CRTFREEIF(mPreHost);
    CRTFREEIF(mHost);
    CRTFREEIF(mPath);
    CRTFREEIF(mRef);
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mSpec);
    CRTFREEIF(mDirectory);
    CRTFREEIF(mFileName);
}

NS_IMPL_AGGREGATED(nsStdURL);

NS_IMETHODIMP
nsStdURL::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "no instance pointer");
    if(!aInstancePtr)
        return NS_ERROR_INVALID_POINTER;

    if (aIID.Equals(NS_GET_IID(nsISupports)))
        *aInstancePtr = GetInner();
    else if (aIID.Equals(kThisStdURLImplementationCID) ||   // used by Equals
            aIID.Equals(NS_GET_IID(nsIURL)) ||
            aIID.Equals(NS_GET_IID(nsIURI)))
        *aInstancePtr = NS_STATIC_CAST(nsIURL*, this);
     else {
        *aInstancePtr = nsnull;
          return NS_NOINTERFACE;
    }
    NS_ADDREF((nsISupports*)*aInstancePtr);
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::Equals(nsIURI *i_OtherURI, PRBool *o_Equals)
{
    PRBool eq = PR_FALSE;
    if (i_OtherURI) {
        nsXPIDLCString spec;
        nsresult rv = i_OtherURI->GetSpec(getter_Copies(spec));
        if (NS_FAILED(rv)) return rv;
        eq = nsAutoString(spec).Equals(this->mSpec);
    }
    *o_Equals = eq;
    return NS_OK;
}

NS_IMETHODIMP
nsStdURL::Clone(nsIURI **o_URI)
{
    nsStdURL* url = new nsStdURL(*this); /// TODO check outer?
    if (url == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    nsresult rv= NS_OK;

    *o_URI = url;
    NS_ADDREF(url);
    return rv;
}

nsresult
nsStdURL::ParseScheme(char* i_Spec, char* *o_Scheme, char* *o_PreHost,
                       char* *o_Host, PRInt32 *o_Port, char* *o_Path)
{
    nsresult rv = NS_OK;

    NS_PRECONDITION( (nsnull != i_Spec), "Parse called on empty url!");
    if (!i_Spec)
        return NS_ERROR_MALFORMED_URI;

    int len = PL_strlen(i_Spec);
    if (len >= 2 && *i_Spec == '/' && *(i_Spec+1) == '/') // No Scheme
    {
        rv = ParsePreHost(i_Spec, o_PreHost, o_Host, o_Port, o_Path);
        return rv;
    }

    static const char delimiters[] = "/:@?"; //this order is optimized.
    char* brk = PL_strpbrk(i_Spec, delimiters);

    if (!brk) // everything is a host
    {
        rv = ExtractString(i_Spec, o_Host, len);
        return rv;
    } else
        len = PL_strlen(brk);

    switch (*brk)
    {
    case '/' :
    case '?' :
        // If the URL starts with a slash then everything is a path
        if (brk == i_Spec)
        {
            rv = ParsePath(brk, o_Path);
            return rv;
        }
        else // The first part is host, so its host/path
        {
            rv = ExtractString(i_Spec, o_Host, (brk - i_Spec));
            if (NS_FAILED(rv))
                return rv;
            rv = ParsePath(brk, o_Path);
            return rv;
        }
        break;
    case ':' :
        if (len >= 2 && *(brk+1) == '/' && *(brk+2) == '/') {
            // Standard http://...
            rv = ExtractString(i_Spec, o_Scheme, (brk - i_Spec));
            if (NS_FAILED(rv))
                return rv;
            rv = ParsePreHost(brk+1, o_PreHost, o_Host,
                              o_Port, o_Path);
            if (rv == NS_ERROR_MALFORMED_URI) {
                // or not ? Try something else
                CRTFREEIF(*o_PreHost);
                CRTFREEIF(*o_Host);
                *o_Port = -1;
                rv = ParsePath(brk+3, o_Path);
            }
            return rv;
        } else {
            if ( len >= 2 && *(brk+1) == '/' && *(brk+2) != '/') {
                // May be it is file:/....
                rv = ExtractString(i_Spec, o_Scheme, (brk - i_Spec));
                if (NS_FAILED(rv))
                    return rv;
                rv = ParsePath(brk+1, o_Path);
                return rv;
            } else {
                // Could be host:port, so try conversion to number
                PRInt32 port = ExtractPortFrom(brk+1);
                if (port <= 0)
                {
                    // No, try normal procedure
                    rv = ExtractString(i_Spec, o_Scheme, (brk - i_Spec));
                    if (NS_FAILED(rv))
                        return rv;
                    rv = ParsePreHost(brk+1, o_PreHost, o_Host,
                                  o_Port, o_Path);
                    if (rv == NS_ERROR_MALFORMED_URI) {
                        // Try something else
                        CRTFREEIF(*o_PreHost);
                        CRTFREEIF(*o_Host);
                        *o_Port = -1;
                        rv = ParsePath(brk+1, o_Path);
                    }
                    return rv;
                } else {
                    rv = ExtractString(i_Spec, o_Host, (brk - i_Spec));
                    if (NS_FAILED(rv))
                        return rv;
                    rv = ParsePort(brk+1, o_Port, o_Path);
                    return rv;
                }
            }
        }
        break;
    case '@' :
        rv = ParsePreHost(i_Spec, o_PreHost, o_Host,
                          o_Port, o_Path);
        return rv;
        break;
    default:
        NS_ASSERTION(0, "This just can't be!");
        break;
    }
    return NS_OK;
}

nsresult
nsStdURL::ParsePreHost(char* i_Spec, char* *o_PreHost, char* *o_Host,
                        PRInt32 *o_Port, char* *o_Path)
{
    nsresult rv = NS_OK;

    // Skip leading two slashes
    char* fwdPtr= (char*) i_Spec;
    if (fwdPtr && (*fwdPtr != '\0') && (*fwdPtr == '/'))
        fwdPtr++;
    if (fwdPtr && (*fwdPtr != '\0') && (*fwdPtr == '/'))
        fwdPtr++;

    // Search for @
    static const char delimiters[] = "@";
    char* brk = PL_strpbrk(fwdPtr, delimiters);

    if (brk)
    {
        rv = ExtractString(fwdPtr, o_PreHost, (brk - fwdPtr));
        if (NS_FAILED(rv))
            return rv;
        rv = ParseHost(brk+1, o_Host, o_Port, o_Path);
        return rv;
    } else {
        rv = ParseHost(fwdPtr, o_Host, o_Port, o_Path);
        return rv;
    }
    return NS_OK;
}

nsresult
nsStdURL::ParseHost(char* i_Spec, char* *o_Host,
                     PRInt32 *o_Port, char* *o_Path)
{
    nsresult rv = NS_OK;

    int len = PL_strlen(i_Spec);
    static const char delimiters[] = ":/?"; //this order is optimized.
    char* brk = PL_strpbrk(i_Spec, delimiters);
    if (!brk) // everything is a host
    {
        rv = ExtractString(i_Spec, o_Host, len);
        return rv;
    }

    switch (*brk)
    {
    case '/' :
    case '?' :
        // Get the Host, the rest is Path
        rv = ExtractString(i_Spec, o_Host, (brk - i_Spec));
        if (NS_FAILED(rv))
            return rv;
        rv = ParsePath(brk, o_Path);
        return rv;
        break;
    case ':' :
        // Get the Host
        rv = ExtractString(i_Spec, o_Host, (brk - i_Spec));
        if (NS_FAILED(rv))
            return rv;
        rv = ParsePort(brk+1, o_Port, o_Path);
        return rv;
        break;
    default:
        NS_ASSERTION(0, "This just can't be!");
        break;
    }
    return NS_OK;
}

nsresult
nsStdURL::ParsePort(char* i_Spec, PRInt32 *o_Port, char* *o_Path)
{
    nsresult rv = NS_OK;
    static const char delimiters[] = "/?"; //this order is optimized.
    char* brk = PL_strpbrk(i_Spec, delimiters);
    if (!brk) // everything is a Port
    {
        *o_Port = ExtractPortFrom(i_Spec);
        if (*o_Port <= 0)
            return NS_ERROR_MALFORMED_URI;
        else
            return NS_OK;
    }

    switch (*brk)
    {
    case '/' :
    case '?' :
        // Get the Port, the rest is Path
        *o_Port = ExtractPortFrom(i_Spec);
        if (*o_Port <= 0)
            return NS_ERROR_MALFORMED_URI;
        rv = ParsePath(brk, o_Path);
        return rv;
        break;
    default:
        NS_ASSERTION(0, "This just can't be!");
        break;
    }
    return NS_OK;
}

nsresult
nsStdURL::ParsePath(char* i_Spec, char* *o_Path)
{
    // Just write the path and check for a starting /
    nsAutoString dir;
    if ('/' != *i_Spec)
        dir += "/";

    dir += i_Spec;

    *o_Path = dir.ToNewCString();
    return (*o_Path ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

nsresult
nsStdURL::ParsePath(char* i_Path, char* *o_Directory, char* *o_FileName,
                    char* *o_Param, char* *o_Query, char* *o_Ref)
{
    // Cleanout
    CRTFREEIF(*o_Directory);
    CRTFREEIF(*o_FileName);
    CRTFREEIF(*o_Param);
    CRTFREEIF(*o_Query);
    CRTFREEIF(*o_Ref);

    // Parse the Path into its components
    if (!i_Path)
    {
        DupString(o_Directory, "/");
        return (o_Directory ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
    }

    char* dirfile = nsnull;
    char* options = nsnull;

    int len = PL_strlen(i_Path);

    /* Factor out the optionpart with ;?# */
    static const char delimiters[] = ";?#"; // for param, query and ref
    char* brk = PL_strpbrk(i_Path, delimiters);

    if (!brk) // Everything is just path and filename
    {
        DupString(&dirfile, i_Path);
    }
    else
    {
        int dirfileLen = brk - i_Path;
        ExtractString(i_Path, &dirfile, dirfileLen);
        len -= dirfileLen;
        ExtractString(i_Path + dirfileLen, &options, len);
        brk = options;
    }

    /* now that we have broken up the path treat every part differently */
    /* first dir+file */

    char* file;

    int dlen = PL_strlen(dirfile);
    if (dlen == 0)
    {
        DupString(o_Directory, "/");
        file = dirfile;
    } else {
        ReplaceMess(dirfile);
        // check new length
        dlen = PL_strlen(dirfile);

        // First find the last slash
        file = PL_strrchr(dirfile, '/');
        if (!file)
        {
            DupString(o_Directory, "/");
            file = dirfile;
        }

        // If its not the same as the first slash then extract directory
        if (file != dirfile)
        {
            ExtractString(dirfile, o_Directory, (file - dirfile)+1);
        } else {
            DupString(o_Directory, "/");
        }
    }

    /* Extract Filename */
    if (dlen > 0) {
        // Look again if there was a slash
        char* slash = PL_strrchr(dirfile, '/');
        if (slash) {
            ExtractString(file+1, o_FileName, dlen-(file-dirfile-1));
        } else {
            // Use the full String as Filename
            ExtractString(dirfile, o_FileName, dlen);
        }
    }

    // Now take a look at the options. "#" has precedence over "?"
    // which has precedence over ";"
    if (options) {
        // Look for "#" first. Everything following it is in the ref
        brk = PL_strchr(options, '#');
        if (brk) {
            int pieceLen = len - (brk + 1 - options);
            ExtractString(brk+1, o_Ref, pieceLen);
            len -= pieceLen + 1;
            *brk = '\0';
        }

        // Now look for "?"
        brk = PL_strchr(options, '?');
        if (brk) {
            int pieceLen = len - (brk + 1 - options);
            ExtractString(brk+1, o_Query, pieceLen);
            len -= pieceLen + 1;
        }

        // Now look for ';'
        brk = PL_strchr(options, ';');
        if (brk) {
            int pieceLen = len - (brk + 1 - options);
            ExtractString(brk+1, o_Param, pieceLen);
            len -= pieceLen + 1;
            *brk = '\0';
        }
    }

    nsCRT::free(dirfile);
    nsCRT::free(options);

    return NS_OK;
}

nsresult
nsStdURL::Parse(void)
{
    // Main parser
    NS_PRECONDITION( (nsnull != mSpec), "Parse called on empty url!");
    if (!mSpec)
        return NS_ERROR_MALFORMED_URI;
    // Pars the spec
    nsresult rv = ParseScheme(mSpec, &mScheme, &mPreHost, &mHost,
                  &mPort, &mPath);
    if (NS_FAILED(rv))
        return rv;
    else {
        // Now parse the path
        rv = ParsePath(mPath, &mDirectory, &mFileName, &mParam,
                       &mQuery, &mRef);
        if (NS_FAILED(rv))
            return rv;
        else
            return ReconstructPath();
    }
}

nsresult
nsStdURL::ReconstructSpec()
{
    if (mSpec) nsCRT::free(mSpec);

    nsCAutoString finalSpec; // guaranteed to be singlebyte.
    finalSpec.SetCapacity(64);
    if (mScheme)
    {
        finalSpec = mScheme;
        finalSpec += "://";
    }
    if (mPreHost)
    {
        finalSpec += mPreHost;
        finalSpec += '@';
    }
    if (mHost)
    {
        finalSpec += mHost;
        if (-1 != mPort)
        {
            char* portBuffer = PR_smprintf(":%d", mPort);
            if (!portBuffer)
                return NS_ERROR_OUT_OF_MEMORY;
            finalSpec += portBuffer;
            PR_smprintf_free(portBuffer);
            portBuffer = 0;
        }
    }
    if (mPath)
    {
        finalSpec += mPath;
    }
    mSpec = finalSpec.ToNewCString();
    return (mSpec ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

NS_METHOD
nsStdURL::Create(nsISupports *aOuter,
    REFNSIID aIID,
    void **aResult)
{
    if (!aResult)
         return NS_ERROR_INVALID_POINTER;

     if (aOuter && !aIID.Equals(NS_GET_IID(nsISupports)))
         return NS_ERROR_INVALID_ARG;

    nsStdURL* url = new nsStdURL(nsnull, aOuter);
    if (url == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = url->AggregatedQueryInterface(aIID, aResult);
    if (NS_FAILED(rv)) {
        delete url;
        return rv;
    }

    return rv;
}

nsresult
nsStdURL::ExtractString(char* i_Src, char* *o_Dest, PRUint32 length)
{
    NS_PRECONDITION( (nsnull != i_Src), "Exract called on empty string!");
    CRTFREEIF(*o_Dest);
    *o_Dest = PL_strndup(i_Src, length);
    return (*o_Dest ? NS_OK : NS_ERROR_OUT_OF_MEMORY);
}

nsresult
nsStdURL::DupString(char* *o_Dest, const char* i_Src)
{
    if (!o_Dest)
        return NS_ERROR_NULL_POINTER;
    if (i_Src)
    {
        *o_Dest = nsCRT::strdup(i_Src);
        return (*o_Dest == nsnull) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
    }
    else
    {
        *o_Dest = nsnull;
        return NS_OK;
    }
}

NS_IMETHODIMP
nsStdURL::SetDirectory(const char* i_Directory)
{
    if (!i_Directory)
        return NS_ERROR_NULL_POINTER;

    if (mDirectory)
        nsCRT::free(mDirectory);

    nsAutoString dir;
    if ('/' != *i_Directory)
        dir += "/";

    dir += i_Directory;

    // if the last slash is missing then attach it
    char* last = (char*)i_Directory+PL_strlen(i_Directory)-1;
    if ('/' != *last) {
        dir += "/";
    }

    mDirectory = dir.ToNewCString();
    if (!mDirectory)
        return NS_ERROR_OUT_OF_MEMORY;
    return ReconstructPath();
}

NS_IMETHODIMP
nsStdURL::SetFileName(const char* i_FileName)
{
    if (!i_FileName)
        return NS_ERROR_NULL_POINTER;

    //Cleanout param, query and ref
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mRef);

    //If it starts with a / then everything is the path.
    if ('/' == *i_FileName) {
        return SetPath(i_FileName);
    }

    CRTFREEIF(mFileName);

    // Concatenate Directory and Filename
    nsAutoString dir;
    dir += mDirectory;
    dir += i_FileName;
    char *eNewPath = dir.ToNewCString();
    if (!eNewPath)
        return NS_ERROR_OUT_OF_MEMORY;
    // Parse the new path
    nsresult status = ParsePath(eNewPath, &mDirectory, &mFileName, &mParam,
                                &mQuery, &mRef);
    CRTFREEIF(eNewPath);
    if (NS_FAILED(status))
        return status;
    status = ReconstructPath();
    return status;
}

NS_IMETHODIMP
nsStdURL::SetRef(const char* i_Ref)
{
    /*
        no check for i_Ref = nsnull becuz you can remove # by using it that way
        So SetRef(nsnull) removed any existing ref values whereas
        SetRef("") will ensure that there is a # at the end.  These apply to
        ? and ; as well.
    */
    nsresult status = DupString(&mRef,
           (i_Ref && (*i_Ref == '#')) ?  (i_Ref+1) : i_Ref);
    return (NS_FAILED(status) ? status : ReconstructPath());
}

NS_IMETHODIMP
nsStdURL::SetParam(const char* i_Param)
{
    nsresult status = DupString(&mParam,
            (i_Param && (*i_Param == ';')) ? (i_Param+1) : i_Param);
    return (NS_FAILED(status) ? status : ReconstructPath());
}

NS_IMETHODIMP
nsStdURL::SetQuery(const char* i_Query)
{
    nsresult status = DupString(&mQuery,
            (i_Query && (*i_Query == '?')) ? (i_Query+1) : i_Query);
    return (NS_FAILED(status) ? status : ReconstructPath());
}

NS_IMETHODIMP
nsStdURL::SetRelativePath(const char* i_Relative)
{
    nsresult rv = NS_OK;
    nsCAutoString options;
    char* ref;
    char* query;
    char* file;
    char* i_Path;
    char* ePath = nsnull;

    if (!i_Relative)
        return NS_ERROR_NULL_POINTER;

    // Make sure that if there is a : its before other delimiters
    // If not then its an absolute case
    static const char delimiters[] = "/;?#:";
    char* brk = PL_strpbrk(i_Relative, delimiters);
    if (brk && (*brk == ':')) // This is an absolute case
    {
        rv = SetSpec((char*) i_Relative);
        return rv;
    }

    if (*i_Relative == '/' && *(i_Relative+1) != '\0' &&
        *(i_Relative+1) == '/') {
        CRTFREEIF(mPreHost);
        CRTFREEIF(mHost);
        mPort = -1;
        CRTFREEIF(mPath);
        rv = ParsePreHost((char*)i_Relative, &mPreHost, &mHost, &mPort,
                          &ePath);
        if (NS_FAILED(rv))
            return rv;
        i_Path = ePath;
    } else {
        i_Path = (char*)i_Relative;
    }

    switch (*i_Path)
    {
        case '/':
            rv = SetPath((char*) i_Path);
            CRTFREEIF(ePath);
            return rv;

        case ';':
            // Append to Filename add then call SetFilePath
            options = mFileName;
            options += (char*)i_Path;
            file = (char*)options.GetBuffer();
            rv = SetFileName(file);
            CRTFREEIF(ePath);
            return rv;

        case '?':
            // check for ref part
            ref = PL_strrchr(i_Path, '#');
            if (!ref) {
                CRTFREEIF(mRef);
                rv = SetQuery((char*)i_Path);
                CRTFREEIF(ePath);
                return rv;
            } else {
                DupString(&query,nsnull);
                ExtractString((char*)i_Path, &query,
                    (PL_strlen(i_Path)-(ref-i_Path)));
                CRTFREEIF(ePath);
                rv = SetQuery(query);
                nsCRT::free(query);
                if (NS_FAILED(rv)) return rv;
                rv = SetRef(ref);
                return rv;
            }
            break;

        case '#':
            rv = SetRef((char*)i_Path);
            CRTFREEIF(ePath);
            return rv;

        default:
            rv = SetFileName((char*)i_Path);
            CRTFREEIF(ePath);
            return rv;
    }
}

NS_IMETHODIMP
nsStdURL::Resolve(const char *relativePath, char **result)
{
    nsresult rv;

    if (!relativePath) return NS_ERROR_NULL_POINTER;

    // Make sure that if there is a : its before other delimiters
    // If not then its an absolute case
    static const char delimiters[] = "/;?#:";
    char* brk = PL_strpbrk(relativePath, delimiters);
    if (brk && (*brk == ':')) // This is an absolute case
    {
        rv = DupString(result, relativePath);
        char* path = PL_strstr(*result,"://");
        if (path) {
            path = PL_strstr((char*)(path+3),"/");
            if (path)
                ReplaceMess(path);
        }
        return rv;
    }

    char* ePreHost = nsnull;
    char* eHost = nsnull;
    PRInt32 ePort = -1;
    char* ePath = nsnull;
    char* i_Path = nsnull;

    if (*relativePath == '/' && *(relativePath+1) != '\0' &&
        *(relativePath+1) == '/') {
        rv = ParsePreHost((char*)relativePath, &ePreHost, &eHost, &ePort,
                          &ePath);
        if (NS_FAILED(rv))
            return rv;
        i_Path = ePath;
    } else {
        i_Path = (char*)relativePath;
    }

    nsCAutoString finalSpec; // guaranteed to be singlebyte.

    if (mScheme)
    {
        finalSpec = mScheme;
        finalSpec += "://";
    }
    if (ePreHost)
    {
        finalSpec += ePreHost;
        finalSpec += '@';
        CRTFREEIF(ePreHost);
    } else {
        if (mPreHost)
        {
            finalSpec += mPreHost;
            finalSpec += '@';
        }
    }
    if (eHost)
    {
        finalSpec += eHost;
        CRTFREEIF(eHost);
        if (-1 != ePort)
        {
            char* portBuffer = PR_smprintf(":%d", ePort);
            if (!portBuffer)
                return NS_ERROR_OUT_OF_MEMORY;
            finalSpec += portBuffer;
            PR_smprintf_free(portBuffer);
            portBuffer = 0;
        }
    } else {
        if (mHost)
        {
            finalSpec += mHost;
            if (-1 != mPort)
            {
                char* portBuffer = PR_smprintf(":%d", mPort);
                if (!portBuffer)
                    return NS_ERROR_OUT_OF_MEMORY;
                finalSpec += portBuffer;
                PR_smprintf_free(portBuffer);
                portBuffer = 0;
            }
        }
    }

    if (i_Path) {
      switch (*i_Path)
      {
        case '/':
            finalSpec += (char*)i_Path;
            break;
        case ';':
            finalSpec += mDirectory;
            finalSpec += mFileName;
            finalSpec += (char*)i_Path;
            break;
        case '?':
            finalSpec += mDirectory;
            finalSpec += mFileName;
            if (mParam)
            {
                finalSpec += ';';
                finalSpec += mParam;
            }
            finalSpec += (char*)i_Path;
            break;
        case '#':
            finalSpec += mDirectory;
            finalSpec += mFileName;
            if (mParam)
            {
                finalSpec += ';';
                finalSpec += mParam;
            }
            if (mQuery)
            {
                finalSpec += '?';
                finalSpec += mQuery;
            }
            finalSpec += (char*)i_Path;
            break;
        default:
            finalSpec += mDirectory;
            finalSpec += (char*)i_Path;
      }
    }
    CRTFREEIF(ePath);
    *result = finalSpec.ToNewCString();
    if (*result) {
        char* path = PL_strstr(*result,"://");
        if (path) {
            path = PL_strstr((char*)(path+3),"/");
            if (path)
                ReplaceMess(path);
        }
        return NS_OK;
    } else
        return NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsStdURL::ReconstructPath(void)
{
    if (mPath) nsCRT::free(mPath);

    //Take all the elements of the path and construct it
    nsCAutoString path;
    path.SetCapacity(64);
    if (mDirectory)
    {
        path = mDirectory;
    }
    if (mFileName)
    {
        path += mFileName;
    }
    if (mParam)
    {
        path += ';';
        path += mParam;
    }
    if (mQuery)
    {
        path += '?';
        path += mQuery;
    }
    if (mRef)
    {
        path += '#';
        path += mRef;
    }
    mPath = path.ToNewCString();

    return (mPath ? ReconstructSpec() : NS_ERROR_OUT_OF_MEMORY);
}

NS_METHOD
nsStdURL::SetSpec(const char* i_Spec)
{
    // Skip leading spaces
    char* fwdPtr= (char*) i_Spec;
    while (fwdPtr && (*fwdPtr != '\0') && (*fwdPtr == ' '))
        fwdPtr++;
    // Remove trailing spaces
    if (fwdPtr) {
        char* bckPtr= (char*)fwdPtr + PL_strlen(fwdPtr) -1;
        if (*bckPtr == ' ') {
            while ((bckPtr-fwdPtr) >= 0 && (*bckPtr == ' ')) {
                bckPtr--;
            }
            *(bckPtr+1) = '\0';
        }
    }

    CRTFREEIF(mSpec);
    nsresult status = DupString(&mSpec, fwdPtr);
    // If spec is being rewritten clean up everything-
    CRTFREEIF(mScheme);
    CRTFREEIF(mPreHost);
    CRTFREEIF(mHost);
    mPort = -1;
    CRTFREEIF(mPath);
    CRTFREEIF(mDirectory);
    CRTFREEIF(mFileName);
    CRTFREEIF(mParam);
    CRTFREEIF(mQuery);
    CRTFREEIF(mRef);
    return (NS_FAILED(status) ? status : Parse());
}

NS_METHOD
nsStdURL::SetPath(const char* i_Path)
{
    if (mPath) nsCRT::free(mPath);
    nsresult status = DupString(&mPath, i_Path);
    if (NS_FAILED(status))
        return status;
    status = ParsePath(mPath, &mDirectory, &mFileName, &mParam,
                       &mQuery, &mRef);
    if (NS_FAILED(status))
        return status;
    status = ReconstructPath();
    return status;
}

NS_METHOD
nsStdURL::GetFilePath(char **o_DirFile)
{
    if (!o_DirFile)
        return NS_ERROR_NULL_POINTER;

    nsAutoString temp;
    if (mDirectory)
    {
        temp = mDirectory;
    }
    if (mFileName)
    {
        temp += mFileName;
    }
    *o_DirFile = temp.ToNewCString();
    if (!*o_DirFile)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_METHOD
nsStdURL::SetFilePath(const char *filePath)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsStdURL::GetFileBaseName(char **o_name)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsStdURL::SetFileBaseName(const char *name)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
