/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

var gLastMessageUriToLoad = null;

function ThreadPaneOnClick(event)
{
    // we are already handling marking as read and flagging
    // in nsMsgDBView.cpp
    // so all we need to worry about here is double clicks
    // and column header.
    //
    // we get in here for clicks on the "outlinercol" (headers)
    // and the "scrollbarbutton" (scrollbar buttons)
    // we don't want those events to cause a "double click"
    var t = event.originalTarget;

    if (t.localName == "outlinercol") {
       HandleColumnClick(t.id);
    }
    else if (event.detail == 2 && t.localName == "outlinerbody") {
       ThreadPaneDoubleClick();
    }
}

function SetHiddenAttributeOnThreadOnlyColumns(value)
{
  // todo, cache these?

  var totalCol = document.getElementById("totalCol");
  var unreadCol = document.getElementById("unreadCol");

  totalCol.setAttribute("hidden",value);
  unreadCol.setAttribute("hidden",value);
}

function HandleColumnClick(columnID)
{
  // if they click on the "threadCol", we need to show the threaded-only columns
  if ((columnID[0] == 't') && (columnID[1] == 'h')) {  
    SetHiddenAttributeOnThreadOnlyColumns(""); // this will show them
  }
  else {
    SetHiddenAttributeOnThreadOnlyColumns("true");  // this will hide them
  }
  
  dump("fix this, is this the right place for persisting?\n");
  dump("fix UpdateSortMenu()\n");
  //UpdateSortMenu(columnID);
  var folder = GetSelectedFolder();

  if (folder) {
    var sortType = ConvertColumnIDToSortType(columnID);
    dump("XXX sortType to save = " + sortType + "\n");
    folder.setAttribute("sortType", sortType);
    dump("XXX fix me to save sortOrder\n");
    //folder.setAttribute("sortOrder", XXX);
  }
}

function MsgComposeDraftMessage()
{
    var loadedFolder = GetLoadedMsgFolder();
    var messageArray = GetSelectedMessages();

    ComposeMessage(msgComposeType.Draft, msgComposeFormat.Default, loadedFolder, messageArray);
}

function ThreadPaneDoubleClick()
{
	var loadedFolder;
	var messageArray;
	var messageUri;

	if (IsSpecialFolderSelected("Drafts")) {
		MsgComposeDraftMessage();
	}
	else if(IsSpecialFolderSelected("Templates")) {
		loadedFolder = GetLoadedMsgFolder();
		messageArray = GetSelectedMessages();
		ComposeMessage(msgComposeType.Template, msgComposeFormat.Default, loadedFolder, messageArray);
	}
	else {
        MsgOpenSelectedMessages();
	}
}

function ThreadPaneKeyPress(event)
{
  return;
}

function MsgSortByDate()
{
    gDBView.sort(nsMsgViewSortType.byDate,nsMsgViewSortOrder.ascending);
}

function MsgSortBySender()
{
    gDBView.sort(nsMsgViewSortType.byAuthor,nsMsgViewSortOrder.ascending);
}

function MsgSortByRecipient()
{
    gDBView.sort(nsMsgViewSortType.byRecipient,nsMsgViewSortOrder.ascending);
}

function MsgSortByStatus()
{
    gDBView.sort(nsMsgViewSortType.byStatus,nsMsgViewSortOrder.ascending);
}

function MsgSortBySubject()
{
    gDBView.sort(nsMsgViewSortType.bySubject,nsMsgViewSortOrder.ascending);
}

function MsgSortByFlagged() 
{
    gDBView.sort(nsMsgViewSortType.byFlagged,nsMsgViewSortOrder.ascending);
}

function MsgSortByPriority()
{
    gDBView.sort(nsMsgViewSortType.byPriority,nsMsgViewSortOrder.ascending);
}

function MsgSortBySize() 
{
    gDBView.sort(nsMsgViewSortType.bySize,nsMsgViewSortOrder.ascending);
}

function MsgSortByLines() 
{
    dump("XXX fix this\n");
    //gDBView.sort(nsMsgViewSortType.byLines,nsMsgViewSortOrder.ascending);
}

function MsgSortByUnread()
{
    gDBView.sort(nsMsgViewSortType.byUnread,nsMsgViewSortOrder.ascending);
}

function MsgSortByOrderReceived()
{
    gDBView.sort(nsMsgViewSortType.byId,nsMsgViewSortOrder.ascending);
}

function MsgSortByTotal()
{
    dump("XXX fix this\n");
    //gDBView.sort(nsMsgViewSortType.byTotal,nsMsgViewSortOrder.ascending);
}

function MsgSortByThread()
{
    gDBView.sort(nsMsgViewSortType.byThread,nsMsgViewSortOrder.ascending);
}

function IsSpecialFolderSelected(folderName)
{
	var selectedFolder = GetThreadPaneFolder();
    if (!selectedFolder) return false;

    dump("fix this, we don't need to use RDF for this.\n");

	var id = selectedFolder.URI;
	var folderResource = RDF.GetResource(id);
	if(!folderResource) return false;

    var db = GetFolderDatasource();

	var property =
        RDF.GetResource('http://home.netscape.com/NC-rdf#SpecialFolder');
    if (!property) return false;
	var result = db.GetTarget(folderResource, property , true);
    if (!result) return false;
	result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
    if (!result) return false;
	if(result.Value == folderName)
		return true;

	return false;
}

//Called when selection changes in the thread pane.
function ThreadPaneSelectionChange(fromDeleteOrMoveHandler)
{
    // we are batching.  bail out, we'll be back when the batch is over
    if (gBatching) return;

	var collapsed = IsThreadAndMessagePaneSplitterCollapsed();

	var numSelected = GetNumSelectedMessages();
    var messageUriToLoad = null;

    if (!gNextMessageAfterDelete && (numSelected == 1) ) {
        messageUriToLoad =  GetFirstSelectedMessage();
    }

    // if the message pane isn't collapsed, and we have a message to load
    // go ahead and load the message
	if (!collapsed && messageUriToLoad) {
        LoadMessageByUri(messageUriToLoad);
	}

    // if gNextMessageAfterDelete is true, we can skip updating the commands because
    // we'll be coming back to load that message, and we'll update the commands then
    //
    // if fromDeleteOrMoveHandler is true, we are calling ThreadPaneSelectionChange after handling
    // a message delete or message move, so we might need to update the commands.  (see below)
    //
    // if gCurrentLoadingFolderURI is non null, we are loading a folder, so we need to update the commands
    //
    // if messageUriToLoad is non null, we are loading a message, so we might need to update commmands.  (see below)
	if (!gNextMessageAfterDelete && (gCurrentLoadingFolderURI || fromDeleteOrMoveHandler || messageUriToLoad)) {
        // if we are moving or deleting, we'll come in here twice.  once to load the message and once when
        // we are done moving or deleting.  when we loaded the message the first time, we called updateCommands().
        // there is no need to do it again.
        if (fromDeleteOrMoveHandler && messageUriToLoad && (messageUriToLoad == gLastMessageUriToLoad)) {
            // skip the call to updateCommands()
        }
        else {
		    document.commandDispatcher.updateCommands('threadTree-select');
        }
	}

	//remember the last message we loaded
    gLastMessageUriToLoad = messageUriToLoad;
}

function GetThreadOutliner()
{
  if (gThreadOutliner) return gThreadOutliner;
	gThreadOutliner = document.getElementById('threadOutliner');
	return gThreadOutliner;
}

function GetThreadPaneFolder()
{
  try {
    return gDBView.msgFolder;
  }
  catch (ex) {
    return null;
  }
}

function EnsureRowInThreadOutlinerIsVisible(index)
{
  var outliner = GetThreadOutliner();
  outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).ensureRowIsVisible(index); 
}

function GetThreadTree()
{
    dump("GetThreadTree, fix (or remove?) this\n");
}

function GetThreadTreeFolder() 
{
    dump("GetThreadTreeFolder, fix (or remove?) this\n");
}

