/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsNetDiskCache.h"
#include "nscore.h"

#include "plstr.h"
#include "prprf.h"
#include "prtypes.h"
#include "prio.h"
#include "prsystem.h" // Directory Seperator
#include "plhash.h"
#include "prclist.h"
#include "prmem.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsAutoLock.h"

#include "nsIPref.h"
#include "mcom_db.h"
#include "nsDBEnumerator.h"

#include "nsDiskCacheRecord.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID) ;
//static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID) ;
static NS_DEFINE_CID(kDBAccessorCID, NS_DBACCESSOR_CID) ;

const static int MAX_FILENAME_LEN = 512 ;
const static int MAX_OBJECTS_IN_RECENTLY_USED_LIST = 20; // change later TODO.

static const PRUint32 DISK_CACHE_SIZE_DEFAULT = 5*1024*1024 ; // 5MB
static const char * const DISK_CACHE_PREF  = "browser.cache.disk_cache_size";
static const char * const CACHE_DIR_PREF   = "browser.cache.directory";

//File static list of recently used cache objects
static PRCList g_RecentlyUsedList;

struct recentlyUsedObject {
  PRCList link ;
  nsINetDataCacheRecord* cacheObject ;
} ;

/* Find pointer to recentlyUsedObject struct 
 * from the list linkaged embedded in it 
 */
#define OBJECT_PTR(_link) \
  ((recentlyUsedObject*) ((char*) (_link) - offsetof(recentlyUsedObject,link)))

class nsDiskCacheRecord ;

nsNetDiskCache::nsNetDiskCache() :
    m_Enabled(PR_TRUE) ,
    m_ReadOnly(PR_FALSE) ,
    m_NumEntries(0) ,
    m_pNextCache(0) ,
    m_pDiskCacheFolder(0) ,
    m_DBFileName("cache.db") ,
    m_DB(0) , 
    m_ListCount(0) 
{
  nsAutoString description("Disk Cache") ;
  m_Description = description.ToNewUnicode() ;

  // set it to INF for now 
  m_MaxEntries = (PRUint32)-1 ;

  Init() ;

  m_Lock = PR_NewLock() ;
  if(!m_Lock)
    NS_ERROR("Error: Failed to create a lock on nsNetDiskCache.") ;

  PR_INIT_CLIST(&g_RecentlyUsedList);

  NS_INIT_REFCNT();

}

nsNetDiskCache::~nsNetDiskCache()
{
  /* Clean up the recently used list */
  recentlyUsedObject* obj;

  int i = 0 ;
  while (!PR_CLIST_IS_EMPTY(&g_RecentlyUsedList))
  {
    i++ ;
    obj = (recentlyUsedObject*) PR_LIST_HEAD(&g_RecentlyUsedList);
    if (obj->cacheObject)
      NS_RELEASE(obj->cacheObject) ;
    PR_REMOVE_LINK(&obj->link);
  }

  printf(" %d entries in RUlist \n", i) ;

  PR_ASSERT(PR_CLIST_IS_EMPTY(&g_RecentlyUsedList));

  NS_IF_RELEASE(m_DB) ;
  
  /* 
  // these are not necessary according to scc
  if(m_pNextCache)
    m_pNextCache = null_nsCOMPtr() ;
  if(m_pDiskCacheFolder)
    m_pDiskCacheFolder = null_nsCOMPtr() ;
  if(m_DBFile)
    m_DBFile = null_nsCOMPtr() ;
    */

  if(m_Lock)
    PR_DestroyLock(m_Lock);
}

NS_IMETHODIMP
nsNetDiskCache::Init(void) 
{
  nsresult rv ;
  NS_WITH_SERVICE(nsIPref, pref, kPrefCID, &rv) ;
  if (NS_FAILED(rv)) 
    NS_ERROR("Failed to get globle preference!\n") ;

  rv = NS_NewFileSpec(getter_AddRefs(m_pDiskCacheFolder));

  if (!m_pDiskCacheFolder || NS_FAILED(rv)) {
    NS_ERROR("ERROR: Could not make a file spec.\n") ;
    return NS_ERROR_FAILURE ;
  }

  char* tempPref = 0 ;
  if(pref) {
//    pref->StartUp() ;
//    pref->ReadUserPrefs() ;

    PRInt32 nTemp = 0 ;

    if(NS_SUCCEEDED(pref->GetIntPref(DISK_CACHE_PREF, &nTemp))) {
      printf("cache size is %d\n", nTemp) ;
      m_Capacity = 1024*nTemp ;
    } else {
      m_Capacity = DISK_CACHE_SIZE_DEFAULT ;
      printf("using default capacity value, %d bytes\n", m_Capacity) ;
    }

    rv = pref->CopyCharPref(CACHE_DIR_PREF, &tempPref) ;
    if (NS_SUCCEEDED(rv)) {
      printf("cache dir is %s\n", tempPref) ;
      m_pDiskCacheFolder->SetUnixStyleFilePath(tempPref) ;
	  PR_Free(tempPref) ;
    } else {
      m_pDiskCacheFolder->SetUnixStyleFilePath("/tmp") ;
      printf("using default folder, /tmp\n") ;
    }
  }
  else {
    // temp hack for now. change later for other platform
    m_Capacity = DISK_CACHE_SIZE_DEFAULT ;
    m_pDiskCacheFolder->SetUnixStyleFilePath("/tmp") ;
  }

//  pref->ShutDown() ;

  InitDB() ;

  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::InitDB(void)
{
  // create hashed cache sub directories
  nsresult rv ; 

  nsCOMPtr<nsIFileSpec> cacheSubDir;
  rv = NS_NewFileSpec(getter_AddRefs(cacheSubDir));
  for (int i=0; i < 32; i++) {
    cacheSubDir->FromFileSpec(m_pDiskCacheFolder) ;

    char dirName[3];
    PR_snprintf (dirName, 3, "%0.2x", i);
    cacheSubDir->AppendRelativeUnixPath (dirName) ;
    CreateDir(cacheSubDir);
  }

  NS_NewFileSpec(getter_AddRefs(m_DBFile)) ;

  m_DBFile->FromFileSpec(m_pDiskCacheFolder) ;
  m_DBFile->AppendRelativeUnixPath("cache.db") ;

  if(!m_DB) {
    NS_WITH_SERVICE(nsIDBAccessor, tempInst, kDBAccessorCID, &rv) ;
    if(NS_SUCCEEDED(rv)) {
      m_DB = tempInst ;
      NS_ADDREF(m_DB) ; // addref to it since tempInst will go out of scope
    } else 
      return NS_ERROR_FAILURE ;
  }

//  CreateDir(m_pDiskCacheFolder) ;

  rv = m_DB->Init(m_DBFile) ;
  if(NS_FAILED(rv))
    return NS_ERROR_FAILURE ;

  // count num of entries in db
  nsISimpleEnumerator* dbEnumerator = new nsDBEnumerator(m_DB) ;
  if(dbEnumerator)
    NS_ADDREF(dbEnumerator) ;
  else
    return NS_ERROR_FAILURE ;

  m_NumEntries = 0 ;

  PRBool more = PR_FALSE ;
  do {
    dbEnumerator->HasMoreElements(&more) ;
    if(more) 
      m_NumEntries++ ;
  } while (more) ;

  NS_IF_RELEASE(dbEnumerator) ;

  return NS_OK ;
}

//////////////////////////////////////////////////////////////////////////
// nsISupports methods
NS_IMETHODIMP
nsNetDiskCache::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr, "no instance pointer");
  if(aIID.Equals(NS_GET_IID(nsINetDataDiskCache)) ||
     aIID.Equals(NS_GET_IID(nsINetDataCache)) ||
     aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = NS_STATIC_CAST(nsINetDataDiskCache*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else
    return NS_NOINTERFACE ;
}

NS_IMPL_ADDREF(nsNetDiskCache) ;
NS_IMPL_RELEASE(nsNetDiskCache) ;

///////////////////////////////////////////////////////////////////////////
// nsINetDataCache Method 

NS_IMETHODIMP
nsNetDiskCache::GetDescription(PRUnichar* *aDescription) 
{
  *aDescription = m_Description ;
  return NS_OK ;
}

/* Don't search RUL, and don't alloc mem for nsICachedNetData. 
 * RecordID is generated using the same scheme in nsCacheDiskData,
 * see GetCachedNetData() for detail.
 */
NS_IMETHODIMP
nsNetDiskCache::Contains(const char* key, PRUint32 length, PRBool *_retval) 
{
  *_retval = PR_FALSE ;

  if(!m_DB) {
    nsresult rv ;
    NS_WITH_SERVICE(nsIDBAccessor, pDB, kDBAccessorCID, &rv) ;
    if(NS_FAILED(rv)) {
      NS_ERROR("Error: failed to access DB\n") ;
      return NS_ERROR_FAILURE ;
    } else {
      m_DB = pDB ;
      NS_ADDREF(m_DB) ; // addref to the service 
    }
  }

  PRInt32 id = 0 ;
  m_DB->GenID(key, length, &id) ;

  void* info = 0 ;
  PRUint32 info_size = 0 ;

  nsresult rv = m_DB->Get(id, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) 
    *_retval = PR_TRUE ;
        
  return NS_OK ;
}

/* regardless if it's cached or not, a copy of nsNetDiskCache would 
 * always be returned. so release it appropriately. 
 * if mem alloced, updata m_NumEntries also.
 * for now, the new nsCachedNetData is not written into db yet since
 * we have nothing to write to. But we could use (seq)(... R_CURSOR) 
 * to create an empty record in DB too if we find it's better that way. 
 */
NS_IMETHODIMP
nsNetDiskCache::GetCachedNetData(const char* key, PRUint32 length, nsINetDataCacheRecord **_retval) 
{
  nsresult rv = 0 ;
  if (!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;

  if(!m_DB) {
    NS_WITH_SERVICE(nsIDBAccessor, pDB, kDBAccessorCID, &rv) ;
    if(NS_FAILED(rv)) {
      NS_ERROR("Error: failed to access DB\n") ;
      return NS_ERROR_FAILURE ;
    } else {
      m_DB = pDB ;
      NS_ADDREF(m_DB) ; // addref to the service
    }
  }

  /* Check among recently used objects */
  PRInt32 id = 0 ;
  m_DB->GenID(key, length, &id) ;

  rv = SearchRUList(id, _retval) ;
  if (NS_SUCCEEDED(rv) && *_retval) {
//    printf(" found in RUList\n") ;
    return NS_OK ;
  }

  /* check the database */

  // construct an empty record
  nsINetDataCacheRecord* newRecord = new nsDiskCacheRecord(key, length, m_DB) ;
  if(newRecord) {
    NS_ADDREF(newRecord) ; // addref for RUList

    AddToRUList(newRecord) ;
    *_retval = newRecord ;
    NS_ADDREF(newRecord) ; // addref for _retval 
  }
  else 
    return NS_ERROR_OUT_OF_MEMORY ;

  void* info = 0 ;
  PRUint32 info_size = 0 ;

  rv = m_DB->Get(id, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) {

    nsDiskCacheRecord* p_newRecord ;
    p_newRecord = NS_STATIC_CAST(nsDiskCacheRecord*, newRecord) ;

    nsresult r1 ;
    r1 = p_newRecord->RetrieveInfo(info, info_size) ;
    if(NS_SUCCEEDED(rv)) 
      return NS_OK ;
    else 
      return r1;
    
  } else if (NS_SUCCEEDED(rv) && !info) {
    // this is a new record. 
    m_NumEntries ++ ;
    return NS_OK ;
  } else 
    return rv ;
}

/* get an nsICachedNetData, mem needs to be de-alloced if not found. */
NS_IMETHODIMP
nsNetDiskCache::GetCachedNetDataByID(PRInt32 RecordID, nsINetDataCacheRecord **_retval) 
{
  if (!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;
  
  nsresult rv ;
  if(!m_DB) {
    NS_WITH_SERVICE(nsIDBAccessor, pDB, kDBAccessorCID, &rv) ;
    if(NS_FAILED(rv)) {
      NS_ERROR("Error: failed to access DB\n") ;
      return NS_ERROR_FAILURE ;
    } else {
      m_DB = pDB ;
      NS_ADDREF(m_DB) ;
    }
  }

  /* Check among recently used objects */
  rv = SearchRUList(RecordID, _retval) ;
  if (NS_SUCCEEDED(rv) && *_retval) 
    return NS_OK ;

  void* info = 0 ;
  PRUint32 info_size = 0 ;

  rv = m_DB->Get(RecordID, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) {

    // construct an empty record if only found in db
    nsINetDataCacheRecord* newRecord = new nsDiskCacheRecord(nsnull, 0, m_DB) ;
    if(newRecord)
      NS_ADDREF(newRecord) ; // addref for _retval
    else 
      return NS_ERROR_OUT_OF_MEMORY ;

    nsDiskCacheRecord* p_newRecord ;
    p_newRecord = NS_STATIC_CAST(nsDiskCacheRecord*, newRecord) ;

    rv = p_newRecord->RetrieveInfo(info, info_size) ;
    if(NS_SUCCEEDED(rv)) {
      NS_ADDREF(newRecord) ; // addref for RUList
      AddToRUList(newRecord) ;
      *_retval = newRecord ;
      return NS_OK ;
    } 
    else {
      // bad record, I guess
      NS_RELEASE(newRecord) ; // release if bad things happen
      return rv ;
    }
  } else {
    NS_ERROR("Error: RecordID not in DB\n") ;
    return rv ;
  }
}

NS_IMETHODIMP
nsNetDiskCache::GetEnabled(PRBool *aEnabled) 
{
  *aEnabled = m_Enabled ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::SetEnabled(PRBool aEnabled) 
{
  nsAutoLock lock(m_Lock) ;

  m_Enabled = aEnabled ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::GetReadOnly(PRBool *aReadOnly) 
{
  *aReadOnly = m_ReadOnly ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::GetNumEntries(PRUint32 *aNumEntries) 
{
  *aNumEntries = m_NumEntries ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::GetMaxEntries(PRUint32 *aMaxEntries)
{
  *aMaxEntries = m_MaxEntries ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::NewCacheEntryIterator(nsISimpleEnumerator **_retval) 
{
  if(!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;

  nsISimpleEnumerator* enumerator = new nsDBEnumerator(m_DB) ;
  if(enumerator) {
    NS_ADDREF(enumerator) ;
    *_retval = enumerator ;
    return NS_OK ;
  }
  else 
    return NS_ERROR_OUT_OF_MEMORY ;
}

NS_IMETHODIMP
nsNetDiskCache::GetNextCache(nsINetDataCache * *aNextCache)
{
  if(!aNextCache)
    return NS_ERROR_NULL_POINTER ;

  *aNextCache = m_pNextCache ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::SetNextCache(nsINetDataCache *aNextCache)
{
  nsAutoLock lock(m_Lock) ;

  m_pNextCache = aNextCache ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::GetCapacity(PRUint32 *aCapacity)
{
  *aCapacity = m_Capacity ;
  return NS_OK ;
}

// I think cache manager should be responsible for purging if current cap is
// bigger than the new value.
NS_IMETHODIMP
nsNetDiskCache::SetCapacity(PRUint32 aCapacity)
{
  nsAutoLock lock(m_Lock) ;

  m_Capacity = aCapacity ;
  return NS_OK ;
}

// note: this could be slow. I left no choice but to do a iteration
// on all cache entries since I don't have control over when all 
// records get cached. 

NS_IMETHODIMP
nsNetDiskCache::GetStorageInUse(PRUint32 *aStorageInUse)
{
  PRUint32 total_size = 0, entry_size ;
  nsresult rv ;

  nsISimpleEnumerator* dbEnum = new nsDBEnumerator(m_DB) ;
  if(dbEnum)
    NS_ADDREF(dbEnum) ;
  else
    return NS_ERROR_FAILURE ;

  nsINetDataCacheRecord* cache_item ;

  PRBool more = PR_FALSE ;

  do {
    dbEnum->HasMoreElements(&more) ;
    if(more) {
      rv = dbEnum->GetNext((nsISupports**)&cache_item) ;
      if(NS_FAILED(rv)) 
        return NS_ERROR_FAILURE ;

      entry_size = 0 ;
      nsCOMPtr<nsINetDataCacheRecord> cache_entry = dont_AddRef(cache_item) ;
      cache_entry->GetStoredContentLength(&entry_size) ;
      total_size += entry_size ;
    }
  } while(more) ;

  NS_RELEASE(dbEnum) ;

  // add the size of the db.
  m_DBFile->GetFileSize(&entry_size) ;
  total_size += entry_size ;

  // we need size in kB
  total_size = total_size/1024 ;

  *aStorageInUse = total_size ;
  return NS_OK ;
}

/*
 * I wish I can just wipe out the whole dir
 * mdb has some nasty problem with seq() after del(). so I have to 
 * call RemoveAll() recursively until all the records are removed. Apparently
 * seq() will be fine again if you re-initialize it with a new 
 * iterator. 
 */
 
NS_IMETHODIMP
nsNetDiskCache::RemoveAll(void)
{
  // shutting down the db
//  m_DB->Shutdown() ;

  // try to remove everything
  nsISimpleEnumerator* dbEnum = new nsDBEnumerator(m_DB) ;
  if(dbEnum)
    NS_ADDREF(dbEnum) ;
  else
    return NS_ERROR_FAILURE ;

  nsINetDataCacheRecord* cache_item ;
  nsresult rv ;
  PRBool more = PR_FALSE ;

  int i = 0, j = 0 ;
  do {
    dbEnum->HasMoreElements(&more) ;
    j++ ;
    if(more) {
      rv = dbEnum->GetNext((nsISupports**)&cache_item) ;
      if(NS_FAILED(rv)) 
        return NS_ERROR_FAILURE ;

      nsCOMPtr<nsINetDataCacheRecord> cache_entry = dont_AddRef(cache_item) ;
      rv = cache_entry->Delete() ;
      i++ ;
      if(NS_FAILED(rv)) 
        return NS_ERROR_FAILURE ;

//      NS_RELEASE(cache_item) ;
    }
  } while(more) ;

  printf("iterated %d times,  delete was called %d times\n", j, i) ;

  NS_RELEASE(dbEnum) ;

  // count num of entries in db
  nsISimpleEnumerator* dbEnumerator = new nsDBEnumerator(m_DB) ;
  if(dbEnumerator)
    NS_ADDREF(dbEnumerator) ;
  else
    return NS_ERROR_FAILURE ;

  PRUint32 NumEntries = 0 ;

  do {
    dbEnumerator->HasMoreElements(&more) ;
    if(more) 
      NumEntries++ ;
  } while (more) ;

  if(NumEntries>0) {
    printf(" %d entries still left, what's wrong?\n", NumEntries) ;
    RemoveAll() ;
  }

  NS_IF_RELEASE(dbEnumerator) ;

  // don't forget the db file itself
  m_DB->Shutdown() ;
  nsFileSpec dbfile ;
  m_DBFile->GetFileSpec(&dbfile) ;
  dbfile.Delete(PR_TRUE) ;

  // reinitilize
  CreateDir(m_pDiskCacheFolder) ;
  InitDB() ;

  return NS_OK ;
}

//////////////////////////////////////////////////////////////////
// nsINetDataDiskCache methods

NS_IMETHODIMP
nsNetDiskCache::GetDiskCacheFolder(nsIFileSpec * *aDiskCacheFolder)
{
  *aDiskCacheFolder = m_pDiskCacheFolder ;
  NS_ADDREF(*aDiskCacheFolder) ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::SetDiskCacheFolder(nsIFileSpec * aDiskCacheFolder)
{
  m_pDiskCacheFolder = aDiskCacheFolder ;

  // re-initialize db
  m_DB->Shutdown() ;
  InitDB() ;

  return NS_OK ;
}

//////////////////////////////////////////////////////////////////
// nsNetDiskCache methods

// we used RecordID to compare, it's easier than key and length
NS_IMETHODIMP
nsNetDiskCache::SearchRUList(PRInt32 id, nsINetDataCacheRecord **_retval) 
{
  if(!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;

  recentlyUsedObject* obj ;
  nsresult rv = 0 ;

  PRCList* list = &g_RecentlyUsedList ;
  if(!PR_CLIST_IS_EMPTY(&g_RecentlyUsedList)) {
    list = g_RecentlyUsedList.next ;
    nsINetDataCacheRecord* pObj ;
    PRInt32 obj_id  = 0 ;

    while(list != &g_RecentlyUsedList) {
      obj = OBJECT_PTR(list) ;
      pObj = obj->cacheObject ;
      rv = pObj->GetRecordID(&obj_id) ;
      if(NS_FAILED(rv))
        return rv ;
      if(obj_id == id) {
        NS_ADDREF(pObj) ; // addref before _retval 
        *_retval = pObj ;
        return NS_OK ;
      }
      list = list->next ;
    }
  }

  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::AddToRUList(nsINetDataCacheRecord* aRecord) 
{
  nsAutoLock lock(m_Lock) ;
  
  int extra = m_ListCount - MAX_OBJECTS_IN_RECENTLY_USED_LIST ;
  while(extra>0)
  {
    recentlyUsedObject* obj = (recentlyUsedObject*) PR_LIST_HEAD(&g_RecentlyUsedList);
    if(obj->cacheObject)
      NS_RELEASE(obj->cacheObject) ;
    PR_REMOVE_LINK(&obj->link) ;
    m_ListCount-- ;
    extra-- ;
  }

  recentlyUsedObject* pNode = PR_NEWZAP(recentlyUsedObject) ;
  PR_APPEND_LINK(&pNode->link, &g_RecentlyUsedList) ;
  pNode->cacheObject = aRecord ;
  m_ListCount++ ; // updata list count
  
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::CountDirSize(PRUint32 * t_size, nsIFileSpec * in_dir) 
{
  nsCOMPtr<nsIDirectoryIterator> i ;
  NS_NewDirectoryIterator(getter_AddRefs(i)) ;
  
  i->Init(in_dir, PR_TRUE) ;
  nsCOMPtr<nsIFileSpec> c_spec ;
  PRBool is_dir, not_done = PR_FALSE ;
  PRUint32 tempsize = 0 ;
  
  do {
    i->GetCurrentSpec(getter_AddRefs(c_spec)) ;

    c_spec->IsDirectory(&is_dir) ;
    if(is_dir) {
      CountDirSize(t_size, c_spec) ;
    }
    c_spec->GetFileSize(&tempsize) ;
    *t_size += tempsize ;

    i->Next() ;
    i->Exists(&not_done) ;
  } while(not_done) ;

  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::CreateDir(nsIFileSpec* dir_spec) 
{
  PRBool does_exist ;
  nsCOMPtr<nsIFileSpec> p_spec ;

  dir_spec->Exists(&does_exist) ;
  if(does_exist) 
    return NS_OK ;

  dir_spec->GetParent(getter_AddRefs(p_spec)) ;
  p_spec->Exists(&does_exist) ;
  if(!does_exist) {
    CreateDir(p_spec) ;
    dir_spec->CreateDir() ;
  }
  else {
    dir_spec->CreateDir() ;
  }
  
  return NS_OK ;
}

