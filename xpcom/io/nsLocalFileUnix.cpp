/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Mike Shaver <shaver@mozilla.org>
 */

/*
 * Implementation of nsIFile for ``Unixy'' systems.
 */

/* we're going to need some autoconf loving, I can just tell */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsIAllocator.h"
#include "nsIDirectoryEnumerator.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsLocalFileUnix.h"
#include "nsIComponentManager.h"

#define VALIDATE_STAT_CACHE()                   \
  PR_BEGIN_MACRO                                \
  if (!mHaveCachedStat) {                       \
      FillStatCache();                          \
      if (!mHaveCachedStat)                     \
         return NSRESULT_FOR_ERRNO();           \
  }                                             \
  PR_END_MACRO

#define CHECK_mPath()				\
  PR_BEGIN_MACRO				\
    if (!(const char *)mPath)			\
        return NS_ERROR_NOT_INITIALIZED;	\
  PR_END_MACRO

#define mLL_II2L(hi, lo, res)                   \
  LL_I2L(res, hi);                              \
  LL_SHL(res, res, 32);                         \
  LL_ADD(res, res, lo);

#define mLL_L2II(a64, hi, lo)                   \
  PR_BEGIN_MACRO                                \
    PRInt64 tmp;                                \
    LL_SHR(tmp, a64, 32);                       \
    LL_L2I(hi, tmp);                            \
    LL_SHL(tmp, a64, 32);                       \
    LL_SHR(tmp, tmp, 32);                       \
    LL_L2I(lo, tmp);                            \
  PR_END_MACRO

/* directory enumerator */
class NS_COM
nsDirEnumeratorUnix : public nsISimpleEnumerator
{
 public:
    nsDirEnumeratorUnix();
    virtual ~nsDirEnumeratorUnix();

    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID,
                            void* *aInstancePtr);

    // nsISupports interface
    NS_DECL_ISUPPORTS
    
    NS_DECL_NSISIMPLEENUMERATOR

    NS_IMETHOD Init(nsIFile *parent, PRBool ignored);
 protected:
    NS_IMETHOD GetNextEntry();

    DIR *mDir;
    struct dirent *mEntry;
    nsXPIDLCString mParentPath;
};

nsDirEnumeratorUnix::nsDirEnumeratorUnix() :
  mDir(nsnull), mEntry(nsnull)
{
    NS_INIT_REFCNT();
}

nsDirEnumeratorUnix::~nsDirEnumeratorUnix()
{
    if (mDir)
        closedir(mDir);
}

NS_IMPL_ISUPPORTS1(nsDirEnumeratorUnix, nsISimpleEnumerator)

NS_IMETHODIMP
nsDirEnumeratorUnix::Init(nsIFile *parent, PRBool resolveSymlinks /*ignored*/)
{
    nsXPIDLCString dirPath;
    if (NS_FAILED(parent->GetPath(getter_Copies(dirPath))) ||
        (const char *)dirPath == 0)
        return NS_ERROR_FILE_INVALID_PATH;

    if (NS_FAILED(parent->GetPath(getter_Copies(mParentPath))))
        return NS_ERROR_FAILURE;

    mDir = opendir(dirPath);
    if (!mDir)
        return NSRESULT_FOR_ERRNO();
    return GetNextEntry();
}

NS_IMETHODIMP
nsDirEnumeratorUnix::HasMoreElements(PRBool *result)
{
    *result = mDir && mEntry;
    return NS_OK;
}

NS_IMETHODIMP
nsDirEnumeratorUnix::GetNext(nsISupports **_retval)
{
    nsresult rv;
    if (!mDir || !mEntry) {
        *_retval = nsnull;
        return NS_OK;
    }

    nsIFile *file = new nsLocalFile();
    if (!file)
        return NS_ERROR_OUT_OF_MEMORY;

    if (NS_FAILED(rv = file->AppendPath(mEntry->d_name))) {
        NS_RELEASE(file);
        return rv;
    }
    *_retval = NS_STATIC_CAST(nsISupports *, file);
    return GetNextEntry();
}

NS_IMETHODIMP
nsDirEnumeratorUnix::GetNextEntry()
{
    do {
        errno = 0;
        mEntry = readdir(mDir);

        /* end of dir or error */
        if (!mEntry)
            return NSRESULT_FOR_ERRNO();

        /* keep going past "." and ".." */
    } while (mEntry->d_name[0] == '.' && 
             (mEntry->d_name[1] == '\0' || /* .\0 */
              (mEntry->d_name[1] == '.' &&
               mEntry->d_name[2] == '\0'))); /* ..\0 */
    return NS_OK;
}

nsLocalFile::nsLocalFile() :
    mHaveCachedStat(PR_FALSE)
{
    mPath = "";
    NS_INIT_REFCNT();
}

nsLocalFile::~nsLocalFile()
{
}

NS_IMPL_ISUPPORTS1(nsLocalFile, nsIFile);

nsresult
nsLocalFile::Create(nsISupports *outer, const nsIID &aIID, void **aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    NS_ENSURE_PROPER_AGGREGATION(outer, aIID);
    
    *aInstancePtr = 0;

    nsCOMPtr<nsIFile> inst = new nsLocalFile();
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;
    return inst->QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
nsLocalFile::Clone(nsIFile **file)
{
    NS_ENSURE_ARG(file);
    
    *file = nsnull;

    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = nsComponentManager::CreateInstance(NS_LOCAL_FILE_PROGID, 
                                                     nsnull, 
                                                     NS_GET_IID(nsILocalFile),
                                                     getter_AddRefs(localFile));
    
    if (NS_FAILED(rv)) 
        return rv;
    
    rv = localFile->InitWithPath(mPath);
    
    if (NS_FAILED(rv)) 
        return rv;

    *file = localFile;
    NS_ADDREF(*file);
    
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::InitWithPath(const char *filePath)
{
    NS_ENSURE_ARG(filePath);
    mPath = filePath;
    InvalidateCache();
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::CreateAllAncestors(PRUint32 permissions)
{
    /* <jband> I promise to play nice */
    char *buffer = NS_CONST_CAST(char *, (const char *)mPath),
        *ptr = buffer;

#ifdef DEBUG_NSIFILE
    fprintf(stderr, "nsIFile: before: %s\n", buffer);
#endif

    while ((ptr = strchr(ptr + 1, '/'))) {
        /*
         * Sequences of '/' are equivalent to a single '/'.
         */
        if (ptr[1] == '/')
            continue;

        /*
         * If the path has a trailing slash, don't make the last component here,
         * because we'll get EEXISTS in Create when we try to build the final
         * component again, and it's easier to condition the logic here than
         * there.
         */
        if (!ptr[1])
            break;
        /* Temporarily NUL-terminate here */
        *ptr = '\0';
#ifdef DEBUG_NSIFILE
        fprintf(stderr, "nsIFile: mkdir(\"%s\")\n", buffer);
#endif
        int result = mkdir(buffer, permissions);
        /* Put the / back before we (maybe) return */
        *ptr = '/';

        /*
         * We could get EEXISTS for an existing file -- not directory --
         * with the name of one of our ancestors, but that's OK: we'll get
         * ENOTDIR when we try to make the next component in the path,
         * either here on back in Create, and error out appropriately.
         */
        if (result == -1 && errno != EEXIST)
            return NSRESULT_FOR_ERRNO();
    }

#ifdef DEBUG_NSIFILE
    fprintf(stderr, "nsIFile: after: %s\n", buffer);
#endif
            
    return NS_OK;

}


NS_IMETHODIMP  
nsLocalFile::OpenNSPRFileDesc(PRInt32 flags, PRInt32 mode, PRFileDesc **_retval)
{
    CHECK_mPath();
   
    *_retval = PR_Open(mPath, flags, mode);
    
    if (*_retval)
        return NS_OK;

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP  
nsLocalFile::OpenANSIFileDesc(const char *mode, FILE * *_retval)
{
    CHECK_mPath(); 
   
    *_retval = fopen(mPath, mode);
    
    if (*_retval)
        return NS_OK;

    return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsLocalFile::Create(PRUint32 type, PRUint32 permissions)
{
    CHECK_mPath();
    if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
        return NS_ERROR_FILE_UNKNOWN_TYPE;

    int result;
    /* use creat(2) for NORMAL_FILE, mkdir(2) for DIRECTORY */
    int (*creationFunc)(const char *, mode_t) =
        type == NORMAL_FILE_TYPE ? creat : mkdir;

    result = creationFunc((const char *)mPath, permissions);

    if (result == -1 && errno == ENOENT) {
        /*
         * If we failed because of missing ancestor components, try to create
         * them and then retry the original creation.
         * 
         * Ancestor directories get the same permissions as the file we're
         * creating, with the X bit set for each of (user,group,other) with
         * an R bit in the original permissions.  If you want to do anything
         * fancy like setgid or sticky bits, do it by hand.
         */
        int dirperm = permissions;
        if (permissions & S_IRUSR)
            dirperm |= S_IXUSR;
        if (permissions & S_IRGRP)
            dirperm |= S_IXGRP;
        if (permissions & S_IROTH)
            dirperm |= S_IXOTH;

#ifdef DEBUG_NSIFILE
        fprintf(stderr, "nsIFile: perm = %o, dirperm = %o\n", permissions, 
                dirperm);
#endif

        if (NS_FAILED(CreateAllAncestors(dirperm)))
            return NS_ERROR_FAILURE;

#ifdef DEBUG_NSIFILE
        fprintf(stderr, "nsIFile: Create(\"%s\") again\n", (const char *)mPath);
#endif
        result = creationFunc((const char *)mPath, permissions);
    }

    /* creat(2) leaves the file open */
    if (result >= 0 && type == NORMAL_FILE_TYPE) {
	close(result);
	return NS_OK;
    }

    return NSRESULT_FOR_RETURN(result);
}

NS_IMETHODIMP
nsLocalFile::AppendPath(const char *fragment)
{
    NS_ENSURE_ARG(fragment);
    CHECK_mPath();
    char * newPath = (char *)nsAllocator::Alloc(strlen(mPath) +
                                                strlen(fragment) + 2);
    if (!newPath)
        return NS_ERROR_OUT_OF_MEMORY;
    strcpy(newPath, mPath);
    strcat(newPath, "/");
    strcat(newPath, fragment);
    mPath = newPath;
    InvalidateCache();
    nsAllocator::Free(newPath);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Normalize()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsLocalFile::GetLeafNameRaw(const char **_retval)
{
    CHECK_mPath();
    char *leafName = strrchr((const char *)mPath, '/');
    if (!leafName)
	return NS_ERROR_FILE_INVALID_PATH;
    *_retval = ++leafName;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetLeafName(char **aLeafName)
{
    NS_ENSURE_ARG_POINTER(aLeafName);
    nsresult rv;
    const char *leafName;
    if (NS_FAILED(rv = GetLeafNameRaw(&leafName)))
	return rv;
    
    *aLeafName = nsCRT::strdup(leafName);
    if (!*aLeafName)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPath(char **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    if (!(const char *)mPath) {
	*_retval = nsnull;
	return NS_OK;
    }

    *_retval = nsCRT::strdup((const char *)mPath);
    if (!*_retval)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::CopyTo(nsIFile *newParent, const char *newName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinks(nsIFile *newParent, const char *newName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::MoveTo(nsIFile *newParent, const char *newName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::Delete(PRBool recursive)
{
    VALIDATE_STAT_CACHE();
    PRBool isDir = S_ISDIR(mCachedStat.st_mode);

    /* XXX ?
     * if (!isDir && recursive)
     *     return NS_ERROR_INVALID_ARG;
     */
    InvalidateCache();

    if (isDir) {
        if (recursive) {
            nsDirEnumeratorUnix *dir = new nsDirEnumeratorUnix();
            if (!dir)
                return NS_ERROR_OUT_OF_MEMORY;

            nsresult rv = dir->Init(this, PR_FALSE);
            if (NS_FAILED(rv)) {
                delete dir;
                return rv;
            }

            nsCOMPtr<nsISimpleEnumerator> iterator;
            iterator = do_QueryInterface(dir, &rv);
            if (NS_FAILED(rv)) {
                delete dir;
                return rv;
            }

            PRBool more;
            rv = iterator->HasMoreElements(&more);
            while (NS_SUCCEEDED(rv) && more) {
                nsCOMPtr<nsISupports> item;
                nsCOMPtr<nsIFile> file;
                rv = iterator->GetNext(getter_AddRefs(item));
                if (NS_FAILED(rv))
                    return NS_ERROR_FAILURE;
                file = do_QueryInterface(item, &rv);
                if (NS_FAILED(rv))
                    return NS_ERROR_FAILURE;
                if (NS_FAILED(rv = file->Delete(recursive)))
                    return rv;
                rv = iterator->HasMoreElements(&more);
            }
        }

        if (rmdir(mPath) == -1)
            return NSRESULT_FOR_ERRNO();
    } else {
        if (unlink(mPath) == -1)
            return NSRESULT_FOR_ERRNO();
    }

    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetLastModificationDate(PRInt64 *aLastModificationDate)
{
    NS_ENSURE_ARG(aLastModificationDate);
    VALIDATE_STAT_CACHE();
    mLL_II2L(0, (PRUint32)mCachedStat.st_mtime, *aLastModificationDate);
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::SetLastModificationDate(PRInt64 aLastModificationDate)
{
    int result;
    if (aLastModificationDate) {
        VALIDATE_STAT_CACHE();
        struct utimbuf ut;
        ut.actime = mCachedStat.st_atime;
        PRInt32 hi, lo;
        mLL_L2II(aLastModificationDate, hi, lo);
        ut.modtime = (time_t)lo;
        result = utime(mPath, &ut);
    } else {
        result = utime(mPath, NULL);
    }
    InvalidateCache();
    return NSRESULT_FOR_RETURN(result);
}

NS_IMETHODIMP  
nsLocalFile::GetLastModificationDateOfLink(PRInt64 *aLastModificationDateOfLink)
{
    NS_ENSURE_ARG(aLastModificationDateOfLink);
    struct stat sbuf;
    if (lstat(mPath, &sbuf) == -1)
        return NSRESULT_FOR_ERRNO();
    mLL_II2L(0, (PRUint32)sbuf.st_mtime, *aLastModificationDateOfLink);
    return NS_OK;
}

/*
 * utime(2) may or may not dereference symlinks, joy.
 */
NS_IMETHODIMP  
nsLocalFile::SetLastModificationDateOfLink(PRInt64 aLastModificationDateOfLink)
{
    return SetLastModificationDate(aLastModificationDateOfLink);
}

/*
 * only send back permissions bits: maybe we want to send back the whole
 * mode_t to permit checks against other file types?
 */
#define NORMALIZE_PERMS(mode)  ((mode)& (S_IRWXU | S_IRWXG | S_IRWXO))

NS_IMETHODIMP  
nsLocalFile::GetPermissions(PRUint32 *aPermissions)
{
    NS_ENSURE_ARG(aPermissions);
    VALIDATE_STAT_CACHE();
    *aPermissions = NORMALIZE_PERMS(mCachedStat.st_mode);
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetPermissionsOfLink(PRUint32 *aPermissionsOfLink)
{
    NS_ENSURE_ARG(aPermissionsOfLink);
    struct stat sbuf;
    if (lstat(mPath, &sbuf) == -1)
        return NSRESULT_FOR_ERRNO();
    *aPermissionsOfLink = NORMALIZE_PERMS(sbuf.st_mode);
    return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::SetPermissions(PRUint32 aPermissions)
{
    InvalidateCache();
    return NSRESULT_FOR_RETURN(chmod(mPath, aPermissions));
}

NS_IMETHODIMP  
nsLocalFile::SetPermissionsOfLink(PRUint32 aPermissions)
{
    return SetPermissions(aPermissions);
}

NS_IMETHODIMP
nsLocalFile::GetFileSize(PRInt64 *aFileSize)
{
    NS_ENSURE_ARG_POINTER(aFileSize);
    InvalidateCache();
    /* XXX autoconf for and use stat64 if available */
    mLL_II2L(0, (PRUint32)mCachedStat.st_size, *aFileSize);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetFileSize(PRInt64 aFileSize)
{
    PRInt32 hi, lo;
    mLL_L2II(aFileSize, hi, lo);
    /* XXX truncate64? */
    if (truncate((const char *)mPath, (off_t)lo) == -1)
        return NSRESULT_FOR_ERRNO();
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetFileSizeOfLink(PRInt64 *aFileSize)
{
    NS_ENSURE_ARG(aFileSize);
    struct stat sbuf;
    if (lstat(mPath, &sbuf) == -1)
        return NSRESULT_FOR_ERRNO();
    /* XXX autoconf for and use lstat64 if available */
    mLL_II2L(0, (PRInt32)sbuf.st_size, *aFileSize);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetDiskSpaceAvailable(PRInt64 *aDiskSpaceAvailable)
{
    NS_ENSURE_ARG_POINTER(aDiskSpaceAvailable);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::GetParent(nsIFile **aParent)
{
    NS_ENSURE_ARG_POINTER(aParent);
    return NS_ERROR_NOT_IMPLEMENTED;
}

/*
 * The results of Exists, isWritable and isReadable are not cached.
 */

NS_IMETHODIMP
nsLocalFile::Exists(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    PRBool accessOK;
    *_retval = accessOK = (access(mPath, F_OK) == 0);
    if (accessOK || errno == EACCES)
        return NS_OK;
    return NSRESULT_FOR_ERRNO();
}

NS_IMETHODIMP
nsLocalFile::IsWritable(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    PRBool accessOK;
    *_retval = accessOK = (access(mPath, W_OK) == 0);
    if (accessOK || errno == EACCES)
        return NS_OK;
    return NSRESULT_FOR_ERRNO();
}

NS_IMETHODIMP
nsLocalFile::IsReadable(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    PRBool accessOK;
    *_retval = accessOK = (access(mPath, R_OK) == 0);
    if (accessOK || errno == EACCES)
        return NS_OK;
    return NSRESULT_FOR_ERRNO();
}

NS_IMETHODIMP
nsLocalFile::IsExecutable(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    PRBool accessOK;
    *_retval = accessOK = (access(mPath, X_OK) == 0);
    if (accessOK || errno == EACCES)
        return NS_OK;
    return NSRESULT_FOR_ERRNO();
}

NS_IMETHODIMP
nsLocalFile::IsDirectory(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    VALIDATE_STAT_CACHE();
    *_retval = S_ISDIR(mCachedStat.st_mode);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsFile(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    VALIDATE_STAT_CACHE();
    *_retval = S_ISREG(mCachedStat.st_mode);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsHidden(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    nsresult rv;
    const char *leafName;
    if (NS_FAILED(rv = GetLeafNameRaw(&leafName)))
	return rv;
    *_retval = (leafName[0] == '.');
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSymlink(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    VALIDATE_STAT_CACHE();
    *_retval = S_ISLNK(mCachedStat.st_mode);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSpecial(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    VALIDATE_STAT_CACHE();
    *_retval = !(S_ISLNK(mCachedStat.st_mode) || S_ISREG(mCachedStat.st_mode) ||
		 S_ISDIR(mCachedStat.st_mode));
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, PRBool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = PR_FALSE;
    
    nsresult rv;
    nsXPIDLCString myPath, inPath;
    
    if (NS_FAILED(rv = GetPath(getter_Copies(myPath))))
        return rv;
    if (NS_FAILED(rv = inFile->GetPath(getter_Copies(inPath))))
        return rv;
    *_retval = !strcmp(inPath, myPath);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsContainedIn(nsIFile *inFile, PRBool recur, PRBool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG_POINTER(_retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::GetTarget(char **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    VALIDATE_STAT_CACHE();
    if (!S_ISLNK(mCachedStat.st_mode))
        return NS_ERROR_FILE_INVALID_PATH;

    PRInt64 targetSize64;
    if (NS_FAILED(GetFileSizeOfLink(&targetSize64)))
        return NS_ERROR_FAILURE;

    PRInt32 hi, lo;
    mLL_L2II(targetSize64, hi, lo);
    char *target = (char *)nsAllocator::Alloc(lo);
    if (!target)
        return NS_ERROR_OUT_OF_MEMORY;

    int result = readlink(mPath, target, (size_t)lo);
    if (!result) {
        *_retval = target;
        return NS_OK;
    }
    nsAllocator::Free(target);
    return NSRESULT_FOR_ERRNO();
}

NS_IMETHODIMP
nsLocalFile::Spawn(const char *args)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator **entries)
{
    nsDirEnumeratorUnix *dir = new nsDirEnumeratorUnix();
    if (!dir)
        return NS_ERROR_OUT_OF_MEMORY;
    
    nsresult rv = dir->Init(this, PR_FALSE);
    if (NS_FAILED(rv)) {
        delete dir;
        return rv;
    }

    /* QI needed? */
    *entries = dir;
    NS_ADDREF(*entries);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Load(PRLibrary **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = PR_LoadLibrary(mPath);
    if (!*_retval)
        return NS_ERROR_FAILURE;
    return NS_OK;
}
