/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "nsSoftwareUninstall.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsSoftwareUpdate.h"
#include "nsInstall.h"
#include "nsManifestLineReader.h"
#include "nsInt64.h"
#include "nsEnumeratorUtils.h"
#include "nsTopProgressNotifier.h"
#include "nsLoggingProgressNotifier.h"
#include "ScheduledTasks.h"
#include "nsProcess.h"
#include "nsInstallExecute.h"

#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif

#define MAX_BUF 4096

static NS_DEFINE_CID(kIProcessCID, NS_PROCESS_CID); 

nsSoftwareUninstall::nsSoftwareUninstall(nsCString *aRegName, nsCString *aPrettyName)
{
  mRegName = aRegName;
  mPrettyName = aPrettyName;
}

nsresult 
nsSoftwareUninstall::GetInstalledPackages(nsISimpleEnumerator **aResult)
{ 
    nsresult rv;

    nsCOMPtr<nsISupportsArray>  packageArray;
    rv = NS_NewISupportsArray(getter_AddRefs(packageArray));
    if (NS_FAILED(rv))  
        return NS_ERROR_FAILURE; 

    char *buf = nsnull;
    PRInt32 bufSize;

    rv = GetUninstallLogContents(&buf, &bufSize);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    nsManifestLineReader reader;
    nsVoidArray *actions = new nsVoidArray();
    reader.Init(buf, bufSize);

    while (reader.NextLine() != PR_FALSE)
    {
        char *line = reader.LinePtr();
        if (line[0] == '[')
        {
            char *regNamePtr;
            char *prettyPtr;

            line[strlen(line)-1] = '\0';  // trim the trailing ]
            regNamePtr = line+1;          // skip the [
            prettyPtr = regNamePtr;

            while (*prettyPtr != '\0' )
            {//while pp not null if above break
                if ((*prettyPtr== '*') && (*(prettyPtr+1)== '*') && (*(prettyPtr+2)== '*'))
                    break;

              ++prettyPtr;     // find the separator
            }
            if (*prettyPtr == '0') 
                continue; // this line is messed up, keep looking.

            *prettyPtr = '\0'; // terminate the regName            
            prettyPtr+=3;      // skip to the beginning of the pretty name


            nsCString *regName = new nsCString();
            nsCString *pretty = new nsCString();
            nsCString *versionString = new nsCString();

            regName->Assign(regNamePtr);
            pretty->Assign(prettyPtr);

            versionString->Assign(NS_LITERAL_CSTRING("Version String")); // have to get the version info
            nsXPIPackageInfo *packInfo = new nsXPIPackageInfo(pretty, regName, versionString);

            packageArray->AppendElement(packInfo);
            reader.NextLine();
            line = reader.LinePtr();
            while(!(line[0] == '['))
            {
                actions->AppendElement(line); 
                if (reader.NextLine() == PR_FALSE)
                    break;
                line = reader.LinePtr();
            }
        }
    }

    nsArrayEnumerator *theEnum = new nsArrayEnumerator(packageArray);
    if (!aResult) 
        return NS_ERROR_FAILURE;

    return CallQueryInterface(theEnum, aResult);
}

nsresult
nsSoftwareUninstall::GetUninstallLogPath(nsString *aPath)
{
    nsresult rv;
    nsCOMPtr<nsIFile> iFile;
    /* we'll never be in the stub installer, so don't check. */
    nsCOMPtr<nsIProperties> dirSvc =
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    
    if (!dirSvc) 
        return NS_ERROR_FAILURE;

    dirSvc->Get(NS_OS_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile),
                getter_AddRefs(iFile));

    if (!nsSoftwareUpdate::GetUninstallLogName())
        rv = iFile->AppendNative(UNINSTALL_LOG);
    else
        rv = iFile->AppendNative(nsDependentCString(nsSoftwareUpdate::GetUninstallLogName()));

    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    PRBool bExists = PR_FALSE, bWritable = PR_FALSE;

    rv = iFile->Exists(&bExists);
    if (NS_FAILED(rv)) 
        return rv;

    if (!bExists) 
    {         // uninstall information is not in the program dir, look in the profile dir
        dirSvc->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                    getter_AddRefs(iFile));

        if (!nsSoftwareUpdate::GetUninstallLogName())
            rv = iFile->AppendNative(UNINSTALL_LOG);
        else
            rv = iFile->AppendNative(nsDependentCString(nsSoftwareUpdate::GetUninstallLogName()));

        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;
    }

    rv = iFile->IsWritable(&bWritable);
    if (NS_FAILED(rv) || !bWritable)
        return NS_ERROR_FAILURE;
    
    rv = iFile->GetPath(*aPath);
    return NS_OK;
}

nsresult
nsSoftwareUninstall::GetUninstallLogContents(char **aBuf, PRInt32 *aSize)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &rv);
    nsString *aPath = new nsString();

    GetUninstallLogPath(aPath);

    rv = localFile->InitWithPath(*aPath);
    if (NS_FAILED(rv)) 
        return rv;

    PRFileDesc* fd = nsnull;

    rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd);
    if (NS_FAILED(rv))
        return rv;

    PRInt64 flen;
    rv = localFile->GetFileSize(&flen);
    if (NS_FAILED(rv))
    {
        PR_Close(fd);
        return rv;
    }

    PRInt32 fileSize = nsInt64(flen);
    if (aSize == 0)
    {
        PR_Close(fd);
        return NS_ERROR_FAILURE;
    }

    *aBuf = new char[fileSize+1];
    if (!*aBuf)
    {
        PR_Close(fd);
        return NS_ERROR_FAILURE;    
    }

    if (fileSize > PR_Read(fd, *aBuf, fileSize))
    {
        PR_Close(fd);
        return NS_ERROR_FAILURE;
    }

    if (fd)
        PR_Close(fd);

    aBuf[fileSize-1] = '\0';
    *aSize = fileSize;

    delete aPath;
    return NS_OK;
}

void 
AppendBackSlash(char *szInput, PRUint32 dwInputSize)
{
  if(szInput != NULL)
  {
    if(*szInput == '\0')
    {
      if(((DWORD)lstrlen(szInput) + 1) < dwInputSize)
      {
        lstrcat(szInput, "\\");
      }
    }
    else if(szInput[strlen(szInput) - 1] != '\\')
    {
      if(((DWORD)lstrlen(szInput) + 1) < dwInputSize)
      {
        lstrcat(szInput, "\\");
      }
    }
  }
}

char* 
GetFirstNonSpace(char *aString)
{
  PRInt32   i;
  PRInt32   iStrLength;

  iStrLength = PL_strlen(aString);

  for(i = 0; i < iStrLength; i++)
  {
    if(!isspace(aString[i]))
      return(&aString[i]);
  }

  return nsnull;
}


nsresult
nsSoftwareUninstall::PerformUninstall( )
{
    char* buf = nsnull;
    PRInt32 flen;
    nsresult rv;

    rv = GetUninstallLogContents(&buf, &flen);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    nsManifestLineReader reader;
    nsVoidArray *actions = new nsVoidArray();
    reader.Init(buf, flen);

// we want the file to contain utf8 strings, so we need to make sure we're 
// doing the right thing with the data we read in

    while (reader.NextLine() != PR_FALSE)
    {
        char *line = reader.LinePtr();
        if ( PL_strcasestr(line,mRegName->get()) && 
             PL_strcasestr(line,mPrettyName->get()) )   
        {
            reader.NextLine();
            line = reader.LinePtr();
            while(line[0] != '[')
            {
                actions->AppendElement(line); // line
                if (reader.NextLine() == PR_FALSE)
                    break;
                line = reader.LinePtr();
            }
        }
    }

    if (actions->Count() == 0)
    {
        delete [] buf;
        return NS_OK;  // nothing to do....
    }

    PerformUninstallActions(actions);  // this should return ok / not ok

    for (PRInt32 index = 0; index < flen-1; ++index)
      if ( buf[index] == '\0' ) 
		  buf[index] = '\n';
	buf[flen-1] = '\0';
    // undo the damage done by reader
    
    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &rv); 
    nsString *aPath = new nsString();
    GetUninstallLogPath(aPath);

    rv = localFile->InitWithPath(*aPath);
    nsFileSpec *fileSpec = nsnull;
      
    nsCOMPtr<nsIFile> iFile = do_QueryInterface(localFile, &rv);
    rv = Convert_nsIFile_To_nsFileSpec(iFile, &fileSpec);
    if (NS_FAILED(rv) || (!fileSpec)) 
    {
        delete [] buf;
        return NS_ERROR_NULL_POINTER;
    }


// this has to be writing out utf8strings
    nsOutputFileStream *outStream;
    outStream = new nsOutputFileStream(*fileSpec, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0744 );

    if (!outStream)
        return NS_ERROR_NULL_POINTER;
        
    reader.Init(buf, flen);
    while (reader.NextLine() != PR_FALSE)
    {
        char *line = reader.LinePtr();
        if ( PL_strcasestr(line,mRegName->get()) && 
             PL_strcasestr(line,mPrettyName->get()) )   
        {
            reader.NextLine();
            line = reader.LinePtr();
            while(line[0] != '[') 
            {
                if (reader.NextLine() == PR_FALSE)
                    break;
                line = reader.LinePtr();
            }
        }
        else
        {
            *outStream << line << nsEndl;
        }
    }

    outStream->close();
    delete outStream;
    outStream = nsnull;

    delete [] buf;
    delete aPath;

    return NS_OK;
}

void
nsSoftwareUninstall::PerformFileDeleteShared(char *aFilename)
{
// stub
    return;
}

void
nsSoftwareUninstall::PerformFileDeleteXPCOMComp(char *aFilename)
{
// stub 
    return;
}


void 
nsSoftwareUninstall::PerformFileDelete(char *anAction, char* aKey, char *filename)
{

    char *tmp = PL_strcasestr(anAction,aKey); // skip the qualifiers before the key
	char *namePtr = GetFirstNonSpace( &(tmp[strlen(aKey)]) );
    PL_strcpy(filename,namePtr);

    if (PL_strcasestr(anAction,KEY_SHARED_FILE))
    {
        PerformFileDeleteShared(filename);
    } 
    else if (PL_strcasestr(anAction,KEY_XPCOM_COMPONENT))
    {
        PerformFileDeleteXPCOMComp(filename);
    } 
    else if (PL_strcasestr(anAction,KEY_DO_NOT_UNINSTALL) == nsnull)
        PerformFileDoDeleteFile(filename);
}

void
nsSoftwareUninstall::PerformFileDoDeleteFile(char *aFilename)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !localFile) 
        return;

    nsString path;
    path = NS_ConvertUTF8toUCS2(aFilename);

    rv = localFile->InitWithPath(path);
    if (NS_FAILED(rv))
        return;

    PRBool flagExists;

    localFile->Exists(&flagExists);
    if(flagExists) // if the file is there, then schedule it for deletion.
    {
        nsCOMPtr<nsIFile> iFile = do_QueryInterface(localFile);    
        ScheduleFileForDeletion(iFile);
    }
}

void 
ParseWinRegKeyElements(char* anAction, char* *aRoot, char* *aKey, char* *aName)
{
  PRInt32 actionLen = PL_strlen(anAction);
  PRInt32 actionIndex = 0;
  PRInt32 keyLen = 0;
  PRInt32 i;
  PRBool bFoundName = PR_FALSE;
  PRBool bFoundOpenBracket = PR_FALSE;
  PRInt32 iBrackets = 0;

  if (!actionLen)
      return;

  *aRoot = anAction;
  while(anAction[actionIndex] != '\\' && anAction[actionIndex]!= '\0')
  {
    ++actionIndex;
    if(actionIndex == actionLen)
    {
      *aRoot = *aKey = *aName = nsnull;
      return;
    }
  } // find the key
    
  *aKey = &(anAction[actionIndex+1]);
  anAction[actionIndex] = '\0';
  
  keyLen = PL_strlen(*aKey);

  for(i = keyLen -1; i>= 0; i--)
  {
    if(bFoundName == PR_FALSE)
    {
      /* Find the Name created in the Windows registry key.
       * Since the Name can contain '[' and ']', we have to
       * parse for the brackets that denote the Name string in
       * szFirstNonSpace.  It parses from right to left.
       */
      if((*aKey)[i] == ']')
      {
        if(iBrackets == 0)
          (*aKey)[i] = '\0';  //the very end of the name, null terminate

        ++iBrackets;
      }
      else if((*aKey)[i] == '[')
      {
        bFoundOpenBracket = PR_TRUE;
        --iBrackets;
      }

      if((bFoundOpenBracket) && (iBrackets == 0))
      {
        // we have foind the location of the beginning of the name.
        *aName = &((*aKey)[i+1]);
        bFoundName = PR_TRUE;
      }
    }
    else
    {
      /* locate the first non space to the left of the last '[' */
      if(!nsCRT::IsAsciiSpace((*aKey)[i]))
      {
        (*aKey)[i + 1] = '\0';
        break;
      }
    }
  }
}

#ifdef WIN32
HKEY  ParseRootKey(char* szRootKey)
{
  HKEY hkRootKey;

  if(PL_strcmp(szRootKey, "HKEY_CURRENT_CONFIG") == 0)
    hkRootKey = HKEY_CURRENT_CONFIG;
  else if(PL_strcmp(szRootKey, "HKEY_CURRENT_USER") == 0)
    hkRootKey = HKEY_CURRENT_USER;
  else if(PL_strcmp(szRootKey, "HKEY_LOCAL_MACHINE") == 0)
    hkRootKey = HKEY_LOCAL_MACHINE;
  else if(PL_strcmp(szRootKey, "HKEY_USERS") == 0)
    hkRootKey = HKEY_USERS;
  else if(PL_strcmp(szRootKey, "HKEY_PERFORMANCE_DATA") == 0)
    hkRootKey = HKEY_PERFORMANCE_DATA;
  else if(PL_strcmp(szRootKey, "HKEY_DYN_DATA") == 0)
    hkRootKey = HKEY_DYN_DATA;
  else /* HKEY_CLASSES_ROOT */
    hkRootKey = HKEY_CLASSES_ROOT;

  return(hkRootKey);
}
#endif

void 
PerformDeleteWinRegValue(char *aRootKey, char *szKey, char *szName)
{
#ifdef WIN32
  HKEY    hkResult;
  HKEY    hkRootKey;
  DWORD   dwErr;

  hkRootKey = ParseRootKey(aRootKey);
  dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_WRITE, &hkResult);
  if(dwErr == ERROR_SUCCESS)
  {
    if(*szName == '\0')
      dwErr = RegDeleteValue(hkResult, NULL);
    else
      dwErr = RegDeleteValue(hkResult, szName);

    RegCloseKey(hkResult);
  }
#else
  return;
#endif
}


void PerformDeleteWinRegKey(char* aRootKey, char *szKey)
{
#ifdef WIN32
  HKEY      hkResult;
  HKEY      hkRootKey;
  DWORD     dwErr;
  DWORD     dwTotalSubKeys;
  DWORD     dwTotalValues;
//  DWORD     dwSubKeySize;
//  FILETIME  ftLastWriteFileTime;
//  char      szSubKey[MAX_BUF];
//  char      szNewKey[MAX_BUF];
// long      lRv;


  hkRootKey = ParseRootKey(aRootKey);

  dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_QUERY_VALUE, &hkResult);
  if(dwErr == ERROR_SUCCESS)
  {
    dwTotalSubKeys = 0;
    dwTotalValues  = 0;
    RegQueryInfoKey(hkResult, NULL, NULL, NULL, &dwTotalSubKeys, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
    RegCloseKey(hkResult);

    if(!dwTotalSubKeys)
        RegDeleteKey(hkRootKey, szKey);

/*
    if(dwTotalSubKeys)
    {
      do
      {
        dwSubKeySize = sizeof(szSubKey);
        lRv = 0;
        if(RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult) == ERROR_SUCCESS)
        {
          if((lRv = RegEnumKeyEx(hkResult, 0, szSubKey, &dwSubKeySize, NULL, NULL, NULL, &ftLastWriteFileTime)) == ERROR_SUCCESS)
          {
            RegCloseKey(hkResult);
            lstrcpy(szNewKey, szKey);
            AppendBackSlash(szNewKey, sizeof(szNewKey));
            lstrcat(szNewKey, szSubKey);
            PerformDeleteWinRegKey(aRootKey, szNewKey);
          }
          else
            RegCloseKey(hkResult);
        }
      } while(lRv != ERROR_NO_MORE_ITEMS);
    }

    dwErr = RegDeleteKey(hkRootKey, szKey);
*/
  }
#else
  return;
#endif
}

void
PerformUninstallCommand(char *anAction)
{
#define ARG_SLOTS 256

    PRInt32   result = NS_OK;
    PRInt32   rv = nsInstall::SUCCESS;
    PRInt32   argcount = 0;
    PRInt32   exeLen = 0;
    PRInt32   exeIndex = 0;
    PRUint32  pid;
    nsresult  nsrv;
    PRBool    bExists;
    char      *cArgs[ARG_SLOTS];
    char      *theExe;
    char      *theArgs;

    theExe = PL_strcasestr(anAction,KEY_REGISTER_UNINSTALL_COMMAND);
    if (theExe)
        theExe = GetFirstNonSpace(&anAction[strlen(KEY_REGISTER_UNINSTALL_COMMAND)+1]); 

    exeLen = strlen(theExe);

    for (exeIndex; exeIndex < exeLen; ++exeIndex)
    {
        if (theExe[exeIndex]== '"')
            break;
        if (theExe[exeIndex] == '\0')
            return;       // we can't find the end of the command.  abort.
    }

    theArgs = GetFirstNonSpace(&(theExe[exeIndex+1]));
    theExe[exeIndex] = '\0';


    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID, &nsrv);
    localFile->InitWithPath(NS_ConvertUTF8toUCS2(theExe));
    nsCOMPtr<nsIFile> iFile = do_QueryInterface(localFile, &nsrv);

    iFile->Exists(&bExists);
    if (bExists)
    {

        nsCOMPtr<nsIProcess> process = do_CreateInstance(kIProcessCID);

        if (PL_strlen(theArgs))
        {
            argcount = xpi_PrepareProcessArguments(theArgs, cArgs, ARG_SLOTS);
        }
        if (argcount >= 0)
        {
            result = process->Init(iFile);
            if (NS_SUCCEEDED(result))
            {
                process->Run(PR_TRUE, (const char**)&cArgs, argcount, &pid);
            }
        }
    }
}

void 
ParseForCopyFile(char* aString, char *aKeyStr, char *aFile, PRInt32 dwShortFilenameBufSize)
{
  int     iLen;
  char*   szFirstNonSpace;
  char*   szSubStr = NULL;
  char    szBuf[MAX_BUF];
  char*   leafName;

  if((szSubStr = PL_strcasestr(aString, " to ")) != NULL)
  {
    if((szFirstNonSpace = GetFirstNonSpace(&(szSubStr[PL_strlen(" to ")]))) != NULL)
    {
      iLen = PL_strlen(szFirstNonSpace);
      if(szFirstNonSpace[iLen - 1] == '\n')
        szFirstNonSpace[iLen -1] = '\0';

      PL_strcpy(szBuf, szFirstNonSpace);
	  leafName = szSubStr;
	  while (leafName != aString)
	  {
		  if(*leafName == '\\')  // for xp it has to be path delimeter.
			  break;
		  --leafName;
	  }
	  *szSubStr = '\0';
	  PL_strcat(szBuf,leafName);

      if(PL_strcmp(aKeyStr, KEY_WINDOWS_SHORTCUT) == 0)
      {
        PL_strcat(szBuf, ".lnk");
      }

      PL_strcpy(aFile,szBuf);
    }
  }
}

void
nsSoftwareUninstall::PerformUninstallActions(nsVoidArray *aActions)
{
    char filename[MAX_BUF];
    char theAction[MAX_BUF];
    char *subString = nsnull;
    char *root = nsnull;
    char *key =  nsnull;
    char *name = nsnull;

    for (PRInt32 index = aActions->Count() - 1; index >= 0; --index)
    {
//        theAction = (char*)aActions->ElementAt(index);
        PL_strcpy(theAction,(char*)aActions->ElementAt(index));

        if (subString = PL_strcasestr(theAction,KEY_INSTALL_FILE))
        {  
            PerformFileDelete(theAction, KEY_INSTALL_FILE, filename); 
        } 
        else if (subString = PL_strcasestr(theAction,KEY_REPLACE_FILE))
        {  
            PerformFileDelete(theAction, KEY_REPLACE_FILE, filename);  
        } 
        else if (subString = PL_strcasestr(theAction, KEY_COPY_FILE))
        {
            ParseForCopyFile(theAction,KEY_COPY_FILE, filename, strlen(filename));
            PerformFileDoDeleteFile(filename);
        }
        else if (subString = PL_strcasestr(theAction, KEY_MOVE_FILE))
        {
            ParseForCopyFile(theAction,KEY_MOVE_FILE, filename, strlen(filename));
            PerformFileDoDeleteFile(filename);
        }
        else if (subString = PL_strcasestr(theAction, KEY_CREATE_FOLDER))
        {
            PerformFileDelete(theAction, KEY_CREATE_FOLDER, filename);  
        }
        else if (subString = PL_strcasestr(theAction, KEY_REGISTER_UNINSTALL_COMMAND))
        {
            PerformUninstallCommand(subString); 
        }
        else if (subString = PL_strcasestr(theAction, KEY_CREATE_REG_KEY))
        {
            ParseWinRegKeyElements(GetFirstNonSpace(&(theAction[strlen(KEY_CREATE_REG_KEY)])), 
                                   &root, &key, &name);
            PerformDeleteWinRegKey(root,key);
        }
        else if (subString = PL_strcasestr(theAction, KEY_STORE_REG_VALUE_STR))
        {
            ParseWinRegKeyElements(GetFirstNonSpace(&(theAction[strlen(KEY_STORE_REG_VALUE_STR)])), 
                         &root, &key, &name);
            PerformDeleteWinRegValue(root, key, name);
        }
        else if (subString = PL_strcasestr(theAction, KEY_STORE_REG_VALUE_NUM))
        {
            ParseWinRegKeyElements(GetFirstNonSpace(&(theAction[strlen(KEY_STORE_REG_VALUE_STR)])), 
                         &root, &key, &name);
            PerformDeleteWinRegValue(root, key, name);
        }
        else if (subString = PL_strcasestr(theAction, KEY_STORE_REG_VALUE))
        {
            ParseWinRegKeyElements(GetFirstNonSpace(&(theAction[strlen(KEY_STORE_REG_VALUE_STR)])), 
                         &root, &key, &name);
            PerformDeleteWinRegValue(root, key, name);
        }
    }
}


//////////////////////////////////////////
//  nsXPIPackageInfo
///////

// should this be called nsInstallPackageInfo???

NS_IMPL_ISUPPORTS1(nsXPIPackageInfo, nsIXPIPackageInfo)

nsXPIPackageInfo::nsXPIPackageInfo()
{
    NS_INIT_ISUPPORTS();
    /* member initializers and constructor code */
}

nsXPIPackageInfo::nsXPIPackageInfo(nsCString *aPrettyName, nsCString *aRegName, nsCString *aVersionString)
{
    NS_INIT_ISUPPORTS();

    mPrettyName    = aPrettyName;
    mRegName       = aRegName;
    mVersionString = aVersionString;
}

nsXPIPackageInfo::~nsXPIPackageInfo()
{
  /* destructor code */
}

/* readonly attribute wstring prettyName; */
NS_IMETHODIMP nsXPIPackageInfo::GetPrettyName(nsACString & aPrettyName)
{
    aPrettyName.Assign(*mPrettyName);
    return NS_OK;
}

/* readonly attribute wstring regName; */
NS_IMETHODIMP nsXPIPackageInfo::GetRegName(nsACString & aRegName)
{
    aRegName.Assign(*mRegName);
    return NS_OK;
}

/* readonly attribute wstring versionString; */
NS_IMETHODIMP nsXPIPackageInfo::GetVersionString(nsACString & aVersionString)
{
    aVersionString.Assign(NS_LITERAL_CSTRING("version"));
    return NS_OK;
}

