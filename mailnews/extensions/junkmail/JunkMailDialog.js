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
 * Copyright (C) 1998-2002 Netscape Communications Corporation. All
 * Rights Reserved.
 */

var gRdfService;
var gSelectedServer;

var hPrefWindow = null;

function Startup()
{
    hPrefWindow = new nsPrefWindow('junkMailFrame');  
      
    if( !hPrefWindow )
        throw "failed to create prefwindow";

    // If this call worked, we could center the window here:
    // centerWindowOnScreen();

    gRdfService = Components.classes["@mozilla.org/rdf/rdf-service;1"]
        .getService(Components.interfaces.nsIRDFService);

    // XXX why do we care about this?
    //
    moveToAlertPosition();

    // get the selected server if it can have filters.
    //
    gSelectedServer = getSelectedServerForFilters();

    // if the selected server cannot have filters, get the default server
    // failing that, check all accounts and get a server that can have filters.
    //
    if (!gSelectedServer) {
        gSelectedServer = getServerThatCanHaveFilters();
    }

    if (gSelectedServer) {
        // make the menu preselect this server
        //
        selectServer(gSelectedServer);

        // tell the nsPrefsWindow code to use the data associated with this
        // server (the server URI is used as the page tag)
        //
        document.getElementById("junkMailFrame") 
            .setAttribute("tag", gSelectedServer); 
    }

    hPrefWindow.init();
}

function onOk()
{
    window.close();
}

function onCancel()
{
}

function onServerClick(event)
{
    // switch to the same page, but with a different tag (in this case, 
    // the mail server URI).
    //
    hPrefWindow.switchPage(
        "chrome://messenger-junkmail/content/JunkMailPane.xul", 
        event.target.id);
}

/**
 * sets up the menulist
 */
function selectServer(uri)
{
    // update the server menu
    var serverMenu = document.getElementById("serverMenu");
    var menuitems = serverMenu.getElementsByAttribute("id", uri);
    serverMenu.selectedItem = menuitems[0];
}

/**
 * get the selected server if it can have filters
 */
function getSelectedServerForFilters()
{
    var firstItem = null;
    var args = window.arguments;
    var selectedFolder = args[0].folder;

    if (args && args[0] && selectedFolder)
    {
        var msgFolder = selectedFolder.QueryInterface(
            Components.interfaces.nsIMsgFolder);
        try
        {
            var rootFolder = msgFolder.rootFolder;
            if (rootFolder.isServer)
            {
                var server = rootFolder.server;

                if (server.canHaveFilters)
                {
                    firstItem = rootFolder.URI;
                }
            }
        }
        catch (ex)
        {
        }
    }

    return firstItem;
}

/** 
 * if the selected server cannot have filters, get the default server
 * if the default server cannot have filters, check all accounts
 * and get a server that can.
 */
function getServerThatCanHaveFilters()
{
    var firstItem = null;

    var accountManager
        = Components.classes["@mozilla.org/messenger/account-manager;1"].
            getService(Components.interfaces.nsIMsgAccountManager);

    var defaultAccount = accountManager.defaultAccount;
    var defaultIncomingServer = defaultAccount.incomingServer;

    // check to see if default server can have filters
    if (defaultIncomingServer.canHaveFilters) {
        firstItem = defaultIncomingServer.serverURI;
    }
    // if it cannot, check all accounts to find a server
    // that can have filters
    else
    {
        var allServers = accountManager.allServers;
        var numServers = allServers.Count();
        var index = 0;
        for (index = 0; index < numServers; index++)
        {
            var currentServer = allServers.GetElementAt(index)
                .QueryInterface(Components.interfaces.nsIMsgIncomingServer);

            if (currentServer.canHaveFilters)
            {
                firstItem = currentServer.serverURI;
                break;
            }
        }
    }

    return firstItem;
}

function doHelpButton()
{
  openHelp("spam-filters");
}
