/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/StdLib.js', true)" -*- */
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
 * Collection of useful functions.
 *
 * 
 */

MOZ_EXPORTED_SYMBOLS = [ "appendUnique",
                         "cloneArray",
                         "cloneObj",
                         "splitArray",
                         "maparray",
                         "mapa",
                         "every",
                         "some",
                         "member",
                         "findif",
                         "flatten",
                         "dflatten",
                         "substitute",
                         "arrayEqual",
                         "union",
                         "intersection",
                         "difference",
                         "createDynamicProxy" ];


//----------------------------------------------------------------------
// appendUnique

function appendUnique(target, src, /*[optional]*/ equals) {
  for (var i=0, sl=src.length; i<sl; ++i) {
    for (var j=0, tl=target.length; j<tl; ++j) {
      if (!equals) {
        if (src[i] == target[j]) break;
      }
      else if (equals(src[i],target[j])) break;
    }
    if (j==tl)
      target.push(src[i]);
  }
  return target;
}

appendUnique._doc_ = "Append array 'src' to array 'target', \
ommiting elements that are already present in 'target' and equal under \
the relation 'equals' (default: '=='). Returns target array";

//----------------------------------------------------------------------
// cloneArray

function cloneArray(src) {
  return src.slice(0);
}

cloneArray._doc_ = "Returns a (shallow) copy of the given array.";

//----------------------------------------------------------------------
// cloneObj

function cloneObj(src) {
  var retval = {};
  for (var p in src) {
    retval[p] = src[p];
  }
  return retval;
}

cloneObj._doc_ = "Returns a (shallow) copy of the given object.";

//----------------------------------------------------------------------
// splitArray

function splitArray(predicate, array) {
  var retval = [];
  for (var i=array.length-1; i>=0; --i) {
    if (predicate(array[i]))
      retval.unshift(array.splice(i,1)[0]);
  }
  return retval;
}

splitArray._doc_ = "Destructively remove items satisfying 'predicate' from 'array'. Return array of removed items.";

//----------------------------------------------------------------------
// maparray

function maparray(fct, array) {
  var res = new Array(array.length);
  for (var i=0, l=array.length; i<l; ++i) {
    res[i] = fct(array[i]);
  }
  return res;
}

maparray._doc_ = "Map function 'fct' over 'array'; accumulate output in new array.";

//----------------------------------------------------------------------
// mapa

function mapa(fct, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    fct(array[i]);
  }
}

mapa._doc_ = "Map function 'fct' over 'array'; don't accumulate output.";

//----------------------------------------------------------------------
// every

function every(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (!predicate(array[i]))
      return false;
  }
  return true;
}

every._doc_ = "Return true if 'predicate' is true for every element of 'array'.";

//----------------------------------------------------------------------
// some

function some(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (predicate(array[i]))
      return true;
  }
  return false;
}

some._doc_ = "Returns true if 'predicate' is true for at least one element of 'array'.";

//----------------------------------------------------------------------
// member

function member(element, array) {
  if (!array) throw("member("+element+","+array+"): array error");
  return some(function(e){ return e==element;}, array);
}

member._doc_ = "Returns true if 'array' contains at least one element equal to (==) 'element'.";

//----------------------------------------------------------------------
// findif

function findif(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (predicate(array[i]))
      return array[i];
  }
  return null;
}

findif._doc_ = "Returns the first item in 'array' that satisfies 'predicate', or null if not found.";

//----------------------------------------------------------------------
// flatten

function flatten(array) {
  var l = array.length;
  var retval = [];
  for (var i=0; i<l; ++i) {
    if (array[i] instanceof Array) {
      mapa(function(e){retval.push(e);}, array[i]);
    }
    else
      retval.push(array[i]);
  }
  return retval;
}

flatten._doc_ = "Return a flattened version of 'array'.";

//----------------------------------------------------------------------
// dflatten

function dflatten(array) {
  for (var i=array.length-1; i>=0; --i) {
    if (array[i] instanceof Array) {
      var a = array.splice(i,1)[0];
      for (var k=0,l=a.length; k<l; ++k)
        array.splice(i+k,0,a[k]);
    }
  }
  return array;
}

dflatten._doc_ = "Destructively flattens 'array': Any members of 'array' which are themselves arrays are spliced into 'array'.";

//----------------------------------------------------------------------
// substitute

function substitute(newElement, oldElement, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (array[i]==oldElement)
      array[i]=newElement;
  }
}

substitute._doc_ = "Destructively substitute 'newElement' for 'oldElement' in 'array'.";

//----------------------------------------------------------------------
// arrayEqual

function arrayEqual(arr1, arr2, /*[optional]*/ eq) {
  if (arr1.length!=arr2.length) return false;
  if (!eq)
    eq = function(a,b) {return a==b;};
  for (var i=0, l=arr1.length; i<l; ++i) {
    if (!eq(arr1[i],arr2[i]))
      return false;
  }
  return true;
}

arrayEqual._doc_ = "Returns true iff arrays 'arr1' and 'arr2' have the length and their elements are equal under 'eq' (defaults to '==' if not given).";

//----------------------------------------------------------------------
// union

function union(arr1, arr2) {
  var retval = [];
  appendUnique(retval, arr1);
  appendUnique(retval, arr2);
  return retval;
}

union._doc_ = "Returns the union of the given arrays.";

//----------------------------------------------------------------------
// intersection

function intersection(arr1, arr2) {
  var retval = [];
  mapa(function(item) {if (member(item, arr2)) appendUnique(retval, [item]);},
       arr1);
  return retval;
}

intersection._doc_ = "Returns the intersection of the given arrays.";

//----------------------------------------------------------------------
// difference

function difference(arr1, arr2) {
  var retval = [];
  mapa(function(item) {if (!member(item, arr2)) retval.push(item);},
       arr1);
  return retval;
}

difference._doc_ = "Returns the set difference 'arr1'-'arr2'.";

//----------------------------------------------------------------------
// createDynamicProxy

function createDynamicProxy(objGetter) {
  return { __noSuchMethod__: function(id, args) {
                               var obj = objGetter();
                               return obj[id].apply(obj, args);
           },
           // For QI, we need to ensure that we return ourselves, not the wrapped object:
           QueryInterface : function(iid) {
             objGetter().QueryInterface(iid); // throws on failure
             return this; // impersonate obj
           }
         };
}

createDynamicProxy._doc_ = "create an object that will forward all method calls to \
to the object returned by 'objGetter'. 'objGetter' will be called for each method call.";
