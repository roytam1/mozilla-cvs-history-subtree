/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib).probeModule('resource:/jscodelib/ProfileLib.js')" -*- */
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
 * Collection of code profiling functions.
 *
 * 
 */

MOZ_EXPORTED_SYMBOLS = [ "time",
                         "calculateMoments" ];

//----------------------------------------------------------------------
// time()

function time(fct) {
  var start = new Date();
  fct();
  var end = new Date();
  return end.getTime() - start.getTime();
};
    
time._doc_ = "Returns the execution time (ms) of the function passed as argument.";

//----------------------------------------------------------------------
// calculateMoments(). See Press et. al., Numerical Recipes, Chapter 14.

function calculateMoments(data, obj) {
  var n = data.length;
  if (n<2) throw "not enough data";
  if (!obj) obj = {};
  
  // calculate mean
  var a=0;
  for (var i=0; i<n; ++i)
    a += data[i];
  obj.mean = a/n;
  
  // calculate higher moments
  obj.average_deviation = 0;
  obj.variance = 0;
  obj.skewness = 0;
  obj.kurtosis = 0;
  var b;
  var c=0;
  for (var i=0; i<n; ++i) {
    a = data[i]-obj.mean;
    obj.average_deviation += Math.abs(a);
    c += a;
    b = a*a;
    obj.variance += b;
    b *= a;
    obj.skewness += b;
    obj.kurtosis += b*a;
  }
  obj.average_deviation /= n;
  obj.variance = (obj.variance-c*c/n)/(n-1);
  obj.standard_deviation = Math.sqrt(obj.variance);
  if (obj.variance) {
    obj.skewness /= n*obj.variance*obj.standard_deviation;
    obj.kurtosis = obj.kurtosis/(n*obj.variance*obj.variance)-3;
  }
  
  return obj;
}

calculateMoments._doc_ =
"Given an array 'data', returns an object with the data's mean, \
average_deviation, standard_deviation, variance, skewness and \
kurtosis. If the optional argument 'obj' is given, the moments will be \
stored on this object and it will be returned.";


