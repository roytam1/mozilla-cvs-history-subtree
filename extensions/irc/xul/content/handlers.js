/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is ChatZilla
 *
 * The Initial Developer of the Original Code is New Dimensions Consulting,
 * Inc. Portions created by New Dimensions Consulting, Inc. are
 * Copyright (C) 1999 New Dimenstions Consulting, Inc. All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Robert Ginda, rginda@netscape.com, original author
 *  Samuel Sieb, samuel@sieb.net
 *  Chiaki Koufugata chiaki@mozilla.gr.jp UI i18n 
 */

window.onresize =
function onresize()
{
    for (var i = 0; i < client.deck.childNodes.length; i++)
        scrollDown(client.deck.childNodes[i], true);
}

function onTopicEditStart()
{
    if (client.currentObject.TYPE != "IRCChannel")
        return;

    var text = client.statusBar["channel-topic"];
    var edit = client.statusBar["channel-topicedit"];
    text.setAttribute("collapsed", "true");
    edit.removeAttribute("collapsed");
    edit.value = client.currentObject.topic;
    edit.focus();
    edit.selectionStart = 0;
    edit.selectionEnd = edit.value.length;
}

function onTopicEditEnd ()
{
    if (!("statusBar" in client))
        return;
    
    var edit = client.statusBar["channel-topicedit"];
    var text = client.statusBar["channel-topic"];
    edit.setAttribute("collapsed", "true");
    text.removeAttribute("collapsed");
    focusInput();
}

function onTopicKeyPress (e)
{
    if (client.currentObject.TYPE != "IRCChannel")
        return;
    
    if (e.keyCode == 13)
    {        
        var line = stringTrim(e.target.value);
        var charset = client.currentObject.charset;
        client.currentObject.setTopic (fromUnicode(line, charset));
        onTopicEditEnd();
    }
}

function onInputFocus ()
{
    if (!("statusBar" in client))
        return;
    
    var edit = client.statusBar["channel-topicedit"];
    var text = client.statusBar["channel-topic"];
    edit.setAttribute("collapsed", "true");
    text.removeAttribute("collapsed");
}

function onLoad()
{
    dd ("Initializing ChatZilla {");
    try
    {
        init();
    }
    catch (ex)
    {
        dd("caught exception while initializing:\n" + dumpObjectTree(ex));
    }
    
    dd("}");
    mainStep();
}

function initHandlers()
{
    var obj;
    obj = document.getElementById("input");
    obj.addEventListener("keypress", onInputKeyPress, false);
    obj = document.getElementById("multiline-input");
    obj.addEventListener("keypress", onMultilineInputKeyPress, false);
    obj = document.getElementById("channel-topicedit");
    obj.addEventListener("keypress", onTopicKeyPress, false);
    obj.active = false;

    window.onkeypress = onWindowKeyPress;
}

function onClose()
{
    if ("userClose" in client && client.userClose)
        return true;
    
    client.userClose = true;
    client.currentObject.display (getMsg("cli_closing"), "INFO");

    if (!("getConnectionCount" in client) ||
        client.getConnectionCount() == 0)
    {
        /* if we're not connected to anything, just close the window */
        return true;
    }

    /* otherwise, try to close out gracefully */
    client.quit (client.userAgent);
    return false;
}

function onUnload()
{
    dd("Shutting down ChatZilla.");
    destroy();
}

function onNotImplemented()
{

    alert (getMsg("onNotImplementedMsg"));
    
}

/* tab click */
function onTabClick (id)
{
    
    var tbi = document.getElementById (id);
    var view = client.viewsArray[tbi.getAttribute("viewKey")];

    setCurrentObject (view.source);
    
}

function onMouseOver (e)
{
    var i = 0;
    var target = e.target;
    var status = "";
    while (!status && target && i < 5)
    {
        if ("getAttribute" in target)
        {
            status = target.getAttribute("href");
            if (!status)
                status = target.getAttribute ("statusText");
        }
        ++i;
        target = target.parentNode;
    }
        
    if (status)
    {
        client.status = status;
    }
    else
    {
        if (client && "defaultStatus" in client)
            client.status = client.defaultStatus;
    }
}
    
function onPopupSimulateCommand (line)
{
    ASSERT(0, "not implemented");
    return;
    
    if ("user" in client._popupContext)
    {
        var nick = client._popupContext.user;
        if (nick.indexOf("ME!") != -1)
        {
            var details = getObjectDetails(client.currentObject);
            if ("server" in details)
                nick = details.server.me.properNick;
        }

        line = line.replace (/\$nick/ig, nick);
    }
    
    onInputCompleteLine ({line: line}, true);
}

function onPopupHighlight (ruleText)
{
    var user = client._popupContext.user;
    
    var rec = findDynamicRule(".msg[msg-user=" + user + "]");
    
    if (!ruleText)
    {
        if (rec)
            rec.sheet.deleteRule(rec.index);
        /* XXX just deleting it doesn't work */
        addDynamicRule (".msg[msg-user=\"" + user + "\"] { }");
    }
    else
    {
        if (rec)
            rec.sheet.deleteRule(rec.index);

        addDynamicRule (".msg[msg-user=\"" + user + "\"] " +
                        "{" + ruleText + "}");
    }

}

function onToggleMungerEntry(entryName)
{
    client.munger.entries[entryName].enabled =
        !client.munger.entries[entryName].enabled;
    var item = document.getElementById("menu-munger-" + entryName);
    item.setAttribute ("checked", client.munger.entries[entryName].enabled);
}

function createPopupContext(event, target)
{
    var targetType;
    client._popupContext = new Object();
    client._popupContext.menu = event.originalTarget;

    if (!target)
        return "unknown";
    
    switch (target.tagName.toLowerCase())
    {
        case "html:a":
            var href = target.getAttribute("href");
            client._popupContext.url = href;
            if (href.indexOf("irc://") == 0)
            {
                var obj = parseIRCUrl(href);
                if (obj)
                {
                    if (obj.target)
                        if (obj.isnick)
                        {
                            targetType="nick-ircurl";
                            client._popupContext.user = obj.target;
                        }
                        else
                            targetType="channel-ircurl";
                    else
                        targetType="untargeted-ircurl";
                }
                else
                    targetType="weburl";
            }
            else
                targetType="weburl";
            break;
            
        case "html:td":
            var user = target.getAttribute("msg-user");
            if (user)
            {
                if (user.indexOf("ME!") != -1)
                    client._popupContext.user = "ME!";
                else
                    client._popupContext.user = user;
            }
            targetType = target.getAttribute("msg-type");
            break;            
    }

    client._popupContext.targetType = targetType;
    client._popupContext.targetClass = target.getAttribute("class");

    return targetType;
}

function onOutputContextMenuCreate(e)
{
    function evalIfAttribute (node, attr)
    {
        var expr = node.getAttribute(attr);
        if (!expr)
            return true;
        
        expr = expr.replace (/\Wor\W/gi, " || ");
        expr = expr.replace (/\Wand\W/gi, " && ");
        return eval("(" + expr + ")");
    }
        
    var target = document.popupNode;
    var foundSomethingUseful = false;
    
    do
    {
        if ("tagName" in target &&
            (target.tagName == "html:a" || target.tagName == "html:td"))
            foundSomethingUseful = true;
        else
            target = target.parentNode;
    } while (target && !foundSomethingUseful);
    
    var targetType = createPopupContext(e, target);
    var targetClass = ("targetClass" in client._popupContext) ?
        client._popupContext.targetClass : "";
    var viewType = client.currentObject.TYPE;
    var targetIsOp = "n/a";
    var targetIsVoice = "n/a";
    var iAmOp = "n/a";
    var targetUser = ("user" in client._popupContext) ?
        String(client._popupContext.user) : "";
    var details = getObjectDetails(client.currentObject);
    var targetServer = ("server" in details) ? details.server : "";

    if (targetServer && targetUser == "ME!")
    {
        targetUser = targetServer.me.nick;
    }

    var targetProperNick = targetUser;
    if (targetServer && targetUser in targetServer.users)
        targetProperNick = targetServer.users[targetUser].properNick;
    
    if (viewType == "IRCChannel" && targetUser)
    {
        if (targetUser in client.currentObject.users)
        {
            var cuser = client.currentObject.users[targetUser];
            targetIsOp = cuser.isOp ? "yes" : "no";
            targetIsVoice = cuser.isVoice ? "yes" : "no";
        }
        
        var server = getObjectDetails(client.currentObject).server;
        if (server &&
            server.me.nick in client.currentObject.users &&
            client.currentObject.users[server.me.nick].isOp)
        {
            iAmOp =  "yes";
        }
        else
        {
            iAmOp = "no";
        }
    }

    var popup = document.getElementById ("outputContext");
    var menuitem = popup.firstChild;

    do
    {
        if (evalIfAttribute(menuitem, "visibleif"))
        {
            menuitem.setAttribute ("hidden", "false");
        }
        else
        {
            menuitem.setAttribute ("hidden", "true");
            continue;
        }
        
        if (menuitem.hasAttribute("checkedif"))
        {
            if (evalIfAttribute(menuitem, "checkedif"))
                menuitem.setAttribute ("checked", "true");
            else
                menuitem.setAttribute ("checked", "false");
        }
            
        var format = menuitem.getAttribute("format");
        if (format)
        {
            format = format.replace (/\$nick/gi, targetProperNick);
            format = format.replace (/\$viewname/gi,
                                     client.currentObject.unicodeName);
            menuitem.setAttribute ("label", format);
        }
        
    } while ((menuitem = menuitem.nextSibling));

    return true;
}

/* popup click in user list */
function onUserListPopupClick (e)
{

    var code = e.target.getAttribute("code");
    var ary = code.substr(1, code.length).match (/(\S+)? ?(.*)/);    

    if (!ary)
        return;

    var command = ary[1];

    var ev = new CEvent ("client", "input-command", client,
                         "onInputCommand");
    ev.command = command;
    ev.inputData =  ary[2] ? stringTrim(ary[2]) : "";    
    ev.target = client.currentObject;

    getObjectDetails (ev.target, ev);

    client.eventPump.addEvent (ev);
}


function onToggleTraceHook()
{
    var h = client.eventPump.getHook ("event-tracer");
    
    h.enabled = client.debugMode = !h.enabled;
    document.getElementById("menu-dmessages").setAttribute ("checked",
                                                            h.enabled);
    if (h.enabled)
        client.currentObject.display (getMsg("debug_on"), "INFO");
    else
        client.currentObject.display (getMsg("debug_off"), "INFO");
}

function onToggleSaveOnExit()
{
    client.SAVE_SETTINGS = !client.SAVE_SETTINGS;
    var m = document.getElementById ("menu-settings-autosave");
    m.setAttribute ("checked", String(client.SAVE_SETTINGS));

    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    pref.setBoolPref ("extensions.irc.settings.autoSave",
                      client.SAVE_SETTINGS);
}

function onToggleVisibility(thing)
{    
}

function onDoStyleChange (newStyle)
{

    if (newStyle == "other")
        newStyle = window.prompt (getMsg("onDoStyleChangeMsg"));

    if (newStyle)
    {
        setOutputStyle (newStyle);
        setCurrentObject(client.currentObject);
    }
    
}

function onSortCol(sortColName)
{
    var node = document.getElementById(sortColName);
    if (!node)
        return false;
 
    // determine column resource to sort on
    var sortResource = node.getAttribute("resource");
    var sortDirection = node.getAttribute("sortDirection");
    
    if (sortDirection == "ascending")
        sortDirection = "descending";
    else
        sortDirection = "ascending";
    
    sortUserList(node, sortDirection);
    
    return false;
}

function onToggleMunger()
{

    client.munger.enabled = !client.munger.enabled;
    var item = document.getElementById("menu-munger-global");
    item.setAttribute ("checked", !client.munger.enabled);

}

function onToggleColors()
{

    client.enableColors = !client.enableColors;
    document.getElementById("menu-colors").setAttribute ("checked",
                                                         client.enableColors);

}

function onMultilineInputKeyPress (e)
{
    if ((e.ctrlKey || e.metaKey) && e.keyCode == 13)
    {
        /* meta-enter, execute buffer */
        onMultilineSend(e);
    }
    else
    {
        if ((e.ctrlKey || e.metaKey) && e.keyCode == 40)
        {
            /* ctrl/meta-down, switch to single line mode */
            multilineInputMode (false);
        }
    }
}

function onMultilineSend(e)
{
    var multiline = document.getElementById("multiline-input");
    e.line = multiline.value;
    if (e.line.search(/\S/) == -1)
        return;
    onInputCompleteLine (e);
    multiline.value = "";
}

function onToggleMsgCollapse()
{
    client.COLLAPSE_MSGS = !client.COLLAPSE_MSGS;
}

function onToggleCopyMessages()
{
    client.COPY_MESSAGES = !client.COPY_MESSAGES;
}

function onToggleStartupURL()
{
    var tb = getTabForObject (client.currentObject);
    if (!tb)
        return;
    
    var vk = Number(tb.getAttribute("viewKey"));
    
    var ary = client.INITIAL_URLS ? 
        client.INITIAL_URLS.split(/\s*;\s*/) : new Array();
    var url = client.currentObject.getURL();
    var index = arrayIndexOf(ary, url);
    if (index != -1)
        arrayRemoveAt(ary, index);
    else
        ary.push(url);
    
    client.INITIAL_URLS = ary.join ("; ");
}

function onViewMenuShowing ()
{
    var loc = client.currentFrame.document.location.href;
    loc = loc.substr (loc.indexOf("?") + 1);

    var val = (loc == "chrome://chatzilla/skin/output-default.css");
    document.getElementById ("menu-view-default").setAttribute ("checked", val);

    val = (loc == "chrome://chatzilla/skin/output-dark.css");
    document.getElementById ("menu-view-dark").setAttribute ("checked", val);

    val = (loc == "chrome://chatzilla/skin/output-light.css");
    document.getElementById ("menu-view-light").setAttribute ("checked", val);

    val = client.COLLAPSE_MSGS;
    document.getElementById ("menu-view-collapse").setAttribute ("checked", val);

    val = client.COPY_MESSAGES;
    document.getElementById ("menu-view-copymsgs").setAttribute ("checked", val);
    
    val = isStartupURL(client.currentObject.getURL());
    document.getElementById ("menu-view-startup").setAttribute ("checked", val);
    return true;
}
    
function onInputKeyPress (e)
{
    
    switch (e.keyCode)
    {        
        case 9:  /* tab */
    		if (e.ctrlKey || e.metaKey)
                cycleView(e.shiftKey ? -1: 1);
            else
                onTabCompleteRequest(e);

            e.preventDefault();
            break;

        case 13: /* CR */
            e.line = e.target.value;
            e.target.value = "";
            if (e.line.search(/\S/) == -1)
                return;
            onInputCompleteLine (e);
            break;

        case 38: /* up */
            if (e.ctrlKey || e.metaKey)
            {
                /* ctrl/meta-up, switch to multi line mode */
                multilineInputMode (true);
            }
            else
            {
                if (client.lastHistoryReferenced == -2)
                {
                    client.lastHistoryReferenced = -1;
                    e.target.value = client.incompleteLine;
                }
                else if (client.lastHistoryReferenced <
                         client.inputHistory.length - 1)
                {
                    e.target.value =
                        client.inputHistory[++client.lastHistoryReferenced];
                }
            }
            e.preventDefault();
            break;

        case 40: /* down */
            if (client.lastHistoryReferenced > 0)
                e.target.value =
                    client.inputHistory[--client.lastHistoryReferenced];
            else if (client.lastHistoryReferenced == -1)
            {
                e.target.value = "";
                client.lastHistoryReferenced = -2;
            }
            else
            {
                client.lastHistoryReferenced = -1;
                e.target.value = client.incompleteLine;
            }
            e.preventDefault();
            break;

        default:
            client.lastHistoryReferenced = -1;
            client.incompleteLine = e.target.value;
            break;
    }

}

function onTest ()
{

}

function onTabCompleteRequest (e)
{
    var elem = document.commandDispatcher.focusedElement;
    var singleInput = document.getElementById("input");
    if (document.getBindingParent(elem) != singleInput)
        return;

    var selStart = singleInput.selectionStart;
    var selEnd = singleInput.selectionEnd;            
    var line = singleInput.value;

    if (!line)
    {
        if ("defaultCompletion" in client.currentObject)
            singleInput.value = client.currentObject.defaultCompletion;
        return;
    }
    
    if (selStart != selEnd) 
    {
        /* text is highlighted, just move caret to end and exit */
        singleInput.selectionStart = singleInput.selectionEnd = line.length;
        return;
    }

    var wordStart = line.substr(0, selStart).search(/\s\S*$/);
    if (wordStart == -1)
        wordStart = 0;
    else
        ++wordStart;
    
    var wordEnd = line.substr(selStart).search(/\s/);
    if (wordEnd == -1)
        wordEnd = line.length;
    else
        wordEnd += selStart;

    if ("performTabMatch" in client.currentObject)
    {
        var word = line.substring (wordStart, wordEnd);
        var matches = client.currentObject.performTabMatch (line, wordStart,
                                                            wordEnd,
                                                            word.toLowerCase(),
                                                            selStart);
        /* if we get null back, we're supposed to fail silently */
        if (!matches)
            return;

        var doubleTab = false;
        var date = new Date();
        if ((date - client.lastTabUp) <= client.DOUBLETAB_TIME)
            doubleTab = true;
        else
            client.lastTabUp = date;

        if (doubleTab)
        {
            /* if the user hit tab twice quickly, */
            if (matches.length > 0)
            {
                /* then list possible completions, */
                client.currentObject.display(getMsg("tabCompleteList",
                                                    [matches.length, word,
                                                     matches.join(MSG_CSP)]),
                                             "INFO");
            }
            else
            {
                /* or display an error if there are none. */
                client.currentObject.display(getMsg("tabCompleteError", [word]),
                                             "ERROR");
            }
        }
        else if (matches.length >= 1)
        {
            var match;
            if (matches.length == 1)
                match = matches[0];
            else
                match = getCommonPfx(matches);
            singleInput.value = line.substr(0, wordStart) + match + 
                    line.substr(wordEnd);
            if (wordEnd < line.length)
            {
                /* if the word we completed was in the middle if the line
                 * then move the cursor to the end of the completed word. */
                var newpos = wordStart + match.length;
                if (matches.length == 1)
                {
                    /* word was fully completed, move one additional space */
                    ++newpos;
                }
                singleInput.selectionEnd = e.target.selectionStart = newpos;
            }
        }
    }

}

function onWindowKeyPress (e)
{
    var code = Number (e.keyCode);
    var w;
    var newOfs;
    var userList = document.getElementById("user-list");
    var elemFocused = document.commandDispatcher.focusedElement;

    switch (code)
    {
        case 112: /* F1 */
        case 113: /* ... */
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 119:
        case 120:
        case 121: /* F10 */
            var idx = code - 112;
            if ((idx in client.viewsArray) && (client.viewsArray[idx].source))
                setCurrentObject(client.viewsArray[idx].source);
            break;

        case 33: /* pgup */
            if (elemFocused == userList)
                break;

            w = client.currentFrame;
            newOfs = w.pageYOffset - (w.innerHeight * 0.9);
            if (newOfs > 0)
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, 0);
            e.preventDefault();
            break;
            
        case 34: /* pgdn */
            if (elemFocused == userList)
                break;

            w = client.currentFrame;
            newOfs = w.pageYOffset + (w.innerHeight * 0.9);
            if (newOfs < (w.innerHeight + w.pageYOffset))
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, (w.innerHeight + w.pageYOffset));
            e.preventDefault();
            break;

        default:
            
    }

}

function onInputCompleteLine(e)
{
    if (!client.inputHistory.length || client.inputHistory[0] != e.line)
        client.inputHistory.unshift (e.line);
    
    if (client.inputHistory.length > client.MAX_HISTORY)
        client.inputHistory.pop();
    
    client.lastHistoryReferenced = -1;
    client.incompleteLine = "";
    
    if (e.line[0] == client.COMMAND_CHAR)
    {
        dispatch(e.line.substr(1), null, true);
    }
    else /* plain text */
    {
        /* color codes */
        if (client.COLORCODES)
        {
            e.line = e.line.replace(/%U/g, "\x1f");
            e.line = e.line.replace(/%B/g, "\x02");
            e.line = e.line.replace(/%O/g, "\x0f");
            e.line = e.line.replace(/%C/g, "\x03");
            e.line = e.line.replace(/%R/g, "\x16");
        }
        client.sayToCurrentTarget (e.line);
    }
}

function onNotifyTimeout ()
{
    for (var n in client.networks)
    {
        var net = client.networks[n];
        if (net.isConnected()) {
            if ("notifyList" in net && net.notifyList.length > 0) {
                net.primServ.sendData ("ISON " +
                                       client.networks[n].notifyList.join(" ")
                                       + "\n");
            } else {
                /* if the notify list is empty, just send a ping to see if we're
                 * alive. */
                net.primServ.sendData ("PING :ALIVECHECK\n");
            }
        }
    }
}

/* 'private' function, should only be used from inside */
CIRCChannel.prototype._addUserToGraph =
function my_addtograph (user)
{
    if (!user.TYPE)
        dd (getStackTrace());
    
    client.rdf.Assert (this.getGraphResource(), client.rdf.resChanUser,
                       user.getGraphResource(), true);
    
}

/* 'private' function, should only be used from inside */
CIRCChannel.prototype._removeUserFromGraph =
function my_remfgraph (user)
{

    client.rdf.Unassert (this.getGraphResource(), client.rdf.resChanUser,
                         user.getGraphResource());
    
}

CIRCNetwork.prototype.onInit =
function net_oninit ()
{
    var myBranch = this.name.replace(/[^\w\d]/g, encodeChar);
    this.localPrefs =
        client.prefService.getBranch(client.prefBranch.root + "viewList." +
                                     myBranch + ".");
}

CIRCNetwork.prototype.onInfo =
function my_netinfo (e)
{
    this.display (e.msg, "INFO");
}

CIRCNetwork.prototype.onUnknown =
function my_unknown (e)
{
    e.params.shift(); /* remove the code */
    e.params.shift(); /* and the dest. nick (always me) */
        /* if it looks like some kind of "end of foo" code, and we don't
         * already have a mapping for it, make one up */
    var length = e.params.length;
    if (!(e.code in client.responseCodeMap) && 
        (e.params[length - 1].search (/^end of/i) != -1))
    {
        client.responseCodeMap[e.code] = "---";
    }
    
    this.display (e.params.join(" "), e.code.toUpperCase());
}

CIRCNetwork.prototype.on001 = /* Welcome! */
CIRCNetwork.prototype.on002 = /* your host is */
CIRCNetwork.prototype.on003 = /* server born-on date */
CIRCNetwork.prototype.on004 = /* server id */
CIRCNetwork.prototype.on005 = /* server features */
CIRCNetwork.prototype.on250 = /* highest connection count */
CIRCNetwork.prototype.on251 = /* users */
CIRCNetwork.prototype.on252 = /* opers online (in params[2]) */
CIRCNetwork.prototype.on254 = /* channels found (in params[2]) */
CIRCNetwork.prototype.on255 = /* link info */
CIRCNetwork.prototype.on265 = /* local user details */
CIRCNetwork.prototype.on266 = /* global user details */
CIRCNetwork.prototype.on375 = /* start of MOTD */
CIRCNetwork.prototype.on372 = /* MOTD line */
CIRCNetwork.prototype.on376 = /* end of MOTD */
function my_showtonet (e)
{
    var p = (3 in e.params) ? e.params[2] + " " : "";
    var str = "";

    switch (e.code)
    {
        case "004":
        case "005":
            str = e.params.slice(3).join (" ");
            break;

        case "001":
            updateTitle(this);
            updateNetwork (this);
            updateStalkExpression(this);
            this.prefs["nickname"] = e.server.me.properNick
            if (client.currentObject == this)
            {
                var status = document.getElementById("offline-status");
                status.removeAttribute ("offline");
            }
            if ("pendingURLs" in this)
            {
                var url = this.pendingURLs.pop();
                while (url)
                {
                    gotoIRCURL(url);
                    url = this.pendingURLs.pop();
                }
                delete this.pendingURLs;
            }
            for (var v in client.viewsArray)
            {
                // reconnect to any existing views
                var source = client.viewsArray[v].source;
                var details = getObjectDetails(client.viewsArray[v].source);
                if ("network" in details && details.network == this)
                    gotoIRCURL(source.getURL());
            }

            str = e.params[2];
            break;
            
        case "372":
        case "375":
        case "376":
            if (this.IGNORE_MOTD)
                return;
            /* no break */

        default:
            var length = e.params.length;
            str = e.params[length - 1];
            break;
    }

    this.displayHere (p + str, e.code.toUpperCase());
    
}

CIRCNetwork.prototype.onUnknownCTCPReply = 
function my_ctcprunk (e)
{
    this.display (getMsg("my_ctcprunk",
                         [e.CTCPCode, e.CTCPData, e.user.properNick]),
                         "CTCP_REPLY", e.user, e.server.me);
}

CIRCNetwork.prototype.onNotice = 
function my_notice (e)
{
    this.display (toUnicode(e.params[2]), "NOTICE", this, e.server.me);
}

CIRCNetwork.prototype.on303 = /* ISON (aka notify) reply */
function my_303 (e)
{
    var onList = stringTrim(e.params[1].toLowerCase()).split(/\s+/);
    var offList = new Array();
    var newArrivals = new Array();
    var newDepartures = new Array();
    var o = getObjectDetails(client.currentObject);
    var displayTab;
    var i;

    if ("network" in o && o.network == this && client.currentObject != this)
        displayTab = client.currentObject;

    for (i in this.notifyList)
        if (!arrayContains(onList, this.notifyList[i]))
            /* user is not on */
            offList.push (this.notifyList[i]);
        
    if ("onList" in this)
    {
        for (i in onList)
            if (!arrayContains(this.onList, onList[i]))
                /* we didn't know this person was on */
                newArrivals.push(onList[i]);
    }
    else
        this.onList = newArrivals = onList;

    if ("offList" in this)
    {
        for (i in offList)
            if (!arrayContains(this.offList, offList[i]))
                /* we didn't know this person was off */
                newDepartures.push(offList[i]);
    }
    else
        this.offList = newDepartures = offList;
    
    if (newArrivals.length > 0)
    {
        this.displayHere (arraySpeak (newArrivals, "is", "are") +
                          " online.", "NOTIFY-ON");
        if (displayTab)
            displayTab.displayHere (arraySpeak (newArrivals, "is", "are") +
                                    " online.", "NOTIFY-ON");
    }
    
    if (newDepartures.length > 0)
    {
        this.displayHere (arraySpeak (newDepartures, "is", "are") +
                          " offline.", "NOTIFY-OFF");
        if (displayTab)
            displayTab.displayHere (arraySpeak (newDepartures, "is", "are") +
                                    " offline.", "NOTIFY-OFF");
    }

    this.onList = onList;
    this.offList = offList;
    
}

CIRCNetwork.prototype.listInit =
function my_list_init ()
{

    function checkEndList (network)
    {
        if (network.list.count == network.list.lastLength)
        {
            network.on323();
        }
        else
        {
            network.list.lastLength = network.list.count;
            network.list.endTimeout =
                setTimeout(checkEndList, 1500, network);
        }
    }

    function outputList (network)
    {
        const CHUNK_SIZE = 5;
        var list = network.list;
        if (list.length > list.displayed)
        {
            var start = list.displayed;
            var end = list.length;
            if (end - start > CHUNK_SIZE)
                end = start + CHUNK_SIZE;
            for (var i = start; i < end; ++i)
                network.displayHere (getMsg("my_322", list[i]), "322");
            list.displayed = end;
        }
        if (list.done && (list.displayed == list.length))
        {
            if (list.event323)
            {
                var length = list.event323.params.length;
                network.displayHere (list.event323.params[length - 1], "323");
            }
            network.displayHere (getMsg("my_323", [list.displayed, list.count]),
                                 "INFO");
            delete network.list;
        }
        else
        {
            setTimeout(outputList, 250, network);
        }
    }

    if (!("list" in this))
    {
        this.list = new Array();
        this.list.regexp = null;
    }
    if (client.currentObject != this)
        client.currentObject.display (getMsg("my_321", this.name), "INFO");
    this.list.lastLength = 0;
    this.list.done = false;
    this.list.count = 0;
    this.list.displayed = 0;
    setTimeout(outputList, 250, this);
    this.list.endTimeout = setTimeout(checkEndList, 1500, this);
}

CIRCNetwork.prototype.on321 = /* LIST reply header */
function my_321 (e)
{

    this.listInit();
    this.displayHere (e.params[2] + " " + e.params[3], "321");
}

CIRCNetwork.prototype.on323 = /* end of LIST reply */
function my_323 (e)
{
    if (this.list.endTimeout)
    {
        clearTimeout(this.list.endTimeout);
        delete this.list.endTimeout;
    }
    this.list.done = true;
    this.list.event323 = e;
}

CIRCNetwork.prototype.on322 = /* LIST reply */
function my_listrply (e)
{
    if (!("list" in this) || !("done" in this.list))
        this.listInit();
    ++this.list.count;
    e.params[2] = toUnicode(e.params[2]);
    if (!(this.list.regexp) || e.params[2].match(this.list.regexp)
                            || e.params[4].match(this.list.regexp))
    {
        this.list.push([e.params[2], e.params[3], toUnicode(e.params[4])]);
    }
}

/* end of WHO */
CIRCNetwork.prototype.on315 =
function my_315 (e)
{
    var matches;
    if ("whoMatches" in this)
        matches = this.whoMatches;
    else
        matches = 0;
    e.user.display (getMsg("my_315", [e.params[2], matches]), e.code);
    delete this.whoMatches;
}

CIRCNetwork.prototype.on352 =
function my_352 (e)
{
    //0-352 1-rginda_ 2-#chatzilla 3-chatzilla 4-h-64-236-139-254.aoltw.net
    //5-irc.mozilla.org 6-rginda 7-H
    var desc;
    var hops = "?";
    var length = e.params.length;
    var ary = e.params[length - 1].match(/(\d+)\s(.*)/);
    if (ary)
    {
        hops = Number(ary[1]);
        desc = ary[2];
    }
    else
    {
        desc = e.params[length - 1];
    }
    
    var status = e.params[7];
    if (e.params[7] == "G")
        status = getMsg("my_352.g");
    else if (e.params[7] == "H")
        status = getMsg("my_352.h");
        
    e.user.display (getMsg("my_352", [e.params[6], e.params[3], e.params[4],
                                      desc, status, toUnicode(e.params[2]), 
                                      e.params[5], hops]), e.code, e.user);
    updateTitle (e.user);
    if ("whoMatches" in this)
        ++this.whoMatches;
    else
        this.whoMatches = 1;
}

CIRCNetwork.prototype.on311 = /* whois name */
CIRCNetwork.prototype.on319 = /* whois channels */
CIRCNetwork.prototype.on312 = /* whois server */
CIRCNetwork.prototype.on317 = /* whois idle time */
CIRCNetwork.prototype.on318 = /* whois end of whois*/
function my_whoisreply (e)
{
    var text = "egads!";
    var nick = e.params[2];
    
    switch (Number(e.code))
    {
        case 311:
            text = getMsg("my_whoisreplyMsg",
                          [nick, e.params[3], e.params[4],
                           toUnicode(e.params[6])]);
            break;
            
        case 319:
            var ary = stringTrim(e.params[3]).split(" ");
            text = getMsg("my_whoisreplyMsg2",[nick, arraySpeak(ary)]);
            break;
            
        case 312:
            text = getMsg("my_whoisreplyMsg3",
                          [nick, e.params[3], e.params[4]]);
            break;
            
        case 317:
            text = getMsg("my_whoisreplyMsg4",
                          [nick, formatDateOffset(Number(e.params[3])),
                          new Date(Number(e.params[4]) * 1000)]);
            break;
            
        case 318:
            text = getMsg("my_whoisreplyMsg5", nick);
            break;
            
    }

    if (nick in e.server.users && "messages" in e.server.users[nick])
    {
        var user = e.server.users[nick];
        updateTitle(user);
        user.display (text, e.code);
    }
    else
    {
        e.server.parent.display(text, e.code);
    }
}

CIRCNetwork.prototype.on341 = /* invite reply */
function my_341 (e)
{
    this.display (getMsg("my_341", [e.params[2], toUnicode(e.params[3])]), "341");
}

CIRCNetwork.prototype.onInvite = /* invite message */
function my_invite (e)
{
    this.display (getMsg("my_Invite", [e.user.properNick, e.user.name,
                                       e.user.host, e.params[2]]), "INVITE");
}

CIRCNetwork.prototype.on433 = /* nickname in use */
function my_433 (e)
{
    if (e.params[2] == this.INITIAL_NICK && this.connecting)
    {
        var newnick = this.INITIAL_NICK + "_";
        this.INITIAL_NICK = newnick;
        e.server.parent.display (getMsg("my_433Retry", [e.params[2], newnick]),
                                 "433");
        this.primServ.sendData("NICK " + newnick + "\n");
    }
    else
    {
        this.display (getMsg("my_433Msg", e.params[2]), "433");
    }
}

CIRCNetwork.prototype.onStartConnect =
function my_sconnect (e)
{
    this.display (getMsg("my_sconnect", [this.name, e.host, e.port,
                                         e.connectAttempt,
                                         this.MAX_CONNECT_ATTEMPTS]), "INFO");
}
    
CIRCNetwork.prototype.onError =
function my_neterror (e)
{
    var msg;
    
    if (typeof e.errorCode != "undefined")
    {
        switch (e.errorCode)
        {
            case JSIRC_ERR_NO_SOCKET:
                msg = getMsg ("my_neterrorNoSocket");
                break;
                
            case JSIRC_ERR_EXHAUSTED:
                msg = getMsg ("my_neterrorExhausted");
                break;
        }
    }
    else
        msg = e.params[e.params.length - 1];
    
    this.display (msg, "ERROR");   
}


CIRCNetwork.prototype.onDisconnect =
function my_netdisconnect (e)
{
    var msg;
    var reconnect = false;
    
    if (typeof e.disconnectStatus != "undefined")
    {
        switch (e.disconnectStatus)
        {
            case 0:
                msg = getMsg("my_netdisconnectConnectionClosed", [this.name,
                             e.server.hostname, e.server.port]);
                break;

            case NS_ERROR_CONNECTION_REFUSED:
                msg = getMsg("my_netdisconnectConnectionRefused", [this.name,
                             e.server.hostname, e.server.port]);
                break;

            case NS_ERROR_NET_TIMEOUT:
                msg = getMsg("my_netdisconnectConnectionTimeout", [this.name,
                             e.server.hostname, e.server.port]);
                break;

            case NS_ERROR_UNKNOWN_HOST:
                msg = getMsg("my_netdisconnectUnknownHost",
                             e.server.hostname);
                break;
            
            default:
                msg = getMsg("my_netdisconnectConnectionClosedStatus",
                             [this.name, e.server.hostname, e.server.port]);
                reconnect = true;
                break;
        }    
    }
    else
    {
        msg = getMsg("my_neterrorConnectionClosed", [this.name,
                     e.server.hostname, e.server.port]);
    }
    
    for (var v in client.viewsArray)
    {
        var obj = client.viewsArray[v].source;
        if (obj != client)
        {        
            var details = getObjectDetails(obj);
            if ("server" in details && details.server == e.server)
                obj.displayHere (msg, "ERROR");
        }
    }

    for (var c in this.primServ.channels)
    {
        var channel = this.primServ.channels[c];
        client.rdf.clearTargets(channel.getGraphResource(),
                                client.rdf.resChanUser);
    }
    
    this.connecting = false;
    updateTitle();
    if ("userClose" in client && client.userClose &&
        client.getConnectionCount() == 0)
        window.close();
}

CIRCNetwork.prototype.onCTCPReplyPing =
function my_replyping (e)
{
    var delay = formatDateOffset ((new Date() - new Date(Number(e.CTCPData))) /
                                  1000);
    display (getMsg("my_replyping", [e.user.properNick, delay]), "INFO",
             e.user, "ME!");
}

CIRCNetwork.prototype.onNick =
function my_cnick (e)
{
    if (userIsMe (e.user))
    {
        if (client.currentObject == this)
            this.displayHere (getMsg("my_cnickMsg", e.user.properNick),
                              "NICK", "ME!", e.user, this);
        updateNetwork();
        updateStalkExpression(this);
    }
    else
    {
        this.display (getMsg("my_cnickMsg2", [e.oldNick, e.user.properNick]),
                      "NICK", e.user, this);
    }
}

CIRCNetwork.prototype.onPing =
function my_netping (e)
{
    updateNetwork (this);
}

CIRCNetwork.prototype.onPong =
function my_netpong (e)
{

    updateNetwork (this);
    
}

CIRCChannel.prototype.onInit =
function chan_oninit ()
{
    var myBranch = this.parent.parent.name + "," + this.name;
    myBranch = myBranch.replace(/[^\w\d,]/g, encodeChar);
    this.localPrefs =
        client.prefService.getBranch(client.prefBranch.root + "viewList." +
                                     myBranch + ".");

    if (getBoolPref("logging", false, this.localPrefs))
       client.startLogging(this);
}

CIRCChannel.prototype.onPrivmsg =
function my_cprivmsg (e)
{
    
    this.display (e.params[2], "PRIVMSG", e.user, this);
    
    if ((typeof client.prefix == "string") &&
        e.params[2].indexOf (client.prefix) == 0)
    {
        try
        {
            var v = eval(e.params[2].substring (client.prefix.length,
                                                e.params[2].length));
        }
        catch (ex)
        {
            this.say (fromUnicode(e.user.nick + ": " + String(ex), this.charset));
            return false;
        }
        
        if (typeof (v) != "undefined")
        {
            if (v != null)                
                v = String(v);
            else
                v = "null";
            
            var rsp = getMsg("my_cprivmsgMsg", e.user.nick);
            
            if (v.indexOf ("\n") != -1)
                rsp += "\n";
            else
                rsp += " ";
            
            this.display (rsp + v, "PRIVMSG", e.server.me, this);
            this.say (fromUnicode(rsp + v, this.charset));
        }
    }

    return true;
    
}

/* end of names */
CIRCChannel.prototype.on366 =
function my_366 (e)
{
    if (client.currentObject == this)    
        /* hide the tree while we add (possibly tons) of nodes */
        client.rdf.setTreeRoot("user-list", client.rdf.resNullChan);
    
    client.rdf.clearTargets(this.getGraphResource(), client.rdf.resChanUser);

    for (var u in this.users)
    {
        this.users[u].updateGraphResource();
        this._addUserToGraph (this.users[u]);
    }
    
    if (client.currentObject == this)
        /* redisplay the tree */
        client.rdf.setTreeRoot("user-list", this.getGraphResource());
    
    if ("pendingNamesReply" in client.currentObject)
    {
        display (e.channel.unicodeName + ": " + e.params[3], "366");
        client.currentObject.pendingNamesReply = false;
    }
}    

CIRCChannel.prototype.onTopic = /* user changed topic */
CIRCChannel.prototype.on332 = /* TOPIC reply */
function my_topic (e)
{

    if (e.code == "TOPIC")
        this.display (getMsg("my_topicMsg", [this.topicBy, this.topic]),
                      "TOPIC");
    
    if (e.code == "332")
    {
        if (this.topic)
            this.display (getMsg("my_topicMsg2", [this.unicodeName, this.topic]),
                          "TOPIC");
        else
            this.display (getMsg("my_topicMsg3", this.unicodeName), "TOPIC");
    }
    
    updateChannel (this);
    updateTitle (this);
    
}

CIRCChannel.prototype.on333 = /* Topic setter information */
function my_topicinfo (e)
{
    
    this.display (getMsg("my_topicinfoMsg", [this.unicodeName, this.topicBy, 
                                             this.topicDate]), "TOPIC");
    
}

CIRCChannel.prototype.on353 = /* names reply */
function my_topic (e)
{
    if ("pendingNamesReply" in client.currentObject)
        display (e.channel.unicodeName + ": " + e.params[4], "NAMES");
}


CIRCChannel.prototype.onNotice =
function my_notice (e)
{
    this.display (toUnicode(e.params[2], this.charset),
                  "NOTICE", e.user, this);   
}

CIRCChannel.prototype.onCTCPAction =
function my_caction (e)
{

    this.display (e.CTCPData, "ACTION", e.user, this);

}

CIRCChannel.prototype.onUnknownCTCP =
function my_unkctcp (e)
{

    this.display (getMsg("my_unkctcpMsg", [e.CTCPCode, e.CTCPData,
                                           e.user.properNick]),
                  "BAD-CTCP", e.user, this);
    
}   

CIRCChannel.prototype.onJoin =
function my_cjoin (e)
{

    if (userIsMe (e.user))
    {
        this.display (getMsg("my_cjoinMsg", e.channel.unicodeName), "JOIN",
                      e.server.me, this);
        setCurrentObject(this);
    }
    else
    {
        this.display(getMsg("my_cjoinmsg2", [e.user.properNick, e.user.name,
                                             e.user.host,
                                             e.channel.unicodeName]),
                     "JOIN", e.user, this);
    }

    this._addUserToGraph (e.user);
    updateUserList()
    updateChannel (e.channel);
    
}

CIRCChannel.prototype.onPart =
function my_cpart (e)
{

    this._removeUserFromGraph(e.user);

    if (userIsMe (e.user))
    {
        this.display (getMsg("my_cpartMsg", e.channel.unicodeName), "PART",
                      e.user, this);
        if (client.currentObject == this)    
            /* hide the tree while we remove (possibly tons) of nodes */
            client.rdf.setTreeRoot("user-list", client.rdf.resNullChan);
        
        client.rdf.clearTargets(this.getGraphResource(),
                                client.rdf.resChanUser, true);

        if (client.currentObject == this)
            /* redisplay the tree */
            client.rdf.setTreeRoot("user-list", this.getGraphResource());

        if (client.DELETE_ON_PART)
            this.dispatch("delete");
    }
    else
        this.display (getMsg("my_cpartMsg2",
                             [e.user.properNick, e.channel.unicodeName]), 
                      "PART", e.user, this);

    updateChannel (e.channel);
    
}

CIRCChannel.prototype.onKick =
function my_ckick (e)
{

    if (userIsMe (e.lamer))
        this.display (getMsg("my_ckickMsg",
                             [e.channel.unicodeName, e.user.properNick, e.reason]),
                      "KICK", e.user, this);
    else
    {
        var enforcerProper, enforcerNick;
        if (userIsMe (e.user))
        {
            enforcerProper = "YOU";
            enforcerNick = "ME!";
        }
        else
        {
            enforcerProper = e.user.properNick;
            enforcerNick = e.user.nick;
        }
        
        this.display (getMsg("my_ckickMsg2",
                             [e.lamer.properNick, e.channel.unicodeName, 
                              enforcerProper, e.reason]), "KICK", e.user, this);
    }
    
    this._removeUserFromGraph(e.lamer);

    updateChannel (e.channel);
    
}

CIRCChannel.prototype.onChanMode =
function my_cmode (e)
{

    if ("user" in e)
    {
        var msg = toUnicode(e.params.slice(1).join(" "),
                            e.channel.charset);
        this.display (getMsg("my_cmodeMsg",
                             //[e.params.slice(1).join(" "),
                             [msg,
                              e.user.properNick]), "MODE", e.user, this);
    }

    for (var u in e.usersAffected)
        e.usersAffected[u].updateGraphResource();

    updateChannel (e.channel);
    updateTitle (e.channel);
    
}

    

CIRCChannel.prototype.onNick =
function my_cnick (e)
{

    if (userIsMe (e.user))
    {
        this.display (getMsg("my_cnickMsg", e.user.properNick), "NICK",
                      "ME!", e.user, this);
        updateNetwork();
    }
    else
        this.display (getMsg("my_cnickMsg2", e.oldNick, e.user.properNick),
                      "NICK", e.user, this);

    /*
      dd ("updating resource " + e.user.getGraphResource().Value +
        " to new nickname " + e.user.properNick);
    */

    e.user.updateGraphResource();
    updateUserList();
}

CIRCChannel.prototype.onQuit =
function my_cquit (e)
{

    if (userIsMe(e.user)) /* I dont think this can happen */
        this.display (getMsg("my_cquitMsg", [e.server.parent.name, e.reason]),
                      "QUIT", e.user, this);
    else
        this.display (getMsg("my_cquitMsg2", [e.user.properNick,
                                              e.server.parent.name, e.reason]),
                      "QUIT", e.user, this);

    this._removeUserFromGraph(e.user);

    updateChannel (e.channel);
    
}

CIRCUser.prototype.onInit =
function user_oninit ()
{
    var myBranch = this.parent.parent.name + "," + this.name;
    myBranch = myBranch.replace(/[^\w\d,]/g, encodeChar);
    this.localPrefs =
        client.prefService.getBranch(client.prefBranch.root + "viewList." +
                                     myBranch + ".");

    if (getBoolPref("logging", false, this.localPrefs))
       client.startLogging(this);
}

CIRCUser.prototype.onPrivmsg =
function my_cprivmsg (e)
{
    if ("messages" in this)
    {
        playSounds(client.QUERY_BEEP);
    }
    else
    {        
        playSounds(client.MSG_BEEP);
        if (client.NEW_TAB_LIMIT == 0 ||
            client.viewsArray.length < client.NEW_TAB_LIMIT)
        {
            var tab = openQueryTab (e.server, e.user.nick);
            if (client.FOCUS_NEW_TAB)
                setCurrentObject(tab);
        }    
    }
    this.display (e.params[2], "PRIVMSG", e.user, e.server.me);
}

CIRCUser.prototype.onNick =
function my_unick (e)
{

    if (userIsMe(e.user))
    {
        updateNetwork();
        updateTitle();
    }
    
}

CIRCUser.prototype.onNotice =
function my_notice (e)
{
    this.display (toUnicode(e.params[2], this.charset),
                  "NOTICE", this, e.server.me);   
}

CIRCUser.prototype.onCTCPAction =
function my_uaction (e)
{

    e.user.display (e.CTCPData, "ACTION", this, e.server.me);

}

CIRCUser.prototype.onUnknownCTCP =
function my_unkctcp (e)
{

    this.parent.parent.display (getMsg("my_unkctcpMsg",
                                       [e.CTCPCode, e.CTCPData,
                                       e.user.properNick]),
                                "BAD-CTCP", this, e.server.me);

}

