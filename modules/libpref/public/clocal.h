/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/********************************************************************/
 

#ifndef _CLOCAL_PUBLIC_H_
#define _CLOCAL_PUBLIC_H_

#include "prtypes.h"
#ifndef NSPR20
#include "prfile.h"
#else
#include "prio.h"
#endif

#include "cprfstor.h"

#if defined(_WIN32)
#define PREF_HASH_CALLBACK  _cdecl
#else
#define PREF_HASH_CALLBACK  PR_CALLBACK_DECL
#endif

/* Bitfields for flags - subclasses start at the 8th bit (256) */

#define PREF_GLOBAL_CONTEXT     256
#define PREF_EVAL_CALLBACKS     512
#define PREF_SKIP_FIRST         1024

/* Class Definition */

class PR_PUBLIC_API(CPrefStoreLocal : public CPrefStore) {

protected:
	char	                *m_filename;
	unsigned long           m_fileLength;
#ifndef NSPR20
    PRFileHandle            m_fp;
#else
    PRFileDesc*             m_fp;
#endif

    char                    *m_fileBuf;

/* constructor/destructor */    
public:
                            CPrefStoreLocal(const char *name, const char *filename, uint16 flags=0);
    virtual                 ~CPrefStoreLocal();

/* core file access functionality */
public:

    virtual PROFILE_ERROR   OpenFile(const PRIntn permissions);
    virtual PROFILE_ERROR   CloseFile(void);
    virtual PROFILE_ERROR   LoadPrefs(PRHashTable* prefsHash);
    virtual PROFILE_ERROR   SavePrefs(PRHashTable* prefsHash);

/* error handling */
public:
    virtual PROFILE_ERROR   HandleError(PROFILE_ERROR err);

/* accessors */

    virtual char *          GetFilename(char *filename, unsigned int *len);

/* preference read/parse */
protected:
    virtual PROFILE_ERROR   ReadPrefs(PRHashTable* prefsHash);
    virtual PROFILE_ERROR   WritePrefs(PRHashTable* prefsHash);

/* hooks */

protected:
    virtual PROFILE_ERROR   PreOpen(const PRIntn permissions);
    virtual PROFILE_ERROR   PostOpen(const PRIntn permissions, PROFILE_ERROR err);
    virtual PROFILE_ERROR   PreRead(void);
    virtual PROFILE_ERROR   PreEvaluate(void);
    virtual PROFILE_ERROR   PostRead(PROFILE_ERROR err);
    virtual PROFILE_ERROR   PreClose(void);
    virtual PROFILE_ERROR   PostClose(PROFILE_ERROR err);
    virtual PROFILE_ERROR   PreWrite(void);
    virtual PROFILE_ERROR   PostWrite(PROFILE_ERROR err);

/* utility functions */

protected:
    static PROFILE_ERROR PREF_HASH_CALLBACK CreatePrefFileEntry(PRHashEntry *he, int i, void *arg);
};


/**********************************************************************/


class PR_PUBLIC_API(CPrefStoreLocalLI : public CPrefStoreLocal) {
    /* constructor/destructor */    
public:
                            CPrefStoreLocalLI(const char *name, const char *filename, uint16 flags=0);

/* preference read/parse */
public:

    virtual PROFILE_ERROR   WritePrefs(PRHashTable* prefsHash);

/* utility functions */

protected:
    static PROFILE_ERROR PREF_HASH_CALLBACK CreatePrefFileEntry(PRHashEntry *he, int i, void *arg);
};

/**********************************************************************/

class PR_PUBLIC_API(CPrefStoreLockFile : public CPrefStoreLocal) {

/* constructor/destructor */    
public:
                            CPrefStoreLockFile(const char *name, const char *filename, uint16 flags=0);

/* core file access functionality */
public:

    virtual PROFILE_ERROR   SavePrefs(PRHashTable* prefsHash);

/* overridden hooks */
protected:

    virtual PROFILE_ERROR   PreEvaluate(void);

/* utility functions */

private:
            XP_Bool         VerifyLockFile(char* buf, unsigned long buflen);
};


#endif
