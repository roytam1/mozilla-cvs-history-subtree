/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.util.importModule('resource:/jscodelib/zap/ClassUtils.js', null)" -*- */
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

Components.util.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.util.importModule("resource:/jscodelib/zap/ObjectUtils.js");

EXPORTED_SYMBOLS = [ "StdClass",
                     "makeClass",
                     "NamedObject",
                     "ErrorReporter",
                     "SupportsImpl",
                     "AttributeParser",
                     "Unwrappable" ];

// name our global object:
function toString() { return "[ClassUtils.js]"; }

// object to hold module's documentation:
var _doc_ = {};

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
    objmerge(this.ctorhook, c.ctorhook);
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
    amap(function(f) {f.apply(inst, args);}, this.ctorhook);
    return inst;
  });

StdClass.fun(
  "\
 This function can be used to create an instance of the class and   \n\
 set its parent (scope, __parent__ property) to 'parent'.            ",
  function instantiateWithParent(parent, args) {
    var inst = new parent.Object();
    inst.__proto__ = this.prototype;
    amap(function(f) {f.apply(inst, args);}, this.ctorhook);
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
  // foo, defined in module [foo.js]. If foo was created with instantiate(),
  // rather than instantiateWithParent, its parent would be [ClassUtils.js]
  // rather than [foo.js]. Its methods, however, defined in [foo.js] would
  // have the parent [foo.js].
  // Consider a method on foo that creates a new timer, sets foo itself
  // as callback and then stores the timer in a variable T. Since foo's
  // method's have [foo.js] as parent, T will be wrapped for [foo.js].
  // When the timer fires, it calls foo.notify and passes itself as
  // argument. In wrapping this argument, xpconnect examines foo for
  // its global object. The timer argument will thus be wrapped for
  // [ClassUtils.js]. If the foo.notify() now compares the argument
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
      return "[Instance of "+this._class_.toString()+"]";
  });


////////////////////////////////////////////////////////////////////////
// Class ErrorReporter

var ErrorReporter = makeClass("ErrorReporter", NamedObject);

ErrorReporter.fun(
  function _error(message) {
    throw(this+"::"+Components.stack.caller.name+": "+message);
  });

ErrorReporter.fun(
  function _assert(cond, message) {
    if (!cond)
      throw(this+"::"+Components.stack.caller.name+": "+message);
  });

ErrorReporter.fun(
  function _warning(message) {
    dump(this+"::"+Components.stack.caller.name+": WARNING: "+message+"\n");
  });

ErrorReporter.fun(
  function _dump(message) {
    dump(this+"::"+Components.stack.caller.name+": "+message+"\n");
  });

ErrorReporter.metafun(
  "\
 Create a prototype stub function 'name' that will throw an error     \n\
 'message' when called.                                                ",
  function stub(name, message) {
    this.prototype[name] = function() {
      throw(this+"::stub '"+name+"': "+message);
    };
  });

// merge into StdClass:
StdClass.mergeClass(ErrorReporter);


////////////////////////////////////////////////////////////////////////
// Class SupportsImpl

var SupportsImpl = makeClass("SupportsImpl", ErrorReporter);

SupportsImpl.obj(
  "interfaces",
  [Components.interfaces.nsISupports]);

SupportsImpl.fun(
  function QueryInterface(iid) {
    var itfs = this.interfaces;
    this._assert(iid, "null interface");
    for (var i=0, l=itfs.length; i<l; ++i) {
      if (itfs[i].equals(iid)) return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
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
 '_attrib' to the prototype. The setter will peform a syntax check   \n\
 using 'regex' and throw an error if the syntax is invalid.           ",
  function parsedAttrib(/*[opt] doc, attrib, regex, [opt] default*/) {
    var i = arguments.length-1;
    var defval = null;
    if (typeof(arguments[i])!="function")
      defval = arguments[i--];
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

