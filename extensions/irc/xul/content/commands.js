/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is JSIRC Test Client #3
 *
 * The Initial Developer of the Original Code is New Dimensions Consulting,
 * Inc. Portions created by New Dimensions Consulting, Inc. are
 * Copyright (C) 1999 New Dimenstions Consulting, Inc. All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Robert Ginda, rginda@ndcico.com, original author
 *  Chiaki Koufugata, chiaki@mozilla.gr.jp, UI i18n
 */

const CMD_CONSOLE    = 0x01;
const CMD_NEED_NET   = 0x02;
const CMD_NEED_CHAN  = 0x04;
const CMD_NO_HELP    = 0x08;

function initCommands()
{
    client.commandManager = new CommandManager(client.defaultBundle);
    
    var cmdary =
        [/* "real" commands */
         ["about",             cmdAbout,                           CMD_CONSOLE],
         ["attach",            cmdAttach,                          CMD_CONSOLE],
         ["away",              cmdAway,             CMD_NEED_NET | CMD_CONSOLE],
         ["cancel",            cmdCancel,           CMD_NEED_NET | CMD_CONSOLE],
         ["charset",           cmdCharset,                         CMD_CONSOLE],
         ["channel-charset",   cmdChannelCharset,  CMD_NEED_CHAN | CMD_CONSOLE],
         ["clear-view",        cmdClearView,                       CMD_CONSOLE],
         ["client",            cmdClient,                          CMD_CONSOLE],
         ["commands",          cmdCommands,                        CMD_CONSOLE],
         ["ctcp",              cmdCTCP,             CMD_NEED_NET | CMD_CONSOLE],
         ["css",               cmdCSS,                             CMD_CONSOLE],
         ["sync-css",          cmdSyncCSS,                                   0],
         ["delete-view",       cmdDeleteView,                      CMD_CONSOLE],
         ["deop",              cmdDeop,            CMD_NEED_CHAN | CMD_CONSOLE],
         ["devoice",           cmdDevoice,         CMD_NEED_CHAN | CMD_CONSOLE],
         ["disconnect",        cmdQuit,             CMD_NEED_NET | CMD_CONSOLE],
         ["echo",              cmdEcho,                            CMD_CONSOLE],
         ["eval",              cmdEval,                            CMD_CONSOLE],
         ["exit",              cmdExit,             CMD_NEED_NET | CMD_CONSOLE],
         ["help",              cmdHelp,                            CMD_CONSOLE],
         ["hide-view",         cmdHideView,                        CMD_CONSOLE],
         ["invite",            cmdInvite,           CMD_NEED_NET | CMD_CONSOLE],
         ["join",              cmdJoin,             CMD_NEED_NET | CMD_CONSOLE],
         ["join-charset",      cmdJoin,             CMD_NEED_NET | CMD_CONSOLE],
         ["kick",              cmdKick,            CMD_NEED_CHAN | CMD_CONSOLE],
         ["leave",             cmdLeave,           CMD_NEED_CHAN | CMD_CONSOLE],
         ["list",              cmdList,             CMD_NEED_NET | CMD_CONSOLE],
         ["log",               cmdLog,                             CMD_CONSOLE],
         ["me",                cmdMe,              CMD_NEED_CHAN | CMD_CONSOLE],
         ["msg",               cmdMsg,              CMD_NEED_NET | CMD_CONSOLE],
         ["names",             cmdNames,            CMD_NEED_NET | CMD_CONSOLE],
         ["network",           cmdNetwork,                         CMD_CONSOLE],
         ["networks",          cmdNetworks,                        CMD_CONSOLE],
         ["nick",              cmdNick,             CMD_NEED_NET | CMD_CONSOLE],
         ["notify",            cmdNotify,           CMD_NEED_NET | CMD_CONSOLE],
         ["op",                cmdOp,              CMD_NEED_CHAN | CMD_CONSOLE],
         ["ping",              cmdPing,             CMD_NEED_NET | CMD_CONSOLE],
         ["query",             cmdQuery,            CMD_NEED_NET | CMD_CONSOLE],
         ["quit",              cmdExit,                            CMD_CONSOLE],
         ["quote",             cmdQuote,            CMD_NEED_NET | CMD_CONSOLE],
         ["rlist",             cmdRlist,            CMD_NEED_NET | CMD_CONSOLE],
         ["server",            cmdServer,                          CMD_CONSOLE],
         ["squery",            cmdSquery,           CMD_NEED_NET | CMD_CONSOLE],
         ["stalk",             cmdStalk,                           CMD_CONSOLE],
         ["testdisplay",       cmdTestDisplay,                     CMD_CONSOLE],
         ["topic",             cmdTopic,           CMD_NEED_CHAN | CMD_CONSOLE],
         ["toggle-ui",         cmdToggleUI,        CMD_NEED_CHAN | CMD_CONSOLE],
         ["unstalk",           cmdUnstalk,                         CMD_CONSOLE],
         ["version",           cmdVersion,                         CMD_CONSOLE],
         ["voice",             cmdVoice,           CMD_NEED_CHAN | CMD_CONSOLE],
         ["who",               cmdSimpleCommand,    CMD_NEED_NET | CMD_CONSOLE],
         ["whois",             cmdWhoIs,            CMD_NEED_NET | CMD_CONSOLE],
         ["whowas",            cmdSimpleCommand,    CMD_NEED_NET | CMD_CONSOLE],

         /* aliases */
         ["desc",              "pref desc",                        CMD_CONSOLE],
         ["name",              "pref name",                        CMD_CONSOLE],
         ["part",              "leave",                            CMD_CONSOLE],
         ["j",                 "join",                             CMD_CONSOLE],
         ["userlist",          "toggle-ui userlist",               CMD_CONSOLE],
         ["tabstrip",          "toggle-ui tabstrip",               CMD_CONSOLE],
         ["statusbar",         "toggle-ui status",                 CMD_CONSOLE],
         ["header",            "toggle-ui header",                 CMD_CONSOLE]

        ];

    cmdary.stringBundle = client.defaultBundle;
    client.commandManager.defineCommands(cmdary);

    client.commandManager.argTypes.__aliasTypes__ (["reason", "action", "text",
                                                    "message", "reason",
                                                    "expression"],
                                                   "rest");
}

function getToggle (toggle, currentState)
{
    if (toggle == "toggle")
        toggle = !currentState;

    return toggle;
}

function cmdCancel(e)
{
    var network = e.context.network;
    
    if (!network.connecting)
    {
        display(MSG_NOTHING_TO_CANCEL, MT_ERROR);
        return;
    }
    
    network.connectAttempt = network.MAX_CONNECT_ATTEMPTS + 1;

    display(getMsg(MSG_CANCELLING, network.name));
}    

function cmdCharset(e)
{
    if (e.charset)
    {
        if (!setCharset(e.charset))
        {
            display(getMsg(MSG_ERR_INVALID_CHARSET, e.charset), MT_ERROR);
            return;
        }
    }

    display(getMsg(MSG_CURRENT_CHARSET, client.CHARSET), MT_INFO);
}

function cmdChannelCharset(e)
{
    if (e.charset)
    {
        if(!checkCharset(e.charset))
        {
            display(getMsg(MSG_ERR_INVALID_CHARSET, e.charset), MT_ERROR);
            return;
        }
        e.context.channel.charset = e.charset;
    }

    display (getMsg(MSG_CURRENT_CHARSET, e.conect.channel.charset),
             MT_INFO);
}

function cmdCSS(e)
{
    if (e.motif)
    {
        if (e.motif.search(/^light$/i) != -1)
            e.motif = "chrome://chatzilla/skin/output-light.css";
        else if (e.motif.search(/^dark$/i) != -1)
            e.motif = "chrome://chatzilla/skin/output-dark.css";
        else if (e.motif.search(/^default$/i) != -1)
            e.motif = "chrome://chatzilla/skin/output-default.css";
        else if (e.motif.search(/^none$/i) != -1)
            e.motif = "chrome://chatzilla/content/output-base.css";
    
        client.prefs["style.default"] = e.motif;
    }
    
    display (getMsg(MSG_CURRENT_CSS, client.prefs["style.default"]), MT_INFO);
}

function cmdSyncCSS(e)
{
    var motif = client.prefs["style.default"];
    var uri = client.prefs["outputWindowURL"].replace("%s", motif);

    for (var i = 0; i < client.deck.childNodes.length; i++)
        client.deck.childNodes[i].loadURI(uri);
}

function cmdSimpleCommand(e)
{    
    e.context.server.sendData(e.command.name + " " + e.inputData + "\n");
}

function cmdSquery(e)
{
    var data;
    
    if (e.commands)
        data = "SQUERY " + e.service + " :" + e.commands + "\n";
    else
        data = "SQUERY " + e.service + "\n";

    e.context.server.sendData(data);
}

function cmdStatus(e)
{    
    function serverStatus (s)
    {
        if (!s.connection.isConnected)
        {
            display(getMsg(MSG_NOT_CONNECTED, s.parent.name), MT_STATUS);
            return;
        }
        
        var serverType = (s.parent.primServ == s) ? MSG_PRIMARY : MSG_SECONDARY;
        display(getMsg(MSG_CONNECTION_INFO,
                       [s.parent.name, s.me.properNick, s.connection.host,
                        s.connection.port, serverType]),
                MT_STATUS);

        var connectTime = Math.floor((new Date() - s.connection.connectDate) /
                                     1000);
        connectTime = formatDateOffset(connectTime);
        
        var pingTime = ("lastPing" in s) ?
            formatDateOffset(Math.floor((new Date() - s.lastPing) / 1000)) :
            MSG_NA;
        var lag = (s.lag >= 0) ? s.lag : MSG_NA;

        display(getMsg(MSG_SERVER_INFO,
                       [s.parent.name, connectTime, pingTime, lag]),
                MT_STATUS);
    }

    function channelStatus (c)
    {
        var cu;
        var net = c.parent.parent.name;

        if ((cu = c.users[c.parent.me.nick]))
        {
            var mtype;
            
            if (cu.isOp && cu.isVoice)
                mtype = MSG_VOICEOP;
            else if (cu.isOp)
                mtype = MSG_OPERATOR;
            else if (cu.isVoice)
                mtype = MSG_VOICED;
            else
                mtype = MSG_MEMBER;

            var mode = c.mode.getModeStr();
            if (!mode)
                mode = MSG_NO_MODE;
            
            display(getMsg(MSG_CHANNEL_INFO,
                           [net, mtype, c.unicodeName, mode,
                            "irc://" + escape(net) + "/" + 
                            escape(c.encodedName) + "/"]),
                    MT_STATUS);
            display(getMsg(MSG_CHANNEL_DETAIL,
                           [net, c.unicodeName, c.getUsersLength(),
                            c.opCount, c.voiceCount]),
                    MT_STATUS);

            if (c.topic)
            {
                display(getMsg(MSG_TOPIC_INFO, [net, c.unicodeName, c.topic]),
                         MT_STATUS);
            }
            else
            {
                display(getMsg(MSG_NOTOPIC_INFO, [net, c.unicodeName]),
                        MT_STATUS);
            }
        }
        else
        {
            display(getMsg(MSG_NONMEMBER, [net, c.unicodeName]), MT_STATUS);
        }
    };

    display(client.userAgent, MT_STATUS);
    display(getMsg(MSG_USER_INFO,
                   [client.prefs["nickname"], client.prefs["username"],
                    client.prefs["desc"]]),
            MT_STATUS);
        
    var n, s, c;

    if (e.context.channel)
    {
        serverStatus(e.context.server);
        channelStatus(e.context.channel);
    }
    else if (e.context.network)
    {
        for (s in e.context.network.servers)
        {
            serverStatus(e.context.network.servers[s]);
            for (c in e.context.network.servers[s].channels)
                channelStatus (e.context.network.servers[s].channels[c]);
        }
    }
    else
    {
        for (n in client.networks)
        {
            for (s in client.networks[n].servers)
            {
                var server = client.networks[n].servers[s]
                    serverStatus(server);
                for (c in server.channels)
                    channelStatus(server.channels[c]);
            }
        }
    }

    display(MSG_END_STATUS, MT_STATUS);
}

function cmdHelp (e)
{
    var ary;
    ary = client.commandManager.list (e.pattern, CMD_CONSOLE);
    
    if (ary.length == 0)
    {
        display (getMsg(MSG_ERR_NO_COMMAND, e.pattern), MT_ERROR);
        return false;
    }

    for (var i in ary)
    {        
        display (getMsg(MSG_FMT_USAGE, [ary[i].name, ary[i].usage]), MT_USAGE);
        display (ary[i].help, MT_HELP);
    }

    return true;
}

function cmdTestDisplay(e)
{
    display(MSG_TEST_HELLO, MT_HELLO);
    display(MSG_TEST_INFO, MT_INFO);
    display(MSG_TEST_ERROR, MT_ERROR);
    display(MSG_TEST_HELP, MT_HELP);
    display(MSG_TEST_USAGE, MT_USAGE);
    display(MSG_TEST_STATUS, MT_STATUS);

    if (e.context.server && e.context.server.me)
    {
        var me = e.context.server.me;
        var sampleUser = {TYPE: "IRCUser", nick: "ircmonkey",
                          name: "IRCMonkey", properNick: "IRCMonkey",
                          host: ""};
        var sampleChannel = {TYPE: "IRCChannel", name: "#mojo"};

        function test (from, to)
        {
            var fromText = (from != me) ? from.TYPE + " ``" + from.name + "''" :
                MSG_YOU;
            var toText   = (to != me) ? to.TYPE + " ``" + to.name + "''" :
                MSG_YOU;
            
            display (getMsg(MSG_TEST_PRIVMSG, [fromText, toText]),
                     "PRIVMSG", from, to);
            display (getMsg(MSG_TEST_ACTION, [fromText, toText]),
                     "ACTION", from, to);
            display (getMsg(MSG_TEST_NOTICE, [fromText, toText]),
                     "NOTICE", from, to);
        }
        
        test (sampleUser, me); /* from user to me */
        test (me, sampleUser); /* me to user */

        display(MSG_TEST_URL, "PRIVMSG", sampleUser, me);
        display(MSG_TEST_STYLES, "PRIVMSG", sampleUser, me);
        display(MSG_TEST_EMOTICON, "PRIVMSG", sampleUser, me);
        display(MSG_TEST_RHEET, "PRIVMSG", sampleUser, me);
        display(unescape(MSG_TEST_CTLCHR), "PRIVMSG", sampleUser, me);
        display(unescape(MSG_TEST_COLOR), "PRIVMSG", sampleUser, me);
        display(MSG_TEST_QUOTE, "PRIVMSG", sampleUser, me);
        

        if (e.context.channel)
        {
            test (sampleUser, sampleChannel); /* user to channel */
            test (me, sampleChannel);         /* me to channel */
            display(MSG_TEST_TOPIC, "TOPIC", sampleUser, sampleChannel);
            display(MSG_TEST_JOIN, "JOIN", sampleUser, sampleChannel);
            display(MSG_TEST_PART, "PART", sampleUser, sampleChannel);
            display(MSG_TEST_KICK, "KICK", sampleUser, sampleChannel);
            display(MSG_TEST_QUIT, "QUIT", sampleUser, sampleChannel);
            display(getMsg(MSG_TEST_STALK, me.nick),
                    "PRIVMSG", sampleUser, sampleChannel);
            display(MSG_TEST_STYLES, "PRIVMSG", me, sampleChannel);
        }
    }
}

function cmdNetwork(e)
{
    if (!(e.networkName in client.networks))
    {
        display (getMsg(MSG_ERR_UNKNOWN_NETWORK, e.networkName), MT_ERROR);
        return;
    }
    
    setCurrentObject(client.networks[e.networkName]);
}

function cmdNetworks(e)
{
    const ns = "http://www.w3.org/1999/xhtml";
    
    var span = document.createElementNS(ns, "html:span");
    
    span.appendChild(newInlineText(MSG_NETWORKS_HEADA));

    var netnames = keys(client.networks).sort();
    var lastname = netnames[netnames.length - 1];
    
    for (n in netnames)
    {
        var net = client.networks[netnames[n]];
        var a = document.createElementNS(ns, "html:a");
        a.setAttribute("class", "chatzilla-link");
        a.setAttribute("href", "irc://" + net.name);
        var t = newInlineText(net.name);
        a.appendChild(t);
        span.appendChild(a);
        if (netnames[n] != lastname)
            span.appendChild(newInlineText (MSG_COMMASP));
    }

    span.appendChild(newInlineText(MSG_NETWORKS_HEADB));

    display(span, MT_INFO);
}   

function cmdServer(e)
{
    if (!e.port)
        e.port = 6667;

    e.hostname = e.hostname.toLowerCase();
    
    if (e.hostname in client.networks)
    {
        var net = client.networks[e.hostname];
        
        if (net.isConnected())
        {
            display (getMsg(MSG_ALREADY_CONNECTED, e.hostname), MT_ERROR);
            return;
        }
        else
        {
            /* if we've already created a network for this server, reinit the
             * serverList, in case the user changed the port or password */
            net.serverList = [{name: e.hostname, port: e.port,
                               password: e.password}];
        }
    }
    else
    {
        /* if there wasn't already a network created for this server,
         * make one. */
        client.addNetwork(e.hostname, [{name: e.hostname, port: e.port,
                                        password: e.password}])
    }

    client.connectToNetwork(e.hostname);
}

function cmdQuit(e)
{
    e.context.network.quit(fromUnicode(e.reason));
}

function cmdExit(e)
{
    client.quit(fromUnicode(e.reason));
    window.close();
}

function cmdDeleteView(e)
{
    if (!e.view)
        e.view = e.context.sourceObject;
    
    if (e.view.TYPE == "IRCChannel" && e.view.active)
        e.view.part();

    if (client.viewsArray.length < 2)
    {
        display(MSG_ERR_LAST_VIEW, MT_ERROR);
        return;
    }
    
    var tb = getTabForObject(e.view);
    if (tb)
    {
        var i = deleteTab (tb);
        if (i != -1)
        {
            delete e.view.messageCount;
            delete e.view.messages;
            client.deck.removeChild(e.view.frame);
            delete e.view.frame;

            if (i >= client.viewsArray.length)
                i = client.viewsArray.length - 1;            
            setCurrentObject(client.viewsArray[i].source);
        }
    }
}

function cmdHideView(e)
{
    if (!e.view)
        e.view = e.context.sourceObject;

    var tb = getTabForObject(e.view);
    
    if (tb)
    {
        var i = deleteTab (tb);
        if (i != -1)
        {
            client.deck.removeChild(view.frame);
            delete view.frame;
            if (i >= client.viewsArray.length)
                i = client.viewsArray.length - 1;
            
            setCurrentObject (client.viewsArray[i].source);
        }
    }
}

function cmdClearView(e)
{
    if (!e.view)
        e.view = e.context.sourceObject;

    e.view.messages = null;
    e.view.messageCount = 0;

    e.view.displayHere(MSG_MESSAGES_CLEARED);

    e.view.frame.reload();
}

function cmdNames(e)
{
    var name;
    
    if (e.channelName)
    {
        var encodedName = fromUnicode(e.channel + " ",
                                      e.context.charset);
        encodedName = encodedName.substr(0, encodedName.length - 1);
        name = encodedName;
    }
    else
    {
        if (!e.context.channel)
        {
            display(getMsg(MSG_ERR_REQUIRED_PARAM, "channel-name"), MT_ERROR);
            return;
        }
        name = e.context.channel.encodedName;
    }
    
    e.context.channel.pendingNamesReply = true;
    e.context.server.sendData ("NAMES " + name + "\n");
}

function cmdToggleUI(e)
{
    var menu = document.getElementById("menu-view-" + e.thing);
    var ids = new Array();
    
    switch (e.thing)
    {
        case "tabstrip":
            ids = ["view-tabs"];
            break;
            
        case "userlist":
            ids = ["main-splitter", "user-list-box"];            
            break;
            
        case "header":
            ids = ["header-bar-tbox"];
            break;
            
        case "status":
            ids = ["status-bar"];
            break;

        default:
            ASSERT (0,"Unknown element ``" + menuId + 
                    "'' passed to onToggleVisibility.");
            return;
    }


    var newState;
    var elem = document.getElementById(ids[0]);
    var d = elem.getAttribute("collapsed");
    
    var sourceObject = e.context.sourceObject;

    if (d == "true")
    {
        if (e.thing == "userlist")
        {
            if (sourceObject.TYPE == "IRCChannel")
            {
                client.rdf.setTreeRoot("user-list", 
                                       sourceObject.getGraphResource());
            }
            else
            {
                client.rdf.setTreeRoot("user-list", client.rdf.resNullChan);
            }
        }
        
        newState = "false";
        menu.setAttribute ("checked", "true");
        client.uiState[e.thing] = true;
    }
    else
    {
        newState = "true";
        menu.setAttribute ("checked", "false");
        client.uiState[e.thing] = false;
    }
    
    for (var i in ids)
    {
        elem = document.getElementById(ids[i]);
        elem.setAttribute ("collapsed", newState);
    }

    updateTitle();
    focusInput();
}

function cmdCommands(e)
{
    display(MSG_COMMANDS_HEADER);
    
    var matchResult = client.commandManager.listNames(e.pattern, CMD_CONSOLE);
    matchResult = matchResult.join(MSG_COMMASP);
    
    if (e.pattern)
        display(getMsg(MSG_MATCHING_COMMANDS, [e.pattern, matchResult]));
    else
        display(getMsg(MSG_ALL_COMMANDS, matchResult));
}

function cmdAttach(e)
{
    if (e.ircUrl.search(/irc:\/\//i) != 0)
        e.ircUrl = "irc://" + e.ircUrl;
    
    var parsedURL = parseIRCURL(e.ircUrl);
    if (!parsedURL)
    {
        display(getMsg(MSG_ERR_BAD_IRCURL, e.ircUrl), MT_ERROR);
        return;
    }
    
    gotoIRCURL(e.ircUrl);
}
    
function cmdMe(e)
{
    var sourceObject = e.context.sourceObject;
    if (!("act" in sourceObject))
    {
        display(getMsg(MSG_ERR_IMPROPER_VIEW, "me"), MT_ERROR);
        return;
    }

    e.action = filterOutput (e.action, "ACTION", "ME!");
    display (e.action, "ACTION", "ME!", sourceObject);
    sourceObject.act (fromUnicode(e.action));
}

function cmdList(e)
{
    e.context.network.list = new Array();
    e.context.network.list.regexp = null;
    cmdSimpleCommand(e);
}

function cmdRlist(e)
{
    e.context.network.list = new Array();
    e.context.network.list.regexp = new RegExp(e.regexp, "i");
    e.context.server.sendData ("list\n");
}

function cmdQuery(e)
{
    var user = openQueryTab(e.context.server, e.nickname);
    setCurrentObject(user);

    if (e.msg)
    {
        e.msg = filterOutput(e.msg, "PRIVMSG", "ME!");
        user.display(e.msg, "PRIVMSG", "ME!", tab);
        user.say(fromUnicode(e.msg));
    }

    return user;
}

function cmdMsg(e)
{
    var usr = e.server.addUser(e.nickname);

    var msg = filterOutput(e.message, "PRIVMSG", "ME!");
    usr.display(e.message, "PRIVMSG", "ME!", usr);
    usr.say(fromUnicode(e.message, e.context.charset));
}

function cmdNick(e)
{
    if (e.context.server) 
        e.context.server.sendData ("NICK " + e.nickname + "\n");

    if (e.context.network)
        e.context.network.prefs["nickname"] = e.nickname;
    else
        client.prefs["nickname"] = e.nickname;
}

function cmdQuote(e)
{
    e.context.server.sendData(fromUnicode(e.ircCommand) + "\n");
}

function cmdEval(e)
{    
    var sourceObject = e.context.sourceObject;
    
    try
    {
        sourceObject.doEval = function (__s) { return eval(__s); }
        sourceObject.display(e.expression, MT_EVALIN);
        var rv = String(sourceObject.doEval (e.expression));
        sourceObject.display (rv, MT_EVALOUT);

    }
    catch (ex)
    {
        sourceObject.display (String(ex), MT_ERROR);
    }
}

function cmdCTCP(e)
{
    e.context.server.ctcpTo (e.target, e.code, e.params);
}

function cmdJoin(e)
{
    if ("charset" in e && e.charset && !checkCharset(e.charset))
    {
        display (getMsg(MSG_ERR_INVALID_CHARSET, e.charset), MT_ERROR);
        return null;
    }

    if (!e.channelName)
    {
        var channel = e.context.channel;
        if (!channel)
        {
            display(getMsg(MSG_ERR_REQUIRED_PARAM, "channel"), MT_ERROR);
            return null;
        }
        
        e.channelName = channel.unicodeName;
        if (channel.mode.key)
            e.key = channel.mode.key
    }    

    if (e.channelName[0].search(/[#&+!]/) != 0)
        e.channelName = "#" + e.channelName;

    e.channel = e.context.server.addChannel(e.channelName, e.charset);
    e.channel.join(e.key);
    
    if (!("messages" in e.channel))
    {
        e.channel.displayHere(getMsg(MSG_CHANNEL_OPENED, 
                                     e.channel.unicodeName),
                              MT_INFO);
    }

    setCurrentObject(e.channel);

    return e.channel;
}

function cmdLeave(e)
{
    if (!e.context.server)
    {
        display(MSG_ERR_IMPROPER_VIEW, MT_ERROR);
        return;
    }

    if (e.channelName)
    {
        e.channelName = fromUnicode(e.channelName);
    }
    else
    {
        if (!e.context.channel)
        {
            display(MSG_ERR_IMPROPER_VIEW, MT_ERROR);
            return;
        }

        e.channelName = e.context.channel.encodedName;
    }

    e.context.server.sendData("PART " + e.channelName + "\n");
}

function cmdWhoIs (e) 
{
    e.context.server.whois(e.nickname);
}

function cmdTopic(e)
{
    if (!e.newTopic)
        e.context.server.sendData ("TOPIC " + e.context.channel.name + "\n");
    else
        e.context.channel.setTopic(fromUnicode(e.newTopic, e.context.charset));
}

function cmdAbout(e)
{
    display(e.context.server.VERSION_RPLY);
    display(MSG_HOMEPAGE);
}

function cmdAway(e)
{ 
    if (e.reason)
    {
        /* going away */
        if (client.prefs["awayNick"])
        {
            dispatch("nick", { nickname: client.prefs["awayNick"],
                               context: e.context });
        }
    
        e.context.server.sendData ("AWAY :" + fromUnicode(e.reason) + "\n");
    }
    else
    {
        /* returning */
        if (client.prefs["awayNick"])
        {
            dispatch("nick", { nickname: client.prefs["nickname"],
                               context: e.context });
        }
    
        e.context.server.sendData ("AWAY\n");
    }
}    

function cmdDeop(e)
{
    for (var i = 0; i < e.nicknameList.length; ++i)
    {
        var cuser = e.context.channel.getUser(e.nickname);
        cuser.setOp(false);
    }
}


function cmdOp(e)
{
    for (var i = 0; i < e.nicknameList.length; ++i)
    {
        var cuser = e.context.channel.getUser(e.nickname);
        cuser.setOp(true);
    }
}

function cmdPing (e) 
{
    dispatch("ctcp", { target: e.target, code: "PING", context: e.context });
}

function cmdVersion(e)
{
    dispatch("ctcp", { target: e.nickname, code: "VERSION",
                       context: e.context });
}

function cmdVoice(e)
{
    for (var i = 0; i < e.nicknameList.length; ++i)
    {
        var cuser = e.context.channel.getUser(e.nickname);
        cuser.setVoice(true);
    }
}

function cmdDevoice(e) 
{
    for (var i = 0; i < e.nicknameList.length; ++i)
    {
        var cuser = e.context.channel.getUser(e.nickname);
        cuser.setVoice(false);
    }
}

function cmdEcho(e)
{
    display(e.message);
}

function cmdInvite(e) 
{
    var channel;
    
    if (!e.channelName)
    {
        channel = e.context.channel;
    }
    else
    {
        var encodeName = fromUnicode(e.channelName.toLowerCase() + " ");
        encodeName = encodeName.substr(0, encodeName.length -1);
        channel = e.context.server.channels[encodeName];
             
        if (!channel) 
        {
            display(getMsg(MSG_ERR_UNKNOWN_CHANNEL, e.channelName), MT_ERROR);
            return;
        }
    }

    channel.invite(e.nickname);
}

function cmdKick(e) 
{
    var cuser = e.context.channel.getUser(e.nickname);

    if (!cuser)
    {
        display(getMsg(MSG_ERR_UNKNOWN_USER, e.nickname), MT_ERROR);
        return;
    }
    
    cuser.kick(e.reason);
}

function cmdClient(e)
{
    if (!client.messages)
        client.display(MSG_CLIENT_OPENED);
    
    getTabForObject (client, true);
    setCurrentObject (client);
}

function cmdNotify(e)
{
    var net = e.context.network;
    
    if (!e.nickname)
    {
        if ("notifyList" in net && net.notifyList.length > 0)
        {
            /* delete the lists and force a ISON check, this will
             * print the current online/offline status when the server
             * responds */
            delete net.onList;
            delete net.offList;
            onNotifyTimeout();
        }
        else
        {
            display(MSG_NO_NOTIFY_LIST);
        }
    }
    else
    {
        var adds = new Array();
        var subs = new Array();
        
        if (!("notifyList" in net))
            net.notifyList = new Array();
        for (var i in e.nicknameList)
        {
            var nickname = e.nicknameList[i];
            var idx = arrayIndexOf (net.notifyList, nickname);
            if (idx == -1)
            {
                net.notifyList.push (nickname);
                adds.push(nickname);
            }
            else
            {
                arrayRemoveAt (net.notifyList, idx);
                subs.push(nickname);
            }
        }

        var msgname;
        
        if (adds.length > 0)
        {
            msgname = (adds.length == 1) ? MSG_NOTIFY_ADDONE :
                                           MSG_NOTIFY_ADDSOME;
            display(getMsg(msgname, arraySpeak(adds)));
        }
        
        if (subs.length > 0)
        {
            msgname = (subs.length == 1) ? MSG_NOTIFY_DELONE : 
                                           MSG_NOTIFY_DELSOME;
            display(getMsg(msgname, arraySpeak(subs)));
        }
            
        delete net.onList;
        delete net.offList;
        onNotifyTimeout();
    }
}

                
function cmdStalk(e)
{
    if (!e.text)
    {
        if (list.length == 0)
            display(MSG_NO_STALK_LIST);
        else
            display(getMsg(MSG_STALK_LIST, list.join(MSG_COMMASP)));
        return;
    }

    client.prefs["stalkWords"].push(e.text);
    client.prefs["stalkWords"].update();
    
    display(getMsg(MSG_STALK_ADD, e.text));
}

function cmdUnstalk(e)
{
    e.text = e.text.toLowerCase();
    var list = client.prefs["stalkWords"];
    
    for (i in list)
    {
        if (list[i].toLowerCase() == e.text)
        {        
            list.splice(i, 1);
            list.update();
            display(getMsg(MSG_STALK_DEL, e.text));
            return;
        }
    }

    display(getMsg(MSG_ERR_UNKNOWN_STALK, e.text), MT_ERROR);
}

function cmdLog(e)
{
    var view = e.context.sourceObject;

    if (e.state == null)
    {
        if (!view.logging)
            view.displayHere(MSG_LOGGING_OFF);
        else
            view.displayHere(getMsg(MSG_LOGGING_ON, view.logFile.path));
        return;
    }

    if (e.state)
        client.startLogging(view);
    else
        client.stopLogging(view);
}
