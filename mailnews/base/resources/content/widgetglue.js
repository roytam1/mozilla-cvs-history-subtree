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
 *
 * Contributor(s):
 *   Jan Varga (varga@utcru.sk)
 *   Hakan Waara (hwaara@chello.se)
 */

/*
 * widget-specific wrapper glue. There should be one function for every
 * widget/menu item, which gets some context (like the current selection)
 * and then calls a function/command in commandglue
 */
 
//The eventual goal is for this file to go away and its contents to be brought into
//mailWindowOverlay.js.  This is currently being done.

//NOTE: gMessengerBundle must be defined and set or this Overlay won't work

function ConvertDOMListToResourceArray(nodeList)
{
    var result = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);

    for (var i=0; i<nodeList.length; i++) {
        result.AppendElement(nodeList[i].resource);
    }

    return result;
}

function GetSelectedFolderURI()
{
    var folderOutliner = GetFolderOutliner();
    var startIndex = {};
    var endIndex = {};
    folderOutliner.outlinerBoxObject.selection.getRangeAt(0, startIndex, endIndex);
    if (startIndex.value >= 0 && startIndex.value == endIndex.value)
    {
        var folderResource = GetFolderResource(startIndex.value);
        return folderResource.Value;
    }

    return null;
}


function MsgRenameFolder() 
{
	var preselectedURI = GetSelectedFolderURI();
	var folderOutliner = GetFolderOutliner();

	var name = GetFolderNameFromUri(preselectedURI, folderOutliner);

	dump("preselectedURI = " + preselectedURI + "\n");
	var dialog = window.openDialog(
	             "chrome://messenger/content/renameFolderDialog.xul",
	             "newFolder",
	             "chrome,titlebar,modal",
	             {preselectedURI: preselectedURI, 
	              okCallback: RenameFolder, name: name});
}

function RenameFolder(name,uri)
{
    dump("uri,name = " + uri + "," + name + "\n");
    var folderOutliner = GetFolderOutliner();
    if (folderOutliner)
    {
	if (uri && (uri != "") && name && (name != "")) {
                var selectedFolder = GetResourceFromUri(uri);
                try
                {
                    messenger.RenameFolder(GetFolderDatasource(), selectedFolder, name);
                }
                catch(e)
                {
                    throw(e); // so that the dialog does not automatically close
                    dump ("Exception : RenameFolder \n");
                }

                ClearThreadPane();
                ClearMessagePane();
                folderOutliner.outlinerBoxObject.selection.clearSelection();
        }
        else {
                dump("no name or nothing selected\n");
        }   
    }
    else {
	dump("no folder tree\n");
    }
}

function MsgEmptyTrash() 
{
    var folderOutliner = GetFolderOutliner();
    var startIndex = {};
    var endIndex = {};
    folderOutliner.outlinerBoxObject.selection.getRangeAt(0, startIndex, endIndex);
    if (startIndex.value >= 0)
    {
        var folderResource = GetFolderResource(startIndex.value);
        try {
            messenger.EmptyTrash(GetFolderDatasource(), folderResource);
        }
            catch(e)
        {  
            dump ("Exception : messenger.EmptyTrash \n");
        }
    }
}

function MsgCompactFolder(isAll) 
{
    // Get the selected folders.
    var selectedFolders = GetSelectedMsgFolders();
    var selectedFolder;
    var folderResource;
    var selectedFolderUri = "";
    var isImap = false;

    if (selectedFolders.length == 1)
    {
        selectedFolder = selectedFolders[0];
        folderResource = selectedFolder.QueryInterface(Components.interfaces.nsIRDFResource);
        selectedFolderUri = folderResource.Value;
        if (selectedFolderUri.indexOf("imap:") != -1)
            isImap = true;
        else
        {
            // set this so we'll reload it when compact is done
            gCurrentFolderToReroot = selectedFolderUri;
 
            // save off the current sort and view info so we'll
            // use it on the folder loaded notification
            var msgDatabase = selectedFolder.getMsgDatabase(msgWindow);
            if (msgDatabase)
            {
                var dbFolderInfo = msgDatabase.dBFolderInfo;
                gCurrentLoadingFolderSortType = dbFolderInfo.sortType;
                gCurrentLoadingFolderSortOrder = dbFolderInfo.sortOrder;
                gCurrentLoadingFolderViewFlags = dbFolderInfo.viewFlags;
                gCurrentLoadingFolderViewType = dbFolderInfo.viewType;
            }

            ClearThreadPaneSelection();
            ClearThreadPane();
            ClearMessagePane();
        }
    }

    for(var i = 0; i < selectedFolders.length; i++)
    {
        selectedFolder = selectedFolders[i];
        folderResource = selectedFolder.QueryInterface(Components.interfaces.nsIRDFResource);
        try
        {
            messenger.CompactFolder(GetFolderDatasource(), folderResource, isAll);
        }
        catch(e)
        {
            dump ("Exception : messenger.CompactFolder \n");
        }
    }

    if (!isImap && selectedFolderUri)
    {
        var folderOutliner = GetFolderOutliner();
        folderOutliner.outlinerBoxObject.selection.clearSelection();
    }
}

function MsgFolderProperties() 
{
	var preselectedURI = GetSelectedFolderURI();
	var serverType = GetMsgFolderFromUri(preselectedURI).server.type;
	var folderOutliner = GetFolderOutliner();

	var name = GetFolderNameFromUri(preselectedURI, folderOutliner);

	var windowTitle = gMessengerBundle.getString("folderProperties");
	var dialog = window.openDialog(
                    "chrome://messenger/content/imapFolderProps.xul",
                    "",
                    "chrome,centerscreen,titlebar,modal",
                    {preselectedURI:preselectedURI, serverType:serverType,
					title:windowTitle,
                    okCallback:FolderProperties, 
                    tabID:"", tabIndex:0, name:name});
}

function FolderProperties(name, uri)
{
}

function MsgToggleMessagePane()
{
	//OnClickThreadAndMessagePaneSplitter is based on the value before the splitter is toggled.
	OnClickThreadAndMessagePaneSplitterGrippy();
  MsgToggleSplitter("threadpane-splitter");
}

function MsgToggleFolderPane()
{
    MsgToggleSplitter("sidebarsplitter");
}

function MsgToggleSplitter(id)
{
    var splitter = document.getElementById(id);
    var state = splitter.getAttribute("state");
    if (state == "collapsed")
        splitter.setAttribute("state", null);
    else
        splitter.setAttribute("state", "collapsed")
}


function NotifyQuitApplication()
{
	var ObserverService = Components.classes["@mozilla.org/observer-service;1"].getService();
	ObserverService = ObserverService.QueryInterface(Components.interfaces.nsIObserverService);
	if (ObserverService)
	{
		try 
		{
			ObserverService.Notify(null, "quit-application", null);
		} 
		catch (ex) 
		{
			// dump("no observer found \n");
		}
	}
}

function LastToClose()
{
	var windowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
	var	windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
	var enumerator = windowManagerInterface.getEnumerator( null );
    var count = 0;

	while ( enumerator.hasMoreElements() && count < 2 )
	{
		var  windowToClose = enumerator.getNext();
        count++;
    }
    if (count == 1)
        return true;
    else
        return false;
}

function MsgSetFolderCharset() 
{
  MsgFolderProperties() 
}
