/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/TemplateLib.js', true)" -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is the Mozilla code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

/**
 * Template utilities.
 *
 * 
 */

importModule("resource:/jscodelib/StdLib.js");

MOZ_EXPORTED_SYMBOLS = [ "Template",
                         "makeTemplate",
                         "NamedObjectTemplate",
                         "ErrorTemplate",
                         "SupportsTemplate" ];


//**********************************************************************
// Template

//----------------------------------------------------------------------
// Template constructor

function Template(name) {
  this.name = name;
  this._proto = {__template:this};
  this._protoflags = {__template:{nomerge:true}};
  this._initializers = [];
  this._ops = {};
}


//----------------------------------------------------------------------
// Template prototype:

Template.prototype.name = "Template Prototype";

Template.prototype.toString = function() {
  return "[Template "+this.name+"]";
};

Template.prototype.__noSuchMethod__ = function(id, args) {
  if (this._ops[id])
    this._ops[id].apply(this, args);
  else
    throw("Unknown method: "+id);
};

Template.prototype.instantiate = function(/*[optional] args */) {
  var obj = {};
  obj.__proto__ = this._proto;
  
  // call initializers.
  var l = this._initializers.length;
  for (var i=0; i<l; ++i)
    this._initializers[i].fct.apply(obj, arguments);

  return obj;
};

Template.prototype.appendInitializer = function(name, fct) {
  this._initializers.push({"name":name, "fct":fct});
};

Template.prototype.addPrototypeObject = function(name, obj, /*[optional]*/ flags) {
  this._proto[name] = obj;
  if (flags)
    this._protoflags[name] = flags;
};

Template.prototype.getPrototypeObject = function(name) {
  return this._proto[name];
};

Template.prototype.addPrototypeGetter = function(name, fct, /*[optional]*/ flags) {
  this._proto.__defineGetter__(name, fct);
  if (flags)
    this._protoflags[name] = flags;
};

Template.prototype.addOpsObject = function(name, obj) {
  this._ops[name] = obj;
};

Template.prototype.mergeTemplate = function(src /* ,src2, src3, ... */) {
  for (var i=0; i<arguments.length; ++i) {
    var src = arguments[i];
    // copy properties of src proto (& flags) to target proto:
    for (var p in src._proto) {
      var flags = src._protoflags[p];
      if (flags && flags.nomerge) continue;
      if (flags)
        this._protoflags[p] = flags;
      
      var target = this._proto[p];
      if (target && flags && flags.mergeover) {
        if (typeof(flags.mergeover)=="function") {
          flags.mergeover(this._proto[p], src._proto[p]);
          continue;
        }
        else if (flags.mergeover!=true) {
          continue;
        } // else merge forced even though property exists
      }
      // getters & setters need some tlc:
      var getter = src._proto.__lookupGetter__(p);
      var setter = src._proto.__lookupSetter__(p);
      if (setter || getter) {
        if (getter) this._proto.__defineGetter__(p, getter);
        if (setter) this._proto.__defineSetter__(p, setter);
      }
      else
        this._proto[p] = src._proto[p];
    }
    // and ditto for initializers:
    appendUnique(this._initializers, src._initializers, function(a,b){return a.name==b.name;});

    // and ditto for ops:
    for (var o in src._ops) { this._ops[o] = src._ops[o]; }
  }
};

//----------------------------------------------------------------------
// Global Template functions:

function makeTemplate(name) {
  return new Template(name);
};

//----------------------------------------------------------------------
// NamedObjectTemplate

var NamedObjectTemplate = makeTemplate("NamedObjectTemplate");

NamedObjectTemplate.addPrototypeGetter("objname",
  function() {
    return "Instance of "+this.__template.name;
  });

NamedObjectTemplate.addPrototypeObject("toString",
  function() {
    return "["+this.objname+"]";
  });

//----------------------------------------------------------------------
// ErrorTemplate

var ErrorTemplate = makeTemplate("ErrorTemplate");

ErrorTemplate.mergeTemplate(NamedObjectTemplate);

ErrorTemplate.addPrototypeObject("error",
  function(message) {
    throw(this.objname+": "+message);
  });

ErrorTemplate.addPrototypeObject("warning",
  function(message) {
    dump(this.objname+": "+message);
  });

    
//----------------------------------------------------------------------
// nsISupports-implementation template

var SupportsTemplate = makeTemplate("SupportsTemplate");

SupportsTemplate.mergeTemplate(ErrorTemplate);

SupportsTemplate.addPrototypeObject("QueryInterface",
  function(iid) {
    var l = this.interfaces.length;
    for (var i=0; i<l; ++i) {
      if (iid.equals(this.interfaces[i]))
        return this;
    }
    this.warning("Interface "+iid+" not found");
    throw Components.results.NS_ERROR_NO_INTERFACE;
  });

SupportsTemplate.addPrototypeObject("interfaces",
                                    [Components.interfaces.nsISupports],
                                    { mergeover: appendUnique });
                                    

SupportsTemplate.addOpsObject("addInterface", function(itf) {
  this.getPrototypeObject("interfaces").push(itf);
});
