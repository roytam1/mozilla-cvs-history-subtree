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

#include "cremote.h"
#include "prefpriv.h"


CPrefStoreRemote::CPrefStoreRemote(const char *name, const char *url, uint16 flags)
     : CPrefStore(name, flags)
{
    m_Flags |= (PREF_GLOBAL_CONTEXT) | (PREF_READ_ONLY);

    if (url) {
        m_URL = XP_STRDUP(url);
    } else {
        m_URL = NULL;
    }

    m_LIProtocol = 0;
    m_LIParams = 0;
    m_LI_LDAPSearchBase = NULL;

}


CPrefStoreRemote::~CPrefStoreRemote()
{

	XP_FREEIF(m_URL);
	XP_FREEIF(m_LI_LDAPSearchBase);

    return;
}

PROFILE_ERROR
CPrefStoreRemote::LoadPrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR       err = PREF_OK;

/*    NET_DownloadAutoAdminCfgFile(); */

    return err;
}

PROFILE_ERROR
CPrefStoreRemote::SavePrefs(PRHashTable* prefsHash)
{
    PROFILE_ERROR       err = PREF_OK;

    return err;
}

PROFILE_ERROR
CPrefStoreRemote::HandleError(PROFILE_ERROR err)
{
    PROFILE_ERROR       result = PREF_OK;

    if (err < PREF_OK) {
        /* if an error occured, fallback */
    }

    return result;
}

char *
CPrefStoreRemote::GetURL(char *url, unsigned int *len)
{
    PROFILE_ERROR       err = PREF_OK;
    char                *urlStr;

    if (!url) {
        urlStr = XP_STRDUP(m_URL);
    } else {
    	XP_STRNCPY_SAFE(url, m_URL, *len-1);
        *len = XP_STRLEN(m_URL);
        urlStr = url;
    }

	return urlStr;
}

    void
CPrefStoreRemote::SetURL(const char *url)
{
    if (m_URL) {
        XP_FREE(m_URL);
    }

    m_URL = XP_STRDUP(url);

    if (m_LIProtocol == PROF_REMOTE_LI_USEHTTP) {
    	PREF_SetCharPref("li.server.http.baseURL",url);
    } else if (m_LIProtocol == PROF_REMOTE_LI_USELDAP) {
  		PREF_SetCharPref("li.server.ldap.url",url);
    }

    return;
}


    void
CPrefStoreRemote::SetLISearchBase(const char *searchpath)
{
    if (m_LI_LDAPSearchBase) {
        XP_FREE(m_LI_LDAPSearchBase);
    }

    m_LI_LDAPSearchBase = XP_STRDUP(searchpath);
    
    PREF_SetCharPref("li.server.ldap.userbase",searchpath);

    return;
}


    PROFILE_ERROR
CPrefStoreRemote::SetLIParams(const uint16 liFlags)
{
    m_LIParams = liFlags;

    PREF_SetBoolPref("li.client.addressbook", (liFlags & PROF_REMOTE_LI_ADDRESSBOOK));
    PREF_SetBoolPref("li.client.bookmarks", (liFlags & PROF_REMOTE_LI_BOOKMARKS));
    PREF_SetBoolPref("li.client.cookies", (liFlags & PROF_REMOTE_LI_COOKIES));
    PREF_SetBoolPref("li.client.filters", (liFlags & PROF_REMOTE_LI_FILTERS));
    PREF_SetBoolPref("li.client.javasecurity", (liFlags & PROF_REMOTE_LI_JAVA));
    PREF_SetBoolPref("li.client.security", (liFlags & PROF_REMOTE_LI_SECURITY));
    PREF_SetBoolPref("li.client.globalhistory", (liFlags & PROF_REMOTE_LI_HISTORY));
    PREF_SetBoolPref("li.client.liprefs", (liFlags & PROF_REMOTE_LI_PREFS));

    return PREF_OK;
}

    PROFILE_ERROR
CPrefStoreRemote::SetLIProtocol(int protocol)
{
    PROFILE_ERROR       err = PREF_OK;

    m_LIProtocol = protocol;

    if (protocol == PROF_REMOTE_LI_USEHTTP) {
        PREF_SetCharPref("li.protocol","http");
    } else if (protocol == PROF_REMOTE_LI_USELDAP) {
    	PREF_SetCharPref("li.protocol","ldap");
    } else {
        err = PREF_BAD_PARAMETER;       /* unsupported protocol */
    }

    return err;
}
    


CPrefStoreRemoteLDAP::CPrefStoreRemoteLDAP(const char *name, const char *url, uint16 flags)
     : CPrefStoreRemote(name, url, flags)
{
    m_Flags |= (PREF_GLOBAL_CONTEXT);
    m_Flags &= (~PREF_READ_ONLY);

    m_Username = NULL;
    m_PrefsLDAPSearchBase = NULL;
}


CPrefStoreRemoteLDAP::~CPrefStoreRemoteLDAP()
{

	XP_FREEIF(m_Username);
	XP_FREEIF(m_PrefsLDAPSearchBase);

    return;
}

    char *
CPrefStoreRemoteLDAP::GetUsername(char *username, unsigned int *len)
{
    PROFILE_ERROR       err = PREF_OK;
    char                *nameStr;

    if (!username) {
        nameStr = XP_STRDUP(m_URL);
    } else {
    	XP_STRNCPY_SAFE(username, m_Username, *len-1);
        *len = XP_STRLEN(m_Username);
        nameStr = username;
    }

	return nameStr;
}

    void
CPrefStoreRemoteLDAP::SetUsername(const char *username)
{
    if (m_Username) {
        XP_FREE(m_Username);
    }

    m_Username = XP_STRDUP(username);

    PREF_SetCharPref("li.login.name",username);

    return;
}

    void
CPrefStoreRemoteLDAP::SetPrefsSearchBase(const char *searchpath)
{
    if (m_PrefsLDAPSearchBase) {
        XP_FREE(m_PrefsLDAPSearchBase);
    }

    m_PrefsLDAPSearchBase = XP_STRDUP(searchpath);

    return;
}

