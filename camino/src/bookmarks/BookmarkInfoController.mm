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
* The Original Code is the Mozilla browser.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation. Portions created by Netscape are
* Copyright (C) 2002 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s):
*   Ben Goodger <ben@netscape.com> (Original Author)
*   David Haas  <haasd@cae.wisc.edu>
*/

#import "NSString+Utils.h"

#import "BookmarkInfoController.h"

#include "nsIContent.h"
#include "nsINamespaceManager.h"


@interface BookmarkInfoController(Private)

- (void)showUIElementPair: (id)aLabel control: (id) aControl;
- (void)hideUIElementPair: (id)aLabel control: (id) aControl;
- (void)commitChanges:(id)sender;
- (void)commitField:(id)textField toProperty:(nsIAtom*)propertyAtom;

@end;

@implementation BookmarkInfoController

/* BookmarkInfoController singelton */
static BookmarkInfoController *sharedBookmarkInfoController = nil;

+ (id)sharedBookmarkInfoController
{
  if (!sharedBookmarkInfoController) {
    sharedBookmarkInfoController = [[BookmarkInfoController alloc] initWithWindowNibName:@"BookmarkInfoPanel"];
  }
  
  return sharedBookmarkInfoController;
}

- (id)initWithWindowNibName:(NSString *)windowNibName
{
  if ((self = [super initWithWindowNibName:@"BookmarkInfoPanel"]))
  {
    //custom field editor lets us undo our changes
    mFieldEditor = [[NSTextView alloc] init];
    [mFieldEditor setAllowsUndo:YES];
    [mFieldEditor setFieldEditor:YES];
    
  }
  return self;
}

- (void)awakeFromNib
{
  // keep a ref so that we can remove and add to the its superview with impunity
  [mNameField retain];
  [mLocationField retain];
  [mKeywordField retain];
  [mDescriptionField retain];
  [mNameLabel retain];
  [mLocationLabel retain];
  [mKeywordLabel retain];
  [mDescriptionLabel retain];
  [mDockMenuCheckbox retain];
  
  [[BookmarksManager sharedBookmarksManager] addBookmarksClient:self];
}

-(void)dealloc
{
  // this is never called
  if (self == sharedBookmarkInfoController)
    sharedBookmarkInfoController = nil;

  [[BookmarksManager sharedBookmarksManager] removeBookmarksClient:self];

  [mFieldEditor release];

  [mNameField release];
  [mLocationField release];
  [mKeywordField release];
  [mDescriptionField release];
  [mNameLabel release];
  [mLocationLabel release];
  [mKeywordLabel release];
  [mDescriptionLabel release];
  [mDockMenuCheckbox release];
  
  [super dealloc];
}

-(void)controlTextDidEndEditing: (NSNotification*) aNotification
{
  [self commitChanges:[aNotification object]];
  [[mFieldEditor undoManager] removeAllActions];
}

-(void)windowDidBecomeKey:(NSNotification*) aNotification
{
  [[self window] makeFirstResponder:mNameField];
}

-(void)windowDidResignKey:(NSNotification*) aNotification
{
  [[self window] makeFirstResponder:[self window]];
	if (![[self window] isVisible])
    mBookmarkItem = nil;
}

- (void)windowWillClose:(NSNotification *)aNotification
{
  mBookmarkItem = nil;
}

- (void)commitChanges:(id)changedField
{
  if (![mBookmarkItem contentNode])
    return;

  // Name
  if (changedField == mNameField)
    [self commitField:mNameField toProperty:BookmarksService::gNameAtom];
  
  // Location
  if (changedField == mLocationField)
    [self commitField:mLocationField toProperty:BookmarksService::gHrefAtom];

  // Keyword
  if (changedField == mKeywordField)
    [self commitField:mKeywordField toProperty:BookmarksService::gKeywordAtom];

  // Description
  if (changedField == mDescriptionField)
    [self commitField:mDescriptionField toProperty:BookmarksService::gDescriptionAtom];

  [[mFieldEditor undoManager] removeAllActions];
  BookmarksService::BookmarkChanged([mBookmarkItem contentNode], TRUE);
}

- (void)commitField:(id)textField toProperty:(nsIAtom*)propertyAtom
{
  unsigned int len;
  PRUnichar* buffer;
  nsXPIDLString buf;

  // we really need a category on NSString for this
  len = [[textField stringValue] length];
  buffer = new PRUnichar[len + 1];
  if (!buffer) return;
  [[textField stringValue] getCharacters:buffer];
  buffer[len] = (PRUnichar)'\0';
  buf.Adopt(buffer);
  [mBookmarkItem contentNode]->SetAttr(kNameSpaceID_None, propertyAtom, buf, PR_TRUE);
}

- (IBAction)dockMenuCheckboxClicked:(id)sender
{
  if ([sender state] == NSOnState)
    BookmarksService::SetDockMenuRoot([mBookmarkItem contentNode]);
  else
    BookmarksService::SetDockMenuRoot(NULL);
}

-(void)setBookmark: (BookmarkItem*) aBookmark
{
  // See bug 154081 - don't show this window if Bookmark doesn't exist
  // after fix - this should never happen unless disaster strikes.
  if (![aBookmark contentNode])
    return;

  nsAutoString group;
  [aBookmark contentNode]->GetAttr(kNameSpaceID_None, BookmarksService::gGroupAtom, group);
  BOOL isGroup  = !group.IsEmpty();
  BOOL isFolder = !isGroup && [aBookmark isFolder];

  // First, Show/Hide the appropriate UI
  if (isGroup) 
  {
    [self showUIElementPair: mNameLabel        control: mNameField];
    [self hideUIElementPair: mLocationLabel    control: mLocationField];
    [self showUIElementPair: mKeywordLabel     control: mKeywordField];
    [self showUIElementPair: mDescriptionLabel control: mDescriptionField];

    [mNameField setNextKeyView:mKeywordField];
    [mDockMenuCheckbox removeFromSuperview];
  }
  else if (isFolder)
  {
    [self showUIElementPair: mNameLabel        control: mNameField];
    [self hideUIElementPair: mLocationLabel    control: mLocationField];
    [self hideUIElementPair: mKeywordLabel     control: mKeywordField];
    [self showUIElementPair: mDescriptionLabel control: mDescriptionField];

    [mNameField setNextKeyView:mDescriptionField];
    [mVariableFieldsContainer addSubview: mDockMenuCheckbox];
  }
  else
  {
    [self showUIElementPair: mNameLabel        control: mNameField];
    [self showUIElementPair: mLocationLabel    control: mLocationField];
    [self showUIElementPair: mKeywordLabel     control: mKeywordField];
    [self showUIElementPair: mDescriptionLabel control: mDescriptionField];

    [mNameField setNextKeyView:mLocationField];
    [mDockMenuCheckbox removeFromSuperview];
  }
  
  // Then, fill with appropriate values from Bookmarks
  nsAutoString value;
  [aBookmark contentNode]->GetAttr(kNameSpaceID_None, BookmarksService::gNameAtom, value);
  NSString* bookmarkName = [NSString stringWith_nsAString: value];
  [mNameField setStringValue: bookmarkName];
  NSString* infoForString = [NSString stringWithFormat:NSLocalizedString(@"BookmarkInfoTitle", @"Info for "), bookmarkName];
  [[self window] setTitle: infoForString];

  if (isFolder && !isGroup)
  {
    [mDockMenuCheckbox setState:([aBookmark isDockMenuRoot] ? NSOnState : NSOffState)];
  }
  
  if (!isGroup && !isFolder)
  {
    [aBookmark contentNode]->GetAttr(kNameSpaceID_None, BookmarksService::gHrefAtom, value);
    [mLocationField setStringValue: [NSString stringWith_nsAString: value]];
  }
    
  if (!isFolder)
  {
    [aBookmark contentNode]->GetAttr(kNameSpaceID_None, BookmarksService::gKeywordAtom, value);
    [mKeywordField setStringValue: [NSString stringWith_nsAString: value]];
  }

  [aBookmark contentNode]->GetAttr(kNameSpaceID_None, BookmarksService::gDescriptionAtom, value);
  [mDescriptionField setStringValue: [NSString stringWith_nsAString: value]];
  
  mBookmarkItem = aBookmark;  
}

-(BookmarkItem *)bookmark
{
  return mBookmarkItem;
}

-(void)showUIElementPair: (id)aLabel control:(id)aControl
{
  if ([aLabel superview] == nil)
    [mVariableFieldsContainer addSubview: aLabel];

  if ([aControl superview] == nil)
    [mVariableFieldsContainer addSubview: aControl];
    
  // we need to resize the fields in case the user resized the window when they were hidden
  NSRect containerBounds = [mVariableFieldsContainer bounds];
  NSRect controlFrame    = [aControl frame];
  controlFrame.size.width = (containerBounds.size.width - controlFrame.origin.x - 20.0);
  [aControl setFrame:controlFrame];
}

-(void)hideUIElementPair: (id)aLabel control:(id)aControl
{
  if ([aLabel superview] != nil)
    [aLabel removeFromSuperview];

  if ([aControl superview] != nil)
    [aControl removeFromSuperview];
}

-(NSText *)windowWillReturnFieldEditor:(NSWindow *)aPanel toObject:(id)aObject
{
  return mFieldEditor;
}

#pragma mark -

- (void)bookmarkAdded:(nsIContent*)bookmark inContainer:(nsIContent*)container
{
}

- (void)bookmarkRemoved:(nsIContent*)bookmark inContainer:(nsIContent*)container
{
  if ([mBookmarkItem contentNode] == bookmark)
    mBookmarkItem = nil;
}

- (void)bookmarkChanged:(nsIContent*)bookmark
{
}

- (void)specialFolder:(EBookmarksFolderType)folderType changedTo:(nsIContent*)newFolderContent
{
}

@end
