/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *   Gagan Saksena <gagan@netscape.com> (original author)
 *   Darin Fisher <darin@netscape.com>
 */

#ifndef nsHttpAuthCache_h__
#define nsHttpAuthCache_h__

#include "nsError.h"
#include "nsVoidArray.h"
#include "nsAString.h"
#include "plhash.h"

//-----------------------------------------------------------------------------
// nsHttpAuthCache
//-----------------------------------------------------------------------------

class nsHttpAuthCache
{
public:
    nsHttpAuthCache();
   ~nsHttpAuthCache();

    nsresult Init();

    // host and port are required
    // directory and realm can be null
    // realm if present takes precidence over directory
    nsresult GetCredentials(const char *host,
                            PRInt32     port,
                            const char *directory,
                            const char *realm,
                            nsACString &credentials);

    // host and port are required
    // directory and realm can be null
    // credentials, if null, clears the entry
    nsresult SetCredentials(const char *host,
                            PRInt32     port,
                            const char *directory,
                            const char *realm,
                            const char *credentials);

    // Expire all existing auth list entries including proxy auths. 
    nsresult ClearAll();

public: /* internal */

    class nsEntry
    {
    public:
        nsEntry(const char *directory,
                const char *realm,
                const char *credentials);
       ~nsEntry();

        const char *Directory()   { return mDirectory; }
        const char *Realm()       { return mRealm; }
        const char *Credentials() { return mCredentials; }

        void SetDirectory(const char *);
        void SetCredentials(const char *);
                
    private:
        char *mDirectory;
        char *mRealm;
        char *mCredentials;
    };

    class nsEntryList
    {
    public:
        nsEntryList();
       ~nsEntryList();

        // both directory and realm can be null, in which case the first
        // element will be returned.
        nsresult GetEntry(const char *directory,
                          const char *realm,
                          nsACString &credentials);

        // if a matching entry is found, then credentials will be changed.
        nsresult SetEntry(const char *directory,
                          const char *realm,
                          const char *credentials);

        PRUint32 Count() { return (PRUint32) mList.Count(); }

    private:
        nsVoidArray mList;
    };

private:
    void GenerateHashKey(const char *host, PRInt32 port, nsACString &);
    
private:
    PLHashTable *mDB; // "host:port" --> nsEntryList
};

#endif // nsHttpAuthCache_h__
