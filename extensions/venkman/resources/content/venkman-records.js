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

function initRecords()
{
    var atomsvc = console.atomService;

    WindowRecord.prototype.property         = atomsvc.getAtom("item-window");

    FileContainerRecord.prototype.property  = atomsvc.getAtom("item-files");

    FileRecord.prototype.property           = atomsvc.getAtom("item-file");

    FrameRecord.prototype.property    = atomsvc.getAtom("item-frame");
    FrameRecord.prototype.atomCurrent = atomsvc.getAtom("current-frame-flag");

    ValueRecord.prototype.atomVoid      = atomsvc.getAtom("item-void");
    ValueRecord.prototype.atomNull      = atomsvc.getAtom("item-null");
    ValueRecord.prototype.atomBool      = atomsvc.getAtom("item-bool");
    ValueRecord.prototype.atomInt       = atomsvc.getAtom("item-int");
    ValueRecord.prototype.atomDouble    = atomsvc.getAtom("item-double");
    ValueRecord.prototype.atomString    = atomsvc.getAtom("item-string");
    ValueRecord.prototype.atomFunction  = atomsvc.getAtom("item-function");
    ValueRecord.prototype.atomObject    = atomsvc.getAtom("item-object");
    ValueRecord.prototype.atomError     = atomsvc.getAtom("item-error");
    ValueRecord.prototype.atomException = atomsvc.getAtom("item-exception");
}

/*******************************************************************************
 * Breakpoint Record.
 * One prototype for all breakpoint types, works only in the Breakpoints View.
 *******************************************************************************/

function BPRecord (breakWrapper)
{
    this.setColumnPropertyName ("col-0", "name");
    this.setColumnPropertyName ("col-1", "line");

    this.breakWrapper = breakWrapper;
    
    if ("pc" in breakWrapper)
    {
        this.type = "instance";
        this.name = breakWrapper.scriptWrapper.functionName;
        this.line = getMsg(MSN_FMT_PC, String(breakWrapper.pc));
    }
    else if (breakWrapper instanceof FutureBreakpoint)
    {
        this.type = "future";
        var ary = breakWrapper.url.match(/\/([^\/?]+)(\?|$)/);
        if (ary)
            this.name = ary[1];
        else
            this.name = breakWrapper.url;

        this.line = breakWrapper.lineNumber;
    }    
}

BPRecord.prototype = new XULTreeViewRecord(console.views.breaks.share);

/*******************************************************************************
 * Stack Frame Record.
 * Represents a jsdIStackFrame, for use in the Stack View only.
 *******************************************************************************/

function FrameRecord (jsdFrame)
{
    if (!(jsdFrame instanceof jsdIStackFrame))
        throw new BadMojo (ERR_INVALID_PARAM, "value");

    this.setColumnPropertyName ("col-0", "functionName");
    this.setColumnPropertyName ("col-1", "location");

    this.functionName = jsdFrame.functionName;
    if (!jsdFrame.isNative)
    {
        this.scriptWrapper = getScriptWrapper(jsdFrame.script);
        this.location = getMsg(MSN_FMT_FRAME_LOCATION,
                               [getFileFromPath(jsdFrame.script.fileName),
                                jsdFrame.line, jsdFrame.pc]);
        this.functionName = this.scriptWrapper.functionName;
    }
    else
    {
        this.scriptWrapper = null;
        this.location = MSG_URL_NATIVE;
    }
    
    this.jsdFrame = jsdFrame;
}

FrameRecord.prototype = new XULTreeViewRecord (console.views.stack.share);

/*******************************************************************************
 * Window Record.
 * Represents a DOM window with files and child windows.  For use in the
 * Windows View.
 *******************************************************************************/

function WindowRecord (win, baseURL)
{
    function none() { return ""; };
    this.setColumnPropertyName ("col-0", "shortName");

    this.window = win;
    this.url = win.location.href;
    if (this.url.search(/^\w+:/) == -1)
    {
        if (this.url[0] == "/")
            this.url = win.location.host + this.url;
        else
            this.url = baseURL + this.url;
        this.baseURL = baseURL;
    }
    else
    {
        this.baseURL = getPathFromURL(this.url);
    }
    
    this.shortName = getFileFromPath (this.url);
    
    this.reserveChildren(true);
    this.filesRecord = new FileContainerRecord(this);
}

WindowRecord.prototype = new XULTreeViewRecord(console.views.windows.share);

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
    
/*******************************************************************************
 * "File Container" Record.
 * List of script tags found in the parent record's |window.document| property.
 * For use in the Windows View, as a child of a WindowRecord.
 *******************************************************************************/

function FileContainerRecord (windowRecord)
{
    function files() { return MSG_FILES_REC; }
    function none() { return ""; }
    this.setColumnPropertyName ("col-0", files);
    this.windowRecord = windowRecord;
    this.reserveChildren(true);
}

FileContainerRecord.prototype =
    new XULTreeViewRecord(console.views.windows.share);

FileContainerRecord.prototype.onPreOpen =
function fcr_getkids ()
{
    if (!("parentRecord" in this))
        return;
    
    this.childData = new Array();
    var doc = this.windowRecord.window.document;
    var nodeList = doc.getElementsByTagName("script");
    dd (nodeList.length + "nodes");
    for (var i = 0; i < nodeList.length; ++i)
    {
        var url = nodeList.item(i).getAttribute("src");
        if (url)
        {
            if (url.search(/^\w+:/) == -1)
                url = getPathFromURL (this.windowRecord.url) + url;
            this.appendChild(new FileRecord(url));
        }
    }
}

/*******************************************************************************
 * File Record
 * Represents a URL, for use in the Windows View only.
 *******************************************************************************/

function FileRecord (url)
{
    function none() { return ""; }
    this.setColumnPropertyName ("col-0", "shortName");
    this.url = url;
    this.shortName = getFileFromPath(url);
}

FileRecord.prototype = new XULTreeViewRecord(console.views.windows.share);

/*******************************************************************************
 * Script Instance Record.
 * Represents a ScriptInstance, for use in the Scripts View only.
 *******************************************************************************/

function ScriptInstanceRecord(scriptInstance)
{
    if (!ASSERT(scriptInstance.isSealed,
                "Attempt to create ScriptInstanceRecord for unsealed instance"))
    {
        return null;
    }
    
    this.setColumnPropertyName ("col-0", "displayName");
    this.setColumnPropertyValue ("col-1", "");
    this.setColumnPropertyValue ("col-2", "");
    this.reserveChildren(true);
    this.url = scriptInstance.url;
    var sv = console.views.scripts;
    this.fileType = sv.atomUnknown;
    this.shortName = this.url;
    this.group = 4;
    this.scriptInstance = scriptInstance;
    this.lastScriptCount = 0;
    
    this.shortName = getFileFromPath(this.url);
    var ary = this.shortName.match (/\.(js|html|xul|xml)$/i);
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

    return this;
}

ScriptInstanceRecord.prototype =
    new XULTreeViewRecord(console.views.scripts.share);

ScriptInstanceRecord.prototype.onDragStart =
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

ScriptInstanceRecord.prototype.super_resort = XTRootRecord.prototype.resort;

ScriptInstanceRecord.prototype.resort =
function scr_resort ()
{
    console._groupFiles = console.prefs["scriptsView.groupFiles"];
    this.super_resort();
    delete console._groupFiles;
}

ScriptInstanceRecord.prototype.sortCompare =
function scr_compare (a, b)
{
    if (0)
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

ScriptInstanceRecord.prototype.onPreOpen =
function scr_preopen ()
{
    if (!this.scriptInstance.sourceText.isLoaded)
        this.scriptInstance.sourceText.loadSource();
    
    if (this.lastScriptCount != this.scriptInstance.scriptCount)
    {
        console.views.scripts.freeze();
        this.childData = new Array();
        var scriptWrapper = this.scriptInstance.topLevel;
        var sr = new ScriptRecord(scriptWrapper);
        scriptWrapper.scriptRecord = sr;
        this.appendChild(sr);

        var functions = this.scriptInstance.functions;
        for (var f in functions)
        {
            if (functions[f].jsdScript.isValid)
            {
                sr = new ScriptRecord(functions[f]);
                functions[f].scriptRecord = sr;
                this.appendChild(sr);
            }
        }
        console.views.scripts.thaw();
        this.lastScriptCount = this.scriptInstance.scriptCount;
    }
}

/*******************************************************************************
 * Script Record.
 * Represents a ScriptWrapper, for use in the Scripts View only.
 *******************************************************************************/

function ScriptRecord(scriptWrapper) 
{
    this.setColumnPropertyName ("col-0", "functionName");
    this.setColumnPropertyName ("col-1", "baseLineNumber");
    this.setColumnPropertyName ("col-2", "lineExtent");

    this.functionName = scriptWrapper.functionName
    this.baseLineNumber = scriptWrapper.jsdScript.baseLineNumber;
    this.lineExtent = scriptWrapper.jsdScript.lineExtent;
    this.scriptWrapper = scriptWrapper;

    this.jsdurl = "jsd:sourcetext?url=" +
        escape(this.scriptWrapper.jsdScript.fileName) + 
        "&base=" + this.baseLineNumber + "&extent=" + this.lineExtent +
        "&name=" + this.functionName;
}

ScriptRecord.prototype = new XULTreeViewRecord(console.views.scripts.share);

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

/*******************************************************************************
 * Value Record.
 * Use this to show a jsdIValue in any tree.
 *******************************************************************************/

function ValueRecord (value, name, flags)
{
    if (!(value instanceof jsdIValue))
        throw new BadMojo (ERR_INVALID_PARAM, "value", String(value));

    this.setColumnPropertyName ("col-0", "displayName");
    this.setColumnPropertyName ("col-1", "displayType");
    this.setColumnPropertyName ("col-2", "displayValue");
    this.setColumnPropertyName ("col-3", "displayFlags");    
    this.displayName = name;
    this.displayFlags = formatFlags(flags);
    this.flags = flags;
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

ValueRecord.prototype.getProperties =
function vr_getprops (properties)
{
    if ("valueIsException" in this || this.flags & PROP_EXCEPTION)
        properties.AppendElement (this.atomException);

    if (this.flags & PROP_ERROR)
        properties.AppendElement (this.atomError);

    properties.AppendElement (this.property);
}

ValueRecord.prototype.refresh =
function vr_refresh ()
{
    if ("onPreRefresh" in this)
    {
        try
        {
            this.onPreRefresh();
            delete this.valueIsException;
        }
        catch (ex)
        {
            if (!(ex instanceof jsdIValue))
                ex = console.jsds.wrapValue(ex);
            
            this.value = ex;
            this.valueIsException = true;
        }
    }

    var sizeDelta = 0;
    var lastType = this.jsType;
    this.jsType = this.value.jsType;
    
    if (0 && lastType != this.jsType && lastType == jsdIValue.TYPE_FUNCTION)
    {
        /* we changed from a non-function to a function */
        --this.hiddenFunctionCount;
        ++sizeDelta;
    }
    
    if (this.jsType != jsdIValue.TYPE_OBJECT)
    {
        if ("childData" in this)
        {
            /* if we're not an object but we have child data, then we must have
             * just turned into something other than an object. */
            delete this.childData;
            this.isContainerOpen = false;
            sizeDelta = 1 - this.visualFootprint;
        }
        
        if ("alwaysHasChildren" in this)
            delete this.alwaysHasChildren;
    }
    else
    {
        this.alwaysHasChildren = true;
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
            this.displayValue = (this.value.isNative) ? MSG_VAL_NATIVE :
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
                                                prop.flags));
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
