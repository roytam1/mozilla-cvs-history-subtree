/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is ChatZilla
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

const DEFAULT_NICK = "IRCMonkey"

function initPrefs()
{
    var dir = getSpecialDirectory("ProfD");

    client.prefManager = new PrefManager("extensions.irc.");
    client.prefs = client.prefManager.prefs;
    
    var prefs =
        [
         ["nickname",        DEFAULT_NICK],
         ["username",        "chatzilla"],
         ["desc",            "New Now Know How"],
         ["charset",         "utf-8"],
         ["collapseMsgs",    false],
         ["copyMessages",    true],
         ["log",             false],
         ["initialURLs",     []],
         ["initialScripts",  []],
         ["stalkWholeWords", true],
         ["stalkWords",      []],
         ["messages.click",     "goto-url"],
         ["messages.ctrlClick", "goto-url-newwin"],
         ["messages.metaClick", "goto-url-newtab"],
         ["motif.dark",      "chrome://chatzilla/skin/output-dark.css"],
         ["motif.light",     "chrome://chatzilla/skin/output-light.css"],
         ["motif.default",   "chrome://chatzilla/skin/output-default.css"],
         ["motif.current",   "chrome://chatzilla/skin/output-default.css"],
         ["outputWindowURL", "chrome://chatzilla/content/output-window.html"]
        ];

    client.prefManager.addPrefs(prefs);
    client.prefManager.onPrefChanged = onPrefChanged;

    initReadPrefs();
}

function getNetworkPrefManager(network)
{
    function defer(prefName)
    {
        return client.prefs[prefName];
    };

    function onPrefChanged(prefName, newValue, oldValue)
    {
        onNetworkPrefChanged (network, prefName, newValue, oldValue);
    };
    
    var prefs =
        [
         ["nickname",        defer],
         ["username",        defer],
         ["desc",            defer],
         ["log",             false],
         ["outputWindowURL", defer],
         ["motif.current",   defer]
        ];

    var branch = "extensions.irc.networks." + escape(network.name) + ".";
    var prefManager = new PrefManager(branch);
    prefManager.addPrefs(prefs);
    prefManager.onPrefChanged = onPrefChanged;
    
    return prefManager;
}

function getChannelPrefManager(channel)
{
    var network = channel.parent.parent;

    function defer(prefName)
    {
        return network.prefs[prefName];
    };
    
    function onPrefChanged(prefName, newValue, oldValue)
    {
        onChannelPrefChanged (channel, prefName, newValue, oldValue);
    };
    
    var prefs =
        [
         ["log",              false],
         ["outputWindowURL",  defer],
         ["motif.current",    defer]
        ];
    
    var branch = "extensions.irc.networks." + escape(network.name) +
        ".channels." + escape(channel.unicodeName);
    var prefManager = new PrefManager(branch);
    prefManager.addPrefs(prefs);
    prefManager.onPrefChanged = onPrefChanged;
    
    return prefManager;
}

function getUserPrefManager(user)
{
    var network = user.parent.parent;

    function defer(prefName)
    {
        return network.prefs[prefName];
    };
    
    function onPrefChanged(prefName, newValue, oldValue)
    {
        onUserPrefChanged (user, prefName, newValue, oldValue);
    };
    
    var prefs =
        [
         ["motif.current",    defer],
         ["outputWindowURL",  defer],
         ["log",              false]
        ];
    
    var branch = "extensions.irc.networks." + escape(network.name) +
        ".users." + escape(user.unicodeName);
    var prefManager = new PrefManager(branch);
    prefManager.addPrefs(prefs);
    prefManager.onPrefChanged = onPrefChanged;
    
    return prefManager;
}
                 
function destroyPrefs()
{
    if ("prefManager" in client)
        client.prefManager.destroy();
}

function onPrefChanged(prefName, newValue, oldValue)
{
    switch (prefName)
    {
        case "nickname":
            CIRCNetwork.prototype.INITIAL_NICK = newValue;
            break;

        case "username":
            CIRCNetwork.prototype.INITIAL_NAME = newValue;
            break;

        case "desc":
            CIRCNetwork.prototype.INITIAL_DESC = newValue;
            break;

        case "stalkWholeWords":
        case "stalkWords":
            updateAllStalkExpressions();
            break;
            
        case "motif.current":
            dispatch("sync-css");
            break;
    }
}

function onNetworkPrefChanged(network, prefName, newValue, oldValue)
{
    switch (prefName)
    {
        case "nickname":
            network.INITIAL_NICK = newValue;
            break;

        case "username":
            network.INITIAL_NAME = newValue;
            break;

        case "desc":
            network.INITIAL_DESC = newValue;
            break;

        case "motif.current":
            dispatch("sync-css");
            break;
    }
}

function onChannelPrefChanged(channel, prefName, newValue, oldValue)
{
    switch (prefName)
    {
        case "motif.current":
            dispatch("sync-css");
    }
}

function onUserPrefChanged(channel, prefName, newValue, oldValue)
{
}

