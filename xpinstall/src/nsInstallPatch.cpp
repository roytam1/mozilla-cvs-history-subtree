/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nsFileSpec.h"
#include "prmem.h"
#include "nsInstall.h"
#include "nsInstallPatch.h"
#include "nsInstallResources.h"
#include "nsIDOMInstallVersion.h"

#include "VerReg.h"
#include "ScheduledTasks.h"


nsInstallPatch::nsInstallPatch( nsInstall* inInstall,
                                const nsString& inVRName,
                                nsIDOMInstallVersion* inVInfo,
                                const nsString& inJarLocation,
                                PRInt32 *error)

: nsInstallObject(inInstall)
{
    char* tempTargetFile = new char[MAXREGPATHLEN];
    char* tempVersionString = inVRName.ToNewCString();

    PRInt32 err = VR_GetPath(tempVersionString, MAXREGPATHLEN, tempTargetFile );
    
    delete [] tempVersionString;
    
    if (err != REGERR_OK)
    {
        if(tempTargetFile)
            delete [] tempTargetFile;
        
        *error = nsInstall::NO_SUCH_COMPONENT;
        return;
    }
    
     
    nsInstallPatch( inInstall, inVRName, inVInfo, inJarLocation, nsString(tempTargetFile), "null", error);
    
    delete [] tempTargetFile;
}


nsInstallPatch::nsInstallPatch( nsInstall* inInstall,
                                const nsString& inVRName,
                                nsIDOMInstallVersion* inVInfo,
                                const nsString& inJarLocation,
                                const nsString& folderSpec,
                                const nsString& inPartialPath,
                                PRInt32 *error)

: nsInstallObject(inInstall)
{
    if ((inInstall == nsnull) || (inVRName == "null") || (inJarLocation == "null")) 
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }
    
    mPatchFile      =   nsnull;
    mTargetFile     =   nsnull;
    mPatchedFile    =   nsnull;
    mRegistryName   =   new nsString(inVRName);
    mJarLocation    =   new nsString(inJarLocation);

    nsString tempString;
    inVInfo->ToString(tempString);
    mVersionInfo = new nsInstallVersion();
    mVersionInfo->Init(tempString);
    

     mTargetFile = new nsFileSpec(folderSpec);
     if(inPartialPath != "null")
        *mTargetFile += inPartialPath;
}

nsInstallPatch::~nsInstallPatch()
{
    if (mVersionInfo)
        delete mVersionInfo;

    if (mTargetFile)
        delete mTargetFile;

    if (mJarLocation)
        delete mJarLocation;
    
    if (mRegistryName)
        delete mRegistryName;

    if (mPatchedFile)
        delete mPatchedFile;
    
    if (mPatchFile)
        delete mPatchFile;

}


PRInt32 nsInstallPatch::Prepare()
{
    PRInt32 err;
    PRBool deleteOldSrc;
    
    if (mTargetFile == nsnull)
        return  nsInstall::INVALID_ARGUMENTS;

    if (mTargetFile->Exists())
    {
        if (mTargetFile->IsFile())
        {
            err = nsInstall::SUCCESS;
        }
        else
        {
            err = nsInstall::FILE_IS_DIRECTORY;
        }
    }
    else
    {
        err = nsInstall::FILE_DOES_NOT_EXIST;
    }

    if (err != nsInstall::SUCCESS)
    {   
        return err;
    }

    err =  mInstall->ExtractFileFromJar(*mJarLocation, mTargetFile, &mPatchFile);
   
    
    nsFileSpec *fileName = nsnull;
    nsVoidKey ikey( HashFilePath( nsFilePath(*mTargetFile) ) );
    
    mInstall->GetPatch(&ikey, fileName);

    if (fileName != nsnull) 
    {
        deleteOldSrc = PR_TRUE;
    } 
    else 
    {
        fileName     = mTargetFile;
        deleteOldSrc = PR_FALSE;
    }

    err = NativePatch(  *fileName,           // the file to patch
                        *mPatchFile,         // the patch that was extracted from the jarfile
                        &mPatchedFile);     // the new patched file
    
    if (err != nsInstall::SUCCESS)
    {   
        return err;
    }

    if ( mPatchedFile != nsnull ) 
    {
        mInstall->AddPatch(&ikey, mPatchedFile );
    } 
    else 
    {
        // This is a wierd state.  NativePatch should have generated an error here.  Is there any
        // reason why we return a Invalid_Arg error here?
        err = nsInstall::INVALID_ARGUMENTS;
    }

    if ( deleteOldSrc ) 
    {
        DeleteFileLater(*fileName );
    }
  
    return err;
}

PRInt32 nsInstallPatch::Complete()
{  
    if ((mInstall == nsnull) || (mVersionInfo == nsnull) || (mPatchedFile == nsnull) || (mTargetFile == nsnull)) 
    {
        return nsInstall::INVALID_ARGUMENTS;
    }
    
    PRInt32 err = nsInstall::SUCCESS;

    nsFileSpec *fileName = nsnull;
    nsVoidKey ikey( HashFilePath( nsFilePath(*mTargetFile) )  );
    
    mInstall->GetPatch(&ikey, fileName);
    
    if (fileName != nsnull && (*fileName == *mPatchedFile) )
    {
        // the patch has not been superceded--do final replacement
        err = ReplaceFileLater( *mTargetFile, *mPatchedFile );
        if ( 0 == err || nsInstall::REBOOT_NEEDED == err ) 
        {
            nsString tempVersionString;
            mVersionInfo->ToString(tempVersionString);
            
            char* tempRegName = mRegistryName->ToNewCString();
            char* tempVersion = tempVersionString.ToNewCString();

            err = VR_Install( tempRegName, 
                              (char*) (const char*) nsprPath(*mTargetFile), 
                              tempVersion, 
                              PR_FALSE );
            
            delete [] tempRegName;
            delete [] tempVersion;

        }
        else
        {
            err = nsInstall::UNEXPECTED_ERROR;
        }
    }
    else
    {
        // nothing -- old intermediate patched file was
        // deleted by a superceding patch
    }

    return err;
}

void nsInstallPatch::Abort()
{
    nsFileSpec *fileName = nsnull;
    nsVoidKey ikey( HashFilePath( nsFilePath(*mTargetFile) ) );

    mInstall->GetPatch(&ikey, fileName);

    if (fileName != nsnull && (*fileName == *mPatchedFile) )
    {
        DeleteFileLater( *mPatchedFile );
    }
}

char* nsInstallPatch::toString()
{
    char* buffer = new char[1024];
    
    sprintf( buffer, nsInstallResources::GetPatchFileString(), mPatchedFile->GetCString());
        
    return buffer;
}


PRBool
nsInstallPatch::CanUninstall()
{
    return PR_FALSE;
}

PRBool
nsInstallPatch::RegisterPackageNode()
{
    return PR_FALSE;
}

PRInt32
nsInstallPatch::NativePatch(const nsFileSpec &sourceFile, const nsFileSpec &patchfile, nsFileSpec **newFile)
{
    return -1;
}

void* 
nsInstallPatch::HashFilePath(const nsFilePath& aPath)
{
    PRUint32 rv = 0;
    const char* cPath = nsprPath(aPath);
    
    if(cPath != nsnull) 
    {
        char  ch;
        char* filePath = nsnull;
        strcpy(filePath, cPath);

        while ((ch = *filePath++) != 0) 
        {
            // FYI: rv = rv*37 + ch
            rv = ((rv << 5) + (rv << 2) + rv) + ch;
        }
        PR_Free(filePath);
    }
    return (void*)rv;
}