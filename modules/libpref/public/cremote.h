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
 

#ifndef _CREMOTE_PUBLIC_H_
#define _CREMOTE_PUBLIC_H_

#include "prtypes.h"
#ifndef NSPR20
#include "prfile.h"
#else
#include "prio.h"
#endif

#include "cprfstor.h"

/* Bitfields for flags - subclasses start at the 8th bit (256) */

#define PREF_GLOBAL_CONTEXT     256
#define PREF_EVAL_CALLBACKS     512
#define PREF_SKIP_FIRST         1024
#define PREF_USE_SSL            2048
#define PREF_APPEND_EMAIL       4096


/* Remote LI Transfer Flags */

#define PROF_REMOTE_LI_USEHTTP          1
#define PROF_REMOTE_LI_USELDAP          2

#define PROF_REMOTE_LI_ADDRESSBOOK      1
#define PROF_REMOTE_LI_BOOKMARKS        2
#define PROF_REMOTE_LI_COOKIES          4
#define PROF_REMOTE_LI_FILTERS          8
#define PROF_REMOTE_LI_JAVA             16
#define PROF_REMOTE_LI_SECURITY         32
#define PROF_REMOTE_LI_NAVCENTER        64  // REMIND this should not be used!
#define PROF_REMOTE_LI_HISTORY			64
#define PROF_REMOTE_LI_PREFS            128


/* Class Definition */

class PR_PUBLIC_API(CPrefStoreRemote : public CPrefStore) {

protected:
	char	                *m_URL;
    uint                    m_LIProtocol;
    uint16                  m_LIParams;
    char                    *m_LI_LDAPSearchBase;
    int                     refreshInterval;

/* constructor/destructor */    
public:
                            CPrefStoreRemote(const char *name, const char *url, uint16 flags=0);
    virtual                 ~CPrefStoreRemote();

/* core file access functionality */
public:

    virtual PROFILE_ERROR   LoadPrefs(PRHashTable* prefsHash);
    virtual PROFILE_ERROR   SavePrefs(PRHashTable* prefsHash);

/* error handling */
public:
    virtual PROFILE_ERROR   HandleError(PROFILE_ERROR err);

/* accessors */
public:
    virtual char *          GetURL(char *url, unsigned int *len);
    virtual void            SetURL(const char *url);

/* LI */

    virtual void            SetLISearchBase(const char *searchBase);
    virtual PROFILE_ERROR   SetLIParams(const uint16 liFlags);
    virtual PROFILE_ERROR   SetLIProtocol(int protocol);

/* preference read/parse */
protected:

/* hooks */

protected:

/* utility functions */

};

class PR_PUBLIC_API(CPrefStoreRemoteLDAP : public CPrefStoreRemote) {

protected:
	char	                *m_Username;
    char                    *m_PrefsLDAPSearchBase;
/* void                     m_LDAPHandle */


/* constructor/destructor */    
public:
                            CPrefStoreRemoteLDAP(const char *name, const char *url=NULL, uint16 flags=0);
    virtual                 ~CPrefStoreRemoteLDAP();

/* accessors */
public:
    virtual char *          GetUsername(char *username, unsigned int *len);
    virtual void            SetUsername(const char *username);

    virtual void            SetPrefsSearchBase(const char *searchpath);
};

#endif
