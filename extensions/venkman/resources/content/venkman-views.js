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
            console.views[v].init();
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
            syncTreeView(treeContent, treeView);
        }
        else
        {
            dd ("trying to sync " + treeContent.getAttribute("id") + " AGAIN");
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

console.realizeView =
function con_realizeview (view, key)
{
    console.views[key] = view;
    console.views[key].init();
}

console.views = new Object();

/*******************************************************************************
 * Breakpoints View
 *******************************************************************************/

var breaksShare = new Object();

console.views.breaks = new XULTreeView(breaksShare);

console.views.breaks.vewId = "breaks";

console.views.breaks.init =
function bv_init ()
{
    this.atomBreakpoint = console.atomService.getAtom("item-breakpoint");
}

console.views.breaks.onShow =
function bv_show()
{
    syncTreeView (getChildById(this.currentContent, "break-tree"), this);
}

console.views.breaks.onHide =
function bv_hide()
{
    syncTreeView (getChildById(this.currentContent, "break-tree"), null);
}

console.views.breaks.onSelect =
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
        dispatch ("find-bp", {breakpointRec: row});
}

console.views.breaks.getContext =
function bv_getcx(cx)
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
    
    if (rec instanceof BPRecord)
    {
        cx.breakpointRec = rec;
        cx.url = cx.fileName = rec.fileName;
        cx.lineNumber = rec.line;
        cx.breakpointIndex = rec.childIndex;
    }
    
    var rangeCount = this.tree.selection.getRangeCount();
    if (rangeCount > 0)
    {
        cx.breakpointRecList = new Array();
        cx.breakpointIndexList = new Array();
        cx.fileList = cx.urlList = new Array();
    }
    
    for (var range = 0; range < rangeCount; ++range)
    {
        var min = new Object();
        var max = new Object();
        this.tree.selection.getRangeAt(range, min, max);
        min = min.value;
        max = max.value;
        for (var i = min; i <= max; ++i)
        {
            rec = this.childData.locateChildByVisualRow(i);
            if (rec instanceof BPRecord)
            {
                cx.breakpointRecList.push(rec);
                cx.breakpointIndexList.push(rec.childIndex);
                cx.fileList.push (rec.fileName);
            }
        }
    }

    return cx;
}

console.views.breaks.getCellProperties =
function bv_cellprops (index, colID, properties)
{
    if (colID == "col-0")
    {
        if (this.childData.locateChildByVisualRow(index))
            properties.AppendElement (this.atomBreakpoint);
    }
}

console.views.breaks.locateChildByFileLine =
function bv_findfl (fileName, line)
{
    for (var i = 0; i < this.childData.length; ++i)
    {
        var child = this.childData[i];
        if (child.line == line &&
            child.fileName == fileName)
            return child;
    }

    return null;
}

function BPRecord (fileName, line)
{
    var record = this;
    function getMatchLength ()
    {
        return record.scriptRecords.length;
    }
        
    this.scriptRecords = new Array();
    this.fileName = fileName;
    this._enabled = true;
    this.stop = true;

    this.setColumnPropertyName ("col-0", "shortName");
    this.setColumnPropertyName ("col-2", "functionName");
    this.setColumnPropertyName ("col-1", "line");
    this.setColumnPropertyName ("col-3", getMatchLength);

    var ary = fileName.match(/\/([^\/?]+)(\?|$)/);
    if (ary)
        this.shortName = ary[1];
    else
        this.shortName = fileName;
    this.line = line;
    this.functionName = MSG_VAL_UNKNOWN;
}

BPRecord.prototype = new XULTreeViewRecord(breaksShare);

BPRecord.prototype.__defineGetter__ ("scriptMatches", bpr_getmatches);
function bpr_getmatches ()
{
    return this.scriptRecords.length;
}

BPRecord.prototype.__defineGetter__ ("enabled", bpr_getenabled);
function bpr_getenabled ()
{
    return this._enabled;
}

BPRecord.prototype.__defineSetter__ ("enabled", bpr_setenabled);
function bpr_setenabled (state)
{
    if (state == this._enabled)
        return;
    
    var delta = (state) ? +1 : -1;
    
    for (var i = 0; i < this.scriptRecords.length; ++i)
    {
        this.scriptRecords[i].bpcount += delta;
        var script = this.scriptRecords[i].script;
        var pc = script.lineToPc(this.line, PCMAP_SOURCETEXT);
        if (state)
            script.setBreakpoint(pc);
        else
            script.clearBreakpoint(pc);
    }
    this._enabled = state;
}

BPRecord.prototype.matchesScriptRecord =
function bpr_matchrec (scriptRec)
{
    return (scriptRec.script.fileName.indexOf(this.fileName) != -1 &&
            scriptRec.containsLine(this.line) &&
            scriptRec.script.isLineExecutable(this.line, PCMAP_SOURCETEXT));
}

BPRecord.prototype.addScriptRecord =
function bpr_addscript (scriptRec)
{
    for (var i = 0; i < this.scriptRecords.length; ++i)
        if (this.scriptRecords[i] == scriptRec)
            return;

    if (this._enabled)
    {
        var pc = scriptRec.script.lineToPc(this.line, PCMAP_SOURCETEXT);
        scriptRec.script.setBreakpoint(pc);
    }
    
    this.functionName = scriptRec.functionName;
    ++(scriptRec.bpcount);
    
    this.scriptRecords.push(scriptRec);
}

BPRecord.prototype.removeScriptRecord =
function bpr_remscript (scriptRec)
{
    for (var i = 0; i < this.scriptRecords.length; ++i)
        if (this.scriptRecords[i] == scriptRec)
        {
            --(this.scriptRecords[i].bpcount);
            arrayRemoveAt(this.scriptRecords, i);
            return;
        }
}

BPRecord.prototype.hasScriptRecord =
function bpr_addscript (scriptRec)
{
    for (var i = 0; i < this.scriptRecords.length; ++i)
        if (this.scriptRecords[i] == scriptRec)
            return true;

    return false;
}

/*******************************************************************************
 * Locals View
 *******************************************************************************/
var localsShare = new Object();

console.views.locals = new XULTreeView(localsShare);

console.views.locals.viewId = "locals";

console.views.locals.init =
function lv_init ()
{
    var atomsvc = console.atomService;
    
    ValueRecord.prototype.atomVoid     = atomsvc.getAtom("item-void");
    ValueRecord.prototype.atomNull     = atomsvc.getAtom("item-null");
    ValueRecord.prototype.atomBool     = atomsvc.getAtom("item-bool");
    ValueRecord.prototype.atomInt      = atomsvc.getAtom("item-int");
    ValueRecord.prototype.atomDouble   = atomsvc.getAtom("item-double");
    ValueRecord.prototype.atomString   = atomsvc.getAtom("item-string");
    ValueRecord.prototype.atomFunction = atomsvc.getAtom("item-function");
    ValueRecord.prototype.atomObject   = atomsvc.getAtom("item-object");
}

console.views.locals.onShow =
function lv_show ()
{
    syncTreeView (getChildById(this.currentContent, "locals-tree"), this);
}

console.views.locals.onHide =
function lv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "locals-tree"), null);
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

function ValueRecord (value, name, flags)
{
    if (!(value instanceof jsdIValue))
        throw new BadMojo (ERR_INVALID_PARAM, "value", String(value));

    this.setColumnPropertyName ("col-0", "displayName");
    this.setColumnPropertyName ("col-1", "displayType");
    this.setColumnPropertyName ("col-2", "displayValue");
    this.setColumnPropertyName ("col-3", "displayFlags");    
    this.displayName = name;
    this.displayFlags = flags;
    this.value = value;
    this.jsType = null;
    this.refresh();
}

ValueRecord.prototype = new XULTreeViewRecord (null);

ValueRecord.prototype.__defineGetter__("_share", vr_getshare);
function vr_getshare()
{
    if ("__share" in this)
        return this.__share;
     
    if ("parentRecord" in this)
        return this.__share = this.parentRecord._share;
 
    ASSERT (0, "ValueRecord cannot be the root of a visible tree.");
    return null;
}

ValueRecord.prototype.hiddenFunctionCount = 0;
ValueRecord.prototype.showFunctions = false;

ValueRecord.prototype.resort =
function cr_resort()
{
    /*
     * we want to override the prototype's resort() method with this empty one
     * because we take care of the sorting business ourselves in onPreOpen()
     */
}

ValueRecord.prototype.refresh =
function vr_refresh ()
{
    if ("onPreRefresh" in this)
        this.onPreRefresh();    

    var sizeDelta = 0;
    var lastType = this.jsType;
    this.jsType = this.value.jsType;
    
    if (0 && lastType != this.jsType && lastType == jsdIValue.TYPE_FUNCTION)
    {
        /* we changed from a non-function to a function */
        --this.hiddenFunctionCount;
        ++sizeDelta;
    }
    
    if (this.jsType != jsdIValue.TYPE_OBJECT && "childData" in this)
    {
        /* if we're not an object but we have child data, then we must have just
         * turned into something other than an object. */
        delete this.childData;
        this.isContainerOpen = false;
        sizeDelta = 1 - this.visualFootprint;
    }
    
    switch (this.jsType)
    {
        case jsdIValue.TYPE_VOID:
            this.displayValue = MSG_TYPE_VOID
            this.displayType  = MSG_TYPE_VOID;
            this.property     = this.atomVoid;
            break;
        case jsdIValue.TYPE_NULL:
            this.displayValue = MSG_TYPE_NULL;
            this.displayType  = MSG_TYPE_NULL;
            this.property     = this.atomNull;
            break;
        case jsdIValue.TYPE_BOOLEAN:
            this.displayValue = this.value.stringValue;
            this.displayType  = MSG_TYPE_BOOLEAN;
            this.property     = this.atomBool;
            break;
        case jsdIValue.TYPE_INT:
            this.displayValue = this.value.intValue;
            this.displayType  = MSG_TYPE_INT;
            this.property     = this.atomInt;
            break;
        case jsdIValue.TYPE_DOUBLE:
            this.displayValue = this.value.doubleValue;
            this.displayType  = MSG_TYPE_DOUBLE;
            this.property     = this.atomDouble;
            break;
        case jsdIValue.TYPE_STRING:
            var strval = this.value.stringValue.quote();
            if (strval.length > MAX_STR_LEN)
                strval = getMsg(MSN_FMT_LONGSTR, strval.length);
            this.displayValue = strval;
            this.displayType  = MSG_TYPE_STRING;
            this.property     = this.atomString;
            break;
        case jsdIValue.TYPE_FUNCTION:
            this.displayType  = MSG_TYPE_FUNCTION;
            this.displayValue = (this.value.isNative) ? MSG_WORD_NATIVE :
                MSG_WORD_SCRIPT;
            this.property = this.atomFunction;
            break;
        case jsdIValue.TYPE_OBJECT:
            this.value.refresh();
            var ctor = this.value.jsClassName;
            if (ctor == "Object")
            {
                if (this.value.jsConstructor)
                    ctor = this.value.jsConstructor.jsFunctionName;
            }
            else if (ctor == "XPCWrappedNative_NoHelper")
            {
                ctor = MSG_CLASS_XPCOBJ;
            }

            this.displayValue = "{" + ctor + ":" + this.value.propertyCount +
                "}";

            this.displayType = MSG_TYPE_OBJECT;
            this.property = this.atomObject;
            /* if we had children, and were open before, then we need to descend
             * and refresh our children. */
            if ("childData" in this && this.childData.length > 0)
            {
                var rc = 0;
                rc = this.refreshChildren();
                sizeDelta += rc;
                //dd ("refreshChildren returned " + rc);
                this.visualFootprint += rc;
            }
            else
            {
                this.childData = new Array();
                this.isContainerOpen = false;
            }
            break;
            

        default:
            ASSERT (0, "invalid value");
    }

    //dd ("refresh returning " + sizeDelta);
    return sizeDelta;

}

ValueRecord.prototype.refreshChildren =
function vr_refreshkids ()
{
    /* XXX add identity check to see if we are a totally different object */
    /* if we now have more properties than we used to, we're going to have
     * to close any children we may have open, because we can't tell where the
     * new property is in any efficient way. */
    if (this.value.propertyCount > this.lastPropertyCount)
    {
        this.onPreOpen();
        return (this.childData.length + 1) - this.visualFootprint;
    }

    /* otherwise, we had children before.  we've got to update each of them
     * in turn. */
    var sizeDelta    = 0; /* total change in size */
    var idx          = 0; /* the new position of the child in childData */
    var deleteCount  = 0; /* number of children we've lost */
    var specialProps = 0; /* number of special properties in this object */

    for (var i = 0; i < this.childData.length; ++i)
    {
        //dd ("refreshing child #" + i);
        var name = this.childData[i]._colValues["col-0"];
        var value;
        switch (name)
        {
            case MSG_VAL_PARENT:
                /* "special" property, doesn't actually exist
                 * on the object */
                value = this.value.jsParent;
                specialProps++;
                break;
            case MSG_VAL_PROTO:
                /* "special" property, doesn't actually exist
                 * on the object */
                value = this.value.jsPrototype;
                specialProps++;
                break;
            default:
                var prop = this.value.getProperty(name);
                if (prop)
                    value = prop.value;
                break;
        }
        
        if (value)
        {
            if (this.showFunctions || value.jsType != jsdIValue.TYPE_FUNCTION)
            {
                /* if this property still has a value, sync it in its (possibly)
                 * new position in the childData array, and refresh it */
                this.childData[idx] = this.childData[i];
                this.childData[idx].childIndex = idx;
                this.childData[idx].value = value;
                sizeDelta += this.childData[idx].refresh();
                ++idx;
                value = null;
            }
            else
            {
                /* if we changed from a non-function to a function, and we're in
                 * "hide function" mode, we need to consider this child deleted
                 */
                ++this.hiddenFunctionCount;
                ++deleteCount;
                sizeDelta -= this.childData[i].visualFootprint;
            }
        }
        else
        {
            /* if the property isn't here anymore, make a note of
             * it */
            ++deleteCount;
            sizeDelta -= this.childData[i].visualFootprint;
        }
    }
    
    /* if we've deleted some kids, adjust the length of childData to
     * match */
    if (deleteCount != 0)
        this.childData.length -= deleteCount;
    
    if ((this.childData.length + this.hiddenFunctionCount - specialProps) !=
        this.value.propertyCount)
    {
        /* if the two lengths *don't* match, then we *must* be in
         * a state where the user added and deleted the same
         * number of properties.  if this is the case, then
         * everything we just did was a totally
         * useless waste of time.  throw it out and re-init
         * whatever children we have.  see the "THESE COMMENTS"
         * comments above for the description of what we're doing
         * here. */
        this.onPreOpen();
        sizeDelta = (this.childData.length + 1) - this.visualFootprint;
    }

    return sizeDelta;
}

ValueRecord.prototype.onPreOpen =
function vr_create()
{
    if (this.value.jsType != jsdIValue.TYPE_OBJECT)
        return;
    
    function vr_compare (a, b)
    {
        aType = a.value.jsType;
        bType = b.value.jsType;
        
        if (aType < bType)
            return -1;
        
        if (aType > bType)
            return 1;
        
        aVal = a.displayName;
        bVal = b.displayName;
        
        if (aVal < bVal)
            return -1;
        
        if (aVal > bVal)
            return 1;
        
        return 0;
    }
    
    this.childData = new Array();
    
    var p = new Object();
    this.value.getProperties (p, {});
    this.lastPropertyCount = p.value.length;
    /* we'll end up with the 0 from the prototype */
    delete this.hiddenFunctionCount;
    for (var i = 0; i < p.value.length; ++i)
    {
        var prop = p.value[i];
        if (this.showFunctions ||
            prop.value.jsType != jsdIValue.TYPE_FUNCTION)
        {
            this.childData.push(new ValueRecord(prop.value,
                                                prop.name.stringValue,
                                                formatFlags(prop.flags)));
        }
        else
        {
            ++this.hiddenFunctionCount;
        }
    }

    this.childData.sort (vr_compare);

    if (this.value.jsPrototype)
        this.childData.unshift (new ValueRecord(this.value.jsPrototype,
                                                MSG_VAL_PROTO));

    if (this.value.jsParent)
        this.childData.unshift (new ValueRecord(this.value.jsParent,
                                                MSG_VAL_PARENT));
    
    for (i = 0; i < this.childData.length; ++i)
    {
        var cd = this.childData[i];
        cd.parentRecord = this;
        cd.childIndex = i;
        cd.isHidden = false;
    }
}

ValueRecord.prototype.onPostClose =
function vr_destroy()
{
    this.childData = new Array();
}

/*******************************************************************************
 * Scripts View
 *******************************************************************************/

var scriptShare = new Object();

console.views.scripts = new XULTreeView (scriptShare);

console.views.scripts.viewId = "scripts";

console.views.scripts.init =
function scv_init ()
{
    this.childData.setSortColumn("baseLineNumber");
    this.groupFiles = true;

    var atomsvc = console.atomService;
    this.atomUnknown    = atomsvc.getAtom("item-unk");
    this.atomHTML       = atomsvc.getAtom("item-html");
    this.atomJS         = atomsvc.getAtom("item-js");
    this.atomXUL        = atomsvc.getAtom("item-xul");
    this.atomXML        = atomsvc.getAtom("item-xml");
    this.atomGuessed    = atomsvc.getAtom("item-guessed");
    this.atomBreakpoint = atomsvc.getAtom("item-has-bp");

    var hooks =
    [
     ["chrome-filter",                        scv_hookChromeFilter,
      "scripts:chrome-filter-hook",           false]
    ];

    console.commandManager.hookCommands(hooks);
}

function scv_hookChromeFilter(e)
{
    console.views.scripts.freeze();
    for (var container in console.scripts)
    {
        if (console.scripts[container].fileName.indexOf("chrome:") == 0)
        {
            var rec = console.scripts[container];
            var scriptList = console.views.scripts.childData;
            if (e.toggle)
            {
                /* filter is on, remove chrome file from scripts view */
                if ("parentRecord" in rec)
                {
                    //dd ("remove index " + rec.childIndex + ", " +
                    //    rec.fileName);
                    scriptList.removeChildAtIndex(rec.childIndex);
                }
                else
                    dd ("record already seems to out of the tree");
            }
            else
            {
                /* filter is off, add chrome file to scripts view */
                if (!("parentRecord" in rec))
                {
                    //dd ("cmdChromeFilter: append " +
                    //    tov_formatRecord(rec, ""));
                    scriptList.appendChild(rec);
                }
                else
                    dd ("record already seems to be in the tree");
            }
        }
    }
    console.views.scripts.thaw();
}

console.views.scripts.onShow =
function scv_show ()
{
    syncTreeView (getChildById(this.currentContent, "scripts-tree"), this);
}

console.views.scripts.onHide =
function scv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "scripts-tree"), null);
}

console.views.scripts.onSelect =
function scv_select (e)
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
        dispatch ("find-script", {scriptRec: row});
    else if (row instanceof ScriptContainerRecord)
        dispatch ("find-url", {url: row.fileName});
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
            case "col=0":
                prop = "functionName";
                break;
            case "col-1":
                prop = "baseLineNumber";
                break;
            case "col-2":
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
    var row;
    if ((row = this.childData.locateChildByVisualRow (index, 0)))
    {
        if ("fileType" in row && colID == "scripts:col-0")
            properties.AppendElement (row.fileType);
        if ("isGuessedName" in row && colID == "scripts:col-0")
            properties.AppendElement (this.atomGuessed);
        if (row.bpcount > 0)
            properties.AppendElement (this.atomBreakpoint);
    }
}

console.views.scripts.getContext =
function scv_getcx(cx)
{
    if (!cx)
        cx = new Object();

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
    
    if (rec instanceof ScriptContainerRecord)
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
            if (rec instanceof ScriptContainerRecord)
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

function ScriptContainerRecord(fileName)
{
    this.setColumnPropertyName ("col-0", "displayName");
    this.setColumnPropertyValue ("col-1", "");
    this.setColumnPropertyValue ("col-2", "");
    this.fileName = fileName;
    var sv = console.views.scripts;
    this.fileType = sv.atomUnknown;
    this.shortName = this.fileName;
    this.group = 4;
    this.bpcount = 0;

    this.shortName = getFileFromPath(this.fileName);
    ary = this.shortName.match (/\.(js|html|xul|xml)$/i);
    if (ary)
    {
        switch (ary[1].toLowerCase())
        {
        case "js":
            this.fileType = sv.atomJS;
            this.group = 0;
            break;
            
        case "html":
            this.group = 1;
            this.fileType = sv.atomHTML;
            break;
            
        case "xul":
            this.group = 2;
            this.fileType = sv.atomXUL;
            break;
            
        case "xml":
            this.group = 3;
            this.fileType = sv.atomXML;
            break;
        }
    }
    
    this.displayName = this.shortName;
}

ScriptContainerRecord.prototype = new XULTreeViewRecord(scriptShare);

ScriptContainerRecord.prototype.onDragStart =
function scr_dragstart (e, transferData, dragAction)
{        
    transferData.data = new TransferData();
    transferData.data.addDataForFlavour("text/x-venkman-file", this.fileName);
    transferData.data.addDataForFlavour("text/x-moz-url", this.fileName);
    transferData.data.addDataForFlavour("text/unicode", this.fileName);
    transferData.data.addDataForFlavour("text/html",
                                        "<a href='" + this.fileName +
                                        "'>" + this.fileName + "</a>");
    return true;
}    

ScriptContainerRecord.prototype.appendScriptRecord =
function scr_addscript(scriptRec)
{
    this.appendChild (scriptRec);
}

ScriptContainerRecord.prototype.__defineGetter__ ("sourceText", scr_gettext);
function scr_gettext ()
{
    if (!("_sourceText" in this))
        this._sourceText = new SourceText (this, this.fileName);
    return this._sourceText;
}

ScriptContainerRecord.prototype.sortCompare =
function scr_compare (a, b)
{
    if (console.views.scripts.groupFiles)
    {
        if (a.group < b.group)
            return -1;
    
        if (a.group > b.group)
            return 1;
    }
    
    if (a.displayName < b.displayName)
        return -1;

    if (a.displayName > b.displayName)
        return 1;
    
    return 0;
}

ScriptContainerRecord.prototype.locateChildByScript =
function scr_locate (script)
{
    for (var i = 0; i < this.childData.length; ++i)
        if (script == this.childData[i].script)
            return this.childData[i];

    return null;
}

ScriptContainerRecord.prototype.guessFunctionNames =
function scr_guessnames (sourceText)
{
    for (var i = 0; i < this.childData.length; ++i)
    {
        this.childData[i].guessFunctionName(sourceText);
    }
    /* XXX invalidate scripts view */
    dispatch ("hook-guess-completed", {url: this.fileName});
}

function ScriptRecord(script) 
{
    if (!(script instanceof jsdIScript))
        throw new BadMojo (ERR_INVALID_PARAM, "script");

    this.setColumnPropertyName ("script-name", "functionName");
    this.setColumnPropertyName ("script-line-start", "baseLineNumber");
    this.setColumnPropertyName ("script-line-extent", "lineExtent");
    this.functionName = (script.functionName) ? script.functionName :
        MSG_VAL_TLSCRIPT;
    this.baseLineNumber = script.baseLineNumber;
    this.lineExtent = script.lineExtent;
    this.script = script;

    this.jsdurl = "jsd:sourcetext?url=" + escape(this.script.fileName) + 
        "&base=" + this.baseLineNumber + "&" + "extent=" + this.lineExtent +
        "&name=" + this.functionName;
}

ScriptRecord.prototype = new XULTreeViewRecord(scriptShare);

ScriptRecord.prototype.onDragStart =
function sr_dragstart (e, transferData, dragAction)
{        
    var fileName = this.script.fileName;
    transferData.data = new TransferData();
    transferData.data.addDataForFlavour("text/x-jsd-url", this.jsdurl);
    transferData.data.addDataForFlavour("text/x-moz-url", fileName);
    transferData.data.addDataForFlavour("text/unicode", fileName);
    transferData.data.addDataForFlavour("text/html",
                                        "<a href='" + fileName +
                                        "'>" + fileName + "</a>");
    return true;
}

ScriptRecord.prototype.containsLine =
function sr_containsl (line)
{
    if (this.script.baseLineNumber <= line && 
        this.script.baseLineNumber + this.lineExtent > line)
        return true;
    
    return false;
}

ScriptRecord.prototype.__defineGetter__ ("sourceText", sr_getsource);
function sr_getsource ()
{
    if (!("_sourceText" in this))
        this._sourceText = new PPSourceText(this);
    return this._sourceText;
}

ScriptRecord.prototype.__defineGetter__ ("bpcount", sr_getbpcount);
function sr_getbpcount ()
{
    if (!("_bpcount" in this))
        return 0;

    return this._bpcount;
}

ScriptRecord.prototype.__defineSetter__ ("bpcount", sr_setbpcount);
function sr_setbpcount (value)
{
    var delta;
    
    if ("_bpcount" in this)
    {
        if (value == this._bpcount)
            return value;
        delta = value - this._bpcount;
    }
    else
        delta = value;

    this._bpcount = value;
    this.invalidate();
    this.parentRecord.bpcount += delta;
    this.parentRecord.invalidate();
    return value;
}

ScriptRecord.prototype.guessFunctionName =
function sr_guessname (sourceText)
{
    var targetLine = this.script.baseLineNumber;
    var sourceLines = sourceText.lines;
    if (targetLine > sourceLines)
    {
        dd ("not enough source to guess function at line " + targetLine);
        return;
    }
    
    if (this.functionName == MSG_VAL_TLSCRIPT)
    {
        if (sourceLines[targetLine].search(/\WsetTimeout\W/) != -1)
            this.functionName = MSD_VAL_TOSCRIPT;
        else if (sourceLines[targetLine].search(/\WsetInterval\W/) != -1)
            this.functionName = MSD_VAL_IVSCRIPT;        
        else if (sourceLines[targetLine].search(/\Weval\W/) != -1)
            this.functionName = MSD_VAL_EVSCRIPT;
        return;
    }
    
    if (this.functionName != "anonymous")
        return;
    var scanText = "";
    
    /* scan at most 3 lines before the function definition */
    switch (targetLine - 3)
    {
        case -2: /* target line is the first line, nothing before it */
            break;

        case -1: /* target line is the second line, one line before it */ 
            scanText = 
                String(sourceLines[targetLine - 2]);
            break;
        case 0:  /* target line is the third line, two before it */
            scanText =
                String(sourceLines[targetLine - 3]) + 
                String(sourceLines[targetLine - 2]);
            break;            
        default: /* target line is the fourth or higher line, three before it */
            scanText += 
                String(sourceLines[targetLine - 4]) + 
                String(sourceLines[targetLine - 3]) +
                String(sourceLines[targetLine - 2]);
            break;
    }

    scanText += String(sourceLines[targetLine - 1]);
    
    scanText = scanText.substring(0, scanText.lastIndexOf ("function"));
    var ary = scanText.match (/(\w+)\s*[:=]\s*$/);
    if (ary)
    {
        this.functionName = getMsg(MSN_FMT_GUESSEDNAME, ary[1]);
        this.isGuessedName = true;
    }
    else
    {
        dd ("unable to guess function name based on text ``" + scanText + "''");
    }
}

/*******************************************************************************
 * Stack View
 *******************************************************************************/

var stackShare = new Object();

console.views.stack = new XULTreeView(stackShare);

console.views.stack.viewId = "stack";

console.views.stack.init =
function skv_init()
{
    this.atomFrame = console.atomService.getAtom("item-frame");

    var hooks =
    [
     ["hook-debug-stop",                skv_hookDebugStop,
      "stack:hook-debug-stop",          false],
     ["hook-debug-continue",            skv_hookDebugCont,
      "stack:hook-debug-continue",      false],
     ["frame",                          skv_hookFrame,
      "stack:hook-frame",               false]
    ];

    console.commandManager.hookCommands(hooks);
}

function skv_hookDebugStop (e)
{
    var frameRec;
    
    for (var i = 0; i < console.frames.length; ++i)
    {
        frameRec = new FrameRecord(console.frames[i]);
        console.views.stack.childData.appendChild (frameRec);
    }
}

function skv_hookDebugCont (e)
{
    while (console.views.stack.childData.length)
        console.views.stack.childData.removeChildAtIndex(0);
}

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

console.views.stack.onSelect =
function skv_select (e)
{
    var stackView = console.views.stack;
    var rowIndex = stackView.selection.currentIndex;

    if (rowIndex == -1)
        return;

    if (rowIndex == -1 || rowIndex > stackView.rowCount)
        return;
    var row =
        stackView.childData.locateChildByVisualRow(rowIndex);
    if (!row)
    {
        ASSERT (0, "bogus row index " + rowIndex);
        return;
    }

    var source;
    
    if (row instanceof FrameRecord)
    {
        dispatch ("frame", {frameIndex: row.childIndex});
    }
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
    if (colID == "stack:col-0")
    {
        if (this.childData.locateChildByVisualRow(index))
            properties.AppendElement (this.atomFrame);
    }

    return;
}

function FrameRecord (frame)
{
    if (!(frame instanceof jsdIStackFrame))
        throw new BadMojo (ERR_INVALID_PARAM, "value");

    this.setColumnPropertyName ("col-0", "functionName");
    this.setColumnPropertyName ("col-2", "location");

    var fn = frame.functionName;
    if (!fn)
        fn = MSG_VAL_TLSCRIPT;

    if (!frame.isNative)
    {
        var sourceRec = console.scripts[frame.script.fileName];
        if (sourceRec)
        {
            this.location = sourceRec.shortName + ":" + frame.line;
            var scriptRec = sourceRec.locateChildByScript(frame.script);
            if (fn == "anonymous")
                fn = scriptRec.functionName;
        }
        else
            dd ("no sourcerec");
    }
    else
    {
        this.location = MSG_URL_NATIVE;
    }
    
    this.functionName = fn;
    this.frame = frame;
}

FrameRecord.prototype = new XULTreeViewRecord (stackShare);

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
    
    var atomsvc = console.atomService;

    this.atomCurrent        = atomsvc.getAtom("current-line");
    this.atomHighlightStart = atomsvc.getAtom("highlight-start");
    this.atomHighlightRange = atomsvc.getAtom("highlight-range");
    this.atomHighlightEnd   = atomsvc.getAtom("highlight-end");
    this.atomBreakpoint     = atomsvc.getAtom("breakpoint");
    this.atomFBreakpoint    = atomsvc.getAtom("future-breakpoint");
    this.atomCode           = atomsvc.getAtom("code");
    this.atomPrettyPrint    = atomsvc.getAtom("prettyprint");
    this.atomWhitespace     = atomsvc.getAtom("whitespace");

    var hooks =
    [
     ["hook-debug-continue",                    sv_hookDebugCont,
      "source:hook-debug-continue",             false],
     ["hook-display-sourcetext",                sv_hookDisplay,
      "source:display-hook",                    false],
     ["hook-display-sourcetext-soft",           sv_hookDisplay,
      "source:display-hook",                    false],
     ["pprint",                                 sv_hookPPrint,
      "source:prettyprint-hook",                false],
     ["hook-source-load-complete",              sv_hookLoadComplete,
      "source:reload-hook",                     false]
    ];

    console.commandManager.hookCommands(hooks);
}

function sv_hookDebugCont (e)
{
    /* Invalidate on continue to remove the highlight line. */
    if (console.views.source.tree)
        console.views.source.tree.invalidate();
}

/**
 * Display requested sourcetext.
 */
function sv_hookDisplay (e)
{
    console.views.source.details = e.details;
    console.views.source.displaySourceText(e.sourceText, Boolean(e.startLine));
    if (console.views.source.tree)
    {
        if (e.startLine && e.command.name == "hook-display-sourcetext-soft")
            console.views.source.softScrollTo (e.startLine);
        else
            console.views.source.scrollTo (e.startLine - 2, -1);
    }
    else
    {
        var url = e.sourceText.url;
        if (!(url in console.views.source.savedState))
            console.views.source.savedState[url] = new Object();
        console.views.source.savedState[url].topLine = e.startLine;
    }   
}

/**
 * Sync with the pretty print state as it changes.
 */
function sv_hookPPrint (e)
{
    if (console.views.source.details)
        sv_hookFindScript(console.views.source.details);
}

function sv_hookLoadComplete (e)
{
    console.views.source.syncTreeView();
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
    };
    
    syncTreeView (getChildById(this.currentContent, "source-tree"), this, cb);
}

console.views.source.onHide =
function sv_hide ()
{
    syncTreeView (getChildById(this.currentContent, "source-tree"), null);
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
        
        if (colID == "breakpoint-col")
        {
            if ("onMarginClick" in console.sourceView.childData)
                console.views.source.childData.onMarginClick (e, row + 1);
        }
    }

}

console.views.source._scrollTo = BasicOView.prototype.scrollTo;
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
    this._scrollTo(line, align);
}

console.views.source.syncTreeView =
function sv_sync(skipScrollRestore)
{
    if (!ASSERT(this.childData, "No childData to sync!"))
        return;
    
    if (this.tree)
    {
        var url = this.childData.url;
        var state;
        if (url in this.savedState)
            state = this.savedState[url];
        else
        {
            dd ("making new state");
            state = this.savedState[url] = new Object();
        }

        if (!("lastRowCount" in state) || state.lastRowCount != this.rowCount)
        {
            dd ("notifying new row count " + this.rowCount);
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
        this.scrollTo (line, 0);
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

    var sourceText = this.childData;
    cx.fileName = sourceText.fileName;
    cx.lineIsExecutable = null;
    var selection = this.tree.selection;
    var row = selection.currentIndex;

    if (row != -1)
    {
        cx.lineNumber = selection.currentIndex + 1;
        if ("lineMap" in sourceText && sourceText.lineMap[row])
            cx.lineIsExecutable = true;
        if (typeof sourceText.lines[row] == "object" &&
            "bpRecord" in sourceText.lines[row])
        {
            cx.breakpointRec = sourceText.lines[row].bpRecord;
            cx.breakpointIndex = cx.breakpointRec.childIndex;
        }
    }
    else
        dd ("no currentIndex");
    
    var rangeCount = selection.getRangeCount();
    if (rangeCount > 0 && !("lineNumberList" in cx))
    {
        cx.lineNumberList = new Array();
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
            cx.lineNumberList.push (row + 1);
            if (range == 0 && row == min &&
                "lineMap" in sourceText && sourceText.lineMap[row])
            {
                cx.lineIsExecutable = true;
            }
            if (typeof sourceText.lines[row] == "object" &&
                "bpRecord" in sourceText.lines[row])
            {
                var sourceLine = sourceText.lines[row];
                if (!("breakpointRecList" in cx))
                    cx.breakpointRecList = new Array();
                cx.breakpointRecList.push(sourceLine.bpRecord);
                if (!("breakpointIndexList" in cx))
                    cx.breakpointIndexList = new Array();
                cx.breakpointIndexList.push(sourceLine.bpRecord.childIndex);
            }
        }
    }

    return cx;
}    

/* nsITreeView */
console.views.source.getRowProperties =
function sv_rowprops (row, properties)
{
    if ("frames" in console)
     {
        if (((!this.prettyPrint && row == console.stopLine - 1) ||
             (this.prettyPrint && row == console.pp_stopLine - 1)) &&
            console.stopFile == this.childData.fileName && this.details)
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
    
    if (colID == "breakpoint-col")
    {
        if (this.prettyPrint)
            properties.AppendElement(this.atomPrettyPrint);
        if (typeof this.childData.lines[row] == "object" &&
            "bpRecord" in this.childData.lines[row])
        {
            if (this.childData.lines[row].bpRecord.scriptRecords.length)
                properties.AppendElement(this.atomBreakpoint);
            else
                properties.AppendElement(this.atomFBreakpoint);
        }
        else if ("lineMap" in this.childData && row in this.childData.lineMap &&
                 this.childData.lineMap[row] & SourceText.LINE_BREAKABLE)
        {
            properties.AppendElement(this.atomCode);
        }
        else
        {
            properties.AppendElement(this.atomWhitespace);
        }
    }
    
    if ("highlightStart" in console)
    {
        var atom;
        if (row == console.highlightStart)
        {
            atom = this.atomHighlightStart;
        }
        else if (row == console.highlightEnd)
        {
            atom = this.atomHighlightEnd;
        }
        else if (row > console.highlightStart && row < console.highlightEnd)
        {
            atom = this.atomHighlightRange;
        }
        
        if (atom && console.highlightFile == this.childData.fileName)
        {
            properties.AppendElement(atom);
        }
    }

    if ("frames" in console)
    {
        if (((!this.prettyPrint && row == console.stopLine - 1) ||
             (this.prettyPrint && row == console.pp_stopLine - 1)) &&
            console.stopFile == this.childData.fileName)
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

var watchShare = new Object();

console.views.watches = new XULTreeView(watchShare);

console.views.watches.viewId = "watches";

console.views.watches.init =
function wv_init()
{
}

console.views.watches.onShow =
function wv_show()
{
    syncTreeView (getChildById(this.currentContent, "watch-tree"), this);
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

/*******************************************************************************
 * Windows View
 *******************************************************************************/

var windowsShare = new Object();

console.views.windows = new XULTreeView(windowsShare);

console.views.windows.viewId = "windows";

console.views.windows.init = 
function winv_init ()
{
    var atomsvc = console.atomService;
    WindowRecord.prototype.property         = atomsvc.getAtom("item-window");
    FileContainerRecord.prototype.property  = atomsvc.getAtom("item-files");
    FileRecord.prototype.property           = atomsvc.getAtom("item-file");

    var hooks =
    [
     ["hook-window-opened",     winv_hookWindowOpened,
      "winv:hook-window-opened", false],
     ["hook-window-closed",     winv_hookWindowClosed,
      "winv:hook-window-closed", false]
     ];
    
    console.commandManager.hookCommands(hooks);

}

function winv_hookWindowOpened (e)
{
    console.views.windows.childData.appendChild (new WindowRecord(e.window, ""));
}

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
    for (var i = 0; i < this.childData.length; ++i)
    {
        var child = this.childData[i];
        if (child.window == win)
            return child;
    }

    return null;
}

function WindowRecord (win, baseURL)
{
    function none() { return ""; };
    this.setColumnPropertyName ("col-0", "shortName");

    this.window = win;
    this.url = win.location.href;
    if (this.url.search(/^\w+:/) == -1 && "url")
    {
        this.url = baseURL + url;
        this.baseURL = baseURL;
    }
    else
    {
        this.baseURL = getPathFromURL(this.url);
    }
    
    this.shortName = getFileFromPath (this.url);
    
    this.reserveChildren();
    this.filesRecord = new FileContainerRecord();
}

WindowRecord.prototype = new XULTreeViewRecord(windowsShare);

WindowRecord.prototype.onPreOpen =
function wr_preopen()
{   
    this.childData = new Array();

    this.appendChild(this.filesRecord);    

    var framesLength = this.window.frames.length;
    for (var i = 0; i < framesLength; ++i)
    {
        this.appendChild(new WindowRecord(this.window.frames[i].window,
                                          this.baseURL));
    }
}
    
function FileContainerRecord ()
{
    function files() { return MSG_FILES_REC; }
    function none() { return ""; }
    this.setColumnPropertyName ("col-0", files);
    this.reserveChildren();
}

FileContainerRecord.prototype = new XULTreeViewRecord(windowsShare);

FileContainerRecord.prototype.onPreOpen =
function fcr_preopen ()
{
    if (!this.parentRecord)
        return;
    
    this.childData = new Array();
    var doc = this.parentRecord.window.document;
    var nodeList = doc.getElementsByTagName("script");
    
    for (var i = 0; i < nodeList.length; ++i)
    {
        var url = nodeList.item(i).getAttribute("src");
        if (url)
        {
            if (url.search(/^\w+:/) == -1)
                url = getPathFromURL (this.parentRecord.url) + url;
            this.appendChild(new FileRecord(url));
        }
    }
}

function FileRecord (url)
{
    function none() { return ""; }
    this.setColumnPropertyName ("col-0", "shortName");
    this.url = url;
    this.shortName = getFileFromPath(url);
}

FileRecord.prototype = new XULTreeViewRecord(windowsShare);

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

if (0) 
{
    
console.stackView.restoreState =
function sv_restore ()
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
                    }
                }
            }
        }
    }

    if ("savedState" in this) {
        this.freeze();
        restoreBranch (this.stack.childData, this.savedState);
        this.thaw();
        this.scrollTo (this.savedState.firstVisible, -1);
    }
}

console.stackView.saveState =
function sv_save ()
{
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
    }
        
    this.savedState = new Object();
    this.savedState.firstVisible = this.tree.getFirstVisibleRow() + 1;
    saveBranch (this.savedState, this.stack.childData);
    //dd ("saved as\n" + dumpObjectTree(this.savedState, 10));
}
    
}
