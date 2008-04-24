/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/DocUtils.jsm', null)" -*- */
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

Components.utils.import("resource://gre/components/FunctionUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");

EXPORTED_SYMBOLS = [ "describe",
                     "gendoc" ];

//----------------------------------------------------------------------
// Generic object introspection

defun(
  "Returns true if the object has any properties not inherited from its prototype.",
  function hasAnyOwnProperties(obj) {
    for (let p in obj) {
      if (obj.hasOwnProperty(p)) return true;
    }
    return false;
  });

//----------------------------------------------------------------------
// Function introspection:

defun(
  "Returns true if 'fun' is an anonymous function.",
  function isAnonymousFunction(fun) {
    return ((typeof fun == "function") && fun.name == "");
  });

defun(
  "Returns true if 'fun' is a native function.",
  function isNativeFunction(fun) {
    return ((typeof fun == "function") &&
            fun.toString().match(/\[native code\]/)!=null);
  });

defun(
  "Returns true if 'fun' is a regular expression.",
  function isRegexp(fun) {
    return (fun.__proto__.toString() == "/(?:)/");
  });

defun(
  function functionName(fun) {
    if (isRegexp(fun))
      return fun.toString();
    if (isAnonymousFunction(fun))
      return "/\\";
    else
      return fun.name;
  });

defun(
  function functionSignature(fun, omitName) {
    var signature = "";
    
    if (isNativeFunction(fun))
      signature += "[native] ";

    if (hasAnyOwnProperties(fun.prototype))
      signature += "[ctor] ";
    
    if (!omitName)
      signature += functionName(fun);

    if (!isRegexp(fun)) {
      var arglist = fun.toString().match(/\((.*)\)/)[1];
      if (arglist != "") {
        signature += "("+arglist+")";
      }
      else {
        signature += "(";
        for (let i=0, argc=fun.arity; i<argc; ++i) {
          if (i) signature += ",";
          signature += "_";
        }
        signature += ")";
      }
    }
    
    return signature;
  });

//----------------------------------------------------------------------
// describe

defun(
  function describe(obj) {
    var docs = gendoc(obj, { depth: 0 });
    // filterXXX(docs);
    return docs;
  });

//----------------------------------------------------------------------

defun(
  function gendoc(obj, ctx) {
    if (obj && obj.__docgen__)
      return obj.__docgen__(ctx);
    if (typeof obj == "function")
      return gendoc_function(obj, ctx);
    else if (isarray(obj))
      return gendoc_array(obj, ctx);
    else if (typeof obj == "object")
      return gendoc_object(obj, ctx);
    else
      return gendoc_primitive(obj, ctx);
  });

defun(
  function gendoc_function(fun, ctx) {
    return {
      obj         : fun,
      type        : "Function",
      synopsis    : functionSignature(fun),
      description : fun._doc_
        };
  });

defun(
  function gendoc_array(arr, ctx) {
    var children = [];
    for (let i=0, l=arr.length; i<l; ++i) {
      children.push(gendoc(arr[i], {__proto__:ctx, depth:ctx.depth+1}));
    }

    return {
      obj         : arr,
      type        : "Array",
      synopsis    : "[."+arr.length+".]",
      description : arr._doc_,
      members     : { synopsis : "Members", objects : children }
        };
  });

defun(
  function gendoc_object(obj, ctx) {
    var doc = "{";
    if (ctx.depth == 0) {
      let first = true;
      for (let p in obj) {
        if (p == "_doc_") continue;
        if (!first)
          doc += ", ";
        else
          first = false;
        doc += p + ":" + gendoc(obj[p], {__proto__:ctx, depth:ctx.depth+1});
      }
      if (obj._doc_) {
        doc += ": " + obj._doc_;
      }
      doc += "}";
    }
    else {
      doc += "...}";
    }
  
    return doc;
  });

defun(
  function gendoc_primitive(val, ctx) {
    if (val === null) return "null";
    if (val === undefined) return "undefined";

    if (typeof val == "number") return val.toString();
    
    var doc = typeof val;

    if (ctx.depth == 0) {
      doc += " '"+val.toString()+"'";
    }

    return doc;
  });

