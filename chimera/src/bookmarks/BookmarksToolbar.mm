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
*   David Hyatt <hyatt@netscape.com> (Original Author)
*   Kathy Brade <brade@netscape.com>
*   David Haas  <haasd@cae.wisc.edu>
*/

#import "BookmarksButton.h"
#import "BookmarksToolbar.h"
#import "BookmarksService.h"
#import "BookmarksDataSource.h"

#include "nsIDOMElement.h"
#include "nsIContent.h"

@interface BookmarksToolbar(Private)

- (void)setButtonInsertionPoint:(id <NSDraggingInfo>)sender;
- (NSRect)insertionRectForButton:(NSView*)aButton position:(int)aPosition;
- (BookmarksButton*)makeNewButtonWithElement:(nsIDOMElement*)element;

@end

@implementation BookmarksToolbar

- (id)initWithFrame:(NSRect)frame
{
  if ( (self = [super initWithFrame:frame]) ) {
    mBookmarks = nsnull;
    mButtons = [[NSMutableArray alloc] init];
    mDragInsertionButton = nil;
    mDragInsertionPosition = BookmarksService::CHInsertNone;
    [self registerForDraggedTypes:[NSArray arrayWithObjects:@"MozURLType", @"MozBookmarkType", NSStringPboardType, nil]];
    mIsShowing = YES;
  }
  return self;
}

-(void)initializeToolbar
{
  // Initialization code here.
  mBookmarks = new BookmarksService(self);
  mBookmarks->AddObserver();
  mBookmarks->EnsureToolbarRoot();
  [self buildButtonList];
}

-(void) dealloc
{
  [mButtons autorelease];
  mBookmarks->RemoveObserver();
  delete mBookmarks;
  [super dealloc];
}

- (void)drawRect:(NSRect)aRect {
  // Fill the background with our background color.
  //[[NSColor colorWithCalibratedWhite: 0.98 alpha: 1.0] set];
  //NSRectFill(aRect);

  //printf("The rect is: %f %f %f %f\n", aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height);
  
  if (aRect.origin.y + aRect.size.height ==
      [self bounds].size.height) {
    // The personal toolbar is 21 pixels tall.  The bottom two pixels
    // are a separator.
    [[NSColor colorWithCalibratedWhite: 0.90 alpha: 1.0] set];
    //NSRectFill(NSMakeRect(aRect.origin.x, [self bounds].size.height-2, aRect.size.width, [self bounds].size.height));
  }

  // The buttons will paint themselves. Just call our base class method.
  [super drawRect: aRect];
  
  // draw a separator at drag n drop insertion point if there is one
  if (mDragInsertionPosition) {
    [[[NSColor controlShadowColor] colorWithAlphaComponent:0.6] set];
    NSRectFill([self insertionRectForButton:mDragInsertionButton position:mDragInsertionPosition]);
  }
}

-(void)buildButtonList
{
  // Build the buttons, and then lay them all out.
  nsCOMPtr<nsIDOMNode> child;
  BookmarksService::gToolbarRoot->GetFirstChild(getter_AddRefs(child));
  while (child) {
    nsCOMPtr<nsIDOMElement> childElt(do_QueryInterface(child));
    if (childElt) {
      BookmarksButton* button = [self makeNewButtonWithElement:childElt];
      [self addSubview: button];
      [mButtons addObject: button];
    }

    nsCOMPtr<nsIDOMNode> temp = child;
    temp->GetNextSibling(getter_AddRefs(child));
  }

  if ([self isShown])
    [self reflowButtons];
}

-(void)addButton: (nsIDOMElement*)aElt atIndex: (int)aIndex
{
  BookmarksButton* button = [self makeNewButtonWithElement:aElt];
  [self addSubview: button];
  [mButtons insertObject: button atIndex: aIndex];
  if ([self isShown])
    [self reflowButtonsStartingAtIndex: aIndex];
}

-(void)editButton: (nsIDOMElement*)aElt
{
  int count = [mButtons count];
  for (int i = 0; i < count; i++) {
    BookmarksButton* button = [mButtons objectAtIndex: i];
    if ([button element] == aElt) {
      [button setElement: aElt];
      if (count > i && [self isShown])
        [self reflowButtonsStartingAtIndex: i];
      break;
    }
  }

  [self setNeedsDisplay: [self isShown]];
}

-(void)removeButton: (nsIDOMElement*)aElt
{
  int count = [mButtons count];
  for (int i = 0; i < count; i++) {
    BookmarksButton* button = [mButtons objectAtIndex: i];
    if ([button element] == aElt) {
      [mButtons removeObjectAtIndex: i];
      [button removeFromSuperview];
      if (count > i && [self isShown])
        [self reflowButtonsStartingAtIndex: i];
      break;
    }
  }

  [self setNeedsDisplay: [self isShown]];
}

-(void)reflowButtons
{
  [self reflowButtonsStartingAtIndex: 0];
}

#define kBookmarkButtonHeight            16.0
#define kMinBookmarkButtonWidth          16.0
#define kMaxBookmarkButtonWidth         150.0
#define kBookmarkButtonHorizPadding       2.0
#define kBookmarkButtonVerticalPadding    1.0

-(void)reflowButtonsStartingAtIndex: (int)aIndex
{
  // coordinates for this view are flipped, making it easier to lay out from top left
  // to bottom right.
  float oldHeight = [self frame].size.height;
  int   count         = [mButtons count];
  float curRowYOrigin = 0.0;
  float curX          = 0.0;
  
  for (int i = 0; i < count; i ++)
  {
    BookmarksButton* button = [mButtons objectAtIndex: i];
    NSRect           buttonRect;
  
    if (i < aIndex)
    {
      buttonRect = [button frame];
      curRowYOrigin = NSMinY(buttonRect) - kBookmarkButtonVerticalPadding;
      curX = NSMaxX(buttonRect) + kBookmarkButtonHorizPadding;
    }
    else
    {
      [button sizeToFit];
      float width = [button frame].size.width;
  
      if (width > kMaxBookmarkButtonWidth)
        width = kMaxBookmarkButtonWidth;
  
      buttonRect = NSMakeRect(curX, curRowYOrigin + kBookmarkButtonVerticalPadding, width, kBookmarkButtonHeight);
      curX += NSWidth(buttonRect) + kBookmarkButtonHorizPadding;
      
      if (NSMaxX(buttonRect) > NSWidth([self bounds]))
      {
        curRowYOrigin += (kBookmarkButtonHeight + 2 * kBookmarkButtonVerticalPadding);
        buttonRect = NSMakeRect(0.0, curRowYOrigin + kBookmarkButtonVerticalPadding, width, kBookmarkButtonHeight);
        curX = NSWidth(buttonRect);
      }
      
      [button setFrame: buttonRect];
    }
  }
  
  float computedHeight = curRowYOrigin + (kBookmarkButtonHeight + 2 * kBookmarkButtonVerticalPadding);
  
  // our size has changed, readjust our view's frame and the content area
  if (computedHeight != oldHeight)
  {
    [self setFrame: NSMakeRect([self frame].origin.x, [self frame].origin.y + (oldHeight - computedHeight),
                               [self frame].size.width, computedHeight)];
    [self setNeedsDisplay: [self isShown]];
    
    // adjust the content area.
    float sizeChange = computedHeight - oldHeight;
    NSView* view = [[[self window] windowController] getTabBrowser];
    [view setFrame: NSMakeRect([view frame].origin.x, [view frame].origin.y,
                               [view frame].size.width, [view frame].size.height - sizeChange)];
  }
}

-(BOOL)isFlipped
{
  return YES; // Use flipped coords, so we can layout out from top row to bottom row.
}

-(void)setFrame:(NSRect)aRect
{
  NSRect oldFrame = [self frame];
  [super setFrame:aRect];

  if (oldFrame.size.width == aRect.size.width || aRect.size.height == 0)
    return;

  int count = [mButtons count];
  if (count <= 2)
    return; // We have too few buttons to care.
  
  // Do some optimizations when we have only one row.
  if (aRect.size.height < 25) // We have only one row.
  {
    if (oldFrame.size.width < aRect.size.width)
      // We got bigger.  If we already only have one row, just bail.
      //	This will optimize for a common resizing case.
      return;
    else {
      // We got smaller.  Just go to the last button and see if it is outside
      // our bounds.
      BookmarksButton* button = [mButtons objectAtIndex:(count-1)];
      if ([button frame].origin.x + [button frame].size.width >
          [self bounds].size.width - 2) {
        // The button doesn't fit any more.  Reflow starting at this index.
        [self reflowButtonsStartingAtIndex:(count-1)];
      }
    }
  }
  else {
    // See if we got bigger or smaller.  We could gain or lose a row.
    [self reflowButtons];
  }
}

-(BOOL)isShown
{
  return mIsShowing;
}

-(void)showBookmarksToolbar: (BOOL)aShow
{
  if (!aShow)
  {
    float height = [self bounds].size.height;
    [self setFrame: NSMakeRect([self frame].origin.x, [self frame].origin.y + height,
                               [self frame].size.width, 0)];
    // We need to adjust the content area.
    NSView* view = [[[self window] windowController] getTabBrowser];
    [view setFrame: NSMakeRect([view frame].origin.x, [view frame].origin.y,
                               [view frame].size.width, [view frame].size.height + height)];
  }
  else
    // Reflowing the buttons will do the right thing.
    [self reflowButtons];
    
  mIsShowing = aShow;
}

- (void)setButtonInsertionPoint:(id <NSDraggingInfo>)sender
{
  NSPoint   dragLocation  = [sender draggingLocation];
  NSPoint   superviewLoc  = [[self superview] convertPoint:dragLocation fromView:nil]; // convert from window
  NSButton* sourceButton  = [sender draggingSource];
  
  mDragInsertionButton = nsnull;
  mDragInsertionPosition = BookmarksService::CHInsertAfter;
  
  NSView* foundView = [self hitTest:superviewLoc];
  if (foundView && [foundView isMemberOfClass:[BookmarksButton class]])
  {
    BookmarksButton* targetButton = foundView;

    nsCOMPtr<nsIAtom> tagName;
    nsCOMPtr<nsIContent> contentNode = do_QueryInterface([targetButton element]);
    contentNode->GetTag(*getter_AddRefs(tagName));

    if (tagName == BookmarksService::gFolderAtom)
    {
      mDragInsertionButton = targetButton;
      mDragInsertionPosition = BookmarksService::CHInsertInto;
    }
    else if (targetButton != sourceButton)
    {
      NSPoint	localLocation = [[self superview] convertPoint:superviewLoc toView:foundView];
      
      mDragInsertionButton = targetButton;
      
      if (localLocation.x < NSWidth([targetButton bounds]) / 2.0)
        mDragInsertionPosition = BookmarksService::CHInsertBefore;
      else
        mDragInsertionPosition = BookmarksService::CHInsertAfter;
    }
  }
  else
  {
    // throw it in at the end
    mDragInsertionButton   = [mButtons objectAtIndex:[mButtons count] - 1];    
    mDragInsertionPosition = BookmarksService::CHInsertAfter;
  }
}

- (BOOL)dropDestinationValid:(id <NSDraggingInfo>)sender
{
  NSPasteboard* draggingPasteboard = [sender draggingPasteboard];
  NSArray*      types = [draggingPasteboard types];

  if ([types containsObject: @"MozBookmarkType"]) 
  {
    NSArray *draggedIDs = [draggingPasteboard propertyListForType: @"MozBookmarkType"];
    
    nsCOMPtr<nsIContent> destinationContent;
    int index = 0;
    
    if (mDragInsertionPosition == BookmarksService::CHInsertInto)						// drop onto folder
    {
      nsCOMPtr<nsIDOMElement> parentElt = [mDragInsertionButton element];
      destinationContent = do_QueryInterface(parentElt);
      index = 0;
    }
    else if (mDragInsertionPosition == BookmarksService::CHInsertBefore ||
             mDragInsertionPosition == BookmarksService::CHInsertAfter)		// drop onto toolbar
    {
      nsCOMPtr<nsIDOMElement> toolbarRoot = BookmarksService::gToolbarRoot;
      destinationContent = do_QueryInterface(toolbarRoot);
      index = [mButtons indexOfObject: mDragInsertionButton];
      if (mDragInsertionPosition == BookmarksService::CHInsertAfter)
        ++index;
    }

    // we rely on IsBookmarkDropValid to filter out drops where the source
    // and destination are the same. 
    BookmarkItem* toolbarFolderItem = BookmarksService::GetWrapperFor(destinationContent);
    if (!BookmarksService::IsBookmarkDropValid(toolbarFolderItem, index, draggedIDs)) {
      return NO;
    }
  }
  
  return YES;
}

// NSDraggingDestination ///////////

- (unsigned int)draggingEntered:(id <NSDraggingInfo>)sender
{
  // we have to set the drag target before we can test for drop validation
  [self setButtonInsertionPoint:sender];

  if (![self dropDestinationValid:sender]) {
    mDragInsertionButton = nil;
    mDragInsertionPosition = BookmarksService::CHInsertNone;
    return NSDragOperationNone;
  }
    
  return NSDragOperationGeneric;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
  if (mDragInsertionPosition)
    [self setNeedsDisplayInRect:[self insertionRectForButton:mDragInsertionButton position:mDragInsertionPosition]];

  mDragInsertionButton = nil;
  mDragInsertionPosition = BookmarksService::CHInsertNone;
}

- (unsigned int)draggingUpdated:(id <NSDraggingInfo>)sender
{
  if (mDragInsertionPosition)
    [self setNeedsDisplayInRect:[self insertionRectForButton:mDragInsertionButton position:mDragInsertionPosition]];

  // we have to set the drag target before we can test for drop validation
  [self setButtonInsertionPoint:sender];
  
  if (![self dropDestinationValid:sender]) {
    mDragInsertionButton = nil;
    mDragInsertionPosition = BookmarksService::CHInsertNone;
    return NSDragOperationNone;
  }
  
  if (mDragInsertionPosition)
    [self setNeedsDisplayInRect:[self insertionRectForButton:mDragInsertionButton position:mDragInsertionPosition]];

  return NSDragOperationGeneric;
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
  return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
  BookmarkItem* parent = nsnull;
  int index = 0;

  if (mDragInsertionPosition == BookmarksService::CHInsertInto) {						// drop onto folder
    nsCOMPtr<nsIDOMElement> parentElt = [mDragInsertionButton element];
    nsCOMPtr<nsIContent> parentContent(do_QueryInterface(parentElt));
    parent = BookmarksService::GetWrapperFor(parentContent);
    index = 0;
  }
  else if (mDragInsertionPosition == BookmarksService::CHInsertBefore ||
             mDragInsertionPosition == BookmarksService::CHInsertAfter) {		// drop onto toolbar
    nsCOMPtr<nsIDOMElement> rootElt = BookmarksService::gToolbarRoot;
    nsCOMPtr<nsIContent> rootContent(do_QueryInterface(rootElt));
    parent = BookmarksService::GetWrapperFor(rootContent);
    index = [mButtons indexOfObject: mDragInsertionButton];
    if (index == NSNotFound)
      rootContent->ChildCount(index);
    else if (mDragInsertionPosition == BookmarksService::CHInsertAfter)
      index++;
    } else {
      mDragInsertionButton = nil;
      mDragInsertionPosition = BookmarksService::CHInsertNone;
      [self setNeedsDisplay:YES];
      return NO;
    }

  BOOL dropHandled = NO;
  NSArray	*draggedTypes = [[sender draggingPasteboard] types];
  if ( [draggedTypes containsObject:@"MozBookmarkType"] )
  {
    NSArray *draggedItems = [[sender draggingPasteboard] propertyListForType: @"MozBookmarkType"];
    dropHandled = BookmarksService::PerformBookmarkDrop(parent, index, draggedItems);
  }
  else if ( [draggedTypes containsObject:@"MozURLType"] )
  {
    NSDictionary* proxy = [[sender draggingPasteboard] propertyListForType: @"MozURLType"];
    nsCOMPtr<nsIContent> beforeContent;
    [parent contentNode]->ChildAt(index, *getter_AddRefs(beforeContent));
    BookmarkItem* beforeItem = mBookmarks->GetWrapperFor(beforeContent);		// can handle nil content
    dropHandled = BookmarksService::PerformProxyDrop(parent, beforeItem, proxy);
	}
  else if ( [draggedTypes containsObject:NSStringPboardType] )
  {
    NSString* draggedText = [[sender draggingPasteboard] stringForType:NSStringPboardType];
    nsCOMPtr<nsIContent> beforeContent;
    [parent contentNode]->ChildAt(index, *getter_AddRefs(beforeContent));
    BookmarkItem* beforeItem = mBookmarks->GetWrapperFor(beforeContent);		// can handle nil content
    // maybe fix URL drags to include the selected text as the title
    dropHandled = BookmarksService::PerformURLDrop(parent, beforeItem, draggedText, draggedText);
  }
  
  mDragInsertionButton = nil;
  mDragInsertionPosition = BookmarksService::CHInsertNone;
  [self setNeedsDisplay: [self isShown]];

  return dropHandled;
}

- (NSRect)insertionRectForButton:(NSView*)aButton position:(int) aPosition
{
  if (aPosition == BookmarksService::CHInsertInto) {
    return NSMakeRect([aButton frame].origin.x, [aButton frame].origin.y, [aButton frame].size.width, [aButton frame].size.height);
  } else if (aPosition == BookmarksService::CHInsertAfter) {
    return NSMakeRect([aButton frame].origin.x+[aButton frame].size.width, [aButton frame].origin.y, 2, [aButton frame].size.height);
  } else {// if (aPosition == BookmarksService::CHInsertBefore) {
    return NSMakeRect([aButton frame].origin.x - 2, [aButton frame].origin.y, 2, [aButton frame].size.height);
  }
}

- (BookmarksButton*)makeNewButtonWithElement:(nsIDOMElement*)element
{
	return [[[BookmarksButton alloc] initWithFrame: NSMakeRect(2, 1, 100, 17) element:element bookmarksService:mBookmarks] autorelease];
}

@end
