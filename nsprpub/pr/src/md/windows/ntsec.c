/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "NPL"); you may not use this file except in
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
 * Copyright (C) 2000 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "primpl.h"

/*
 * ntsec.c
 *
 * Implement the POSIX-style mode bits (access permissions) for
 * files and other securable objects in Windows NT using the
 * Windows NT security model.
 */

/*
 * The security identifiers (SIDs) for owner, primary group,
 * and the Everyone (World) group.
 */
static struct {
    PSID owner;
    PSID group;
    PSID everyone;
} _pr_nt_sids;

/*
 * Initialize the SIDs for owner, primary group, and the Everyone
 * group in the _pr_nt_sids structure.
 *
 * This function needs to be called by NSPR initialization.
 */
void _PR_NT_InitSids(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    HANDLE hToken;
    UCHAR infoBuffer[1024];
    PTOKEN_OWNER pTokenOwner = (PTOKEN_OWNER) infoBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroup
            = (PTOKEN_PRIMARY_GROUP) infoBuffer;
    DWORD dwLength;
    BOOL rv;

    /* Create a well-known SID for the Everyone group. */
    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &_pr_nt_sids.everyone)) {
        /*
         * On non-NT systems, this function is not implemented,
         * and neither are the other security functions. There
         * is no point in going further.
         */
        PR_ASSERT(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);
        return;
    }

    /*
     * Look up and make a copy of the owner and primary group
     * SIDs in the access token of the calling process.
     */
    rv = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    PR_ASSERT(rv != 0);

    rv = GetTokenInformation(hToken, TokenOwner, infoBuffer,
            sizeof(infoBuffer), &dwLength);
    PR_ASSERT(rv != 0);
    dwLength = GetLengthSid(pTokenOwner->Owner);
    _pr_nt_sids.owner = (PSID) HeapAlloc(GetProcessHeap(), 0, dwLength);
    PR_ASSERT(_pr_nt_sids.owner != NULL);
    rv = CopySid(dwLength, _pr_nt_sids.owner, pTokenOwner->Owner);
    PR_ASSERT(rv != 0);

    rv = GetTokenInformation(hToken, TokenPrimaryGroup, infoBuffer,
            sizeof(infoBuffer), &dwLength);
    PR_ASSERT(rv != 0);
    dwLength = GetLengthSid(pTokenPrimaryGroup->PrimaryGroup);
    _pr_nt_sids.group = (PSID) HeapAlloc(GetProcessHeap(), 0, dwLength);
    PR_ASSERT(_pr_nt_sids.group != NULL);
    rv = CopySid(dwLength, _pr_nt_sids.group,
            pTokenPrimaryGroup->PrimaryGroup);
    PR_ASSERT(rv != 0);

    rv = CloseHandle(hToken);
    PR_ASSERT(rv != 0);
}

/*
 * Free the SIDs for owner, primary group, and the Everyone group
 * in the _pr_nt_sids structure.
 *
 * This function needs to be called by NSPR cleanup.
 */
void
_PR_NT_FreeSids(void)
{
    if (_pr_nt_sids.owner) {
        HeapFree(GetProcessHeap(), 0, (LPVOID) _pr_nt_sids.owner);
    }
    if (_pr_nt_sids.group) {
        HeapFree(GetProcessHeap(), 0, (LPVOID) _pr_nt_sids.group);
    }
    if (_pr_nt_sids.everyone) {
        FreeSid(_pr_nt_sids.everyone);
    }
}

/*
 * Construct a security descriptor whose discretionary access-control
 * list implements the specified mode bits.  The SIDs for owner, group,
 * and everyone are obtained from the global _pr_nt_sids structure.
 * Both the security descriptor and access-control list are returned
 * and should be freed by a _PR_NT_FreeSecurityDescriptorACL call.
 */
PRStatus
_PR_NT_MakeSecurityDescriptorACL(
    PRIntn mode,
    PSECURITY_DESCRIPTOR *resultSD,
    PACL *resultACL)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;
    DWORD cbACL = 1024;
    DWORD accessMask;

    if (_pr_nt_sids.owner == NULL) {
        PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
        return PR_FAILURE;
    }

    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
            SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pSD == NULL) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!SetSecurityDescriptorOwner(pSD, _pr_nt_sids.owner, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!SetSecurityDescriptorGroup(pSD, _pr_nt_sids.group, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    /* Initialize a new ACL. */

    pACL = (PACL) LocalAlloc(LPTR, cbACL);
    if (pACL == NULL) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!InitializeAcl(pACL, cbACL, ACL_REVISION)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00400) accessMask |= GENERIC_READ;
    if (mode & 00200) accessMask |= GENERIC_WRITE;
    if (mode & 00100) accessMask |= GENERIC_EXECUTE;
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.owner)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00040) accessMask |= GENERIC_READ;
    if (mode & 00020) accessMask |= GENERIC_WRITE;
    if (mode & 00010) accessMask |= GENERIC_EXECUTE;
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.group)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00004) accessMask |= GENERIC_READ;
    if (mode & 00002) accessMask |= GENERIC_WRITE;
    if (mode & 00001) accessMask |= GENERIC_EXECUTE;
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.everyone)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    *resultSD = pSD;
    *resultACL = pACL;
    return PR_SUCCESS;

failed:
    if (pSD) {
        LocalFree((HLOCAL) pSD);
    }
    if (pACL) {
        LocalFree((HLOCAL) pACL);
    }
    return PR_FAILURE;
}

/*
 * Free the specified security descriptor and access-control list
 * previously created by _PR_NT_MakeSecurityDescriptorACL.
 */
void
_PR_NT_FreeSecurityDescriptorACL(PSECURITY_DESCRIPTOR pSD, PACL pACL)
{
    if (pSD) {
        LocalFree((HLOCAL) pSD);
    }
    if (pACL) {
        LocalFree((HLOCAL) pACL);
    }
}
