/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
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
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//	This file is included by nsFileSpec.cp, and includes the Windows-specific
//	implementations.

#if !defined(WINCE)
#include <sys/stat.h>
#include <direct.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include "prio.h"
#include "nsError.h"
#include "prlog.h"

#include "windows.h"

#if (_MSC_VER == 1100)
#define INITGUID
#include "objbase.h"
DEFINE_OLEGUID(IID_IPersistFile, 0x0000010BL, 0, 0);
#endif

#include "shlobj.h"
#include "shellapi.h"
#include "shlguid.h"

#ifdef UNICODE
#define CreateDirectoryW  CreateDirectory
#else
#define CreateDirectoryA  CreateDirectory
#endif 

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)
// Canonify, make absolute, and check whether directories exist. This
// takes a (possibly relative) native path and converts it into a
// fully qualified native path.
//----------------------------------------------------------------------------------------
{
    if (ioPath.IsEmpty())
        return;
  
    NS_ASSERTION(strchr((const char*)ioPath, '/') == 0,
		"This smells like a Unix path. Native path expected! "
		"Please fix.");
	if (inMakeDirs)
    {
        const int mode = 0755;
        nsSimpleCharString unixStylePath = ioPath;
        nsFileSpecHelpers::NativeToUnix(unixStylePath);
        nsFileSpecHelpers::MakeAllDirectories((const char*)unixStylePath, mode);
    }

    //
    // WINCE uses full paths all the time.
    // This may break stuff.
    //
#if !defined(WINCE)
    char buffer[_MAX_PATH];
    errno = 0;
    *buffer = '\0';
    char* canonicalPath = _fullpath(buffer, ioPath, _MAX_PATH);

	if (canonicalPath)
	{
		NS_ASSERTION( canonicalPath[0] != '\0', "Uh oh...couldn't convert" );
		if (canonicalPath[0] == '\0')
			return;
	}
    ioPath = canonicalPath;
#endif
} // nsFileSpecHelpers::Canonify

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::UnixToNative(nsSimpleCharString& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything
//----------------------------------------------------------------------------------------
{
	// Allow for relative or absolute.  We can do this in place, because the
	// native path is never longer.
	
	if (ioPath.IsEmpty())
		return;
		
  // Strip initial slash for an absolute path
	char* src = (char*)ioPath;
  if (*src == '/') {
    if (PL_strlen(src+1)==0) {
      // allocate new string by copying from ioPath[1]
      nsSimpleCharString temp = src + 1;
      ioPath = temp;
      return;
    }
	  // Since it was an absolute path, check for the drive letter
		char* colonPointer = src + 2;
		if (strstr(src, "|/") == colonPointer)
	    *colonPointer = ':';
	  // allocate new string by copying from ioPath[1]
	  nsSimpleCharString temp = src + 1;
	  ioPath = temp;
	}

	src = (char*)ioPath;
		
    if (*src) {
	    // Convert '/' to '\'.
	    while (*++src)
        {
            if (*src == '/')
                *src = '\\';
        }
    }
} // nsFileSpecHelpers::UnixToNative

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::NativeToUnix(nsSimpleCharString& ioPath)
// This just does string manipulation.  It doesn't check reality, or canonify, or
// anything.  The unix path is longer, so we can't do it in place.
//----------------------------------------------------------------------------------------
{
	if (ioPath.IsEmpty())
		return;
		
	// Convert the drive-letter separator, if present
	nsSimpleCharString temp("/");

	char* cp = (char*)ioPath + 1;
	if (strstr(cp, ":\\") == cp)
		*cp = '|';    // absolute path
    else
        temp[0] = '\0'; // relative path
	
	// Convert '\' to '/'
	for (; *cp; cp++)
    {
      if(IsDBCSLeadByte(*cp) && *(cp+1) != nsnull)
      {
         cp++;
         continue;
      }
      if (*cp == '\\')
        *cp = '/';
    }
	// Add the slash in front.
	temp += ioPath;
	ioPath = temp;
}

//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const nsFilePath& inPath)
//----------------------------------------------------------------------------------------
{
//    NS_ASSERTION(0, "nsFileSpec is unsupported - use nsIFile!");
	*this = inPath;
}

//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const nsFilePath& inPath)
//----------------------------------------------------------------------------------------
{
	mPath = (const char*)inPath;
	nsFileSpecHelpers::UnixToNative(mPath);
	mError = NS_OK;
} // nsFileSpec::operator =

//----------------------------------------------------------------------------------------
nsFilePath::nsFilePath(const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
{
	*this = inSpec;
} // nsFilePath::nsFilePath

//----------------------------------------------------------------------------------------
void nsFilePath::operator = (const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
{
	mPath = inSpec.mPath;
	nsFileSpecHelpers::NativeToUnix(mPath);
} // nsFilePath::operator =

//----------------------------------------------------------------------------------------
void nsFileSpec::SetLeafName(const char* inLeafName)
//----------------------------------------------------------------------------------------
{
	NS_ASSERTION(inLeafName, "Attempt to SetLeafName with a null string");
	mPath.LeafReplace('\\', inLeafName);
} // nsFileSpec::SetLeafName

//----------------------------------------------------------------------------------------
char* nsFileSpec::GetLeafName() const
//----------------------------------------------------------------------------------------
{
    return mPath.GetLeaf('\\');
} // nsFileSpec::GetLeafName

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::Exists() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
	struct stat st;
	return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st); 
#else
	return !mPath.IsEmpty() && PR_SUCCESS == PR_Access(nsNSPRPath(*this), PR_ACCESS_EXISTS); 
#endif
} // nsFileSpec::Exists

//----------------------------------------------------------------------------------------
void nsFileSpec::GetModDate(TimeStamp& outStamp) const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
	struct stat st;
    if (!mPath.IsEmpty() && stat(nsNSPRPath(*this), &st) == 0) 
        outStamp = st.st_mtime; 
    else
        outStamp = 0;
#else
    PRFileInfo prfi;
    if (!mPath.IsEmpty() && PR_SUCCESS == PR_GetFileInfo(nsNSPRPath(*this), &prfi)) 
        outStamp = (TimeStamp)(prfi.modifyTime / PR_USEC_PER_SEC);
    else
        outStamp = 0;
#endif
} // nsFileSpec::GetModDate

//----------------------------------------------------------------------------------------
PRUint32 nsFileSpec::GetFileSize() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
	struct stat st;
    if (!mPath.IsEmpty() && stat(nsNSPRPath(*this), &st) == 0) 
        return (PRUint32)st.st_size; 
    return 0;
#else
    PRFileInfo prfi;
    if (!mPath.IsEmpty() && PR_SUCCESS == PR_GetFileInfo(nsNSPRPath(*this), &prfi)) 
        return prfi.size;
    else
        return 0;
#endif
} // nsFileSpec::GetFileSize

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsFile() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
  struct stat st;
  return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFREG & st.st_mode);
#else
    PRFileInfo prfi;
    return !mPath.IsEmpty() && PR_SUCCESS == PR_GetFileInfo(nsNSPRPath(*this), &prfi) && (PR_FILE_FILE == prfi.type);
#endif
} // nsFileSpec::IsFile

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsDirectory() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
	struct stat st;
	return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFDIR & st.st_mode);
#else
    PRFileInfo prfi;
    return !mPath.IsEmpty() && PR_SUCCESS == PR_GetFileInfo(nsNSPRPath(*this), &prfi) && (PR_FILE_DIRECTORY == prfi.type);
#endif
} // nsFileSpec::IsDirectory

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsHidden() const
//----------------------------------------------------------------------------------------
{
    PRBool hidden = PR_FALSE;
    if (!mPath.IsEmpty())
    {
        DWORD attr = GetFileAttributesA(mPath);
        if (FILE_ATTRIBUTE_HIDDEN & attr)
            hidden = PR_TRUE;
    }
    return hidden;
}
// nsFileSpec::IsHidden

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsSymlink() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
    HRESULT hres; 
    IShellLink* psl; 
    
    PRBool isSymlink = PR_FALSE;
    
    CoInitialize(NULL);
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
        
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        {
            WORD wsz[MAX_PATH]; 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, mPath, -1, wsz, MAX_PATH); 
 
            // Load the shortcut. 
            hres = ppf->Load(wsz, STGM_READ); 
            if (SUCCEEDED(hres)) 
            {
                isSymlink = PR_TRUE;
            }
            
            // Release the pointer to the IPersistFile interface. 
            ppf->Release(); 
        }
        
        // Release the pointer to the IShellLink interface. 
        psl->Release();
    }

    CoUninitialize();

    return isSymlink;
#else
    WCHAR wsz[MAX_PATH];
    WCHAR wtz[MAX_PATH];

    if(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mPath, -1, wsz, MAX_PATH))
    {
        return SHGetShortcutTarget(wsz, wtz, MAX_PATH);
    }
    return FALSE;
#endif
}


//----------------------------------------------------------------------------------------
nsresult nsFileSpec::ResolveSymlink(PRBool& wasSymlink)
//----------------------------------------------------------------------------------------
{
    wasSymlink = PR_FALSE;  // assume failure

	if (Exists())
		return NS_OK;

#if !defined(WINCE)
    HRESULT hres; 
    IShellLink* psl; 

    CoInitialize(NULL);

    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
        
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        {
            WORD wsz[MAX_PATH]; 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, mPath, -1, wsz, MAX_PATH); 
 
            // Load the shortcut. 
            hres = ppf->Load(wsz, STGM_READ); 
            if (SUCCEEDED(hres)) 
            {
                wasSymlink = PR_TRUE;

                // Resolve the link. 
                hres = psl->Resolve(nsnull, SLR_NO_UI ); 
                if (SUCCEEDED(hres)) 
                { 
                    char szGotPath[MAX_PATH]; 
                    WIN32_FIND_DATA wfd; 

                    // Get the path to the link target. 
                    hres = psl->GetPath( szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY ); 

                    if (SUCCEEDED(hres))
                    {
                        // Here we modify the nsFileSpec;
                        mPath = szGotPath;
                        mError = NS_OK;
                    }
                } 
            }
            else {
                // It wasn't a shortcut. Oh well. Leave it like it was.
                hres = 0;
            }

            // Release the pointer to the IPersistFile interface. 
            ppf->Release(); 
        }
        // Release the pointer to the IShellLink interface. 
        psl->Release();
    }

    CoUninitialize();

    if (SUCCEEDED(hres))
        return NS_OK;

    return NS_FILE_FAILURE;
#else
    WCHAR wsz[MAX_PATH];
    
    if(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mPath, -1, wsz, MAX_PATH))
    {
        WCHAR wtz[MAX_PATH];
        wasSymlink = SHGetShortcutTarget(wsz, wtz, MAX_PATH);
        if(FALSE != wasSymlink)
        {
            char wcz[MAX_PATH];
            
            if(WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, wtz, -1, wcz, sizeof(wcz), NULL, NULL))
            {
                mPath = wcz;
                mError = NS_OK; // ???
                
                return NS_OK;
            }
        }
    }
    return NS_FILE_FAILURE;
#endif
}



//----------------------------------------------------------------------------------------
void nsFileSpec::GetParent(nsFileSpec& outSpec) const
//----------------------------------------------------------------------------------------
{
	outSpec.mPath = mPath;
	char* chars = (char*)outSpec.mPath;
	chars[outSpec.mPath.Length() - 1] = '\0'; // avoid trailing separator, if any
    char* cp = strrchr(chars, '\\');
    if (cp++)
	    outSpec.mPath.SetLength(cp - chars); // truncate.
} // nsFileSpec::GetParent

//----------------------------------------------------------------------------------------
void nsFileSpec::operator += (const char* inRelativePath)
//----------------------------------------------------------------------------------------
{
	NS_ASSERTION(inRelativePath, "Attempt to do += with a null string");

	if (!inRelativePath || mPath.IsEmpty())
		return;
	
	if (mPath[mPath.Length() - 1] == '\\')
		mPath += "x";
	else
		mPath += "\\x";
	
	// If it's a (unix) relative path, make it native
	nsSimpleCharString dosPath = inRelativePath;
	nsFileSpecHelpers::UnixToNative(dosPath);
	SetLeafName(dosPath);
} // nsFileSpec::operator +=

//----------------------------------------------------------------------------------------
void nsFileSpec::CreateDirectory(int /*mode*/)
//----------------------------------------------------------------------------------------
{
	// Note that mPath is canonical!
#if !defined(WINCE)
	if (!mPath.IsEmpty())
	    mkdir(nsNSPRPath(*this));
#else
    if (!mPath.IsEmpty())
        PR_MkDir(nsNSPRPath(*this), 0777);
#endif
} // nsFileSpec::CreateDirectory

//----------------------------------------------------------------------------------------
void nsFileSpec::Delete(PRBool inRecursive) const
//----------------------------------------------------------------------------------------
{
    if (IsDirectory())
    {
	    if (inRecursive)
        {
            for (nsDirectoryIterator i(*this, PR_FALSE); i.Exists(); i++)
                {
                    nsFileSpec& child = (nsFileSpec&)i;
                    child.Delete(inRecursive);
                }		
        }
#if !defined(WINCE)
	    rmdir(nsNSPRPath(*this));
#else
        PR_RmDir(nsNSPRPath(*this));
#endif
    }
	else if (!mPath.IsEmpty())
    {
#if !defined(WINCE)
        remove(nsNSPRPath(*this));
#else
        PR_Delete(nsNSPRPath(*this));
#endif
    }
} // nsFileSpec::Delete


//----------------------------------------------------------------------------------------
void nsFileSpec::RecursiveCopy(nsFileSpec newDir) const
//----------------------------------------------------------------------------------------
{
    if (IsDirectory())
    {
		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

		for (nsDirectoryIterator i(*this, PR_FALSE); i.Exists(); i++)
		{
			nsFileSpec& child = (nsFileSpec&)i;

			if (child.IsDirectory())
			{
				nsFileSpec tmpDirSpec(newDir);

				char *leafname = child.GetLeafName();
				tmpDirSpec += leafname;
				nsCRT::free(leafname);

				child.RecursiveCopy(tmpDirSpec);
			}
			else
			{
   				child.RecursiveCopy(newDir);
			}
		}
    }
    else if (!mPath.IsEmpty())
    {
		nsFileSpec& filePath = (nsFileSpec&) *this;

		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

        filePath.CopyToDir(newDir);
    }
} // nsFileSpec::RecursiveCopy

//----------------------------------------------------------------------------------------
nsresult
nsFileSpec::Truncate(PRInt32 aNewFileLength) const
//----------------------------------------------------------------------------------------
{
    DWORD status;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    // Leave it to Microsoft to open an existing file with a function
    // named "CreateFile".
    hFile = CreateFileA(mPath,
                       GENERIC_WRITE, 
                       FILE_SHARE_READ, 
                       NULL, 
                       OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL); 
    if (hFile == INVALID_HANDLE_VALUE)
        return NS_FILE_FAILURE;

    // Seek to new, desired end of file
    status = SetFilePointer(hFile, aNewFileLength, NULL, FILE_BEGIN);
    if (status == 0xffffffff)
        goto error;

    // Truncate file at current cursor position
    if (!SetEndOfFile(hFile))
        goto error;

    if (!CloseHandle(hFile))
        return NS_FILE_FAILURE;

    return NS_OK;

 error:
    CloseHandle(hFile);
    return NS_FILE_FAILURE;

} // nsFileSpec::Truncate

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Rename(const char* inNewName)
//----------------------------------------------------------------------------------------
{
	NS_ASSERTION(inNewName, "Attempt to Rename with a null string");

    // This function should not be used to move a file on disk. 
    if (strchr(inNewName, '/')) 
        return NS_FILE_FAILURE;

    char* oldPath = nsCRT::strdup(mPath);
    
    SetLeafName(inNewName);        

    if (PR_Rename(oldPath, mPath) != NS_OK)
    {
        // Could not rename, set back to the original.
        mPath = oldPath;
        return NS_FILE_FAILURE;
    }
    
    nsCRT::free(oldPath);
    
    return NS_OK;
} // nsFileSpec::Rename

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::CopyToDir(const nsFileSpec& inParentDirectory) const
//----------------------------------------------------------------------------------------
{
    // We can only copy into a directory, and (for now) can not copy entire directories
    if (inParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inParentDirectory.GetCString());
        destPath += "\\";
        destPath += leafname;
        nsCRT::free(leafname);
        
        // CopyFile returns non-zero if succeeds
        int copyOK = CopyFileA(GetCString(), destPath, PR_TRUE);
        if (copyOK)
            return NS_OK;
    }
    return NS_FILE_FAILURE;
} // nsFileSpec::CopyToDir

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::MoveToDir(const nsFileSpec& inNewParentDirectory)
//----------------------------------------------------------------------------------------
{
    // We can only copy into a directory, and (for now) can not copy entire directories
    if (inNewParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inNewParentDirectory.GetCString());
        destPath += "\\";
        destPath += leafname;
        nsCRT::free(leafname);

        // MoveFile returns non-zero if succeeds
        int copyOK = MoveFileA(GetCString(), destPath);

        if (copyOK)
        {
            *this = inNewParentDirectory + GetLeafName(); 
            return NS_OK;
        }
        
    }
    return NS_FILE_FAILURE;
} // nsFileSpec::MoveToDir

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Execute(const char* inArgs ) const
//----------------------------------------------------------------------------------------
{    
    if (!IsDirectory())
    {
#if !defined(WINCE)
        nsSimpleCharString fileNameWithArgs = "\"";
        fileNameWithArgs += mPath + "\" " + inArgs;
        int execResult = WinExec( fileNameWithArgs, SW_NORMAL );     
        if (execResult > 31)
            return NS_OK;
#else
        WCHAR wPath[MAX_PATH];
        if(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mPath, -1, wPath, sizeof(wPath) / sizeof(WCHAR)))
        {
            WCHAR wArgs[MAX_PATH] = { L'\0' };
            if(NULL == inArgs || MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inArgs, -1, wArgs, sizeof(wArgs) / sizeof(WCHAR)))
            {
                SHELLEXECUTEINFO execInfo;
                
                memset(&execInfo, 0, sizeof(execInfo));
                
                execInfo.cbSize = sizeof(execInfo);
                execInfo.lpFile = wPath;
                execInfo.lpParameters = wArgs;
                execInfo.nShow = SW_SHOWNORMAL;
                
                BOOL started = ShellExecuteEx(&execInfo);
                
                return FALSE != started ? NS_OK : NS_FILE_FAILURE;
            }
        }

#endif
    }
    return NS_FILE_FAILURE;
} // nsFileSpec::Execute


//----------------------------------------------------------------------------------------
PRInt64 nsFileSpec::GetDiskSpaceAvailable() const
//----------------------------------------------------------------------------------------
{
#if !defined(WINCE)
    PRInt64 int64;
    
    LL_I2L(int64 , LONG_MAX);

    char aDrive[_MAX_DRIVE + 2];
	_splitpath( (const char*)mPath, aDrive, NULL, NULL, NULL);

	if (aDrive[0] == '\0')
	{
        // The back end is always trying to pass us paths that look
        //   like /c|/netscape/mail.  See if we've got one of them
        if (mPath.Length() > 2 && mPath[0] == '/' && mPath[2] == '|')
        {
            aDrive[0] = mPath[1];
            aDrive[1] = ':';
            aDrive[2] = '\0';
        }
        else
        {
            // Return bogus large number and hope for the best
            return int64; 
        }
    }

	strcat(aDrive, "\\");

    // Check disk space
    DWORD dwSecPerClus, dwBytesPerSec, dwFreeClus, dwTotalClus;
    ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes, liTotalNumberOfFreeBytes;
    double nBytes = 0;

    BOOL (WINAPI* getDiskFreeSpaceExA)(LPCTSTR lpDirectoryName, 
                                       PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                       PULARGE_INTEGER lpTotalNumberOfBytes,    
                                       PULARGE_INTEGER lpTotalNumberOfFreeBytes) = NULL;

    HINSTANCE hInst = LoadLibrary("KERNEL32.DLL");
    NS_ASSERTION(hInst != NULL, "COULD NOT LOAD KERNEL32.DLL");
    if (hInst != NULL)
    {
        getDiskFreeSpaceExA =  (BOOL (WINAPI*)(LPCTSTR lpDirectoryName, 
                                               PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                               PULARGE_INTEGER lpTotalNumberOfBytes,    
                                               PULARGE_INTEGER lpTotalNumberOfFreeBytes)) 
        GetProcAddress(hInst, "GetDiskFreeSpaceExA");
        FreeLibrary(hInst);
    }

    if (getDiskFreeSpaceExA && (*getDiskFreeSpaceExA)(aDrive,
                                                      &liFreeBytesAvailableToCaller, 
                                                      &liTotalNumberOfBytes,  
                                                      &liTotalNumberOfFreeBytes))
    {
        nBytes = (double)(signed __int64)liFreeBytesAvailableToCaller.QuadPart;
    }
    else if ( GetDiskFreeSpace(aDrive, &dwSecPerClus, &dwBytesPerSec, &dwFreeClus, &dwTotalClus))
    {
        nBytes = (double)dwFreeClus*(double)dwSecPerClus*(double) dwBytesPerSec;
    }
    return (PRInt64)nBytes;
#else
    ULARGE_INTEGER freeBytesAvailableToCaller;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    //
    // This will only target the object store (NULL path).
    // There could be UNC paths to memory cards, remote UNC paths, whatever.
    // May need to be fixed one day.
    //
    BOOL gotten = GetDiskFreeSpaceEx(NULL, &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes);

    if(FALSE != gotten)
    {
        return (PRInt64)freeBytesAvailableToCaller.QuadPart;
    }
    else
    {
        return 0;
    }
#endif
}



//========================================================================================
//								nsDirectoryIterator
//========================================================================================

//----------------------------------------------------------------------------------------
nsDirectoryIterator::nsDirectoryIterator(const nsFileSpec& inDirectory, PRBool resolveSymlink)
//----------------------------------------------------------------------------------------
	: mCurrent(inDirectory)
	, mDir(nsnull)
    , mStarting(inDirectory)
	, mExists(PR_FALSE)
    , mResoveSymLinks(resolveSymlink)
{
    mDir = PR_OpenDir(inDirectory);
	mCurrent += "dummy";
    mStarting += "dummy";
    ++(*this);
} // nsDirectoryIterator::nsDirectoryIterator

//----------------------------------------------------------------------------------------
nsDirectoryIterator::~nsDirectoryIterator()
//----------------------------------------------------------------------------------------
{
    if (mDir)
	    PR_CloseDir(mDir);
} // nsDirectoryIterator::nsDirectoryIterator

//----------------------------------------------------------------------------------------
nsDirectoryIterator& nsDirectoryIterator::operator ++ ()
//----------------------------------------------------------------------------------------
{
	mExists = PR_FALSE;
	if (!mDir)
		return *this;
    PRDirEntry* entry = PR_ReadDir(mDir, PR_SKIP_BOTH); // Ignore '.' && '..'
	if (entry)
    {
      mExists = PR_TRUE;
      mCurrent = mStarting;
      mCurrent.SetLeafName(entry->name);
      if (mResoveSymLinks)
      {   
          PRBool ignore;
          mCurrent.ResolveSymlink(ignore);
      }
    }
	return *this;
} // nsDirectoryIterator::operator ++

//----------------------------------------------------------------------------------------
nsDirectoryIterator& nsDirectoryIterator::operator -- ()
//----------------------------------------------------------------------------------------
{
	return ++(*this); // can't do it backwards.
} // nsDirectoryIterator::operator --

