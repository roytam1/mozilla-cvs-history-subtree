/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 2002
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

#import "NSString+Utils.h"

#import "BrowserWindowController.h"
#import "HistoryDataSource.h"
#import "CHBrowserView.h"

#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFResource.h"

#include "nsXPIDLString.h"
#include "nsString.h"

#include "nsComponentManagerUtils.h"

//
// class HistoryDataSourceObserver
//
// An observer, enabled whenver the history panel in the drawer is selected. Tracks
// changes to the RDF data source and pokes the history outline view
//

class HistoryDataSourceObserver : public nsIRDFObserver
{
public:
  HistoryDataSourceObserver(NSOutlineView* outlineView) : 
    mOutlineView(outlineView), mEnabled(false) 
  { 
    NS_INIT_ISUPPORTS();
  }
  virtual ~HistoryDataSourceObserver() { } ;
  
  NS_DECL_ISUPPORTS

  NS_IMETHODIMP OnAssert(nsIRDFDataSource*, nsIRDFResource*, nsIRDFResource*, nsIRDFNode*) { return NS_OK; }
  NS_IMETHODIMP OnUnassert(nsIRDFDataSource*, nsIRDFResource*, nsIRDFResource*, nsIRDFNode*) { return NS_OK; }

  NS_IMETHODIMP OnMove(nsIRDFDataSource*, nsIRDFResource*, nsIRDFResource*, nsIRDFResource*, nsIRDFNode*) 
    { return NS_OK; }

  NS_IMETHODIMP OnChange(nsIRDFDataSource*, nsIRDFResource*, nsIRDFResource*, nsIRDFNode*, nsIRDFNode*);

  NS_IMETHODIMP BeginUpdateBatch(nsIRDFDataSource*) { return NS_OK; }
  NS_IMETHODIMP EndUpdateBatch(nsIRDFDataSource*) { return NS_OK; }

  void Enable() { mEnabled = true; }
  void Disable() { mEnabled = false; }

private:

  NSOutlineView* mOutlineView;
  bool mEnabled;
};

NS_IMPL_ISUPPORTS1(HistoryDataSourceObserver, nsIRDFObserver)

NS_IMETHODIMP
HistoryDataSourceObserver::OnChange(nsIRDFDataSource*, nsIRDFResource*, 
                                      nsIRDFResource* aProperty, nsIRDFNode*, nsIRDFNode*)
{
  if(mEnabled) {
    const char* p;
    aProperty->GetValueConst(&p);
    if (strcmp("http://home.netscape.com/NC-rdf#Date", p) == 0)
      [mOutlineView reloadData];
  }
  return NS_OK;
}


@implementation HistoryDataSource

- (void) dealloc
{
  if ( mDataSource && mObserver ) {
    mDataSource->RemoveObserver(mObserver);
    NS_RELEASE(mObserver);
  }
  [super dealloc];
}


//
// ensureDataSourceLoaded
//
// Called when the history panel is selected or brought into view. Inits all
// the RDF-fu to make history go. We defer loading everything because it's 
// sorta slow and we don't want to take the hit when the user creates new windows
// or just opens the bookmarks panel.
//
- (void) ensureDataSourceLoaded
{
  [super ensureDataSourceLoaded];
  
  NS_ASSERTION(mRDFService, "Uh oh, RDF service not loaded in parent class");
  
  if ( !mDataSource ) {
    // Get the Global History DataSource
    mRDFService->GetDataSource("rdf:history", &mDataSource);
    // Get the Date Folder Root
    mRDFService->GetResource("NC:HistoryByDate", &mRootResource);
  
    [mOutlineView setTarget: self];
    [mOutlineView setDoubleAction: @selector(openHistoryItem:)];
  
    mObserver = new HistoryDataSourceObserver(mOutlineView);
    if ( mObserver ) {
      NS_ADDREF(mObserver);
      mDataSource->AddObserver(mObserver);
    }
    [mOutlineView reloadData];
  }
  
  NS_ASSERTION(mDataSource, "Uh oh, History RDF Data source not created");
}

- (void) enableObserver
{
  if ( mObserver )
    mObserver->Enable();
}

-(void) disableObserver
{
  if ( mObserver )
    mObserver->Disable();
}


//
// createCellContents:withColumn:byItem
//
// override to create an NSAttributedString instead of just the string with the
// given text. We add an icon and adjust the positioning of the text w/in the cell
//
-(id) createCellContents:(const nsAString&)inValue withColumn:(NSString*)inColumn byItem:(id) inItem
{
  NSMutableAttributedString *cellValue = [[NSMutableAttributedString alloc] init];

	//Set cell's textual contents
  [cellValue replaceCharactersInRange:NSMakeRange(0, [cellValue length]) withString:[NSString stringWith_nsAString:inValue]];
  
  if ([inColumn isEqualToString: @"http://home.netscape.com/NC-rdf#Name"]) {
    NSMutableAttributedString *attachmentAttrString = nil;
    NSFileWrapper *fileWrapper = [[NSFileWrapper alloc] initRegularFileWithContents:nil];
    NSTextAttachment *textAttachment = [[NSTextAttachment alloc] initWithFileWrapper:fileWrapper];
    NSCell *attachmentAttrStringCell;

    //Create an attributed string to hold the empty attachment, then release the components.
    attachmentAttrString = [[NSMutableAttributedString attributedStringWithAttachment:textAttachment] retain];
    [textAttachment release];
    [fileWrapper release];

    //Get the cell of the text attachment.
    attachmentAttrStringCell = (NSCell *)[(NSTextAttachment *)[attachmentAttrString attribute:
              NSAttachmentAttributeName atIndex:0 effectiveRange:nil] attachmentCell];

    if ([self outlineView:mOutlineView isItemExpandable:inItem]) {
      [attachmentAttrStringCell setImage:[NSImage imageNamed:@"folder"]];
    }
    else
      [attachmentAttrStringCell setImage:[NSImage imageNamed:@"smallbookmark"]];

    //Insert the image
    [cellValue replaceCharactersInRange:NSMakeRange(0, 0) withAttributedString:attachmentAttrString];

    //Tweak the baseline to vertically center the text.
    [cellValue addAttribute:NSBaselineOffsetAttributeName
                      value:[NSNumber numberWithFloat:-5.0]
                      range:NSMakeRange(0, 1)];
  }

  return cellValue;
}


-(IBAction)openHistoryItem: (id)aSender
{
    int index = [mOutlineView selectedRow];
    if (index == -1)
        return;
    
    id item = [mOutlineView itemAtRow: index];
    if (!item)
        return;
    
    // expand if collapsed and double click
    if ([mOutlineView isExpandable: item]) {
      if ([mOutlineView isItemExpanded: item])
        [mOutlineView collapseItem: item];
      else
        [mOutlineView expandItem: item];
        
      return;
    }
    
    nsCOMPtr<nsIRDFResource> propertyResource;
    mRDFService->GetResource("http://home.netscape.com/NC-rdf#URL", getter_AddRefs(propertyResource));
    
    nsCOMPtr<nsIRDFResource> resource = dont_AddRef([item resource]);
            
    nsCOMPtr<nsIRDFNode> valueNode;
    mDataSource->GetTarget(resource, propertyResource, PR_TRUE, getter_AddRefs(valueNode));
    if (!valueNode) {
#if DEBUG
        NSLog(@"ValueNode is null in openHistoryItem");
#endif
        return;
    }
    
    nsCOMPtr<nsIRDFLiteral> valueLiteral(do_QueryInterface(valueNode));
    if (!valueLiteral)
        return;
    
    nsXPIDLString literalValue;
    valueLiteral->GetValue(getter_Copies(literalValue));

    // get uri
    NSString* url = [NSString stringWith_nsAString: literalValue];
    [[mBrowserWindowController getBrowserWrapper] loadURI: url referrer: nil flags: NSLoadFlagsNone activate:YES];
}

@end
