/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the new Mozilla toolkit.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <bsmedberg@covad.net>
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "prclist.h"

#ifdef XP_WIN
#include <windows.h>
#include <shlobj.h>
#endif
#ifdef XP_BEOS
#include <Path.h>
#endif
#ifdef XP_MACOSX
#include <CFURL.h>
#endif
#ifdef XP_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "prnetdb.h"
#include "prsystem.h"
#include "prprf.h"
#endif

#include "nsIToolkitProfileService.h"
#include "nsIToolkitProfile.h"
#include "nsIFactory.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"

#include "nsEmbedString.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"

#include "nsINIParser.h"
#include "nsXREDirProvider.h"
#include "nsAppRunner.h"



class nsToolkitProfile : public nsIToolkitProfile
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITOOLKITPROFILE

    friend class nsToolkitProfileService;
    nsCOMPtr<nsToolkitProfile> mNext;
    nsToolkitProfile          *mPrev;

private:
    nsToolkitProfile(const nsACString& aName, nsILocalFile* aFile,
                     nsToolkitProfile* aPrev);
    ~nsToolkitProfile() { }

    friend class nsProfileLock;

    nsEmbedCString             mName;
    nsCOMPtr<nsILocalFile>     mFile;
    nsIProfileLock*            mLock;
};

class nsProfileLock : public nsIProfileLock
#if defined (XP_UNIX)
                     ,protected PRCList
#endif
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROFILELOCK

    nsresult Init(nsToolkitProfile* aProfile);
    nsresult Init(nsILocalFile* aDirectory);

    nsProfileLock();

private:
    ~nsProfileLock();
    void Unlock();

    nsCOMPtr<nsToolkitProfile> mProfile;
    nsCOMPtr<nsILocalFile> mDirectory;

#if defined (XP_WIN)
    HANDLE         mLockFileHandle;
#elif defined (XP_OS2)
    LHANDLE        mLockFileHandle;
#else
    static void    RemovePidLockFiles();
    static void    FatalSignalHandler(int signo);

    nsresult       LockWithFcntl(const nsEmbedCString& lockFilePath);
    nsresult       LockWithSymlink(const nsEmbedCString& lockFilePath);

    nsEmbedCString mPidLockFileName;
    int            mLockFileDesc;
#endif
};

class nsToolkitProfileService : public nsIToolkitProfileService,
                                public nsIFactory
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITOOLKITPROFILESERVICE

    // we implement nsIFactory because we can't be registered by location,
    // like most ordinary components are. Instead, during startup we register
    // our factory. Then we return our singleton-self when asked.
    NS_DECL_NSIFACTORY

private:
    friend class nsToolkitProfile;
    friend nsresult NS_NewToolkitProfileService(nsIToolkitProfileService**);

    nsToolkitProfileService() :
        mDirty(PR_FALSE),
        mStartWithLast(PR_TRUE)
    {
        gService = this;
    }
    ~nsToolkitProfileService()
    {
        gService = nsnull;
    }

    NS_HIDDEN_(nsresult) Init();

    nsCOMPtr<nsToolkitProfile> mFirst;
    nsCOMPtr<nsToolkitProfile> mChosen;
    nsCOMPtr<nsILocalFile>     mAppData;
    nsCOMPtr<nsILocalFile>     mListFile;
    PRBool mDirty;
    PRBool mStartWithLast;

    static NS_HIDDEN_(nsresult) GetAppDataDir(nsILocalFile* *aResult);

    static nsToolkitProfileService *gService;

    class ProfileEnumerator : public nsISimpleEnumerator
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSISIMPLEENUMERATOR

        ProfileEnumerator(nsToolkitProfile *first)
          { mCurrent = first; }
    private:
        ~ProfileEnumerator() { }
        nsCOMPtr<nsToolkitProfile> mCurrent;
    };
};

nsToolkitProfile::nsToolkitProfile(const nsACString& aName, nsILocalFile* aFile,
                                   nsToolkitProfile* aPrev) :
    mPrev(aPrev),
    mName(aName),
    mFile(aFile),
    mLock(nsnull)
{
    NS_ASSERTION(aFile, "No file!");

    if (aPrev)
        aPrev->mNext = this;
    else
        nsToolkitProfileService::gService->mFirst = this;
}

NS_IMPL_ISUPPORTS1(nsToolkitProfile, nsIToolkitProfile)

NS_IMETHODIMP
nsToolkitProfile::GetRootDir(nsILocalFile* *aResult)
{
    NS_ADDREF(*aResult = mFile);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfile::GetName(nsACString& aResult)
{
    NS_CStringCopy(aResult, mName);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfile::SetName(const nsACString& aName)
{
    NS_ASSERTION(nsToolkitProfileService::gService,
                 "Where did my service go?");

    const char* stringdata;
    PRUint32 length = NS_CStringGetData(aName, &stringdata);

    /* we only allow profile names in ASCII, because we don't have a good way to
       convert from native charsets to unicode without starting up xpcom */
    for (const char* end = stringdata + length; stringdata < end; ++stringdata) {
        if (*stringdata & 0x80)
            return NS_ERROR_INVALID_ARG;
    }

    mName = aName;
    nsToolkitProfileService::gService->mDirty = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfile::Remove(PRBool removeFiles)
{
    NS_ASSERTION(nsToolkitProfileService::gService,
                 "Whoa, my service is gone.");

    if (mLock)
        return NS_ERROR_FILE_IS_LOCKED;

    if (removeFiles)
        mFile->Remove(PR_TRUE);

    if (mPrev)
        mPrev->mNext = mNext;
    else
        nsToolkitProfileService::gService->mFirst = mNext;

    if (mNext)
        mNext->mPrev = mPrev;

    mPrev = nsnull;
    mNext = nsnull;

    if (nsToolkitProfileService::gService->mChosen == this)
        nsToolkitProfileService::gService = nsnull;

    nsToolkitProfileService::gService->mDirty = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfile::Lock(nsIProfileLock* *aResult)
{
    if (mLock) {
        NS_ADDREF(*aResult = mLock);
        return NS_OK;
    }

    nsCOMPtr<nsProfileLock> lock = new nsProfileLock();
    if (!lock) return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = lock->Init(this);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = lock);
    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsProfileLock, nsIProfileLock)

nsProfileLock::nsProfileLock() :
#if defined (XP_WIN)
    mLockFileHandle(INVALID_HANDLE_VALUE)
#elif defined (XP_OS2)
    mLockFileHandle(-1)
#elif defined (XP_UNIX)
    mLockFileDesc(-1)
#endif
{
#if defined XP_UNIX
    next = prev = this;
#endif
}

#if defined (XP_UNIX)
#define OLD_LOCKFILE_NAME "lock"
#define LOCKFILE_NAME     ".parentlock"
#else
#define LOCKFILE_NAME     "parent.lock"
#endif

nsresult
nsProfileLock::Init(nsToolkitProfile* aProfile)
{
    nsresult rv;
    rv = Init(aProfile->mFile);
    if (NS_SUCCEEDED(rv))
        mProfile = aProfile;

    return rv;
}

nsresult
nsProfileLock::Init(nsILocalFile* aDirectory)
{
    nsresult rv;

    PRBool isDir;
    rv = aDirectory->IsDirectory(&isDir);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isDir)
        return NS_ERROR_FILE_NOT_DIRECTORY;

    nsCOMPtr<nsIFile> lockFile;
    rv = aDirectory->Clone(getter_AddRefs(lockFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lockFile->AppendNative(nsEmbedCString(LOCKFILE_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX)
    // First, try locking using fcntl. It is more reliable on
    // a local machine, but may not be supported by an NFS server.
    nsEmbedCString filePath;
    rv = lockFile->GetNativePath(filePath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = LockWithFcntl(filePath);
    if (NS_FAILED(rv) && (rv != NS_ERROR_FILE_ACCESS_DENIED))
    {
        // If that failed for any reason other than NS_ERROR_FILE_ACCESS_DENIED,
        // assume we tried an NFS that does not support it. Now, try with symlink.
        rv = LockWithSymlink(filePath);
    }
#elif defined(XP_WIN)
    nsEmbedCString filePath;
    rv = lockFile->GetNativePath(filePath);
    NS_ENSURE_SUCCESS(rv, rv);

    mLockFileHandle = CreateFile(filePath.get(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0, // no sharing - of course
                                 nsnull,
                                 OPEN_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 nsnull);
    if (mLockFileHandle == INVALID_HANDLE_VALUE)
        return NS_ERROR_FILE_ACCESS_DENIED;
#elif defined(XP_OS2)
    nsEmbedCString filePath;
    rv = lockFile->GetNativePath(filePath);
    NS_ENSURE_SUCCESS(rv, rv);

    ULONG   ulAction = 0;
    APIRET  rc;
    rc = DosOpen(filePath.get(),
                  &mLockFileHandle,
                  &ulAction,
                  0,
                  FILE_NORMAL,
                  OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                  OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE | OPEN_FLAGS_NOINHERIT,
                  0 );
    if (rc != NO_ERROR)
    {
        mLockFileHandle = -1;
        return NS_ERROR_FILE_ACCESS_DENIED;
    }
#elif defined(VMS)
    nsEmbedCString filePath;
    rv = lockFile->GetNativePath(filePath);
    NS_ENSURE_SUCCESS(rv, rv);

    mLockFileDesc = open_noshr(filePath.get(), O_CREAT, 0666);
    if (mLockFileDesc == -1)
    {
	if ((errno == EVMSERR) && (vaxc$errno == RMS$_FLK))
	{
	    return NS_ERROR_FILE_ACCESS_DENIED;
	}
	else
	{
	    NS_ERROR("Failed to open lock file.");
	    return NS_ERROR_FAILURE;
	}
    }
#else
    nsCOMPtr<nsIFile> oldLockFile;
    rv = aDirectory->Clone(getter_AddRefs(oldLockFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = oldLockFile->AppendNative(nsEmbedCString(OLD_LOCKFILE_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

    nsEmbedCString filePath;
    rv = oldLockFile->GetNativePath(filePath);
    NS_ENSURE_SUCCESS(rv, rv);

    // First, try the 4.x-compatible symlink technique, which works
    // with NFS without depending on (broken or missing, too often)
    // lockd.
    rv = LockWithSymlink(filePath);
    if (rv == NS_ERROR_FILE_ACCESS_DENIED)
        return rv;

    if (NS_FAILED(rv))
    {
        // If that failed with an error other than
        // NS_ERROR_FILE_ACCESS_DENIED, symlinks aren't
        // supported (for example, on Win32 SAMBA servers).
        // Try locking with fcntl.
        rv = lockFile->GetNativePath(filePath);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = LockWithFcntl(filePath);
        NS_ENSURE_SUCCESS(rv, rv);
    }
#endif /* XP_UNIX */

    // We made it!
    mDirectory = aDirectory;
    return NS_OK;
}

NS_IMETHODIMP
nsProfileLock::GetDirectory(nsILocalFile* *aResult)
{
    NS_ASSERTION(mDirectory, "Not initialized!");
    NS_ADDREF(*aResult = mDirectory);
    return NS_OK;
}

nsProfileLock::~nsProfileLock()
{
    Unlock();
}

void
nsProfileLock::Unlock()
{
    if (mDirectory) {
#if defined (XP_WIN)
        if (mLockFileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mLockFileHandle);
            mLockFileHandle = INVALID_HANDLE_VALUE;
        }
#elif defined (XP_OS2)
        if (mLockFileHandle != -1)
        {
            DosClose(mLockFileHandle);
            mLockFileHandle = -1;
        }
#elif defined (XP_UNIX)
        if (mPidLockFileName.Length())
        {
            PR_REMOVE_LINK(this);
            (void) unlink(mPidLockFileName.get());
        }
        else if (mLockFileDesc != -1)
        {
            close(mLockFileDesc);
            mLockFileDesc = -1;
            // Don't remove it
        }
#endif

        if (mProfile) {
            mProfile->mLock = nsnull;
            mProfile = nsnull;
        }
        mDirectory = nsnull;
    }
}

#ifdef XP_UNIX
static PRBool sSetupPidLockCleanup = PR_FALSE;

static PRCList sPidLockList =
    PR_INIT_STATIC_CLIST(&sPidLockList);

void
nsProfileLock::RemovePidLockFiles()
{
    while (!PR_CLIST_IS_EMPTY(&sPidLockList))
    {
        nsProfileLock *lock = NS_STATIC_CAST(nsProfileLock*, sPidLockList.next);
        lock->Unlock();
    }
}

static struct sigaction SIGHUP_oldact;
static struct sigaction SIGINT_oldact;
static struct sigaction SIGQUIT_oldact;
static struct sigaction SIGILL_oldact;
static struct sigaction SIGABRT_oldact;
static struct sigaction SIGSEGV_oldact;
static struct sigaction SIGTERM_oldact;

void
nsProfileLock::FatalSignalHandler(int signo)
{
    // Remove any locks still held.
    RemovePidLockFiles();

    // Chain to the old handler, which may exit.
    struct sigaction *oldact = nsnull;

    switch (signo) {
      case SIGHUP:
        oldact = &SIGHUP_oldact;
        break;
      case SIGINT:
        oldact = &SIGINT_oldact;
        break;
      case SIGQUIT:
        oldact = &SIGQUIT_oldact;
        break;
      case SIGILL:
        oldact = &SIGILL_oldact;
        break;
      case SIGABRT:
        oldact = &SIGABRT_oldact;
        break;
      case SIGSEGV:
        oldact = &SIGSEGV_oldact;
        break;
      case SIGTERM:
        oldact = &SIGTERM_oldact;
        break;
      default:
        NS_NOTREACHED("bad signo");
        break;
    }

    if (oldact) {
        if (oldact->sa_handler == SIG_DFL) {
            // Make sure the default sig handler is executed
            // We need it to get Mozilla to dump core.
            sigaction(signo,oldact,NULL);

            // Now that we've restored the default handler, unmask the
            // signal and invoke it.

            sigset_t unblock_sigs;
            sigemptyset(&unblock_sigs);
            sigaddset(&unblock_sigs, signo);

            sigprocmask(SIG_UNBLOCK, &unblock_sigs, NULL);

            raise(signo);
        }
        else if (oldact->sa_handler && oldact->sa_handler != SIG_IGN)
        {
            oldact->sa_handler(signo);
        }
    }

    // Backstop exit call, just in case.
    _exit(signo);
}

nsresult
nsProfileLock::LockWithFcntl(const nsEmbedCString& lockFilePath)
{
  mLockFileDesc = open(lockFilePath.get(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (mLockFileDesc == -1) {
      NS_WARNING("Couldn't open lock file.");
      return NS_ERROR_FAILURE;
  }
      
  struct flock lock;
  lock.l_start = 0;
  lock.l_len = 0; // len = 0 means entire file
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  if (fcntl(mLockFileDesc, F_SETLK, &lock) == -1)
  {
      close(mLockFileDesc);
      mLockFileDesc = -1;

      // With OS X, on NFS, errno == ENOTSUP
      // XXX Check for that and return specific rv for it?
#ifdef DEBUG
      printf("fcntl(F_SETLK) failed. errno = %d\n", errno);
#endif
      if (errno == EAGAIN || errno == EACCES)
          return NS_ERROR_FILE_ACCESS_DENIED;

      return NS_ERROR_FAILURE;                
  }
  return NS_OK;
}

nsresult
nsProfileLock::LockWithSymlink(const nsEmbedCString& lockFilePath)
{
  nsresult rv;
  
  struct in_addr inaddr;
  inaddr.s_addr = INADDR_LOOPBACK;

  char hostname[256];
  PRStatus status = PR_GetSystemInfo(PR_SI_HOSTNAME, hostname, sizeof hostname);
  if (status == PR_SUCCESS)
  {
      char netdbbuf[PR_NETDB_BUF_SIZE];
      PRHostEnt hostent;
      status = PR_GetHostByName(hostname, netdbbuf, sizeof netdbbuf, &hostent);
      if (status == PR_SUCCESS)
          memcpy(&inaddr, hostent.h_addr, sizeof inaddr);
  }

  char *signature =
      PR_smprintf("%s:%lu", inet_ntoa(inaddr), (unsigned long)getpid());
  int symlink_rv, symlink_errno, tries = 0;

  // use ns4.x-compatible symlinks if the FS supports them
  while ((symlink_rv = symlink(signature, lockFilePath.get())) < 0)
  {
      symlink_errno = errno;
      if (symlink_errno != EEXIST)
          break;

      // the link exists; see if it's from this machine, and if
      // so if the process is still active
      char buf[1024];
      int len = readlink(lockFilePath.get(), buf, sizeof buf - 1);
      if (len > 0)
      {
          buf[len] = '\0';
          char *colon = strchr(buf, ':');
          if (colon)
          {
              *colon++ = '\0';
              unsigned long addr = inet_addr(buf);
              if (addr != (unsigned long) -1)
              {
                  char *after = nsnull;
                  pid_t pid = strtol(colon, &after, 0);
                  if (pid != 0 && *after == '\0')
                  {
                      if (addr != inaddr.s_addr)
                      {
                          // Remote lock: give up even if stuck.
                          break;
                      }

                      // kill(pid,0) is a neat trick to check if a
                      // process exists
                      if (kill(pid, 0) == 0 || errno != ESRCH)
                      {
                          // Local process appears to be alive, ass-u-me it
                          // is another Mozilla instance, or a compatible
                          // derivative, that's currently using the profile.
                          // XXX need an "are you Mozilla?" protocol
                          break;
                      }
                  }
              }
          }
      }

      // Lock seems to be bogus: try to claim it.  Give up after a large
      // number of attempts (100 comes from the 4.x codebase).
      (void) unlink(lockFilePath.get());
      if (++tries > 100)
          break;
  }

  PR_smprintf_free(signature);
  signature = nsnull;

  if (symlink_rv == 0)
  {
      // We exclusively created the symlink: record its name for eventual
      // unlock-via-unlink.
      rv = NS_OK;
      mPidLockFileName = lockFilePath.get();
      PR_APPEND_LINK(this, &sPidLockList);
      if (!sSetupPidLockCleanup)
      {
          // Clean up on normal termination.
          atexit(RemovePidLockFiles);

          // Clean up on abnormal termination, using POSIX sigaction.
          // Don't arm a handler if the signal is being ignored, e.g.,
          // because mozilla is run via nohup.
          struct sigaction act, oldact;
          act.sa_handler = FatalSignalHandler;
          act.sa_flags = 0;
          sigfillset(&act.sa_mask);

#define CATCH_SIGNAL(signame)                                           \
PR_BEGIN_MACRO                                                          \
if (sigaction(signame, NULL, &oldact) == 0 &&                         \
    oldact.sa_handler != SIG_IGN)                                     \
{                                                                     \
    sigaction(signame, &act, &signame##_oldact);                      \
}                                                                     \
PR_END_MACRO

          CATCH_SIGNAL(SIGHUP);
          CATCH_SIGNAL(SIGINT);
          CATCH_SIGNAL(SIGQUIT);
          CATCH_SIGNAL(SIGILL);
          CATCH_SIGNAL(SIGABRT);
          CATCH_SIGNAL(SIGSEGV);
          CATCH_SIGNAL(SIGTERM);

#undef CATCH_SIGNAL

          sSetupPidLockCleanup = PR_TRUE;
      }
  }
  else if (symlink_errno == EEXIST)
      rv = NS_ERROR_FILE_ACCESS_DENIED;
  else
  {
#ifdef DEBUG
      printf("symlink() failed. errno = %d\n", errno);
#endif
      rv = NS_ERROR_FAILURE;
  }
  return rv;
}

#endif

nsToolkitProfileService*
nsToolkitProfileService::gService = nsnull;

NS_IMPL_ISUPPORTS2(nsToolkitProfileService,
                   nsIToolkitProfileService,
                   nsIFactory)

nsresult
nsToolkitProfileService::Init()
{
    nsresult rv;

    rv = GetAppDataDir(getter_AddRefs(mAppData));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> listFile;
    rv = mAppData->Clone(getter_AddRefs(listFile));
    NS_ENSURE_SUCCESS(rv, rv);

    mListFile = do_QueryInterface(listFile);
    NS_ENSURE_TRUE(listFile, NS_ERROR_NO_INTERFACE);

    rv = mListFile->AppendNative(nsEmbedCString("profiles.ini"));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool exists;
    rv = mListFile->IsFile(&exists);
    if (NS_FAILED(rv) || !exists) {
        return NS_OK;
    }

    nsINIParser parser;
    rv = parser.Init(mListFile);
    // Parsing errors are troublesome... we're gonna continue even on
    // parsing errors, and let people manually re-locate their profile
    // if something goes wacky

    char parserBuf[MAXPATHLEN];
    rv = parser.GetString("General", "StartWithLastProfile", parserBuf, MAXPATHLEN);
    if (NS_SUCCEEDED(rv) && strcmp("0", parserBuf) == 0)
        mStartWithLast = PR_FALSE;

    nsToolkitProfile* currentProfile = nsnull;
    nsEmbedCString filePath;

    unsigned int c = 0;
    for (c = 0; PR_TRUE; ++c) {
        char profileID[12];
        sprintf(profileID, "Profile%u", c);

        rv = parser.GetString(profileID, "IsRelative", parserBuf, MAXPATHLEN);
        if (NS_FAILED(rv)) break;

        PRBool isRelative = (strcmp(parserBuf, "1") == 0);

        rv = parser.GetString(profileID, "Path", parserBuf, MAXPATHLEN);
        if (NS_FAILED(rv)) {
            NS_ERROR("Malformed profiles.ini: Path= not found");
            continue;
        }

        filePath = parserBuf;

        rv = parser.GetString(profileID, "Name", parserBuf, MAXPATHLEN);
        if (NS_FAILED(rv)) {
            NS_ERROR("Malformed profiles.ini: Name= not found");
            continue;
        }

        nsCOMPtr<nsILocalFile> rootDir;
        rv = NS_NewNativeLocalFile(nsEmbedCString(), PR_TRUE,
                                   getter_AddRefs(rootDir));
        NS_ENSURE_SUCCESS(rv, rv);

        if (isRelative) {
            rv = rootDir->SetRelativeDescriptor(mAppData, filePath);
        } else {
            rv = rootDir->SetPersistentDescriptor(filePath);
        }
        if (NS_FAILED(rv)) continue;

        currentProfile = new nsToolkitProfile(nsEmbedCString(parserBuf), rootDir,
                                              currentProfile);
        NS_ENSURE_TRUE(currentProfile, NS_ERROR_OUT_OF_MEMORY);

        rv = parser.GetString(profileID, "Default", parserBuf, MAXPATHLEN);
        if (NS_SUCCEEDED(rv) && strcmp("1", parserBuf) == 0)
            mChosen = currentProfile;
    }

    return NS_OK;
}

nsresult
nsToolkitProfileService::GetAppDataDir(nsILocalFile* *aResult)
{
    NS_ASSERTION(gAppData, "Whoops, no gAppData");

#ifdef XP_MACOSX
    FSRef fsRef;
    OSErr err = ::FSFindFolder(kUserDomain, kDomainLibraryFolderType, kCreateFolder, &fsRef);
    if (err) return NS_ERROR_FAILURE;

    nsCOMPtr<nsILocalFile> dirFile;
    nsresult rv = NS_NewNativeLocalFile(nsEmbedCString(), PR_TRUE,
                                        getter_AddRefs(dirFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsILocalFileMac> dirFileMac = do_QueryInterface(dirFile);
    NS_ENSURE_TRUE(dirFileMac, NS_ERROR_UNEXPECTED);

    rv = dirFileMac->InitWithFSRef(fsRef);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = dirFileMac->AppendNative(nsEmbedCString(gAppData->appName));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aResult = dirFile);
    return NS_OK;
#elif defined(XP_WIN)
    LPMALLOC pMalloc;
    LPITEMIDLIST pItemIDList = NULL;

    if (!SUCCEEDED(SHGetMalloc(&pMalloc)))
        return NS_ERROR_OUT_OF_MEMORY;

    char appDataPath[MAXPATHLEN];

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pItemIDList)) &&
        SUCCEEDED(SHGetPathFromIDList(pItemIDList, appDataPath))) {
    } else {
        if (!GetWindowsDirectory(appDataPath, MAXPATHLEN)) {
            NS_WARNING("Aaah, no windows directory!");
            return NS_ERROR_FAILURE;
        }
    }

    if (pItemIDList) pMalloc->Free(pItemIDList);
    pMalloc->Release();

    nsCOMPtr<nsILocalFile> lf;
    nsresult rv = NS_NewNativeLocalFile(nsEmbedCString(appDataPath),
                                        PR_TRUE, getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lf->AppendNative(nsEmbedCString(gAppData->appName));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aResult = lf);
    return NS_OK;

#elif defined(XP_OS2)
    // we want an environment variable of the form
    // FIREFOX_HOME, etc
    char envVar[100];
    char *writing = envVar;
    const char *reading = gAppData->appName;

    while (*reading) {
        *writing = toupper(*reading);
        ++writing; ++reading;
    }
    static const char kSuffix[] = "_HOME";
    memcpy(writing, kSuffix, sizeof(kSuffix));
    
    char *pHome = getenv(envVar);
    if (pHome && *pHome) {
        return NS_NewNativeLocalFile(nsEmbedCString(pHome), PR_TRUE,
                                     aResult);
    }

    PPID ppid;
    PTIB ptib;
    char appDir[CCHMAXPATH];

    DosGetInfoBlocks(&ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte, CCHMAXPATH, appDir);
    *strrchr(appDir, '\\') = '\0';
    return NS_NewNativeLocalFile(nsEmbedCString(appDir), PR_TRUE, aResult);

#elif defined(XP_BEOS)
    char appDir[MAXPATHLEN];
    if (find_directory(B_USER_DIRECTORY, NULL, true, appDir, MAXPATHLEN))
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsILocalFile> lf;
    nsresult rv = NS_NewNativeLocalFile(nsEmbedCString(appDir), PR_TRUE,
                                        getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = lf->AppendNative(nsEmbedCString(gAppData->appName));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aResult = lf);
    return NS_OK;

#elif defined(XP_UNIX)
    const char* homeDir = getenv("HOME");
    if (!homeDir || !*homeDir)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsILocalFile> lf;
    nsresult rv = NS_NewNativeLocalFile(nsEmbedCString(homeDir), PR_TRUE,
                                        getter_AddRefs(lf));
    NS_ENSURE_SUCCESS(rv, rv);

    char appname[MAXPATHLEN] = ".";

    const char *reading = gAppData->appName;
    char *writing = appname + 1;

    while (*reading) {
        *writing = tolower(*reading);
        ++writing; ++reading;
    }
    *writing = '\0';

    rv = lf->AppendNative(nsEmbedCString(appname));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aResult = lf);
    return NS_OK;

#else
#error Need platform-specific code here.
#endif
}

NS_IMETHODIMP
nsToolkitProfileService::SetStartWithLastProfile(PRBool aValue)
{
    if (mStartWithLast != aValue) {
        mStartWithLast = aValue;
        mDirty = PR_TRUE;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::GetStartWithLastProfile(PRBool *aResult)
{
    *aResult = mStartWithLast;
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::GetProfiles(nsISimpleEnumerator* *aResult)
{
    *aResult = new ProfileEnumerator(this->mFirst);
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsToolkitProfileService::ProfileEnumerator,
                   nsISimpleEnumerator)

NS_IMETHODIMP
nsToolkitProfileService::ProfileEnumerator::HasMoreElements(PRBool* aResult)
{
    *aResult = mCurrent ? PR_TRUE : PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::ProfileEnumerator::GetNext(nsISupports* *aResult)
{
    if (!mCurrent) return NS_ERROR_FAILURE;

    NS_ADDREF(*aResult = mCurrent);

    mCurrent = mCurrent->mNext;
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::GetSelectedProfile(nsIToolkitProfile* *aResult)
{
    if (!mChosen && mFirst && !mFirst->mNext) // only one profile
        mChosen = mFirst;

    if (!mChosen) return NS_ERROR_FAILURE;

    NS_ADDREF(*aResult = mChosen);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::SetSelectedProfile(nsIToolkitProfile* aProfile)
{
    if (mChosen != aProfile) {
        mChosen = aProfile;
        mDirty = PR_TRUE;
    }
    return NS_OK;
}

static PRBool
CStringsEqual(const nsACString& str1, const nsACString& str2)
{
    PRUint32 length1, length2;
    const char *data1, *data2;
    length1 = NS_CStringGetData(str1, &data1);
    length2 = NS_CStringGetData(str2, &data2);

    if (length1 != length2) return PR_FALSE;

    while (length1) {
        if (*data1 != *data2) return PR_FALSE;
        ++data1; ++data2;
        --length1;
    }
    return PR_TRUE;
}

NS_IMETHODIMP
nsToolkitProfileService::GetProfileByName(const nsACString& aName,
                                          nsIToolkitProfile* *aResult)
{
    nsToolkitProfile* curP = mFirst;
    while (curP) {
        if (CStringsEqual(curP->mName, aName)) {
            NS_ADDREF(*aResult = curP);
            return NS_OK;
        }
        curP = curP->mNext;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsToolkitProfileService::LockProfilePath(nsILocalFile* aDirectory,
                                         nsIProfileLock* *aResult)
{
    return NS_LockProfilePath(aDirectory, aResult);
}

nsresult
NS_LockProfilePath(nsILocalFile* aPath, nsIProfileLock* *aResult)
{
    nsCOMPtr<nsProfileLock> lock = new nsProfileLock();
    if (!lock) return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = lock->Init(aPath);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = lock);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::CreateProfile(nsILocalFile* aRootDir,
                                       const nsACString& aName,
                                       nsIToolkitProfile* *aResult)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> rootDir = aRootDir;

    if (!rootDir) {
        rv = GetAppDataDir(getter_AddRefs(rootDir));
        NS_ENSURE_SUCCESS(rv, rv);

        rootDir->AppendNative(aName);
    }

    PRBool exists;
    rv = rootDir->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (exists) {
        rv = rootDir->IsDirectory(&exists);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!exists)
            return NS_ERROR_FILE_NOT_DIRECTORY;
    }
    else {
        nsCOMPtr<nsIFile> profileDefaultsDir;
        nsCOMPtr<nsIFile> profileDirParent;
        nsEmbedCString profileDirName;

        rv = rootDir->GetParent(getter_AddRefs(profileDirParent));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = rootDir->GetNativeLeafName(profileDirName);
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool dummy;
        rv = gDirServiceProvider->GetFile(NS_APP_PROFILE_DEFAULTS_50_DIR, &dummy,
                                          getter_AddRefs(profileDefaultsDir));

        if (NS_SUCCEEDED(rv))
            rv = profileDefaultsDir->CopyToNative(profileDirParent,
                                                  profileDirName);
        if (NS_FAILED(rv)) {
            // if copying failed, lets just ensure that the profile directory exists.
            rv = rootDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
            NS_ENSURE_SUCCESS(rv, rv);
        }
      
        rv = rootDir->SetPermissions(0700);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsToolkitProfile* last = mFirst;
    if (last) {
        while (last->mNext)
            last = last->mNext;
    }

    nsCOMPtr<nsIToolkitProfile> profile =
        new nsToolkitProfile(aName, rootDir, last);
    if (!profile) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = profile);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::GetProfileCount(PRUint32 *aResult)
{
    if (!mFirst)
        *aResult = 0;
    else if (! mFirst->mNext)
        *aResult = 1;
    else
        *aResult = 2;

    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::Flush()
{
    // Errors during writing might cause unhappy semi-written files.
    // To avoid this, write the entire thing to a buffer, then write
    // that buffer to disk.

    nsresult rv;
    PRUint32 pCount = 0;
    nsToolkitProfile *cur;

    for (cur = mFirst; cur != nsnull; cur = cur->mNext)
        ++pCount;

    PRUint32 length;

    char* buffer = (char*) malloc(100 + MAXPATHLEN * pCount);
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

    char *end = buffer;

    end += sprintf(end,
                   "[General]\n"
                   "StartWithLastProfile=%s\n\n",
                   mStartWithLast ? "1" : "0");

    nsEmbedCString path;
    cur = mFirst;
    pCount = 0;

    while (cur) {
        // if the profile dir is relative to appdir...
        PRBool isRelative;
        rv = mAppData->Contains(cur->mFile, PR_TRUE, &isRelative);
        if (NS_SUCCEEDED(rv) && isRelative) {
            // we use a relative descriptor
            rv = cur->mFile->GetRelativeDescriptor(mAppData, path);
        } else {
            // otherwise, a persistent descriptor
            rv = cur->mFile->GetPersistentDescriptor(path);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        end += sprintf(end,
                       "[Profile%u]\n"
                       "Name=%s\n"
                       "IsRelative=%s\n"
                       "Path=%s\n",
                       pCount, cur->mName.get(),
                       isRelative ? "1" : "0", path.get());

        if (mChosen == cur) {
            end += sprintf(end, "Default=1\n");
        }

        end += sprintf(end, "\n");

        cur = cur->mNext;
        ++pCount;
    }

    FILE* writeFile;
    rv = mListFile->OpenANSIFileDesc("w", &writeFile);
    NS_ENSURE_SUCCESS(rv, rv);

    if (buffer) {
        length = end - buffer;

        if (fwrite(buffer, sizeof(char), length, writeFile) != length) {
            fclose(writeFile);
            return NS_ERROR_UNEXPECTED;
        }
    }

    fclose(writeFile);
    return NS_OK;
}

NS_IMETHODIMP
nsToolkitProfileService::CreateInstance(nsISupports* aOuter, const nsID& aIID,
                                        void** aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    // return this object
    return QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsToolkitProfileService::LockFactory(PRBool aVal)
{
    return NS_OK;
}

nsresult
NS_NewToolkitProfileService(nsIToolkitProfileService* *aResult)
{
    nsToolkitProfileService* aThis = new nsToolkitProfileService();
    nsresult rv = aThis->Init();
    if (NS_FAILED(rv)) {
        NS_ERROR("nsToolkitProfileService::Init failed!");
        delete aThis;
        return rv;
    }

    NS_ADDREF(*aResult = aThis);
    return NS_OK;
}

nsresult
NS_GetFileFromPath(const char *aPath, nsILocalFile* *aResult)
{
#if defined(XP_MACOSX)
    PRInt32 pathLen = strlen(aPath);
    if (pathLen > MAXPATHLEN)
        return NS_ERROR_INVALID_ARG;

    CFURL *fullPath =
        CFURLCreateFromFileSystemRepresentation(NULL, aPath, pathLen, true);
    if (!fullPath)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsILocalFile> lf;
    nsresult rv = NS_NewNativeLocalFile(nsEmbedCString(), PR_TRUE,
                                        getter_AddRefs(lf));
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocalFileMac> lfMac = do_QueryInterface(lf, &rv);
        if (NS_SUCCEEDED(rv)) {
            rv = lfMac->InitWithCFURL(fullPath);
            if (NS_SUCCEEDED(rv))
                NS_ADDREF(*aResult = lf);
        }
    }
    CFRelease(fullPath);
    return rv;

#elif defined(XP_UNIX) || defined(XP_OS2)
    char fullPath[MAXPATHLEN];

    if (!realpath(aPath, fullPath))
        return NS_ERROR_FAILURE;

    return NS_NewNativeLocalFile(nsEmbedCString(fullPath), PR_TRUE,
                                 aResult);

#elif defined(XP_WIN)
    char fullPath[MAXPATHLEN];

    if (!_fullpath(fullPath, aPath, MAXPATHLEN))
        return NS_ERROR_FAILURE;

    return NS_NewNativeLocalFile(nsEmbedCString(fullPath), PR_TRUE,
                                 aResult);

#elif defined(XP_BEOS)
    BPath fullPath;
    if (fullPath.SetTo(aPath, NULL, true))
        return NS_ERROR_FAILURE;

    return NS_NewNativeLocalFile(nsEmbedCString(fullPath.Leaf()), PR_TRUE,
                                 aResult);

#else
#error Platform-specific logic needed here.
#endif
}
