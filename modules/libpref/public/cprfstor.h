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
 
#ifndef _CPRFSTOR_PUBLIC_H_
#define _CPRFSTOR_PUBLIC_H_

#include "prtypes.h"
#include "prefapi.h"

#define PREF_READ_ONLY              1
#define PREF_REQUIRED_STORE         2
#define PREF_CREATE_IF_NOT_EXIST    4


/* Class Definition */

class PR_PUBLIC_API(CPrefStore) {
protected:
    uint16                  m_Flags;
    char                    *m_name;

/* constructor/destructor */    
public:
                            CPrefStore(const char *name, uint16 flags=0);
    virtual                 ~CPrefStore()       {;}

/* authentication */

/*	virtual PROFILE_ERROR	Authenticate(char *method, void *key);
	virtual PROFILE_ERROR	ChangeKey(char *oldMethod, void *oldKey, char *newMethod, void *newKey);*/

/* core file access functionality */
public:
    virtual PROFILE_ERROR   LoadPrefs(PRHashTable* prefsHash) = 0;
    virtual PROFILE_ERROR   SavePrefs(PRHashTable* prefsHash) = 0;

/* accessors */
    virtual char *          GetName()   {return m_name;}

/* error handling  */
    virtual PROFILE_ERROR   HandleError(PROFILE_ERROR err) = 0;

};

#endif
