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
 

#ifndef _PROFILE_PUBLIC_H_
#define _PROFILE_PUBLIC_H_

#include "prefapi.h"
#include "prtypes.h"
#ifndef NSPR20
#include "prhash.h"
#else
#include "plhash.h"
#endif
#include "cprfstor.h"

typedef struct _storeList {
  CPrefStore          *store;
  struct _storeList   *next;
} PrefStorage;


/* Bitfields for flags */

#define PROFILE_OPENED_WITH_ERROR		1
#define PROFILE_CALLBACKS_ENABLED   	2
#define PROFILE_SOME_PREFS_LOCKED       4
#define PROFILE_NO_LOCAL_STORE          8
#define PROFILE_COPY_UPGRADED_FILES     16

class PR_PUBLIC_API(CProfile) {	

protected:
	char	        m_ProfileName[80];
    char            m_Password[40];

    uint32	        m_ProfileFlags;

    CPrefStore *    m_CurrentStore;

    PrefStorage *   m_PrefsStoreList;
    PRHashTable *   m_PrefsHash;
    
protected:
/* class specific types*/

    typedef union
    {
	    char*		            stringVal;
	    int32		            intVal;
	    PRBool		            boolVal;
    } PrefValue;

    typedef struct 
    {
        PrefValue	defaultPref;
	    PrefValue 	userPref;
	    uint8		flags;
        CPrefStore  *pSource;
    } PrefNode;

/* constructor/destructor */    
public:
    CProfile();
    CProfile(const char *profileName);

/* profile authentication */
public:
/*	virtual PROFILE_ERROR	Authenticate(char *method, void *key);
	virtual PROFILE_ERROR	ChangeKey(char *oldMethod, void *oldKey, char *newMethod, void *newKey);*/

/* load up the prefs hash table from all of the referenced stores */
public:
    virtual PROFILE_ERROR	LoadPrefs(const char *pStoreName = NULL, PRBool readAfter = PR_FALSE);
	virtual PROFILE_ERROR	SavePrefs(void);
	virtual PROFILE_ERROR	SavePrefs(const char *pStoreName);

/* accessors */
/*	virtual PROFILE_ERROR	GetProfileLocation(char *loc, int len);*/
	virtual PROFILE_ERROR	GetProfileName(char *name, int len);
	virtual PROFILE_ERROR	GetProfilePassword(char *password, int len);
    virtual uint32          GetFlags(void);

    virtual uint32          AddFlag(uint32 flag);
/*	virtual PROFILE_ERROR	SetProfileLocation(char *newLocation);*/
	virtual PROFILE_ERROR	SetProfileName(const char *newName);
	virtual PROFILE_ERROR	SetProfilePassword(const char *password);

/* store management */
    virtual PROFILE_ERROR   AddPrefsStoreLast(CPrefStore *newStore);
    virtual PROFILE_ERROR   AddPrefsStoreBefore(CPrefStore *newStore, const char *beforeThis);
    virtual PROFILE_ERROR   RemovePrefsStore(const char *name, PRBool removePrefs = PR_FALSE);
    virtual CPrefStore *    GetPrefsStore(const char *name);

/* convenience functions for store management */
    virtual PROFILE_ERROR   CreateAndAddLocalStores(const char *userDirectory);
    virtual PROFILE_ERROR   CreateAndAddRemoteStores();

/* get/set individual preferences */
    virtual PROFILE_ERROR   GetCharPref(const char *pref_name, char * return_buffer,
                                int * length, PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   CopyCharPref(const char *pref_name, char ** return_buffer,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetIntPref(const char *pref_name,int32 * return_int,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetBoolPref(const char *pref_name, PRBool * return_value,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetBinaryPref(const char *pref_name, void * return_value, int *size,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   CopyBinaryPref(const char *pref_name, void ** return_value, int *size,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetColorPref(const char *pref_name, uint8 *red, uint8 *green, uint8 *blue,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetColorPref(const char *pref_name, uint32 *colorref,
                                PRBool get_default = PR_FALSE);
    virtual PROFILE_ERROR   GetRectPref(const char *pref_name, int16 *left, int16 *top,
                                int16 *right, int16 *bottom, PRBool get_default = PR_FALSE);

    virtual PROFILE_ERROR   SetPref(const char *pref_name, const char *value, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetPref(const char *pref_name, int32 value, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetPref(const char *pref_name, PRBool value, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetPref(const char *pref_name, void * value, long size, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetColorPref(const char *pref_name, uint8 red, uint8 green, uint8 blue, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetColorPref(const char *pref_name, uint32 colorref, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetPathPref(const char *pref_name, const char *path, PrefAction action = PREF_SETUSER);
    virtual PROFILE_ERROR   SetRectPref(const char *pref_name, int16 left, int16 top, int16 right, int16 bottom, PrefAction action = PREF_SETUSER);

/* child list operations */
    virtual PROFILE_ERROR   CreateChildList(const char* parent_node, char **child_list);
    virtual char *          NextChild(char *child_list, int *index);

/* preference operations */
    virtual PROFILE_ERROR   GetPrefType(const char *pref_name);
    virtual PRBool         PrefLocked(const char *pref_name);
/*    virtual PROFILE_ERROR   LockPref(const char *pref_name); */
    virtual PROFILE_ERROR   UnlockPref(const char *pref_name);

#ifdef MOZ_LI
    virtual PROFILE_ERROR   SetLILocal(const char *pref_name);
#endif

    virtual PROFILE_ERROR   DeleteBranch(const char *branch_name);
    virtual PROFILE_ERROR   ClearUserPref(const char *pref_name);
#ifdef MOZ_LI 
    virtual PROFILE_ERROR   ClearLIPref(const char *pref_name);
#endif

    /* debugging/about:config support */
    virtual char *          DumpPrefs(void);

    /* note: these should be protected, but currently can't be because of the migration from the
       old way of doing things.  Don't rely on these functions staying public */

    virtual void            SetCallbacks(const PRBool status);

/* hash table manipulation functions */
public:
    /* these are public because they're referenced from within the NSPR Hash table implementation.
       they should never be called directly*/

    static void *           AllocPrefsHashTable(void *pool, size_t size);
    static void             FreePrefsHashTable(void *pool, void *item);
    static PRHashEntry *    AllocPrefsEntry(void *pool, const void *key);
    static void             FreePrefsEntry(void *pool, PRHashEntry *he, uint flag);

/* internal utility functions */
protected:
    virtual PRBool         CallbacksEnabled(void);
    static void             SetValue(CProfile::PrefValue* oldValue, CProfile::PrefValue newValue,
                                CProfile::PrefType type);
    static PRBool          ValueChanged(CProfile::PrefValue oldValue, CProfile::PrefValue newValue,
                                PrefType type);
           PROFILE_ERROR    HashPref(const char *key, CProfile::PrefValue value, PrefType type,
                                PrefAction action, CPrefStore *pSource = NULL);
    
    friend static int       pref_printDebugInfo(PRHashEntry *he, int i, void *arg);

};


#endif
