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

function initMenus()
{    
    function isVisible (view)
    {
        return "'currentContent' in console.views." + view;
    };

    console.menuSpecs = new Object();
    var menuManager = 
        console.menuManager = new MenuManager(console.commandManager,
                                              console.menuSpecs,
                                              getCommandContext);

    console.menuSpecs["maintoolbar"] = {
        items:
        [
         ["stop"],
         ["-"],
         ["cont"],
         ["next"],
         ["step"],
         ["finish"],
         ["-"],
         ["profile-tb"],
         ["toggle-pprint"]
        ]
    };

    console.menuSpecs["mainmenu:file"] = {
        label: MSG_MNU_FILE,
        items:
        [
         ["open-url"],
         ["find-file"],
         ["-"],
         ["close"],
         ["save-source"],
         ["save-profile"],
         ["-"],
         ["quit"]
        ]
    };

    console.menuSpecs["mainmenu:view"] = {
        label: MSG_MNU_VIEW,
        items:
        [
         [">popup:showhide"],
         ["-"],
         ["reload"],
         ["toggle-pprint",
                 {type: "checkbox",
                  checkedif: "console.prefs['prettyprint']"}],
         ["-"],
         ["toggle-chrome",
                 {type: "checkbox",
                  checkedif: "console.enableChromeFilter"}]
        ]
    };
    
    console.menuSpecs["mainmenu:debug"] = {
        label: MSG_MNU_DEBUG,
        items:
        [
         ["stop",
                 {type: "checkbox",
                  checkedif: "console.jsds.interruptHook"}],
         ["cont"],
         ["next"],
         ["step"],
         ["finish"],
         ["-"],
         [">popup:emode"],
         [">popup:tmode"],
         ["-"],
         ["toggle-ias",
                 {type: "checkbox",
                  checkedif: "console.jsds.initAtStartup"}],
        ]
    };
    
    console.menuSpecs["mainmenu:profile"] = {
        label: MSG_MNU_PROFILE,
        items:
        [
         ["toggle-profile",
                 {type: "checkbox",
                  checkedif: "console.jsds.flags & COLLECT_PROFILE_DATA"}],
         ["clear-profile"],
         ["save-profile"]
        ]
    };

    console.menuSpecs["popup:emode"] = {
        label: MSG_MNU_EMODE,
        items:
        [
         ["em-ignore",
                 {type: "radio", name: "em",
                  checkedif: "console.errorMode == EMODE_IGNORE"}],
         ["em-trace",
                 {type: "radio", name: "em",
                  checkedif: "console.errorMode == EMODE_TRACE"}],
         ["em-break",
                 {type: "radio", name: "em",
                  checkedif: "console.errorMode == EMODE_BREAK"}]
        ]
    };
    
    console.menuSpecs["popup:tmode"] = {
        label: MSG_MNU_TMODE,
        items:
        [
         ["tm-ignore",
                 {type: "radio", name: "tm",
                  checkedif: "console.throwMode == TMODE_IGNORE"}],
         ["tm-trace",
                 {type: "radio", name: "tm",
                  checkedif: "console.throwMode == TMODE_TRACE"}],
         ["tm-break",
                 {type: "radio", name: "tm",
                  checkedif: "console.throwMode == TMODE_BREAK"}]
        ]
    };

    console.menuSpecs["popup:showhide"] = {
        label: MSG_MNU_SHOWHIDE,
        items:
        [
         ["toggle-breaks", 
                 {type: "checkbox", checkedif: isVisible("breaks")}],
         ["toggle-stack",
                 {type: "checkbox", checkedif: isVisible("stack")}],
         ["toggle-session",
                 {type: "checkbox", checkedif: isVisible("session")}],
         ["toggle-locals",
                 {type: "checkbox", checkedif: isVisible("locals")}],
         ["toggle-scripts",
                 {type: "checkbox", checkedif: isVisible("scripts")}],
         ["toggle-windows",
                 {type: "checkbox", checkedif: isVisible("windows")}],
         ["toggle-source",
                 {type: "checkbox", checkedif: isVisible("source")}],
         ["toggle-watch",
                 {type: "checkbox", checkedif: isVisible("watches")}]
        ]
    };
}

function createMainMenu(document)
{
    var mainmenu = document.getElementById("mainmenu");
    var menuManager = console.menuManager;
    for (var id in console.menuSpecs)
    {
        if (id.indexOf("mainmenu:") == 0)
            menuManager.createMenu (mainmenu, null, id);
    }
}

function createMainToolbar(document)
{
    var maintoolbar = document.getElementById("maintoolbar");
    var menuManager = console.menuManager;
    var spec = console.menuSpecs["maintoolbar"];
    for (var i in spec.items)
    {
        menuManager.appendToolbarItem (maintoolbar, null, spec.items[i]);
    }
}

function getCommandContext (id)
{
    var cx = {};
    
    if (id in console.menuSpecs)
    {
        if ("getContext" in console.menuSpecs[id])
            cx = console.menuSpecs[id].getContext(cx);
    }
    else
    {
        dd ("getCommandContext: unknown menu id " + id);
    }

    if (typeof cx == "object")
    {
        if (!("menuManager" in cx))
            cx.menuManager = console.menuManager;
        if (!("contextSource" in cx))
            cx.contextSource = id;
        if ("dbgContexts" in console && console.dbgContexts)
            dd ("context '" + id + "'\n" + dumpObjectTree(cx));
    }

    return cx;
}
