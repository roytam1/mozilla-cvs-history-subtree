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

#import "BookmarksService.h"

#import "PreferenceManager.h"
#import "CHBrowserView.h"
#import "CHBrowserService.h"
#import "SiteIconProvider.h"

#include "nsCRT.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsITextContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMParser.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPrefBranch.h"
#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIXMLHttpRequest.h"
#include "nsIDOMSerializer.h"
#include "nsIDocumentEncoder.h"
#include "nsNetUtil.h"
#include "nsINamespaceManager.h"
#include "nsIXBLService.h"
#include "nsIWebBrowser.h"


// Helper for stripping whitespace
static void StripWhitespaceNodes(nsIContent* aElement)
{
  PRInt32 childCount = 0;
  aElement->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    aElement->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsITextContent> text = do_QueryInterface(child);
    if (text) {
      PRBool isEmpty = PR_FALSE;
      text->IsOnlyWhitespace(&isEmpty);
      if (isEmpty) {
        // This node contained nothing but whitespace.
        // Remove it from the content model.
        aElement->RemoveChildAt(i, PR_TRUE);
        i--; // Decrement our count, since we just removed this child.
        childCount--; // Also decrement our total count.
      }
    }
    else
      StripWhitespaceNodes(child);
  }
}

static bool ElementIsOrContains(nsIDOMElement* inSearchRoot, nsIDOMElement* inFindElt)
{
  nsCOMPtr<nsIContent> curContent = do_QueryInterface(inSearchRoot);
  if (!curContent) return false;

  if (inSearchRoot == inFindElt)
    return true;
    
  // recurse to children
  PRInt32 numChildren;
  curContent->ChildCount(numChildren);
  for (PRInt32 i = 0; i < numChildren; i ++)
  {
    nsCOMPtr<nsIContent> curChild;
    curContent->ChildAt(i, *getter_AddRefs(curChild));
    
    nsCOMPtr<nsIDOMElement> curElt = do_QueryInterface(curChild);
    if (ElementIsOrContains(curElt, inFindElt))
      return true;
  }
  
  return false;
}

static bool SearchChildrenForMatchingFolder(nsIDOMElement* inCurElt, const nsAString& inAttribute, const nsAString& inValue, nsIDOMElement** foundElt)
{
  nsCOMPtr<nsIContent> curContent = do_QueryInterface(inCurElt);
  if (!curContent) return false;
  
  // sanity check
  nsCOMPtr<nsIAtom> tagName;
  curContent->GetTag(*getter_AddRefs(tagName));
  if (tagName == BookmarksService::gFolderAtom)
  {
    nsAutoString attribValue;
    inCurElt->GetAttribute(inAttribute, attribValue);

    if (attribValue.Equals(inValue))
    {
      *foundElt = inCurElt;
      NS_ADDREF(*foundElt);
      return true;
    }
  }

  // recurse to children
  PRInt32 numChildren;
  curContent->ChildCount(numChildren);
  for (PRInt32 i = 0; i < numChildren; i ++)
  {
    nsCOMPtr<nsIContent> curChild;
    curContent->ChildAt(i, *getter_AddRefs(curChild));
    
    nsCOMPtr<nsIDOMElement> curElt = do_QueryInterface(curChild);
    if (SearchChildrenForMatchingFolder(curElt, inAttribute, inValue, foundElt))
      return true;
  }
  
  return false;
}

#pragma mark -

static nsIDocument* gBookmarksDocument = nsnull;

// A dictionary that maps from content IDs (which uniquely identify content nodes)
// to Obj-C bookmarkItem objects.  These objects are handed back to UI elements like
// the outline view.
static NSMutableDictionary* gBookmarkItemDictionary = nil;          // maps content IDs to items
static NSMutableDictionary* gBookmarkFaviconURLDictionary = nil;    // maps favicon urls to dicts of items

static NSMutableArray* gBookmarkClientsArray = nsnull;

nsIDOMElement* BookmarksService::gToolbarRoot = nsnull;
nsIDOMElement* BookmarksService::gDockMenuRoot = nsnull;

nsIAtom* BookmarksService::gBookmarkAtom = nsnull;
nsIAtom* BookmarksService::gDescriptionAtom = nsnull;
nsIAtom* BookmarksService::gFolderAtom = nsnull;
nsIAtom* BookmarksService::gGroupAtom = nsnull;
nsIAtom* BookmarksService::gHrefAtom = nsnull;
nsIAtom* BookmarksService::gKeywordAtom = nsnull;
nsIAtom* BookmarksService::gNameAtom = nsnull;
nsIAtom* BookmarksService::gOpenAtom = nsnull;


BOOL BookmarksService::gBookmarksFileReadOK = NO;

int BookmarksService::CHInsertNone = 0;
int BookmarksService::CHInsertInto = 1;
int BookmarksService::CHInsertBefore = 2;
int BookmarksService::CHInsertAfter = 3;

const int BookmarksService::kBookmarksDividerTag = -1;

void
BookmarksService::Init()
{
  static bool sInitted = false;
  if (sInitted) return;
  
  sInitted = true;
  gBookmarkAtom = NS_NewAtom("bookmark");
  gFolderAtom = NS_NewAtom("folder");
  gNameAtom = NS_NewAtom("name");
  gHrefAtom = NS_NewAtom("href");
  gOpenAtom = NS_NewAtom("open");
  gKeywordAtom = NS_NewAtom("id");
  gDescriptionAtom = NS_NewAtom("description");
  gGroupAtom = NS_NewAtom("group");
  
  ReadBookmarks();
}


void
BookmarksService::Shutdown()
{
  [gBookmarkClientsArray release];
  gBookmarkClientsArray = nil;
  
  // we should have already written bookmarks in response to a shutdown notification
  NS_IF_RELEASE(gBookmarksDocument);

  NS_RELEASE(gBookmarkAtom);
  NS_RELEASE(gFolderAtom);
  NS_RELEASE(gNameAtom);
  NS_RELEASE(gHrefAtom);
  NS_RELEASE(gOpenAtom);
  NS_RELEASE(gKeywordAtom);
  NS_RELEASE(gDescriptionAtom);
  NS_RELEASE(gGroupAtom);
  
  [gBookmarkItemDictionary release];
  gBookmarkItemDictionary = nil;

  [gBookmarkFaviconURLDictionary release];
  gBookmarkFaviconURLDictionary = nil;
}

#pragma mark -

void
BookmarksService::GetRootContent(nsIContent** aResult)
{
  *aResult = nsnull;
  if (gBookmarksDocument) {
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(gBookmarksDocument));
    if (!domDoc) return;

    nsCOMPtr<nsIDOMElement> elt;
    domDoc->GetDocumentElement(getter_AddRefs(elt));
    if (elt)
      elt->QueryInterface(NS_GET_IID(nsIContent), (void**)aResult); // Addref happens here.
  }
}

BookmarkItem*
BookmarksService::GetRootItem()
{
  nsCOMPtr<nsIContent> rootContent;
  BookmarksService::GetRootContent(getter_AddRefs(rootContent));
  BookmarkItem* rootItem = BookmarksService::GetWrapperFor(rootContent);
  return rootItem;
}

BookmarkItem*
BookmarksService::GetWrapperFor(nsIContent* aContent)
{
  if ( !aContent )
    return nil;
    
  if (!gBookmarkItemDictionary)
    gBookmarkItemDictionary = [[NSMutableDictionary alloc] initWithCapacity: 30];

  PRUint32 contentID = 0;
  aContent->GetContentID(&contentID);

  BookmarkItem* item = BookmarksService::GetWrapperFor(contentID);
  if (item)
    return item;

  // Create an item.
  item = [[BookmarkItem alloc] init]; // The dictionary retains us.
  [item setContentNode: aContent];
  [gBookmarkItemDictionary setObject: item forKey: [NSNumber numberWithInt: contentID]];
  [item release];
  return item;
}

BookmarkItem*
BookmarksService::GetWrapperFor(PRUint32 contentID)
{
  return [gBookmarkItemDictionary objectForKey: [NSNumber numberWithUnsignedInt: contentID]];
}

void
BookmarksService::BookmarkAdded(nsIContent* aContainer, nsIContent* aChild, bool shouldFlush)
{
  if (!gBookmarkItemDictionary)
    return;

  unsigned int numClients = [gBookmarkClientsArray count];
  for (unsigned int i = 0; i < numClients; i ++)
  {
    id<BookmarksClient> client = [gBookmarkClientsArray objectAtIndex:i];
    [client bookmarkAdded:aChild inContainer:aContainer];
  }
  
  if (shouldFlush)
    FlushBookmarks();  
}

void
BookmarksService::BookmarkChanged(nsIContent* aItem, bool shouldFlush)
{
  if (!gBookmarkItemDictionary)
    return;

  unsigned int numClients = [gBookmarkClientsArray count];
  for (unsigned int i = 0; i < numClients; i ++)
  {
    id<BookmarksClient> client = [gBookmarkClientsArray objectAtIndex:i];
    [client bookmarkChanged:aItem];
  }

  if (shouldFlush)
    FlushBookmarks();  
}

void
BookmarksService::BookmarkRemoved(nsIContent* aContainer, nsIContent* aChild, bool shouldFlush)
{
  unsigned int numClients = [gBookmarkClientsArray count];
  for (unsigned int i = 0; i < numClients; i ++)
  {
    id<BookmarksClient> client = [gBookmarkClientsArray objectAtIndex:i];
    [client bookmarkRemoved:aChild inContainer:aContainer];
  }

  if (shouldFlush)
      FlushBookmarks(); 
}

void
BookmarksService::SpecialBookmarkChanged(EBookmarksFolderType aFolderType, nsIContent* aNewContent)
{
  unsigned int numClients = [gBookmarkClientsArray count];
  for (unsigned int i = 0; i < numClients; i ++)
  {
    id<BookmarksClient> client = [gBookmarkClientsArray objectAtIndex:i];
    [client specialFolder:aFolderType changedTo:aNewContent];
  }
}

void
BookmarksService::AddBookmarkToFolder(const nsString& aURL, const nsString& aTitle, nsIDOMElement* aFolder, nsIDOMElement* aBeforeElt)
{
  // XXX if no folder provided, default to root folder
  if (!aFolder) return;
  
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(gBookmarksDocument));
  if (!domDoc) return;
  
  nsCOMPtr<nsIDOMElement> elt;
  domDoc->CreateElementNS(NS_LITERAL_STRING("http://chimera.mozdev.org/bookmarks/"),
                          NS_LITERAL_STRING("bookmark"),
                          getter_AddRefs(elt));

  elt->SetAttribute(NS_LITERAL_STRING("name"), aTitle);
  elt->SetAttribute(NS_LITERAL_STRING("href"), aURL);

  MoveBookmarkToFolder(elt, aFolder, aBeforeElt);
}

void
BookmarksService::MoveBookmarkToFolder(nsIDOMElement* aBookmark, nsIDOMElement* aFolder, nsIDOMElement* aBeforeElt)
{
  if (!aBookmark || !aFolder) return;
  
  nsCOMPtr<nsIDOMNode> oldParent;
  aBookmark->GetParentNode(getter_AddRefs(oldParent));

  nsCOMPtr<nsIDOMNode> dummy;
  if (oldParent) {
    nsCOMPtr<nsIDOMNode> bookmarkNode = do_QueryInterface(aBookmark);
    oldParent->RemoveChild(bookmarkNode, getter_AddRefs(dummy));
  }

  if (aBeforeElt) {
    aFolder->InsertBefore(aBookmark, aBeforeElt, getter_AddRefs(dummy));
  } else {
    aFolder->AppendChild(aBookmark, getter_AddRefs(dummy));
  }
  
  nsCOMPtr<nsIContent> childContent(do_QueryInterface(aBookmark));
  nsCOMPtr<nsIContent> parentContent(do_QueryInterface(aFolder));

  if (oldParent) {
    nsCOMPtr<nsIContent> oldParentContent(do_QueryInterface(oldParent));
    BookmarkRemoved(oldParentContent, childContent);
  }
  
  BookmarkAdded(parentContent, childContent);
}

void
BookmarksService::DeleteBookmark(nsIDOMElement* aBookmark)
{
  if (!aBookmark || aBookmark == gToolbarRoot) return;
  
  if (ElementIsOrContains(aBookmark, gDockMenuRoot))
  {
    gDockMenuRoot = gToolbarRoot;
    nsCOMPtr<nsIContent> menuRootContent = do_QueryInterface(gDockMenuRoot);
    BookmarksService::SpecialBookmarkChanged(eBookmarksFolderDockMenu, menuRootContent);
  }

  nsCOMPtr<nsIDOMNode> oldParent;
  aBookmark->GetParentNode(getter_AddRefs(oldParent));

  if (oldParent) {
    nsCOMPtr<nsIDOMNode> dummy;
    nsCOMPtr<nsIDOMNode> bookmarkNode = do_QueryInterface(aBookmark);
    oldParent->RemoveChild(bookmarkNode, getter_AddRefs(dummy));

    nsCOMPtr<nsIContent> childContent(do_QueryInterface(aBookmark));
    nsCOMPtr<nsIContent> oldParentContent(do_QueryInterface(oldParent));
    BookmarkRemoved(oldParentContent, childContent);
  }
}

static PRBool
CheckXMLDocumentParseSuccessful(nsIDOMDocument* inDOMDoc)
{
  nsCOMPtr<nsIDOMElement> docElement;
  inDOMDoc->GetDocumentElement(getter_AddRefs(docElement));
  if (!docElement)
    return PR_FALSE;
  
  nsCOMPtr<nsIAtom> tagName;
  nsCOMPtr<nsIContent>   docContent = do_QueryInterface(docElement);
  docContent->GetTag(*getter_AddRefs(tagName));

  nsCOMPtr<nsIAtom>	parserErrorAtom = do_GetAtom("parsererror");
  return (tagName != parserErrorAtom);
}

static PRBool
ValidateXMLDocument(nsIDOMDocument* inDOMDoc)
{
  if (!inDOMDoc)
    return PR_FALSE;

  nsCOMPtr<nsIDOMElement> elt;
  inDOMDoc->GetDocumentElement(getter_AddRefs(elt));
  if (!elt)
    return PR_FALSE;

  nsCOMPtr<nsIDOMSerializer> domSerializer = do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID);
  if (!domSerializer)
    return PR_FALSE;
  
  nsXPIDLString encodedDocStr;
  nsresult rv = domSerializer->SerializeToString(inDOMDoc, getter_Copies(encodedDocStr));
  if (NS_FAILED(rv))
    return PR_FALSE;
  
  nsCOMPtr<nsIDOMParser> domParser = do_CreateInstance(NS_DOMPARSER_CONTRACTID);
  if (!domParser)
    return PR_FALSE;

  nsCOMPtr<nsIDOMDocument> newDomDoc;
  domParser->ParseFromString(encodedDocStr.get(), "text/xml", getter_AddRefs(newDomDoc));
  if (newDomDoc)
   	return CheckXMLDocumentParseSuccessful(newDomDoc);
  
  return PR_FALSE;
}

void
BookmarksService::ReadBookmarks()
{
  nsCOMPtr<nsIFile> profileDirBookmarks;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDirBookmarks));
  if (!profileDirBookmarks) return;
  
  profileDirBookmarks->Append(NS_LITERAL_STRING("bookmarks.xml"));
  
  PRBool fileExists = PR_FALSE;
  profileDirBookmarks->Exists(&fileExists);
  
  // If the bookmarks file does not exist, copy from the defaults so we don't
  // crash or anything dumb like that. 
  if (!fileExists) {
    nsCOMPtr<nsIFile> defaultBookmarksFile;
    NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_50_DIR, getter_AddRefs(defaultBookmarksFile));
    defaultBookmarksFile->Append(NS_LITERAL_STRING("bookmarks.xml"));
    
    // XXX for some reason unknown to me, leaving this code in causes the program to crash
    //     with 'cannot dereference null COMPtr.'
#if I_WANT_TO_CRASH
    PRBool defaultFileExists;
    defaultBookmarksFile->Exists(&defaultFileExists);
    if (defaultFileExists)
      return;
#endif
  
    nsCOMPtr<nsIFile> profileDirectory;
    NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDirectory));
  
    defaultBookmarksFile->CopyToNative(profileDirectory, NS_LITERAL_CSTRING("bookmarks.xml"));
  }
  
  nsCAutoString bookmarksFileURL;
  NS_GetURLSpecFromFile(profileDirBookmarks, bookmarksFileURL);
  
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), bookmarksFileURL.get());
  
  // XXX this is somewhat lame. we have no way of knowing whether or not the parse succeeded
  //     or failed. sigh.
  // Actually, we do. We check for a root <parsererror> node. This relies on the XMLContentSink
  // behaviour.
  nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));    
  xblService->FetchSyncXMLDocument(uri, &gBookmarksDocument);   // addref here
    
  // test for a parser error. The XML parser replaces the document with one
  // that has a <parsererror> node as the root.
  nsCOMPtr<nsIDOMDocument> bookmarksDOMDoc = do_QueryInterface(gBookmarksDocument);
  BOOL validBookmarksFile = CheckXMLDocumentParseSuccessful(bookmarksDOMDoc);
  
  if (!validBookmarksFile) {
    // uh oh, parser error. Throw some UI
    NSString *alert     = NSLocalizedString(@"CorruptedBookmarksAlert",@"");
    NSString *message   = NSLocalizedString(@"CorruptedBookmarksMsg",@"");
    NSString *okButton  = NSLocalizedString(@"OKButtonText",@"");
    NSRunAlertPanel(alert, message, okButton, nil, nil);

    // save the error to a file so the user can see what's wrong
    SaveBookmarksToFile(NS_LITERAL_STRING("bookmarks_parse_error.xml"));

    // maybe we should read the default bookmarks here?
    gBookmarksFileReadOK = PR_FALSE;
    return;
  }
  
  gBookmarksFileReadOK = PR_TRUE;
  
  nsCOMPtr<nsIContent> rootNode;
  GetRootContent(getter_AddRefs(rootNode));
  StripWhitespaceNodes(rootNode);
  
  EnsureToolbarRoot();
}

void
BookmarksService::FlushBookmarks()
{
  // XXX we need to insert a mechanism here to ensure that we don't write corrupt
  //     bookmarks files (e.g. full disk, program crash, whatever), because our
  //     error handling in the parse stage is NON-EXISTENT.
  //  This is now partially handled by looking for a <parsererror> node at read time.
  if (!gBookmarksFileReadOK)
    return;
  
  SaveBookmarksToFile(NS_LITERAL_STRING("bookmarks.xml"));
}

void
BookmarksService::SaveBookmarksToFile(const nsAString& inFileName)
{
  nsCOMPtr<nsIFile> bookmarksFile;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(bookmarksFile));
  bookmarksFile->Append(inFileName);
  
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(gBookmarksDocument));
  if (!domDoc) {
    NSLog(@"No bookmarks document to save!");
    return;
  }

  nsCOMPtr<nsIOutputStream> outputStream;
  NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), bookmarksFile);

  nsCOMPtr<nsIDOMSerializer> domSerializer(do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID));
  if (domSerializer)
    domSerializer->SerializeToStream(domDoc, outputStream, nsnull);
}

NSImage*
BookmarksService::CreateIconForBookmark(nsIDOMElement* aElement, PRBool useSiteIcon)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  if (content)
  {
    nsCOMPtr<nsIAtom> tagName;
    content->GetTag(*getter_AddRefs(tagName));
  
    nsAutoString group;
    content->GetAttr(kNameSpaceID_None, gGroupAtom, group);
    if (!group.IsEmpty())
      return [NSImage imageNamed:@"groupbookmark"];
  
    if (tagName == BookmarksService::gFolderAtom)
      return [NSImage imageNamed:@"folder"];
    
    // fire off a site icon load
    if (useSiteIcon && [[PreferenceManager sharedInstance] getBooleanPref:"browser.chrome.favicons" withSuccess:NULL])
    {
      nsAutoString href;
      content->GetAttr(kNameSpaceID_None, gHrefAtom, href);
      if (href.Length() > 0)
      {
        BookmarkItem* contentItem = BookmarksService::GetWrapperFor(content);
        if ([contentItem siteIcon])
          return [contentItem siteIcon];
        
        if (contentItem && ![contentItem siteIcon])
          [[BookmarksManager sharedBookmarksManager] loadProxyImageFor:contentItem withURI:[NSString stringWith_nsAString:href]];
      }
    }
  }
  return [NSImage imageNamed:@"smallbookmark"];
}

static bool AttributeStringContainsValue(const nsAString& inAttribute, const nsAString& inValue)
{
  nsAutoString attributeString(inAttribute);
  attributeString.StripWhitespace();

  PRInt32 begin = 0;
  PRInt32 len = attributeString.Length();
  while (begin < len)
  {
    PRInt32 end = attributeString.FindChar(PRUnichar(','), begin);
    if (end == kNotFound)
      end = len;
    if (Substring(attributeString, begin, end - begin).Equals(inValue))
      return true;

    begin = end + 1;
  }

  return false;
}

bool BookmarksService::FindFolderWithAttribute(const nsAString& inAttribute, const nsAString& inValue, nsIDOMElement** foundElt)
{
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(gBookmarksDocument));
  if (!domDoc) return false;
  
  nsCOMPtr<nsIDOMElement> rootElt;
  domDoc->GetDocumentElement(getter_AddRefs(rootElt));

  return SearchChildrenForMatchingFolder(rootElt, inAttribute, inValue, foundElt);
}

void BookmarksService::EnsureToolbarRoot()
{
  if (!gBookmarksDocument) return;
  
  if (!gToolbarRoot)
  {
    nsCOMPtr<nsIDOMElement> foundElt;
    FindFolderWithAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("toolbar"), getter_AddRefs(foundElt));
    if (foundElt)
      gToolbarRoot = foundElt;

    if (!gToolbarRoot)
    {
      NSLog(@"Repairing personal toolbar");
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(gBookmarksDocument);

      nsCOMPtr<nsIDOMElement> rootElt;
      domDoc->GetDocumentElement(getter_AddRefs(rootElt));

      nsCOMPtr<nsIDOMElement> elt;
      domDoc->CreateElementNS(NS_LITERAL_STRING("http://chimera.mozdev.org/bookmarks/"),
                              NS_LITERAL_STRING("folder"),
                              getter_AddRefs(elt));
  
      elt->SetAttribute(NS_LITERAL_STRING("name"), NS_LITERAL_STRING("Toolbar Bookmarks"));
      elt->SetAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("toolbar"));
  
      nsCOMPtr<nsIDOMNode> dummy;
      rootElt->AppendChild(elt, getter_AddRefs(dummy));
      gToolbarRoot = elt;
    }
  }
  
  if (!gDockMenuRoot)
  {
    nsCOMPtr<nsIDOMElement> foundElt;
    FindFolderWithAttribute(NS_LITERAL_STRING("dockmenu"), NS_LITERAL_STRING("true"), getter_AddRefs(foundElt));
    if (foundElt)
      gDockMenuRoot = foundElt;
    
    if (!gDockMenuRoot)
      gDockMenuRoot = gToolbarRoot;
  }
}


void BookmarksService::SetDockMenuRoot(nsIContent* inDockRootContent)
{
  if (!inDockRootContent)
  {
    // we allow it to be set to nil
      if (gDockMenuRoot)
        gDockMenuRoot->RemoveAttribute(NS_LITERAL_STRING("dockmenu"));
      gDockMenuRoot = nil;
      BookmarksService::SpecialBookmarkChanged(eBookmarksFolderDockMenu, nil);
  }
  else
  {
    // sanity check
    nsCOMPtr<nsIAtom> tagName;
    inDockRootContent->GetTag(*getter_AddRefs(tagName));
  
    nsAutoString group;
    inDockRootContent->GetAttr(kNameSpaceID_None, gGroupAtom, group);
  
    nsCOMPtr<nsIDOMElement> dockRootElement = do_QueryInterface(inDockRootContent);
    
    if (dockRootElement && group.IsEmpty() && tagName == gFolderAtom)
    {
      // remove the attribute from the old node
      if (gDockMenuRoot)
        gDockMenuRoot->RemoveAttribute(NS_LITERAL_STRING("dockmenu"));
      // set the attribute on the new node
      dockRootElement->SetAttribute(NS_LITERAL_STRING("dockmenu"), NS_LITERAL_STRING("true"));
      
      gDockMenuRoot = dockRootElement;
      BookmarksService::SpecialBookmarkChanged(eBookmarksFolderDockMenu, inDockRootContent);
    }
  }
}

void BookmarksService::SetToolbarRoot(nsIContent* inToolbarRootContent)
{
  if (!inToolbarRootContent) return;

  // sanity check
  nsCOMPtr<nsIAtom> tagName;
  inToolbarRootContent->GetTag(*getter_AddRefs(tagName));

  nsAutoString group;
  inToolbarRootContent->GetAttr(kNameSpaceID_None, gGroupAtom, group);

  nsCOMPtr<nsIDOMElement> toolbarRootElement = do_QueryInterface(inToolbarRootContent);
  
  if (toolbarRootElement && group.IsEmpty() && tagName == gFolderAtom)
  {
    // remove the attribute from the old node
    if (gToolbarRoot)
      gToolbarRoot->RemoveAttribute(NS_LITERAL_STRING("type"));
    // set the attribute on the new node
    toolbarRootElement->SetAttribute(NS_LITERAL_STRING("type"), NS_LITERAL_STRING("toolbar"));

    gToolbarRoot = toolbarRootElement;
    BookmarksService::SpecialBookmarkChanged(eBookmarksFolderToolbar, inToolbarRootContent);
  }
}

static void GetImportTitle(nsIDOMElement* aSrc, nsString& aTitle)
{
  aTitle.Truncate(0);
  nsCOMPtr<nsIDOMNode> curr;
  aSrc->GetFirstChild(getter_AddRefs(curr));
  while (curr) {
    nsCOMPtr<nsIDOMCharacterData> charData(do_QueryInterface(curr));
    if (charData) {
      nsAutoString data;
      charData->GetData(data);
      aTitle += data;
    }
    else {
      // Handle Omniweb's nesting of <a> inside <h3> for its folders.
      nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(curr));
      if (elt) {
        nsAutoString localName;
        elt->GetLocalName(localName);
        ToLowerCase(localName);
        if (localName.Equals(NS_LITERAL_STRING("a"))) {
          aTitle = NS_LITERAL_STRING("");
          return GetImportTitle(elt, aTitle);
        }
      }
    }
    
    nsCOMPtr<nsIDOMNode> temp = curr;
    temp->GetNextSibling(getter_AddRefs(curr));
  }
}

// sniff for control chars, which are defined to be in the range U+0000 to U+001F and U+007F to U+009F.
static PRBool ContainsControlChars(const nsString& inString)
{
  PRUint16 *c = (PRUint16*)inString.get();  // be sure to get unsigned
  PRUint16 *cEnd = c + inString.Length();   // use Length, rather than looking for null byte,
                                            // because there may be extra nulls at the end.
  
  while (c < cEnd)
  {
    if ((*c <= 0x001F) || (*c >= 0x007F && *c <= 0x009F))
      return PR_TRUE;
    c ++;
  }
  
  return PR_FALSE;
}

static void CleanControlChars(nsString& ioString)
{
  if (ContainsControlChars(ioString))
  {
    // strip control chars here. this is inefficient, but does it matter?
    NSString* cleanedTitle  = [[NSString stringWith_nsAString: ioString]
                stringByReplacingCharactersInSet:[NSCharacterSet controlCharacterSet] withString:@""];
    [cleanedTitle assignTo_nsAString:ioString];
    NSLog(@"Removed control characters from bookmark string '%@'", cleanedTitle);
  }
}

static void CreateBookmark(nsIDOMElement* aSrc, nsIDOMElement* aDst,
                           nsIDOMDocument* aDstDoc, PRBool aIsFolder,
                           nsIDOMElement** aResult)
{
  nsAutoString tagName(NS_LITERAL_STRING("bookmark"));
  if (aIsFolder)
    tagName = NS_LITERAL_STRING("folder");

  aDstDoc->CreateElementNS(NS_LITERAL_STRING("http://chimera.mozdev.org/bookmarks/"),
                           tagName,
                           aResult); // Addref happens here.

  nsAutoString title;
  GetImportTitle(aSrc, title);
  CleanControlChars(title);

  (*aResult)->SetAttribute(NS_LITERAL_STRING("name"), title);

  if (!aIsFolder) {
    nsAutoString href;
    aSrc->GetAttribute(NS_LITERAL_STRING("href"), href);
    CleanControlChars(href);
    (*aResult)->SetAttribute(NS_LITERAL_STRING("href"), href);
  }

  nsCOMPtr<nsIDOMNode> dummy;
  aDst->AppendChild(*aResult, getter_AddRefs(dummy));
}

static void AddImportedBookmarks(nsIDOMElement* aSrc, nsIDOMElement* aDst, nsIDOMDocument* aDstDoc,
                                 PRInt32& aBookmarksType)
{
  nsAutoString localName;
  aSrc->GetLocalName(localName);
  ToLowerCase(localName);
  nsCOMPtr<nsIDOMElement> newBookmark;
  if (localName.Equals(NS_LITERAL_STRING("bookmarkinfo")))
    aBookmarksType = 1; // Omniweb.
  else if (localName.Equals(NS_LITERAL_STRING("dt"))) {
    // We have found either a folder or a leaf.
    nsCOMPtr<nsIDOMNode> curr;
    aSrc->GetFirstChild(getter_AddRefs(curr));
    while (curr) {
      nsCOMPtr<nsIDOMElement> childElt(do_QueryInterface(curr));
      if (childElt) {
        childElt->GetLocalName(localName);
        ToLowerCase(localName);
        if (localName.Equals(NS_LITERAL_STRING("a"))) {
          // Guaranteed to be a bookmark in IE.  Could be either in Omniweb.
          nsCOMPtr<nsIDOMElement> dummy;
          CreateBookmark(childElt, aDst, aDstDoc, PR_FALSE, getter_AddRefs(dummy));
        }
        // Ignore the H3 we encounter.  This will be dealt with later.
      }
      nsCOMPtr<nsIDOMNode> temp = curr;
      temp->GetNextSibling(getter_AddRefs(curr));
    }
  }
  else if (localName.Equals(NS_LITERAL_STRING("dl"))) {
    // The children of a folder.  Recur inside.
    // Locate the parent to create the folder.
    nsCOMPtr<nsIDOMNode> node;
    aSrc->GetPreviousSibling(getter_AddRefs(node));
    nsCOMPtr<nsIDOMElement> folderElt(do_QueryInterface(node));
    if (folderElt) {
      // Make sure it's an H3 folder in Mozilla and IE.  In Mozilla it will probably have an ID.
      PRBool hasID;
      folderElt->HasAttribute(NS_LITERAL_STRING("ID"), &hasID);
      if (aBookmarksType != 1) {
        if (hasID)
          aBookmarksType = 2; // Mozilla
        else
          aBookmarksType = 0; // IE
      }
      nsAutoString localName;
      folderElt->GetLocalName(localName);
      ToLowerCase(localName);
      if (localName.Equals(NS_LITERAL_STRING("h3")))
        CreateBookmark(folderElt, aDst, aDstDoc, PR_TRUE, getter_AddRefs(newBookmark));
    }
    if (!newBookmark)
      newBookmark = aDst;
    // Recur over all our children.
    nsCOMPtr<nsIDOMNode> curr;
    aSrc->GetFirstChild(getter_AddRefs(curr));
    while (curr) {
      nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(curr));
      if (elt)
        AddImportedBookmarks(elt, newBookmark, aDstDoc, aBookmarksType);
      nsCOMPtr<nsIDOMNode> temp = curr;
      temp->GetNextSibling(getter_AddRefs(curr));
    }
  }
  else {
    // Recur over all our children.
    nsCOMPtr<nsIDOMNode> curr;
    aSrc->GetFirstChild(getter_AddRefs(curr));
    while (curr) {
      nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(curr));
      if (elt)
        AddImportedBookmarks(elt, aDst, aDstDoc, aBookmarksType);
      nsCOMPtr<nsIDOMNode> temp = curr;
      temp->GetNextSibling(getter_AddRefs(curr));
    }
  }
}


void
BookmarksService::ImportBookmarks(nsIDOMHTMLDocument* aHTMLDoc)
{
  nsCOMPtr<nsIDOMElement> htmlDocRoot;
  aHTMLDoc->GetDocumentElement(getter_AddRefs(htmlDocRoot));

  nsCOMPtr<nsIDOMElement>  bookmarksRoot;
  nsCOMPtr<nsIDOMDocument> bookmarksDOMDoc(do_QueryInterface(gBookmarksDocument));
  if (!bookmarksDOMDoc) return;
  
  bookmarksDOMDoc->GetDocumentElement(getter_AddRefs(bookmarksRoot));

  nsCOMPtr<nsIDOMNode> dummy;
  
  // Create the root of the new bookmarks by hand.
  nsCOMPtr<nsIDOMElement> importedRootElement;
  bookmarksDOMDoc->CreateElementNS(	NS_LITERAL_STRING("http://chimera.mozdev.org/bookmarks/"),
                                      NS_LITERAL_STRING("folder"),
                                      getter_AddRefs(importedRootElement));
  
  // Now crawl through the file and look for <DT> elements.  They signify folders
  // or leaves.
  PRInt32 bookmarksType = 0; // Assume IE.
  AddImportedBookmarks(htmlDocRoot, importedRootElement, bookmarksDOMDoc, bookmarksType);

  if (bookmarksType == 0)
    importedRootElement->SetAttribute(NS_LITERAL_STRING("name"), NS_LITERAL_STRING("Internet Explorer Favorites"));
  else if (bookmarksType == 1)
    importedRootElement->SetAttribute(NS_LITERAL_STRING("name"), NS_LITERAL_STRING("Omniweb Favorites"));
  else if (bookmarksType == 2)
    importedRootElement->SetAttribute(NS_LITERAL_STRING("name"), NS_LITERAL_STRING("Mozilla/Netscape Favorites"));

  // now put the new child into the doc, and validate it
  bookmarksRoot->AppendChild(importedRootElement, getter_AddRefs(dummy));

  PRBool bookmarksGood = ValidateXMLDocument(bookmarksDOMDoc);
  if (!bookmarksGood) {
    // uh oh, parser error. Remove the new node, and then throw some UI
    bookmarksRoot->RemoveChild(importedRootElement, getter_AddRefs(dummy));
        
    NSString *alert     = NSLocalizedString(@"ErrorImportingBookmarksAlert",@"");
    NSString *message   = NSLocalizedString(@"ErrorImportingBookmarksMsg",@"");
    NSString *okButton  = NSLocalizedString(@"OKButtonText",@"");
    NSRunAlertPanel(alert, message, okButton, nil, nil);
    return;
  }

  // Now do a notification that the root Favorites folder got added.  This
  // will update all our views.
  nsCOMPtr<nsIContent> parentContent(do_QueryInterface(bookmarksRoot));
  nsCOMPtr<nsIContent> childContent(do_QueryInterface(importedRootElement));
  
#if 0
  // XXX testing
  if (gBookmarkItemDictionary)
    [gBookmarkItemDictionary removeAllObjects];
#endif

  // this will save the file
  BookmarkAdded(parentContent, childContent, true /* flush */);
}

NSString* 
BookmarksService::ResolveKeyword(NSString* aKeyword)
{
  nsAutoString keyword;
  [aKeyword assignTo_nsAString:keyword];

  if (keyword.IsEmpty())
    return [NSString string];
  
#if DEBUG
  NSLog(@"str = %s", keyword.get());
#endif
  
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(gBookmarksDocument));
  if (!domDoc) return @"";
  
  nsCOMPtr<nsIDOMElement> elt;
  domDoc->GetElementById(keyword, getter_AddRefs(elt));

  nsCOMPtr<nsIContent> content(do_QueryInterface(elt));
  nsAutoString url;
  if (content) {
    content->GetAttr(kNameSpaceID_None, gHrefAtom, url);
    return [NSString stringWith_nsAString: url];
  }
  return [NSString string];
}

// Is searchItem equal to bookmark or bookmark's parent, grandparent, etc?
bool
BookmarksService::DoAncestorsIncludeNode(BookmarkItem* bookmark, BookmarkItem* searchItem)
{
  nsCOMPtr<nsIContent> search = [searchItem contentNode];
  nsCOMPtr<nsIContent> current = [bookmark contentNode];
  nsCOMPtr<nsIContent> root;
  GetRootContent(getter_AddRefs(root));
    
  // If the search item is the root node, return yes immediatly
  if (search == root)
    return true;

  //  for each ancestor
  while (current) {
    // If this is the root node we can't search farther, and there was no match
    if (current == root)
      return false;
    
    // If the two nodes match, then the search term is an ancestor of the given bookmark
    if (search == current)
      return true;

    // If a match wasn't found, set up the next node to compare
    nsCOMPtr<nsIContent> oldCurrent = current;
    oldCurrent->GetParent(*getter_AddRefs(current));
  }
  
  return false;
}


#ifdef FILTER_DESCENDANT_ON_DRAG
/*
this has been disabled because it is too slow, and can cause a large
delay when the user is dragging lots of items.  this needs to get
fixed someday.

It should filter out every node whose parent is also being dragged.
*/

NSArray*
BookmarksService::FilterOutDescendantsForDrag(NSArray* nodes)
{
  NSMutableArray *toDrag = [NSMutableArray arrayWithArray: nodes];
  unsigned int i = 0;

  while (i < [toDrag count]) {
    BookmarkItem* item = [toDrag objectAtIndex: i];
    bool matchFound = false;
    
    for (unsigned int j = 0; j < [toDrag count] && matchFound == NO; j++) {
      if (i != j) // Don't compare to self, will always match
        matchFound = BookmarksService::DoAncestorsIncludeNode(item, [toDrag objectAtIndex: j]);
    }
    
    //  if a match was found, remove the node from the array
    if (matchFound)
      [toDrag removeObjectAtIndex: i];
    else
      i++;
  }
  
  return toDrag;
}
#endif

bool
BookmarksService::IsBookmarkDropValid(BookmarkItem* proposedParent, int index, NSArray* draggedIDs)
{
  if ( !draggedIDs )
    return false;

  NSMutableArray *draggedItems = [NSMutableArray arrayWithCapacity: [draggedIDs count]];
  BOOL toolbarRootMoving = NO;
  
  BOOL          haveCommonParent = YES;
  BookmarkItem* commonDragParent = nil;
  
  for (unsigned int i = 0; i < [draggedIDs count]; i++)
  {
    NSNumber* contentID = [draggedIDs objectAtIndex: i];
    BookmarkItem* bookmarkItem = BookmarksService::GetWrapperFor([contentID unsignedIntValue]);
    nsCOMPtr<nsIContent> itemContent = [bookmarkItem contentNode];    
    nsCOMPtr<nsIDOMElement> itemElement(do_QueryInterface(itemContent));

    if (itemElement == BookmarksService::gToolbarRoot)
      toolbarRootMoving = YES;

    if (bookmarkItem)
      [draggedItems addObject: bookmarkItem];
    
    if (haveCommonParent)
    {
      if (!commonDragParent)
        commonDragParent = [bookmarkItem parentItem];
      else if ([bookmarkItem parentItem] != commonDragParent)
      {
        commonDragParent = nil;
        haveCommonParent = NO;
      }
    }
  }

  // if there is only one item being dragged (or, if items are contiguous),
  // prevent drops which would not affect the final order
  if (haveCommonParent && (commonDragParent == proposedParent))
  {
    // XXX bail for now if > 1 being dragged. For the correct feedback when > 1 item is
    // selected, we should reject drags when the source items are contiguous (under the
    // same parent), and the target is within their range.
    if ([draggedItems count] == 1)
    {
      BookmarkItem* draggedItem = [draggedItems objectAtIndex:0];
      nsCOMPtr<nsIContent> parentContent;
      [draggedItem contentNode]->GetParent(*getter_AddRefs(parentContent));
      PRInt32 childIndex;
      if (parentContent && NS_SUCCEEDED(parentContent->IndexOf([draggedItem contentNode], childIndex)))
      {
        if (childIndex == index || (childIndex + 1) == index)
          return false;
      }
    }
  }

  // If we are being dropped into the top level, allow it
  if ([proposedParent contentNode] == [BookmarksService::GetRootItem() contentNode])
    return true;
    
  // If we are not being dropped on the top level, and the toolbar root is being moved, disallow
  if (toolbarRootMoving)
    return false;

  // Make sure that we are not being dropped into one of our own children
  // If the proposed parent, or any of it's ancestors matches one of the nodes being dragged
  // then deny the drag.
  for (unsigned int i = 0; i < [draggedItems count]; i++)
  {
    if (BookmarksService::DoAncestorsIncludeNode(proposedParent, [draggedItems objectAtIndex: i]))
      return false;
  }
    
  return true;
}


bool
BookmarksService::PerformProxyDrop(BookmarkItem* parentItem, BookmarkItem* beforeItem, NSDictionary* data)
{
  if ( !data )
    return NO;

  nsCOMPtr<nsIDOMElement> parentElt;
  parentElt = do_QueryInterface([parentItem contentNode]);

  nsCOMPtr<nsIDOMElement> beforeElt;
  beforeElt = do_QueryInterface([beforeItem contentNode]);

  nsAutoString url; [[data objectForKey:@"url"] assignTo_nsAString:url];
  nsAutoString title; [[data objectForKey:@"title"] assignTo_nsAString:title];
  BookmarksService::AddBookmarkToFolder(url, title, parentElt, beforeElt);
  return YES;  
}


bool
BookmarksService::PerformBookmarkDrop(BookmarkItem* parent, BookmarkItem* beforeItem, int index, NSArray* draggedIDs, bool doCopy)
{
  NSEnumerator *enumerator = [draggedIDs reverseObjectEnumerator];
  NSNumber *contentID;
  
  //  for each item being dragged
  while ( (contentID = [enumerator nextObject]) )
  {  
    //  get dragged node
    nsCOMPtr<nsIContent> draggedNode = [GetWrapperFor([contentID unsignedIntValue]) contentNode];
    
    //  get the dragged nodes parent
    nsCOMPtr<nsIContent> draggedParent;
    if (draggedNode)
      draggedNode->GetParent(*getter_AddRefs(draggedParent));

    //  get the proposed parent
    nsCOMPtr<nsIContent> proposedParent = [parent contentNode];

    PRInt32 existingIndex = 0;
    if (draggedParent)
      draggedParent->IndexOf(draggedNode, existingIndex);
    
    //  if the deleted nodes parent and the proposed parents are equal
    //  and if the deleted point is earlier in the list than the inserted point
    if (!doCopy && proposedParent == draggedParent && existingIndex < index)
      index--;  //  if so, move the inserted point up one to compensate
    
    //  remove it from the tree
    if (draggedNode != proposedParent)		// paranoia. This should never happen
    {
      nsCOMPtr<nsIAtom> tagName;
      draggedNode->GetTag(*getter_AddRefs(tagName));
      bool srcIsFolder = (tagName == BookmarksService::gFolderAtom);

      if (doCopy && !srcIsFolder)
      {
        // need to clone
        nsCOMPtr<nsIDOMElement> srcElement = do_QueryInterface(draggedNode);
        nsCOMPtr<nsIDOMElement> destParent = do_QueryInterface(proposedParent);
        
        nsAutoString title;
        nsAutoString href;
        srcElement->GetAttribute(NS_LITERAL_STRING("href"), href);
        srcElement->GetAttribute(NS_LITERAL_STRING("name"), title);

        nsCOMPtr<nsIDOMElement> beforeElt = do_QueryInterface([beforeItem contentNode]);
        AddBookmarkToFolder(href, title, destParent, beforeElt);
      }
      else
      {
        if (draggedParent)
          draggedParent->RemoveChildAt(existingIndex, PR_TRUE);
        BookmarkRemoved(draggedParent, draggedNode, false);
        
        //  insert into new position
        if (proposedParent)
          proposedParent->InsertChildAt(draggedNode, index, PR_TRUE, PR_TRUE);
        BookmarkAdded(proposedParent, draggedNode, false);
      }
    }
  }

  FlushBookmarks();
  
  return true;
}

bool
BookmarksService::PerformURLDrop(BookmarkItem* parentItem, BookmarkItem* beforeItem, NSString* inTitle, NSString* inUrl) 
{
  if ( !inUrl || [inUrl length] == 0 )
    return NO;

  nsCOMPtr<nsIDOMElement> parentElt;
  parentElt = do_QueryInterface([parentItem contentNode]);

  nsCOMPtr<nsIDOMElement> beforeElt;
  beforeElt = do_QueryInterface([beforeItem contentNode]);

  nsAutoString url;   [inUrl assignTo_nsAString:url];
  nsAutoString title; [inTitle assignTo_nsAString:title];

  if (title.Length() == 0)
    [inUrl assignTo_nsAString:title];

  // we have to be paranoid about adding bad characters to the XML DOM here, because
  // that will cause our XML file to not validate when it's read in
  CleanControlChars(url);
  CleanControlChars(title);

  BookmarksService::AddBookmarkToFolder(url, title, parentElt, beforeElt);
  return YES;  
}


#pragma mark -

@implementation BookmarkItem

-(void)dealloc
{
  [mSiteIcon release];
  [super dealloc];
}

-(nsIContent*)contentNode
{
  return mContentNode;		// no addreffing
}

- (NSNumber*)contentID
{
  PRUint32 contentID = 0;
  mContentNode->GetContentID(&contentID);
  return [NSNumber numberWithInt: (int)contentID];
}

- (int)intContentID
{
  PRUint32 contentID = 0;
  mContentNode->GetContentID(&contentID);
  return (int)contentID;
}

- (NSString *)description
{
  nsCOMPtr<nsIContent> item = [self contentNode];
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(item));
  nsAutoString href;
  element->GetAttribute(NS_LITERAL_STRING("name"), href);
  NSString* info = [NSString stringWith_nsAString: href];
  return [NSString stringWithFormat:@"<BookmarkItem, name = \"%@\">", info];
}

- (NSString *)url
{
  nsCOMPtr<nsIContent> item = [self contentNode];
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(item));
  nsAutoString href;
  element->GetAttribute(NS_LITERAL_STRING("href"), href);
  return [NSString stringWith_nsAString: href];
}

- (void)setSiteIcon:(NSImage*)image
{
  [mSiteIcon autorelease];
  mSiteIcon = [image retain];
}

- (NSImage*)siteIcon
{
  return mSiteIcon;
}

-(void)setContentNode: (nsIContent*)aContentNode
{
  mContentNode = aContentNode;		// no addreffing
}

- (id)copyWithZone:(NSZone *)aZone
{
  BookmarkItem* copy = [[[self class] allocWithZone: aZone] init];
  [copy setContentNode: mContentNode];
  [copy setSiteIcon: mSiteIcon];
  return copy;
}

- (BOOL)isFolder
{
  nsCOMPtr<nsIAtom> tagName;
  mContentNode->GetTag(*getter_AddRefs(tagName));
  return (tagName == BookmarksService::gFolderAtom);
}

- (BOOL)isGroup
{
  nsCOMPtr<nsIAtom> tagName;
  mContentNode->GetTag(*getter_AddRefs(tagName));
  if (tagName != BookmarksService::gFolderAtom)
    return NO;

  nsAutoString group;
  mContentNode->GetAttr(kNameSpaceID_None, BookmarksService::gGroupAtom, group);
  return !group.IsEmpty();
}

- (BOOL)isToobarRoot
{
  nsCOMPtr<nsIContent> toolbarRoot = do_QueryInterface(BookmarksService::gToolbarRoot);
  return (mContentNode == toolbarRoot.get());
}

- (BOOL)isDockMenuRoot
{
  nsCOMPtr<nsIContent> dockRoot = do_QueryInterface(BookmarksService::gDockMenuRoot);
  return (mContentNode == dockRoot.get());
}

- (BookmarkItem*)parentItem
{
  nsCOMPtr<nsIContent> parentContent;
  mContentNode->GetParent(*getter_AddRefs(parentContent));

  nsCOMPtr<nsIContent> rootContent;
  BookmarksService::GetRootContent(getter_AddRefs(rootContent));

  // The root has no item
  if (parentContent != rootContent)
  {
    PRUint32 contentID;
    parentContent->GetContentID(&contentID);
    return BookmarksService::GetWrapperFor(contentID);
  }
  
  return nil;
}

@end


#pragma mark -

@interface BookmarksManager(Private)

- (void)registerNotificationListener;
- (void)imageLoadedNotification:(NSNotification*)notification;

- (void)buildFoldersListWalkChildren:(NSMenu*)menu curNode:(nsIContent*)curNode depth:(int)depth;
- (void)buildFoldersListProcessItem:(NSMenu*)menu curNode:(nsIContent*)curNode depth:(int)depth;

@end


@implementation BookmarksManager

enum
{
  eSiteLoadingStateNone    = 0,
  eSiteLoadingStateQueued  = 1,
  eSiteLoadingStatePending = 2,
  eSiteLoadingStateLoaded  = 3
};

static BookmarksManager* gBookmarksManager = nil;

+ (BookmarksManager*)sharedBookmarksManager;
{
  if (!gBookmarksManager)
    gBookmarksManager = [[BookmarksManager alloc] init];
  
  return gBookmarksManager;
}

+ (BookmarksManager*)sharedBookmarksManagerDontAlloc;
{
  return gBookmarksManager;
}

- (id)init
{
  if ((self = [super init]))
  {
    [self registerNotificationListener];
    BookmarksService::Init();
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  if (self == gBookmarksManager)
    gBookmarksManager = nil;
  [super dealloc];
}

- (void)loadProxyImageFor:(id)requestor withURI:(NSString*)inURIString
{	
  NSString* faviconURL = [SiteIconProvider faviconLocationStringFromURI:inURIString];
  
  // to avoid redundant loads, we keep a hash table which maps favicons urls
  // to NSDictionaries, which in turn contain a state flag, and an array of
  // UI items that use that url for their favicon.
  if (!gBookmarkFaviconURLDictionary)
    gBookmarkFaviconURLDictionary = [[NSMutableDictionary alloc] initWithCapacity:30];
  
  int loadingState = eSiteLoadingStateNone;

  // each item is an array of bookmark items
  NSMutableDictionary* siteDict = [gBookmarkFaviconURLDictionary objectForKey:faviconURL];
  if (siteDict)
  {
    NSMutableArray* siteBookmarks = [siteDict objectForKey:@"sites"];
    if (![siteBookmarks containsObject:requestor])
      [siteBookmarks addObject:requestor];
    
    loadingState = [[siteDict objectForKey:@"state"] intValue];
  }
  else
  {
    NSMutableArray* siteBookmarks = [[NSMutableArray alloc] initWithObjects:requestor, nil];

    siteDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                                   siteBookmarks, @"sites",
                  [NSNumber numberWithInt:eSiteLoadingStateNone], @"state",
                                                                  nil];

    [gBookmarkFaviconURLDictionary setObject:siteDict forKey:faviconURL];
  }
  
  if (loadingState == eSiteLoadingStateNone)
  {
    BOOL startedLoad = [[SiteIconProvider sharedFavoriteIconProvider] loadFavoriteIcon:self
                              forURI:inURIString
                              withUserData:faviconURL
                              allowNetwork:NO];

    if (startedLoad)
    {
      loadingState = eSiteLoadingStatePending;
      [faviconURL retain]; 		// keep alive while the load is active
    }
    else
      loadingState = eSiteLoadingStateLoaded;
      
    [siteDict setObject:[NSNumber numberWithInt:loadingState] forKey:@"state"];
  }

}

- (void)updateProxyImage:(NSImage*)image forSiteIcon:(NSString*)inSiteIconURI
{
  if (gBookmarkFaviconURLDictionary)
  {
    NSMutableDictionary* siteDict      = [gBookmarkFaviconURLDictionary objectForKey:inSiteIconURI];
    NSArray*             siteBookmarks = [siteDict objectForKey:@"sites"];
    if (siteBookmarks)
    {
      for (unsigned int i = 0; i < [siteBookmarks count]; i ++)
      {
        BookmarkItem* foundItem = [siteBookmarks objectAtIndex:i];
        if (foundItem)
        {
          [foundItem setSiteIcon:image];
          BookmarksService::BookmarkChanged([foundItem contentNode], FALSE);
        }
      }
    }

    // set the state
    [siteDict setObject:[NSNumber numberWithInt: eSiteLoadingStateLoaded] forKey:@"state"];
  }
}

- (void)registerNotificationListener
{
  [[NSNotificationCenter defaultCenter] addObserver:  self
                                        selector:     @selector(imageLoadedNotification:)
                                        name:         SiteIconLoadNotificationName
                                        object:       self];

  [[NSNotificationCenter defaultCenter] addObserver:  self
                                        selector:     @selector(shutdown:)
                                        name:         TermEmbeddingNotificationName
                                        object:       nil];
}

- (void)shutdown: (NSNotification*)aNotification
{
  BookmarksService::FlushBookmarks();
  BookmarksService::Shutdown();		// need to do this before XPCOM shuts down
  [gBookmarksManager release];
}

// callback for [[SiteIconProvider sharedFavoriteIconProvider] loadFavoriteIcon]
- (void)imageLoadedNotification:(NSNotification*)notification
{
  NSDictionary* userInfo = [notification userInfo];
  if (userInfo)
  {
    NSString* faviconURL = [userInfo objectForKey:SiteIconLoadUserDataKey];
  	NSImage*  iconImage  = [userInfo objectForKey:SiteIconLoadImageKey];
      
    if (iconImage)
      [self updateProxyImage:iconImage forSiteIcon:faviconURL];
    
    [faviconURL release]; 		// match retain in loadProxyImageFor
  }
}

- (void)addBookmarksClient:(id<BookmarksClient>)client
{
  if (!gBookmarkClientsArray)
    gBookmarkClientsArray = [[NSMutableArray alloc] initWithCapacity:4];

  if (![gBookmarkClientsArray containsObject:client])
    [gBookmarkClientsArray addObject:client];
}

- (void)removeBookmarksClient:(id<BookmarksClient>)client
{
  [gBookmarkClientsArray removeObject:client];
}

- (BookmarkItem*)getWrapperForContent:(nsIContent*)item
{
  return BookmarksService::GetWrapperFor(item);
}

- (BookmarkItem*)getWrapperForID:(int)contentID
{
  return BookmarksService::GetWrapperFor((PRUint32)contentID);
}

- (BookmarkItem*)getWrapperForNumber:(NSNumber*)contentIDNum
{
  return [gBookmarkItemDictionary objectForKey: contentIDNum];
}

- (nsIContent*)getRootContent
{
  nsIContent* rootContent = NULL;
  BookmarksService::GetRootContent(&rootContent);	// addrefs
  return rootContent;
}

- (nsIContent*)getToolbarRoot
{
  nsIContent* rootAsContent = NULL;
  if (BookmarksService::gToolbarRoot)
    BookmarksService::gToolbarRoot->QueryInterface(nsIContent::GetIID(), (void**)&rootAsContent);
    
  return rootAsContent;
}

- (nsIContent*)getDockMenuRoot
{
  nsIContent* rootAsContent = NULL;
  if (BookmarksService::gDockMenuRoot)
    BookmarksService::gDockMenuRoot->QueryInterface(nsIContent::GetIID(), (void**)&rootAsContent);
    
  return rootAsContent;
}

- (nsIDOMDocument*)getBookmarksDocument
{
  nsIDOMDocument* domDoc = NULL;

  if (gBookmarksDocument)
    gBookmarksDocument->QueryInterface(nsIDOMDocument::GetIID(), (void**)&domDoc);
    
  return domDoc;
}

- (NSArray*)getBookmarkGroupURIs:(BookmarkItem*)item
{
  nsIContent* content = [item contentNode];
  nsAutoString group;
  content->GetAttr(kNameSpaceID_None, BookmarksService::gGroupAtom, group);
  if (group.IsEmpty())
    return nil;

  nsCOMPtr<nsIDOMNode> folderNode = do_QueryInterface(content);
  if (!folderNode) return nil;

  // use nsIContent::ChildAt
  nsCOMPtr<nsIDOMNodeList> childNodes;
  folderNode->GetChildNodes(getter_AddRefs(childNodes));
  if (!childNodes) return nil;
  
  PRUint32 numChildren;
  childNodes->GetLength(&numChildren);
  
  NSMutableArray* uriArray = [[[NSMutableArray alloc] initWithCapacity:(int)numChildren] autorelease];

  for (PRUint32 i = 0; i < numChildren; i ++)
  {
    nsCOMPtr<nsIDOMNode> child;
    childNodes->Item(i, getter_AddRefs(child));
    nsCOMPtr<nsIDOMElement> elt = do_QueryInterface(child);
    if (elt)
    {
      nsAutoString href;
      elt->GetAttribute(NS_LITERAL_STRING("href"), href);
      if (!href.IsEmpty())
      {
        NSString* url = [NSString stringWith_nsAString: href];
        [uriArray addObject:url];
      }
    }
  }

  return uriArray;
}

- (void)buildFoldersListWalkChildren:(NSMenu*)menu curNode:(nsIContent*)content depth:(int)depth
{
  if (!content) return;
  
  // Now walk our children, and for folders also recur into them.
  PRInt32 childCount;
  content->ChildCount(childCount);
  
  for (PRInt32 i = 0; i < childCount; i++)
  {
    nsCOMPtr<nsIContent> child;
    content->ChildAt(i, *getter_AddRefs(child));
    [self buildFoldersListProcessItem:menu curNode:child depth:depth];
  }
}

- (void)buildFoldersListProcessItem:(NSMenu*)menu curNode:(nsIContent*)content depth:(int)depth
{
  if (!content) return;
  
  nsCOMPtr<nsIAtom> tagName;
  content->GetTag(*getter_AddRefs(tagName));

  nsAutoString group;
  if (tagName == BookmarksService::gFolderAtom)
    content->GetAttr(kNameSpaceID_None, BookmarksService::gGroupAtom, group);

  BOOL isFolder = (tagName == BookmarksService::gFolderAtom);
  BOOL isGroup  = isFolder && !group.IsEmpty();

  if (isFolder && !isGroup) // folder
  {
    // add it
    nsAutoString name;
    content->GetAttr(kNameSpaceID_None, BookmarksService::gNameAtom, name);

    NSString* 				itemName 	= [[NSString stringWith_nsAString: name] stringByTruncatingTo:80 at:kTruncateAtMiddle];
    NSMutableString*	title 		= [NSMutableString stringWithString:itemName];

    for (int j = 0; j <= depth; ++j) 
      [title insertString:@"    " atIndex: 0];

    NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle: title action: NULL keyEquivalent: @""] autorelease];
    
    PRUint32 contentID;
    content->GetContentID(&contentID);
    [menuItem setTag: contentID];
    
    [menu addItem: menuItem];

    [self buildFoldersListWalkChildren:menu curNode:content depth:depth+1];
  }
}


- (void)buildFlatFolderList:(NSMenu*)menu fromRoot:(nsIContent*)rootContent
{
  nsCOMPtr<nsIContent> startContent = rootContent;
  if (!startContent)
    BookmarksService::GetRootContent(getter_AddRefs(startContent));
  
  [self buildFoldersListWalkChildren:menu curNode:startContent depth:0];
}

@end

