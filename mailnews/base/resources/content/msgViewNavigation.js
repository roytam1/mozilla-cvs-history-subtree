/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

/*  This file contains the js functions necessary to implement view navigation within the 3 pane. */

var Bundle = srGetStrBundle("chrome://messenger/locale/messenger.properties");
var commonDialogs = Components.classes["@mozilla.org/appshell/commonDialogs;1"].getService();
commonDialogs = commonDialogs.QueryInterface(Components.interfaces.nsICommonDialogs);
var accountManager = Components.classes["@mozilla.org/messenger/account-manager;1"].getService(Components.interfaces.nsIMsgAccountManager);

function FindNextFolder(originalFolderURI)
{
    if (!originalFolderURI) return null;

    var originalFolderResource = RDF.GetResource(originalFolderURI);
	var folder = originalFolderResource.QueryInterface(Components.interfaces.nsIFolder);
    if (!folder) return null;
    dump("folder = " + folder.URI + "\n");

    try {
       var subFolderEnumerator = folder.GetSubFolders();
       var done = false;
       while (!done) {
         var element = subFolderEnumerator.currentItem();
         var currentSubFolder = element.QueryInterface(Components.interfaces.nsIMsgFolder);
         dump("current folder = " + currentSubFolder.URI + "\n");
         if (currentSubFolder.getNumUnread(false /* don't descend */) > 0) {
           dump("if the child has unread, use it.\n");
           return currentSubFolder.URI;
         }
         else if (currentSubFolder.getNumUnread(true /* descend */) > 0) {
           dump("if the child doesn't have any unread, but it's children do, recurse\n");
           return FindNextFolder(currentSubFolder.URI);
         }

         try {
           subFolderEnumerator.next();
         } 
         catch (ex) {
           done=true;
         }
      } // while
    }
    catch (ex) {
        // one way to get here is if the folder has no sub folders
    }
 
    if (folder.parent && folder.parent.URI) {
      dump("parent = " + folder.parent.URI + "\n");
      return FindNextFolder(folder.parent.URI);
    }
    else {
      dump("no parent\n");
    }
    return null;
}

function ScrollToFirstNewMessage()
{
  // this needs rewritten.
/*
	var tree = GetThreadTree();
	var treeFolder = GetThreadTreeFolder();

	var folderURI = treeFolder.getAttribute('ref');
	var folderResource = RDF.GetResource(folderURI);
	var folder = folderResource.QueryInterface(Components.interfaces.nsIMsgFolder);
	var hasNew = folder.hasNewMessages;
	if(hasNew)
	{
		var newMessage = folder.firstNewMessage;

		if(messageView.showThreads)
		{
			//if we're in thread mode, then we need to actually make sure the message is showing.
			var topLevelMessage = GetTopLevelMessageForMessage(newMessage, folder);
			var topLevelResource = topLevelMessage.QueryInterface(Components.interfaces.nsIRDFResource);
			var topLevelURI = topLevelResource.Value;
			var topElement = document.getElementById(topLevelURI);
			if(topElement)
			{
//				msgNavigationService.OpenTreeitemAndDescendants(topElement);
			}

		}
		
		var messageResource = newMessage.QueryInterface(Components.interfaces.nsIRDFResource);
		var messageURI = messageResource.Value;
		var messageElement = document.getElementById(messageURI);

		if(messageElement)
		{
			tree.ensureElementIsVisible(messageElement); 
		}
	}
  */
}

function GetTopLevelMessageForMessage(message, folder)
{
	if(!folder)
		folder = message.msgFolder;

	var thread = folder.getThreadForMessage(message);
    var outIndex = new Object();
	var rootHdr = thread.GetRootHdr(outIndex);

	var topMessage = folder.createMessageFromMsgDBHdr(rootHdr);

	return topMessage;

}

function GoNextMessage(type, startFromBeginning)
{
  try {
    dump("XXX GoNextMessage(" + type + "," + startFromBeginning + ")\n");

    var selection = new Object;

    var outlinerView = gDBView.QueryInterface(Components.interfaces.nsIOutlinerView);
    var outlinerSelection = outlinerView.selection;
    var currentIndex = outlinerSelection.currentIndex;

    dump("XXX outliner selection = " + outlinerSelection + "\n");
    dump("XXX current Index = " + currentIndex + "\n");

    var status = gDBView.navigateStatus(type);

    dump("XXX status = " + status + "\n");
    dump("XXX selection = " + selection.value + "\n");

    var resultId = new Object;
    var resultIndex = new Object;
    var threadIndex = new Object;
    var resultFolder = new Object;

    gDBView.viewNavigate(type, resultId, resultIndex, threadIndex, true /* wrap */, resultFolder);

    dump("XXX selection = " + selection.value + "\n");
    dump("XXX resultID = " + resultId.value + "\n");
    dump("XXX resultIndex = " + resultIndex.value + "\n");
    dump("XXX threadIndex = " + threadIndex.value + "\n");
    dump("XXX resultFolder = " + resultFolder.value + "\n");

    outlinerSelection.select(resultIndex.value);

    EnsureRowInThreadOutlinerIsVisible(resultIndex.value); 
  }
  catch (ex) {
    dump("XXX ex = " + ex + "\n");
  }
}
