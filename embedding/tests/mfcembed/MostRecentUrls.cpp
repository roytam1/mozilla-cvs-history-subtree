/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: Mozilla-sample-code 1.0
 *
 * Copyright (c) 2002 Netscape Communications Corporation and
 * other contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this Mozilla sample software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Contributor(s):
 *   Rod Spears <rods@netscape.com>   
 *
 * ***** END LICENSE BLOCK ***** */

//
// CMostRecentUrls object is responsible for keeping track of the
// 16 most recently used URLs. It stores this list in a file named
// "urls.txt" on a per profile basis in the user's profile directory
//
// The constructor loads the URL list
// The destructor saves the URL list
//

#include "StdAfx.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "MostRecentUrls.h"

//--------------------------------------------------------
//-- CMostRecentUrls
//--------------------------------------------------------
CMostRecentUrls::CMostRecentUrls() :
        mNumURLs(0)
{
	for (int i=0;i<MAX_URLS;i++) {
		mURLs[i] = NULL;
	}

	FILE * fd = GetFD(_T("r"));
	if (fd) {
		TCHAR line[512];
		while (_fgetts(line, 512, fd)) {
			if (_tcslen(line) > 1) {
				line[_tcslen(line)-1] = 0;
				mURLs[mNumURLs++] = _tcsdup(line);
			}
		}
		fclose(fd);
	}

}

FILE * CMostRecentUrls::GetFD(LPCTSTR aMode) 
{
#if defined(UNICODE)
    USES_CONVERSION;
#endif /* UNICODE */

    FILE * fd = nsnull;
    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocalFile> local_file(do_QueryInterface(file));
        local_file->AppendNative(NS_LITERAL_CSTRING("urls.txt"));
        local_file->OpenANSIFileDesc(
#if !defined(UNICODE)
            aMode,
#else /* UNICODE */
            W2A(aMode),
#endif /* UNICODE */
            &fd);
    }

    return fd;
}

CMostRecentUrls::~CMostRecentUrls() 
{
    FILE * fd = GetFD(_T("w"));
    if (fd) {
        for (int i=0;i<MAX_URLS;i++) {
            if(mURLs[i])
                fprintf(fd, "%s\n", mURLs[i]);
        }
    fclose(fd);
    }

    for (int i=0;i<MAX_URLS;i++) {
        if(mURLs[i])
          free(mURLs[i]);
    }
}

LPTSTR CMostRecentUrls::GetURL(int aInx)
{
    if (aInx < mNumURLs) {
        return mURLs[aInx];
    }

    return NULL;
}

void CMostRecentUrls::AddURL(LPCTSTR aURL)
{
	TCHAR szTemp[512];
	_tcscpy(szTemp, aURL);

	// check to see if an existing url matches the one passed in
	for (int i=0; i<MAX_URLS-1; i++)
	{
        if(mURLs[i])
        {
            if(_tcsicmp(mURLs[i], szTemp) == 0)
                break; 
        }
	}

    // if there was a match "i" will point to matching url entry
    // if not i will be MAX_URLS-1

    // move all url entries before this one down
	for (; i>0; i--)
	{
        if(mURLs[i])
          free(mURLs[i]);

        if(mURLs[i-1])  
            mURLs[i] = _tcsdup(mURLs[i-1]);
	}

	// place this url at the top
    if(mURLs[0])
        free(mURLs[0]);
    mURLs[0] = _tcsdup(szTemp);
}
