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

// prefwutils.h
// Various utilities used by preference window
// They are apart from Prefw, so that our file size is manageable

#pragma once

#include "MoreMixedMode.h"

#include <LTable.h>
#include <LGAEditField.h>

class CValidEditField;
class LArrowControl;
class CApplicationIconInfo;
class CPrefHelpersContain;
//  class CMimeMapper;
class CStr255;

/********************************************************************************
 * Classes
 ********************************************************************************/

//======================================
class CFilePicker
//======================================
	:	public LView
	,	public LListener
	,	public LBroadcaster
{
public:
	enum				{ class_ID = 'fpck' }; // illegal, needs one UC char.- jrm
	
	enum				PickEnum { Folders = 0, Applications, TextFiles,
							ImageFiles, MailFiles, AnyFile };

						CFilePicker( LStream* inStream );

		
	virtual void		ListenToMessage( MessageT inMessage, void* ioParam );

	void				SetFSSpec( const FSSpec& fileSpec, Boolean touchSetFlag = true );
	const FSSpec&		GetFSSpec() const { return fCurrentValue; }

	void				SetPickType( CFilePicker::PickEnum pickTypes ) { fPickTypes = pickTypes; }
	void				SetCaptionForPath( LCaption* captionToSet, const FSSpec& folderSpec );
	CStr255				FSSpecToPathName( const FSSpec& spec );
	
	Boolean				WasSet() const { return fSet; }

	
	static Boolean		DoCustomGetFile( StandardFileReply& spec,
							CFilePicker::PickEnum fileType,
							Boolean inited );
	static Boolean		DoCustomPutFile( StandardFileReply& spec,
							const CStr255& prompt,
							Boolean inited );
protected:
	struct PickClosure
	{
		StandardFileReply*		reply;
		Boolean					inited;
	};

	enum EPaneIDs {
		kPathNameCaption	= 1,
		kBrowseButton		= 2
	};

	virtual void		FinishCreateSelf();
		
	static pascal short		SetCurrDirHook( short item, DialogPtr dialog, void* data );
	PROCDECL( static, SetCurrDirHook )
	static pascal short		DirectoryHook( short item, DialogPtr dialog, void* data );
	PROCDECL( static, DirectoryHook )

	static pascal Boolean	OnlyFoldersFileFilter( CInfoPBPtr pBlock, void* data );
	static pascal Boolean	IsMailFileFilter( CInfoPBPtr pBlock, void* data );
	PROCDECL( static, OnlyFoldersFileFilter )
	PROCDECL( static, IsMailFileFilter )
	
	static void			SetButtonTitle( Handle buttonHdl, CStr255& name, const Rect& buttonRect );
	
	static CStr255		sPrevName;
	static Boolean		sResult;
	static Boolean		sUseDefault;
	
	FSSpec				fCurrentValue;
	LControl*			fBrowseButton;
	LCaption*			fPathName;
	Boolean				fSet;
	PickEnum			fPickTypes;
}; // class CFilePicker

//	COtherSizeDialog.cp	<- double-click + Command-D to see class implementation
//
//	This is a PowerPlant dialog box to handle the OtherÉ command in the Size
//	menu. 

class LEditField;

class COtherSizeDialog: public LDialogBox, public LBroadcaster
{
public:
	enum { class_ID = 'OFnt' };
						COtherSizeDialog( LStream* inStream );
	

	virtual	void		SetValue( Int32 inFontSize );
	virtual Int32		GetValue() const;

	void				SetReference( LControl* which );

	virtual void		ListenToMessage( MessageT inMessage, void* ioParam );

	LControl*			fRef;
protected:
	virtual void		FinishCreateSelf();
	LEditField*			mSizeField;	
};

class LArrowGroup: public LView, public LListener
{
public:
						LArrowGroup( LStream* inStream );

	void				ListenToMessage( MessageT message, void* ioParam );	

	void				SetValue( Int32 value );
	Int32				GetValue() const { return fValue; }
	
	void				SetMaxValue( Int32 value );
	void				SetMinValue( Int32 value );
	void				SetStringID(ResIDT stringID);
protected:
	void				BuildControls();
	
	Int32				fValue;
	Int32				fMinValue;
	Int32				fMaxValue;
	ResIDT				fStringID;
	LCaption*			fSize;
	LArrowControl*		fArrows;
};

/*****************************************************************************
 * class CColorButton
 * Just a button that pops up a color wheel when pressed
 *****************************************************************************/
	
class CColorButton: public LButton
{
public:
	enum { class_ID = 'pcol' };
	// ¥¥ constructors
						CColorButton( LStream* inStream );

	// ¥¥ colors
	void				SetColor( const RGBColor& color ) { fColor = color; }
	RGBColor			GetColor() { return fColor; }

	// ¥¥ control overrides
	virtual void		HotSpotResult( short inHotSpot );
	virtual void		DrawGraphic( ResIDT inGraphicID );
protected:
	RGBColor			fColor;
	Boolean				fInside;
};

//-----------------------------------------------------------------------------
#include "PopupBox.h"
class FileIconsLister: public StdPopup {
public:
					FileIconsLister (CGAPopupMenu * target);
	virtual			~FileIconsLister();
	CStr255			GetText (short item);
	void			SetIconList(CApplicationIconInfo *);
	short			GetCount();
private:
	CApplicationIconInfo *	fIcons;
};


/*****************************************************************************
 * Class OneClickLListBox
 * ----------------------
 * Just like an LListBox, except that it will send messages on
 * a single click. Used in the Document Encoding Dialog Box.
 *****************************************************************************/
 
 class OneClickLListBox : public LListBox
{

	public:
						OneClickLListBox(LStream * inStream);
	
	Int16				GetSingleClickMessage() { return mSingleClickMessage; }
	virtual void		SetSingleClickMessage(Int16 inMessage) 
											{ mSingleClickMessage = inMessage;}
	virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);

	protected:
	Int16				mSingleClickMessage;
	
};

/*****************************************************************************
 * Class OneRowLListBox
 *****************************************************************************/
class OneRowLListBox : public OneClickLListBox
{

	public:
	enum { class_ID = 'ocLB' };
						OneRowLListBox(LStream * inStream);
	virtual Boolean		HandleKeyPress(const EventRecord &inKeyEvent);
	
	virtual Int16		GetRows();		
	virtual void		AddRow(Int32 rowNum, char* data, Int16 datalen);
	virtual void		RemoveRow(Int32 rowNum);
	virtual void		GetCell(Int32 rowNum, char* data, Int16* datalen);
	virtual void		SetCell(Int32 rowNum, char* data, Int16 datalen);
};
