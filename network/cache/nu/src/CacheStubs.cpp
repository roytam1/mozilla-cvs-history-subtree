/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/* Design and original implementation by Gagan Saksena '98 */

#include "CacheStubs.h"
#include "nsCacheManager.h"
#include "nsDiskModule.h"
#include "nsMemModule.h"
#include "nsCacheTrace.h"

#define CACHEMGR nsCacheManager::GetInstance()

/* CacheManager functions */
PRBool
CacheManager_Contains(const char* i_url)
{
    return CACHEMGR->Entries();
}

PRUint32 
CacheManager_Entries()
{
    return CACHEMGR->Entries();
}

void*
CacheManager_GetObject(const char* i_url)
{
    return CACHEMGR->GetObj(i_url);
}

PRBool
CacheManager_IsOffline(void)
{
    return CACHEMGR->IsOffline();
}

void
CacheManager_Offline(PRBool bSet)
{
    CACHEMGR->Offline(bSet);
}

PRUint32
CacheManager_WorstCaseTime(void)
{
    return CACHEMGR->WorstCaseTime();
}

/* CacheObject functions */
void*
CacheObject_Create(const char* i_url)
{
    return new nsCacheObject(i_url);
}

const char*
CacheObject_GetAddress(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->Address() : 0;
}

const char*
CacheObject_GetEtag(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->Etag() : 0;
}

PRIntervalTime
CacheObject_GetExpires(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->Expires() : 0;
}

PRIntervalTime
CacheObject_GetLastAccessed(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->LastAccessed() : 0;
}

PRIntervalTime
CacheObject_GetLastModified(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->LastModified() : 0;
}

PRUint32
CacheObject_GetSize(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->Size() : 0;
}

PRUint32
CacheObject_Hits(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->Hits() : 0;
}

PRBool
CacheObject_IsExpired(const void* pThis)
{
    return pThis ? ((nsCacheObject*)pThis)->IsExpired() : PR_FALSE;
}

void
CacheObject_SetAddress(void* pThis, const char* i_Address)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->Address(i_Address);
    }
}

void
CacheObject_SetEtag(void* pThis, const char* i_Etag)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->Etag(i_Etag);
    }
    
}

void
CacheObject_SetExpires(void *pThis, const PRIntervalTime i_Time)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->Expires(i_Time);
    }
    
}

void
CacheObject_SetLastModified(void* pThis, const PRIntervalTime i_Time)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->LastModified(i_Time);
    }
    
}

void
CacheObject_SetSize(void* pThis, const PRUint32 i_Size)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->Size(i_Size);
    }
    
}

void
CacheObject_Destroy(void* pThis)
{
    if (pThis)
    {
        ((nsCacheObject*)pThis)->~nsCacheObject();
        pThis = 0;
    }
    
}

/* CachePref functions */
PRUint32
CachePref_GetDiskCacheSize(void)
{
    return nsCachePref::DiskCacheSize();
}

PRBool
CachePref_GetDiskCacheSSL(void)
{
    return nsCachePref::DiskCacheSSL();
}

PRUint32
CachePref_GetMemCacheSize(void)
{
    return nsCachePref::MemCacheSize();
}

void
CachePref_SetDiskCacheSize(const PRUint32 i_Size)
{
    nsCachePref::DiskCacheSize(i_Size);
}

void
CachePref_SetDiskCacheSSL(PRBool bSet)
{
    nsCachePref::DiskCacheSSL(bSet);
}

void
CachePref_SetMemCacheSize(const PRUint32 i_Size)
{
    nsCachePref::MemCacheSize(i_Size);
}

/* CacheTrace functions */
void
CacheTrace_Enable(PRBool bEnable)
{
    nsCacheTrace::Enable(bEnable);
}

PRBool
CacheTrace_IsEnabled(void)
{
    return nsCacheTrace::IsEnabled();
}

/* DiskModule functions */
#define DISKMOD nsCacheManager::GetInstance()->GetDiskModule()

PRBool
DiskModule_AddObject(void* pObject)
{
    return DISKMOD->AddObject((nsCacheObject*)pObject);
}

PRBool
DiskModule_Contains(const char* i_url)
{
    return DISKMOD->Contains(i_url);
}

PRUint32
DiskModule_Entries(void)
{
    return DISKMOD->Entries();
}

PRUint32
DiskModule_GetSize(void)
{
    return DISKMOD->Size();
}

PRUint32
DiskModule_GetSizeInUse(void)
{
    return DISKMOD->SizeInUse(); 
}

PRBool
DiskModule_IsEnabled(void)
{
    return DISKMOD->IsEnabled();
}

PRBool
DiskModule_Remove(const char* i_url)
{
    return DISKMOD->Remove(i_url);
}

PRBool
DiskModule_RemoveAll(void)
{
    return DISKMOD->RemoveAll();
}

void
DiskModule_SetSize(PRUint32 i_Size)
{
    DISKMOD->Size(i_Size);
}

/* MemModule functions */
#define MEMMOD nsCacheManager::GetInstance()->GetMemModule()

PRBool
MemModule_AddObject(void* pObject)
{
    return MEMMOD->AddObject((nsCacheObject*)pObject);
}

PRBool
MemModule_Contains(const char* i_url)
{
    return MEMMOD->Contains(i_url);
}

PRUint32
MemModule_Entries(void)
{
    return MEMMOD->Entries();
}

PRUint32
MemModule_GetSize(void)
{
    return MEMMOD->Size();
}

PRUint32
MemModule_GetSizeInUse(void)
{
    return MEMMOD->SizeInUse() ;
}

PRBool
MemModule_IsEnabled(void)
{
    return MEMMOD->IsEnabled();
}

PRBool
MemModule_Remove(const char* i_url)
{
    return MEMMOD->Remove(i_url);
}

PRBool
MemModule_RemoveAll(void)
{
    return MEMMOD->RemoveAll();
}

void
MemModule_SetSize(PRUint32 i_Size)
{
    MEMMOD->Size(i_Size);
}

#undef MEMMOD
#undef DISKMOD
#undef CACHEMGR
