/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/IntrospectionLib.js', true)" -*- */
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
 * Collection of introspection functions.
 */

importModule("resource:/jscodelib/StdLib.js");

MOZ_EXPORTED_SYMBOLS = [
  "describe",
  "isDocumented",

  "hasAnyOwnProperties",
  "describeObject",

  "resolveProperty",  
  "describeProperty",
  "describeProperties",

  "describeArray",
  
  "isNativeFunction",
  "isAnonymousFunction",
  "functionSignature",
  "describeFunction",

  "describePrimitive"
];

//----------------------------------------------------------------------
// helpers

// firstLine: return first line of the given string
function firstLine(str) {
  var lines = str.split('\n');
  var retval = lines[0];
  if (lines[1]) retval +="[...]";
  return retval;
}


//----------------------------------------------------------------------
// describe

// context parameter documentation:
_describe_context_doc_ = "\
  layout: 'inline'|'brief'|*'normal'*";

function describe(obj, context) {
  if (obj instanceof Function)
    return describeFunction(obj, context);
  else if (obj instanceof Array)
    return describeArray(obj, context);
  else if (obj instanceof Object)
    return describeObject(obj, context);
  else
    return describePrimitive(obj, context);
}
describe._doc_ = "Returns a string documenting 'obj'.\n\
'obj' can be an object, a function or a primitive value. 'describe' will dispatch to 'describeObject' if 'obj' is a generic object, to 'describeFunction' if 'obj' is a function, to 'describeArray' if 'obj' is an array, or to 'describePrimitive' if 'obj' is a primitive value.\n\
The optional 'context' parameter is an object containing any of the following properties:\n"+_describe_context_doc_+"\n\
Depending on the type of 'obj' other 'context' properties might be appropriate as well. See the corresponding describe* function for details.";

//----------------------------------------------------------------------
// isDocumented

function isDocumented(obj) {
  return (obj._doc_ != null || obj._gendoc_ != null)
}
isDocumented._doc_ = "Returns true if the given object has associated documentation (in property '_doc_') or generates its own documentation (with property '_docgen_').";

//----------------------------------------------------------------------
// Primitive values introspection

function describePrimitive(val, context) {
  if (val===null) return "null";
  if (val===undefined) return "undefined";

  var ctx = { __proto__:context };
  if (!ctx.layout) ctx.layout = "normal";

  var description = typeof val;

  if (ctx.layout == "normal") {
    description += " '"+val.toString()+"'";
  }
  else {
    // brief or inline -> restrict to 1 line:
    description += " '";
    description += firstLine(val.toString());
    description += "'";
  }
  
  return description;
}

//----------------------------------------------------------------------
// Generic object introspection

function hasAnyOwnProperties(obj) {
  for (var p in obj) {
    if (obj.hasOwnProperty(p)) return true;
  }
  return false;
}
hasAnyOwnProperties._doc_ = "Returns true if the object has any own properties.";

function describeObject(obj, context) {
  if (!obj) return "";
  if (obj._docgen_) {
    // object provides its own documentation
    return obj._docgen_(context);
  }

  var ctx = { __proto__:context };
  if (!ctx.layout) ctx.layout = "normal";
  
  var description = "";

  description += "object ";
  if (obj.hasOwnProperty(toString))
    description += obj.toString();

  // properties:
  // don't show for 'inline' or 'brief' layouts
  if (ctx.layout == 'normal') {
    description += "{\n";
    var pctx = {__proto__:ctx, layout:"brief", obj:obj};
    description += describeProperties(obj);
    description += "}";
  }
  
  return description;
}

describeObject._doc_ = "Returns a string documenting the object 'obj'.\n\
The optional 'context' parameter is an object containing any of the following properties:\n"+_describe_context_doc_;

//----------------------------------------------------------------------
// Function introspection

function isNativeFunction(func) {
  return ((func instanceof Function) &&
          func.toString().match(/\[native code\]/)!=null);
}
isNativeFunction._doc_ = "Returns true if 'func' is a native function";


function isAnonymousFunction(func) {
  // func.name is not reliable, since the function might contain a variable 'name'
//  return ((func instanceof Function) &&
//          func.name=="");
  return ((func instanceof Function) &&
          (/^\s*function\s*\w+\s*\(/(func.toString())==null));
}
isAnonymousFunction._doc_ = "Returns true if 'func' is an anonymous function";

function functionSignature(func) {
  var isNative = isNativeFunction(func);
  var isAnonymous = isAnonymousFunction(func);
  
  var signature = "";
  
  if (isNative)
    signature += "native ";
  if (isAnonymous)
    signature += "anonymous ";
  
  signature += "function";

  if (hasAnyOwnProperties(func.prototype))
    signature += "+proto";

  if (!isAnonymous)
    signature += " "+/^\s*function\s*(\w+)\s*\(/(func.toString())[1];

  var arglist = func.toString().match(/\((.*)\)/)[1];

  if (arglist!="") {
    signature += "("+arglist+")";
  }
  else {
    signature += "(";
    var first = true;
    for (var i=0,argc=func.arity;i<argc;++i) {
      if (!first) signature += ", ";
      signature += "_";
      first = false;
    }
    signature += ")";
  }
  
  return signature;
}
functionSignature._doc_ = "Returns the signature of the given function";

function describeFunction(func, context) {
  if (!func) return "";
  if (func._docgen_) {
    // function provides its own documentation
    return func._docgen_(context);
  }

  var ctx = {__proto__:context};
  if (!ctx.layout) ctx.layout = "normal";
  
  var description = functionSignature(func);

  if (func._doc_) {
    if (ctx.layout == 'normal') {
      description += ": "+func._doc_;
    }
    else if (ctx.layout == 'brief') {
      // limit description to first line:
      description += ": "+firstLine(func._doc_);
    }
  }
  
  return description;
}
describeFunction._doc_="Returns a string describing the given function.\n\
Functions can have associated documentation in the property '_doc_', which can either contain a helpstring or a function to generate the description.\n\
The optional 'context' parameter is an object containing any of the following properties:\n"+_describe_context_doc_;

//----------------------------------------------------------------------
// Array introspection

function describeArray(arr, context) {
  if (arr._docgen_) {
    // array provides its own documentation
    return arr._docgen_(context);
  }

  var ctx = {__proto__:context};
  if (!ctx.layout) ctx.layout = "normal";

  var description = "";

  description += "[";
  
  for (var i=0, l=arr.length; i<l; ++i) {
    if (i) {
      description += ", ";
    }
    description += describe(arr[i], {__proto__:ctx, layout:"inline"});
  }

  description += "]";
  return description;
}

//----------------------------------------------------------------------
// Property introspection

// propertypath syntax documentation:
_propertypath_doc_ = "\
PROPERTYPATH = IDENTIFIER                  | \n\
               'PROPERTYNAME'              | \n\
               \"PROPERTYNAME\"              | \n\
               EXPRESSION['PROPERTYNAME']  | \n\
               EXPRESSION[\"PROPERTYNAME\"]  | \n\
               EXPRESSION[NUMBER]          | \n\
               EXPRESSION.IDENTIFIER";

function resolveProperty(propertypath, base) {
  var left, right, match;
  // EXPRESSION.IDENTIFIER
  if ((match = /^(.+)\.\s*((?:\$|\w)+)\s*$/(propertypath))) {
    left = match[1]; right = match[2];
  }
  // EXPRESSION["PROPERTYNAME"]
  else if ((match = /^(.+)\s*\[\s*\"((?:[^\"]|\\.)*)\"\s*\]\s*$/(propertypath))) { 
    left = match[1]; right = match[2];
  }
  // EXPRESSION['PROPERTYNAME']
  else if ((match = /^(.+)\s*\[\s*\'((?:[^\']|\\.)*)\'\s*\]\s*$/(propertypath))) {
    left = match[1]; right = match[2];
  }
  // EXPRESSION[NUMBER]
  else if ((match = /^(.+)\s*\[\s*(\d)*\s*\]\s*$/(propertypath))) {
    left = match[1]; right = match[2];
  }
  // IDENTIFIER
  else if ((match = /^\s*((?:\$|\w)+)\s*$/(propertypath))) {
    right = match[1];
  }
  // "PROPERTYNAME"
  else if ((match = /^\s*\"((?:[^\"]|\\.)*)\"\s*$/(propertypath))) {
    right = match[1];
  }
  // 'PROPERTYNAME'
  else if ((match = /^\s*\'((?:[^\']|\\.)*)\'\s*$/(propertypath))) {
    right = match[1];
  }
  if (!base) base = this.eval("this"); // need this indirection to get the caller's 'this'
  return { obj: (left ? base.eval(left) : base), id:right };
}
resolveProperty._doc_ = "Returns an object {obj,id} such that obj[id] gives the property identified by propertypath relative to 'base' or relative to the calling context's 'this' if base is not given.\n\
The syntax for 'propertypath' is:\n"+_propertypath_doc_;

function describeProperty(propertypath, context) {
  var ctx = { __proto__:context };
  if (!ctx.obj) ctx.obj = this.eval("this"); // need this indirection to get the caller's 'this'
  if (!ctx.layout) ctx.layout = "normal";

  var property = resolveProperty(propertypath, ctx.obj);

  var docgen = property.obj["_docgen_"+property.id+"_"];
  if (docgen) return docgen(context);
  
  var doc = property.obj["_doc_"+property.id+"_"];
  
  var value = property.obj[property.id];
  
  var description = "";
  
  if (value===undefined)
    description = "undefined ";
  else if (!property.obj.hasOwnProperty(property.id))
    description = "inherited ";
           
  description += property.id;

  if (doc) {
    if (value!==undefined) {
      var valuedescription = describe(value,
                                     { __proto__:ctx, layout:"inline"});
      description += " (" + valuedescription + ")";
      description += ": "+doc;
    }
  }
  else if (value!==undefined) {
    // we 'inherit' the property's value doc:
    description += " = " + describe(value,
                                    {__proto__:ctx, layout:"brief"});
  }
  return description;
}
describeProperty._doc_ ="Returns a string describing the given property referenced by 'propertypath'.\n\
The property will be resolved from 'propertypath' using the function 'resolveProperty' starting from the calling context's 'this' object or from 'ctx.obj' if given.\n\
The syntax for 'propertypath' is:\n"+_propertypath_doc_+"\n\
Properties can be documented by providing a documentation string in property '_doc_PROPERTYNAME_' on the property's parent object or a documentation generating function in property '_docgen_PROPERTYNAME'.";


function describeProperties(obj, context) {
  var ctx = { __proto__:context, obj:obj};
  if (!ctx.sort) ctx.sort = 'alphabetical';
  if (!ctx.layout) ctx.layout = 'brief';
  
  // build array of properties:
  var props = [];
  for (var p in obj) {
    if (ctx.ignoreProperties &&
        some(function(r){return r.test(p);}, ctx.ignoreProperties))
      continue;
    if (ctx.onlyProperties &&
        !some(function(r){return r.test(p);}, ctx.onlyProperties))
      continue;
    props.push(p);
  }
  // sort:
  if (ctx.sort == 'alphabetical') {
    props.sort();
  }
  
  // build output:
  var description = "";
  for (var i=0, l=props.length; i<l; ++i) {
    description += describeProperty("'"+props[i]+"'", ctx)+"\n";
  }
  return description;
}
describeProperties._doc_ = "Returns a string listing the given obj's properties.\n\
The optional 'context' parameter is an object containing any of the following properties:\n"+_describe_context_doc_+"\n\
  ignoreProperties: array of RegExps to ignore\n\
  onlyProperties: include only properties matching at least one RegExp in this array\n\
  sortProperties: *'alphabetical'*|'none'";

//----------------------------------------------------------------------
// formatDescriptionText

// var defaultFormatter = {
//   heading : function(txt

// var formatters = {
// };

// function formatDescriptionText(txt, options) {
//   // XXX fix up links, markup headers, etc.
//   return txt;
// }
