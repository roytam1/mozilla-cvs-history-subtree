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


/*
 * Command-specific code. This stuff should be called by the widgets
 */

//The eventual goal is for this file to go away and for the functions to either be brought into
//mailCommands.js or into 3pane specific code.

var gFolderJustSwitched = false;
var gBeforeFolderLoadTime;
var gRDFNamespace = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

function OpenURL(url)
{
  //dump("\n\nOpenURL from XUL\n\n\n");
  messenger.SetWindow(window, msgWindow);
  messenger.OpenURL(url);
}

function GetMsgFolderFromNode(folderNode)
{
	var folderURI = folderNode.getAttribute("id");
	return GetMsgFolderFromURI(folderURI);
}

function GetMsgFolderFromURI(folderURI)
{
	var folderResource = RDF.GetResource(folderURI);
	if(folderResource)
	{
		var msgFolder = folderResource.QueryInterface(Components.interfaces.nsIMsgFolder);
		return msgFolder;
	}

	return null;


}

function GetServer(uri)
{
    if (!uri) return null;
    try {
        var folder = GetMsgFolderFromUri(uri);
        return folder.server;
    }
    catch (ex) {
        dump("GetServer("+uri+") failed, ex="+ex+"\n");
    }
    return null;
}

function LoadMessage(messageNode)
{
	var uri = messageNode.getAttribute('id');
	LoadMessageByUri(uri);
}

function LoadMessageByUri(uri)
{  
	if(uri != gCurrentDisplayedMessage)
	{
		var resource = RDF.GetResource(uri);
		var message = resource.QueryInterface(Components.interfaces.nsIMessage); 
		if (message)
			setTitleFromFolder(message.msgFolder, message.mime2DecodedSubject);

		var nsIMsgFolder = Components.interfaces.nsIMsgFolder;
		if (message.msgFolder.server.downloadOnBiff)
			message.msgFolder.biffState = nsIMsgFolder.nsMsgBiffState_NoMail;

		gCurrentDisplayedMessage = uri;
		gHaveLoadedMessage = true;
		OpenURL(uri);
	}

}

function ChangeFolderByDOMNode(folderNode)
{
  var uri = folderNode.getAttribute('id');
  dump(uri + "\n");

  var isThreaded = folderNode.getAttribute('threaded');
  
  if ((isThreaded == "") && isNewsURI(uri)) {
    isThreaded = "true";
  }

  var sortResource = folderNode.getAttribute('sortResource');
  if(!sortResource)
	sortResource = "";

  var sortDirection = folderNode.getAttribute('sortDirection');

  var viewType = folderNode.getAttribute('viewType');

  if (uri)
	  ChangeFolderByURI(uri, isThreaded == "true", sortResource, sortDirection, viewType);
}

function setTitleFromFolder(msgfolder, subject)
{
    if (!msgfolder) return;

    var title;
    var server = msgfolder.server;

    if (null != subject)
      title = subject+" - ";
    else
      title = "";

    if (msgfolder.isServer) {
            // <hostname>
            title += server.hostName;
    }
    else {
        var middle;
        var end;
        if (server.type == "nntp") {
            // <folder> on <hostname>
            middle = Bundle.GetStringFromName("titleNewsPreHost");
            end = server.hostName;
        } else {
            var identity;
            try {
                var identities = accountManager.GetIdentitiesForServer(server);

                identity = identities.QueryElementAt(0, Components.interfaces.nsIMsgIdentity);
                // <folder> for <email>
                middle = Bundle.GetStringFromName("titleMailPreHost");
                end = identity.email;
            } catch (ex) {
            }
            
        }

        title += msgfolder.prettyName;
        if (middle) title += " " + middle;
        if (end) title += " " + end;
    }

    title += " - " + BrandBundle.GetStringFromName("brandShortName");
    window.title = title;
}

function ChangeFolderByURI(uri, isThreaded, sortID, sortDirection, viewType)
{
  dump('In ChangeFolderByURI uri = ' + uri + "\n");
  if (uri == gCurrentLoadingFolderURI)
    return;
  var resource = RDF.GetResource(uri);
  var msgfolder =
      resource.QueryInterface(Components.interfaces.nsIMsgFolder);

  try {
      setTitleFromFolder(msgfolder, null);
  } catch (ex) {
      dump("error setting title: " + ex + "\n");
  }

  
  //if it's a server, clear the threadpane and don't bother trying to load.
  if(msgfolder.isServer)
  {
        ClearThreadPane();

        // Load AccountCentral page here.
        ShowAccountCentral(); 
	return;
  }

  // If the user clicks on msgfolder, time to display thread pane and message pane.
  // Hide AccountCentral page
  if (gAccountCentralLoaded)
  {
      HideAccountCentral();
  }
   
  if (showPerformance) { 
    gBeforeFolderLoadTime = new Date();
  }

  gCurrentLoadingFolderURI = uri;
  gNextMessageAfterDelete = null; // forget what message to select, if any

  if(msgfolder.manyHeadersToDownload())
  {
	try
	{
		SetBusyCursor(window, true);
		gCurrentFolderToReroot = uri;
		gCurrentLoadingFolderIsThreaded = isThreaded;
		gCurrentLoadingFolderSortID = sortID;
		gCurrentLoadingFolderSortDirection = sortDirection;
        gCurrentLoadingFolderViewType = viewType;
		msgfolder.startFolderLoading();
		msgfolder.updateFolder(msgWindow);
	}
	catch(ex)
	{
        dump("Error loading with many headers to download: " + ex + "\n");
	}
  }
  else
  {
    SetBusyCursor(window, true);
	gCurrentFolderToReroot = "";
	gCurrentLoadingFolderIsThreaded = false;
	gCurrentLoadingFolderSortID = "";
    gCurrentLoadingFolderViewType = "";
	RerootFolder(uri, msgfolder, isThreaded, sortID, sortDirection, viewType);

	//Need to do this after rerooting folder.  Otherwise possibility of receiving folder loaded
	//notification before folder has actually changed.
	msgfolder.updateFolder(msgWindow);
  }
}

function isNewsURI(uri)
{
    if (!uri || uri[0] != 'n') {
        return false;
    }
    else {
        return ((uri.substring(0,6) == "news:/") || (uri.substring(0,14) == "news_message:/"));
    }
}

function RerootFolder(uri, newFolder, isThreaded, sortID, sortDirection, viewType)
{
  dump('In reroot folder\n');
	
  // workaround for #39655
  gFolderJustSwitched = true;

  ClearThreadTreeSelection();

  //Set the window's new open folder.
  msgWindow.openFolder = newFolder;

  //Set threaded state
  ShowThreads(isThreaded);

  //Set the view type
  SetViewType(viewType);

  //Clear the new messages of the old folder
/*
  var oldFolderURI = folder.getAttribute("ref");
  if(oldFolderURI && (oldFolderURI != "null") && (oldFolderURI !=""))
  {
   var oldFolder = GetMsgFolderFromURI(oldFolderURI);
   if(oldFolder)
   {
       if (oldFolder.hasNewMessages)
           oldFolder.clearNewMessages();
   }
  }
*/
  //the new folder being selected should have its biff state get cleared.   
  if(newFolder)
  {
    newFolder.biffState = 
          Components.interfaces.nsIMsgFolder.nsMsgBiffState_NoMail;
  }

  //Clear out the thread pane so that we can sort it with the new sort id without taking any time.
  // folder.setAttribute('ref', "");
   if (isNewsURI(uri))
       SetNewsFolderColumns(true);
   else
       SetNewsFolderColumns(false);



  // null this out, so we don't try sort.
  gDBView = null;
  // fix me
  // SetSentFolderColumns(IsSpecialFolder(newFolder, [ "Sent", "Drafts", "Unsent Messages" ]));

  // now create the db view, which will sort it.

  CreateDBView(newFolder, isThreaded, viewType, sortID, sortDirection);
  // that should have initialized gDBView, now re-root the thread pane
  var outlinerView = gDBView.QueryInterface(Components.interfaces.nsIOutlinerView);
  if (outlinerView)
  {     
    var outliner = GetThreadOutliner();
    outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).view = outlinerView; 
    dump('set outliner view\n');
  }

  // Since SetSentFolderColumns() may alter the template's structure,
  // we need to explicitly force the builder to recompile its rules.
  //when switching folders, switch back to closing threads
  SetTemplateTreeItemOpen(false);

  SetUpToolbarButtons(uri);

  msgNavigationService.EnsureDocumentIsLoaded(document);

  UpdateStatusMessageCounts(newFolder);
}

function SetSentFolderColumns(isSentFolder)
{
	var senderColumn = document.getElementById("SenderColumnHeader");
	var senderColumnTemplate = document.getElementById("SenderColumnTemplate");
	var authorColumnHeader = document.getElementById("AuthorColumn");

	if(isSentFolder)
	{
		senderColumn.setAttribute("value", Bundle.GetStringFromName("recipientColumnHeader"));
		senderColumn.setAttribute("onclick", "return top.MsgSortByRecipient();");
		senderColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Recipient");
		authorColumnHeader.setAttribute("resource", "http://home.netscape.com/NC-rdf#Recipient");
	}
	else
	{
		senderColumn.setAttribute("value", Bundle.GetStringFromName("senderColumnHeader"));
		senderColumn.setAttribute("onclick", "return top.MsgSortBySender();");
		senderColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Sender");
		authorColumnHeader.setAttribute("resource", "http://home.netscape.com/NC-rdf#Sender");
	}


}

function SetNewsFolderColumns(isNewsFolder)
{
  // mscott - i don't know what this does but it's going to need to be re-written...
/* 
	var sizeColumn = document.getElementById("SizeColumnHeader");
  var sizeColumnTemplate = document.getElementById("SizeColumnTemplate");

  var memoryColumnHeader = document.getElementById("MemoryColumn");

  if (isNewsFolder)
  { 
     sizeColumn.setAttribute("value",Bundle.GetStringFromName("linesColumnHeader"));
     sizeColumn.setAttribute("onclick", "return top.MsgSortByLines();");
     sizeColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Lines");
     memoryColumnHeader.setAttribute("resource","http://home.netscape.com/NC-rdf#Lines");
  }
  else
  {
     sizeColumn.setAttribute("value", Bundle.GetStringFromName("sizeColumnHeader"));
     sizeColumn.setAttribute("onclick", "return top.MsgSortBySize();");
     sizeColumnTemplate.setAttribute("value", "rdf:http://home.netscape.com/NC-rdf#Size");
     memoryColumnHeader.setAttribute("resource","http://home.netscape.com/NC-rdf#Size");
  }
*/
} 
        
        

function UpdateStatusMessageCounts(folder)
{
	var unreadElement = GetUnreadCountElement();
	var totalElement = GetTotalCountElement();
	if(folder && unreadElement && totalElement)
	{
		var numUnread =
            Bundle.formatStringFromName("unreadMsgStatus",
                                        [ folder.getNumUnread(false)], 1);
		var numTotal =
            Bundle.formatStringFromName("totalMsgStatus",
                                        [folder.getTotalMessages(false)], 1);

		unreadElement.setAttribute("value", numUnread);
		totalElement.setAttribute("value", numTotal);

	}

}


function FindOutlinerColumnBySortType(sortKey)
{
	if(sortKey == nsMsgViewSortType.byDate)
		return "dateCol";
	else if(sortKey == nsMsgViewSortType.byAuthor)
		return "senderCol";
	else if(sortKey == nsMsgViewSortType.bySubject)
		return "subjectCol";
	else if(sortKey == nsMsgViewSortType.byUnread)
		return "unreadButtonColHeader";
 	else if(sortKey == nsMsgViewSortType.byStatus)
		return "statusCol";
  else if (sortKey == nsMsgViewSortType.bySize)
    return "sizeCol";
  else if (sortKey == nsMsgViewSortType.byPriority)
    return "priorityCol";

  // oops we haven't added the column for the sort key they are using yet...return null.
	return null;
}

function FindThreadPaneColumnBySortResource(sortID)
{

	if(sortID == "http://home.netscape.com/NC-rdf#Date")
		return "DateColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Sender")
		return "AuthorColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Recipient")
		return "AuthorColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Status")
		return "StatusColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Subject")
		return "SubjectColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Flagged")
		return "FlaggedButtonColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Priority")
		return "PriorityColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Size")
		return "MemoryColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#Lines")
		return "MemoryColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#IsUnread")
		return "UnreadButtonColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#TotalUnreadMessages")
		return "UnreadColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#TotalMessages")
		return "TotalColumn";
	else if(sortID == "http://home.netscape.com/NC-rdf#OrderReceived")
		return "OrderReceivedColumn";

	return null;
}

//If toggleCurrentDirection is true, then get current direction and flip to opposite.
//If it's not true then use the direction passed in.
function SortThreadPane(column, sortKey, secondarySortKey, toggleCurrentDirection, direction, changeCursor)
{
	//dump("In SortThreadPane, toggleCurrentDirection = " + toggleCurrentDirection + "\n");
	var node = document.getElementById(column);
	if(!node)
		return false;

	if(!direction)
	{
		direction = "ascending";
		//If we just clicked on the same column, then change the direction
		if(toggleCurrentDirection)
		{
			var currentDirection = node.getAttribute('sortDirection');
			if (currentDirection == "ascending")
					direction = "descending";
			else if (currentDirection == "descending")
					direction = "ascending";
		}
	}

	UpdateSortIndicator(column, direction);
	UpdateSortMenu(column);

   var folder = GetSelectedFolder();
	if(folder)
	{
		folder.setAttribute("sortResource", sortKey);
		folder.setAttribute("sortDirection", direction);
	}

	SetActiveThreadPaneSortColumn(column);

	var beforeSortTime;
	if(showPerformance) {
        beforeSortTime = new Date();
    }

	if(changeCursor)
		SetBusyCursor(window, true);
	var result = SortColumn(node, sortKey, secondarySortKey, direction);
	if(changeCursor)
		SetBusyCursor(window, false);

	if(showPerformance) {
	    var afterSortTime = new Date();
	    var timeToSort = (afterSortTime.getTime() - beforeSortTime.getTime())/1000;
		dump("timeToSort is " + timeToSort + " seconds\n");
    }

  SortDBView(sortKey,direction);
	return result;
}

var nsMsgViewSortType = Components.interfaces.nsMsgViewSortType;
var nsMsgViewSortOrder = Components.interfaces.nsMsgViewSortOrder;
var nsMsgViewFlagsType = Components.interfaces.nsMsgViewFlagsType;
var nsMsgViewCommandType = Components.interfaces.nsMsgViewCommandType;

var gDBView = null;

function CreateDBView(msgFolder, isThreaded, viewType, sortKey, sortDirection)
{
    dump("XXX CreateDBView(" + msgFolder + "," + isThreaded + "," + viewType + "," + sortKey + "," + sortDirection + ")\n");

    var dbviewContractId = "@mozilla.org/messenger/msgdbview;1?type=";

    // eventually, we will not be using nsMsgViewType.eShow*, but for now
    // this will help us mirror the thread pane
    var dbViewType = ConvertViewType(viewType);
    switch (dbViewType) {
        case nsMsgViewType.eShowUnread:
            dbviewContractId += "threadswithunread";
            break;
        case nsMsgViewType.eShowWatched:
            dbviewContractId += "watchedthreadswithunread";
            break;
        case nsMsgViewType.eShowAll:
        case nsMsgViewType.eShowRead:
        default:
            dbviewContractId += "threaded";
            break;
    }

    dump("XXX creating " + dbviewContractId + "\n");
    gDBView = Components.classes[dbviewContractId].createInstance(Components.interfaces.nsIMsgDBView);

    var sortType = ConvertSortKey(sortKey);
    var sortOrder = ConvertSortDirection(sortDirection)

    var viewFlags;
    if (isThreaded) {
        viewFlags = nsMsgViewFlagsType.kOutlineDisplay;
    }
    else {  
        viewFlags = nsMsgViewFlagsType.kFlatDisplay;
    }

    var count = new Object;
    gDBView.init(messenger, msgWindow);
    gDBView.open(msgFolder, sortType, sortOrder, viewFlags, count);

    var colID = FindOutlinerColumnBySortType(sortType);
    if (colID)
    {
      var column = document.getElementById(colID);
      gDBView.sortedColumn = column;
    }

    dump("XXX count = " + count.value + "\n");
}

function SetViewFlags(viewFlags)
{
    if (!gDBView) return;
    dump("XXX SetViewFlags(" + viewFlags + ")\n");
    gDBView.viewFlags = viewFlags;
}

function SortDBView(sortKey, direction)
{
    if (!gDBView) return;

    dump("XXX SortDBView(" + sortKey + "," + direction + ")\n");

    var sortOrder = ConvertSortDirection(direction);
    var sortType = ConvertSortKey(sortKey);

    gDBView.sort(sortType,sortOrder);
}

function ConvertSortDirection(direction)
{
    if (direction == "ascending") {
        return nsMsgViewSortOrder.ascending;
    }
    else {
        return nsMsgViewSortOrder.descending;
    }
}

function ConvertSortKey(sortKey)
{
    switch (sortKey) {
        case 'http://home.netscape.com/NC-rdf#Date':
            return nsMsgViewSortType.byDate;
        case 'http://home.netscape.com/NC-rdf#Subject':
            return nsMsgViewSortType.bySubject;
        case 'http://home.netscape.com/NC-rdf#Sender':
            return nsMsgViewSortType.byAuthor;
        case 'http://home.netscape.com/NC-rdf#ID':
            return nsMsgViewSortType.byId;
        case 'http://home.netscape.com/NC-rdf#Thread':
            return nsMsgViewSortType.byThread;
        case 'http://home.netscape.com/NC-rdf#Priority':
            return nsMsgViewSortType.byPriority;
        case 'http://home.netscape.com/NC-rdf#Status':
            return nsMsgViewSortType.byStatus;
        case 'http://home.netscape.com/NC-rdf#Size':
            return nsMsgViewSortType.bySize;
        case 'http://home.netscape.com/NC-rdf#Flagged':
            return nsMsgViewSortType.byFlagged;
        case 'http://home.netscape.com/NC-rdf#IsUnread':
            return nsMsgViewSortType.byUnread;
        case 'http://home.netscape.com/NC-rdf#Recipient':
            return nsMsgViewSortType.byRecipient;
        default:
            dump("unexpected\n");
            break;
    }
    return nsMsgViewSortType.byDate;
}

function DumpView()
{
    dump("XXX DumpView()\n");
    gDBView.dumpView();
}

//------------------------------------------------------------
// Sets the column header sort icon based on the requested 
// column and direction.
// 
// Notes:
// (1) This function relies on the first part of the 
//     <treecell id> matching the <treecol id>.  The treecell
//     id must have a "Header" suffix.
// (2) By changing the "sortDirection" attribute, a different
//     CSS style will be used, thus changing the icon based on
//     the "sortDirection" parameter.
//------------------------------------------------------------
function UpdateSortIndicator(column,sortDirection)
{
  // this is obsolete
}

function UpdateSortMenu(currentSortColumn)
{
/*
	UpdateSortMenuitem(currentSortColumn, "sortByDateMenuitem", "DateColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByFlagMenuitem", "FlaggedButtonColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByOrderReceivedMenuitem", "OrderReceivedColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByPriorityMenuitem", "PriorityColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySenderMenuitem", "AuthorColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySizeMenuitem", "MemoryColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByStatusMenuitem", "StatusColumn");
	UpdateSortMenuitem(currentSortColumn, "sortBySubjectMenuitem", "SubjectColumn");
	UpdateSortMenuitem(currentSortColumn, "sortByUnreadMenuitem", "UnreadButtonColumn");
*/
}

function UpdateSortMenuitem(currentSortColumnID, menuItemID, columnID)
{
	var menuItem = document.getElementById(menuItemID);
	if(menuItem)
	{
		menuItem.setAttribute("checked", currentSortColumnID == columnID);
	}
}

function SortFolderPane(column, sortKey)
{
	var node = FindInSidebar(window, column);
	if(!node)
	{
		dump('Couldnt find sort column\n');
		return false;
	}
	SortColumn(node, sortKey, null, null);
	//Remove the sortActive attribute because we don't want this pane to have any
	//sort styles.
	node.setAttribute("sortActive", "false");
	return true;
}

function SortColumn(node, sortKey, secondarySortKey, direction)
{
	//dump('In SortColumn\n');
	var xulSortService = Components.classes["@mozilla.org/rdf/xul-sort-service;1"].getService();

	if (xulSortService)
	{
		xulSortService = xulSortService.QueryInterface(Components.interfaces.nsIXULSortService);
		if (xulSortService)
		{
			// sort!!!
			var sortDirection;
			if(direction)
				sortDirection = direction;
			else
			{
				var currentDirection = node.getAttribute('sortDirection');
				if (currentDirection == "ascending")
						sortDirection = "descending";
				else if (currentDirection == "descending")
						sortDirection = "ascending";
				else    sortDirection = "ascending";
			}

			try
			{
				if(secondarySortKey)
					node.setAttribute('resource2', secondarySortKey);
				xulSortService.Sort(node, sortKey, sortDirection);
			}
			catch(e)
			{
                		//dump("Sort failed: " + e + "\n");
			}
		}
	}

}

function GetSelectedFolderResource()
{
	var folderTree = GetFolderTree();
	var selectedFolderList = folderTree.selectedItems;
	var selectedFolder = selectedFolderList[0];
	var uri = selectedFolder.getAttribute('id');


	var folderResource = RDF.GetResource(uri);
	return folderResource;

}



function ToggleMessageRead(treeItem)
{
/* 
	var tree = GetThreadTree();

	var messageResource = RDF.GetResource(treeItem.getAttribute('id'));

	var property = RDF.GetResource('http://home.netscape.com/NC-rdf#IsUnread');
	var result = tree.database.GetTarget(messageResource, property , true);
	result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
	var isUnread = (result.Value == "true")

	var message = messageResource.QueryInterface(Components.interfaces.nsIMessage);
	var messageArray = new Array(1);
	messageArray[0] = message;

	MarkMessagesRead(tree.database, messageArray, isUnread);
*/
}

function ToggleMessageFlagged(treeItem)
{
/*
	var tree = GetThreadTree();

	var messageResource = RDF.GetResource(treeItem.getAttribute('id'));

	var property = RDF.GetResource('http://home.netscape.com/NC-rdf#Flagged');
	var result = tree.database.GetTarget(messageResource, property , true);
	result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
	var flagged = (result.Value == "flagged")

	var message = messageResource.QueryInterface(Components.interfaces.nsIMessage);
	var messageArray = new Array(1);
	messageArray[0] = message;

	MarkMessagesFlagged(tree.database, messageArray, !flagged);
*/
}

//Called when the splitter in between the thread and message panes is clicked.
function OnClickThreadAndMessagePaneSplitter()
{
/*
	dump("We are in OnClickThreadAndMessagePaneSplitter()\n");
	var collapsed = IsThreadAndMessagePaneSplitterCollapsed();
	//collapsed is the previous state so we know we are opening.
	if(collapsed)
  {
		LoadSelectionIntoMessagePane();	
    setTimeout("PositionThreadPane();",0);
   }
*/                
}

function PositionThreadPane()
{
/*
       var tree = GetThreadTree();
  
       var selArray = tree.selectedItems;
           
       if ( selArray && (selArray.length > 0))
       try { 
       tree.ensureElementIsVisible(selArray[0]);
       }
       catch(e) { }
*/
}

function FolderPaneSelectionChange()
{
	var tree = GetFolderTree();
	if(tree)
	{
		var selArray = tree.selectedItems;
		if ( selArray && (selArray.length == 1) )
        {
			ChangeFolderByDOMNode(selArray[0]);
        }
		else
		{
			ClearThreadPane();
		}
	}
        if (!gAccountCentralLoaded)
            ClearMessagePane();

}

function ClearThreadPane()
{
}

function OpenFolderTreeToFolder(folderURI)
{
	var tree = GetFolderTree();
	return OpenToFolder(tree, folderURI);

}

function OpenToFolder(item, folderURI)
{

	if(item.nodeType != Node.ELEMENT_NODE)
		return null;

	var uri = item.getAttribute('id');
	dump(uri);
	dump('\n');
	if(uri == folderURI)
	{
		dump('found folder: ' + uri);
		dump('\n');
		return item;
	}

	var children = item.childNodes;
	var length = children.length;
	var i;
	dump('folder ' + uri);
	dump('has ' + length);
	dump('children\n');
	for(i = 0; i < length; i++)
	{
		var child = children[i];
		var folder = OpenToFolder(child, folderURI);
		if(folder)
		{
			child.setAttribute('open', 'true');
			return folder;
		}
	}
	return null;
}


function IsSpecialFolder(msgFolder, specialFolderNames)
{
	var db = GetFolderDatasource();
	var folderResource = msgFolder.QueryInterface(Components.interfaces.nsIRDFResource);
	if(folderResource)
	{
		var property =
			RDF.GetResource('http://home.netscape.com/NC-rdf#SpecialFolder');
		if (!property) return false;
		var result = db.GetTarget(folderResource, property , true);
		if (!result) return false;
		result = result.QueryInterface(Components.interfaces.nsIRDFLiteral);
		if (!result) return false;

		//dump("We are looking for " + specialFolderNames + "\n");
		//dump("special folder name = " + result.Value + "\n");
        
        var count = specialFolderNames.length;
        var i;
        for (i = 0; i < count; i++) {
            if(result.Value == specialFolderNames[i])
                return true;
        }
	}

	return false;
}

function ConvertViewType(viewType)
{
    if (!viewType || (viewType == "")) {
        return nsMsgViewType.eShowAll;
    }
    else {
        return viewType;
    }
}


function SetViewType(viewType)
{
    if(messageView)
    {
        messageView.viewType = ConvertViewType(viewType);
    }
}

function ShowThreads(showThreads)
{
	//dump('in showthreads\n');
	if(messageView)
	{
		messageView.showThreads = showThreads;
		var threadColumn = document.getElementById('ThreadColumnHeader');
		if(threadColumn)
		{
			if(showThreads)
			{
				threadColumn.setAttribute('currentView', 'threaded');
			}
			else
			{
				threadColumn.setAttribute('currentView', 'unthreaded');
			}
		}
	}
    
    var viewFlags;
    if (showThreads) {
        viewFlags = nsMsgViewFlagsType.kOutlineDisplay;
    }
    else {
        viewFlags = nsMsgViewFlagsType.kFlatDisplay;
    }
    SetViewFlags(viewFlags);
}


function GetNextMessageAfterDelete(messages)
{
/*
	var count = messages.length;

	var curMessage = messages[0];
	var nextMessage = null;
	var tree = GetThreadTree();

	//search forward
	while(curMessage)
	{
		nextMessage = msgNavigationService.FindNextMessage(navigateAny, tree, curMessage, RDF, document, false, messageView.showThreads);
		if(nextMessage)
		{
			if(nextMessage.getAttribute("selected") != "true")
			{
				break;
			}
		}
		curMessage = nextMessage;
	}
	//if no nextmessage then search backwards
	if(!nextMessage)
	{

		curMessage = messages[0];
		nextMessage = null;
		//search forward
		while(curMessage)
		{
			nextMessage = msgNavigationService.FindPreviousMessage(navigateAny, tree, curMessage, RDF, document, false, messageView.showThreads);
			if(nextMessage)
			{
				if(nextMessage.getAttribute("selected") != "true")
				{
					break;
				}
			}
			curMessage = nextMessage;
		}



	}
	return nextMessage;
*/
}


function SelectNextMessage(nextMessage)
{
/*
	var tree = GetThreadTree();
	ChangeSelection(tree, nextMessage);
*/
}

function GetSelectTrashUri(folder)
{
    if (!folder) return null;
    var uri = folder.getAttribute('id');
    dump (uri + "\n");
    var resource = RDF.GetResource(uri);
    var msgFolder =
        resource.QueryInterface(Components.interfaces.nsIMsgFolder);
    if (msgFolder)
    {
        dump("GetSelectTrashUri" + "\n");
        var rootFolder = msgFolder.rootFolder;
        var numFolder;
        var out = new Object();
        var trashFolder = rootFolder.getFoldersWithFlag(0x0100, 1, out); 
        numFolder = out.value;
        dump (numFolder + "\n");
        if (trashFolder)
        {
            dump (trashFolder.URI + "\n");
            return trashFolder.URI;
        }
    }
    return null;
}

function Undo()
{
    messenger.Undo(msgWindow);
}

function Redo()
{
    messenger.Redo(msgWindow);
}

