/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "prefwutil.h"

#include "resgui.h"
#include "uprefd.h"
#include "macutil.h"
#include "ufilemgr.h"
#include "uerrmgr.h"
#include "xp_mem.h"
#include "macgui.h"
#include "COffscreenCaption.h"

Boolean	CFilePicker::sResult = FALSE;
Boolean	CFilePicker::sUseDefault = FALSE;		// Use default directory
CStr255	CFilePicker::sPrevName;

CFilePicker::CFilePicker( LStream* inStream ): LView( inStream )
{
	// WARNING: if you add any streamable data here (because you're making a new custom
	// Cppb in constructor) you will clobber CPrefFilePicker, which inherits from
	// this class.  So beware!
	fCurrentValue.vRefNum = 0;
	fCurrentValue.parID = 0;
	fCurrentValue.name[ 0 ] = 0;
	fSet = FALSE;
	fBrowseButton = NULL;
	fPathName = NULL;
	fPickTypes = Applications;
}

void CFilePicker::FinishCreateSelf()
{
	ThrowIfNil_(fBrowseButton	= dynamic_cast<LControl*>(FindPaneByID(kBrowseButton)));
	ThrowIfNil_(fPathName		= dynamic_cast<LCaption*>(FindPaneByID(kPathNameCaption)));

	fBrowseButton->SetValueMessage(msg_Browse);
	fBrowseButton->AddListener(this);
}

void CFilePicker::SetFSSpec( const FSSpec& fileSpec, Boolean touchSetFlag )
{
	if (touchSetFlag)
		fSet = TRUE;
	fCurrentValue = fileSpec;
	
	SetCaptionForPath( fPathName, fileSpec );
	BroadcastMessage( msg_FolderChanged, this );
}

void CFilePicker::ListenToMessage( MessageT inMessage, void* /*ioParam*/ )
{
	StandardFileReply		reply;
	FSSpec					currentValue;

	currentValue = GetFSSpec();
	reply.sfFile = currentValue;
	
	if ( inMessage == msg_Browse )
	{
		switch ( fPickTypes )
		{
			case MailFiles:
				reply.sfFile = CPrefs::GetFilePrototype( CPrefs::MailFolder );
			case AnyFile:
			case Folders:
			case TextFiles:
			case ImageFiles:
			case Applications:
			{
				// ¥ pose the dialog and get the users new choice
				if ( CFilePicker::DoCustomGetFile( reply, fPickTypes, TRUE ) )
					SetFSSpec( reply.sfFile );
			}
			break;
		}
	}
}

void CFilePicker::SetCaptionForPath( LCaption* captionToSet, const FSSpec& folderSpec )
{
	CStr255	pathName = FSSpecToPathName( folderSpec );
	SetCaptionDescriptor( captionToSet, pathName, smTruncMiddle );	
}

CStr255 CFilePicker::FSSpecToPathName( const FSSpec& spec )
{
	char* pPathName = CFileMgr::PathNameFromFSSpec( spec, true );
	if ( pPathName == NULL && fPickTypes == Applications )
	{
		CStr255 badName(spec.name);
		ErrorManager::PlainAlert
			( (char *)GetCString(BAD_APP_LOCATION_RESID), badName,
				(char *)GetCString(REBUILD_DESKTOP_RESID), NULL );
		return badName;
	}
	CStr255 cPathName(pPathName);
	XP_FREE( pPathName );
	return cPathName;
}

void CFilePicker::SetButtonTitle( Handle buttonHdl, CStr255& name, const Rect& buttonRect )
{	
	short			result;	//
	short			width;	//

	sPrevName = name;
	
	CStr255 finalString(::GetCString(SELECT_RESID));

	width = ( buttonRect.right - buttonRect.left )
		- ( StringWidth( finalString ) + 2 * CharWidth( 'Ê' ) );

	result = ::TruncString( width, name, smTruncMiddle );
	
	finalString += name;
	SetControlTitle( (ControlHandle)(buttonHdl), finalString );
	ValidRect( &buttonRect );
}

#define kIsFolderFlag				0x10	// flag indicating the file is a directory.
#define kInvisibleFlag				0x4000	// flag indicating that the file is invisible.

#define kGetDirButton				10
#define kDefaultButton				12

PROCEDURE( CFilePicker::OnlyFoldersFileFilter, uppFileFilterYDProcInfo )
pascal Boolean CFilePicker::OnlyFoldersFileFilter( CInfoPBPtr pBlock, void* /*data*/ )
{	
	Boolean		isFolder;
	Boolean		isInvisible;
	Boolean		dontShow;
	
	isFolder = ((pBlock->hFileInfo).ioFlAttrib) & kIsFolderFlag;
	isInvisible = ((pBlock->dirInfo).ioDrUsrWds.frFlags) & kInvisibleFlag;
	dontShow = !isFolder || isInvisible;

	return dontShow;
}

PROCEDURE( CFilePicker::IsMailFileFilter, uppFileFilterYDProcInfo )
pascal Boolean CFilePicker::IsMailFileFilter( CInfoPBPtr pBlock, void* /*data*/ )
{
	Boolean		isEudora;
	Boolean		isMine;
	Boolean		dontShow;
	
	isMine = ( pBlock->hFileInfo).ioFlFndrInfo.fdCreator == emSignature;
	isEudora = ( pBlock->hFileInfo).ioFlFndrInfo.fdCreator == 'CSOm';
	dontShow = !isMine && !isEudora;
	
	return dontShow;
}

PROCEDURE( CFilePicker::SetCurrDirHook, uppDlgHookYDProcInfo )
pascal short CFilePicker::SetCurrDirHook( short item, DialogPtr /*dialog*/, void* /*data*/ )
{
	if ( item == sfHookFirstCall )
		return sfHookChangeSelection;
	return item;
}

PROCEDURE( CFilePicker::DirectoryHook, uppDlgHookYDProcInfo )
pascal short CFilePicker::DirectoryHook( short item, DialogPtr dialog, void* data )
{
	short 				type;				//	menu item selected
	Handle				handle;				//	needed for GetDialogItem
	Rect				rect;				//	needed for GetDialogItem

	CStr255				tempName = sPrevName;
	CStr255 			name;

	CInfoPBRec			pb;
	StandardFileReply*	sfrPtr;
	OSErr				err;
	short				returnVal;

	// ¥ default, except in special cases below
	returnVal = item;
	
	// ¥ this function is only for main dialog box
	if ( GetWRefCon((WindowPtr)dialog ) != sfMainDialogRefCon )
		return returnVal;					
	
	DirInfo* dipb = (DirInfo*)&pb;
	// ¥ get sfrPtr from 3rd parameter to hook function
	sfrPtr = (StandardFileReply*)((PickClosure*)data)->reply;

	GetDialogItem( dialog, kGetDirButton, &type, &handle, &rect );

	if ( item == sfHookFirstCall )
	{
		// ¥ determine current folder name and set title of Select button
		dipb->ioCompletion = NULL;
		dipb->ioNamePtr = (StringPtr)&tempName;
		dipb->ioVRefNum = sfrPtr->sfFile.vRefNum;
		dipb->ioDrDirID = sfrPtr->sfFile.parID;
		dipb->ioFDirIndex = - 1;
		err = PBGetCatInfoSync(&pb);
		name = tempName;
		CFilePicker::SetButtonTitle( handle, name, rect );

		return sfHookChangeSelection;		
	}
	else
	{
		// ¥ track name of folder that can be selected
		// (also allow selection of folder aliases)
		if ( ( sfrPtr->sfIsFolder) || ( sfrPtr->sfIsVolume) ||
			 ((sfrPtr->sfFlags & kIsAlias) && (sfrPtr->sfType == kContainerFolderAliasType)) )
			name = sfrPtr->sfFile.name;
		else
		{
			dipb->ioCompletion = NULL;
			dipb->ioNamePtr = (StringPtr)&tempName;
			dipb->ioVRefNum = sfrPtr->sfFile.vRefNum;
			dipb->ioFDirIndex = -1;
			dipb->ioDrDirID = sfrPtr->sfFile.parID;
			err = PBGetCatInfoSync(&pb);
			name = tempName;
		}
		
		// ¥ change directory name in button title as needed
		if ( name != sPrevName )
			CFilePicker::SetButtonTitle( handle, name, rect );

		switch ( item )
		{
			// ¥ force return by faking a cancel
			case kGetDirButton:	
				sResult = TRUE;			
				returnVal = sfItemCancelButton;
			break;
			
			// ¥ use default directory
			case kDefaultButton:
				sUseDefault = TRUE;
				returnVal = sfItemCancelButton;
			break;									
			
			// ¥ flag no directory was selected
			case sfItemCancelButton:
				sResult = FALSE;
			break;	
			
			default:
				break;
		}
	}
	return returnVal;
}

Boolean CFilePicker::DoCustomGetFile( StandardFileReply& reply, 
	CFilePicker::PickEnum pickTypes, Boolean isInited )
{
	Point				loc = { -1, -1 };
	OSErr				gesErr;
	long				gesResponse;
	FileFilterYDUPP		filter = NULL;
	DlgHookYDUPP		dialogHook;
	short				dlogID = 0;
	OSType				types[ 4 ];
	OSType*				typeP;
	short				typeCount = -1;
	Boolean*			resultP = &reply.sfGood;
	Boolean				wantPreview = FALSE;
	PickClosure			closure;
	
	reply.sfIsVolume = 0;
	reply.sfIsFolder = 0;
	reply.sfFlags = 0;
	reply.sfScript = smSystemScript;
	reply.sfGood = FALSE;
	closure.reply = &reply;
	closure.inited = isInited;
	typeP = types;
	
	if ( isInited )
		dialogHook = &PROCPTR( SetCurrDirHook );
	else
		dialogHook = NULL;
		
	// ¥Êinitialize name of previous selection
	sPrevName = reply.sfFile.name;

	StPrepareForDialog preparer;	
	gesErr = Gestalt( gestaltStandardFileAttr, &gesResponse );
	if ( gesResponse & ( 1L << gestaltStandardFile58 ) )
	{
		switch ( pickTypes )
		{
			case Folders:
			{
				filter = &PROCPTR( OnlyFoldersFileFilter );	
				dlogID = DLOG_GETFOLDER;
				dialogHook = &PROCPTR( DirectoryHook );
				resultP = &sResult;
				typeP = NULL;
			}
			break;
			
			case AnyFile:
			{
				typeP = NULL;
			}
			break;
			
			case MailFiles:
			{
				filter = &PROCPTR( IsMailFileFilter );
				types[ 0 ] = 'TEXT';
				typeCount = 1;
			}
			break;
			
			case TextFiles:
			{
				types[ 0 ] = 'TEXT';
				typeCount = 1;
			}
			break;
			
			case ImageFiles:
			{
				types[ 0 ] = 'GIFf';
				types[ 1 ] = 'TEXT';
				types[ 2 ] = 'JPEG';
				typeCount = 3;
				wantPreview = TRUE;
			}
			break;
			
			case Applications:
			{
				types[0] = 'APPL';
				types[1] = 'adrp';	// Application aliases
				typeCount = 2;
			}
			break;	
		}
	}
	
	if ( wantPreview && UEnvironment::HasGestaltAttribute( gestaltQuickTime, 0xFFFF ) )
		::CustomGetFilePreview( filter, typeCount, typeP, &reply, dlogID, loc, dialogHook,
			NULL, NULL, NULL, &closure );
	else
		::CustomGetFile( filter, typeCount, typeP, &reply, dlogID, loc, dialogHook,
			NULL, NULL, NULL, &closure );

	// follow any folder aliases that may be returned
	if (*resultP && pickTypes == Folders) {
		Boolean b1, b2;
		ThrowIfOSErr_( ResolveAliasFile(&reply.sfFile, true, &b1, &b2) );
	}
	
	return *resultP;
}

Boolean CFilePicker::DoCustomPutFile( StandardFileReply& reply,
	const CStr255& prompt, Boolean inited )
{
	Point			loc = { -1, -1 };
	CStr255			filename = CStr255::sEmptyString;
	DlgHookYDUPP	dialogHook = NULL;
	
	if ( inited  )
	{
		filename = reply.sfFile.name;
		dialogHook = &PROCPTR( SetCurrDirHook );
	}
	
	CustomPutFile( (ConstStr255Param)prompt,
		(ConstStr255Param)filename,
		&reply,
		0,
		loc,
		dialogHook,
		NULL,
		NULL,
		NULL,
		NULL );
		
	return reply.sfGood;
}	

// ===========================================================================
//	COtherSizeDialog
// ===========================================================================

const ResIDT	edit_SizeField = 		1504;

COtherSizeDialog::COtherSizeDialog( LStream* inStream ): LDialogBox( inStream )
{
}

void COtherSizeDialog::FinishCreateSelf()
{
	LDialogBox::FinishCreateSelf();
	
	mSizeField = (LEditField*)FindPaneByID( edit_SizeField );

	//	Set the edit field to go on duty whenever
	//	this dialog box goes on duty.

	fRef = NULL;
	SetLatentSub( mSizeField );
}

void COtherSizeDialog::SetValue( Int32 inFontSize )
{
	if ( inFontSize > 128 )
		inFontSize = 128;
		
	mSizeField->SetValue( inFontSize );
	mSizeField->SelectAll();
}

Int32 COtherSizeDialog::GetValue() const
{
	Int32		size;
	
	size = mSizeField->GetValue();
	if ( size > 128 )
		size = 128;
		
	return size;
}

void COtherSizeDialog::SetReference( LControl* which )
{
	fRef = which;
}

// ---------------------------------------------------------------------------
//		¥ ListenToMessage
// ---------------------------------------------------------------------------
//	This member function illustrates an alternate method for this dialog box
//	to communicate with its invoker when the user clicks the defualt (OK) button.
//	Usually, when the user clicks the default button, LDialogBox::ListenToMessage()
//	calls the supercommander's ObeyCommand() function passing it the value message
//	of the default button and a pointer to the dialog box object in the ioParam
//	parameter. This method also passes the value message of the default button,
//	but rather than sending a pointer to the dialog in ioParam, it sends the
//	new point size.
//
//	The advantage of using the alternate method is that the client that invokes
//	the dialog doesn't have to call any of the dialog class's member functions
//	to get its value and is not responsible for closing the dialog box. The
//	disadvantage is that you have to pass all the information back through
//	ObeyCommand()'s single ioParam parameter. 

void COtherSizeDialog::ListenToMessage(MessageT inMessage, void *ioParam)
{
	Int32	newFontSize;

	switch ( inMessage )
	{
		case msg_ChangeFontSize:
			//	This is the command number associated with this dialog's
			//	OK button. It's also used as a command number to send
			//	to the dialog's supercommander which is presumably
			//	the commander that created and invoked the dialog.

			newFontSize = mSizeField->GetValue();

			//	Note that we use the second parameter of ObeyCommand() to
			//	specify the new font size. When the menu mechanism invokes
			//	ObeyCommand(), the second parameter is always nil.
			
			BroadcastMessage( msg_ChangeFontSize, this );
			
			//	Since we want the OK button to close the dialog box as
			//	well as change the font size, we change the message to close
			//	and let the flow fall through to the default case.

			inMessage = cmd_Close;
			
			//	FALLTHROUGH INTENTIONAL
			//	to get the base class to close the dialog box

		default:
			LDialogBox::ListenToMessage(inMessage, ioParam);
			break;
	}
}


LArrowGroup::LArrowGroup( LStream* inStream ): LView( inStream )
{
	fSize = NULL;
	fArrows = NULL;
	fValue = 0L;
	fMinValue = 0L;
	fMaxValue = 99999999L;
	fStringID = MEGA_RESID;
	this->BuildControls();
}

void LArrowGroup::SetStringID(ResIDT stringID)
{
	fStringID = stringID;
}

void LArrowGroup::ListenToMessage( MessageT message, void* ioParam )
{
	if ( message == msg_ArrowsHit )
	{
		Int16	whichHalf = *(Int16*)ioParam;
		
		if ( whichHalf == mTopHalf )
			SetValue( fValue + 1 );
		else
			SetValue( fValue - 1 );
	}
}

void LArrowGroup::SetValue( Int32 value )
{
	if ( value < fMinValue )
		value = fMinValue;
	else if ( value > fMaxValue )
		value = fMaxValue;
		
	CStr255	string;
	NumToString( value, string );
	string += CStr255(*GetString(fStringID));
	fSize->SetDescriptor( string );
	fValue = value;
}

void LArrowGroup::SetMaxValue( Int32 newMax )
{
	if ( fValue > newMax )
		SetValue( newMax );
	fMaxValue = newMax;
}

void LArrowGroup::SetMinValue( Int32 newMin )
{
	if ( fValue < newMin )
		SetValue( newMin );
	fMinValue = newMin;
}


void LArrowGroup::BuildControls()
{
	SPaneInfo		paneInfo;
	
	paneInfo.paneID = 'Actl';
	paneInfo.width = 15;
	paneInfo.height = 25;
	paneInfo.visible = TRUE;
	paneInfo.enabled = TRUE;
	paneInfo.left = mFrameSize.width - 16;
	paneInfo.top = 0;
	paneInfo.userCon = 0;
	paneInfo.superView = this;

	fArrows = new LArrowControl( paneInfo, msg_ArrowsHit );
	ThrowIfNil_(fArrows);
	fArrows->AddListener( this );
	
	paneInfo.paneID = 'capt';
	paneInfo.width = mFrameSize.width - 16;
	paneInfo.height = 12;
	paneInfo.left = 0;
	paneInfo.top = 5;
	
	fSize = new COffscreenCaption(paneInfo, "\p", 10000);
}

CColorButton::CColorButton( LStream *inStream ): LButton( inStream )
{
	fInside = FALSE;
	mNormalID = 0;
	mPushedID = 1;
}

void CColorButton::DrawGraphic( ResIDT inGraphicID )
{
	Rect	frame;

	CalcLocalFrameRect( frame );
	UGraphics::SetFore( CPrefs::Black );
	if ( IsEnabled() )
	{
		if ( inGraphicID == mPushedID )
		{
			::PenSize( 2, 2 );
			::PenPat( &qd.black );
			::FrameRect( &frame );
		}
		else
		{
			::PenSize( 1, 1 );
			::PenPat( &qd.black );
			::FrameRect( &frame );
			UGraphics::SetFore( CPrefs::White );
			::InsetRect( &frame, 1, 1 );
			::FrameRect( &frame );
			UGraphics::SetFore( CPrefs::Black );
		}
	}
	else
	{
		::PenSize( 1, 1 );
		::PenPat( &qd.gray );
		::FrameRect( &frame );
		::PenPat( &qd.black );

	}
		
	RGBForeColor( &fColor );

	if ( inGraphicID == mPushedID )
		::InsetRect( &frame, 3, 3 );
	else
		::InsetRect( &frame, 2, 2 );
		
	::FillRect( &frame, &( UQDGlobals::GetQDGlobals()->black ) );
	UGraphics::SetFore( CPrefs::Black );
}

void CColorButton::HotSpotResult( short inHotSpot )
{
	Point where;
	where.h = where.v = 0;
	RGBColor outColor;

	if ( ::GetColor(where, (unsigned char *)*GetString(PICK_COLOR_RESID),
														&fColor,&outColor) )
	{
		if ( !UGraphics::EqualColor(fColor, outColor) )
		{
			fColor = outColor;
			BroadcastValueMessage();
		}
	}

	// ¥ turn off the black border
	HotSpotAction( inHotSpot, FALSE, TRUE );

}



/********************************************************************

OneClickLListBox implementation.

Derived from LListBox. Overrides ClickSelf so that it sends a 
message when it has only been clicked once, not twice.

********************************************************************/
OneClickLListBox::OneClickLListBox( LStream* inStream ): LListBox( inStream )
{
	mSingleClickMessage = msg_Nothing;
}


void	OneClickLListBox::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	if(SwitchTarget(this))
	{
		FocusDraw();
		if (::LClick(inMouseDown.whereLocal, inMouseDown.macEvent.modifiers, mMacListH)) 
		{
			BroadcastMessage(mDoubleClickMessage, this);
		} else {
			BroadcastMessage(mSingleClickMessage, this);
		}
	}
}

Boolean		OneRowLListBox::HandleKeyPress(const EventRecord &inKeyEvent)
{
	if(OneClickLListBox::HandleKeyPress(inKeyEvent))
	{
		Char16	theKey = inKeyEvent.message & charCodeMask;
		// based on LListBox::HandleKeyPress--only broadcast when our class handled it
		// (window might close--deleting us--before we broadcast)
		if ( UKeyFilters::IsNavigationKey(theKey) ||
			( UKeyFilters::IsPrintingChar(theKey) && ! (inKeyEvent.modifiers & cmdKey) ) )
		{
			BroadcastMessage(mSingleClickMessage, this);
		}
		return true;
	}
	else
		return false;
}


int16		OneRowLListBox::GetRows() 
{ 
	return ((**mMacListH).dataBounds.bottom); 
}

OneRowLListBox::OneRowLListBox( LStream* inStream ): OneClickLListBox( inStream )
{
	FocusDraw();
	if((*mMacListH)->dataBounds.right == 0)
		::LAddColumn(1 , 0, mMacListH);
	(*mMacListH)->selFlags |= lOnlyOne;
}

void 	OneRowLListBox::AddRow(int32 rowNum, char* data, int16 datalen)
{
	if(SwitchTarget(this))
	{
		FocusDraw();
		Cell theCell;
		theCell.h = 0;
		rowNum = ::LAddRow(1 , rowNum, mMacListH);
		theCell.v = rowNum;
		::LSetCell(data,datalen ,theCell, mMacListH);
	}
}
void 	OneRowLListBox::GetCell(int32 rowNum, char* data, int16* datalen)
{
	FocusDraw();
	Cell theCell;
	theCell.h = 0;
	theCell.v = rowNum;
	::LGetCell(data,datalen ,theCell, mMacListH);
}
void 	OneRowLListBox::SetCell(int32 rowNum, char* data, int16 datalen)
{
	if(SwitchTarget(this))
	{
		FocusDraw();
		Cell theCell;
		theCell.h = 0;
		theCell.v = rowNum;
		::LSetCell(data,datalen ,theCell, mMacListH);
	}
}

void 	OneRowLListBox::RemoveRow(int32 rowNum)
{
	FocusDraw();
	::LDelRow(1 , rowNum, mMacListH);
}
