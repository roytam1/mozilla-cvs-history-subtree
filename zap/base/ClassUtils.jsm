/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/ClassUtils.jsm', null)" -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/AsyncUtils.jsm");

EXPORTED_SYMBOLS = [ "StdClass",
                     "makeClass",
                     "NamedObject",
                     "ErrorReporter",
                     "ErrorReporterSink",
                     "ConsoleErrorReporterFunction",
                     "getNSPRErrorReportFunction",
                     "SupportsImpl",
                     "AttributeParser",
                     "Unwrappable",
                     "ClassInfoImpl",
                     "PropertyBag",
                     "makePropertyBag",
                     "makePropertyBag2Proxy",
                     "StateMachine",
                     "AsyncObject",
                     "Scheduler",
                     "WeakHash"];

// name our global object:
// function toString() { return "[ClassUtils.jsm]"; }

////////////////////////////////////////////////////////////////////////
// StdClass bootstrapping
//

// StdClass is a class whose instances are classes:
var StdClass = {};

// StdClassProto is the prototype for all classes...
var StdClassProto = {};

// ... including StdClass:
StdClass.__proto__ = StdClassProto;

// StdClass.prototype holds the template for new instances of
// StdClass. Since StdClass generates new classes it is equal to
// StdClassProto:
StdClass.prototype = StdClassProto;

// StdClass.protometa provides meta information (e.g. merge flags)
// about StdClass.prototype members:
StdClass.protometa = {};

StdClass._name_ = "StdClass";

// StdClass.ctorhook holds methods that will be executed on new
// instances:
StdClass.ctorhook = [
  function StdClassCtorHook(args) {
    this._name_ = args.name;
    this.prototype = {};
    this.protometa = {};
    this.ctorhook = [];
    this.obj("_class_", this);
    this.obj({nomerge:true}, "_doc_", {});
  }
];

// c.fun(doc, metadata, fct) is shorthand for:
//   c.prototype['fctname'] = fct
//   c.protometa['fctname'] = metadata
//   c.prototype._doc_['fctname'] = doc
StdClass.prototype.fun = function fun(/*[opt] doc, [opt] metadata, fct*/) {
  var doc, meta, fct;
  if (arguments.length == 1)
    fct = arguments[0];
  else if (arguments.length == 2) {
    fct = arguments[1];
    if (typeof(arguments[0])=="string")
      doc = arguments[0];
    else
      meta = arguments[0];
  }
  else {
    doc = arguments[0];
    meta = arguments[1];
    fct = arguments[2];
  }
  
  var name = fct.name;

  this.prototype[name] = fct;
  if (meta)
    this.protometa[name] = meta;
  if (doc) {
    this.prototype._doc_[name] = doc;
  }
};

// obj(doc, metadata, objname, obj):
// c.obj(doc, metadata, objname, obj) is shorthand for:
//   c.prototype['objname'] = obj
//   c.protometa['objname'] = metadata
//   c.prototype._doc_['fctname'] = doc
StdClass.fun(
  function obj(/*[opt] doc, [opt] metadata, objname, obj*/) {
    var doc, meta, name, obj;
    if (arguments.length == 2) {
      name = arguments[0];
      obj = arguments[1];
    }
    else if (arguments.length == 3) {
      name = arguments[1];
      obj = arguments[2];
      if (typeof(arguments[0])=="string")
        doc = arguments[0];
      else
        meta = arguments[0];
    }
    else {
      doc = arguments[0];
      meta = arguments[1];
      name = arguments[2];
      obj = arguments[3];
    }
    
    this.prototype[name] = obj;
    if (meta)
      this.protometa[name] = meta;
    if (doc) {
      this.prototype._doc_[name] = doc;
    }
  });

// getter(doc, metadata, name, fct) adds a getter function to the
// prototype managed by this class:
StdClass.fun(
  function getter(/*[opt] doc, [opt] metadata, name, fct*/) {
    var doc, meta, name, fct;
    var ai = arguments.length;
    fct = arguments[--ai];
    name = arguments[--ai];
    if (ai==1) {
      if (typeof(arguments[0])=="string")
        doc = arguments[0];
      else
        meta = arguments[0];
    }
    else {
      doc = arguments[0];
      meta = arguments[1];
    }

    this.prototype.__defineGetter__(name, fct);
    if (meta)
      this.protometa[name] = meta;
    if (doc) {
      this.prototype._doc_[name] = doc;
    }
  });

// gettersetter(doc, metadata, name, get_fct, set_fct) adds a
// getter/setter function pair to the prototype managed by this class:
StdClass.fun(
  function gettersetter(/*[opt] doc, [opt] metadata, name, get_fct, set_fct*/) {
    var doc, meta, name, get_fct, set_fct;
    var ai = arguments.length;
    set_fct = arguments[--ai];
    get_fct = arguments[--ai];
    name = arguments[--ai];
    if (ai==1) {
      if (typeof(arguments[0])=="string")
        doc = arguments[0];
      else
        meta = arguments[0];
    }
    else {
      doc = arguments[0];
      meta = arguments[1];
    }
    this.prototype.__defineSetter__(name, set_fct);
    this.prototype.__defineGetter__(name, get_fct);
    if (meta)
      this.protometa[name] = meta;
    if (doc) {
      this.prototype._doc_[name] = doc;
    }
  });

// A function specialization to overwrite an existing function.
// behaves as 'fun'. in addition:
// - asserts that a function with the given name already exists
//   in prototype
// - the old function will be stored as prototype["_"+classname+"_"+name]
// XXX handle old metadata properly
StdClass.fun(
  function spec(/*[opt] doc, [opt] metadata, fct*/) {
    var doc, meta, fct;
    if (arguments.length == 1)
      fct = arguments[0];
    else if (arguments.length == 2) {
      fct = arguments[1];
      if (typeof(arguments[0])=="string")
        doc = arguments[0];
      else
        meta = arguments[0];
    }
    else {
      doc = arguments[0];
      meta = arguments[1];
      fct = arguments[2];
    }
    
    var name = fct.name;

    var old = this.prototype[name];
    this._assert(old, "function "+name+" does not exist - can't specialize");
    this.prototype["_"+this._name_+"_"+name] = old;

    this.prototype[name] = fct;
    if (meta)
      this.protometa[name] = meta;
    if (doc) {
      this.prototype._doc_[name] = doc;
    }
  });

StdClass.fun(
  function mergeClass(c) {
    var me = this;
    function mergeProperty(dest, src, p) {
      var flags = c.protometa[p];
      // skip property if nomerge flag is set:
      if (flags && flags.nomerge===true) return true;
      
      var merged = false;
      var is_getset_prop = hasgettersetter(src, p);
      
      if (hasproperty(dest, p)) {
        // we are merging
        var mergeover = flags ? flags.mergeover : null;
        if (typeof(mergeover) == "function") {
          merged = mergeover(dest, src, p);
        }
        else if (mergeover !== false) {
          // default merging:
          if (!is_getset_prop && isarray(src[p])) {
            arraymerge(dest[p], src[p]);
            merged = true;
          }
          else if (!is_getset_prop && typeof(src[p]) == "object") {
            objmerge(dest[p], src[p]);
            merged = true;
          }
          else
            me._dump("Property "+p+" not merged from "+c);
        }
      }
      else {
        // dest doesn't exist yet -> we are copying
        var mergenew = flags ? flags.mergenew : null;
        if (typeof(mergenew) == "function") {
          merged = mergenew(dest, src, p);
        }
        else if (mergenew !== false) {
          // default copying:
          if (!is_getset_prop && isarray(src[p]))
            dest[p] = arrayclone(src[p]);
          else if (!is_getset_prop && typeof(src[p]) == "object")
            dest[p] = objclone(src[p]);
          else
            propcopy(dest, src, p);
          merged = true;
        }
      }
      if (merged) {
        // we merged the property. now merge in metadata and
        // documentation:
        
        if (flags)
          me.protometa[p] = flags;
        if (c._doc_[p]) {
          me.prototype._doc_[p] = c._doc_[p];
        }
      }
      
      return true; // true means that we handled the merge
    }

    // First we merge the prototypes, using merge information provided
    // in c.protometa. This step will also merge the protometa itself
    // where appropriate:
    objmerge(this.prototype, c.prototype, mergeProperty);
    
    // Then we merge the class object itself:
    //  documentation and ctorhook are treated specially:
    objmerge(this._doc_, c._doc_);
    arraymerge(this.ctorhook, c.ctorhook);
    objmerge(this, c);
  });

StdClass.obj(
  {nomerge:true},
  "_class_",
  StdClass);

StdClass.obj(
  "_doc_",
  {});

//----------------------------------------------------------------------
// class object manipulation:

StdClass.fun(
  "\
 Add a 'meta' property <name> with value <obj> to this class object.",
  function metaobj(/*[opt] doc, name, obj*/) {
    var i = arguments.length;
    var obj = arguments[--i];
    var name = arguments[--i];
    if (i>0)
      this._doc_[name] = arguments[--i];
    this[name] = obj;
      });

StdClass.fun(
  "\
 Add a meta function <fct>, i.e. a method that operates on the class\n\
 rather than instances of this class                                 ",  
  function metafun(/*[opt] doc, fct*/) {
    var doc, fct;
    if (arguments.length == 1)
      fct = arguments[0];
    else {
      doc = arguments[0];
      fct = arguments[1];
    }

    var name = fct.name;

    this[name] = fct;
    if (doc) {
      this._doc_[name] = doc;
    }
  });

//----------------------------------------------------------------------
// instantiation

StdClass.fun(
  function appendCtor(fct) {
    this.ctorhook.push(fct);
  });

StdClass.fun(
  "\
 Create an instance of the class. Optional arguments will be passed  \n\
 to each of the classes constructors. The new instance will have the \n\
 same parent (scope, __parent__ property) as the class, and its      \n\
 prototype (__proto__ property) will be set to class.prototype.       ",
  function instantiate(/*[opt] arg1, [opt] arg2, [opt] arg3, ... */) {
    // Ensure that new instances get the same parent as the class
    // object:
    var inst = new this.__parent__.Object();
    inst.__proto__ = this.prototype;
    var args = arguments;
    this.ctorhook.forEach(function(f) {f.apply(inst, args);});
    return inst;
  });

StdClass.fun(
  "\
 This function can be used to create an instance of the class and   \n\
 set its parent (scope, __parent__ property) to 'parent'.            ",
  function instantiateWithParent(parent, args) {
    var inst = new parent.Object();
    inst.__proto__ = this.prototype;
    this.ctorhook.forEach(function(f) {f.apply(inst, args);});
    return inst;
  });

function makeClass(name /*, [opt] base classes to merge */) {
  // Make sure the new class's parent is set to the (global-)object
  // that makeClass() was called on:
  //
  // Note on the scope chain:
  // In most cases the parent chain of objects is irrelevant. One
  // notable exception is the xpconnect wrapping of native objects:
  // The same native object will get wrapped differently depending
  // on the global object of the caller that causes the object to be
  // wrapped.
  // This can cause COM identity problems (and inefficiencies) if we are
  // not careful about object's parent chains. E.g. consider a class
  // foo, defined in module [foo.jsm]. If foo was created with instantiate(),
  // rather than instantiateWithParent, its parent would be [ClassUtils.jsm]
  // rather than [foo.jsm]. Its methods, however, defined in [foo.jsm] would
  // have the parent [foo.jsm].
  // Consider a method on foo that creates a new timer, sets foo itself
  // as callback and then stores the timer in a variable T. Since foo's
  // method's have [foo.jsm] as parent, T will be wrapped for [foo.jsm].
  // When the timer fires, it calls foo.notify and passes itself as
  // argument. In wrapping this argument, xpconnect examines foo for
  // its global object. The timer argument will thus be wrapped for
  // [ClassUtils.jsm]. If the foo.notify() now compares the argument
  // against T, the objects will appear as not equal, because they
  // have different wrappers!
  // To avoid these problems, it is usually sufficient to ensure that
  // a class's methods have the same global object as the class and
  // its instances. (Merged methods can still have a different global
  // object, but that's usually ok.))
  var new_class = StdClass.instantiateWithParent(this, [{name:name}]);
  for (var i=1, l=arguments.length; i<l ; ++i)
    new_class.mergeClass(arguments[i]);
  return new_class;
}


////////////////////////////////////////////////////////////////////////
// Class NamedObject

var NamedObject = makeClass("NamedObject");

NamedObject.fun(
  {mergeover:propcopy}, // need mergeover flag here, since toString is
                        // present in all objects
  function toString() {
    if (this._name_)
      return "["+this._name_+"]";
    else if (this._class_.prototype == this)
      return "["+this._class_.toString()+".prototype]";
    else
      return "["+this._class_.toString()+" instance@"+Components.utils.getObjectId(this)+"]";
  });


////////////////////////////////////////////////////////////////////////
// Class ErrorReporter

var ErrorReporter = makeClass("ErrorReporter", NamedObject);
ErrorReporter.appendCtor(
  function() {
    this._logName_ = this._class_.toString().replace(/[\[\]]+/g,"");
  });

var gVerboseErrorService;
function getVerboseErrorService() {
  if (!gVerboseErrorService) {
    // create via xpcom:
    //gVerboseErrorService = Components.classes["@mozilla.org/zap/verbose-error-reporter;1"].getService(Components.interfaces.zapIVerboseErrorService);

    // create via js:
    gVerboseErrorService = Components.utils.import("resource://gre/components/zapVerboseErrorService.js", null).theVerboseErrorService;
  }
  return gVerboseErrorService;
}

// The standard error reporter sink, directing messages to the
// console:
var ConsoleErrorReporterFunction = function (msg, logName) { dump(msg); };

// ErrorReporterSink: The function referenced by this global exported
// variable will be used to sink messages from the
// ErrorReporter. Users can set this to a different function to
// redirect messages to e.g. a file:
var ErrorReporterSink = { reporterFunction : ConsoleErrorReporterFunction };


// NSPR Logging facility
var gLogUtils;

var NSPRErrorReporterFunction = function(msg, logName) {
  gLogUtils.logMessage(logName || "class-none", msg);
}

function getNSPRErrorReportFunction() {
  try {
    gLogUtils = Components.classes["@mozilla.org/zap/logutils;1"].getService(Components.interfaces.zapILogUtils);
    if (!gLogUtils) {
      dump("ERROR: Could not get logutils service!\n");
    } else  if (gLogUtils.loggingEnabled) {
      return NSPRErrorReporterFunction;
    }
  } catch (e) {
    dump("ERROR: An error occured while enabling NSPR logging: " + e + "\n");
  }
  return function() {};
}

ErrorReporter.fun(
  {mergeover:false},
  function _verboseError(message) {
    ErrorReporterSink.reporterFunction(this+"::"+Components.stack.caller.name+
                                       ": Verbose Error: "+message+"\n",
                                       this._logName_);
    throw(getVerboseErrorService().setVerboseErrorMessage(message));
  });

ErrorReporter.fun(
  {mergeover:false},
  function _error(message) {
    ErrorReporterSink.reporterFunction(this+"::"+Components.stack.caller.name+
                                       ": ERROR: "+message+"\n",
                                       this._logName_);
    throw(this+"::"+Components.stack.caller.name+": ERROR: "+message);
  });

ErrorReporter.fun(
  {mergeover:false},
  function _assert(cond, message) {
    if (!cond) {
      ErrorReporterSink.reporterFunction(this+"::"+Components.stack.caller.name+
                                         ": ASSERTION FAILED: "+message+"\n",
                                         this._logName_);
      throw(this+"::"+Components.stack.caller.name+": ASSERTION FAILED: "+message);
    }
  });

ErrorReporter.fun(
  {mergeover:false},
  function _warning(message) {
    ErrorReporterSink.reporterFunction(this+"::"+Components.stack.caller.name+
                                       ": WARNING: "+message+"\n",
                                       this._logName_);
  });

ErrorReporter.fun(
  {mergeover:false},
  function _dump(message) {
    ErrorReporterSink.reporterFunction(this+"::"+Components.stack.caller.name+
                                       ": "+message+"\n", this._logName_);
  });

ErrorReporter.fun(
  {mergeover:false},
  function _backtrace(offset, max_depth) {
    var str = "";
    if (!offset) offset = 1;
    if (!max_depth) max_depth = 10;
    var frame = Components.stack;
    for (var i=0; (i<max_depth) && frame; frame=frame.caller, ++i) {
      if (i >= offset) {
        str += "["+i+"] "+frame+"\n";
      }
    }
    return str;
  });

ErrorReporter.metafun(
  "\
 Create a prototype stub function 'name' that will throw an error     \n\
 'message' when called.                                                ",
  function stub(name, message) {
    this.prototype[name] = function() {
      ErrorReporterSink.reporterFunction(this+"::stub '"+name+"': "+message+"\n",
                                         this._logName_);
      throw(this+"::stub '"+name+"': "+message);
    };
  });

ErrorReporter.metafun(
  "\
 Create a prototype stub getter for property 'name' that will throw an \n\
 error 'message' when the property is being accessed.                   ",
  function stub_getter(name, message) {
    this.prototype.__defineGetter__(name,
                                    function() {
                                      ErrorReporterSink.reporterFunction(
                                        this+"::stub_getter '"+name+"': "+message+"\n",
                                        this._logName_);
                                      throw(this+"::stub_getter '"+name+"': "+message);
                                    });
  });

// merge into StdClass:
StdClass.mergeClass(ErrorReporter);


////////////////////////////////////////////////////////////////////////
// Class SupportsImpl

var SupportsImpl = makeClass("SupportsImpl", ErrorReporter);

SupportsImpl.obj(
  "interfaces",
  [Components.interfaces.nsISupports]);

var NS_ERROR_NO_INTERFACE = Components.results.NS_ERROR_NO_INTERFACE;

SupportsImpl.fun(
  function QueryInterface(iid) {
    var itfs = this.interfaces;
    this._assert(iid, "null interface");
    for (var i=0, l=itfs.length; i<l; ++i) {
      if (itfs[i].equals(iid)) return this;
    }
    throw NS_ERROR_NO_INTERFACE;
  });

SupportsImpl.metafun(
  "\
 Add interfaces 'itf1',... to the prototype's interface list.         ",
  function addInterfaces(/* itf1, itf2, ... */) {
    arraymerge(this.prototype.interfaces, arguments);
  });


////////////////////////////////////////////////////////////////////////
// Class AttributeParser

var AttributeParser = makeClass("AttributeParser", ErrorReporter);

AttributeParser.metafun(
  "\
 Adds a getter/setter pair for 'attrib' and a variable with name     \n\
 '_attrib' to the prototype. The setter will perform a syntax check  \n\
 using 'regex' and throw an error if the syntax is invalid. A null   \n\
 argument will be converted to the empty string prior to the syntax  \n\
 check.                                                               ",
  function parsedAttrib(/*[opt] doc, attrib, regex, default*/) {
    var i = arguments.length-1;
    var defval = arguments[i--];
    var regex = arguments[i--];
    var attrib = arguments[i--];

    // optional documentation:
    if (i>0)
      this.prototype._doc_[attrib] = arguments[i--];

    // add a property '_attrib':
    this.obj("_"+attrib, defval);

    // construct getter and setter for 'attrib':
    var getter, setter;
    eval("getter = function "+attrib+"_get() { return this._"+attrib+";}");
    eval("setter = function "+attrib+"_set(v) { "+
         "if (v==null) v='';"+
         "if (!regex.test(v)) this._error('Attribute parse error'); "+
         "return this._"+attrib+"=v;}");

    // install getter/setter for 'attrib':
    this.gettersetter(
      attrib,
      getter,
      setter
      );
  });


////////////////////////////////////////////////////////////////////////
// Class Unwrappable: allows clients to get access to the underlying
// JS object if it has been wrapped by XPConnect using
// 'wrappedObj.wrappedJSObject'. See nsIXPConnect.idl for more details.

var Unwrappable = makeClass("Unwrappable");

Unwrappable.getter("wrappedJSObject", function() { return this; });

////////////////////////////////////////////////////////////////////////
// Class ClassInfoImpl: implements nsIClassInfo
var ClassInfoImpl = makeClass("ClassInfoImpl", SupportsImpl);
ClassInfoImpl.addInterfaces(Components.interfaces.nsIClassInfo);

ClassInfoImpl.metafun(
  function markThreadsafe() {
    this.prototype.flags |= Components.interfaces.nsIClassInfo.THREADSAFE;
  });

ClassInfoImpl.fun(
  function getInterfaces(count) {
    // expose all of our interfaces from SupportsImpl::interfaces array:
    if (count) count.value = this.interfaces.length;
    return this.interfaces;
  });

ClassInfoImpl.fun(
  function getHelperForLanguage(language) {
    return null;
  });

ClassInfoImpl.obj("contractID", null);

ClassInfoImpl.getter(
  "classDescription",
  function get_classDescription() {
    return this._class_.toString();
  });

ClassInfoImpl.obj("classID", null);

ClassInfoImpl.obj("implementationLanguage",
                  Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT);

ClassInfoImpl.obj("flags", 0);


////////////////////////////////////////////////////////////////////////
// Class PropertyBag: a class implementing nsIPropertyBag, 
// nsIPropertyBag2 and nsIWritablePropertyBag.
// Values are stored as properties on the instance, with keys prefixed
// by '$' (see ObjectUtils.jsm hash functions).

var PropertyBag = makeClass("PropertyBag", SupportsImpl);
PropertyBag.addInterfaces(Components.interfaces.nsIPropertyBag,
                          Components.interfaces.nsIPropertyBag2,
                          Components.interfaces.nsIWritablePropertyBag);

//----------------------------------------------------------------------
// nsIPropertyBag implementation:

var NS_ERROR_FAILURE = Components.results.NS_ERROR_FAILURE;

/* readonly attribute nsISimpleEnumerator enumerator; */
PropertyBag.getter(
  "enumerator",
  function get_enumerator() {
    var keys = hashkeys(this);
    var bag = this;
    var i = 0;
    return { // object implementing nsISimpleEnumerator
      hasMoreElements : function() { return i<keys.length; },
      getNext : function() {
        if (i>keys.length) throw NS_ERROR_FAILURE;
        var prop = { name: keys[i], value: bag.getProperty(keys[i]) };
        ++i;
        return prop;
      }          
    };
  });

/* nsIVariant getProperty(in AString name); */
PropertyBag.fun(
  function getProperty(key) {
    var val = hashget(this, key);
    if (val === undefined) throw(NS_ERROR_FAILURE);
    return val;
  });

//----------------------------------------------------------------------
// nsIPropertyBag2 implementation:
// XXX these methods will not perform type conversion when called
// directly from JS

var NS_ERROR_NOT_AVAILABLE = Components.results.NS_ERROR_NOT_AVAILABLE;
// implementation helper for getter methods. Note that this differs
// from getProperty in the error thrown if the property is not found.
PropertyBag.fun(
  function _getProperty(key) {
    var val = hashget(this, key);
    if (val === undefined) throw(NS_ERROR_NOT_AVAILABLE);
    return val;
  });

/* PRInt32     getPropertyAsInt32       (in AString prop); */
PropertyBag.obj("getPropertyAsInt32", PropertyBag.prototype._getProperty);

/* PRUint32    getPropertyAsUint32      (in AString prop); */
PropertyBag.obj("getPropertyAsUint32", PropertyBag.prototype._getProperty);

/* PRInt64     getPropertyAsInt64       (in AString prop); */
PropertyBag.obj("getPropertyAsInt64", PropertyBag.prototype._getProperty);

/* PRUint64    getPropertyAsUint64      (in AString prop); */
PropertyBag.obj("getPropertyAsUint64", PropertyBag.prototype._getProperty);

/* double      getPropertyAsDouble      (in AString prop); */
PropertyBag.obj("getPropertyAsDouble", PropertyBag.prototype._getProperty);

/* AString     getPropertyAsAString     (in AString prop); */
PropertyBag.obj("getPropertyAsAString", PropertyBag.prototype._getProperty);

/* ACString    getPropertyAsACString    (in AString prop); */
PropertyBag.obj("getPropertyAsACString", PropertyBag.prototype._getProperty);

/* AUTF8String getPropertyAsAUTF8String (in AString prop); */
PropertyBag.obj("getPropertyAsAUTF8String", PropertyBag.prototype._getProperty);

/* boolean     getPropertyAsBool        (in AString prop); */
PropertyBag.obj("getPropertyAsBool", PropertyBag.prototype._getProperty);

/*  void       getPropertyAsInterface   (in AString prop,
                                        in nsIIDRef iid,
                                        [iid_is(iid), retval] out nsQIResult result); */
PropertyBag.fun(
  function getPropertyAsInterface(key, iid) {
    return this._getProperty(key);
  });

//PropertyBag.obj("getPropertyAsInterface", PropertyBag.prototype.getProperty);

/* nsIVariant  get                      (in AString prop); */
PropertyBag.fun(
  function get(prop) {
    var val = hashget(this, key);
    if (val === undefined) val = null;
    return val;
  });    

/*  PRBool      hasKey                   (in AString prop); */
PropertyBag.fun(
  function hasKey(prop) {
    return (hashget(this, key) !== undefined);
  });

//----------------------------------------------------------------------
// nsIWritablePropertyBag implementation:

/* void setProperty(in AString name, in nsIVariant value); */
PropertyBag.fun(
  function setProperty(key, value) {
    hashset(this, key, value);
  });

/* void deleteProperty(in AString name); */
PropertyBag.fun(
  function deleteProperty(key) {
    if (!hashhas(this, key))
      throw(NS_ERROR_FAILURE);
    hashdel(this, key);
  });

//----------------------------------------------------------------------

// wrap 'obj' with nsIPropertyBag/nsIWritablePropertyBag interfaces
function makePropertyBag(obj) {
  obj.__proto__ = PropertyBag.prototype;
  return obj;
}

//----------------------------------------------------------------------

// In theory it should be enough to implement nsIClassInfo and mark
// PropertyBag as THREADSAFE, to get a threadsafe version. In practice
// xpconnect appears to choke up and there are problems with the
// marshalling of the iid parameter of getPropertyAsInterface()
//
// var PropertyBagThreadsafe = makeClass("PropertyBagThreadsafe",
//                                       PropertyBag,
//                                       ClassInfoImpl);
// PropertyBagThreadsafe.markThreadsafe();

// // wrap 'obj' with nsIPropertyBag/nsIWritablePropertyBag interfaces
// function makePropertyBagTS(obj) {
//   obj.__proto__ = PropertyBagThreadsafe.prototype;
//   return obj;
// }

function makePropertyBag2Proxy(obj) {
  obj.__proto__ = PropertyBag.prototype;
  return getSyncProxyOnMainThread(obj, Components.interfaces.nsIPropertyBag2);
}

////////////////////////////////////////////////////////////////////////
// Class StateMachine

var StateMachine = makeClass("StateMachine", ErrorReporter);

StateMachine.obj("currentState", "*");

StateMachine.obj("states", []);

StateMachine.fun(
  function changeState(state) {
    this.currentState = state;
  });

StateMachine.metafun(
  function statefun(state, fct) {
    var name = fct.name;
    
    // create a dispatcher function if neccessary:
    if (!this.prototype[name]) {      
      this.prototype[name] = function() {
        if (this.states[this.currentState] &&
            this.states[this.currentState][name])
          return this.states[this.currentState][name].apply(this, arguments);
        else if (this.states["*"] && this.states["*"][name])
          return this.states["*"][name].apply(this, arguments);
        else
          this._error("StateMachine "+this+": "+name+" not found in state "+this.currentState);
      }
    }

    // create state if neccessary:
    if (!this.prototype.states[state]) this.prototype.states[state] = {};

    // define state function:
    this.prototype.states[state][name] = fct;
  });

////////////////////////////////////////////////////////////////////////
// Class AsyncObject
// A class with some facilities for asynchronous notifications

var AsyncObject = makeClass("AsyncObject", ErrorReporter);

AsyncObject.metafun(
  function addCondition(/*[opt] doc, condition*/) {
    var i = arguments.length-1;
    var condition = arguments[i--];

    // optional documentation:
    if (i>0)
      this.prototype._doc_[attrib] = arguments[i--];

    // add a property '_condition':
    this.obj("_"+condition, false);

    // add a property '_conditionHook':
    this.obj("_"+condition+"Hook", null);

    // install a constructor to initialize the hook to an empty array:
    var f;
    eval("f = function() { this._"+condition+"Hook = [] };");
    this.appendCtor(f);
    
    // install condition setter/getter:
    var s,g;
    eval("s = function set"+condition+"(v) {"+
         "  if (this._Terminated) return; "+
         "  this._"+condition+"=v;"+
         "  if (!v) return; "+
         "  var me = this; "+
         "  this._"+condition+"Hook.forEach(function(a) {a.apply(me);});"+
         "  this._"+condition+"Hook = []; };");
    this.fun(f);

    eval("g = function get"+condition+"() {"+
         "  return this._"+condition+";};");
    
    this.gettersetter(condition, g, s);

    // install 'whenCondition':
    eval("f = function when"+condition+"(fct) {"+
         "  if (this._"+condition+")"+
         "    fct.apply(this);"+
         "  else"+
         "    this._"+condition+"Hook.push(fct);};");
    this.fun(f);    
  });


// When Terminated=true, no more hooks will be executed:
AsyncObject.addCondition("Terminated");
// XXX maybe clear pending functions on hooks when we enter the
// Terminated condition. This would aid GC in the face of XPCOM<->JS
// reference cycles.


////////////////////////////////////////////////////////////////////////
// Class Scheduler
// A class with facilites for asynchronous scheduling of member calls.
// All active schedules will automatically be descheduled on
// termination (i.e. when scheduleObj.Terminated is set to 'true').

var Scheduler = makeClass("Scheduler", AsyncObject);

Scheduler.appendCtor(
  function() {
    this._activeSchedules = {};
    var me = this;
    this.whenTerminated(this.cancelAllSchedules);
  });

// schedules a call of 'method'
Scheduler.fun(
  function schedule(method, interval, args) {
    var me = this;
    var scheduleId;
    var timer = makeOneShotTimer(
      {
        notify : function(timer) {
          delete me._activeSchedules[scheduleId];
          method.apply(me, args);
        }
      },
      interval);
    scheduleId = Components.utils.getObjectId(timer);
    this._activeSchedules[scheduleId] = [timer,
                                         Components.utils.getObjectId(method)];
    
    // this next line is important to prevent a COM reference cycle:
    // (since timers don't clear their callback reference)
    timer = null;
    
    return scheduleId;
  });

Scheduler.fun(
  function cancelSchedulesForMethod(method) {
    var methodId = Components.utils.getObjectId(method);
    for (var s in this._activeSchedules) {
      if (this._activeSchedules[s][1] == methodId) {
        this._activeSchedules[s][0].cancel();
        delete this._activeSchedules[s];
      }
    }
  });

Scheduler.fun(
  function cancelAllSchedules() {
    for (var s in this._activeSchedules) {
      this._activeSchedules[s][0].cancel();
      delete this._activeSchedules[s];
    }
  });

////////////////////////////////////////////////////////////////////////
// Class WeakHash

var WeakHash = makeClass("WeakHash", ErrorReporter);

// The hash will be checked for stale references after 'reapThreshold'
// set() calls:
WeakHash.obj("reapThreshold", 100);

WeakHash.obj("_hashSize", 0);

WeakHash.fun(
  function reapStaleReferences() {
    this._hashSize = 0;
    var hash = this;
    hashmap(this, function(key, val) {
              if (!val.get())
                hashdel(hash, key);
            });
  });

WeakHash.fun(
  function set(key, value) {
    if (++this._hashSize > this.reapThreshold)
      this.reapStaleReferences();
    hashset(this, key, Components.utils.getWeakReference(value));
  });

WeakHash.fun(
  function get(key) {
    var val = hashget(this, key);
    if (!val || !(val = val.get())) return null;
    return val;
  });

WeakHash.fun(
  function getKeys() {
    return hashkeys(this);
  });

WeakHash.fun(
  function remove(key) {
    --this._hashSize;
    hashdel(this, key);
  });

