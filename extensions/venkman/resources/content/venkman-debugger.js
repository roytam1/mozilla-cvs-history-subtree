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

const JSD_CTRID = "@mozilla.org/js/jsd/debugger-service;1";
const jsdIDebuggerService = Components.interfaces.jsdIDebuggerService;
const jsdIExecutionHook   = Components.interfaces.jsdIExecutionHook;
const jsdIErrorHook       = Components.interfaces.jsdIErrorHook;
const jsdICallHook        = Components.interfaces.jsdICallHook;
const jsdIValue           = Components.interfaces.jsdIValue;
const jsdIProperty        = Components.interfaces.jsdIProperty;
const jsdIScript          = Components.interfaces.jsdIScript;
const jsdIStackFrame      = Components.interfaces.jsdIStackFrame;
const jsdIFilter          = Components.interfaces.jsdIFilter;

const COLLECT_PROFILE_DATA = jsdIDebuggerService.COLLECT_PROFILE_DATA;

const PCMAP_SOURCETEXT    = jsdIScript.PCMAP_SOURCETEXT;
const PCMAP_PRETTYPRINT   = jsdIScript.PCMAP_PRETTYPRINT;

const FTYPE_STD     = 0;
const FTYPE_SUMMARY = 1;
const FTYPE_ARRAY   = 2;

const FILTER_ENABLED  = jsdIFilter.FLAG_ENABLED;
const FILTER_DISABLED = ~jsdIFilter.FLAG_ENABLED;
const FILTER_PASS     = jsdIFilter.FLAG_PASS;
const FILTER_SYSTEM   = 0x100; /* system filter, do not show in UI */

var $ = new Array(); /* array to store results from evals in debug frames */

function initDebugger()
{   
    dd ("initDebugger {");
    
    console._continueCodeStack = new Array(); /* top of stack is the default  */
                                              /* return code for the most     */
                                              /* recent debugTrap().          */
    console.scriptWrappers = new Object();
    console.scriptManagers = new Object();
    console.breaks = new Object();
    console.fbreaks = new Object();
    console.sbreaks = new Object();
    
    /* create the debugger instance */
    if (!Components.classes[JSD_CTRID])
        throw new BadMojo (ERR_NO_DEBUGGER);
    
    console.jsds = Components.classes[JSD_CTRID].getService(jsdIDebuggerService);
    console.jsds.on();
    console.executionHook = { onExecute: jsdExecutionHook };
    console.errorHook = { onError: jsdErrorHook };
    console.scriptHook = { onScriptCreated: jsdScriptCreated,
                           onScriptDestroyed: jsdScriptDestroyed };
    console.jsds.breakpointHook = console.executionHook;
    console.jsds.debuggerHook = console.executionHook;
    console.jsds.debugHook = console.executionHook;
    console.jsds.errorHook = console.errorHook;
    console.jsds.scriptHook = console.scriptHook;
    console.jsds.flags = jsdIDebuggerService.ENABLE_NATIVE_FRAMES;

    console.chromeFilter = {
        globalObject: null,
        flags: FILTER_SYSTEM | FILTER_ENABLED,
        urlPattern: "chrome:*",
        startLine: 0,
        endLine: 0
    };
    
    if (console.prefs["enableChromeFilter"])
    {
        console.enableChromeFilter = true;
        console.jsds.appendFilter(console.chromeFilter);
    }
    else
        console.enableChromeFilter = false;

    var venkmanFilter1 = {  /* glob based filter goes first, because it's the */
        globalObject: this, /* easiest to match.                              */
        flags: FILTER_SYSTEM | FILTER_ENABLED,
        urlPattern: null,
        startLine: 0,
        endLine: 0
    };
    var venkmanFilter2 = {  /* url based filter for XPCOM callbacks that may  */
        globalObject: null, /* not happen under our glob.                     */
        flags: FILTER_SYSTEM | FILTER_ENABLED,
        urlPattern: "chrome://venkman/*",
        startLine: 0,
        endLine: 0
    };
    console.jsds.appendFilter (venkmanFilter1);
    console.jsds.appendFilter (venkmanFilter2);

    console.throwMode = TMODE_IGNORE;
    console.errorMode = EMODE_IGNORE;

    console.jsds.enumerateScripts({ enumerateScript: jsdScriptCreated });

    dd ("} initDebugger");
}

function detachDebugger()
{
    if ("frames" in console)
        console.jsds.exitNestedEventLoop();
    
    console.jsds.topLevelHook = null;
    console.jsds.functionHook = null;
    console.jsds.breakpointHook = null;
    console.jsds.debuggerHook = null;
    console.jsds.errorHook = null;
    console.jsds.scriptHook = null;
    console.jsds.interruptHook = null;
    console.jsds.clearAllBreakpoints();
    console.jsds.clearFilters();
    if (!console.jsds.initAtStartup)
        console.jsds.off();
}

function jsdExecutionHook (frame, type, rv)
{
    var hookReturn = jsdIExecutionHook.RETURN_CONTINUE;

    if (!ASSERT(!("frames" in console), "Execution hook called while stopped") ||
        frame.isNative ||
        !ASSERT(frame.script, "Execution hook called with no script") ||
        frame.script.fileName == MSG_VAL_CONSOLE)
    {
        return hookReturn;
    }

    var targetWindow = null;
    var wasModal = false;
    var ex;
    var cx;

    try
    {
        cx = frame.executionContext;
    }
    catch (ex)
    {
        dd ("no context");
        cx = null;
    }
    
    var targetWasEnabled = true;
    var debuggerWasEnabled = console.debuggerWindow.enabled;
    console.debuggerWindow.enabled = true;
    
    if (cx)
    {
        cx.scriptsEnabled = false;
        var glob = cx.globalObject;
        if (glob)
        {
            console.targetWindow =
                getBaseWindowFromWindow(glob.getWrappedValue());
            targetWasEnabled = console.targetWindow.enabled;
            console.targetWindow.enabled = false;
        }
    }

    try
    {
        hookReturn = debugTrap(frame, type, rv);
    }
    catch (ex)
    {
        display (MSG_ERR_INTERNAL_BPT, MT_ERROR);
        display (formatException(ex), MT_ERROR);
    }
    
    if (cx)
        cx.scriptsEnabled = true;

    
    if (console.targetWindow)
        console.targetWindow.enabled = targetWasEnabled;
    console.debuggerWindow.enabled = debuggerWasEnabled;
    delete console.frames;
    delete console.targetWindow;
    if ("__exitAfterContinue__" in console)
        window.close();

    return hookReturn;
}

function jsdCallHook (frame, type)
{
    if (type == jsdICallHook.TYPE_FUNCTION_CALL)
    {
        setStopState(false);
        ++console._stepOverLevel;
    }
    else if (type == jsdICallHook.TYPE_FUNCTION_RETURN)
    {
        if (--console._stepOverLevel <= 0)
        {
            setStopState(true);
            console.jsds.functionHook = null;
        }
    }
    //dd ("Call Hook: " + frame.functionName + ", type " +
    //    type + " callCount: " + console._stepOverLevel);
}

function jsdErrorHook (message, fileName, line, pos, flags, exception)
{
    try
    {
        var flagstr;
        flagstr =
            (flags && jsdIErrorHook.REPORT_EXCEPTION) ? "x" : "-";
        flagstr +=
            (flags && jsdIErrorHook.REPORT_STRICT) ? "s" : "-";
        
        //dd ("===\n" + message + "\n" + fileName + "@" + 
        //    line + ":" + pos + "; " + flagstr);
        var msn = (flags & jsdIErrorHook.REPORT_WARNING) ?
            MSN_ERPT_WARN : MSN_ERPT_ERROR;

        if (console.errorMode != EMODE_IGNORE)
            display (getMsg(msn, [message, flagstr, fileName,
                                  line, pos]), MT_ETRACE);
        
        if (console.errorMode == EMODE_BREAK)
            return false;
        
        return true;
    }
    catch (ex)
    {
        dd ("error in error hook: " + ex);
    }
    return true;
}

function jsdScriptCreated (jsdScript)
{
    var url = jsdScript.fileName;
    var manager;
    
    if (!(url in console.scriptManagers))
    {
        manager = console.scriptManagers[url] = new ScriptManager(url);
        dispatch ("hook-script-manager-created", { scriptManager: manager });
    }
    else
    {
        manager = console.scriptManagers[url];
    }
    
    manager.onScriptCreated(jsdScript);
}

function jsdScriptDestroyed (jsdScript)
{
    var scriptWrapper = console.scriptWrappers[jsdScript.tag];
    scriptWrapper.scriptManager.onScriptInvalidated(scriptWrapper);
    
    if (scriptWrapper.scriptManager.instances.length == 0 && 
        scriptWrapper.scriptManager.transientCount == 0)
    {
        delete console.scriptManagers[scriptWrapper.scriptManager.url];
        dispatch ("hook-script-manager-destroyed",
                  { scriptManager: scriptWrapper.scriptManager });
    }
}

function ScriptManager (url)
{
    this.url = url;
    this.instances = new Array();
    this.transients = new Object();
    this.transientCount = 0;
}

ScriptManager.prototype.onScriptCreated =
function smgr_created (jsdScript)
{
    var instance;
    
    if (this.instances.length == 0 ||
        this.instances[this.instances.length - 1].isSealed)
    {
        /* first instance of this file, or most recent instance is already
         * sealed. */
        instance = new ScriptInstance(this);
        this.instances.push(instance);
        dispatch ("hook-script-instance-created", { scriptInstance: instance });
    }
    else
    {
        instance = this.instances[this.instances.length - 1];
    }

    var scriptWrapper = new ScriptWrapper(jsdScript);
    scriptWrapper.scriptManager = this;

    console.scriptWrappers[jsdScript.tag] = scriptWrapper;

    if (instance.isSealed)
    {
        ++this.transients;
        this.transients[tag] = scriptWrapper;
        scriptWrapper.scriptInstance = null;
        scriptWrapper.functionName = MSG_VAL_EVSCRIPT;
        dispatch ("hook-transient-script", { scriptWrapper: scriptWrapper });
    }
    else
    {
        scriptWrapper.scriptInstance = instance;
        instance.onScriptCreated (scriptWrapper);
    }

    //dispatch ("hook-script-created", { scriptWrapper: scriptWrapper });
}

ScriptManager.prototype.onScriptInvalidated =
function smgr_invalidated (scriptWrapper)
{
    delete console.scriptWrappers[scriptWrapper.tag];
    if (scriptWrapper.tag in this.transients)
    {
        --this.transientCount;
        delete this.transients[scriptWrapper.tag];
        //dispatch ("hook-script-invalidated", { scriptWrapper: scriptWrapper });
    }
    else
    {
        scriptWrapper.scriptInstance.onScriptInvalidated(scriptWrapper);
        //dispatch ("hook-script-invalidated", { scriptWrapper: scriptWrapper });

        if (scriptWrapper.scriptInstance.scriptCount == 0)
        {
            var i = arrayIndexOf(this.instances, scriptWrapper.scriptInstance);
            arrayRemoveAt(this.instances, i);
            dispatch ("hook-script-instance-destroyed",
                      { scriptInstance: scriptWrapper.scriptInstance });
        }
    }
}    

ScriptManager.prototype.__defineGetter__ ("sourceText", smgr_sourcetext);
function smgr_sourcetext()
{
    return this.instances[this.instances.length - 1].sourceText;
}

ScriptManager.prototype.__defineGetter__ ("lineMap", smgr_linemap);
function smgr_linemap()
{
    return this.instances[this.instances.length - 1].lineMap;
}

ScriptManager.prototype.hasBreakpoint =
function smgr_hasbp (line)
{
    for (var i in instances)
    {
        if (instance[i].hasBreakpoint(line))
            return true;
    }
    
    return false;
}

ScriptManager.prototype.setBreakpoint =
function smgr_break (line)
{
    for (var i in instances)
        instance[i].setBreakpoint(line, fbreak);
}

ScriptManager.prototype.clearBreakpoint =
function smgr_break (line)
{
    for (var i in instances)
        instance[i].clearBreakpoint(line);
}

ScriptManager.prototype.hasFutureBreakpoint =
function smgr_hasbp (line)
{
    var key = this.url + "#" + line;
    return (key in console.fbreaks);
}

ScriptManager.prototype.getFutureBreakpoint =
function smgr_getfbp (line)
{
    var key = this.url + "#" + line;
    if (key in console.fbreaks)
        return console.fbreaks[key];
    
    return null;
}

ScriptManager.prototype.setFutureBreakpoint =
function smgr_fbreak (line)
{
    var key = this.url + "#" + line;
    if (key in console.fbreaks)
    {
        display (getMsg(MSN_BP_EXISTS, [this.url, this.line]), MT_ERROR);
        return null;
    }
    
    var fbreak = new FutureBreakpoint (this.url, line);
    console.fbreaks[key] = fbreak;

    for (var i in this.instances)
    {
        if (this.instances[i]._lineMapInited)
            arrayOrFlag (this.instances[i]._lineMap, line - 1, LINE_FBREAK);
    }
    
    dispatch ("hook-fbreak-set", { fbreak: fbreak });

    return fbreak;
}

ScriptManager.prototype.clearFutureBreakpoint =
function smgr_clear (line)
{
    var key = this.url + "#" + line;
    if (!(key in console.fbreaks))
    {
        display (getMsg(MSN_ERR_NO_DICE, [this.url, this.line]), MT_ERROR);
        return;
    }

    var fbreak = console.fbreaks[key];
    delete console.fbreaks[key];

    for (var i in this.instances)
    {
        if (this.instances[i]._lineMapInited)
            arrayAndFlag (this.instances[i]._lineMap, line - 1, ~LINE_FBREAK);
    }

    dispatch ("hook-fbreak-clear", { fbreak: fbreak });
}

function FutureBreakpoint (url, lineNumber)
{
    this.url = url;
    this.lineNumber = lineNumber;
    this.enabled = true;
    this.scriptText = null;
    this.childrenBP = new Object;
}

function ScriptInstance (manager)
{
    this.scriptManager = manager;
    this.url = manager.url;
    this.creationDate = new Date();
    this.topLevel = null;
    this.functions = new Object();
    this.isSealed = false;
    this.scriptCount = 0;
    this.breakpointCount = 0;
    this._lineMap = new Array();
    this._lineMapInited = false;
}

ScriptInstance.prototype.onScriptCreated =
function si_created (scriptWrapper)
{
    var tag = scriptWrapper.jsdScript.tag;
    
    if (scriptWrapper.functionName)
    {
        this.functions[tag] = scriptWrapper;
    }
    else
    {
        this.sealDate = new Date();
        this.topLevel = scriptWrapper;
        scriptWrapper.functionName = MSG_VAL_TLSCRIPT;
        this.isSealed = true;
        scriptWrapper.addToLineMap(this._lineMap);
        dispatch ("hook-script-instance-sealed", { scriptInstance: this });
    }

    ++this.scriptCount;
}

ScriptInstance.prototype.onScriptInvalidated =
function si_invalidated (scriptWrapper)
{
    --this.scriptCount;
}

ScriptInstance.prototype.__defineGetter__ ("sourceText", si_gettext);
function si_gettext ()
{
    if (!("_sourceText" in this))
        this._sourceText = new SourceText (this);

    return this._sourceText;
}

ScriptInstance.prototype.__defineGetter__ ("lineMap", si_linemap);
function si_linemap()
{
    if (!this._lineMapInited)
    {
        for (var i in this.functions)
            this.functions[i].addToLineMap(this._lineMap);

        for (var fbp in console.fbreaks)
        {
            var fbreak = console.fbreaks[fbp];
            if (fbreak.url == this.url)
                arrayOrFlag (this._lineMap, fbreak.line - 1, LINE_FBREAK);
        }
        
        this._lineMapInited = true;
    }

    return this._lineMap;            
}

ScriptInstance.prototype.hasBreakpoint =
function si_hasbp (line)
{
    function hasBP (scriptWrapper)
    {        
        var jsdScript = scriptWrapper.jsdScript;
        if (!jsdScript.isValid)
            return false;

        if (line >= jsdScript.baseLineNumber &&
            line <= jsdScript.baseLineNumber + jsdScript.lineExtent &&
            scriptWrapper.hasBreakpoint(jsdScript.lineToPc(line,
                                                           PCMAP_SOURCETEXT)))
        {
            return true;
        }

        return false;
    };
    
    if (hasBP(this.topLevel))
        return true;

    for (var f in this.functions)
    {
        if (hasBP(this.functions[f]))
            return true;
    }
    
    return false;
}

ScriptInstance.prototype.setBreakpoint =
function si_setbp (line, parentBP)
{
    function setBP (scriptWrapper)
    {
        var jsdScript = scriptWrapper.jsdScript;
        if (!jsdScript.isValid)
            return false;

        if (line >= jsdScript.baseLineNumber &&
            line <= jsdScript.baseLineNumber + jsdScript.lineExtent &&
            scriptWrapper.jsdScript.isLineExecutable (line, PCMAP_SOURCETEXT))
        {
            scriptWrapper.setBreakpoint(jsdScript.lineToPc(line,
                                                           PCMAP_SOURCETEXT),
                                        parentBP);
            return true;
        }
        return false;
    };

    var foundOne = setBP (this.topLevel);
    for (var f in this.functions)
        foundOne |= setBP (this.functions[f]);

    if (this._lineMapInited && foundOne)
        arrayOrFlag(this._lineMap, line - 1, LINE_BREAK);    
}

ScriptInstance.prototype.clearBreakpoint =
function si_setbp (line)
{
    function clearBP (scriptWrapper)
    {
        var jsdScript = scriptWrapper.jsdScript;
        if (!jsdScript.isValid)
            return;
        
        var pc = jsdScript.lineToPc(line, PCMAP_SOURCETEXT);
        if (line >= jsdScript.baseLineNumber &&
            line <= jsdScript.baseLineNumber + jsdScript.lineExtent &&
            scriptWrapper.hasBreakpoint(pc))
        {
            scriptWrapper.clearBreakpoint(pc);
        }
    };

    if (this._lineMapInited)
        arrayAndFlag(this._lineMap, line - 1, ~LINE_BREAK);    
    
    clearBP (this.topLevel);
    for (var f in this.functions)
        clearBP (this.functions[f]);
}
    
ScriptInstance.prototype.guessFunctionNames =
function si_guessnames ()
{
    var sourceLines = this._sourceText.lines;
    var context = console.prefs["guessContext"];
    var pattern = new RegExp (console.prefs["guessPattern"]);
    var scanText;
    
    function getSourceContext (center)
    {
        var startLine = center - context;
        if (startLine < 0)
            startLine = 0;

        var text;
        
        for (i = startLine; i <= targetLine; ++i)
            text += String(sourceLines[i]);
    
        var pos = text.lastIndexOf ("function");
        if (pos == -1)
            pos = text.lastIndexOf ("get");
        if (pos == -1)
            pos = text.lastIndexOf ("set");
        if (pos != -1)
            text = text.substring(0, pos);
        return text;
    };
        
    for (var i in this.functions)
    {
        var scriptWrapper = this.functions[i];
        if (scriptWrapper.jsdScript.functionName != "anonymous")
            continue;
        
        var targetLine = scriptWrapper.jsdScript.baseLineNumber;
        if (targetLine > sourceLines.length)
        {
            dd ("not enough source to guess function at line " + targetLine);
            return;
        }

        scanText = getSourceContext(targetLine);
        var ary = scanText.match (pattern);
        if (ary)
        {
            scriptWrapper.functionName = getMsg(MSN_FMT_GUESSEDNAME, ary[1]);
            this.isGuessedName = true;
        }
        else
        {
            dd ("unable to guess function name based on text ``" + 
                scanText + "''");
        }
    }

    dispatch ("hook-guess-complete", { scriptInstance: this });
}

function ScriptWrapper (jsdScript)
{
    this.jsdScript = jsdScript;
    this.tag = jsdScript.tag;
    this.functionName = jsdScript.functionName;
    this.breakpointCount = 0;
    this._lineMap = null;
    this.breaks = new Object();
}

ScriptWrapper.prototype.__defineGetter__ ("sourceText", sw_getsource);
function sw_getsource ()
{
    if (!("_sourceText" in this))
        this._sourceText = new PPSourceText(this);
    return this._sourceText;
}

ScriptWrapper.prototype.__defineGetter__ ("lineMap", sw_linemap);
function sw_linemap ()
{
    if (!this._lineMap)
        this.addToLineMap(this._lineMap);
    
    return this._lineMap;
}

ScriptWrapper.prototype.hasBreakpoint =
function sw_hasbp (pc)
{
    var key = this.jsdScript.tag + ":" + pc;
    return key in console.breaks;
}

ScriptWrapper.prototype.setBreakpoint =
function sw_setbp (pc, parentBP)
{
    var key = this.jsdScript.tag + ":" + pc;
    
    dd ("setting breakpoint in " + this.jsdScript.functionName + " " + key);
    
    if (key in console.breaks)
    {
        display (getMsg(MSN_BP_EXISTS, [this.functionName, pc]), MT_ERROR);
        return;
    }

    var brk = {
        parentBP: parentBP,
        scriptWrapper: this,
        pc: pc
    };

    if (parentBP)
    {
        parentBP.childrenBP[key] = brk;
        brk.__proto__ = parentBP;
    }

    if ("_sourceText" in this)
    {
        var line = this.jsdScript.pcToLine(brk.pc, PCMAP_PRETTYPRINT);
        arrayOrFlag (this._sourceText.lineMap, line - 1, LINE_BREAK);
    }

    console.breaks[key] = brk;
    this.breaks[key] = brk;
    
    ++this.scriptInstance.breakpointCount;
    ++this.breakpointCount;
    
    dispatch ("hook-break-set", { breakInstance: brk });
    
    this.jsdScript.setBreakpoint (pc);    
}

ScriptWrapper.prototype.clearBreakpoint =
function sw_clearbp (pc)
{
    var key = this.jsdScript.tag + ":" + pc;
    if (!(key in console.breaks))
    {
        display (getMsg(MSN_ERR_NO_DICE, [this.functionName, pc]), MT_ERROR);
        return;
    }

    var brk = console.breaks[key];

    delete console.breaks[key];
    delete this.breaks[key];

    if (brk.parentBP)
        delete brk.parentBP.childrenBP[key];

    if ("_sourceText" in this)
    {
        var line = this.jsdScript.pcToLine(brk.pc, PCMAP_PRETTYPRINT);
        arrayAndFlag (this._sourceText.lineMap, line - 1, ~LINE_BREAK);
    }
    
    --this.scriptInstance.breakpointCount;
    --this.breakpointCount;

    dispatch ("hook-break-clear", { breakInstance: brk });
    
    this.jsdScript.clearBreakpoint (pc);    
}

ScriptWrapper.prototype.addToLineMap =
function sw_addmap (lineMap)
{    
    var jsdScript = this.jsdScript;
    var end = jsdScript.baseLineNumber + jsdScript.lineExtent;
    for (var i = jsdScript.baseLineNumber; i < end; ++i)
    {
        if (jsdScript.isLineExecutable(i, PCMAP_SOURCETEXT))
            arrayOrFlag (lineMap, i - 1, LINE_BREAKABLE);
    }

    for (i in this.breaks)
    {
        var line = jsdScript.pcToLine(this.breaks[i].pc, PCMAP_SOURCETEXT);
        arrayOrFlag (lineMap, line - 1, LINE_BREAK);
    }
}

function isURLFiltered (url)
{
    return (!url || url == MSG_VAL_CONSOLE ||
            (console.enableChromeFilter && url.indexOf ("chrome:") == 0) ||
            url.indexOf ("x-jsd:internal") == 0);
}    

const EMODE_IGNORE = 0;
const EMODE_TRACE  = 1;
const EMODE_BREAK  = 2;

const TMODE_IGNORE = 0;
const TMODE_TRACE  = 1;
const TMODE_BREAK  = 2;

function debugTrap (frame, type, rv)
{
    var tn = "";
    var retcode = jsdIExecutionHook.RETURN_CONTINUE;

    $ = new Array();
    
    switch (type)
    {
        case jsdIExecutionHook.TYPE_BREAKPOINT:
            tn = MSG_WORD_BREAKPOINT;
            break;
        case jsdIExecutionHook.TYPE_DEBUG_REQUESTED:
            tn = MSG_WORD_DEBUG;
            break;
        case jsdIExecutionHook.TYPE_DEBUGGER_KEYWORD:
            tn = MSG_WORD_DEBUGGER;
            break;
        case jsdIExecutionHook.TYPE_THROW:
            display (getMsg(MSN_EXCP_TRACE, [formatValue(rv.value),
                                             formatFrame(frame)]), MT_ETRACE);
            if (rv.value.jsClassName == "Error")
                display (formatProperty(rv.value.getProperty("message")));

            if (console.throwMode != TMODE_BREAK)
                return jsdIExecutionHook.RETURN_CONTINUE_THROW;

            $[0] = rv.value;
            retcode = jsdIExecutionHook.RETURN_CONTINUE_THROW;
            tn = MSG_WORD_THROW;
            break;
        case jsdIExecutionHook.TYPE_INTERRUPTED:
            var line;
            if (console.prefs["prettyprint"])
                line = frame.script.pcToLine (frame.pc, PCMAP_PRETTYPRINT);
            else
                line = frame.line;
            if (console._stepPast == frame.script.fileName + line)
                return jsdIExecutionHook.RETURN_CONTINUE;
            delete console._stepPast;
            setStopState(false);
            break;
        default:
            /* don't print stop/cont messages for other types */
    }

    console.jsds.functionHook = null;

    /* set our default return value */
    console._continueCodeStack.push (retcode);

    if (tn)
        display (getMsg(MSN_STOP, tn), MT_STOP);
    if (0 in $)
        display (getMsg(MSN_FMT_TMP_ASSIGN, [0, formatValue ($[0])]),
                 MT_FEVAL_OUT);
    
    /* build an array of frames */
    console.frames = new Array(frame);
    
    while ((frame = frame.callingFrame))
        console.frames.push(frame);
    
    console.trapType = type;
    window.focus();
    window.getAttention();

    console.jsds.enterNestedEventLoop({onNest: eventLoopNested}); 
    
    /* execution pauses here until someone calls 
     * console.dbg.exitNestedEventLoop() 
     */

    clearCurrentFrame();
    delete console.frames;
    delete console.trapType;
    rv.value = (0 in $) ? $[0] : null;
    $ = new Array();
    
    console.onDebugContinue();

    dispatch ("hook-debug-continue");

    if (tn)
        display (getMsg(MSN_CONT, tn), MT_CONT);

    return console._continueCodeStack.pop();
}

function eventLoopNested ()
{
    dispatch ("hook-debug-stop");
}

function getCurrentFrame()
{
    return console.frames[console._currentFrameIndex];
}

function getCurrentFrameIndex()
{
    if (typeof console._currentFrameIndex == "undefined")
        return -1;
    
    return console._currentFrameIndex;
}

function setCurrentFrameByIndex (index)
{
    if (!console.frames)
        throw new BadMojo (ERR_NO_STACK);
    
    ASSERT (index >= 0 && index < console.frames.length, "index out of range");

    console._currentFrameIndex = index;
    var cf = console.frames[console._currentFrameIndex];
    console.stopFile = (cf.isNative) ? MSG_URL_NATIVE : cf.script.fileName;
    console.stopLine = cf.line;
    delete console._pp_stopLine;
    return cf;
}

function clearCurrentFrame ()
{
    if (!console.frames)
        throw new BadMojo (ERR_NO_STACK);

    delete console.stopLine;
    delete console._pp_stopLine;
    delete console.stopFile;
    delete console._currentFrameIndex;
}

function formatArguments (v)
{
    if (!v)
        return "";

    var ary = new Array();
    var p = new Object();
    v.getProperties (p, {});
    p = p.value;
    for (var i = 0; i < p.length; ++i)
    {
        if (p[i].flags & jsdIProperty.FLAG_ARGUMENT)
            ary.push (getMsg(MSN_FMT_ARGUMENT,
                             [p[i].name.stringValue,
                              formatValue(p[i].value, FTYPE_SUMMARY)]));
    }
    
    return ary.join (MSG_COMMASP); 
}

function formatFlags (flags)
{
    var s = "";
    
    if (flags & jsdIProperty.FLAG_ENUMERATE)
        s += MSG_VF_ENUMERABLE;
    if (flags & jsdIProperty.FLAG_READONLY)
        s += MSG_VF_READONLY;        
    if (flags & jsdIProperty.FLAG_PERMANENT)
        s += MSG_VF_PERMANENT;
    if (flags & jsdIProperty.FLAG_ALIAS)
        s += MSG_VF_ALIAS;
    if (flags & jsdIProperty.FLAG_ARGUMENT)
        s += MSG_VF_ARGUMENT;
    if (flags & jsdIProperty.FLAG_VARIABLE)
        s += MSG_VF_VARIABLE;
    if (flags & jsdIProperty.FLAG_HINTED)
        s += MSG_VF_HINTED;

    return s;
}

function formatProperty (p, formatType)
{
    if (!p)
        throw new BadMojo (ERR_REQUIRED_PARAM, "p");

    var s = formatFlags (p.flags);

    if (formatType == FTYPE_ARRAY)
    {
        var rv = formatValue (p.value, FTYPE_ARRAY);
        return [p.name.stringValue, rv[1] ? rv[1] : rv[0], rv[2], s];
    }
    
    return getMsg(MSN_FMT_PROPERTY, [s, p.name.stringValue,
                                     formatValue(p.value)]);
}

function formatScript (script)
{
    if (!script)
        throw new BadMojo (ERR_REQUIRED_PARAM, "script");

    return getMsg (MSN_FMT_SCRIPT, [script.functionName, script.fileName]);
}

function formatFrame (f)
{
    if (!f)
        throw new BadMojo (ERR_REQUIRED_PARAM, "f");
    var url = (f.isNative) ? MSG_URL_NATIVE : f.script.fileName;
    return getMsg (MSN_FMT_FRAME,
                   [f.functionName, formatArguments(f.scope), url, f.line]);
}

function formatValue (v, formatType)
{
    if (!v)
        throw new BadMojo (ERR_REQUIRED_PARAM, "v");

    if (!(v instanceof jsdIValue))
        throw new BadMojo (ERR_INVALID_PARAM, "v", String(v));

    var type;
    var value;
        
    switch (v.jsType)
    {
        case jsdIValue.TYPE_BOOLEAN:
            type = MSG_TYPE_BOOLEAN;
            value = String(v.booleanValue);
            break;
        case jsdIValue.TYPE_DOUBLE:
            type = MSG_TYPE_DOUBLE;
            value = v.doubleValue;
            break;
        case jsdIValue.TYPE_INT:
            type = MSG_TYPE_INT;
            value = v.intValue;
            break;
        case jsdIValue.TYPE_FUNCTION:
            type = MSG_TYPE_FUNCTION;
            value = v.jsFunctionName;
            break;
        case jsdIValue.TYPE_NULL:
            type = MSG_TYPE_NULL;
            value = MSG_TYPE_NULL;
            break;
        case jsdIValue.TYPE_OBJECT:
            if (formatType == FTYPE_STD)
            {
                type = MSG_TYPE_OBJECT;
                value = getMsg(MSN_FMT_OBJECT, String(v.propertyCount));
            }
            else
            {
                if (v.jsClassName)
                    if (v.jsClassName == "XPCWrappedNative_NoHelper")
                        type = MSG_CLASS_XPCOBJ;
                    else
                        type = v.jsClassName;
                else
                    type = MSG_TYPE_OBJECT;
                value = "{" + String(v.propertyCount) + "}";
            }
            break;
        case jsdIValue.TYPE_STRING:
            type = MSG_TYPE_STRING;
            value = v.stringValue.quote();
            break;
        case jsdIValue.TYPE_VOID:
            type = MSG_TYPE_VOID;
            value = MSG_TYPE_VOID;            
            break;
        default:
            type = MSG_TYPE_UNKNOWN;
            value = MSG_TYPE_UNKNOWN;
            break;
    }

    if (formatType == FTYPE_SUMMARY)
        return getMsg (MSN_FMT_VALUE_SHORT, [type, value]);

    var className;
    if (v.jsClassName)
        if (v.jsClassName == "XPCWrappedNative_NoHelper")
            /* translate this long, unintuitive, and common class name into
             * something more palatable. */
            className = MSG_CLASS_XPCOBJ;
        else
            className = v.jsClassName;

    if (formatType == FTYPE_ARRAY)
        return [type, className, value];

    if (className)
        return getMsg (MSN_FMT_VALUE_LONG, [type, v.jsClassName, value]);

    return getMsg (MSN_FMT_VALUE_MED, [type, value]);

}

function displayCallStack ()
{
    for (var i = 0; i < console.frames.length; ++i)
        displayFrame (console.frames[i], i);
}

function displayProperties (v)
{
    if (!v)
        throw new BadMojo (ERR_REQUIRED_PARAM, "v");

    if (!(v instanceof jsdIValue))
        throw new BadMojo (ERR_INVALID_PARAM, "v", String(v));

    var p = new Object();
    v.getProperties (p, {});
    for (var i in p.value) display(formatProperty (p.value[i]));
}

function displaySource (url, line, contextLines)
{
    function onSourceLoaded (status)
    {
        if (status == Components.results.NS_OK)
            displaySource (url, line, contextLines);
    }
    
    var rec = console.scripts[url];
 
    if (!rec)
    {
        display (getMsg(MSN_ERR_NO_SCRIPT, url), MT_ERROR);
        return;
    }
    
    var sourceText = rec.sourceText;
    
    if (sourceText.isLoaded)
        for (var i = line - contextLines; i <= line + contextLines; ++i)
        {
            if (i > 0 && i < sourceText.lines.length)
            {
                display (getMsg(MSN_SOURCE_LINE, [zeroPad (i, 3),
                                                  sourceText.lines[i - 1]]),
                         i == line ? MT_STEP : MT_SOURCE);
            }
        }
    else
    {
        sourceText.loadSource (onSourceLoaded);
    }
}
    
function displayFrame (f, idx, showSource)
{
    if (!f)
        throw new BadMojo (ERR_REQUIRED_PARAM, "f");

    if (typeof idx == "undefined")
    {
        for (idx = 0; idx < console.frames.length; ++idx)
            if (f == console.frames[idx])
                break;
    
        if (idx >= console.frames.length)
            idx = MSG_VAL_UNKNOWN;
    }
    
    if (typeof idx == "number")
        idx = "#" + idx;
    
    display(idx + ": " + formatFrame (f));
    if (!f.isNative && f.script.fileName && showSource)
            displaySource (f.script.fileName, f.line, 2);
}

function getBreakpoint (fileName, line)
{
    return console.breakpoints.locateChildByFileLine (fileName, line);
}

function disableBreakpoint (fileName, line)
{
    var bpr = console.breakpoints.locateChildByFileLine (fileName, line);
    if (!bpr)
    {
        display (getMsg(MSN_ERR_BP_NODICE, [fileName, line]), MT_ERROR);
        return 0;
    }
    
    return disableBreakpointByNumber (bpr.childIndex);
}

function disableBreakpointByNumber (number)
{
    var bpr = console.breakpoints.childData[number];
    if (!bpr)
    {
        display (getMsg(MSN_ERR_BP_NOINDEX, number, MT_ERROR));
        return;
    }

    bpr.enabled = false;
    display (getMsg(MSN_BP_DISABLED, [bpr.fileName, bpr.line,
                                      bpr.scriptMatches]));
}

function clearBreakpoint (fileName, line)
{
    var bpr = console.breakpoints.locateChildByFileLine (fileName, line);
    if (!bpr)
    {
        display (getMsg(MSN_ERR_BP_NODICE, [fileName, line]), MT_ERROR);
        return 0;
    }
    
    return clearBreakpointByNumber (bpr.childIndex);
}

function clearBreakpointByNumber (number)
{
    /* XXX breaks view */
    return null;
    
    var bpr = console.breakpoints.childData[number];
    if (!bpr)
    {
        display (getMsg(MSN_ERR_BP_NOINDEX, number, MT_ERROR));
        return null;
    }

    bpr.enabled = false;

    var fileName = bpr.fileName;
    var line = bpr.line;

    var sourceText;
    
    if (fileName in console.scripts)
        sourceText = console.scripts[fileName].sourceText;
    else if (fileName in console.files)
        sourceText = console.files[fileName];

    if (sourceText && sourceText.isLoaded && sourceText.lines[line - 1])
    {
        delete sourceText.lines[line - 1].bpRecord;
        if (console.sourceView.childData.fileName == fileName)
            console.sourceView.tree.invalidateRow (line - 1);
    }
    
    console.breakpoints.removeChildAtIndex(number);
    return bpr;
}
    
function setBreakpoint (fileName, line)
{
    /* XXX breaks view */
    return null;

    var scriptRec = console.scripts[fileName];
    
    if (!scriptRec)
    {
        display (getMsg(MSN_ERR_NOSCRIPT, fileName), MT_ERROR);
        return null;
    }

    var bpr = console.breakpoints.locateChildByFileLine (fileName, line);
    if (bpr)
    {
        display (getMsg(MSN_BP_EXISTS, [fileName, line]), MT_INFO);
        return null;
    }
    
    bpr = new BPRecord (fileName, line);
    
    var ary = scriptRec.childData;
    var found = false;
    
    for (var i = 0; i < ary.length; ++i)
    {
        if (ary[i].containsLine(line) &&
            ary[i].script.isLineExecutable(line, PCMAP_SOURCETEXT))
        {
            found = true;
            bpr.addScriptRecord(ary[i]);
        }
    }

    var matches = bpr.scriptMatches;
    if (!matches)
    {
        display (getMsg(MSN_ERR_BP_NOLINE, [fileName, line]), MT_ERROR);
        return null;
    }
    
    if (scriptRec.sourceText.isLoaded &&
        scriptRec.sourceText.lines[line - 1])
    {
        scriptRec.sourceText.lines[line - 1].bpRecord = bpr;
        if (console.sourceView.childData.fileName == fileName)
            console.sourceView.tree.invalidateRow (line - 1);
    }
    
    console.breakpoints.appendChild (bpr);
    return bpr;
}

function setFutureBreakpoint (filePattern, line)
{
    /* XXX breaks view */
    return null;

    var bpr = console.breakpoints.locateChildByFileLine (filePattern, line);
    if (bpr)
    {
        display (getMsg(MSN_BP_EXISTS, [filePattern, line]), MT_INFO);
        return null;
    }
    
    bpr = new BPRecord (filePattern, line);
    
    if (filePattern in console.files)
    {
        var sourceText = console.files[filePattern];
        if (sourceText.isLoaded)
        {
            sourceText.lines[line - 1].bpRecord = bpr;
            if (console.sourceView.childData.fileName == filePattern)
                console.sourceView.tree.invalidateRow (line - 1);
        }
    }
    
    console.breakpoints.appendChild (bpr);
    return bpr;
}
