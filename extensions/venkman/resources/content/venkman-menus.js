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
 * The Original Code is The JavaScript Debugger
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

/*
 * The _C(), _M(), __m(), and __t() functions are defined below.  They are
 * Venkman specific wrappers around calls the the CommandManager.
 *
 * _C(id, name)
 *  Creates a new context menu attached to the object with the id |id|.
 *  The label will be derived from the string named "mnu." + |name|.
 *
 * _M(parent, name)
 *  Creates a new menu dropdown in an existing menu bar with the id |parent|.
 *  The label will be derived from the string named "mnu." + |name|.
 *
 * __m(commandName)
 *  Creates a menuitem for the command named commandName.
 *
 * __t(parent, commandName)
 *  Creates a toolbaritem for the command named |commandName| in the toolbar
 *  with the id |parent|.
 */

function initMainMenus()
{
    
    /* main toolbar */
    __t("maintoolbar", "stop");
    __t("maintoolbar", "-");
    __t("maintoolbar", "cont");
    __t("maintoolbar", "next");
    __t("maintoolbar", "step");
    __t("maintoolbar", "finish");
    __t("maintoolbar", "-");
    __t("maintoolbar", "profile-tb");
    __t("maintoolbar", "toggle-pprint");


    _M("mainmenu", "file");
    __m("open-url");
    __m("find-file");
    __m("-");
    __m("close");
    __m("save-source");
    __m("save-profile");
    __m("-");
    __m("quit");
    
    /* View menu */
    function isVisible (view)
    {
        return "'currentContent' in console.views." + view;
    };

    _M("mainmenu", "view");
    _M("mainmenu:view", "toggle-views");
    __m("toggle-breaks",  {type: "checkbox", checkedif: isVisible("breaks")});
    __m("toggle-stack",   {type: "checkbox", checkedif: isVisible("stack")});
    __m("toggle-locals",  {type: "checkbox", checkedif: isVisible("locals")});
    __m("toggle-scripts", {type: "checkbox", checkedif: isVisible("scripts")});
    __m("toggle-windows", {type: "checkbox", checkedif: isVisible("windows")});
    __m("toggle-source",  {type: "checkbox", checkedif: isVisible("source")});
    __m("toggle-watch",   {type: "checkbox", checkedif: isVisible("watches")});
    console.lastMenu = "mainmenu:view";
    __m("-");
    __m("reload");
    __m("toggle-pprint", {type: "checkbox",
                          checkedif: "console.prefs['prettyprint']"});
    __m("-");
    __m("toggle-chrome", {type: "checkbox",
                          checkedif: "console.enableChromeFilter"});

    
     
    /* Debug menu */
    _M("mainmenu", "debug");
    __m("stop", {type: "checkbox",
                 checkedif: "console.jsds.interruptHook"});
    __m("cont");
    __m("next");
    __m("step");
    __m("finish");
    __m("-");
    __m("em-ignore", {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_IGNORE"});
    __m("em-trace",  {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_TRACE"});
    __m("em-break",  {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_BREAK"});
    __m("-");
    __m("tm-ignore",  {type: "radio", name: "tm",
                       checkedif: "console.throwMode == TMODE_IGNORE"});
    __m("tm-trace",   {type: "radio", name: "tm",
                       checkedif: "console.throwMode == TMODE_TRACE"});
    __m("tm-break",   {type: "radio", name: "tm",
                       checkedif: "console.throwMode == TMODE_BREAK"});
    __m("-");
    __m("toggle-ias", {type: "checkbox",
                       checkedif: "console.jsds.initAtStartup"});

    _M("mainmenu", "profile");
    __m("toggle-profile", {type: "checkbox",
                           checkedif:
                            "console.jsds.flags & COLLECT_PROFILE_DATA"});
    __m("clear-profile");
    __m("save-profile");
}

function initViewMenus()
{
    /* Context menu for console view */
    _C("output-iframe", "console");
    __m("stop", {type: "checkbox",
                 checkedif: "console.jsds.interruptHook"});
    __m("cont");
    __m("next");
    __m("step");
    __m("finish");
    __m("-");
    __m("em-ignore", {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_IGNORE"});
    __m("em-trace",  {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_TRACE"});
    __m("em-break",  {type: "radio", name: "em",
                      checkedif: "console.errorMode == EMODE_BREAK"});
    __m("-");
    __m("tm-ignore", {type: "radio", name: "tm",
                      checkedif: "console.throwMode == TMODE_IGNORE"});
    __m("tm-trace",  {type: "radio", name: "tm",
                      checkedif: "console.throwMode == TMODE_TRACE"});
    __m("tm-break",  {type: "radio", name: "tm",
                      checkedif: "console.throwMode == TMODE_BREAK"});
     
    /* Context menu for project view */
    _C("project-tree", "project");
    __m("find-url");
    __m("-");
    __m("clear-all", {enabledif:
                      "cx.target instanceof BPRecord || " +
                      "(has('breakpointLabel') && cx.target.childData.length)"});
    __m("clear");
    __m("-");
    __m("save-profile", {enabledif: "has('url')"});

    /* Context menu for source view */
    _C("source-tree", "source");
    __m("save-source");
    __m("-");
    __m("break",  {enabledif: "cx.lineIsExecutable && !has('breakpointRec')"});
    __m("fbreak", {enabledif: "!cx.lineIsExecutable && !has('breakpointRec')"});
    __m("clear");
    __m("-");
    __m("cont");
    __m("next");
    __m("step");
    __m("finish");
    __m("-");
    __m("toggle-pprint", {type: "checkbox",
                          checkedif: "console.prefs['prettyprint']"});

    /* Context menu for script view */
    _C("script-list-tree", "script");
    __m("find-url");
    __m("find-script");
    __m("clear-script", {enabledif: "cx.target.bpcount"});
    __m("-");
    __m("save-profile");
    __m("clear-profile");
     
    /* Context menu for stack view */
    _C("stack-tree", "stack");
    __m("frame",        {enabledif: "cx.target instanceof FrameRecord"});
    __m("find-creator",
         {enabledif: "cx.target instanceof ValueRecord && " +
                     "cx.target.jsType == jsdIValue.TYPE_OBJECT"});
    __m("find-ctor",
         {enabledif: "cx.target instanceof ValueRecord && " +
                     "cx.target.jsType == jsdIValue.TYPE_OBJECT"});
    
}

function _M(parent, id, attribs)
{
    console.lastMenu = parent + ":" + id;
    console.commandManager.appendSubMenu(parent, console.lastMenu,
                                         getMsg("mnu." + id));
}

function _C(elementId, id)
{
    console.lastMenu = "popup:" + id;
    console.commandManager.appendPopupMenu("dynamicPopups", console.lastMenu,
                                           getMsg("popup." + id));
    var elemObject = document.getElementById(elementId);
    elemObject.setAttribute ("context", console.lastMenu);
}

function __m(command, attribs)
{            
    if (command != "-")
    {
        if (!(command in console.commandManager.commands))
        {
            dd("no such command: " + command)
                return;
        }
        command = console.commandManager.commands[command];
    }
    console.commandManager.appendMenuItem(console.lastMenu, command, attribs);
}

function __t(parent, command, attribs)
{
    if (command != "-")
        command = console.commandManager.commands[command];
    console.commandManager.appendToolbarItem(parent, command, attribs);
}
