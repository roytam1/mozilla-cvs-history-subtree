/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator.
 * 
 * The Initial Developer of the Original Code is Intel Corp.
 * Portions created by Intel Corp. are
 * Copyright (C) 1999, 1999 Intel Corp.  All
 * Rights Reserved.
 * 
 * Contributor(s): Yixiong Zou <yixiong.zou@intel.com>
 *                 Carl Wong <carl.wong@intel.com>
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
#include "prlog.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsIPref.h"
#include "mcom_db.h"
#include "nsDBEnumerator.h"

#include "nsDiskCacheRecord.h"
#include "netCore.h"

#if !defined(IS_LITTLE_ENDIAN) && !defined(IS_BIG_ENDIAN)
ERROR! Must have a byte order
#endif

#ifdef IS_LITTLE_ENDIAN
#define COPY_INT32(_a,_b)  memcpy(_a, _b, sizeof(int32))
#else
#define COPY_INT32(_a,_b)  /* swap */                   \
    do {                                                \
    ((char *)(_a))[0] = ((char *)(_b))[3];              \
    ((char *)(_a))[1] = ((char *)(_b))[2];              \
    ((char *)(_a))[2] = ((char *)(_b))[1];              \
    ((char *)(_a))[3] = ((char *)(_b))[0];              \
    } while(0)
#endif

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID) ;
static NS_DEFINE_CID(kDBAccessorCID, NS_DBACCESSOR_CID) ;

static const PRUint32 DISK_CACHE_SIZE_DEFAULT = 5*1024*1024 ; // 5MB
static const char * const DISK_CACHE_PREF  = "browser.cache.disk_cache_size";
static const char * const CACHE_DIR_PREF   = "browser.cache.directory";

class nsDiskCacheRecord ;

nsNetDiskCache::nsNetDiskCache() :
  m_Enabled(PR_TRUE) ,
  m_NumEntries(0) ,
  m_pNextCache(0) ,
  m_pDiskCacheFolder(0) ,
  m_StorageInUse(0) ,
  m_DB(0) ,
  m_DBCorrupted(PR_FALSE) 
{
  // set it to INF for now 
  m_MaxEntries = (PRUint32)-1 ;

  NS_INIT_REFCNT();

}

nsNetDiskCache::~nsNetDiskCache()
{
  SetSpecialEntry() ;

  NS_IF_RELEASE(m_DB) ;
  

  // FUR
  // I think that, eventually, we also want a distinguished key in the DB which
  // means "clean cache shutdown".  You clear this flag when the db is first
  // opened and set it just before the db is closed.  If the db wasn't shutdown
  // cleanly in a prior session, i.e. because the app crashed, on startup you
  // scan all the individual files in directories and look for "orphans",
  // i.e. cache files which don't have corresponding entries in the db.  That's
  // also when storage-in-use and number of entries would be recomputed.
  //
  // We don't necessarily need all this functionality immediately, though.


  if(m_DBCorrupted) {
    
    nsFileSpec cacheFolder ;
    m_pDiskCacheFolder->GetFileSpec(&cacheFolder) ;
    
    char nameInt[6] ;

    for(nsDirectoryIterator di(cacheFolder, PR_FALSE); di.Exists(); di++) {
      char* filename = di.Spec().GetLeafName() ;
      char* pname = nameInt ;
      pname = PL_strncpyz(pname, filename, 6) ;

      if(PL_strcmp(pname, "trash") == 0)
        RemoveFolder(di.Spec()) ;
      
      nsCRT::free(filename) ;  
    }
  }
}

NS_IMETHODIMP
nsNetDiskCache::Init(void) 
{
  nsresult rv ;
  
  // don't initialize if no cache folder is set. 
  if(!m_pDiskCacheFolder) return NS_OK ;

  if(!m_DB) {
    m_DB = new nsDBAccessor() ;
    if(!m_DB)
      return NS_ERROR_OUT_OF_MEMORY ;
    else
      NS_ADDREF(m_DB) ;
  }

  // create cache sub directories
  nsCOMPtr<nsIFileSpec> cacheSubDir;
  rv = NS_NewFileSpec(getter_AddRefs(cacheSubDir));

  for (int i=0; i < 32; i++) {
    rv = cacheSubDir->FromFileSpec(m_pDiskCacheFolder) ;
    if(NS_FAILED(rv))
      return rv ;

    char dirName[3];
    PR_snprintf (dirName, 3, "%0.2x", i);
    cacheSubDir->AppendRelativeUnixPath (dirName) ;
    CreateDir(cacheSubDir);
  }

  return InitDB() ;
}

NS_IMETHODIMP
nsNetDiskCache::InitDB(void)
{
  nsresult rv ; 

  if(!m_DBFile) {
    NS_NewFileSpec(getter_AddRefs(m_DBFile)) ;
    if(!m_DBFile)
      return NS_ERROR_OUT_OF_MEMORY ;
  }

  rv = m_DBFile->FromFileSpec(m_pDiskCacheFolder) ;
  if(NS_FAILED(rv))
    return rv ;

  m_DBFile->AppendRelativeUnixPath("cache.db") ;

  rv = m_DB->Init(m_DBFile) ;

  if(rv == NS_ERROR_FAILURE) {
    // try recovery if error 
    DBRecovery() ;
  }

  rv = GetSpecialEntry() ;
  if(rv == NS_ERROR_FAILURE) {
    // try recovery if error 
    DBRecovery() ;
  }

  return rv ;
}

//////////////////////////////////////////////////////////////////////////
// nsISupports methods

NS_IMPL_ISUPPORTS3(nsNetDiskCache,
                   nsINetDataDiskCache,
                   nsINetDataCache,
                   nsISupports) 

///////////////////////////////////////////////////////////////////////////
// nsINetDataCache Method 

NS_IMETHODIMP
nsNetDiskCache::GetDescription(PRUnichar* *aDescription) 
{
  nsAutoString description("Disk Cache") ;
  *aDescription = description.ToNewUnicode() ;
  if(!*aDescription)
    return NS_ERROR_OUT_OF_MEMORY ;

  return NS_OK ;
}

/* don't alloc mem for nsICachedNetData. 
 * RecordID is generated using the same scheme in nsCacheDiskData,
 * see GetCachedNetData() for detail.
 */
NS_IMETHODIMP
nsNetDiskCache::Contains(const char* key, PRUint32 length, PRBool *_retval) 
{
  *_retval = PR_FALSE ;

  NS_ASSERTION(m_DB, "no db.") ;

  PRInt32 id = 0 ;
  nsresult rv = m_DB->GetID(key, length, &id) ;

  if(NS_FAILED(rv)) {
    // try recovery if error 
    DBRecovery() ;
    return rv ;
  }

  void* info = 0 ;
  PRUint32 info_size = 0 ;

  rv = m_DB->Get(id, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) 
    *_retval = PR_TRUE ;
        
  if(NS_FAILED(rv)) {
    // try recovery if error 
    DBRecovery() ;
  }

  return rv ;
}

/* regardless if it's cached or not, a copy of nsNetDiskCache would 
 * always be returned. so release it appropriately. 
 * if mem alloced, updata m_NumEntries also.
 * for now, the new nsCachedNetData is not written into db yet since
 * we have nothing to write.
 */
NS_IMETHODIMP
nsNetDiskCache::GetCachedNetData(const char* key, PRUint32 length, nsINetDataCacheRecord **_retval) 
{
  NS_ASSERTION(m_DB, "no db.") ;

  nsresult rv = 0 ;
  if (!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;

  PRInt32 id = 0 ;
  rv = m_DB->GetID(key, length, &id) ;
  if(NS_FAILED(rv)) {
    // try recovery if error 
    DBRecovery() ;
    return rv ;
  }

  // construct an empty record
  nsDiskCacheRecord* newRecord = new nsDiskCacheRecord(m_DB, this) ;
  if(!newRecord) 
    return NS_ERROR_OUT_OF_MEMORY ;

  rv = newRecord->Init(key, length, id) ;
  if(NS_FAILED(rv)) {
    delete newRecord ;
    return rv ;
  }

  NS_ADDREF(newRecord) ; // addref for _retval 
  *_retval = (nsINetDataCacheRecord*) newRecord ;
  
  void* info = 0 ;
  PRUint32 info_size = 0 ;

  rv = m_DB->Get(id, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) {

    // this is a previously cached record 
    nsresult r1 ;
    r1 = newRecord->RetrieveInfo(info, info_size) ;

    if(NS_SUCCEEDED(rv)) 
      return NS_OK ;
    else { 
      // probably a bad one
      NS_RELEASE(newRecord) ;
      *_retval = nsnull ;
      return r1;
    }
    
  } else if (NS_SUCCEEDED(rv) && !info) {
    // this is a new record. 
    m_NumEntries ++ ;
    return NS_OK ;
  } else {
    // database error.
    DBRecovery() ;
    return rv ;
  }
}

/* get an nsICachedNetData, mem needs to be de-alloced if not found. */
NS_IMETHODIMP
nsNetDiskCache::GetCachedNetDataByID(PRInt32 RecordID, nsINetDataCacheRecord **_retval) 
{
  NS_ASSERTION(m_DB, "no db.") ;

  if (!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;
  
  nsresult rv ;

  void* info = 0 ;
  PRUint32 info_size = 0 ;

  rv = m_DB->Get(RecordID, &info, &info_size) ;
  if(NS_SUCCEEDED(rv) && info) {

    // construct an empty record if only found in db
    nsDiskCacheRecord* newRecord = new nsDiskCacheRecord(m_DB, this) ;
    if(!newRecord) 
      return NS_ERROR_OUT_OF_MEMORY ;

    NS_ADDREF(newRecord) ; // addref for _retval
    rv = newRecord->RetrieveInfo(info, info_size) ;

    if(NS_SUCCEEDED(rv)) {
      *_retval = (nsINetDataCacheRecord*) newRecord ;
      return NS_OK ;
    } 
    else {
      // bad record, I guess
      NS_RELEASE(newRecord) ; // release if bad things happen
      return rv ;
    }
  } else {
    NS_ERROR("Error: RecordID not in DB\n") ;
    DBRecovery() ;
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
  m_Enabled = aEnabled ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::GetFlags(PRUint32 *aFlags) 
{
  *aFlags = FILE_PER_URL_CACHE;
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
  NS_ASSERTION(m_DB, "no db.") ;

  if(!_retval)
    return NS_ERROR_NULL_POINTER ;

  *_retval = nsnull ;

  nsISimpleEnumerator* enumerator = new nsDBEnumerator(m_DB, this) ;
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
  m_pNextCache = aNextCache ;
  return NS_OK ;
}

// db size can always be measured at the last minute. Since it's hard
// to know before hand. 
NS_IMETHODIMP
nsNetDiskCache::GetStorageInUse(PRUint32 *aStorageInUse)
{
  NS_ASSERTION(m_DB, "no db.") ;

  PRUint32 total_size = m_StorageInUse ;

  /*
  PRUint32 len = 0 ;
  // add the size of the db.
  m_DB->GetDBFilesize(&len) ;
  total_size += len ;
  */

  // we need size in kB
  total_size = total_size >> 10 ;

  *aStorageInUse = total_size ;
  return NS_OK ;
}

/*
 * The whole cache dirs can be whiped clean since all the cache 
 * files are resides in seperate hashed dirs. It's safe to do so.
 */
 
NS_IMETHODIMP
nsNetDiskCache::RemoveAll(void)
{
  NS_ASSERTION(m_DB, "no db.") ;
  NS_ASSERTION(m_pDiskCacheFolder, "no cache folder.") ;

  // remove all the sub folders
  nsFileSpec cacheSubDir;

  for (int i=0; i < 32; i++) {
    m_pDiskCacheFolder->GetFileSpec(&cacheSubDir) ;

    char dirName[3];
    PR_snprintf (dirName, 3, "%0.2x", i);
    cacheSubDir += dirName ;
    RemoveFolder(cacheSubDir) ;
  }

  // don't forget the db file itself
  m_DB->Shutdown() ;
  nsFileSpec dbfile ;
  m_DBFile->GetFileSpec(&dbfile) ;
  dbfile.Delete(PR_TRUE) ;

  // reinitilize
  return Init() ;
}

//////////////////////////////////////////////////////////////////
// nsINetDataDiskCache methods

NS_IMETHODIMP
nsNetDiskCache::GetDiskCacheFolder(nsIFileSpec * *aDiskCacheFolder)
{
  *aDiskCacheFolder = nsnull ;
  NS_ASSERTION(m_pDiskCacheFolder, "no cache folder.") ;

  *aDiskCacheFolder = m_pDiskCacheFolder ;
  NS_ADDREF(*aDiskCacheFolder) ;
  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::SetDiskCacheFolder(nsIFileSpec * aDiskCacheFolder)
{
  if(!m_pDiskCacheFolder) {
    NS_NewFileSpec(getter_AddRefs(m_pDiskCacheFolder));
    if(!m_pDiskCacheFolder) 
      return NS_ERROR_OUT_OF_MEMORY ;
    
    m_pDiskCacheFolder = aDiskCacheFolder ;
    return Init() ;
  } 
  else {
    char *newfolder, *oldfolder ;
    m_pDiskCacheFolder->GetNativePath(&oldfolder) ;
    aDiskCacheFolder->GetNativePath(&newfolder) ;

    if(PL_strcmp(newfolder, oldfolder) != 0) {
      m_pDiskCacheFolder = aDiskCacheFolder ;

      // do we need to blow away old cache before building a new one?
      // return RemoveAll() ;

      m_DB->Shutdown() ;
      return Init() ;

    } else 
      return NS_OK ;
  }
}

//////////////////////////////////////////////////////////////////
// nsNetDiskCache methods

// create a directory (recursively)
NS_IMETHODIMP
nsNetDiskCache::CreateDir(nsIFileSpec* dir_spec) 
{
  PRBool does_exist ;
  nsCOMPtr<nsIFileSpec> p_spec ;

  dir_spec->Exists(&does_exist) ;
  if(does_exist) 
    return NS_OK ;

  nsresult rv = dir_spec->GetParent(getter_AddRefs(p_spec)) ;
  if(NS_FAILED(rv))
    return rv ;

  p_spec->Exists(&does_exist) ;
  if(!does_exist) {
    CreateDir(p_spec) ;
    rv = dir_spec->CreateDir() ;
    if(NS_FAILED(rv))
      return rv ;
  }
  else {
    rv = dir_spec->CreateDir() ;
    if(NS_FAILED(rv))
      return rv ;
  }
  
  return NS_OK ;
}

// We can't afford to make a *separate* pass over the whole db on every
// startup, just to figure out m_NumEntries and m_StorageInUse.  (This is a
// several second operation on a large db).  We'll likely need to store
// distinguished keys in the db that contain these values and update them
// incrementally, except when failure to shut down the db cleanly is detected.

NS_IMETHODIMP
nsNetDiskCache::GetSpecialEntry(void)
{
  void* pInfo ;
  PRUint32 InfoSize ;

  nsresult rv = m_DB->GetSpecialEntry(&pInfo, &InfoSize) ;
  if(NS_FAILED(rv))
    return rv ;

  if(!pInfo && InfoSize == 0) {
    // must be a new DB
    m_NumEntries = 0 ;
    m_StorageInUse = 0 ;
  } 
  else {
    char * cur_ptr = NS_STATIC_CAST(char*, pInfo) ;
    
    // get m_NumEntries
    COPY_INT32(&m_NumEntries, cur_ptr) ;
    cur_ptr += sizeof(PRUint32) ;
    
    // get m_StorageInUse
    COPY_INT32(&m_StorageInUse, cur_ptr) ;
    cur_ptr += sizeof(PRUint32) ;
    
    PR_ASSERT(cur_ptr == NS_STATIC_CAST(char*, pInfo) + InfoSize);
  }

  return NS_OK ;
}

NS_IMETHODIMP
nsNetDiskCache::SetSpecialEntry(void)
{
  PRUint32 InfoSize ;
  
  InfoSize = sizeof m_NumEntries ;
  InfoSize += sizeof m_StorageInUse ;
  
  void* pInfo = nsAllocator::Alloc(InfoSize*sizeof(char)) ;
  if(!pInfo) 
    return NS_ERROR_OUT_OF_MEMORY ;
  
  char* cur_ptr = NS_STATIC_CAST(char*, pInfo) ;

  COPY_INT32(cur_ptr, &m_NumEntries) ;
  cur_ptr += sizeof(PRUint32) ;

  COPY_INT32(cur_ptr, &m_StorageInUse) ;
  cur_ptr += sizeof(PRUint32) ;
  
  PR_ASSERT(cur_ptr == NS_STATIC_CAST(char*, pInfo) + InfoSize);
  
  return m_DB->SetSpecialEntry(pInfo, InfoSize) ;
}

// this routine will be called everytime we have a db corruption. 
// m_DB will be re-initialized, m_StorageInUse and m_NumEntries will
// be reset.
NS_IMETHODIMP
nsNetDiskCache::DBRecovery(void)
{
  // rename all the sub cache dirs and remove them later during dtor. 
  nsresult rv = RenameCacheSubDirs() ;
  if(NS_FAILED(rv))
    return rv ;

  // remove corrupted db file, don't care if db->shutdown fails or not.
  m_DB->Shutdown() ;
  
  nsFileSpec dbfile ;
  m_DBFile->GetFileSpec(&dbfile) ;
  dbfile.Delete(PR_TRUE) ;

  // make sure it's not there any more
  PRBool exists = dbfile.Exists() ;
  if(exists) {
    NS_ERROR("can't remove old db.") ;
    return NS_ERROR_FAILURE ;
  }

  // reinitilize DB 
  return InitDB() ;
}

// this routine will add string "trash" to current CacheSubDir names. 
// e.g. 00->trash00, 1f->trash1f. and update the m_DBCorrupted. 

NS_IMETHODIMP
nsNetDiskCache::RenameCacheSubDirs(void) 
{
  nsCOMPtr<nsIFileSpec> cacheSubDir;
  nsresult rv = NS_NewFileSpec(getter_AddRefs(cacheSubDir)) ;

  for (int i=0; i < 32; i++) {
    rv = cacheSubDir->FromFileSpec(m_pDiskCacheFolder) ;
    if(NS_FAILED(rv))
      return rv ;

    char oldName[3], newName[8];
    PR_snprintf(oldName, 3, "%0.2x", i) ;
    cacheSubDir->AppendRelativeUnixPath(oldName) ;

    // re-name the directory
    PR_snprintf(newName, 8, "trash%0.2x", i) ;
    rv = cacheSubDir->Rename(newName) ;
    if(NS_FAILED(rv))
      // TODO, error checking
      return NS_ERROR_FAILURE ;
  }

  // update m_DBCorrupted 
  m_DBCorrupted = PR_TRUE ;

  return NS_OK ;
}

// this routine is used by dtor and RemoveAll() to clean up dirs.
NS_IMETHODIMP
nsNetDiskCache::RemoveFolder(nsFileSpec aFolder)
{
  for(nsDirectoryIterator di(aFolder, PR_FALSE); di.Exists(); di++) {
    di.Spec().Delete(PR_TRUE) ;
  }
  
  aFolder.Delete(PR_FALSE) ; // recursive delete
  
  return NS_OK ;
}
