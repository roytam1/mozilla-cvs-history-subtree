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

const VMGR_VURL_SCHEME = "x-vloc:";
const VMGR_SCHEME_LEN  = VMGR_VURL_SCHEME.length;

const VMGR_HIDDEN      = "hidden";
const VMGR_NEW         = "new";
const VMGR_MAINWINDOW  = "mainwindow";

const VMGR_VURL_HIDDEN     = VMGR_VURL_SCHEME + "/" + VMGR_HIDDEN;
const VMGR_VURL_NEW        = VMGR_VURL_SCHEME + "/" + VMGR_NEW;
const VMGR_VURL_MAINWINDOW = VMGR_VURL_SCHEME + "/" + VMGR_MAINWINDOW;

const VMGR_DEFAULT_CONTAINER = "initial-container";

function ViewManager(commandManager, mainWindow)
{
    this.commandManager = commandManager;    
    this.floaterSequence = 0;
    this.windows = new Object();
    this.mainWindow = mainWindow;
    this.windows[VMGR_MAINWINDOW] = mainWindow;
    this.views = new Object();
}

ViewManager.prototype.realizeViews =
function vmgr_realizeviews (views)
{
    for (var v in views)
    {
        if (ASSERT("init" in views[v],
                   "View " + v + " does not have an init() property"))
        {
            this.views[v] = views[v];
            this.realizeView(views[v]);
        }
    }
}    

ViewManager.prototype.realizeView =
function vmgr_realizeview (view)
{
    var key = view.viewId;
    this.views[key] = view;
    view.init();
    if ("cmdary" in view)
    {
        var commands = this.commandManager.defineCommands(view.cmdary);
        for (var i in this.windows)
            this.commandManager.installKeys (this.windows[i].document, commands);
        view.commands = commands;
    }
    if ("hooks" in view)
        this.commandManager.addHooks (view.hooks, key);
}

ViewManager.prototype.unrealizeView =
function vmgr_unrealizeview (view)
{
    delete this.views[view.viewId];
    if ("commands" in view)
        this.commandManager.uninstallKeys(view.commands);
    if ("hooks" in view)
        this.commandManager.removeHooks (view.hooks, view.viewId);
}

ViewManager.prototype.getWindowById =
function vmgr_getwindow (windowId)
{
    if (windowId in this.windows)
        return this.windows[windowId];

    return null;
}

ViewManager.prototype.createWindow =
function vmgr_createwindow (windowId, cb)
{
    if (!ASSERT(!(windowId in this.windows), "window " + windowId +
                " already exists"))
    {
        return null;
    }
    
    var win = openDialog ("chrome://venkman/content/venkman-floater.xul?id=" +
                          escape(windowId), "_blank",
                          "chrome,menubar,toolbar,resizable,dialog=no", cb);
    this.windows[windowId] = win;
    return win;
}

ViewManager.prototype.destroyWindows =
function vmgr_destroywindows ()
{
    for (var w in this.windows)
    {
        if (w == VMGR_MAINWINDOW)
            this.destroyWindow(w);
        else
            this.windows[w].close();
    }
}

ViewManager.prototype.destroyWindow =
function vmgr_destroywindow (windowId)
{
    if (!ASSERT(windowId in this.windows, "unknown window id"))
        return;

    var document = this.windows[windowId].document;
    
    for (var viewId in this.views)
    {
        var view = this.views[viewId];
        
        if ("currentContent" in view &&
            view.currentContent.ownerDocument == document)
        {
            this.moveViewURL(VMGR_VURL_HIDDEN, viewId);
        }
    }

    delete this.windows[windowId];
}

ViewManager.prototype.createContainer =
function vmgr_createcontainer (parsedLocation, containerId, type)
{
    if (!type)
        type = "horizontal";
    
    var parentContainer = this.getLocation(parsedLocation);
    
    if (!ASSERT(parentContainer, "location is hidden or does not exist"))
        return null;

    var document = parentContainer.ownerDocument;
    var container = document.createElement("viewcontainer");
    container.setAttribute ("id", containerId);
    container.setAttribute ("flex", "1");
    container.setAttribute ("type", type);
    if ("width" in parsedLocation)
        container.setAttribute ("width", parsedLocation.width);
    if ("height" in parsedLocation)
        container.setAttribute ("height", parsedLocation.height);

    var before;
    if ("before" in parsedLocation)
        before = getChildById(parentContainer, parsedLocation.before);
    parentContainer.insertBefore(container, before);
    this.groutContainer(parentContainer);

    return container;
}

/*
 * x-vloc:/<window-id>[/<container-id>][?<option>[&<option>...]]
 *
 * <option> is one of...
 *   before=<view-id>
 *   type=(horizontal|vertical|tab)
 *   id=(<view-id>|<container-id>)
 *   target=(view|container)
 *   width=<number>
 *   height=<number>
 *
 * parseResult
 *  +- url
 *  +- windowId
 *  +- containerId
 *  +- <option>
 *  +- ...
 */

ViewManager.prototype.parseLocation =
function vmgr_parselocation (locationURL)
{
    var parseResult;
    var ary;
    
    if (locationURL.indexOf(VMGR_VURL_SCHEME) != 0)
        return null;
    
    ary = locationURL.substr(VMGR_SCHEME_LEN).match(/([^\/?]+)(?:\/([^\/?]+))?/);
    if (!ary)
        return null;

    parseResult = {
        url: locationURL,
        windowId: ary[1],
        containerId: (2 in ary) ? ary[2] : VMGR_DEFAULT_CONTAINER
    };

    var rest = RegExp.rightContext.substr(1);

    ary = rest.match(/([^&]+)/);

    while (ary)
    {
        rest = RegExp.rightContext.substr(1);
        var assignment = ary[1];
        ary = assignment.match(/(.+)=(.*)/);
        if (ASSERT(ary, "error parsing ``" + assignment + "'' from " +
                   locationURL))
        {
            /* only set the property the first time we see it */
            if (2 in ary && !(ary[1] in parseResult))
                parseResult[ary[1]] = ary[2];
        }
        ary = rest.match(/([^&]+)/);
    }
    
    if (parseResult.windowId == VMGR_NEW)
        parseResult.windowId = "floater-" + this.floaterSequence++;

    //dd (dumpObjectTree(parseResult));
    return parseResult;
}

ViewManager.prototype.getWindowId =
function getwindowid (window)
{
    for (var m in this.windows)
    {
        if (this.windows[m] == window)
            return m;
    }

    ASSERT (0, "unknown window");
    return null;
}   

ViewManager.prototype.getContainerContents =
function vmgr_getcontents (container, recurse, ary)
{
    if (!ary)
        ary = new Array();
    var child = container.firstChild;
    
    while (child)
    {
        ary.push (this.computeLocation(child));
        if (child.localName == "viewcontainer" && recurse)
            this.getContainerContents(child, true, ary);
        child = child.nextSibling;
    }
}

ViewManager.prototype.reconstituteVURLs =
function vmgr_remake (ary)
{
    for (i in ary)
    {
        var parsedLocation = console.viewManager.parseLocation (ary[i]);
        if (!ASSERT(parsedLocation, "can't parse " + ary[i]) ||
            !ASSERT("target" in parsedLocation, "no target in " + ary[i]) ||
            !ASSERT("id" in parsedLocation, "no id in " + ary[i]))
        {
            continue;
        }

        if (parsedLocation.target == "container")
        {
            if (!ASSERT("type" in parsedLocation, "no type in " + ary[i]))
                parsedLocation.type = "horizontal";
            this.createContainer (parsedLocation, parsedLocation.id,
                                  parsedLocation.type);
        }
        else if (parsedLocation.target == "view")
        {
            this.moveView (parsedLocation, parsedLocation.id);
        }
        else
        {
            ASSERT(0, "unknown target in " + ary[i]);
        }
    }
}

ViewManager.prototype.computeLocation =
function vmgr_getlocation (element)
{
    if (!element.parentNode)
        return VMGR_VURL_HIDDEN;
    
    var result = VMGR_VURL_SCHEME;

    if (element.localName == "floatingview")
    {
        if (element.parentNode.localName != "viewcontainer")
            return VMGR_VURL_HIDDEN;

        result += ("/" + this.getWindowId(element.ownerWindow) +
                   "/" + element.parentNode.getAttribute("id") +
                   "?target=view" +
                   "&id=" + element.getAttribute("id") +
                   "&height=" + element.getAttribute("height") +
                   "&width=" + element.getAttribute("width"));

    }
    else if (element.localName == "viewcontainer")
    {
        result += ("/" + this.getWindowId(element.ownerWindow) +
                   "/" + element.parentNode.getAttribute("id") +
                   "?target=container" +
                   "&type=" + element.getAttribute("type") +
                   "&id=" + element.getAttribute("id") +
                   "&height=" + element.getAttribute("height") +
                   "&width=" + element.getAttribute("width"));
    }
    else
    {
        ASSERT(0, "can't get location for unknown element " + element.localName);
        return null;
    }

    var beforeNode = element.nextSibling;
    if (beforeNode)
    {
        if (beforeNode.hasAttribute("grout"))
            beforeNode = beforeNode.nextSibling;
        
        if (ASSERT(beforeNode, "nothing before the grout?"))
            result += "&before=" + beforeNode.getAttribute("id");
    }

    return result;
}

ViewManager.prototype.locationExists =
function vmgr_islocation (parsedLocation)
{
    if (parsedLocation.windowId == VMGR_HIDDEN)
        return true;
    
    if (this.getLocation(parsedLocation))
        return true;

    return false;
}

ViewManager.prototype.getLocation =
function vmgr_getlocation (parsedLocation)
{
    if (parsedLocation.windowId == VMGR_HIDDEN)
        return null;
    
    var window = this.getWindowById(parsedLocation.windowId);
    if (!window)
        return false;

    var container = window.document.getElementById(parsedLocation.containerId);
    return container;
}    

ViewManager.prototype.ensureLocation =
function vmgr_ensurelocation (parsedLocation, cb)
{
    function onWindowLoaded (window)
    {
        var container = 
            window.document.getElementById (parsedLocation.containerId);
        cb (window, container);
    };
    
    if (parsedLocation.windowId == VMGR_HIDDEN)
    {
        cb(null, null);
        return;
    }
    
    var window = this.getWindowById(parsedLocation.windowId);
    if (window)
    {
        onWindowLoaded(window);
        return;
    }
    
    this.createWindow (parsedLocation.windowId, onWindowLoaded);
}

ViewManager.prototype.moveViewURL =
function vmgr_moveurl (locationURL, viewId)
{
    var parsedLocation = this.parseLocation(locationURL);
    if (!ASSERT(parsedLocation, "can't parse " + locationURL))
        return;
    
    this.moveView(parsedLocation, viewId);
}

ViewManager.prototype.moveView =
function vmgr_move (parsedLocation, viewId)
{
    dd ("moving: ``" + viewId + "''\n" + dumpObjectTree(parsedLocation));
    
    var viewManager = this;
    
    function moveContent (content, newParent, before)
    {
        if (!("originalParent" in content))
        {
            if (!ASSERT(content.parentNode, "no original parent to record"))
                return;
            content.originalParent = content.parentNode;
        }
        
        dd ("moveContent {");
        
        var previousParent = content.parentNode;
        previousParent.removeChild(content);
        if (previousParent.localName == "viewcontainer")
            viewManager.groutContainer (previousParent);

        if (newParent)
        {
            dd ("placing in new parent");
            content.setAttribute ("containertype",
                                  newParent.getAttribute("type"));
            newParent.insertBefore (content, before);
            viewManager.groutContainer (newParent);
        }
        else
        {
            dd ("hiding");
            content.originalParent.appendChild(content);
            content.setAttribute ("containertype", "hidden");
        }

        dd ("}");
    };

    function onLocationFound (window, container)
    {
        if (!window)
        {
            /* view needs to be hidden. */
            if (currentContent)
            {
                if ("onHide" in view)
                    view.onHide();

                moveContent (view.currentContent, null);
                delete view.currentContent;
            }
            return;
        }
        
        var content = window.document.getElementById(viewId);
        if (!ASSERT(content, "no content for ``" + viewId + "'' found in " +
                    parsedLocation.windowId))
        {
            return;
        }

        var beforeNode;
        if ("before" in parsedLocation)
        {
            beforeNode = window.document.getElementById(parsedLocation.before);
        }
        
        if (!container)
        {
            /* container does not exist. */
            if (beforeNode)
            {
                container = beforeNode.parentNode;
            }
            else
            {
                container =
                    window.document.getElementById(VMGR_DEFAULT_CONTAINER);
            }
            if (!ASSERT(container, "can't locate default container"))
                return;
        }
        else if (beforeNode && beforeNode.parentNode != container)
        {
            /* container portion of url wins in a mismatch situation */
            beforeNode = null;
        }   
        
        if (currentContent)
        {
            dd ("have content");
            /* view is already visible, fire an onHide().  If we're destined for
             * a new document, return the current content to it's hiding place.
             */
            if (beforeNode == currentContent ||
                (currentContent.nextSibling == beforeNode &&
                 container == currentContent.parentNode))
            {
                dd ("TEASE!");
                /* unless of course we're already where we are supposed to be. */
                return;
            }   
            
            if ("onHide" in view)
                view.onHide();
            if (currentContent != content)
            {
                dd ("returning content to home");
                moveContent (currentContent, null);
            }
        }
        
        dd ("moving to new location");
        moveContent (content, container, beforeNode);
        view.currentContent = content;
        
        //if ("onShow" in view)
        //    view.onShow();
    };

    var view = console.views[viewId];
    var currentContent = ("currentContent" in view ?
                          view.currentContent : null);
    if (currentContent)
        view.previousLocation = this.computeLocation(currentContent);

    this.ensureLocation (parsedLocation, onLocationFound);
    
}

ViewManager.prototype.groutContainer =
function vmgr_groutcontainer (container)
{
    dd ("grouting: " + container.localName);
    
    var type = container.getAttribute ("type");
    
    if (!ASSERT(type in this.groutFunctions, "unknown container type ``" +
                type + "''"))
    {
        return;
    }
    
    this.groutFunctions[type](this, container);
}

ViewManager.prototype.groutFunctions = new Object();

ViewManager.prototype.groutFunctions["horizontal"] =
ViewManager.prototype.groutFunctions["vertical"] =
function vmgr_groutbox (viewManager, container)
{
    function closeWindow (window)
    {
        window.close();
    };
    
    function isContainerEmpty (container)
    {
        return container.localName == "viewcontainer" && !container.firstChild;
    };
    
    if (!container)
        throw new InvalidParam ("container", container);

    ASSERT(container.localName == "viewcontainer",
           "Attempt to grout something that is not a view container");
    
    var doc = container.ownerDocument;
    var content = container.firstChild;
    
    if (container.parentNode.localName == "viewcontainer")
        viewManager.groutContainer(container.parentNode);

    if (isContainerEmpty(container))
    {
        container.removeAttribute("flex");
        if (container.parentNode.localName == "window" &&
            !("isClosing" in container.ownerWindow))
        {
            setTimeout (closeWindow, 1, container.ownerWindow);
        }
        return;
    }

    container.setAttribute("flex", "1");
    
    while (content)
    {
        var previousContent = content.previousSibling;
        var nextContent = content.nextSibling;

        if (content.hasAttribute("grout"))
        {
            if (!previousContent || !nextContent ||
                previousContent.hasAttribute("grout") ||
                nextContent.hasAttribute("grout") ||
                isContainerEmpty(previousContent) ||
                (nextContent == container.lastChild &&
                 isContainerEmpty(nextContent)))
            {
                container.removeChild(content);
            }
        }
        else
        {
            if (nextContent && !nextContent.hasAttribute("grout") &&
                !isContainerEmpty(nextContent))
            {
                var collapse;
                if (content.hasAttribute("splitter-collapse"))
                    collapse = content.getAttribute("splitter-collapse");
                else
                    collapse = "after";
                var split = doc.createElement("splitter");
                split.setAttribute("grout", "true");
                split.setAttribute("collapse", collapse);
                split.appendChild(doc.createElement("grippy"));
                container.insertBefore(split, nextContent);
            }
        }
        content = nextContent;
    }
}
