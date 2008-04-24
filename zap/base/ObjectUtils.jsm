/* -*- moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/ObjectUtils.jsm', null)" -*- */
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

EXPORTED_SYMBOLS = [ "hasproperty",
                     "hasgettersetter",
                     "propcopy",
                     "objclone",
                     "objmerge",
                     "hashget",
                     "hashhas",
                     "hashset",
                     "hashdel",
                     "hashkeys",
                     "hashmap"];

//----------------------------------------------------------------------
// hasproperty

defun(
  "Returns true iff 'obj' has a property, getter or setter with name "+
  "'prop'; false otherwise.",
  function hasproperty(obj, prop) {
    return (hasgettersetter(obj, prop) ||
            obj[prop] !== undefined);
  });
  
//----------------------------------------------------------------------
// hasgettersetter

defun(
  "Returns true iff 'obj' has a getter or getter/setter with name "+
  "'prop'; false otherwise.",
  function hasgettersetter(obj, prop) {
    return (obj.__lookupGetter__(prop) ||
            obj.__lookupSetter__(prop));
  });

//----------------------------------------------------------------------
// propcopy

defun(
  "Copies property 'prop' from object 'src' to object 'dest'. 'prop' "+
  "can name a 'normal' property or a getter/setter. "+
  "Returns true.",
  function propcopy(dest, src, prop) {
    // src[prop] might be a getter and/or setter:
    var getter = src.__lookupGetter__(prop);
    var setter = src.__lookupSetter__(prop);
    if (getter || setter) {
      if (getter)
        dest.__defineGetter__(prop, getter);
      if (setter)
        dest.__defineSetter__(prop, setter);
    }
    else 
      dest[prop] = src[prop];
    
    return true;
  });

//----------------------------------------------------------------------
// objclone

defun(
  "Returns a shallow copy of the given object. The copy includes all "+
  "own and inherited properties of 'src' as well as any getters and "+
  "setters.",
  function objclone(src) {
    if (src == null) return null;
    var dest = {};
    for (var p in src) {
      propcopy(dest, src, p);
    }
    return dest;
  });

//----------------------------------------------------------------------
// objmerge

defun(
  "Merge properties from object 'src' into object 'dest'. Only copy "+
  "properties that aren't present in 'dest'. Merging can be customized "+
  "by providing an optional function mergefun(dest, src, prop) which "+
  "will be called for each property in turn. If this function returns "+
  "true, the given property will not be copied by objmerge. ",
  function objmerge(dest, src, /*[opt]*/ mergefun) {
    for (var p in src) {
      if (mergefun && mergefun(dest, src, p))
        continue; // merging handled by mergefun
      if (!hasproperty(dest, p))
        propcopy(dest, src, p);
    }
  });

//----------------------------------------------------------------------
// hashget

defun(
  "Normal JS objects can be used as hashes, but we need to take care to "+
  "avoid reserved names such as __prototype__, etc. as key values. "+
  "hashget(obj, key), hashhas(obj, key), hashset(obj, key, value), "+
  "hashdel(obj, key) translate key values appropriately. "+
  "hashkeys(obj) returns an array of all keys in the hash. "+
  "hashmap(obj, fct) can be used to map 'fct(key,value)' over all "+
  "hash elements. Mapping stops when 'fct' returns a value != false "+
  "and that value will be returned by the hashmap(). ",
  function hashget(obj, key) {
    return obj["$"+key];
  });

//----------------------------------------------------------------------
// hashhas

defun(
  "see hashget()",
  function hashhas(obj, key) {
    return obj["$"+key] !== undefined;
  });

//----------------------------------------------------------------------
// hashset

defun(
  "see hashget()",
  function hashset(obj, key, value) {
    return obj["$"+key] = value;
  });

//----------------------------------------------------------------------
// hashdel

defun(
  "see hashget()",
  function hashdel(obj, key) {
    delete obj["$"+key];
  });

//----------------------------------------------------------------------
// hashkeys

var REGEXP_HASHKEY = /^\$(.*)$/;

defun(
  "see hashget()",
  function hashkeys(obj) {
    var rv = [];
    var match;
    for (var k in obj) {
      if ((match = REGEXP_HASHKEY(k)))
        rv.push(match[1]);
    }
    return rv;
  });

//----------------------------------------------------------------------
// hashmap

defun(
  "see hashget()",
  function hashmap(obj, fct) {
    var match;
    var rv;
    for (var k in obj) {
      if ((match = REGEXP_HASHKEY(k))) {
        if ((rv = fct(match[1], obj[k])))
          return rv;
      }
    }
    return null;
  });
