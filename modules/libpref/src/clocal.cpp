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

#include "xp_core.h"
#include "xp_mcom.h"
#include "xp_str.h"
#include "xp_qsort.h"

#include "jsapi.h"

#include "clocal.h"
#include "prefpriv.h"

#include "sechash.h"
#ifndef NSPR20
#include "prhash.h"
#else
#include "plhash.h"
#endif
#include "prsystem.h"

/* We cheat here and redeclare these types, originally declared in CProfile,
   because we need access to the data in order to be able to properly write out
   the JS file.
*/

typedef union
{
	char*		            stringVal;
	int32		            intVal;
	XP_Bool		            boolVal;
} PrefValue;

typedef struct 
{
    PrefValue	defaultPref;
	PrefValue 	userPref;
	uint8		flags;
} PrefNode;


/* cheating again: the functions that evaluate the JS files should be in this file */
extern  JSContext *m_mochaContext;


static char * EscapeJSString(const char * original);
static int    pref_CompareStrings (const void *v1, const void *v2);

    char *
EscapeJSString(const char * original)
{
	const char *p;
	char * ret_str, *q;

	if (original == NULL)
		return NULL;
	
	ret_str = (char *) XP_ALLOC(2*strlen(original) + 1);  /* Paranoid worse case all slashes will free quickly */
	for(p = original, q=ret_str ; *p; p++, q++)
        switch(*p) {
            case '\\':
                q[0] = '\\';
				q[1] = '\\';
				q++;
                break;
            case '\"':
                q[0] = '\\';
				q[1] = '\"';
				q++;
                break;
            default:
				*q = *p;
                break;
        }
	*q = 0;
	return ret_str;
}

    int
pref_CompareStrings (const void *v1, const void *v2)
{
	char *s1 = *(char**) v1;
	char *s2 = *(char**) v2;

	if (!s1)
	{
		if (!s2)
			return 0;
		else
			return -1;
	}
	else if (!s2)
		return 1;
	else
		return strcmp(s1, s2);
}



    int PREF_HASH_CALLBACK
CPrefStoreLocal::CreatePrefFileEntry(PRHashEntry *he, int i, void *arg)
{
	char **prefArray = (char**) arg;
    PrefNode *pref = (PrefNode *) he->value;

	if (pref && PREF_HAS_USER_VALUE(pref)) {
		char buf[2048];

		if (pref->flags & PREF_STRING) {
			char *tmp_str = EscapeJSString(pref->userPref.stringVal);
			if (tmp_str) {
				PR_snprintf(buf, 2048, "user_pref(\"%s\", \"%s\");" LINEBREAK,
					(char*) he->key, tmp_str);
				XP_FREE(tmp_str);
			}
		}
		else if (pref->flags & PREF_INT) {
			PR_snprintf(buf, 2048, "user_pref(\"%s\", %ld);" LINEBREAK,
				(char*) he->key, (long) pref->userPref.intVal);
		}
		else if (pref->flags & PREF_BOOL) {
			PR_snprintf(buf, 2048, "user_pref(\"%s\", %s);" LINEBREAK, (char*) he->key,
				(pref->userPref.boolVal) ? "true" : "false");
		}

		prefArray[i] = XP_STRDUP(buf);
	} else if (pref && PREF_IS_LOCKED(pref)) {
		char buf[2048];

		if (pref->flags & PREF_STRING) {
			char *tmp_str = EscapeJSString(pref->defaultPref.stringVal);
			if (tmp_str) {
				PR_snprintf(buf, 2048, "user_pref(\"%s\", \"%s\");" LINEBREAK,
					(char*) he->key, tmp_str);
				XP_FREE(tmp_str);
			}
		}
		else if (pref->flags & PREF_INT) {
			PR_snprintf(buf, 2048, "user_pref(\"%s\", %ld);" LINEBREAK,
				(char*) he->key, (long) pref->defaultPref.intVal);
		}
		else if (pref->flags & PREF_BOOL) {
			PR_snprintf(buf, 2048, "user_pref(\"%s\", %s);" LINEBREAK, (char*) he->key,
				(pref->defaultPref.boolVal) ? "true" : "false");
		}

		prefArray[i] = XP_STRDUP(buf);
	}

    return PREF_OK;
}


CPrefStore::CPrefStore(const char *name, uint16 flags)
{
    XP_ASSERT(name);

    m_name = XP_STRDUP(name);
    m_Flags = flags;
}


CPrefStoreLocal::CPrefStoreLocal(const char *name, const char *filename, uint16 flags)
     : CPrefStore(name, flags)
{
    m_fp = NULL;
    m_fileLength = 0L;
    m_fileBuf = NULL;

	m_filename = XP_STRDUP(filename);
}


CPrefStoreLocal::~CPrefStoreLocal()
{

    if (m_fp != NULL) {
        CloseFile();
    }

    if (m_fileBuf != NULL) {
        XP_FREE(m_fileBuf);    
    }

	XP_FREEIF(m_filename);

    return;
}



PROFILE_ERROR
CPrefStoreLocal::OpenFile(const PRIntn permissions)
{
    PROFILE_ERROR       err;
	XP_StatStruct       stats;
    XP_Bool             createFile = FALSE;

	if (!m_filename) {
		return PREF_ERROR;
	}

    if ((m_fp != NULL) && (m_fileLength > 0))
        return PREF_OK;

    if ((err = this->PreOpen(permissions)) < PREF_OK)
        return err;

	stats.st_size = 0;

#ifdef XP_WIN
	if ( stat(m_filename, (struct stat *) &stats) == -1)
#else
	if ( XP_Stat(m_filename, &stats, xpUserPrefs) == -1)
#endif
    {
        if (!(m_Flags & PREF_CREATE_IF_NOT_EXIST))
            return PREF_ERROR;
        else {
            createFile  = TRUE;
        }
    }

    if (createFile) {
        XP_File     createHandle;

        createHandle = fopen(m_filename, XP_FILE_WRITE);
        fputc('\r', createHandle);
        fputc('\r', createHandle);
        if (createHandle) {
            fclose(createHandle);
        }

    } else {
	    m_fileLength = stats.st_size;
	    if (m_fileLength <= 1)
		    return PREF_ERROR;
    }

    m_fp = PR_Open(m_filename, permissions, 0);

    err = this->PostOpen(permissions, err);

    return err;
}


PROFILE_ERROR
CPrefStoreLocal::PreOpen(const PRIntn permissions)
{
    return PREF_OK;
}

PROFILE_ERROR
CPrefStoreLocal::PostOpen(const PRIntn permissions, PROFILE_ERROR err)
{
    return err;
}


PROFILE_ERROR
CPrefStoreLocal::ReadPrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR       err = PREF_ERROR;

    if ((err = this->OpenFile(PR_RDONLY)) < PREF_OK)
        return err;

    if ((err = this->PreRead()) < PREF_OK)
        return err;

    m_fileBuf = (char *) XP_ALLOC(m_fileLength * sizeof(char));

	if (!m_fileBuf) {
        return PREF_OUT_OF_MEMORY;
    }

    m_fileLength = PR_Read(m_fp, m_fileBuf, m_fileLength);


    if ((err = this->PreEvaluate()) < PREF_OK)
        return err;

    if (PREF_EvaluateConfigScript(m_fileBuf, m_fileLength, m_filename,
        m_Flags & PREF_GLOBAL_CONTEXT, FALSE, m_Flags & PREF_SKIP_FIRST) == JS_TRUE) {
        err = PREF_OK;
    }

    JS_GC(m_mochaContext);

    err = this->PostRead(err);

    XP_FREE(m_fileBuf);

    return err;
}


PROFILE_ERROR
CPrefStoreLocal::PreRead(void)
{
    return PREF_OK;
}

PROFILE_ERROR
CPrefStoreLocal::PreEvaluate(void)
{
    return PREF_OK;
}

PROFILE_ERROR
CPrefStoreLocal::PostRead(PROFILE_ERROR err)
{
    return err;
}


PROFILE_ERROR
CPrefStoreLocal::CloseFile(void)
{
    PROFILE_ERROR       err;

    if (!m_fp) {
        return PREF_OK;
    }

    if ((err = this->PreClose()) < PREF_OK)
        return err;

	err = PR_Close(m_fp);

    err = this->PostClose(err);

    m_fp = NULL;

    return err;
}


PROFILE_ERROR
CPrefStoreLocal::PreClose(void)
{
    return PREF_OK;
}

PROFILE_ERROR
CPrefStoreLocal::PostClose(PROFILE_ERROR err)
{
    return err;
}


PROFILE_ERROR
CPrefStoreLocal::LoadPrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR       err;

    err = OpenFile(PR_RDONLY);

    if (err >= PREF_OK) {
        err = ReadPrefs(prefsHash);
    }

    CloseFile();

    return err;
}


PROFILE_ERROR
CPrefStoreLocal::SavePrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR   err = PREF_ERROR;

    if (m_Flags & PREF_READ_ONLY) {
        return PREF_OK;
    }

    err = OpenFile(PR_WRONLY);

    if ((err = this->PreWrite()) < PREF_OK)
        return err;

    err = WritePrefs(prefsHash);

    err = PostWrite(err);
    CloseFile();

	return err;
}

PROFILE_ERROR
CPrefStoreLocal::PreWrite(void)
{
    return PREF_OK;
}

PROFILE_ERROR
CPrefStoreLocal::WritePrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR   err = PREF_ERROR;
	char            **valueArray = NULL;
	unsigned int    valueIdx;
    const char      *header1 = "// Netscape User Preferences" LINEBREAK;
    const char      *header2 = "// This is a generated file!  Do not edit." LINEBREAK LINEBREAK;

	valueArray = (char**) XP_CALLOC(sizeof(char*), prefsHash->nentries);
	if (!valueArray)
		return PREF_OUT_OF_MEMORY;

	PR_Write(m_fp, header1, XP_STRLEN(header1));
	PR_Write(m_fp, header2, XP_STRLEN(header2));
	
    PR_HashTableEnumerateEntries(prefsHash, CPrefStoreLocal::CreatePrefFileEntry, valueArray);
	
	/* Sort the preferences to make a readable file on disk */
	XP_QSORT (valueArray, prefsHash->nentries, sizeof(char*), pref_CompareStrings);
	for (valueIdx = 0; valueIdx < prefsHash->nentries; valueIdx++)
	{
		if (valueArray[valueIdx])
		{
			PR_Write(m_fp, valueArray[valueIdx], XP_STRLEN(valueArray[valueIdx]));
			XP_FREE(valueArray[valueIdx]);
		}
	}

	err = PREF_OK;

    XP_FREE(valueArray);

	return err;
}

PROFILE_ERROR
CPrefStoreLocal::PostWrite(PROFILE_ERROR err)
{
    return err;
}


PROFILE_ERROR
CPrefStoreLocal::HandleError(PROFILE_ERROR err)
{
    PROFILE_ERROR       result = PREF_OK;

    if (err < PREF_OK) {
        /* if an error occurred while reading these preferences, don't save them back out */
        m_Flags |= PREF_READ_ONLY;

        if (m_Flags & PREF_REQUIRED_STORE) {
            result = err;
        }

        /* FIXME! Make this non platform specific and non-hardcoded */
#ifdef XP_WIN
        MessageBox(NULL,"Error in preference file (prefs.js [%s]).  Default preferences will be used.","Netscape - Warning", MB_OK);
#endif

    }

    return result;
}

    char *
CPrefStoreLocal::GetFilename(char *filename, unsigned int *len)
{
    PROFILE_ERROR       err = PREF_OK;
    char                *fileStr;

    if (!filename) {
        fileStr = XP_STRDUP(m_filename);
    } else {
    	XP_STRNCPY_SAFE(filename, m_filename, *len-1);
        *len = XP_STRLEN(m_filename);
        fileStr = filename;
    }

	return fileStr;
}


CPrefStoreLocalLI::CPrefStoreLocalLI(const char *name, const char *filename, uint16 flags) :
    CPrefStoreLocal(name, filename, flags)
{
    return;
}



PROFILE_ERROR
CPrefStoreLocalLI::WritePrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR   err = PREF_ERROR;
	char            **valueArray = NULL;
	unsigned int    valueIdx;
    const char      *header1 = "// Netscape User Preferences" LINEBREAK;
    const char      *header2 = "// This is a generated file!  Do not edit." LINEBREAK LINEBREAK;

	valueArray = (char**) XP_CALLOC(sizeof(char*), prefsHash->nentries);
	if (!valueArray)
		return PREF_OUT_OF_MEMORY;

	PR_Write(m_fp, header1, XP_STRLEN(header1));
	PR_Write(m_fp, header2, XP_STRLEN(header2));

    PR_HashTableEnumerateEntries(prefsHash, CPrefStoreLocalLI::CreatePrefFileEntry, valueArray);
	
	/* Sort the preferences to make a readable file on disk */
	XP_QSORT (valueArray, prefsHash->nentries, sizeof(char*), pref_CompareStrings);
	for (valueIdx = 0; valueIdx < prefsHash->nentries; valueIdx++)
	{
		if (valueArray[valueIdx])
		{
			PR_Write(m_fp, valueArray[valueIdx], XP_STRLEN(valueArray[valueIdx]));
			XP_FREE(valueArray[valueIdx]);
		}
	}

	err = PREF_OK;

    XP_FREE(valueArray);

	return err;
}


    int PREF_HASH_CALLBACK
CPrefStoreLocalLI::CreatePrefFileEntry(PRHashEntry *he, int i, void *arg)
{
	char **prefArray = (char**) arg;
    PrefNode *pref = (PrefNode *) he->value;

	if (pref && !PREF_HAS_LI_VALUE(pref)) {
        CPrefStoreLocal::CreatePrefFileEntry(he, i, arg);
    }

    return 0;
}


CPrefStoreLockFile::CPrefStoreLockFile(const char *name, const char *filename, uint16 flags)
    : CPrefStoreLocal(name, filename, flags)
{
    m_Flags |= (PREF_GLOBAL_CONTEXT) | (PREF_READ_ONLY);
    m_Flags &= ~(PREF_SKIP_FIRST);

}

PROFILE_ERROR
CPrefStoreLockFile::PreEvaluate(void)
{
    if (VerifyLockFile(m_fileBuf, m_fileLength) == FALSE)
        return PREF_BAD_LOCKFILE;

    return PREF_OK;
}

XP_Bool 
CPrefStoreLockFile::VerifyLockFile(char* buf, unsigned long buflen)
{
/* Computes the MD5 hash of the given buffer (not including the first line)
   and verifies the first line of the buffer expresses the correct hash in the form:
   // xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
   where each 'xx' is a hex value. */

	XP_Bool success = FALSE;
	const int obscure_value = 7;
	const long hash_length = 51;		/* len = 48 chars of MD5 + // + EOL */
    unsigned char digest[16];
    char szHash[64];

	/* Unobscure file by subtracting some value from every char. */
	unsigned long i;
	for (i = 0; i < buflen; i++) {
		buf[i] -= obscure_value;
	}

    if (buflen >= hash_length) {
    	const unsigned char magic_key[] = "VonGloda5652TX75235ISBN";
	    unsigned char *pStart = (unsigned char*) buf + hash_length;
	    unsigned int len;
	    
	    MD5Context * md5_cxt = MD5_NewContext();
		MD5_Begin(md5_cxt);
		
		/* start with the magic key */
		MD5_Update(md5_cxt, magic_key, sizeof(magic_key));

		MD5_Update(md5_cxt, pStart, buflen - hash_length);
		
		MD5_End(md5_cxt, digest, &len, 16);
		
		MD5_DestroyContext(md5_cxt, PR_TRUE);
		
	    XP_SPRINTF(szHash, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
	        (int)digest[0],(int)digest[1],(int)digest[2],(int)digest[3],
	        (int)digest[4],(int)digest[5],(int)digest[6],(int)digest[7],
	        (int)digest[8],(int)digest[9],(int)digest[10],(int)digest[11],
	        (int)digest[12],(int)digest[13],(int)digest[14],(int)digest[15]);

		success = ( strncmp((const char*) buf + 3, szHash, hash_length - 4) == 0 );
	}
	
	return success;
}

CPrefStoreLockFile::SavePrefs(PRHashTable* prefsHash)
{
    return PREF_OK;

    /* unused: prefsHash */
}


