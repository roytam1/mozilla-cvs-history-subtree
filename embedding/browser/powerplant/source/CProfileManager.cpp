/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 */

// PPBrowser
#include "CProfileManager.h"
#include "ApplIDs.h"
#include "UMacUnicode.h"

// PowerPlant
#include <LEditText.h>
#include <LTextTableView.h>
#include <LPushButton.h>
#include <LTableMonoGeometry.h>
#include <LTableArrayStorage.h>
#include <LTableSingleSelector.h>


// Mozilla
#include "nsIProfile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsXPIDLString.h"

// Constants

const MessageT    msg_OnNewProfile 	        = 2000;
const MessageT    msg_OnDeleteProfile 	    = 2001;
const MessageT    msg_OnRenameProfile 	    = 2002;


//*****************************************************************************
//***    CProfileManager
//*****************************************************************************

CProfileManager::CProfileManager(const PRUnichar* programName,
                                 const PRUnichar* profilesRootDirName) :
    LAttachment(msg_AnyMessage,true),
    mProgramName(programName), mProfilesRootDirName(profilesRootDirName) 
{
}

CProfileManager::~CProfileManager()
{
}

void CProfileManager::StartUp()
{
    nsresult rv;
     
    {
        // Redefine NS_APP_USER_PROFILES_ROOT_DIR to be a directory of our choice
        // The default implementation of appfilelocprovider makes it <Documents>Mozilla:Users50:
        // Note that we could also do this by supplying our own implementation of appfilelocprovider
        // to NS_InitEmbedding. Since we only need to change one location, redefining is easier.
        
        nsCOMPtr<nsIFile> profileRootDir;
        PRBool exists;

        NS_WITH_SERVICE(nsIProperties, directoryService, NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        ThrowIfNil_(directoryService);
        (void) directoryService->Undefine(NS_APP_USER_PROFILES_ROOT_DIR);
        rv = directoryService->Get(NS_MAC_DOCUMENTS_DIR, NS_GET_IID(nsIFile), getter_AddRefs(profileRootDir));
        ThrowIfNil_(profileRootDir);
        rv = profileRootDir->AppendUnicode(mProgramName.GetUnicode());
        ThrowIfError_(rv);
        rv = profileRootDir->AppendUnicode(mProfilesRootDirName.GetUnicode());
        ThrowIfError_(rv);
        rv = profileRootDir->Exists(&exists);
        if (NS_SUCCEEDED(rv) && !exists)
            rv = profileRootDir->Create(nsIFile::DIRECTORY_TYPE, 0);
        ThrowIfError_(rv);
        rv = directoryService->Define(NS_APP_USER_PROFILES_ROOT_DIR, profileRootDir);
        ThrowIfError_(rv);
    }
    
    NS_WITH_SERVICE(nsIProfile, profileService, NS_PROFILE_CONTRACTID, &rv);
    ThrowIfNil_(profileService);
        
    PRInt32 profileCount;
    rv = profileService->GetProfileCount(&profileCount);
    ThrowIfError_(rv);
    if (profileCount == 0)
    {
        // Make a new default profile
        nsString newProfileName; newProfileName.AssignWithConversion("default");
        rv = profileService->CreateNewProfile(newProfileName.GetUnicode(), nsnull, nsnull, PR_FALSE);
        ThrowIfError_(rv);
        rv = profileService->SetCurrentProfile(newProfileName.GetUnicode());
        ThrowIfError_(rv);
    }
    else
    {
        // If we have more than one profile, put up the UI which will allow the user to select one
        // We really don't need to do this. If we just executed the code below the next else, it
        // would activate the last used profile. Considering that we can switch profiles at any
        // time, this may be preferable.
                        
        if (profileCount > 1)
        {
            DoManageProfilesDialog();
        }
        else
        {
            // GetCurrentProfile returns the profile which was last used but is not nescesarily
            // active. Call SetCurrentProfile to make it installed and active.
            
            nsXPIDLString   currProfileName;
            rv = profileService->GetCurrentProfile(getter_Copies(currProfileName));
            ThrowIfError_(rv);
            rv = profileService->SetCurrentProfile(currProfileName);
            ThrowIfError_(rv);
        }    
    }
}

Boolean CProfileManager::DoNewProfileDialog(Str255& profileName)
{
    Boolean confirmed;
    StDialogHandler	theHandler(dlog_NewProfile, LCommander::GetTopCommander());
    LWindow			 *theDialog = theHandler.GetDialog();
    
    ThrowIfNil_(theDialog);
    LEditText *responseText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Name'));
    ThrowIfNil_(responseText);
    theDialog->SetLatentSub(responseText);

    theDialog->Show();
    theDialog->Select();
	
  	while (true)  // This is our modal dialog event loop
  	{				
  		MessageT	hitMessage = theHandler.DoDialog();
  		
  		if (hitMessage == msg_OK)
  		{
 		    responseText->GetDescriptor(profileName);
            confirmed = PR_TRUE;
     		break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    confirmed = PR_FALSE;
   		    break;
   		}
  	}
  	return confirmed;
}


void CProfileManager::DoManageProfilesDialog()
{
    nsresult rv;
    StDialogHandler	theHandler(dlog_ManageProfiles, LCommander::GetTopCommander());
    LWindow			 *theDialog = theHandler.GetDialog();

    NS_WITH_SERVICE(nsIProfile, profileService, NS_PROFILE_CONTRACTID, &rv);
    ThrowIfNil_(profileService);
    
    // Set up the dialog by filling the list of current profiles
    LTextTableView *table = (LTextTableView*) theDialog->FindPaneByID('List');    
    ThrowIfNil_(table);
    LPushButton *deleteButton = (LPushButton *) theDialog->FindPaneByID('Dele');
    ThrowIfNil_(deleteButton);
    
    Str255 pascalStr;
    nsAutoString unicodeStr;
    UInt32 dataSize;
    
    // PowerPlant stuff to set up the list view
    STableCell selectedCell(1, 1);
    SDimension16 tableSize;
    TableIndexT rows, cols;

    table->GetFrameSize(tableSize);
	table->SetTableGeometry(new LTableMonoGeometry(table, tableSize.width, 16));
	table->SetTableStorage(new LTableArrayStorage(table, 0UL));
	table->SetTableSelector(new LTableSingleSelector(table));
	table->InsertCols(1, 0);

    // Get the name of the current profile so we can select it
    nsXPIDLString   currProfileName;
    profileService->GetCurrentProfile(getter_Copies(currProfileName));
    
    // Get the list of profile names and add them to the list
    PRUint32 listLen;
    PRUnichar **profileList;
    rv = profileService->GetProfileList(&listLen, &profileList);
    ThrowIfError_(rv);
    
    for (PRUint32 index = 0; index < listLen; index++)
    {
          UMacUnicode::StringToStr255(nsString(profileList[index]), pascalStr);
          table->InsertRows(1, LONG_MAX, &pascalStr[1], pascalStr[0], true);
          
          if (nsCRT::strcmp(profileList[index], currProfileName.get()) == 0)
            selectedCell.row = index + 1;
    }
    
    PRInt32 numProfiles;
    rv = profileService->GetProfileCount(&numProfiles);
    ThrowIfError_(rv);    
    (numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
    table->SelectCell(selectedCell);
    
    theDialog->Show();
    theDialog->Select();
	
  	while (true)  // This is our modal dialog event loop
  	{				
  		MessageT	hitMessage = theHandler.DoDialog();
  		
  		if (hitMessage == msg_OK)
  		{
   		    selectedCell = table->GetFirstSelectedCell();
   		    if (selectedCell.row > 0)
   		    {
   		        dataSize = 255;
   		        table->GetCellData(selectedCell, &pascalStr[1], dataSize);
   		        pascalStr[0] = dataSize;
   		        UMacUnicode::Str255ToString(pascalStr, unicodeStr);
   		        
   		        rv = profileService->SetCurrentProfile(unicodeStr.GetUnicode());
            }
  		    break;
  		}
        else if (hitMessage == msg_Cancel)
        {
           	break;
        }
        else if (hitMessage == msg_OnNewProfile)
   		{
   		    if (DoNewProfileDialog(pascalStr))
   		    {
   		        UMacUnicode::Str255ToString(pascalStr, unicodeStr);
   		        rv = profileService->CreateNewProfile(unicodeStr.GetUnicode(), nsnull, nsnull, PR_FALSE);
   		        if (NS_FAILED(rv))
   		            break;
   		        
                table->InsertRows(1, LONG_MAX, &pascalStr[1], pascalStr[0], true);
                table->GetTableSize(rows, cols);
                table->SelectCell(STableCell(rows, cols));
                
                rv = profileService->GetProfileCount(&numProfiles);
                (NS_SUCCEEDED(rv) && numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
   		    }    
   		}
   		else if (hitMessage == msg_OnDeleteProfile)
   		{
   		    selectedCell = table->GetFirstSelectedCell();
   		    if (selectedCell.row > 0)
   		    {
   		        dataSize = 255;
   		        table->GetCellData(selectedCell, &pascalStr[1], dataSize);
   		        pascalStr[0] = dataSize;
   		        UMacUnicode::Str255ToString(pascalStr, unicodeStr);
   		        
   		        rv = profileService->DeleteProfile(unicodeStr.GetUnicode(), PR_TRUE);
   		        if (NS_FAILED(rv))
   		            break;
   		        
   		        table->RemoveRows(1, selectedCell.row, true);
   		        table->GetTableSize(rows, cols);    
   		        if (selectedCell.row >= rows)
   		            selectedCell.row = rows - 1;
   		        table->SelectCell(selectedCell);
   		        
                rv = profileService->GetProfileCount(&numProfiles);
                (NS_SUCCEEDED(rv) && numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
   		    }
   		}
   		else if (hitMessage == msg_OnRenameProfile)
   		{
	        dataSize = 255;
   		    selectedCell = table->GetFirstSelectedCell();
	        table->GetCellData(selectedCell, &pascalStr[1], dataSize);
	        pascalStr[0] = dataSize;
	        nsAutoString oldName;
	        UMacUnicode::Str255ToString(pascalStr, oldName);

   		    if (DoNewProfileDialog(pascalStr))
   		    {
   		        UMacUnicode::Str255ToString(pascalStr, unicodeStr);
                profileService->RenameProfile(oldName.GetUnicode(), unicodeStr.GetUnicode());
                table->SetCellData(selectedCell, &pascalStr[1], pascalStr[0]); 		        
   		    }
   		}
  	}  
}


//*****************************************************************************
//***    CProfileManager::LAttachment
//*****************************************************************************

void CProfileManager::ExecuteSelf(MessageT inMessage, void *ioParam)
{
	mExecuteHost = true;
	// update status
	if (inMessage == msg_CommandStatus) {
		SCommandStatus	*status = (SCommandStatus *)ioParam;
		if (status->command == cmd_ManageProfiles) {
			*status->enabled = true;
			*status->usesMark = false;
			mExecuteHost = false; // we handled it
		}
	}
	else if (inMessage == cmd_ManageProfiles) {
	    DoManageProfilesDialog();
	    mExecuteHost = false; // we handled it
	}
}
