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
 
#include "prtypes.h"
#ifdef NSPR20
#include "prio.h"
#else
#include "prfile.h"
#endif


#include "xp_core.h"
#include "xp_mcom.h"
#include "xp_str.h"

#include "prefapi.h"
#include "prefpriv.h"
#include "cprofile.h"
#include "clocal.h"
#include "cremote.h"

#ifndef NSPR20
#include "prhash.h"
#else
#include "plhash.h"
#endif

#include "secnav.h"

#define PREF_SOURCE_UNCHANGED    ((CPrefStore *) -1)

/* types */

typedef int (*CharPrefReadFunc)(const char*, char**);


/* forward declarations */

int             pref_DeleteItem(PRHashEntry *he, int i, void *arg);
int             pref_AddChild(PRHashEntry *he, int i, void *arg);
PRBool         profile_FileExists(const char *nativePath);
char *          profile_AllocCombineStr(const char *str1, const char *str2);

static int      pref_printDebugInfo(PRHashEntry *he, int i, void *arg);



/* copied from xp_str.c */
/************************************************************************
 * These are "safe" versions of the runtime library routines. The RTL
 * versions do not null-terminate dest IFF strlen(src) >= destLength.
 * These versions always null-terminate, which is why they're safe.
 */

char *XP_STRNCPY_SAFE (char *dest, const char *src, size_t destLength)
{
	char *result = strncpy (dest, src, --destLength);
	dest[destLength] = '\0';
	return result;
}


/* Hash table functions */

void *
CProfile::AllocPrefsHashTable(void *pool, size_t size)
{
    return malloc(size);
}

void
CProfile::FreePrefsHashTable(void *pool, void *item)
{
    free(item);		/* free items? */
}

PRHashEntry *
CProfile::AllocPrefsEntry(void *pool, const void *key)
{
    return (PRHashEntry *) malloc(sizeof(PRHashEntry));
}

void
CProfile::FreePrefsEntry(void *pool, PRHashEntry *he, uint flag)
{
	CProfile::PrefNode *pref = (CProfile::PrefNode *) he->value;
	if (pref) {
		if (pref->flags & PREF_STRING) {
			XP_FREEIF(pref->defaultPref.stringVal);
			XP_FREEIF(pref->userPref.stringVal);
		}
		XP_FREE(he->value);
	}

    if (flag == HT_FREE_ENTRY) {
#ifdef OSF1
		XP_FREEIF((void *)he->key);
#else
		if (he->key) {
			XP_FREE((void *) he->key);
			he->key = 0;
		}
#endif
        XP_FREE(he);
	}
}

static PRHashAllocOps pref_HashAllocOps = {
	CProfile::AllocPrefsHashTable, CProfile::FreePrefsHashTable,
    CProfile::AllocPrefsEntry, CProfile::FreePrefsEntry
};

/* Utility functions */

/* Delete a branch. Used for deleting mime types */
int
pref_DeleteItem(PRHashEntry *he, int i, void *arg)
{
	const char *to_delete = (const char *) arg;
	unsigned int len = strlen(to_delete);
	
	/* note if we're deleting "ldap" then we want to delete "ldap.xxx"
		and "ldap" (if such a leaf node exists) but not "ldap_1.xxx" */
	if (to_delete && (XP_STRNCMP((char *) he->key, to_delete, len) == 0 ||
		(len-1 == strlen((char *) he->key) && XP_STRNCMP((char *) he->key, to_delete, len-1) == 0)))
		return HT_ENUMERATE_REMOVE;
	else
		return HT_ENUMERATE_NEXT;
}

/* if entry begins with the given string, i.e. if string is
  "a"
  and entry is
  "a.b.c" or "a.b"
  then add "a.b" to the list. */
int
pref_AddChild(PRHashEntry *he, int i, void *arg)
{
	PrefChildIter* pcs = (PrefChildIter*) arg;
	if ( XP_STRNCMP((char *) he->key, pcs->parent, strlen(pcs->parent)) == 0 ) {
		char buf[512];
		char* nextdelim;
		unsigned int parentlen = strlen(pcs->parent);
		char* substring;
		int buflen;
		XP_Bool substringBordersSeparator = FALSE;

		strncpy(buf, (char *) he->key, PR_MIN(512, strlen((char *) he->key) + 1));
		nextdelim = buf + parentlen;
		buflen = strlen(buf);
		if (parentlen < strlen(buf)) {
			/* Find the next delimiter if any and truncate the string there */
			nextdelim = strstr(nextdelim, ".");
			if (nextdelim) {
				*nextdelim = '\0';
			}
		}

		substring = strstr(pcs->childList, buf);
		if (substring)
		{
			if (substring[buflen] == '\0' || substring[buflen] == ';')
				substringBordersSeparator = TRUE;
		}

		if (!substring || !substringBordersSeparator) {
			unsigned int newsize = strlen(pcs->childList) + strlen(buf) + 2;
			if (newsize > pcs->bufsize) {
#ifdef XP_WIN16
			return HT_ENUMERATE_STOP;
#else
				pcs->bufsize *= 3;
				pcs->childList = (char*) realloc(pcs->childList, sizeof(char) * pcs->bufsize);
				if (!pcs->childList)
					return HT_ENUMERATE_STOP;
#endif
			}
			XP_STRCAT(pcs->childList, buf);
			XP_STRCAT(pcs->childList, ";");
		}
	}
	return 0;
}


/* Encoding/Decoding functions */

#ifndef XP_WIN
extern char *EncodeBase64Buffer(char *subject, long size);
extern char *DecodeBase64Buffer(char *subject);
#else
/* temporary to make windows into a DLL...add a assert if used */
char *EncodeBase64Buffer(char *subject, long size) 
{
	assert(0);
	return NULL;
}

char *DecodeBase64Buffer(char *subject)
{
	assert(0);
	return NULL;
}
#endif

/* Class implementation */

CProfile::CProfile()
{
    CPrefStore          *pNSCPCfg;

    m_ProfileFlags = PROFILE_CALLBACKS_ENABLED;

    m_CurrentStore = PREF_SOURCE_UNCHANGED;

    m_PrefsStoreList = NULL;
    m_PrefsHash = PR_NewHashTable(2048, PR_HashString, PR_CompareStrings,
		PR_CompareValues, &pref_HashAllocOps, NULL);

    /* Every profile must include netscape.cfg */

    /* readonly/required, compute path to netscape.cfg */

    pNSCPCfg = new CPrefStoreLockFile("netscape.cfg", "netscape.cfg", PREF_REQUIRED_STORE);
    AddPrefsStoreLast(pNSCPCfg);

}

CProfile::CProfile(const char *profileName)
{
    CProfile();
	SetProfileName(profileName);
}

CProfile::~CProfile()
{
	if (m_PrefsHash)
		PR_HashTableDestroy(m_PrefsHash);

    if (m_PrefsStoreList) {
        /* free all of the profile stores */
    }

    m_PrefsHash = NULL;
}


    uint32
CProfile::GetFlags()
{
	return m_ProfileFlags;
}


PROFILE_ERROR
CProfile::GetProfileName(char *name, int len)
{
	XP_STRNCPY_SAFE(name, m_ProfileName, len-1);
	return 0;
}

PROFILE_ERROR
CProfile::GetProfilePassword(char *name, int len)
{
	XP_STRNCPY_SAFE(name, m_Password, len-1);
	return 0;
}

    uint32
CProfile::AddFlag(uint32 flags)
{
    m_ProfileFlags |= flags;

	return m_ProfileFlags;
}

PROFILE_ERROR
CProfile::SetProfileName(const char *newName)
{
	XP_STRNCPY_SAFE(m_ProfileName, newName, 79);
    PREF_SetDefaultCharPref( "profile.name", newName );
	return 0;
}

PROFILE_ERROR
CProfile::SetProfilePassword(const char *password)
{
	char * pstr = SECNAV_MungeString(password);

	XP_STRNCPY_SAFE(m_Password, pstr, 39);

    XP_FREEIF(pstr);
	return 0;
}

PRBool
CProfile::CallbacksEnabled(void)
{
    return (PRBool) (m_ProfileFlags & PROFILE_CALLBACKS_ENABLED);
}

PROFILE_ERROR
CProfile::LoadPrefs(const char *storeName, PRBool readAfter)
{
    PROFILE_ERROR   err = PREF_OK;
    CPrefStore      *pStartStore = NULL;
    PrefStorage     *currentStore = m_PrefsStoreList;
    XP_Bool         bStartReading = FALSE;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

    if (storeName) {
        pStartStore =  GetPrefsStore(storeName);
    } else {
        bStartReading = TRUE;
    }

    if (pStartStore && !readAfter) {
        m_CurrentStore = pStartStore;
        err = m_CurrentStore->LoadPrefs(m_PrefsHash);

        if (err < PREF_OK) {
            /* the store will display an error message (or not) and will return
               PREF_OK if it's ok to move on, or an error to abort the load */

            err = m_CurrentStore->HandleError(err);
        }
    } else {

	    /* Walk through each of the stores, loading from each in turn */

        while ((currentStore != NULL) && (err == PREF_OK)) {
            XP_ASSERT(currentStore->store);
        
            if ((pStartStore) && (!bStartReading)) {
                if (pStartStore == currentStore->store) {
                    bStartReading = TRUE;
                }
            }

            if (bStartReading) {
                m_CurrentStore = currentStore->store;

                if ((!storeName) || (XP_STRCMP(storeName, m_CurrentStore->GetName()) == 0)) {
                    err = m_CurrentStore->LoadPrefs(m_PrefsHash);

                    if (err < PREF_OK) {
                        /* the store will display an error message (or not) and will return
                           PREF_OK if it's ok to move on, or an error to abort the load */

                        err = m_CurrentStore->HandleError(err);
                    }
                }
            }

            currentStore = currentStore->next;
        }
    }

    m_CurrentStore = PREF_SOURCE_UNCHANGED;
    return err;
}
    
PROFILE_ERROR
CProfile::SavePrefs()
{
    PROFILE_ERROR   err = PREF_OK;
    PrefStorage     *currentStore = m_PrefsStoreList;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	/* ?! Don't save (blank) user prefs if there was an error reading them */
	if (m_ProfileFlags & PROFILE_OPENED_WITH_ERROR)
		return PREF_OK;

	/* Walk through each of the stores, instructing each to save */

    while ((currentStore != NULL) && (err == PREF_OK)) {
        XP_ASSERT(currentStore->store);
        
        err = currentStore->store->SavePrefs(m_PrefsHash);
        currentStore = currentStore->next;
    }

    return err;
}


PROFILE_ERROR
CProfile::SavePrefs(const char *pStoreName)
{
    PROFILE_ERROR   err = PREF_BAD_PARAMETER;
    CPrefStore      *pStore;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

    pStore = GetPrefsStore(pStoreName);

    if (pStore) {
        err = pStore->SavePrefs(m_PrefsHash);
    }

    return err;
}


PROFILE_ERROR
CProfile::AddPrefsStoreLast(CPrefStore *newStore)
{
    PrefStorage     *pNewItem, *pCurrentItem, *pLastItem = NULL;

    pNewItem = (PrefStorage *) XP_ALLOC(sizeof(PrefStorage));

    if (!pNewItem) {
        return PREF_OUT_OF_MEMORY;
    }

    pNewItem->store = newStore;
    pNewItem->next = NULL;

    pCurrentItem = m_PrefsStoreList;

    while (pCurrentItem) {
        pLastItem = pCurrentItem;
        pCurrentItem = pCurrentItem->next;
    }

    if (pLastItem == NULL) {
        m_PrefsStoreList = pNewItem;
    } else {
        pLastItem->next = pNewItem;
    }

    return PREF_OK;
}

PROFILE_ERROR
CProfile::AddPrefsStoreBefore(CPrefStore *newStore, const char *beforeThis)
{
    PrefStorage     *pNewItem, *pCurrentItem, *pLastItem = NULL;
    char            *storeName;

    pNewItem = (PrefStorage *) XP_ALLOC(sizeof(PrefStorage));

    if (!pNewItem) {
        return PREF_OUT_OF_MEMORY;
    }

    pNewItem->store = newStore;
    pNewItem->next = NULL;

    pCurrentItem = m_PrefsStoreList;

    while (pCurrentItem) {
        storeName = pCurrentItem->store->GetName();

        if (!XP_STRCMP(storeName, beforeThis)) {
            break;
        }
            
        pLastItem = pCurrentItem;
        pCurrentItem = pCurrentItem->next;
    }

    if (pLastItem == NULL) {
        pNewItem->next = m_PrefsStoreList;
        m_PrefsStoreList = pNewItem;
    } else {
        pNewItem->next = pCurrentItem;
        pLastItem->next = pNewItem;
    }

    return PREF_OK;
}

PROFILE_ERROR
CProfile::RemovePrefsStore(const char *name, PRBool removePrefs)
{
    PrefStorage     *pCurrentItem, *pLastItem = NULL;
    char            *storeName;

    pCurrentItem = m_PrefsStoreList;

    while (pCurrentItem) {
        storeName = pCurrentItem->store->GetName();

        if (!XP_STRCMP(storeName, name)) {
            break;
        }
            
        pLastItem = pCurrentItem;
        pCurrentItem = pCurrentItem->next;
    }

    if (pLastItem == NULL) {
        m_PrefsStoreList = pCurrentItem->next;
    } else {
        pLastItem->next = pCurrentItem->next;
    }

    if (pCurrentItem) {
        if (removePrefs) {
            /* now go through and remove the preferences associated with this store */
        }

        XP_FREE(pCurrentItem);
        return PREF_OK;
    } else {
        return PREF_DOES_NOT_EXIST;
    }
}

CPrefStore *
CProfile::GetPrefsStore(const char *name)
{
    PrefStorage     *pCurrentItem;
    char            *storeName;

    pCurrentItem = m_PrefsStoreList;

    while (pCurrentItem) {
        storeName = pCurrentItem->store->GetName();

        if (!XP_STRCMP(storeName, name)) {
            return pCurrentItem->store;
        }
            
        pCurrentItem = pCurrentItem->next;
    }

    return NULL;
}


/* after adding local stores, the search path of stores for a local-only
   profile should look like:

    if (exists(prefs.js))
        pPrefsJS = CPrefStoreLocal(readwrite)

    if (exists(user.js))
        pUserJS = CPrefStoreLocal(readonly)

    if (exists(liprefs.js))
        pLIPrefs = CPrefStoreLocal(readwrite)

    if (exists(profile.cfg))
        pProfileCfg = CPrefStoreLockFile(readonly)

    <the Netscape.cfg file>

*/

    PROFILE_ERROR
CProfile::CreateAndAddLocalStores(const char *userDirectory)
{
    char            *filePath;
    char            filePart[15];
    CPrefStore      *pPrefsStore;

    strcpy(&(filePart[1]), "prefs.js");
    filePart[0] = PR_DIRECTORY_SEPARATOR;

    filePath = profile_AllocCombineStr(userDirectory, filePart);
    if (filePath) {
        pPrefsStore = new CPrefStoreLocal("prefs.js", filePath,
            (m_ProfileFlags & PROFILE_NO_LOCAL_STORE) ? 0 : PREF_CREATE_IF_NOT_EXIST);
        AddPrefsStoreBefore(pPrefsStore, "netscape.cfg");

        XP_FREE(filePath);
    }
     
    strcpy(&(filePart[1]), "user.js");
    filePart[0] = PR_DIRECTORY_SEPARATOR;

    filePath = profile_AllocCombineStr(userDirectory, filePart);
    if (filePath) {
        if (profile_FileExists(filePath)) {

            pPrefsStore = new CPrefStoreLocal("user.js", filePath, PREF_READ_ONLY);
            AddPrefsStoreBefore(pPrefsStore, "netscape.cfg");
        }

        XP_FREE(filePath);
    }
     
 /*   strcpy(&(filePart[1]), "liprefs.js");
    filePart[0] = PR_DIRECTORY_SEPARATOR;

    filePath = profile_AllocCombineStr(userDirectory, filePart);
    if (filePath) {
        if (profile_FileExists(filePath)) {

            pPrefsStore = new CPrefStoreLocalLI("liprefs.js", filePath);
            AddPrefsStoreBefore(pPrefsStore, "netscape.cfg");
        }

        XP_FREE(filePath);
    }
   */  
    strcpy(&(filePart[1]), "profile.cfg");
    filePart[0] = PR_DIRECTORY_SEPARATOR;

    filePath = profile_AllocCombineStr(userDirectory, filePart);
    if (filePath) {
        if (profile_FileExists(filePath)) {

            pPrefsStore = new CPrefStoreLockFile("profile.cfg", filePath);
            AddPrefsStoreBefore(pPrefsStore, "netscape.cfg");
        }

        XP_FREE(filePath);
    }
    
    return PREF_OK;
}

    PROFILE_ERROR
CProfile::CreateAndAddRemoteStores()
{
    char            *url;
    CPrefStore      *pPrefsStore;
    

    /* In order to determine where the .jsc file lives, we need to have
       done a read of the preferences at one point prior to this call.
       
       Then, for efficiency, after the store is created, call LoadPrefs
       on just it. */

    CopyCharPref("autoadmin.global_config_url", &url, TRUE);

    if (url && *url) {
        pPrefsStore = new CPrefStoreRemote("config.jsc", url);

        /* we should probably set the 'append email address flag here, but
           since it gets checked again in netlib, it doesn't really matter */

        AddPrefsStoreBefore(pPrefsStore, "netscape.cfg");

        XP_FREE(url);
    }
    
    return PREF_OK;
}

PROFILE_ERROR
CProfile::GetCharPref(const char *pref_name, char * return_buffer, int * length, PRBool get_default)
{
	int result = PREF_ERROR;
	char* stringVal;
	
	CProfile::PrefNode* pref;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);

	if (pref) {
		if (get_default || PREF_IS_LOCKED(pref) || !PREF_HAS_USER_VALUE(pref))
			stringVal = pref->defaultPref.stringVal;
		else
			stringVal = pref->userPref.stringVal;
		
		if (stringVal) {
			if (*length == 0) {
				*length = strlen(stringVal) + 1;
			}
			else {
				strncpy(return_buffer, stringVal, PR_MIN((unsigned int) (*length) - 1, strlen(stringVal) + 1));
				return_buffer[*length - 1] = '\0';
			}
			result = PREF_OK;
		}
	}
	return result;
}

PROFILE_ERROR
CProfile::CopyCharPref(const char *pref_name, char ** return_buffer, PRBool get_default)
{
	int result = PREF_ERROR;
	char* stringVal;	
	CProfile::PrefNode* pref;

    XP_ASSERT(return_buffer);

    *return_buffer = NULL;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);

	if (pref && pref->flags & PREF_STRING) {
		if (get_default || PREF_IS_LOCKED(pref) || !PREF_HAS_USER_VALUE(pref))
			stringVal = pref->defaultPref.stringVal;
		else
			stringVal = pref->userPref.stringVal;
		
		if (stringVal) {
			*return_buffer = XP_STRDUP(stringVal);
			result = PREF_OK;
		}
	}
	return result;
}

PROFILE_ERROR
CProfile::GetIntPref(const char *pref_name,int32 * return_int, PRBool get_default)
{
	int result = PREF_ERROR;	
	CProfile::PrefNode* pref;

    XP_ASSERT(return_int);

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
	if (pref && pref->flags & PREF_INT) {
		if (get_default || PREF_IS_LOCKED(pref) || !PREF_HAS_USER_VALUE(pref))
			*return_int = pref->defaultPref.intVal;
		else
			*return_int = pref->userPref.intVal;
		result = PREF_OK;
	}
	return result;
}

PROFILE_ERROR
CProfile::GetBoolPref(const char *pref_name, PRBool * return_value, PRBool get_default)
{
	int result = PREF_ERROR;
	CProfile::PrefNode* pref;

    XP_ASSERT(return_value);

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);	
	if (pref && pref->flags & PREF_BOOL) {
		if (get_default || PREF_IS_LOCKED(pref) || !PREF_HAS_USER_VALUE(pref))
			*return_value = pref->defaultPref.boolVal;
		else
			*return_value = pref->userPref.boolVal;
		result = PREF_OK;
	}
	return result;
}

PROFILE_ERROR
CProfile::GetBinaryPref(const char *pref_name, void * return_value, int *size, PRBool get_default)
{
	char* buf;
	int result;

    if (!return_value) {
        return PREF_BAD_PARAMETER;
    }

    result = CopyCharPref(pref_name, &buf, get_default);

	if (result == PREF_NOERROR) {
		char* debuf;
		if (strlen(buf) == 0) {		/* don't decode empty string ? */
			XP_FREE(buf);
			return -1;
		}
	
		debuf = DecodeBase64Buffer(buf);
		XP_MEMCPY(return_value, debuf, *size);
		
		XP_FREE(buf);
		XP_FREE(debuf);
	}

	return result;
}

PROFILE_ERROR
CProfile::CopyBinaryPref(const char *pref_name, void ** return_value, int *size, PRBool get_default)
{
	char* buf;
	int result;

    if (!return_value) {
        return PREF_BAD_PARAMETER;
    }

    result = CopyCharPref(pref_name, &buf, get_default);

	if (result == PREF_NOERROR) {
		if (strlen(buf) == 0) {		/* don't decode empty string ? */
			XP_FREE(buf);
			return -1;
		}
	
		*return_value = DecodeBase64Buffer(buf);
		*size = strlen(buf);
		
		XP_FREE(buf);
	}

	return result;
}

PROFILE_ERROR
CProfile::GetColorPref(const char *pref_name, uint8 *red, uint8 *green, uint8 *blue, PRBool get_default)
{
	char            colstr[8];
	int             iSize = 8;
    PROFILE_ERROR   result;

    result = GetCharPref(pref_name, colstr, &iSize, get_default);

	if (result == PREF_NOERROR) {
		int r, g, b;
		sscanf(colstr, "#%02X%02X%02X", &r, &g, &b);
		*red = r;
		*green = g;
		*blue = b;
	}
	
	return result;
}

#define MYRGB(r, g ,b)  ((uint32) (((uint8) (r) | ((uint16) (g) << 8)) | (((uint32) (uint8) (b)) << 16))) 


PROFILE_ERROR
CProfile::GetColorPref(const char *pref_name, uint32 *colorref, PRBool get_default)
{
    uint8 red, green, blue;
    PROFILE_ERROR   result;

    XP_ASSERT(colorref);

    result = GetColorPref(pref_name, &red, &green, &blue, get_default);

    if (result == PREF_NOERROR) {
    	*colorref = MYRGB(red,green,blue);
    }

    return result;
}

PROFILE_ERROR
CProfile::GetRectPref(const char *pref_name, int16 *left, int16 *top, int16 *right, int16 *bottom, PRBool get_default)
{
	char rectstr[64];
	int iSize=64;
	int result = GetCharPref(pref_name, rectstr, &iSize, get_default);
	
	if (result == PREF_NOERROR) {
		int l, t, r, b;
		sscanf(rectstr, "%i,%i,%i,%i", &l, &t, &r, &b);
		*left = l;	*top = t;
		*right = r;	*bottom = b;
	}

	return result;
}



PROFILE_ERROR
CProfile::CreateChildList(const char* parent_node, char **child_list)
{
	PrefChildIter pcs;

#ifdef XP_WIN16
	pcs.bufsize = 20480;
#else
	pcs.bufsize = 2048;
#endif
	pcs.childList = (char*) malloc(sizeof(char) * pcs.bufsize);
	if (*parent_node > 0)
		pcs.parent = PR_smprintf("%s.", parent_node);
	else
		pcs.parent = XP_STRDUP("");
	if (!pcs.parent || !pcs.childList)
		return PREF_OUT_OF_MEMORY;
	pcs.childList[0] = '\0';

	PR_HashTableEnumerateEntries(m_PrefsHash, pref_AddChild, &pcs);

	*child_list = pcs.childList;
	XP_FREE(pcs.parent);
	
	return (pcs.childList == NULL) ? PREF_OUT_OF_MEMORY : PREF_OK;
}

char *
CProfile::NextChild(char *child_list, int *index)
{
	char* child = strtok(&child_list[*index], ";");
	if (child)
		*index += strlen(child) + 1;
	return child;
}


PROFILE_ERROR
CProfile::GetPrefType(const char *pref_name)
{
	CProfile::PrefNode*   pref;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
    
    if (pref) {
		if (pref->flags & PREF_STRING)
			return PREF_STRING;
		else if (pref->flags & PREF_INT)
			return PREF_INT;
		else if (pref->flags & PREF_BOOL)
			return PREF_BOOL;
	} else {
        return PREF_DOES_NOT_EXIST;
    }

    return PREF_ERROR;
}

PRBool
CProfile::PrefLocked(const char *pref_name)
{
	PRBool result = PR_FALSE;

    if (m_ProfileFlags & PROFILE_SOME_PREFS_LOCKED) {
		CProfile::PrefNode* pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
		if (pref && PREF_IS_LOCKED(pref))
			result = PR_TRUE;
	}
	
	return result;
}

PROFILE_ERROR
CProfile::UnlockPref(const char *pref_name)
{
	CProfile::PrefNode*   pref;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
    
	if (!pref) {
        return PREF_DOES_NOT_EXIST;
    }

    if (PREF_IS_LOCKED(pref)) {
		pref->flags &= ~PREF_LOCKED;
		if (CallbacksEnabled()) {
			pref_DoCallback(pref_name);
		}
    }

    return PREF_OK;
}


#ifdef MOZ_LI
PROFILE_ERROR
CProfile::SetLILocal(const char *pref_name)
{
	CProfile::PrefNode*   pref;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);

	if (pref && !PREF_HAS_LI_VALUE(pref)) {
		pref->flags |= PREF_LILOCAL;
		if (CallbacksEnabled()) {
			pref_DoCallback(pref_name);
		}
	}

    return PREF_OK;
}
#endif

PROFILE_ERROR
CProfile::SetPref(const char *pref_name, const char *value, PrefAction action)
{
	CProfile::PrefValue pref;
	pref.stringVal = (char*) value;
	
    return HashPref(pref_name, pref, PREF_STRING, action, m_CurrentStore);
}

PROFILE_ERROR
CProfile::SetPref(const char *pref_name, int32 value, PrefAction action)
{
	CProfile::PrefValue pref;
	pref.intVal = value;
	
    return HashPref(pref_name, pref, PREF_INT, action, m_CurrentStore);
}


PROFILE_ERROR
CProfile::SetPref(const char *pref_name, PRBool value, PrefAction action)
{
	CProfile::PrefValue pref;
	pref.boolVal = value;
	
	return HashPref(pref_name, pref, PREF_BOOL, action, m_CurrentStore);
}

PROFILE_ERROR
CProfile::SetPref(const char *pref_name, void * value, long size, PrefAction action)
{
	char* buf = EncodeBase64Buffer((char *) value, size);

	if (buf) {
		CProfile::PrefValue pref;
		pref.stringVal = buf;

        return HashPref(pref_name, pref, PREF_STRING, action, m_CurrentStore);
    }
	else
		return PREF_ERROR;
}


PROFILE_ERROR
CProfile::SetColorPref(const char *pref_name, uint8 red, uint8 green, uint8 blue, PrefAction action)
{
	char colstr[63];
	CProfile::PrefValue pref;
	XP_SPRINTF( colstr, "#%02X%02X%02X", red, green, blue);

	pref.stringVal = colstr;
	return HashPref(pref_name, pref, PREF_STRING, action, m_CurrentStore);
}

#define MYGetboolVal(rgb)   ((uint8) ((rgb) >> 16))
#define MYGetGValue(rgb)   ((uint8) (((uint16) (rgb)) >> 8)) 
#define MYGetRValue(rgb)   ((uint8) (rgb)) 

PROFILE_ERROR
CProfile::SetColorPref(const char *pref_name, uint32 colorref, PrefAction action)
{
	int red,green,blue;

	red = MYGetRValue(colorref);
	green = MYGetGValue(colorref);
	blue = MYGetboolVal(colorref);

    return SetColorPref(pref_name, red, green, blue, action);
}

PROFILE_ERROR
CProfile::SetPathPref(const char *pref_name, const char *path, PrefAction action)
{
	CProfile::PrefValue pref;
	pref.stringVal = (char*) path;
	
	return HashPref(pref_name, pref, PREF_STRING, action, m_CurrentStore);
}

PROFILE_ERROR
CProfile::SetRectPref(const char *pref_name, int16 left, int16 top, int16 right, int16 bottom, PrefAction action)
{
	char rectstr[63];
	CProfile::PrefValue pref;
	XP_SPRINTF( rectstr, "%d,%d,%d,%d", left, top, right, bottom);

	pref.stringVal = rectstr;
	return HashPref(pref_name, pref, PREF_STRING, action, m_CurrentStore);
}


void
CProfile::SetValue(CProfile::PrefValue* oldValue, CProfile::PrefValue newValue, PrefType type)
{
	switch (type) {
		case PREF_STRING:
			XP_ASSERT(newValue.stringVal);
			XP_FREEIF(oldValue->stringVal);
			oldValue->stringVal = newValue.stringVal ? XP_STRDUP(newValue.stringVal) : NULL;
			break;
		
		default:
			*oldValue = newValue;
	}

    return;
}

PRBool
CProfile::ValueChanged(CProfile::PrefValue oldValue, CProfile::PrefValue newValue, PrefType type)
{
	XP_Bool changed = TRUE;
	switch (type) {
		case PREF_STRING:
			if (oldValue.stringVal && newValue.stringVal)
				changed = (strcmp(oldValue.stringVal, newValue.stringVal) != 0);
			break;
		
		case PREF_INT:
			changed = oldValue.intVal != newValue.intVal;
			break;
			
		case PREF_BOOL:
			changed = oldValue.boolVal != newValue.boolVal;
			break;
	}
	return (PRBool) changed;
}


PROFILE_ERROR
CProfile::HashPref(const char *key, CProfile::PrefValue value, PrefType type, PrefAction action, CPrefStore *source)
{
	CProfile::PrefNode* pref;
	int                 result = PREF_OK;

	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

	pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, key);
	if (!pref) {
    	pref = (CProfile::PrefNode*) calloc(sizeof(CProfile::PrefNode), 1);
    	if (!pref)	
    		return PREF_OUT_OF_MEMORY;
    	pref->flags = type;
		if (pref->flags & PREF_BOOL)
			pref->defaultPref.boolVal = (PRBool) -2;
		/* ugly ass hack -- define it to a default that no pref will ever default to
		   this should really get fixed right by some out of band data */
		if (pref->flags & PREF_INT)
			pref->defaultPref.intVal = (int32) -5632;
        pref->pSource = source;
    	PR_HashTableAdd(m_PrefsHash, XP_STRDUP(key), pref);
    }
    else if (!(pref->flags & type)) {
		XP_ASSERT(0);			/* this shouldn't happen */
		return PREF_TYPE_CHANGE_ERR;
    }

    if (source != PREF_SOURCE_UNCHANGED) {
        pref->pSource = source;
    }

    switch (action) {
    	case PREF_SETDEFAULT:
    	case PREF_SETCONFIG:
    		if (!PREF_IS_LOCKED(pref)) {		/* ?? change of semantics? */
				if (ValueChanged(pref->defaultPref, value, type)) {
					SetValue(&pref->defaultPref, value, type);
					if (!PREF_HAS_USER_VALUE(pref))
				    	result = PREF_VALUECHANGED;
			    }
			}
			if (action == PREF_SETCONFIG)
				pref->flags |= PREF_CONFIG;
			break;
			
		case PREF_SETUSER:
			/* If setting to the default value, then un-set the user value.
			   Otherwise, set the user value only if it has changed */
			if ( !ValueChanged(pref->defaultPref, value, type) ) {
				if (PREF_HAS_USER_VALUE(pref)) {
					pref->flags &= ~PREF_USERSET;
					if (!PREF_IS_LOCKED(pref))
						result = PREF_VALUECHANGED;
				}
			}
			else if ( !PREF_HAS_USER_VALUE(pref) ||
					   ValueChanged(pref->userPref, value, type) ) {    	
				SetValue(&pref->userPref, value, type);
				pref->flags |= PREF_USERSET;
				if (!PREF_IS_LOCKED(pref))
			    	result = PREF_VALUECHANGED;
		    }
			break;
			
		case PREF_LOCK:
			if (ValueChanged(pref->defaultPref, value, type)) {
				SetValue(&pref->defaultPref, value, type);
		    	result = PREF_VALUECHANGED;
		    }
		    else if (!PREF_IS_LOCKED(pref)) {
		    	result = PREF_VALUECHANGED;
		    }
		    pref->flags |= PREF_LOCKED;
		    m_ProfileFlags |= PROFILE_SOME_PREFS_LOCKED;
		    break;
	}

    if (result == PREF_VALUECHANGED && CallbacksEnabled()) {
    	int result2 = pref_DoCallback(key);
    	if (result2 < 0)
    		result = result2;
    }
    return result;
}


PROFILE_ERROR
CProfile::DeleteBranch(const char *branch_name)
{
    PROFILE_ERROR   err = PREF_OK;
	char*           branch_dot = PR_smprintf("%s.", branch_name);

	if (!branch_dot)
		return PREF_OUT_OF_MEMORY;		

	if (!m_PrefsHash) {
		err = PREF_NOT_INITIALIZED;
    } else {
	    PR_HashTableEnumerateEntries(m_PrefsHash, pref_DeleteItem, (void*) branch_dot);
    }
	
	XP_FREE(branch_dot);
	return err;
}

PROFILE_ERROR
CProfile::ClearUserPref(const char *pref_name)
{
	int         success = PREF_ERROR;
    CProfile::PrefNode*   pref;
    
	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

    pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
	if (pref && PREF_HAS_USER_VALUE(pref)) {
		pref->flags &= ~PREF_USERSET;
		if (CallbacksEnabled())
    		pref_DoCallback(pref_name);
		success = PREF_OK;
	}

    return success;
}

#ifdef MOZ_LI 
PROFILE_ERROR
CProfile::ClearLIPref(const char *pref_name)
{
	int         success = PREF_ERROR;
    CProfile::PrefNode*   pref;
    
	if (!m_PrefsHash)
		return PREF_NOT_INITIALIZED;

    pref = (CProfile::PrefNode*) PR_HashTableLookup(m_PrefsHash, pref_name);
	if (pref && PREF_HAS_LI_VALUE(pref)) {
		pref->flags &= ~PREF_LILOCAL;
		if (CallbacksEnabled())
    		pref_DoCallback(pref_name);
		success = PREF_OK;
	}

    return success;
}
#endif


char *
CProfile::DumpPrefs()
{
	PrefChildIter pcs;

	if (!m_PrefsHash)
		return NULL;
    
    pcs.bufsize = 8192;
	pcs.childList = (char*) XP_ALLOC(sizeof(char) * pcs.bufsize);
	pcs.childList[0] = '\0';
	XP_STRCAT(pcs.childList, "<HTML>");	

	PR_HashTableEnumerateEntries(m_PrefsHash, pref_printDebugInfo, &pcs);
	
	return pcs.childList;
}

void
CProfile::SetCallbacks(const PRBool status)
{
    if (status) {
        m_ProfileFlags |= PROFILE_CALLBACKS_ENABLED;
    } else {
        m_ProfileFlags &= ~PROFILE_CALLBACKS_ENABLED;
    }
}


/* Dump debugging info in response to about:config.
 */
static int
pref_printDebugInfo(PRHashEntry *he, int i, void *arg)
{
	char *buf1, *buf2;
	CProfile::PrefValue val;
	PrefChildIter* pcs = (PrefChildIter*) arg;
	CProfile::PrefNode *pref = (CProfile::PrefNode *) he->value;
	
	if (PREF_HAS_USER_VALUE(pref) && !PREF_IS_LOCKED(pref)) {
		buf1 = PR_smprintf("<font color=\"blue\">%s = ", (char*) he->key);
		val = pref->userPref;
	}
	else {
		buf1 = PR_smprintf("<font color=\"%s\">%s = ",
			PREF_IS_LOCKED(pref) ? "red" : (PREF_IS_CONFIG(pref) ? "black" : "green"),
			(char*) he->key);
		val = pref->defaultPref;
	}
	
	if (pref->flags & PREF_STRING) {
		buf2 = PR_smprintf("%s %s</font><br>", buf1, val.stringVal);
	}
	else if (pref->flags & PREF_INT) {
		buf2 = PR_smprintf("%s %d</font><br>", buf1, val.intVal);
	}
	else if (pref->flags & PREF_BOOL) {
		buf2 = PR_smprintf("%s %s</font><br>", buf1, val.boolVal ? "true" : "false");
	}
	
	if ((strlen(buf2) + strlen(pcs->childList) + 1) > pcs->bufsize) {
		pcs->bufsize *= 3;
		pcs->childList = (char*) realloc(pcs->childList, sizeof(char) * pcs->bufsize);
		if (!pcs->childList)
			return HT_ENUMERATE_STOP;
	}
	XP_STRCAT(pcs->childList, buf2);
	XP_FREE(buf1);
	XP_FREE(buf2);
	return 0;
}


    PRBool
profile_FileExists(const char *nativePath)
{
    PRBool         retVal = PR_FALSE;
    PRFileInfo      fInfo;

	if ( PR_GetFileInfo((char *) nativePath, &fInfo) != -1 ) {
        retVal = PR_TRUE;
    }

    return retVal;
}


    char *
profile_AllocCombineStr(const char *str1, const char *str2)
{
    int len1, len2;
    char *retStr;

    len1 = XP_STRLEN(str1);
    len2 = XP_STRLEN(str2);

    retStr = (char *) XP_ALLOC((len1 + len2 + 1) * sizeof(char));

    if (retStr) {
        XP_MEMCPY(retStr, str1, len1);
        XP_MEMCPY(retStr+len1, str2, len2+1);
    }

    return retStr;
}

static int
ReadCharPrefUsing(const char *pref_name, void** return_value, int *size, CharPrefReadFunc inFunc)
{
	char* buf;
	int result;

/*	if (!m_mochaPrefObject || !return_value)*/
	if (!return_value)
		return -1;
	*return_value = NULL;

	result = inFunc(pref_name, &buf);

	if (result == PREF_NOERROR) {
		if (strlen(buf) == 0) {		/* do not decode empty string? */
			XP_FREE(buf);
			return -1;
		}
	
		*return_value = DecodeBase64Buffer(buf);
		*size = strlen(buf);
		
		XP_FREE(buf);
	}
	return result;
}

