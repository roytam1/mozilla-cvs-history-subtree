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

function initViews()
{
    const ATOM_CTRID = "@mozilla.org/atom-service;1";
    const nsIAtomService = Components.interfaces.nsIAtomService;
    console.atomService =
        Components.classes[ATOM_CTRID].getService(nsIAtomService);

    console.floatWindows = new Object(); 

    console.floaterSequence = 0;

    for (var v in console.views)
    {
        if (ASSERT("init" in console.views[v],
                   "View " + v + " does not have an init() property"))
        {
            console.realizeView(console.views[v], v);
        }
    }
}

console.destroyViews = destroyViews;

function destroyViews(doc)
{
    for (var v in console.views)
    {
        var view = console.views[v];
        
        if ("currentContent" in view &&
            view.currentContent.ownerDocument == doc)
        {
            console.dispatch ("hide-view", {viewId: v});
        }
    }
}

console.realizeView =
function con_realizeview (view, key)
{
    console.views[key] = view;
    console.views[key].init();
    if ("hooks" in console.views[key])
        console.commandManager.addHooks (console.views[key].hooks, key);
}

/**
 * Sync a XUL tree with the view that it represents.
 *
 * XUL trees seem to take a small amount of time to initialize themselves to
 * the point where they're willing to accept a view object.  This method will
 * call itself back after a small delay if the XUL tree is not ready yet.
 */
function syncTreeView (treeContent, treeView, cb)
{
    function tryAgain()
    {
        if ("treeBoxObject" in treeContent)
        {
            syncTreeView(treeContent, treeView, cb);
        }
        else
        {
            //dd ("trying to sync " + treeContent.getAttribute("id") + " AGAIN");
            setTimeout (tryAgain, 500);
        }
    };
    
    if (!("treeBoxObject" in treeContent))
    {
        setTimeout (tryAgain, 500);
        return;
    }
    
    treeContent.treeBoxObject.view = treeView;
    if (treeContent.treeBoxObject.selection)
        treeContent.treeBoxObject.selection.tree = treeContent.treeBoxObject;

    if (typeof cb == "function")
        cb();
}

function getTreeContext (view, cx, recordContextGetter)
{
    dd ("getTreeContext {");

    var i = 0;
    var selection = view.tree.selection;
    var row = selection.currentIndex;
    var rec;
    
    if (view instanceof XULTreeView)
    {
        rec = view.childData.locateChildByVisualRow (row);
        if (!rec)
        {
            dd ("} no record at currentIndex " + row);
            return cx;
        }
    }
    else
    {
        rec = row;
    }
    
    cx.target = rec;

    recordContextGetter(cx, rec, i++);
    var rangeCount = selection.getRangeCount();

    dd ("walking ranges {");
    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        selection.getRangeAt(range, min, max);
        min = min.value;
        max = max.value;

        for (row = min; row <= max; ++row)
        {
            if (view instanceof XULTreeView)
            {
                rec = view.childData.locateChildByVisualRow(row);
                if (!rec)
                    dd ("no record at range index " + row);
            }
            else
            {
                rec = row;
            }

            recordContextGetter(cx, rec, i++);
        }
    }
    dd ("}");
    dd ("cleaning up {");
    /* delete empty arrays as that may have been left behind. */
    for (var p in cx)
    {
        if (cx[p] instanceof Array && !cx[p].length)
            delete cx[p];
    }
    dd ("}");
    dd ("}");
    
    return cx;
}

function initContextMenu (document, id)
{
    if (!document.getElementById(id))
    {
        if (!ASSERT(id in console.menuSpecs, "unknown context menu " + id))
            return;

        var dp = document.getElementById("dynamic-popups");
        var popup = console.menuManager.appendPopupMenu (dp, null, id);
        var items = console.menuSpecs[id].items;
        console.menuManager.createMenuItems (popup, null, items);
    }
}

console.views = new Object();

/*******************************************************************************
 * Breakpoints View
 *******************************************************************************/

console.views.breaks = new XULTreeView();

console.views.breaks.vewId = "breaks";

console.views.breaks.hooks = new Object();

console.views.breaks.hooks["hook-fbreak-set"] =
function bv_hookFBreakSet (e)
{
    var breakRecord = new BPRecord (e.fbreak);
    breakRecord.reserveChildren();
    e.fbreak.breakRecord = breakRecord;
    console.views.breaks.childData.appendChild(breakRecord);        
}

console.views.breaks.hooks["hook-fbreak-clear"] =
function bv_hookFBreakClear (e)
{
    var breakRecord = e.fbreak.breakRecord;
    delete e.fbreak.breakRecord;
    console.views.breaks.childData.removeChildAtIndex(breakRecord.childIndex);
    for (var i in breakRecord.childData)
    {
        console.views.breaks.childData.appendChild(breakRecord.childData[i]);
    }
}

console.views.breaks.hooks["hook-break-set"] =
function bv_hookBreakSet (e)
{
    var breakRecord = new BPRecord (e.breakWrapper);
    e.breakWrapper.breakRecord = breakRecord;
    if (e.breakWrapper.parentBP)
    {
        var parentRecord = e.breakWrapper.parentBP.breakRecord;
        parentRecord.appendChild(breakRecord);
    }
    else
    {
        console.views.breaks.childData.appendChild(breakRecord);
    }
}

console.views.breaks.hooks["hook-break-clear"] =
function bv_hookBreakClear (e)
{
    var breakRecord = e.breakWrapper.breakRecord;
    if (e.breakWrapper.parentBP)
    {
        var parentRecord = e.breakWrapper.parentBP.breakRecord;
        parentRecord.removeChildAtIndex(breakRecord.childIndex);
    }
    else
    {
        var idx = breakRecord.childIndex;
        console.views.breaks.childData.removeChildAtIndex(idx);
    }
}

console.views.breaks.init =
function bv_init ()
{
    console.menuSpecs["context:breaks"] = {
        getContext: this.getContext,
        items:
        [
         ["find-url"],
         ["-"],
         ["clear-break",  { enabledif: "has('hasBreak')" }],
         ["clear-fbreak", { enabledif: "has('hasFBreak')" }],
         ["-"],
         ["clear-all"],
         ["fclear-all"]
        ]
    };

    this.atomBreak = console.atomService.getAtom("item-breakpoint");
    this.atomFBreak = console.atomService.getAtom("future-breakpoint");
}

console.views.breaks.onShow =
function bv_show()
{
    syncTreeView (getChildById(this.currentContent, "break-tree"), this);
    initContextMenu(this.currentContent.ownerDocument, "context:breaks");
}

console.views.breaks.onHide =
function bv_hide()
{
    syncTreeView (getChildById(this.currentContent, "break-tree"), null);
}

console.views.breaks.onDblClick =
function bv_sel (e)
{
    var rowIndex = this.tree.selection.currentIndex;
    if (rowIndex == -1 || rowIndex > this.rowCount)
        return;
    var row = this.childData.locateChildByVisualRow(rowIndex);
    if (!row)
    {
        ASSERT (0, "bogus row index " + rowIndex);
        return;
    }

    if (row instanceof BPRecord)
        dispatch ("find-bp", {breakpoint: row.breakWrapper});
}

console.views.breaks.getContext =
function bv_getcx(cx)
{
    cx.breakWrapperList = new Array();
    cx.urlList = new Array();
    cx.lineNumberList = new Array();
    cx.pcList = new Array();
    cx.scriptWrapperList = new Array();
    
    function recordContextGetter (cx, rec, i)
    {
        if (i == 0)
        {
            cx.breakWrapper = rec.breakWrapper;
            if (rec.type == "instance")
            {
                cx.hasBreak = true;
                cx.scriptWrapper = rec.breakWrapper.scriptWrapper;
                cx.pc = rec.breakWrapper.pc;
            }
            else if (rec.type == "future")
            {
                cx.hasFBreak = true;
                cx.url = rec.breakWrapper.url;
                cx.lineNumber = rec.breakWrapper.lineNumber;
            }
        }
        else
        {
            cx.breakWrapperList.push(rec.breakWrapper);
            if (rec.type == "instance")
            {
                cx.hasBreak = true;
                cx.scriptWrapperList.push(rec.breakWrapper.scriptWrapper);
                cx.pcList.push(rec.breakWrapper.pc);
            }
            else if (rec.type == "instance")
            {
                cx.hasFBreak = true;
                cx.urlList.push(rec.breakWrapper.url);
                cx.lineNumberList.push(rec.breakWrapper.lineNumber);
            }
        }
    };

    return getTreeContext (console.views.breaks, cx, recordContextGetter);
}

console.views.breaks.getCellProperties =
function bv_cellprops (index, colID, properties)
{
    if (colID == "breaks:col-0")
    {
        var row = this.childData.locateChildByVisualRow(index);
        if (row.type == "future")
            properties.AppendElement (this.atomFBreak);
        else
            properties.AppendElement (this.atomBreak);
    }
}

/*******************************************************************************
 * Locals View
 *******************************************************************************/

var localsShare = new Object();

console.views.locals = new XULTreeView();

console.views.locals.viewId = "locals";

console.views.locals.init =
function lv_init ()
{
    /* max number of properties the "scope" or "this" object can have before
     * we stop auto-opening the tree node. */
    console.addPref("localsView.autoOpenMax", 25);
    /* max number of functions to keep open/close states for. */
    console.addPref("localsView.savedStatesMax", 20);

    console.menuSpecs["context:locals"] = {
        getContext: this.getContext,
        items:
        [
         ["find-creator",
                 {enabledif: "cx.target instanceof ValueRecord && " +
                  "cx.target.jsType == jsdIValue.TYPE_OBJECT && " +
                  "cx.target.value.objectValue.creatorURL"}],
         ["find-ctor",
                 {enabledif: "cx.target instanceof ValueRecord && " +
                  "cx.target.jsType == jsdIValue.TYPE_OBJECT && " +
                  "cx.target.value.objectValue.constructorURL"}]
        ]
    };

    this.jsdFrame = null;
    this.savedStates = new Object();
    this.stateTags = new Array();
}

console.views.locals.clear =
function lv_clear ()
{
    while (this.childData.childData.length)
        this.childData.removeChildAtIndex(0);
}

console.views.locals.changeFrame =
function lv_renit (jsdFrame)
{
    this.clear();
    
    if (!jsdFrame)
    {
        delete this.jsdFrame;
        return;
    }
    
    this.jsdFrame = jsdFrame;
    var tag = jsdFrame.script.tag;
    var state;
    if (tag in this.savedStates)
        state = this.savedStates[tag];
    
    if (jsdFrame.scope)
    {
        this.scopeRecord = new ValueRecord (jsdFrame.scope, MSG_VAL_SCOPE, "");
        if (!state && jsdFrame.scope.propertyCount <
            console.prefs["localsView.autoOpenMax"])
        {
            this.scopeRecord.open();
        }
        
    }
    else
    {
        this.scopeRecord = new XTLabelRecord ("locals:col-0", MSV_VAL_SCOPE,
                                              ["locals:col-1", "locals:col-2",
                                               "locals:col-3"]);
        this.scopeRecord.property = ValueRecord.prototype.atomObject;
    }
    
    if (jsdFrame.thisValue)
    {
        this.thisRecord = new ValueRecord (jsdFrame.thisValue, MSG_VAL_THIS, "");
        if (!state && jsdFrame.thisValue.propertyCount < 
            console.prefs["localsView.autoOpenMax"])
        {
            this.scopeRecord.open();
        }
    }    
    else
    {
        this.thisRecord = new XTLabelRecord ("locals:col-0", MSV_VAL_THIS,
                                             ["locals:col-1", "locals:col-2",
                                              "locals:col-3"]);
        this.thisRecord.property = ValueRecord.prototype.atomObject;
    }

    this.childData.appendChild(this.scopeRecord);
    this.childData.appendChild(this.thisRecord);

    if (state)
        this.restoreState(state);
}

console.views.locals.restoreState =
function lv_restore (state)
{
    function restoreBranch (target, source)
    {
        for (var i in source)
        {
            if (typeof source[i] == "object")
            {
                var name = source[i].name;
                var len = target.length;
                for (var j = 0; j < len; ++j)
                {
                    if (target[j]._colValues["col-0"] == name &&
                        "childData" in target[j])
                    {
                        //dd ("opening " + name);
                        target[j].open();
                        restoreBranch (target[j].childData, source[i]);
                        break;
                    }
                }
            }
        }
    };
    
    this.freeze();
    if ("scopeState" in state)
    {
        this.scopeRecord.open();
        restoreBranch (this.scopeRecord.childData, state.scopeState);
    }
    if ("thisState" in state)
    {
        this.thisRecord.open();
        restoreBranch (this.thisRecord.childData, state.thisState);
    }
    this.thaw();
    this.scrollTo (state.firstVisible, -1);
}

console.views.locals.saveState =
function sv_save ()
{
    if (!ASSERT(this.jsdFrame, "no frame"))
        return;
        
    function saveBranch (target, source)
    {
        var len = source.length;
        for (var i = 0; i < len; ++i)
        {
            if (source[i].isContainerOpen)
            {
                target[i] = new Object();
                target[i].name = source[i]._colValues["col-0"];
                saveBranch (target[i], source[i].childData);
            }
        }
    };

    var tag = this.jsdFrame.script.tag;    

    if (!tag in this.savedStates &&
        this.stateTags.length == console.prefs["localsView.maxSavedStates"])
    {
        delete this.savedStates[this.stateTags.shift()];
        this.stateTags.push(tag);
    }
        
    var state = this.savedStates[tag] = new Object();
    if (this.tree)
        state.firstVisible = this.tree.getFirstVisibleRow() + 1;
    else
        state.firstVisible = 1;

    if (this.scopeRecord.isContainerOpen)
    {
        state.scopeState = { name: "scope" };
        saveBranch (state.scopeState, this.scopeRecord.childData);
    }
    
    if (this.thisRecord.isContainerOpen)
    {
        state.thisState = { name: "this" };
        saveBranch (state.thisState, this.thisRecord.childData);
    }
    
    //dd ("saved as\n" + dumpObjectTree(this.savedState, 10));
}

console.views.locals.hooks = new Object()

console.views.locals.hooks["hook-debug-stop"] =
function lv_hookDebugStop (e)
{
    console.views.locals.changeFrame(console.frames[0]);
}

console.views.locals.hooks["find-frame"] =
function lv_hookDebugStop (e)
{
    console.views.locals.changeFrame(console.frames[e.frameIndex]);
}

console.views.locals.hooks["hook-debug-continue"] =
function lv_hookDebugStop (e)
{
    console.views.locals.saveState();
    console.views.locals.clear();
}

console.views.locals.onShow =
function lv_show ()
{
    syncTreeView (getChildById(this.currentContent, "locals-tree"), this);
    initContextMenu(this.currentContent.ownerDocument, "context:locals");
}

console.views.locals.onHide =
function lv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "locals-tree"), null);
}

console.views.locals.onDblClick =
function lv_dblclick ()
{
    dd ("locals double click");
}

console.views.locals.getCellProperties =
function lv_cellprops (index, colID, properties)
{
    if (colID != "locals:col-0")
        return null;
    
    var row = this.childData.locateChildByVisualRow(index);
    if (row)
    {
        if ("getProperties" in row)
            return row.getProperties (properties);

        if (row.property)
            return properties.AppendElement (row.property);
    }

    return null;
}

console.views.locals.getContext =
function lv_getcx(cx)
{
    var view = console.views.locals;
    var selection = view.tree.selection;
    var row = selection.currentIndex;
    var rec = view.childData.locateChildByVisualRow (row);
    
    if (!rec)
    {
        dd ("no record at currentIndex " + row);
        return cx;
    }
    
    cx.target = rec;
    
    if (rec instanceof ValueRecord)
    {
        cx.jsdValue = rec.value;
        cx.jsdValueList = [rec.value];
    }

    var rangeCount = selection.getRangeCount();


    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        selection.getRangeAt(range, min, max);
        min = min.value;
        max = max.value;

        for (row = min; row <= max; ++row)
        {
            rec = this.childData.locateChildByVisualRow(row);
            if (rec instanceof ValueRecord)
                cx.jsdValueList.push(rec.value);
        }
    }

    return cx;
}

console.views.locals.refresh =
function lv_refresh()
{
    var delta = 0;
    var thisDelta;
    
    for (var i = 0; i < this.childData.length; ++i)
    {
        var item = this.childData[i];
        thisDelta = item.refresh();
        /* if the container isn't open, we still have to update the children,
         * but we don't care about any visual footprint changes */
        if (item.isContainerOpen)
            delta += thisDelta;
    }

    this.visualFootprint += delta;
    this.invalidateCache();
    this.syncTreeView();
}

/*******************************************************************************
 * Scripts View
 *******************************************************************************/

console.views.scripts = new XULTreeView ();

console.views.scripts.viewId = "scripts";

console.views.scripts.init =
function scv_init ()
{
    console.addPref ("scriptsView.groupFiles", true);    

    console.menuSpecs["context:scripts"] = {
        getContext: this.getContext,
        items:
        [
         ["find-url"],
         ["find-script"],
         ["clear-script", {enabledif: "cx.target.bpcount"}],
         ["-"],
         ["save-profile"],
         ["clear-profile"]
        ]
    };

    this.childData.setSortColumn("baseLineNumber");
    this.groupFiles = console.prefs["scriptsView.groupFiles"];

    var atomsvc = console.atomService;
    this.atomUnknown    = atomsvc.getAtom("item-unk");
    this.atomHTML       = atomsvc.getAtom("item-html");
    this.atomJS         = atomsvc.getAtom("item-js");
    this.atomXUL        = atomsvc.getAtom("item-xul");
    this.atomXML        = atomsvc.getAtom("item-xml");
    this.atomGuessed    = atomsvc.getAtom("item-guessed");
    this.atomBreakpoint = atomsvc.getAtom("item-has-bp");
    this.atomFunction   = atomsvc.getAtom("file-function");
}

console.views.scripts.hooks = new Object();

console.views.scripts.hooks["chrome-filter"] =
function scv_hookChromeFilter(e)
{
    var scriptsView = console.views.scripts;
    var nodes = scriptsView.childData;
    scriptsView.freeze();

    dd ("e.toggle is " + e.toggle);
                
    for (var m in console.scriptManagers)
    {
        if (console.scriptManagers[m].url.search(/^chrome:/) == -1)
            continue;
        
        for (var i in console.scriptManagers[m].instances)
        {
            var instance = console.scriptManagers[m].instances[i];
            if ("scriptInstanceRecord" in instance)
            {
                var rec = instance.scriptInstanceRecord;
                
                if (e.toggle)
                {
                    if (!ASSERT("parentRecord" in rec, "Record for " +
                                console.scriptManagers[m].url + 
                                " is already removed"))
                    {
                        continue;
                    }
                    /* filter is on, remove chrome file from scripts view */
                    dd ("removing " + console.scriptManagers[m].url + 
                        " kid at " + rec.childIndex);
                    nodes.removeChildAtIndex(rec.childIndex);
                }
                else
                {
                    if ("parentRecord" in rec)
                        continue;
                    //dd ("cmdChromeFilter: append " +
                    //    tov_formatRecord(rec, ""));
                    nodes.appendChild(rec);
                }
            }
        }
    }

    scriptsView.thaw();
}

console.views.scripts.hooks["hook-break-set"] =
console.views.scripts.hooks["hook-break-clear"] =
console.views.scripts.hooks["hook-fbreak-set"] =
console.views.scripts.hooks["hook-fbreak-clear"] =
function sch_hookBreakChange(e)
{
    if (console.views.scripts.tree)
        console.views.scripts.tree.invalidate();
}

console.views.scripts.hooks["hook-guess-complete"] =
function sch_hookGuessComplete(e)
{
    if (!("scriptInstanceRecord" in e.scriptInstance))
        return;
    
    var rec = e.scriptInstance.scriptInstanceRecord;
    if (!rec.childData.length)
        return;

    for (var i in rec.childData)
    {
        rec.childData[i].functionName =
            rec.childData[i].scriptWrapper.functionName;
    }

    if (console.views.scripts.tree)
        console.views.scripts.tree.invalidate();
}

console.views.scripts.hooks["hook-script-instance-sealed"] =
function scv_hookScriptInstanceSealed (e)
{
    //dd ("instance sealed: " + e.scriptInstance.url);
    
    var scr = new ScriptInstanceRecord (e.scriptInstance);
    e.scriptInstance.scriptInstanceRecord = scr;

    if (console.prefs["enableChromeFilter"] &&
        e.scriptInstance.url.search(/^chrome:/) == 0)
    {
        return;
    }
    
    console.views.scripts.childData.appendChild(scr);
}

console.views.scripts.hooks["hook-script-instance-destroyed"] =
function scv_hookScriptInstanceDestroyed (e)
{
    if (!("scriptInstanceRecord" in e.scriptInstance))
        return;
    
    var rec = e.scriptInstance.scriptInstanceRecord;
    if ("parentRecord" in rec)
        console.views.scripts.childData.removeChildAtIndex(rec.childIndex);
}

console.views.scripts.hooks["hook-window-opened"] =
function scv_hookWindowOpen (e)
{
    //console.views.scripts.freeze();
}

console.views.scripts.hooks["hook-window-loaded"] =
function scv_hookWindowOpen (e)
{
    //console.views.scripts.thaw();
}

console.views.scripts.onShow =
function scv_show ()
{
    syncTreeView (getChildById(this.currentContent, "scripts-tree"), this);
    initContextMenu(this.currentContent.ownerDocument, "context:scripts");
}

console.views.scripts.onHide =
function scv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "scripts-tree"), null);
}

console.views.scripts.onDblClick =
function scv_dblclick (e)
{
    var scriptsView = console.views.scripts;
    var rowIndex = scriptsView.tree.selection.currentIndex;
    
    if (rowIndex == -1)
        return;

    if (rowIndex == -1 || rowIndex > scriptsView.rowCount)
    {
        dd ("row out of bounds");
        return;
    }
    
    var row = scriptsView.childData.locateChildByVisualRow(rowIndex);
    ASSERT (row, "bogus row");

    if (row instanceof ScriptRecord)
        dispatch ("find-script", { scriptWrapper: row.scriptWrapper });
    else if (row instanceof ScriptInstanceRecord)
        dispatch ("find-sourcetext",
                  { sourceText: row.scriptInstance.sourceText });
}

console.views.scripts.onClick =
function scv_click (e)
{
    if (e.originalTarget.localName == "treecol")
    {
        /* resort by column */
        var rowIndex = new Object();
        var colID = new Object();
        var childElt = new Object();
        
        var treeBox = console.views.scripts.tree;
        treeBox.getCellAt(e.clientX, e.clientY, rowIndex, colID, childElt);
        var prop;
        switch (colID.value)
        {
            case "scripts:col-0":
                prop = "functionName";
                break;
            case "scripts:col-1":
                prop = "baseLineNumber";
                break;
            case "scripts:col-2":
                prop = "lineExtent";
                break;
        }

        var scriptsRoot = console.views.scripts.childData;
        var dir = (prop == scriptsRoot._share.sortColumn) ?
            scriptsRoot._share.sortDirection * -1 : 1;
        dd ("sort direction is " + dir);
        scriptsRoot.setSortColumn (prop, dir);
    }
}

console.views.scripts.onDragStart = Prophylactic(console.views.scripts,
                                                 scv_dstart);
function scv_dstart (e, transferData, dragAction)
{
    var row = new Object();
    var colID = new Object();
    var childElt = new Object();

    this.tree.getCellAt(e.clientX, e.clientY, row, colID, childElt);
    if (!colID.value)
        return false;
    
    row = this.childData.locateChildByVisualRow (row.value);
    var rv = false;
    if (row && ("onDragStart" in row))
        rv = row.onDragStart (e, transferData, dragAction);

    return rv;
}

console.views.scripts.fullNameMode = false;

console.views.scripts.setFullNameMode =
function scv_setmode (flag)
{
    this.fullNameMode = flag;
    for (var i = 0; i < this.childData.length; ++i)
        this.childData[i].setFullNameMode (flag);
}

console.views.scripts.getCellProperties =
function scv_getcprops (index, colID, properties)
{
    var row = this.childData.locateChildByVisualRow (index)
    if (row)
    {
        if (colID == "scripts:col-0")
        {
            if (row instanceof ScriptInstanceRecord)
            {
                properties.AppendElement (row.fileType);

                if ("isGuessedName" in row)
                    properties.AppendElement (this.atomGuessed);

                if (row.scriptInstance.breakpointCount)
                    properties.AppendElement (this.atomBreakpoint);
            }
            else if (row instanceof ScriptRecord)
            {
                properties.AppendElement (this.atomFunction);

                if (row.scriptWrapper.breakpointCount)
                    properties.AppendElement (this.atomBreakpoint);
            }
        }        
    }
}

console.views.scripts.getContext =
function scv_getcx(cx)
{
    var selection = this.tree.selection;
    var row = selection.currentIndex;
    var rec = this.childData.locateChildByVisualRow (row);
    var firstRec = rec;
    
    if (!rec)
    {
        dd ("no record at currentIndex " + row);
        return cx;
    }
    
    cx.target = rec;
    
    if (rec instanceof ScriptInstanceRecord)
    {
        cx.url = cx.fileName = rec.fileName;
        cx.scriptRec = rec.childData[0];
        cx.scriptRecList = rec.childData;
    }
    else if (rec instanceof ScriptRecord)
    {
        cx.scriptRec = rec;
        cx.lineNumber = rec.script.baseLineNumber;
        cx.rangeStart = cx.lineNumber;
        cx.rangeEnd   = rec.script.lineExtent + cx.lineNumber;
    }

    var rangeCount = selection.getRangeCount();
    if (rangeCount > 0 && !("lineNumberList" in cx))
    {
        cx.lineNumberList = new Array();
    }
    
    if (rangeCount > 0)
    {
        cx.urlList = cx.fileNameList = new Array();
        if (firstRec instanceof ScriptRecord)
            cx.scriptRecList  = new Array();
        cx.lineNumberList = new Array();
        cx.rangeStartList = new Array();
        cx.rangeEndList   = new Array();        
    }
    
    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        selection.getRangeAt(range, min, max);
        min = min.value;
        max = max.value;

        for (row = min; row <= max; ++row)
        {
            rec = this.childData.locateChildByVisualRow(row);
            if (rec instanceof ScriptInstanceRecord)
            {
                cx.fileNameList.push (rec.fileName);
            }
            else if (rec instanceof ScriptRecord)
            {
                //cx.fileNameList.push (rec.script.fileName);
                if (firstRec instanceof ScriptRecord)
                    cx.scriptRecList.push (rec);
                cx.lineNumberList.push (rec.script.baseLineNumber);
                cx.rangeStartList.push (rec.script.baseLineNumber);
                cx.rangeEndList.push (rec.script.lineExtent +
                                      rec.script.baseLineNumber);
            }
        }
    }

    return cx;
}    

/*******************************************************************************
 * Session View
 *******************************************************************************/

console.views.session = new Object();

console.views.session.viewId = "session";

console.views.session.init =
function ss_init ()
{
    console.addPref ("sessionView.commandHistory", 20);
    console.addPref ("sessionView.dtabTime", 500);

    console.menuSpecs["context:session"] = {
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
        ]
    };

    /* input history (up/down arrow) related vars */
    this.inputHistory = new Array();
    this.lastHistoryReferenced = -1;
    this.incompleteLine = "";

    /* tab complete */
    this.lastTabUp = new Date();

    this.outputTBody = new htmlTBody({ id: "session-output-tbody" })
    this.outputTable = null;
    this.outputWindow = null;
    this.outputDocument = null;
    this.outputElement = null;
    this.intputElement = null;

    this.munger = new CMunger();
    this.munger.enabled = true;
    this.munger.addRule
        ("link", /((\w+):\/\/[^<>()\'\"\s:]+|www(\.[^.<>()\'\"\s:]+){2,})/,
         insertLink);
    this.munger.addRule ("word-hyphenator",
                         new RegExp ("(\\S{" + MAX_WORD_LEN + ",})"),
                         insertHyphenatedWord);

}

console.views.session.hooks = new Object();

console.views.session.hooks["focus-input"] =
function ss_hookFocus(e)
{
    var sessionView = console.views.session;
    
    if ("inputElement" in sessionView)
        sessionView.inputElement.focus();
}

console.views.session.hooks["hook-session-display"] =
function ss_hookDisplay (e)
{
    var message = e.message;
    var msgtype = ("msgtype" in e) ? e.msgtype : MT_INFO;
    
    function setAttribs (obj, c, attrs)
    {
        if (attrs)
        {
            for (var a in attrs)
                obj.setAttribute (a, attrs[a]);
        }
        obj.setAttribute("class", c);
        obj.setAttribute("msg-type", msgtype);
    }

    var msgRow = htmlTR("msg");
    setAttribs(msgRow, "msg");

    var msgData = htmlTD();
    setAttribs(msgData, "msg-data");
    if (typeof message == "string")
        msgData.appendChild(console.views.session.stringToDOM(message));
    else
        msgData.appendChild(message);

    msgRow.appendChild(msgData);

    var sessionView = console.views.session;
    sessionView.outputTBody.appendChild(msgRow);
    sessionView.scrollDown();
}

console.views.session.hooks["hook-window-resized"] =
function ss_hookResize (e)
{
    console.views.session.scrollDown();
}

console.views.session.scrollDown =
function ss_scroll()
{
    if (this.outputWindow)
        this.outputWindow.scrollTo(0, this.outputDocument.height);
}

console.views.session.onShow =
function ss_show ()
{
    var sessionView = this;
    function tryAgain ()
    {
        //dd ("session view trying again...");
        sessionView.onShow();
    };

    var doc = this.currentContent.ownerDocument;
    
    try
    {
        this.outputDocument = 
            doc.getElementById("session-output-iframe").contentDocument;

        var win = this.currentContent.ownerWindow;
        for (var f = 0; f < win.frames.length; ++f)
        {
            if (win.frames[f].document == this.outputDocument)
            {
                this.outputWindow = win.frames[f];
                break;
            }
        }

        this.outputTable =
            this.outputDocument.getElementById("session-output-table");
        this.inputElement = doc.getElementById("session-sl-input");
    }
    catch (ex)
    {
        //dd ("caught exception showing session view...");
        //dd (dumpObjectTree(ex));
    }
    
    if (!this.outputDocument || !this.outputTable || !this.inputElement)
    {
        setTimeout (tryAgain, 500);
        return;
    }

    if (this.outputTable.firstChild)
        this.outputTable.removeChild (this.outputTable.firstChild);

    this.outputTable.appendChild (this.outputTBody);
    this.scrollDown();
    this.hooks["focus-input"]();
    initContextMenu(this.currentContent.ownerDocument, "context:session");
}

console.views.session.onHide =
function ss_hide ()
{
    this.outputTBody.parentNode.removeChild(this.outputTBody);
    this.outputTable = null;
    this.outputWindow = null;
    this.outputDocument = null;
    this.outputElement = null;
    this.intputElement = null;
}

console.views.session.stringToDOM = 
function ss_stringToDOM (message)
{
    var ary = message.split ("\n");
    var span = htmlSpan();
    
    for (var l in ary)
    {
        this.munger.munge(ary[l], span);
        span.appendChild (htmlBR());
    }

    return span;
}

console.views.session.onInputCompleteLine =
function ss_icline (e)
{    
    if (this.inputHistory.length == 0 || this.inputHistory[0] != e.line)
        this.inputHistory.unshift (e.line);
    
    if (this.inputHistory.length > console.prefs["sessionView.commandHistory"])
        this.inputHistory.pop();
    
    this.lastHistoryReferenced = -1;
    this.incompleteLine = "";
    
    var ev = {isInteractive: true, initialEvent: e};
    dispatch (e.line, ev, CMD_CONSOLE);
}

console.views.session.onSLKeyPress =
function ss_slkeypress (e)
{
    var w = this.outputWindow;
    var newOfs;
    
    switch (e.keyCode)
    {
        case 13:
            if (!e.target.value)
                return;
            e.line = e.target.value;
            this.onInputCompleteLine (e);
            e.target.value = "";
            break;

        case 38: /* up */
            if (this.lastHistoryReferenced == -2)
            {
                this.lastHistoryReferenced = -1;
                e.target.value = this.incompleteLine;
            }
            else if (this.lastHistoryReferenced <
                     this.inputHistory.length - 1)
            {
                e.target.value =
                    this.inputHistory[++this.lastHistoryReferenced];
            }
            break;

        case 40: /* down */
            if (this.lastHistoryReferenced > 0)
                e.target.value =
                    this.inputHistory[--this.lastHistoryReferenced];
            else if (this.lastHistoryReferenced == -1)
            {
                e.target.value = "";
                this.lastHistoryReferenced = -2;
            }
            else
            {
                this.lastHistoryReferenced = -1;
                e.target.value = this.incompleteLine;
            }
            break;
            
        case 33: /* pgup */
            newOfs = w.pageYOffset - (w.innerHeight / 2);
            if (newOfs > 0)
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, 0);
            break;
            
        case 34: /* pgdn */
            newOfs = w.pageYOffset + (w.innerHeight / 2);
            if (newOfs < (w.innerHeight + w.pageYOffset))
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, (w.innerHeight + w.pageYOffset));
            break;

        case 9: /* tab */
            e.preventDefault();
            this.onTabCompleteRequest(e);
            break;       

        default:
            this.lastHistoryReferenced = -1;
            this.incompleteLine = e.target.value;
            break;
    }
}


console.views.session.onTabCompleteRequest =
function ss_tabcomplete (e)
{
    var selStart = e.target.selectionStart;
    var selEnd   = e.target.selectionEnd;            
    var v        = e.target.value;
    
    if (selStart != selEnd) 
    {
        /* text is highlighted, just move caret to end and exit */
        e.target.selectionStart = e.target.selectionEnd = v.length;
        return;
    }

    var firstSpace = v.indexOf(" ");
    if (firstSpace == -1)
        firstSpace = v.length;

    var pfx;
    var d;
    
    if ((selStart <= firstSpace))
    {
        /* The cursor is positioned before the first space, so we're completing
         * a command
         */
        var partialCommand = v.substring(0, firstSpace).toLowerCase();
        var cmds = console.commandManager.listNames(partialCommand, CMD_CONSOLE);

        if (!cmds)
            /* partial didn't match a thing */
            display (getMsg(MSN_NO_CMDMATCH, partialCommand), MT_ERROR);
        else if (cmds.length == 1)
        {
            /* partial matched exactly one command */
            pfx = cmds[0];
            if (firstSpace == v.length)
                v =  pfx + " ";
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length + 1;
        }
        else if (cmds.length > 1)
        {
            /* partial matched more than one command */
            d = new Date();
            if ((d - this.lastTabUp) <= console.prefs["sessionView.dtabTime"])
                display (getMsg (MSN_CMDMATCH,
                                 [partialCommand, cmds.join(MSG_COMMASP)]));
            else
                this.lastTabUp = d;
            
            pfx = getCommonPfx(cmds);
            if (firstSpace == v.length)
                v =  pfx;
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length;
            
        }
                
    }

}

/*******************************************************************************
 * Stack View
 *******************************************************************************/

console.views.stack = new XULTreeView();

console.views.stack.viewId = "stack";

console.views.stack.init =
function skv_init()
{
}

console.views.stack.hooks = new Object();

console.views.stack.hooks["hook-debug-stop"] =
function skv_hookDebugStop (e)
{
    var frameRec;
    
    for (var i = 0; i < console.frames.length; ++i)
    {
        frameRec = new FrameRecord(console.frames[i]);
        console.views.stack.childData.appendChild (frameRec);
    }
}

console.views.stack.hooks["hook-debug-continue"] =
function skv_hookDebugCont (e)
{
    while (console.views.stack.childData.childData.length)
        console.views.stack.childData.removeChildAtIndex(0);
}

console.views.stack.hooks["hook-guess-complete"] =
function skv_hookGuessComplete(e)
{
    var frameRecs = console.views.stack.childData.childData;
    
    for (var i = 0; i < frameRecs.length; ++i)
    {
        if (!frameRecs[i].jsdFrame.isNative)
            frameRecs[i].functionName = frameRecs[i].scriptWrapper.functionName;
    }

    if (console.views.stack.tree)
        console.views.stack.tree.invalidate();
}

console.views.stack.hooks["find-frame"] =
function skv_hookFrame (e)
{
    if (e.frameIndex && console.views.stack.tree)
    {
        console.views.stack.scrollTo (e.frameIndex, 0);
        console.views.stack.tree.selection.currentIndex = e.frameIndex;
    }
}

console.views.stack.onShow =
function skv_show()
{
    syncTreeView (getChildById(this.currentContent, "stack-tree"), this);
}

console.views.stack.onHide =
function skv_hide()
{
    syncTreeView (getChildById(this.currentContent, "stack-tree"), null);
}

console.views.stack.onDblClick =
function skv_select (row)
{
    var rowIndex = console.views.stack.tree.selection.currentIndex;
    
    if (rowIndex == -1)
        return;

    if (rowIndex >= 0 && rowIndex < console.frames.length)
        dispatch ("frame", { frameIndex: rowIndex });
}

console.views.stack.getContext =
function sv_getcx(cx)
{
    if (!cx)
        cx = new Object();

    var selection = this.tree.selection;

    var rec = this.childData.locateChildByVisualRow(selection.currentIndex);
    
    if (!rec)
    {
        dd ("no current index.");
        return cx;
    }

    cx.target = rec;    
    cx.frameIndex = rec.childIndex;

    var rangeCount = selection.getRangeCount();
    if (rangeCount > 0)
        cx.frameIndexList = new Array();
    
    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        selection.getRangeAt(range, min, max);
        for (var i = min.value; i < max.value; ++i)
        {
            rec = this.childData.locateChildByVisualRow(i);
            cx.frameIndexList.push(rec.childIndex);
        }
    }

    return cx;
}    

console.views.stack.getCellProperties =
function sv_cellprops (index, colID, properties)
{
    if (colID != "stack:col-0")
        return;

    var row = this.childData.locateChildByVisualRow(index);
    if (row)
    {
        if ("getProperties" in row)
            row.getProperties (properties);
        else if (row.property)
            properties.AppendElement (row.property);
    }

    return;
}

/*******************************************************************************
 * Source View
 *******************************************************************************/
console.views.source = new BasicOView();

console.views.source.viewId = "source";

console.views.source.details = null;
console.views.source.prettyPrint = false;

console.views.source.init =
function sv_init()
{
    this.savedState = new Object();

    console.menuSpecs["context:source"] = {
        getContext: this.getContext,
        items:
        [
         ["save-source"],
         ["-"],
         ["break", 
                 {enabledif: "cx.lineIsExecutable && !has('hasBreak')"}],
         ["clear",
                 {enabledif: "has('hasBreak')"}],
         ["fbreak",
                 {enabledif: "!has('hasFBreak')"}],
         ["fclear",
                 {enabledif: "has('hasFBreak')"}],
         ["-"],
         ["cont"],
         ["next"],
         ["step"],
         ["finish"],
         ["-"],
         ["toggle-pprint",
                 {type: "checkbox",
                  checkedif: "console.prefs['prettyprint']"}]
        ]
    };
    
    var atomsvc = console.atomService;

    this.atomCurrent        = atomsvc.getAtom("current-line");
    this.atomHighlightStart = atomsvc.getAtom("highlight-start");
    this.atomHighlightRange = atomsvc.getAtom("highlight-range");
    this.atomHighlightEnd   = atomsvc.getAtom("highlight-end");
    this.atomBreak          = atomsvc.getAtom("breakpoint");
    this.atomFBreak         = atomsvc.getAtom("future-breakpoint");
    this.atomCode           = atomsvc.getAtom("code");
    this.atomPrettyPrint    = atomsvc.getAtom("prettyprint");
    this.atomWhitespace     = atomsvc.getAtom("whitespace");
}

console.views.source.hooks = new Object();

console.views.source.hooks["hook-break-set"] =
console.views.source.hooks["hook-break-clear"] =
console.views.source.hooks["hook-fbreak-set"] =
console.views.source.hooks["hook-fbreak-clear"] =
function sv_hookBreakChange(e)
{
    if (console.views.source.tree)
        console.views.source.tree.invalidate();
}

console.views.source.hooks["hook-debug-continue"] =
function sv_hookDebugCont (e)
{
    /* Invalidate on continue to remove the highlight line. */
    if (console.views.source.tree)
        console.views.source.tree.invalidate();
}

/**
 * Display requested sourcetext.
 */
console.views.source.hooks["hook-display-sourcetext"] =
console.views.source.hooks["hook-display-sourcetext-soft"] =
function sv_hookDisplay (e)
{
    var sourceView = console.views.source;
    sourceView.details = e.details;
    sourceView.displaySourceText(e.sourceText, Boolean(e.targetLine));
    sourceView.rangeStart = e.rangeStart - 1;
    sourceView.rangeEnd   = e.rangeEnd - 1;
    var targetLine;
    if (e.targetLine == null)
        targetLine = sourceView.rangeStart;
    else
        targetLine = e.targetLine - 1;

    if (sourceView.tree)
    {
        if (e.targetLine && e.command.name == "hook-display-sourcetext-soft")
        {
            sourceView.softScrollTo (targetLine);
        }
        else if (sourceView.rangeEnd && 
                 (targetLine > e.rangeStart && targetLine <= e.rangeEnd) ||
                 (e.rangeStart == e.rangeEnd))
        {
            /* if there is a range, and the target is in the range,
             * scroll target to the center */
            sourceView.scrollTo (targetLine, 0);
        }
        else
        {
            /* otherwise scroll near the top */
            sourceView.scrollTo (targetLine - 2, -1);
        }
    }
    else
    {
        var url = e.sourceText.url;
        if (!(url in sourceView.savedState))
            sourceView.savedState[url] = new Object();
        sourceView.savedState[url].topLine = targetLine;
    }   

    console.views.source.currentLine = targetLine + 1;
    console.views.source.syncHeader();
}

/**
 * Sync with the pretty print state as it changes.
 */
console.views.source.hooks["hook-source-load-complete"] =
function sv_hookLoadComplete (e)
{
    console.views.source.syncTreeView();
}

console.views.source.hooks["pprint"] =
function sv_hookLoadComplete (e)
{
    console.views.source.prettyPrint = e.toggle;
}

console.views.source.onShow =
function sv_show ()
{
    var sourceView = this;
    function cb ()
    {
        var sourceText = sourceView.childData;
        if (sourceText && sourceText.url in sourceView.savedState)
        {
            dd ("clearing lastRowCount");
            delete sourceView.savedState[sourceText.url].lastRowCount;
        }
        sourceView.syncTreeView();
        sourceView.urlLabel = getChildById(sourceView.currentContent,
                                           "source-url");
        sourceView.syncHeader();
    };
    
    syncTreeView (getChildById(this.currentContent, "source-tree"), this, cb);
    initContextMenu(this.currentContent.ownerDocument, "context:source");
}

console.views.source.onHide =
function sv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "source-tree"), null);
    delete this.urlLabel;
}

console.views.source.onClick =
function sv_click (e)
{
    var target = e.originalTarget;
    
    if (target.localName == "treechildren")
    {
        var row = new Object();
        var colID = new Object();
        var childElt = new Object();
        
        var treeBox = console.views.source.tree;
        treeBox.getCellAt(e.clientX, e.clientY, row, colID, childElt);
        if (row.value == -1)
          return;
        
        colID = colID.value;
        row = row.value;
        
        if (colID == "source:col-0")
        {
            if ("onMarginClick" in console.views.source.childData)
                console.views.source.childData.onMarginClick (e, row + 1);
        }
    }

}

console.views.source.onSelect =
function sv_select (e)
{
    var sourceView = console.views.source;
    sourceView.currentLine = sourceView.tree.selection.currentIndex + 1;
    console.views.source.syncHeader();
}

console.views.source.super_scrollTo = BasicOView.prototype.scrollTo;

console.views.source.scrollTo =
function sv_scrollto (line, align)
{
    if (!("childData" in this))
        return;

    if (!this.childData.isLoaded)
    {
        /* the source hasn't been loaded yet, store line/align for processing
         * when the load is done. */
        this.childData.pendingScroll = line;
        this.childData.pendingScrollType = align;
        return;
    }
    this.super_scrollTo(line, align);

    if (this.tree)
        this.tree.invalidate();
}

console.views.source.syncHeader =
function sv_synch ()
{
    if ("urlLabel" in this)
    {
        var value;
        if ("childData" in this)
        {
            value = getMsg(MSN_SOURCEHEADER_URL,
                           [this.childData.url, this.currentLine]);
        }
        else
        {
            value = MSG_VAL_NONE;
        }

        this.urlLabel.setAttribute ("value", value);    
    }
}
    
console.views.source.syncTreeView =
function sv_synct (skipScrollRestore)
{    
    if (this.tree)
    {
        if (!("childData" in this) || !this.childData)
        {
            this.rowCount = 0;
            this.tree.rowCountChanged (0, 0);
            this.tree.invalidate();
            return;
        };

        var url = this.childData.url;
        var state;
        if (url in this.savedState)
            state = this.savedState[url];
        else
        {
            //dd ("making new state");
            state = this.savedState[url] = new Object();
        }

        if (!("lastRowCount" in state) || state.lastRowCount != this.rowCount)
        {
            //dd ("notifying new row count " + this.rowCount);
            this.tree.rowCountChanged(0, this.rowCount);
        }

        state.lastRowCount = this.rowCount;

        if (!skipScrollRestore && "topLine" in state)
            this.scrollTo (state.topLine, 1);

        this.tree.invalidate();
        delete state.topLine;
    }
}

/*
 * pass in a SourceText to be displayed on this tree
 */
console.views.source.displaySourceText =
function sv_dsource (sourceText, skipScrollRestore)
{
    var sourceView = this;

    if (!sourceText)
    {
        delete this.childData;
        this.rowCount = 0;
        this.syncTreeView();
        return;
    }

    if (!ASSERT(sourceText.isLoaded,
                "Source text for '" + sourceText.url + "' has not been loaded."))
    {
        return;
    }
    
    if ("childData" in this && sourceText == this.childData)
        return;
    
    /* save the current position before we change to another source */
    if ("childData" in this && this.tree)
    {
        this.savedState[this.childData.url].topLine = 
            this.tree.getFirstVisibleRow() + 1;
    }
    
    
    this.childData = sourceText;
    this.rowCount = sourceText.lines.length;
    this.tabString = leftPadString ("", sourceText.tabWidth, " ");
    //var hdr = document.getElementById("source-line-text");
    //hdr.setAttribute ("label", sourceText.fileName);

    this.syncTreeView(skipScrollRestore);
}

/*
 * "soft" scroll to a line number in the current source.  soft, in this
 * case, means that if the target line somewhere in the center of the
 * source view already, then we can just exit.  otherwise, we'll center on the
 * target line.  this is used when single stepping through source, when constant
 * one-line scrolls would be distracting.
 *
 * the line parameter is one based.
 */
console.views.source.softScrollTo =
function sv_lscroll (line)
{
    if (!("childData" in this))
        return;
    
    if (!this.childData.isLoaded)
    {
        /* the source hasn't been loaded yet, queue the scroll for later. */
        this.childData.pendingScroll = line;
        this.childData.pendingScrollType = 0;
        return;
    }

    delete this.childData.pendingScroll;
    delete this.childData.pendingScrollType;

    var first = this.tree.getFirstVisibleRow();
    var last = this.tree.getLastVisibleRow();
    var fuzz = 2;
    if (line < (first + fuzz) || line > (last - fuzz))
        this.scrollTo (line - 2, -1);
    else
        this.tree.invalidate(); /* invalidate to show the new currentLine if
                                 * we don't have to scroll. */
}    

/**
 * Create a context object for use in the sourceView context menu.
 */
console.views.source.getContext =
function sv_getcx(cx)
{
    if (!cx)
        cx = new Object();

    var sourceText = console.views.source.childData;
    cx.url = cx.urlPattern = sourceText.url;
    cx.breakWrapperList = new Array();
    cx.lineNumberList = new Array();
    cx.lineIsExecutable = null;

    function rowContextGetter (cx, row, i)
    {
        if (!("lineMap" in sourceText))
            return cx;
        
        if (i == 0)
        {
            cx.lineNumber = row + 1;
            
            if (sourceText.lineMap[row] & LINE_BREAKABLE)
                cx.lineIsExecutable = true;
            if (sourceText.lineMap[row] & LINE_BREAK)
            {
                cx.hasBreak = true;
                cx.breakWrapper =
                    sourceText.scriptInstance.getBreakpoint(row + 1);
                if (cx.breakWrapper.parentBP)
                    cx.hasFBreak = true;
            }
            else if (sourceText.lineMap[row] & LINE_FBREAK)
            {
                cx.hasFBreak = true;
                cx.breakWrapper = getFutureBreakpoint(row + 1);
            }
        }
        else
        {
            if (sourceText.lineMap[row] & LINE_BREAK)
            {
                cx.hasBreak = true;
                var wrapper = sourceText.scriptInstance.getBreakpoint(row + 1);
                if (wrapper.parentBP)
                    cx.hasFBreak = true;
                cx.breakWrapperList.push(wrapper);
            }
            else if (sourceText.lineMap[row] & LINE_FBREAK)
            {
                cx.hasFBreak = true;
                cx.breakWrapperList.push(getFutureBreakpoint(row + 1));
            }
        }
        
        return cx;
    };
    
    getTreeContext (console.views.source, cx, rowContextGetter);

    return cx;
}    

/* nsITreeView */
console.views.source.getRowProperties =
function sv_rowprops (row, properties)
{
    var prettyPrint = console.prefs["prettyprint"];
    
    if ("frames" in console)
    {
        if (((!prettyPrint && row == console.stopLine - 1) ||
             (prettyPrint && row == console.pp_stopLine - 1)) &&
            console.stopFile == this.childData.url)
        {
            properties.AppendElement(this.atomCurrent);
        }
    }
}

/* nsITreeView */
console.views.source.getCellProperties =
function sv_cellprops (row, colID, properties)
{
    if (!("childData" in this) || !this.childData.isLoaded ||
        row < 0 || row >= this.childData.lines.length)
        return;

    var line = this.childData.lines[row];
    if (!line)
        return;
    
    if (colID == "source:col-0")
    {
        if ("lineMap" in this.childData && row in this.childData.lineMap)
        {
            var flags = this.childData.lineMap[row];
            if (flags & LINE_BREAK)
                properties.AppendElement(this.atomBreak);
            if (flags & LINE_FBREAK)
                properties.AppendElement(this.atomFBreak);
            if (flags & LINE_BREAKABLE)
                properties.AppendElement(this.atomCode);
        }
    }
    
    if (this.rangeEnd != null)
    {
        if (row == this.rangeStart)
        {
            properties.AppendElement(this.atomHighlightStart);
        }
        else if (row == this.rangeEnd)
        {
            properties.AppendElement(this.atomHighlightEnd);
        }
        else if (row > this.rangeStart && row < this.rangeEnd)
        {
            properties.AppendElement(this.atomHighlightRange);
        }        
    }

    if ("frames" in console)
    {
        if (((!this.prettyPrint && row == console.stopLine - 1) ||
             (this.prettyPrint && row == console.pp_stopLine - 1)) &&
            console.stopFile == this.childData.url)
        {
            properties.AppendElement(this.atomCurrent);
        }
    }
}

/* nsITreeView */
console.views.source.getCellText =
function sv_getcelltext (row, colID)
{    
    if (!this.childData.isLoaded || 
        row < 0 || row > this.childData.lines.length)
        return "";
    
    var ary = colID.match (/:(.*)/);
    if (ary)
        colID = ary[1];
    
    switch (colID)
    {
        case "col-2":
            return this.childData.lines[row].replace(/\t/g, this.tabString);

        case "col-1":
            return row + 1;
            
        default:
            return "";
    }
}

/*******************************************************************************
 * Watch View
 *******************************************************************************/

console.views.watches = new XULTreeView();

console.views.watches.viewId = "watches";

console.views.watches.init =
function wv_init()
{
    console.menuSpecs["context:watches"] = {
        getContext: this.getContext,
        items:
        [
         ["find-creator",
                 {enabledif: "cx.target instanceof ValueRecord && " +
                  "cx.target.jsType == jsdIValue.TYPE_OBJECT && " +
                  "cx.target.value.objectValue.creatorURL"}],
         ["find-ctor",
                 {enabledif: "cx.target instanceof ValueRecord && " +
                  "cx.target.jsType == jsdIValue.TYPE_OBJECT && " +
                  "cx.target.value.objectValue.constructorURL"}]
        ]
    };
}

console.views.watches.onShow =
function wv_show()
{
    syncTreeView (getChildById(this.currentContent, "watch-tree"), this);
    initContextMenu(this.currentContent.ownerDocument, "context:watches");
}

console.views.watches.onHide =
function onHide()
{
    syncTreeView (getChildById(this.currentContent, "watch-tree"), null);
}

console.views.watches.getCellProperties =
function wv_cellprops (index, colID, properties)
{
    if (colID != "watches:col-0")
        return null;
    
    var row = this.childData.locateChildByVisualRow(index);
    if (row)
    {
        if ("getProperties" in row)
            return row.getProperties (properties);

        if (row.property)
            return properties.AppendElement (row.property);
    }

    return null;
}

console.views.watches.getContext =
function wv_getcx(cx)
{
    cx.jsdValueList = new Array();

    function recordContextGetter (cx, rec, i)
    {
        if (rec instanceof ValueRecord)
        {
            if (i == 0)
                cx.jsdValue = rec.value;
            else
                cx.jsdValueList.push(rec.value);
        }
    };
    
    return getTreeContext (console.views.watches, cx, recordContextGetter);
}

console.views.watches.refresh =
function wv_refresh()
{
    var delta = 0;
    
    for (var i = 0; i < this.childData.length; ++i)
    {
        var item = this.childData[i];
        var thisDelta = 0;
        for (var j = 0; j < item.childData.length; ++j)
            thisDelta += item.childData[j].refresh();
        /* if the container isn't open, we still have to update the children,
         * but we don't care about any visual footprint changes */
        if (item.isContainerOpen)
        {
            item.visualFootprint += thisDelta;
            delta += thisDelta;
        }
    }

    this.childData.visualFootprint += delta;
    this.childData.invalidateCache();
    this.tree.rowCountChanged (0, this.visualFootprint);
    this.tree.invalidate();
}

console.views.watches.onDblClick =
function wv_dblclick ()
{
    dd ("locals double click");
}

/*******************************************************************************
 * Windows View
 *******************************************************************************/

console.views.windows = new XULTreeView();

console.views.windows.viewId = "windows";

console.views.windows.init = 
function winv_init ()
{
}

console.views.windows.hooks = new Object();

console.views.windows.hooks["hook-window-opened"] =
function winv_hookWindowOpened (e)
{
    console.views.windows.childData.appendChild (new WindowRecord(e.window, ""));
}

console.views.windows.hooks["hook-window-closed"] =
function winv_hookWindowClosed (e)
{
    var winRecord = console.views.windows.locateChildByWindow(e.window);
    if (!ASSERT(winRecord, "Can't find window record for closed window."))
        return;
    console.views.windows.childData.removeChildAtIndex(winRecord.childIndex);
}

console.views.windows.onShow =
function winv_show ()
{
    syncTreeView (getChildById(this.currentContent, "windows-tree"), this);
}

console.views.windows.onHide =
function winv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "windows-tree"), null);
}

console.views.windows.getCellProperties =
function winv_cellprops (index, colID, properties)
{
    if (colID == "windows:col-0")
    {
        var row = this.childData.locateChildByVisualRow(index);
        if (row)
            properties.AppendElement (row.property);
    }

    return;
}

console.views.windows.onDblClick =
function winv_dblclick ()
{
    var rowIndex = this.tree.selection.currentIndex;
    if (rowIndex == -1 || rowIndex > this.rowCount)
        return;
    var row = this.childData.locateChildByVisualRow(rowIndex);
    if (!row)
    {
        ASSERT (0, "bogus row index " + rowIndex);
        return;
    }

    if ("url" in row)
        dispatch ("find-url", { url: row.url });
                  
}

console.views.windows.getContext =
function winv_getcx(cx)
{
    if (!cx)
        cx = new Object();

    var selection = this.tree.selection;

    var rec = this.childData.locateChildByVisualRow(selection.currentIndex);
    
    if (!rec)
    {
        dd ("no current index.");
        return cx;
    }

    cx.target = rec;
    
    if (rec instanceof WindowRecord || rec instanceof FileRecord)
        cx.url = cx.fileName = rec.url;
    
    var rangeCount = selection.getRangeCount();
    if (rangeCount > 0)
        cx.fileList = cx.urlList = new Array();
    
    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        selection.getRangeAt(range, min, max);
        for (var i = min.value; i <= max.value; ++i)
        {
            rec = this.childData.locateChildByVisualRow(i);
            if (rec instanceof WindowRecord || rec instanceof FileRecord)
                cx.fileList.push (rec.url);
        }
    }

    return cx;
}

console.views.windows.locateChildByWindow =
function winv_find (win)
{
    var children = this.childData.childData;
    for (var i = 0; i < children.length; ++i)
    {
        var child = children[i];
        if (child.window == win)
            return child;
    }

    return null;
}

/************************/

function formatRecord (rec, indent)
{
    var str = "";
    
    for (var i in rec._colValues)
        str += rec._colValues[i] + ", ";
    
    str += "[";
    
    str += rec.calculateVisualRow() + ", ";
    str += rec.childIndex + ", ";
    str += rec.level + ", ";
    str += rec.visualFootprint + ", ";
    str += rec.isContainerOpen + ", ";
    str += rec.isHidden + "]";
    
    dd (indent + str);
}

function formatBranch (rec, indent)
{
    for (var i = 0; i < rec.childData.length; ++i)
    {
        formatRecord (rec.childData[i], indent);
        if (rec.childData[i].childData)
            formatBranch(rec.childData[i], indent + "  ");
    }
}
