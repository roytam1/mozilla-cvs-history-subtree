/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is nsDiskCacheDevice.cpp, released February 22, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan <gordon@netscape.com>
 *    Patrick C. Beard <beard@netscape.com>
 */

#include <limits.h>

#include "nsDiskCacheDevice.h"
#include "nsDiskCacheEntry.h"
#include "nsDiskCacheMap.h"
#include "nsDiskCache.h"

#include "nsCacheService.h"
#include "nsCache.h"

#include "nsIFileTransportService.h"
#include "nsITransport.h"
#include "nsICacheVisitor.h"
#include "nsXPIDLString.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISupportsArray.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsIPref.h"

#include "nsQuickSort.h"

static nsresult ensureCacheDirectory(nsIFile * cacheDirectory);

static const char DISK_CACHE_DEVICE_ID[] = { "disk" };


/******************************************************************************
 *  nsDiskCacheObserver
 *****************************************************************************/

// Using nsIPref will be subsumed with nsIDirectoryService when a selector
// for the cache directory has been defined.

#define CACHE_DIR_PREF "browser.newcache.directory"
#define CACHE_DISK_CAPACITY_PREF "browser.cache.disk_cache_size"

class nsDiskCacheObserver : public nsIObserver {
    nsDiskCacheDevice* mDevice;
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsDiskCacheObserver(nsDiskCacheDevice * device)
        :   mDevice(device)
    {
        NS_INIT_ISUPPORTS();
    }
    
    virtual ~nsDiskCacheObserver() {}
};

NS_IMPL_ISUPPORTS1(nsDiskCacheObserver, nsIObserver);

NS_IMETHODIMP nsDiskCacheObserver::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
    nsresult rv;
    
    // did a preference change?
    if (NS_LITERAL_STRING("nsPref:changed").Equals(aTopic)) {
        nsCOMPtr<nsIPref> prefs = do_QueryInterface(aSubject, &rv);
        if (!prefs) {
            prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
            if (NS_FAILED(rv)) return rv;
        }
        
        // which preference changed?
        nsLiteralString prefName(someData);
        if (prefName.Equals(NS_LITERAL_STRING(CACHE_DIR_PREF))) {
        	nsCOMPtr<nsILocalFile> cacheDirectory;
            rv = prefs->GetFileXPref(CACHE_DIR_PREF, getter_AddRefs(cacheDirectory));
        	if (NS_SUCCEEDED(rv)) {
                rv = ensureCacheDirectory(cacheDirectory);
                if (NS_SUCCEEDED(rv))
                    mDevice->setCacheDirectory(cacheDirectory);
            }
        } else
        if (prefName.Equals(NS_LITERAL_STRING(CACHE_DISK_CAPACITY_PREF))) {
            PRInt32 cacheCapacity;
            rv = prefs->GetIntPref(CACHE_DISK_CAPACITY_PREF, &cacheCapacity);
        	if (NS_SUCCEEDED(rv))
                mDevice->setCacheCapacity(cacheCapacity * 1024);
        }
    }  else if (NS_LITERAL_STRING("profile-do-change").Equals(aTopic)) {
        // XXX need to regenerate the cache directory. hopefully the
        // cache service has already been informed of this change.
        nsCOMPtr<nsIFile> profileDir;
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, 
                                    getter_AddRefs(profileDir));
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsILocalFile> cacheDirectory = do_QueryInterface(profileDir);
            if (cacheDirectory) {
                cacheDirectory->Append("NewCache");
                // always make sure the directory exists, we may have just switched to a new profile dir.
                rv = ensureCacheDirectory(cacheDirectory);
                if (NS_SUCCEEDED(rv))
                    mDevice->setCacheDirectory(cacheDirectory);
            }
        }
    }
    
    return NS_OK;
}

static nsresult ensureCacheDirectory(nsIFile * cacheDirectory)
{
    // make sure the Cache directory exists.
    PRBool exists;
    nsresult rv = cacheDirectory->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = cacheDirectory->Create(nsIFile::DIRECTORY_TYPE, 0777);
    return rv;
}

static nsresult installObservers(nsDiskCacheDevice* device)
{
	nsresult rv;
	nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
	if (NS_FAILED(rv))
		return rv;

    nsCOMPtr<nsIObserver> observer = new nsDiskCacheObserver(device);
    if (!observer) return NS_ERROR_OUT_OF_MEMORY;
    
    device->setPrefsObserver(observer);
    
    rv = prefs->AddObserver(CACHE_DISK_CAPACITY_PREF, observer);
	if (NS_FAILED(rv))
		return rv;

    PRInt32 cacheCapacity;
    rv = prefs->GetIntPref(CACHE_DISK_CAPACITY_PREF, &cacheCapacity);
    if (NS_FAILED(rv)) {
#if DEBUG
        const PRInt32 kTenMegabytes = 10 * 1024 * 1024;
        rv = prefs->SetIntPref(CACHE_DISK_CAPACITY_PREF, kTenMegabytes);
#else
		return rv;
#endif
    } else {
        // XXX note the units of the pref seems to be in kilobytes.
        device->setCacheCapacity(cacheCapacity * 1024);
    }

    rv = prefs->AddObserver(CACHE_DIR_PREF, observer);
	if (NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsILocalFile> cacheDirectory;
    rv = prefs->GetFileXPref(CACHE_DIR_PREF, getter_AddRefs(cacheDirectory));
    if (NS_FAILED(rv)) {
        // XXX no preference has been defined, use the directory service. instead.
        nsCOMPtr<nsIFile> profileDir;
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, 
                                    getter_AddRefs(profileDir));
        if (NS_SUCCEEDED(rv)) {
            // XXX this path will probably only succeed when running under Mozilla.
            // store the new disk cache files in a directory called NewCache
            // in the interim.
            cacheDirectory = do_QueryInterface(profileDir, &rv);
        	if (NS_FAILED(rv))
        		return rv;

            rv = cacheDirectory->Append("NewCache");
        	if (NS_FAILED(rv))
        		return rv;

            // XXX since we didn't find a preference, we'll assume that the cache directory
            // can only change when profiles change, so remove the prefs listener and install
            // a profile listener.
            prefs->RemoveObserver(CACHE_DIR_PREF, observer);
            
            nsCOMPtr<nsIObserverService> observerService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
            if (observerService)
                observerService->AddObserver(observer, NS_LITERAL_STRING("profile-do-change").get());
        } else {
#if DEBUG
            // XXX use current process directory during development only.
            nsCOMPtr<nsIFile> currentProcessDir;
            rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, 
                                        getter_AddRefs(currentProcessDir));
        	if (NS_FAILED(rv))
        		return rv;

            cacheDirectory = do_QueryInterface(currentProcessDir, &rv);
        	if (NS_FAILED(rv))
        		return rv;

            rv = cacheDirectory->Append("Cache");
        	if (NS_FAILED(rv))
        		return rv;
#else
            return rv;
#endif
        }
    }

    // always make sure the directory exists, the user may blow it away.
    rv = ensureCacheDirectory(cacheDirectory);
    if (NS_FAILED(rv))
        return rv;

    // cause the preference to be set up initially.
    device->setCacheDirectory(cacheDirectory);
    
    return NS_OK;
}

static nsresult removeObservers(nsDiskCacheDevice* device)
{
    nsresult rv;

    nsCOMPtr<nsIObserver> observer;
    device->getPrefsObserver(getter_AddRefs(observer));
    device->setPrefsObserver(nsnull);
    NS_ASSERTION(observer, "removePrefListeners");

    if (observer) {
    	nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &rv);
    	if (prefs) {
            prefs->RemoveObserver(CACHE_DISK_CAPACITY_PREF, observer);
            prefs->RemoveObserver(CACHE_DIR_PREF, observer);
        }
        
        nsCOMPtr<nsIObserverService> observerService = do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
        if (observerService)
            observerService->RemoveObserver(observer, NS_LITERAL_STRING("profile-do-change").get());
    }
    
    return NS_OK;
}


/******************************************************************************
 *  nsDiskCacheDeviceInfo
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark DISK CACHE DEVICE INFO
#endif

class nsDiskCacheDeviceInfo : public nsICacheDeviceInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEDEVICEINFO

    nsDiskCacheDeviceInfo(nsDiskCacheDevice* device)
        :   mDevice(device)
    {
        NS_INIT_ISUPPORTS();
    }

    virtual ~nsDiskCacheDeviceInfo() {}
    
private:
    nsDiskCacheDevice* mDevice;
};

NS_IMPL_ISUPPORTS1(nsDiskCacheDeviceInfo, nsICacheDeviceInfo);

/* readonly attribute string description; */
NS_IMETHODIMP nsDiskCacheDeviceInfo::GetDescription(char ** aDescription)
{
    NS_ENSURE_ARG_POINTER(aDescription);
    *aDescription = nsCRT::strdup("Disk cache device");
    return *aDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute string usageReport; */
NS_IMETHODIMP nsDiskCacheDeviceInfo::GetUsageReport(char ** usageReport)
{
    NS_ENSURE_ARG_POINTER(usageReport);
    nsCString buffer;
    
    buffer.Assign("<table>\n");

    buffer.Append("<tr><td><b>Cache Directory:</b></td><td><tt> ");
    nsCOMPtr<nsILocalFile> cacheDir;
    char *                 path;
    mDevice->getCacheDirectory(getter_AddRefs(cacheDir)); 
    nsresult rv = cacheDir->GetPath(&path);
    if (NS_SUCCEEDED(rv)) {
        buffer.Append(path);
    } else {
        buffer.Append("directory unavailable");
    }
    buffer.Append("</tt></td></tr>");
    buffer.Append("<tr><td><b>Files:</b></td><td><tt> XXX</tt></td></tr>");
    buffer.Append("</table>");
    *usageReport = buffer.ToNewCString();
    if (!*usageReport) return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

/* readonly attribute unsigned long entryCount; */
NS_IMETHODIMP nsDiskCacheDeviceInfo::GetEntryCount(PRUint32 *aEntryCount)
{
    NS_ENSURE_ARG_POINTER(aEntryCount);
    *aEntryCount = mDevice->getEntryCount();
    return NS_OK;
}

/* readonly attribute unsigned long totalSize; */
NS_IMETHODIMP nsDiskCacheDeviceInfo::GetTotalSize(PRUint32 *aTotalSize)
{
    NS_ENSURE_ARG_POINTER(aTotalSize);
    *aTotalSize = mDevice->getCacheSize();
    return NS_OK;
}

/* readonly attribute unsigned long maximumSize; */
NS_IMETHODIMP nsDiskCacheDeviceInfo::GetMaximumSize(PRUint32 *aMaximumSize)
{
    NS_ENSURE_ARG_POINTER(aMaximumSize);
    *aMaximumSize = mDevice->getCacheCapacity();
    return NS_OK;
}


/******************************************************************************
 *  nsDiskCache
 *****************************************************************************/

/**
 *  nsDiskCache::Hash(const char * key)
 *
 *  This algorithm of this method implies nsDiskCacheRecords will be stored
 *  in a certain order on disk.  If the algorithm changes, existing cache
 *  map files may become invalid, and therefore the kCurrentVersion needs
 *  to be revised.
 */
PLDHashNumber
nsDiskCache::Hash(const char * key)
{
    PLDHashNumber h = 0;
    for (const PRUint8* s = (PRUint8*) key; *s != '\0'; ++s)
        h = (h >> (PL_DHASH_BITS - 4)) ^ (h << 4) ^ *s;
    return (h == 0 ? ULONG_MAX : h);
}


/******************************************************************************
 *  nsDiskCacheDevice
 *****************************************************************************/
static nsCOMPtr<nsIFileTransportService> gFileTransportService;

#ifdef XP_MAC
#pragma mark -
#pragma mark DISK CACHE DEVICE
#endif

nsDiskCacheDevice::nsDiskCacheDevice()
    :   mInitialized(PR_FALSE), mCacheCapacity(0), mCacheMap(nsnull)
{
}

nsDiskCacheDevice::~nsDiskCacheDevice()
{
    Shutdown();
    delete mCacheMap;
}


/**
 *  methods of nsCacheDevice
 */
nsresult
nsDiskCacheDevice::Init()
{
    nsresult rv;
    
    rv = mBindery.Init();
    if (NS_FAILED(rv)) return rv;

    // XXX examine preferences, and install observers to react to changes.
    rv = installObservers(this);
    if (NS_FAILED(rv)) return rv;
    
    // hold the file transport service to avoid excessive calls to the service manager.
    gFileTransportService = do_GetService("@mozilla.org/network/file-transport-service;1", &rv);
    if (NS_FAILED(rv)) return rv;

    // XXX are we sure we want to do this on startup?
    // delete "Cache.Trash" folder
    nsCOMPtr<nsIFile> cacheTrashDir;
    rv = GetCacheTrashDirectory(getter_AddRefs(cacheTrashDir));
    if (NS_FAILED(rv))  return rv;
    (void) cacheTrashDir->Delete(PR_TRUE);      // ignore errors, we tried...

    // Try opening cache map file.
    mCacheMap = new nsDiskCacheMap;
    if (!mCacheMap) return NS_ERROR_OUT_OF_MEMORY;

    rv = mCacheMap->Open(mCacheDirectory);
    if (NS_FAILED(rv)) {
        rv = InitializeCacheDirectory();        // retry one time
        if (NS_FAILED(rv)) return rv;
    }
    
    mInitialized = PR_TRUE;                     // record that initialization succeeded.
    return NS_OK;

error_exit:
    // XXX de-install observers?

    (void) mCacheMap->Close();
    gFileTransportService   = nsnull;

    return rv;
}


nsresult
nsDiskCacheDevice::Shutdown()
{
    if (mInitialized) {
        // check cache limits in case we need to evict.
        EvictDiskCacheEntries();

        // write out persistent information about the cache.
        (void) mCacheMap->Close();

        // no longer initialized.
        mInitialized = PR_FALSE;
    }

    // disconnect observers.
    removeObservers(this);
    
    // release the reference to the cached file transport service.
    gFileTransportService = nsnull;
    
    return NS_OK;
}


nsresult
nsDiskCacheDevice::Create(nsCacheDevice **result)
{
    nsDiskCacheDevice * device = new nsDiskCacheDevice();
    if (!device)  return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = device->Init();
    if (NS_FAILED(rv)) {
        delete device;
        device = nsnull;
    }
    *result = device;
    return rv;
}


const char *
nsDiskCacheDevice::GetDeviceID()
{
    return DISK_CACHE_DEVICE_ID;
}


/**
 *  FindEntry -
 *
 *      cases:  key not in disk cache, hash number free
 *              key not in disk cache, hash number used
 *              key in disk cache
 */
nsCacheEntry *
nsDiskCacheDevice::FindEntry(nsCString * key)
{
    nsCacheEntry *        entry    = nsnull;
    nsDiskCacheBindData * bindData = nsnull;

#if DEBUG  /*because we shouldn't be called for active entries */
    bindData = mBindery.GetEntry(key->get());
    NS_ASSERTION(!bindData, "### FindEntry() called for a bound entry.");
    bindData = nsnull;
#endif

    PLDHashNumber       hashNumber = nsDiskCache::Hash(key->get());
    nsDiskCacheRecord   record;
    nsresult            rv;
    
    // lookup hash number in cache map
    rv = mCacheMap->FindRecord(hashNumber, &record);
    if (NS_FAILED(rv))  return nsnull;  // XXX log error?
    
    nsDiskCacheEntry * diskEntry;
    rv = mCacheMap->ReadDiskCacheEntry(&record, &diskEntry);
    if (NS_FAILED(rv))  goto error_exit;
    
    entry = diskEntry->CreateCacheEntry(this);
    delete diskEntry;       // XXX best place?
    if (!entry)  {
        // XXX evict entry in case it's bad
        rv = NS_ERROR_UNEXPECTED;
        goto error_exit;
    }
    
    // XXX pass record to bindery
    bindData = mBindery.CreateBindDataForCacheEntry(entry);
    if (!bindData) {
        delete entry;
    }
    
    return entry;
    
error_exit:
    delete entry;
    return nsnull;
}


nsresult
nsDiskCacheDevice::DeactivateEntry(nsCacheEntry * entry)
{
    nsDiskCacheBindData * bindData = GetBindDataFromCacheEntry(entry);
    NS_ASSERTION(bindData, "DeactivateEntry: bindData == nsnull");
    if (!bindData)  return NS_ERROR_UNEXPECTED;

    if (entry->IsDoomed()) {
        // XXX delete data, entry, record from disk for entry
        mCacheMap->DecrementTotalSize(entry->DataSize());  // XXX what about meta-data?
    } else {
        // XXX save stuff to disk for entry
        nsDiskCacheEntry * diskEntry =  CreateDiskCacheEntry(bindData);
        if (!diskEntry)  return NS_ERROR_UNEXPECTED;        // XXX remove map record?

        nsresult rv = mCacheMap->WriteDiskCacheEntry(diskEntry, bindData);
        // XXX if (NS_FAILED(rv))  then what? 
    }

    // XXX extract bindData from collision detection stuff

    delete entry;   // which will release bindData
    return NS_OK;

#if 0    
    if (mBoundEntries.GetEntry(bindData->mRecord.HashNumber()) == bindData) {
        mBoundEntries.RemoveEntry(bindData);
    }
    // XXX else { what if the the entry we're deactivating is further down the list,
    // XXX        or the head } 
    
    if (!entry->IsDoomed()) {
        // commit any changes about this entry to disk.
        updateDiskCacheEntry(bindData);

        // XXX if this entry collided with other concurrently bound entries, then its
        // generation count will be non-zero. The other entries that came before it
        // will be linked to it and doomed. deletion of the entry can only be done
        // when all of the other doomed entries are deactivated, so that the final live entry
        // can have its generation number reset to zero.
        if (bindData->getGeneration() != 0)
            scavengeDiskCacheEntries(bindData);
        else
            delete entry;
    } else {
        // keep track of the cache total size.
        mCacheMap->DecrementTotalSize(entry->DataSize());  // XXX what about meta-data?

        // obliterate all knowledge of this entry on disk.
        deleteDiskCacheEntry(bindData);

        // XXX if this entry resides on a list, then there must have been a collision
        // during the entry's lifetime. use this deactivation as a trigger to scavenge
        // generation numbers, and reset the live entry's generation to zero.
        if (!PR_CLIST_IS_EMPTY(bindData)) {
            scavengeDiskCacheEntries(bindData);
        }
        
        // delete entry from memory.
        delete entry;
    }

    return NS_OK;
#endif
}


/**
 * BindEntry()
 *      no hash number collision -> no problem
 *      collision
 *          record not active -> evict, no problem
 *          record is active
 *              record is already doomed -> record shouldn't have been in map, no problem
 *              record is not doomed -> doom, and replace record in map
 *              
 *              walk matching hashnumber list to find lowest generation number
 *              take generation number from other (data/meta) location,
 *                  or walk active list
 */
nsresult
nsDiskCacheDevice::BindEntry(nsCacheEntry * newEntry)
{
    nsresult rv = NS_OK;
    // XXX check cache map or bindery first?

    // Make sure this entry has its associated nsDiskCacheBindData attached.
    // XXX pass new nsDiskCacheRecord to bindery (HashNumber, EvictionRank)
    nsDiskCacheBindData * newBindData = mBindery.CreateBindDataForCacheEntry(newEntry);
    NS_ASSERTION(newBindData, "nsDiskCacheDevice::BindEntry");
    if (!newBindData) return NS_ERROR_OUT_OF_MEMORY;
    
    
    return rv;

#if 0    
    // XXX check for cache collision. if an entry exists on disk that has the same
    // hash code as this newly bound entry, AND there is already a bound entry for
    // that key, we need to ask the cache service to doom that entry, since two
    // simultaneous entries that have the same hash code aren't allowed until
    // some sort of chaining mechanism is implemented.
    nsDiskCacheBindData* oldBindData = mBoundEntries.GetEntry(newEntry->Key()->get());
    if (oldBindData) {
        // XXX Hacky liveness test, remove when we've figured this all out.
        if (oldBindData->getRefCount() > 1) {
            // set the generation count on the newly bound entry,
            // so that files created will be unique and won't conflict
            // with the doomed entries that are still active.
            newBindData->setGeneration(oldBindData->getGeneration() + 1);
            PR_APPEND_LINK(newBindData, oldBindData);
            
            // XXX Whom do we tell about this impending doom?
            nsCacheEntry* oldEntry = oldBindData->getCacheEntry();
            // XXX Yes Virginia, a doomed entry can be bound.
            // NS_ASSERTION(!oldEntry->IsDoomed(), "a bound entry is doomed!");
            if (!oldEntry->IsDoomed())
                nsCacheService::GlobalInstance()->DoomEntry_Locked(oldEntry);
            else
                mBoundEntries.RemoveEntry(oldBindData);
        } else {
            // XXX somehow we didn't hear about the entry going away. Ask gordon.
            NS_NOTREACHED("bound disk cache entry with no corresponding cache entry.");
            mBoundEntries.RemoveEntry(oldBindData);
        }
    }

    rv = mBoundEntries.AddEntry(newBindData);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 dataSize = newEntry->DataSize();
    if (dataSize)
        OnDataSizeChange(newEntry, dataSize);

    // XXX Need to make this entry known to other entries?
    // this probably isn't needed while the entry is bound,
    // only when the entry is deactivated. this could be
    // the reason disk cache performance suffers.
    return updateDiskCacheEntry(newBindData);
#endif
}


void
nsDiskCacheDevice::DoomEntry(nsCacheEntry * entry)
{
    // so it can't be seen by FindEntry() ever again.
    nsDiskCacheBindData * bindData = GetBindDataFromCacheEntry(entry);
    NS_ASSERTION(bindData, "DoomEntry: bindData == nsnull");
    if (!bindData)  return;

//    mBindery.RemoveEntry(bindData);
    // XXX but if a subsequent BindEntry() is made for the same HashNumber,
    // XXX we need to know what generation number to give it.
    
    // XXX clear this entry out of the cache map.
    // this is done in deleteDiskCacheEntry().
}


nsresult
nsDiskCacheDevice::GetTransportForEntry(nsCacheEntry * entry,
                                        nsCacheAccessMode mode, 
                                        nsITransport ** result)
{
    NS_ENSURE_ARG_POINTER(entry);
    NS_ENSURE_ARG_POINTER(result);

    nsDiskCacheBindData * bindData = GetBindDataFromCacheEntry(entry);
    NS_ASSERTION(bindData, "GetTransportForEntry: bindData == nsnull");
    if (!bindData)  return NS_ERROR_UNEXPECTED;

#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
    nsCOMPtr<nsITransport>& transport = bindData->getTransport(mode);
    if (transport) {
        NS_ADDREF(*result = transport);
        return NS_OK;
    }
#endif
    // XXX check/set bindData->mRecord for separate or block file, sync w/mCacheMap

    // generate the name of the cache entry from the hash code of its key,
    // modulo the number of files we're willing to keep cached.
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheMap->GetFileForDiskCacheRecord(&bindData->mRecord,
                                                       nsDiskCache::kData,
                                                       getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    PRInt32 ioFlags = 0;
    switch (mode) {
    case nsICache::ACCESS_READ:
        ioFlags = PR_RDONLY;
        break;
    case nsICache::ACCESS_WRITE:
        ioFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
        break;
    case nsICache::ACCESS_READ_WRITE:
        ioFlags = PR_RDWR | PR_CREATE_FILE;
        break;
    }

    rv = gFileTransportService->CreateTransport(file, ioFlags, PR_IRUSR | PR_IWUSR, result);
    return rv;
}


nsresult
nsDiskCacheDevice::GetFileForEntry(nsCacheEntry *    entry,
                                   nsIFile **        result)
{
    nsDiskCacheBindData * bindData = GetBindDataFromCacheEntry(entry);
    NS_ASSERTION(bindData, "GetFileForEntry: bindData == nsnull");
    if (!bindData)  return NS_ERROR_UNEXPECTED;
    
    // XXX check/set bindData->mRecord for separate file, sync w/mCacheMap
    
    nsCOMPtr<nsIFile>  file;
    nsresult rv = mCacheMap->GetFileForDiskCacheRecord(&bindData->mRecord,
                                                       nsDiskCache::kData,
                                                       getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = file);
    return NS_OK;
}


/**
 * This routine will get called every time an open descriptor is written to.
 */
nsresult
nsDiskCacheDevice::OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize)
{
    mCacheMap->IncrementTotalSize(deltaSize);
    EvictDiskCacheEntries();
    
    return NS_OK;
}


/******************************************************************************
 *  EntryInfoVisitor
 *****************************************************************************/
class EntryInfoVisitor : public nsDiskCacheRecordVisitor
{
public:
    EntryInfoVisitor(nsDiskCacheDevice * device, nsICacheVisitor * visitor)
        : mDevice(device), mVisitor(visitor)
    {}
    
    virtual PRInt32  VisitRecord(nsDiskCacheRecord *  mapRecord)
    {
        // XXX do we have this record in memory?
        // read in the entry (metadata)
        nsDiskCacheEntry * diskEntry = (nsDiskCacheEntry *)new char[1];

        // create nsICacheEntryInfo
        nsDiskCacheEntryInfo * entryInfo = new nsDiskCacheEntryInfo(DISK_CACHE_DEVICE_ID, diskEntry);
        if (!entryInfo) {
            mResult = NS_ERROR_OUT_OF_MEMORY;
            return kStopVisitingRecords;
        }
        nsCOMPtr<nsICacheEntryInfo> ref(entryInfo);
        // XXX point entryInfo at metadata
        
        PRBool  keepGoing;
        nsresult rv = mVisitor->VisitEntry(DISK_CACHE_DEVICE_ID, entryInfo, &keepGoing);
        // XXX delete diskEntry;
        return keepGoing ? kVisitNextRecord : kStopVisitingRecords;
    }
 
private:
        nsDiskCacheDevice * mDevice;
        nsICacheVisitor *   mVisitor;
        nsresult            mResult;
};


nsresult
nsDiskCacheDevice::Visit(nsICacheVisitor * visitor)
{
    nsDiskCacheDeviceInfo* deviceInfo = new nsDiskCacheDeviceInfo(this);
    nsCOMPtr<nsICacheDeviceInfo> ref(deviceInfo);
    
    PRBool keepGoing;
    nsresult rv = visitor->VisitDevice(DISK_CACHE_DEVICE_ID, deviceInfo, &keepGoing);
    if (NS_FAILED(rv)) return rv;
    
    if (keepGoing) {
        EntryInfoVisitor  infoVisitor(this, visitor);
        return mCacheMap->VisitRecords(&infoVisitor);
//        return visitEntries(visitor);
    }

    return NS_OK;
}


/******************************************************************************
 *  EvictForClientID
 *****************************************************************************/
class EvictForClientID : public nsDiskCacheRecordVisitor
{
public:
    EvictForClientID(nsDiskCacheDevice * device, const char * clientID)
        : mDevice(device), mClientID(clientID)
    {}
    
    virtual PRInt32  VisitRecord(nsDiskCacheRecord *  mapRecord)
    {
        return kVisitNextRecord;
    }
 
private:
        nsDiskCacheDevice * mDevice;
        const char *        mClientID;
};


nsresult
nsDiskCacheDevice::EvictEntries(const char * clientID)
{
    nsresult rv;
    
    PRUint32 prefixLength  = (clientID ? nsCRT::strlen(clientID) : 0);
    PRInt32  newDataSize   = mCacheMap->TotalSize();
    PRInt32  newEntryCount = mCacheMap->EntryCount();

    EvictForClientID  evictor(this, clientID);
    rv = mCacheMap->VisitRecords(&evictor);
#if 0
    // XXX make sure meta data is up to date.
    rv = updateDiskCacheEntries();
    if (NS_FAILED(rv)) return rv;
    
    nsDiskCacheRecord records[kRecordsPerBucket];
    for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
        // XXX copy the i-th bucket from the cache map. GetBucket()
        // should probably be changed to do this.
        PRUint32 j, count = 0;
        const nsDiskCacheRecord* bucket = mCacheMap->GetBucket(i);
        for (j = 0; j < kRecordsPerBucket; ++j) {
            const nsDiskCacheRecord* record = bucket++;
            if (record->HashNumber() == 0)
                break;
            records[count++] = *record;
        }
        for (j = 0; j < count; ++j) {
            nsDiskCacheRecord* record = &records[j];

            // if the entry is currently in use, then doom it rather than evicting right here.
            nsDiskCacheBindData* bindData = mBoundEntries.GetEntry(record->HashNumber());
            if (bindData) {
                nsCacheService::GlobalInstance()->DoomEntry_Locked(bindData->getCacheEntry());
                continue;
            }
            
            // delete the metadata file.
            nsCOMPtr<nsIFile> metaFile;
            rv = getFileForHashNumber(record->HashNumber(), PR_TRUE, record->FileGeneration(), getter_AddRefs(metaFile));
            if (NS_SUCCEEDED(rv)) {
                if (clientID) {
                    // if filtering by clientID, make sure key prefix and clientID match.
                    // if anything fails, assume they don't by continuing with the loop.
                    nsCOMPtr<nsIInputStream> input;
                    rv = openInputStream(metaFile, getter_AddRefs(input));
                    if (NS_FAILED(rv)) continue;
                    
                    // read the metadata file.
                    MetaDataFile metaDataFile;
                    rv = metaDataFile.Read(input);
                    input->Close();
                    if (NS_FAILED(rv)) continue;
                    
                    if (nsCRT::strncmp(clientID, metaDataFile.mKey, prefixLength) != 0)
                        continue;
                }
                rv = metaFile->Delete(PR_FALSE);
            }
        
            PRUint32 dataSize = 0;

            // delete the data file
            nsCOMPtr<nsIFile> dataFile;
            rv = getFileForHashNumber(record->HashNumber(), PR_FALSE, record->FileGeneration(), getter_AddRefs(dataFile));
            if (NS_SUCCEEDED(rv)) {
                PRInt64 fileSize;
                rv = dataFile->GetFileSize(&fileSize);
                if (NS_SUCCEEDED(rv))
                    LL_L2I(dataSize, fileSize);
                rv = dataFile->Delete(PR_FALSE);
            }

            // remove from cache map.
            nsDiskCacheRecord* deletedRecord = mCacheMap->GetRecord(record->HashNumber());
            if (record->HashNumber() == deletedRecord->HashNumber() && record->FileGeneration() == deletedRecord->FileGeneration())
                mCacheMap->DeleteRecord(deletedRecord);

            // update the cache size/entry count.
            if (newDataSize >= dataSize) newDataSize -= dataSize;
            if (newEntryCount) --newEntryCount;
        }
    }
    
    if (clientID) {
        mCacheMap->DataSize() = newDataSize;
        mCacheMap->EntryCount() = newEntryCount;
    } else {
        mCacheMap->DataSize() = 0;
        mCacheMap->EntryCount() = 0;
    }
#endif
    
    return NS_OK;
}

/**
 *  private methods
 */
#ifdef XP_MAC
#pragma mark -
#pragma mark PRIVATE METHODS
#endif

nsresult
nsDiskCacheDevice::InitializeCacheDirectory()
{
    nsresult rv;
    
    // recursively delete the disk cache directory.
    rv = mCacheDirectory->Delete(PR_TRUE);
    if (NS_FAILED(rv)) {
        // try moving it aside
        
        // create "Cache.Trash" directory if necessary
        nsCOMPtr<nsIFile> cacheTrashDir;
        rv = GetCacheTrashDirectory(getter_AddRefs(cacheTrashDir));
        if (NS_FAILED(rv))  return rv;
        
        PRBool exists = PR_FALSE;
        rv = cacheTrashDir->Exists(&exists);
        if (NS_FAILED(rv))  return rv;
        
        if (!exists) {
            // create the "Cache.Trash" directory
            rv = cacheTrashDir->Create(nsIFile::DIRECTORY_TYPE,0777);
            if (NS_FAILED(rv))  return rv;
        }
        
        // create a directory with unique name to contain existing cache directory
        rv = cacheTrashDir->Append("Cache");
        if (NS_FAILED(rv))  return rv;
        rv = cacheTrashDir->CreateUnique(nsnull,nsIFile::DIRECTORY_TYPE, 0777); 
        if (NS_FAILED(rv))  return rv;
        
        // move existing cache directory into profileDir/Cache.Trash/CacheUnique
        nsCOMPtr<nsIFile> existingCacheDir;
        rv = mCacheDirectory->Clone(getter_AddRefs(existingCacheDir));
        if (NS_FAILED(rv))  return rv;
        rv = existingCacheDir->MoveTo(cacheTrashDir, nsnull);
        if (NS_FAILED(rv))  return rv;
    }
    
    rv = mCacheDirectory->Create(nsIFile::DIRECTORY_TYPE, 0777);
    if (NS_FAILED(rv)) return rv;
    
    // reopen the cache map     
    rv = mCacheMap->Open(mCacheDirectory);
    return rv;
}


nsresult
nsDiskCacheDevice::GetCacheTrashDirectory(nsIFile ** result)
{
    nsCOMPtr<nsIFile> cacheTrashDir;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(cacheTrashDir));
    if (NS_FAILED(rv))  return rv;
    rv = cacheTrashDir->SetLeafName("Cache.Trash");
    if (NS_FAILED(rv))  return rv;
    
    *result = cacheTrashDir.get();
    NS_ADDREF(*result);
    return rv;
}


nsresult
nsDiskCacheDevice::EvictDiskCacheEntries()
{
    // XXX mCacheMap->GetMeTheNextRecordToEvict();
    return NS_OK;
}


/**
 *  methods for prefs
 */
#ifdef XP_MAC
#pragma mark -
#pragma mark PREF METHODS
#endif
void nsDiskCacheDevice::setPrefsObserver(nsIObserver* observer)
{
    mPrefsObserver = observer;
}

void nsDiskCacheDevice::getPrefsObserver(nsIObserver** result)
{
    NS_IF_ADDREF(*result = mPrefsObserver);
}

void nsDiskCacheDevice::setCacheDirectory(nsILocalFile* cacheDirectory)
{
    mCacheDirectory = cacheDirectory;
}


void
nsDiskCacheDevice::getCacheDirectory(nsILocalFile ** result)
{
    *result = mCacheDirectory;
    NS_IF_ADDREF(*result);
}


void nsDiskCacheDevice::setCacheCapacity(PRUint32 capacity)
{
    mCacheCapacity = capacity;
    if (mInitialized) {
        // start evicting entries if the new size is smaller!
        // XXX need to enter cache service lock here!
        EvictDiskCacheEntries();
    }
}

PRUint32 nsDiskCacheDevice::getCacheCapacity()
{
    return mCacheCapacity;
}

PRUint32 nsDiskCacheDevice::getCacheSize()
{
    return mCacheMap->TotalSize();
}

PRUint32 nsDiskCacheDevice::getEntryCount()
{
    return mCacheMap->EntryCount();
}



/******************************************************************************
 *  Old Disk Cache Code
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark OLD DISK CACHE CODE
#endif

 #if 0   
// called from BindEntry(), FindEntry(nsCString * key)->readDiskCacheEntry
static nsDiskCacheBindData*
ensureDiskCacheBindData(nsCacheEntry * entry)
{
    nsCOMPtr<nsISupports> data;
    nsresult rv = entry->GetData(getter_AddRefs(data));
    if (NS_SUCCEEDED(rv) && !data) {
        nsDiskCacheBindData* bindData = new nsDiskCacheBindData(entry);
        data = bindData;
        if (NS_SUCCEEDED(rv) && data)
            entry->SetData(data.get());
    }
    return (nsDiskCacheBindData*) (nsISupports*) data.get();
}


nsresult nsDiskCacheDevice::visitEntries(nsICacheVisitor * visitor)
{
    nsresult rv = NS_OK;
    // XXX make sure meta data is up to date.
    rv = updateDiskCacheEntries();
    if (NS_FAILED(rv)) return rv;

    nsDiskCacheEntryInfo* entryInfo = new nsDiskCacheEntryInfo(DISK_CACHE_DEVICE_ID);
    if (!entryInfo) return NS_ERROR_OUT_OF_MEMORY;
    nsCOMPtr<nsICacheEntryInfo> ref(entryInfo);
    
    
    for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
        nsDiskCacheRecord* bucket = mCacheMap->GetBucket(i);
        for (PRUint32 j = 0; j < kRecordsPerBucket; ++j) {
            nsDiskCacheRecord* record = bucket++;
            if (record->HashNumber() == 0)
                break;
            nsCOMPtr<nsIFile> file;
            rv = getFileForHashNumber(record->HashNumber(), PR_TRUE,
                                      record->FileGeneration(), getter_AddRefs(file));
            if (NS_FAILED(rv)) continue;
            
            nsCOMPtr<nsIInputStream> input;
            rv = openInputStream(file, getter_AddRefs(input));
            if (NS_FAILED(rv)) {
                // delete non-existent record.
                mCacheMap->DeleteRecord(record);
                --bucket;
                continue;
            }
            
            // read the metadata file.
            rv = entryInfo->Read(input);
            input->Close();
            if (NS_FAILED(rv)) break;
            
            // tell the visitor about this entry.
            PRBool keepGoing;
            rv = visitor->VisitEntry(DISK_CACHE_DEVICE_ID, entryInfo, &keepGoing);
            if (NS_FAILED(rv)) return rv;
            if (!keepGoing) break;
        }
    }
    return NS_OK;
}


class UpdateEntryVisitor : public nsDiskCacheHashTable::Visitor {
    nsDiskCacheDevice* mDevice;
public:
    UpdateEntryVisitor(nsDiskCacheDevice * device) : mDevice(device) {}
    
    virtual PRBool VisitEntry(nsDiskCacheBindData * bindData)
    {
        mDevice->updateDiskCacheEntry(bindData);
        return PR_TRUE;
    }
};

nsresult nsDiskCacheDevice::updateDiskCacheEntries()
{
    UpdateEntryVisitor visitor(this);
    mBoundEntries.VisitEntries(&visitor);
    return NS_OK;
}

nsresult nsDiskCacheDevice::updateDiskCacheEntry(nsDiskCacheBindData* bindData)
{
    nsresult rv;
    nsCacheEntry* entry = bindData->getCacheEntry();
    if (entry->IsMetaDataDirty() || entry->IsEntryDirty() || entry->IsDataDirty()) {
        // make sure this disk entry is known to the cache map.
        rv = updateCacheMap(bindData);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIFile> file;
        rv = getFileForDiskCacheEntry(bindData, PR_TRUE,
                                      getter_AddRefs(file));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIOutputStream> output;
        rv = openOutputStream(file, getter_AddRefs(output));
        if (NS_FAILED(rv)) return rv;
        
        // write the metadata to the file.
        MetaDataFile metaDataFile(entry);
        rv = metaDataFile.Init(entry);
        if (NS_FAILED(rv)) return rv;
        rv = metaDataFile.Write(output);
        output->Close();
        if (NS_FAILED(rv)) return rv;
        
        // mark the disk entry as being consistent with meta data file.
        entry->MarkMetaDataClean();
        entry->MarkEntryClean();
        entry->MarkDataClean();
    }
    return NS_OK;
}


nsresult nsDiskCacheDevice::readDiskCacheEntry(const char * key, nsDiskCacheBindData ** result)
{
    // result should be nsull on cache miss.
    *result = nsnull;

    // up front, check to see if the file we are looking for exists.
    nsCOMPtr<nsIFile> file;
    nsresult rv = getFileForKey(key, PR_TRUE, 0, getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;

    // XXX getFileForKey() is the wrong interface, because entry may be in block file

    nsCOMPtr<nsIInputStream> input;
    rv = openInputStream(file, getter_AddRefs(input));
    if (NS_FAILED(rv)) return rv;
    
    // XXX metaDataFile assumes it can read from the beginning of the stream
    
    // read the metadata file.
    MetaDataFile metaDataFile;
    rv = metaDataFile.Read(input);
    input->Close();
    if (NS_FAILED(rv)) return rv;
        
    // Ensure that the keys match.
    if (nsCRT::strcmp(key, metaDataFile.mKey) != 0) return NS_ERROR_NOT_AVAILABLE;
    
    nsCacheEntry* entry = nsnull;
    rv = nsCacheEntry::Create(key, nsICache::STREAM_BASED, nsICache::STORE_ON_DISK, this, &entry);
    if (NS_FAILED(rv)) return rv;

    // initialize the entry.
    entry->SetFetchCount(metaDataFile.mFetchCount);
    entry->SetLastFetched(metaDataFile.mLastFetched);
    entry->SetLastModified(metaDataFile.mLastModified);
    entry->SetExpirationTime(metaDataFile.mExpirationTime);
    entry->SetDataSize(metaDataFile.mDataSize);
    
    // restore the metadata.
    if (metaDataFile.mMetaDataSize) {
        rv = entry->UnflattenMetaData(metaDataFile.mMetaData, metaDataFile.mMetaDataSize);
        if (NS_FAILED(rv)) goto error;
    }
    
    // celebrate!
    *result = ensureDiskCacheBindData(entry);
    if (!*result) goto error;
    return NS_OK;

error:
    // XXX  oh, auto_ptr<> would be nice right about now.
    delete entry;
    return NS_ERROR_NOT_AVAILABLE;
}


/**
 *  DeleteDiskCacheEntry - only called from DeactivateEntry on doomed entries
 */
nsresult nsDiskCacheDevice::deleteDiskCacheEntry(nsDiskCacheBindData * bindData)
{
    nsresult rv;
    
    // delete the metadata
    if (bindData->record.MetaLocation() == 0) {
        nsCOMPtr<nsIFile> metaFile;
        rv = getFileForDiskCacheEntry(bindData->record.MetaLocation(), kMetaData,
                                      getter_AddRefs(metaFile));
        if (NS_SUCCEEDED(rv)) {
            rv = metaFile->Delete(PR_FALSE);
            // NS_ASSERTION(NS_SUCCEEDED(rv), "nsDiskCacheDevice::deleteDiskCacheEntry");
        }
    } else {
        // XXX delete blocks in block file
    }
    
    // delete the data
    nsCOMPtr<nsIFile> dataFile;
    rv = getFileForDiskCacheEntry(bindData, PR_FALSE,
                                  getter_AddRefs(dataFile));
    if (NS_SUCCEEDED(rv)) {
        rv = dataFile->Delete(PR_FALSE);
        // NS_ASSERTION(NS_SUCCEEDED(rv), "nsDiskCacheDevice::deleteDiskCacheEntry");
    }
    
    // delete disk cache record
    nsDiskCacheRecord* record = mCacheMap->GetRecord(bindData->getHashNumber());
    if (record->HashNumber() == bindData->getHashNumber() && record->FileGeneration() == bindData->getGeneration())
        mCacheMap->DeleteRecord(record);

    return NS_OK;
}


nsresult nsDiskCacheDevice::getTransportForFile(nsIFile* file, nsCacheAccessMode mode, nsITransport ** result)
{
    PRInt32 ioFlags = 0;
    switch (mode) {
    case nsICache::ACCESS_READ:
        ioFlags = PR_RDONLY;
        break;
    case nsICache::ACCESS_WRITE:
        ioFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
        break;
    case nsICache::ACCESS_READ_WRITE:
        ioFlags = PR_RDWR | PR_CREATE_FILE;
        break;
    }
    return gFileTransportService->CreateTransport(file, ioFlags, PR_IRUSR | PR_IWUSR, result);
}


nsresult nsDiskCacheDevice::getFileForHashNumber(PLDHashNumber hashNumber, PRBool meta, PRUint32 generation, nsIFile ** result)
{
    if (mCacheDirectory) {
        nsCOMPtr<nsIFile> entryFile;
        nsresult rv = mCacheDirectory->Clone(getter_AddRefs(entryFile));
    	if (NS_FAILED(rv))
    		return rv;
        // generate the hash code for this entry, and use that as a file name.
        char name[32];
        ::sprintf(name, "%08X%c%02X", hashNumber, (meta ? 'm' : 'd'), generation);
        entryFile->Append(name);
        NS_ADDREF(*result = entryFile);
        return NS_OK;
    }
    return NS_ERROR_NOT_AVAILABLE;
}
nsresult nsDiskCacheDevice::getFileForKey(const char* key, PRBool meta,
                                          PRUint32 generation, nsIFile ** result)
{
    PLDHashNumber hash = nsDiskCacheBindData::Hash(key);
    return getFileForHashNumber(hash, meta, generation, result);
}


nsresult nsDiskCacheDevice::getFileForDiskCacheEntry(nsDiskCacheBindData* bindData,
                                                     PRBool              meta,
                                                     nsIFile **          result)
{
    return getFileForHashNumber(bindData->getHashNumber(), meta, bindData->getGeneration(), result);
}

nsresult nsDiskCacheDevice::scavengeDiskCacheEntries(nsDiskCacheBindData * bindData)
{
    nsresult rv;
    
    // count the number of doomed entries still in the list, not
    // including the passed-in entry. if this number is zero, then
    // the liveEntry, if inactive, can have its generation reset
    // to zero.
    PRUint32 doomedEntryCount = 0;
    nsDiskCacheBindData * youngestBindData = bindData;
    nsCacheEntry * youngestEntry = bindData->getCacheEntry();
    nsDiskCacheBindData * nextBindData = NS_STATIC_CAST(nsDiskCacheBindData*, PR_NEXT_LINK(bindData));
    while (nextBindData != bindData) {
        nsCacheEntry* nextEntry = nextBindData->getCacheEntry();
        if (nextEntry->IsDoomed()) {
            ++doomedEntryCount;
        } else if (nextBindData->getGeneration() > youngestBindData->getGeneration()) {
            youngestEntry = nextEntry;
            youngestBindData = nextBindData;
        }
        nextBindData = NS_STATIC_CAST(nsDiskCacheBindData*, PR_NEXT_LINK(nextBindData));
    }
    
    if (doomedEntryCount == 0 && !youngestEntry->IsDoomed() && !youngestEntry->IsActive()) {
        PRUint32 generation = youngestBindData->getGeneration();
        // XXX reset generation number.
        const char* key = youngestEntry->Key()->get();
        nsCOMPtr<nsIFile> oldFile, newFile;
        nsXPIDLCString newName;

        // rename metadata file.
        rv = getFileForKey(key, PR_TRUE, generation, getter_AddRefs(oldFile));
        if (NS_FAILED(rv)) return rv;
        rv = getFileForKey(key, PR_TRUE, 0, getter_AddRefs(newFile));
        rv = newFile->GetLeafName(getter_Copies(newName));
        if (NS_FAILED(rv)) return rv;
        rv = oldFile->MoveTo(nsnull, newName);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsDiskCacheDevice::scavengeDiskCacheEntries");

        // rename data file.
        rv = getFileForKey(key, PR_FALSE, generation, getter_AddRefs(oldFile));
        if (NS_FAILED(rv)) return rv;
        rv = getFileForKey(key, PR_FALSE, 0, getter_AddRefs(newFile));
        rv = newFile->GetLeafName(getter_Copies(newName));
        if (NS_FAILED(rv)) return rv;
        rv = oldFile->MoveTo(nsnull, newName);
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsDiskCacheDevice::scavengeDiskCacheEntries");

        // delete the youngestEntry, otherwise nobody else will.
        delete youngestEntry;
    }
    
    return NS_OK;
}



PR_STATIC_CALLBACK(int)
compareRecords(const void* e1, const void* e2, void* /*unused*/)
{
    const nsDiskCacheRecord* r1 = (const nsDiskCacheRecord*) e1;
    const nsDiskCacheRecord* r2 = (const nsDiskCacheRecord*) e2;
    return (r1->EvictionRank() - r2->EvictionRank());
}

nsresult nsDiskCacheDevice::evictDiskCacheEntries()
{
    nsresult rv;

    if (mCacheMap->TotalSize() < mCacheCapacity) return NS_OK;

    // XXX make sure meta data is up to date.
    rv = updateDiskCacheEntries();
    if (NS_FAILED(rv)) return rv;
    
    // 1. gather all records into an array, sorted by eviction rank. keep deleting them until we recover enough space.
    PRUint32 count = 0;
    nsDiskCacheRecord* sortedRecords = new nsDiskCacheRecord[mCacheMap->EntryCount()];
    if (sortedRecords) {
        for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
            nsDiskCacheRecord* bucket = mCacheMap->GetBucket(i);
            for (PRUint32 j = 0; j < kRecordsPerBucket; ++j) {
                nsDiskCacheRecord* record = bucket++;
                if (record->HashNumber() == 0)
                    break;
                NS_ASSERTION(count < mCacheMap->EntryCount(), "EntryCount is wrong");
                sortedRecords[count++] = *record;
            }
        }
        NS_QuickSort((void*)sortedRecords, count, sizeof(nsDiskCacheRecord), compareRecords, nsnull);
    } else {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    // these are sorted by eviction rank. lower eviction ranks are more eligible for eviction.
    for (PRUint32 i = 0; i < count; ++i) {
        nsDiskCacheRecord* record = &sortedRecords[i];
            
        // XXX if this entry is currently active, then leave it alone,
        // as it is likely to be modified very soon.
        nsDiskCacheBindData* bindData = mBoundEntries.GetEntry(record->HashNumber());
        if (bindData) continue;
        
        // delete the metadata file.
        nsCOMPtr<nsIFile> metaFile;
        rv = getFileForHashNumber(record->HashNumber(), PR_TRUE, 0, getter_AddRefs(metaFile));
        if (NS_SUCCEEDED(rv)) {
            rv = metaFile->Delete(PR_FALSE);
        }
        
        PRUint32 dataSize = 0;

        // delete the data file
        nsCOMPtr<nsIFile> dataFile;
        rv = getFileForHashNumber(record->HashNumber(), PR_FALSE, 0, getter_AddRefs(dataFile));
        if (NS_SUCCEEDED(rv)) {
            PRInt64 fileSize;
            rv = dataFile->GetFileSize(&fileSize);
            if (NS_SUCCEEDED(rv))
                LL_L2I(dataSize, fileSize);
            rv = dataFile->Delete(PR_FALSE);
        }

        // remove from cache map.
        nsDiskCacheRecord* deletedRecord = mCacheMap->GetRecord(record->HashNumber());
        if (record->HashNumber() == deletedRecord->HashNumber() && record->FileGeneration() == deletedRecord->FileGeneration())
            mCacheMap->DeleteRecord(deletedRecord);

        // update the cache size.
        if ((mCacheMap->DataSize() -= dataSize) <= mCacheCapacity)
            break;
    }
    
    delete[] sortedRecords;
    
    return NS_OK;
}


/**
 *  updateCacheMap : called by updateDiskCacheEntry
 */
nsresult nsDiskCacheDevice::updateCacheMap(nsDiskCacheBindData * bindData)
{
    nsresult rv = NS_OK;
    // get a record from the cache map, and use the fetch time for eviction ranking.
    PRBool commitBucket = PR_TRUE;
    nsDiskCacheRecord* record = mCacheMap->GetRecord(bindData->getHashNumber());
    if (record->HashNumber() != bindData->getHashNumber()) {
        if (record->HashNumber() != 0) {
            // eviction of eldest entry in this bucket.
            evictDiskCacheRecord(record);
        } else {
            mCacheMap->EntryCount() += 1;
        }
        // newly bound record. fill in the blanks.
        record->SetHashNumber(bindData->getHashNumber());
        record->SetEvictionRank(0);
        record->SetFileGeneration(bindData->getGeneration());
    } else if (record->FileGeneration() != bindData->getGeneration()) {
        // a collision has occurred
        record->SetHashNumber(bindData->getHashNumber());
        record->SetEvictionRank(0);
        record->SetFileGeneration(bindData->getGeneration());
    } else {
        commitBucket = PR_FALSE;
        record->SetEvictionRank(record->EvictionRank() + 1);
    }
    
    // make sure the on-disk cache map is kept up to date.
    if (commitBucket) {
        if (!mCacheStream) {
            rv = openCacheMap();
            if (NS_FAILED(rv)) return rv;
        }
        if (!mCacheMap->IsDirty()) {
            mCacheMap->IsDirty() = PR_TRUE;
            rv = mCacheMap->WriteHeader(mCacheStream);
        }
        rv = mCacheMap->WriteBucket(mCacheStream, mCacheMap->GetBucketIndex(record));
    }
    return rv;
}

/**
 *  evictDiskCacheRecord
 *
 *  This evicts the disk cache entry that corresponds to the given record.
 *  If the entry happens to be bound, it must be doomed, otherwise, it can
 *  be eagerly removed from disk.
 */
nsresult nsDiskCacheDevice::evictDiskCacheRecord(nsDiskCacheRecord * record)
{
    nsresult rv;
    
    // if the entry is currently in use, then doom it rather than evicting right here.
    nsDiskCacheBindData* bindData = mBoundEntries.GetEntry(record->HashNumber());
    if (bindData)
        return nsCacheService::GlobalInstance()->DoomEntry_Locked(bindData->getCacheEntry());

    // delete the metadata file.
    nsCOMPtr<nsIFile> metaFile;
    rv = getFileForHashNumber(record->HashNumber(), PR_TRUE,
                              record->FileGeneration(), getter_AddRefs(metaFile));
    if (NS_SUCCEEDED(rv))
        rv = metaFile->Delete(PR_FALSE);
    
    // delete the data file
    nsCOMPtr<nsIFile> dataFile;
    rv = getFileForHashNumber(record->HashNumber(), PR_FALSE,
                              record->FileGeneration(), getter_AddRefs(dataFile));
    if (NS_SUCCEEDED(rv))
        rv = dataFile->Delete(PR_FALSE);
    
    return rv;
}
#endif   
