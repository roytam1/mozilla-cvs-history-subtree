/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.util.importModule('resource:/jscodelib/zap/ArrayUtils.js', null)" -*- */
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

EXPORTED_SYMBOLS = [ "arraymerge",
                     "arrayclone",
                     "arraysplit",
                     "arraymap",
                     "amap",
                     "every",
                     "some",
                     "member",
                     "findif",
                     "isarray",
                     "flatten",
                     "dflatten",
                     "arraysubst",
                     "arrayequal",
                     "union",
                     "intersection",
                     "difference" ];

// name our global object:
function toString() { return "[ArrayUtils.js]"; }

// object to hold module's documentation:
var _doc_ = {};

//----------------------------------------------------------------------
// arraymerge

_doc_.arraymerge = "\
 Append elements from array 'src' to array 'dest', ommitting elements\n\
 that are present in both 'src' and 'dest' and equal under 'eq'      \n\
 (default: '==').                                                    \n\
 Returns 'dest' array.                                               ";

function arraymerge(dest, src, /*[opt]*/ eq) {
  if (!eq)
    eq = function(a,b) {return a==b;};
  
  for (var i=0, sl=src.length; i<sl; ++i) {
    for (var j=0, dl=dest.length; j<dl; ++j) {
      if (eq(src[i], dest[j])) break;
    }
    if (j==dl)
      dest.push(src[i]);
  }
  return dest;
}

//----------------------------------------------------------------------
// arrayclone

_doc_.arrayclone = "\
 Returns a shallow copy of array 'src'.                               ";

function arrayclone(src) {
  return src.slice(0);
}

//----------------------------------------------------------------------
// arraysplit

_doc_.arraysplit = "\
 Destructively remove items satisfying 'predicate' from 'array'.     \n\
 Returns array of removed items.                                      ";

function arraysplit(predicate, array) {
  var removed = [];
  for (var i=array.length-1; i>=0; --i) {
    if (predicate(array[i]))
      removed.unshift(array.splice(i,1)[0]);
  }
  return removed;
}

//----------------------------------------------------------------------
// arraymap

_doc_.arraymap = "\
 Map function 'fct' over 'array'. Accumulate output of 'fct' in new  \n\
 array.                                                              \n\
 'fct' will be passed two arguments: the array element and the index \n\
 Returns array of accumulated output.                                 ";

function arraymap(fct, array) {
  var accu = new Array(array.length);
  for (var i=0, l=array.length; i<l; ++i) {
    accu[i] = fct(array[i], i);
  }
  return accu;
}

//----------------------------------------------------------------------
// amap

_doc_.amap = "\
 Map function 'fct' over 'array'. Don't accumulate output of 'fct'.  \n\
 If 'fct' returns 'true', processing stops at the current element    \n\
 and amap returns.                                                   \n\
 'fct' will be passed two arguments: the array element and the index. ";

function amap(fct, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (fct(array[i], i))
      return;
  }
}

//----------------------------------------------------------------------
// every

_doc_.every = "\
 Returns true if 'predicate' is true for every element of 'array'.    ";

function every(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (!predicate(array[i]))
      return false;
  }
  return true;
}

//----------------------------------------------------------------------
// some

_doc_.some = "\
 Returns true if 'predicate' is true for at least one element of     \n\
 'array'.                                                             ";

function some(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (predicate(array[i]))
      return true;
  }
  return false;
}

//----------------------------------------------------------------------
// member

_doc_.member = "\
 Returns true if 'array' contains at least one element equal ('==')  \n\
 to 'element'.                                                        ";

function member(element, array) {
  return some(function(e) {return e==element;}, array);
}

//----------------------------------------------------------------------
// findif

_doc_.findif = "\
 Returns the first element in 'array' that satisfies 'predicate', or \n\
 null if no matching element is found.                                ";

function findif(predicate, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (predicate(array[i]))
      return array[i];
  }
  return null;
}

//----------------------------------------------------------------------
// isarray

_doc_.isarray = "\
  Check whether 'obj' is of array type. More reliable than           \n\
  'obj instanceof Array' which gives false negatives if the context  \n\
  that 'obj' was created in is different to the content that the test\n\
  is performed in.                                                    ";
  
function isarray(obj) {
  // XXX ideally we would like to use 'instanceof' to test whether
  // something is an array. Unfortunately, arrays might be instances
  // of different 'Array' objects, depending on the context that they
  // were created in. We have to use the following hack instead:
  if (obj && obj.length!=null && obj.push)
    return true;
  return false;
}

//----------------------------------------------------------------------
// flatten

_doc_.flatten = "\
 Returns a flattened version of 'array'.                              ";

function flatten(array) {
  var l = array.length;
  var retval = [];
  for (var i=0; i<l; ++i) {
    if (isarray(array[i])) {
      amap(function(e){retval.push(e);}, array[i]);
    }
    else
      retval.push(array[i]);
  }
  return retval;
}

//----------------------------------------------------------------------
// dflatten

_doc_.dflatten = "\
 Destructively flattens 'array': Any members of 'array' which are    \n\
 arrays themselves are spliced into 'array'.                          ";

function dflatten(array) {
  for (var i=array.length-1; i>=0; --i) {
    if (isarray(array[i])) {
      var a = array.splice(i,1)[0];
      for (var k=0,l=a.length; k<l; ++k)
        array.splice(i+k,0,a[k]);
    }
  }
  return array;
}

//----------------------------------------------------------------------
// arraysubst

_doc_.arraysubst = "\
 Destructively substitute 'newelem' for 'oldelem' in 'array'.         ";

function arraysubst(newelem, oldelem, array) {
  for (var i=0, l=array.length; i<l; ++i) {
    if (array[i]==oldelem)
      array[i]=newelem;
  }
}

//----------------------------------------------------------------------
// arrayequal

_doc_.arrayequal = "\
 Returns true iff arrays 'arr1' and 'arr2' have the same length and  \n\
 their elements are equal under 'eq' (default: '==').                 ";

function arrayequal(arr1, arr2, /*[opt]*/ eq) {
  if (arr1.length!=arr2.length) return false;
  if (!eq)
    eq = function(a,b) {return a==b;};
  for (var i=0, l=arr1.length; i<l; ++i) {
    if (!eq(arr1[i],arr2[i]))
      return false;
  }
  return true;
}

//----------------------------------------------------------------------
// union

_doc_.union = "\
 Returns the union of the given arrays.                               ";

function union(arr1, arr2) {
  var retval = [];
  arraymerge(retval, arr1);
  arraymerge(retval, arr2);
  return retval;
}

//----------------------------------------------------------------------
// intersection

_doc_.intersection = "\
 Returns the intersection of the given arrays.                        ";

function intersection(arr1, arr2) {
  var retval = [];
  amap(function(item) {if (member(item, arr2)) arraymerge(retval, [item]);},
       arr1);
  return retval;
}

//----------------------------------------------------------------------
// difference

_doc_.difference = "\
 Returns the set difference 'arr1'-'arr2'.                            ";

function difference(arr1, arr2) {
  var retval = [];
  amap(function(item) {if (!member(item, arr2)) retval.push(item);},
       arr1);
  return retval;
}
