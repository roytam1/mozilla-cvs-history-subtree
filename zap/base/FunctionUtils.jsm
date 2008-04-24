/* -*- Mode: javascript; moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/FunctionUtils.jsm', null)" -*- */
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

EXPORTED_SYMBOLS = [ "defun",
                     "noop",
                     "makeServiceGetter" ];

// name our global object:
// function toString() { return "[FunctionUtils.jsm]"; }

//----------------------------------------------------------------------
// defun

function defun(/* [opt] docstring, fun */) {
  var i = arguments.length;
  var fun = arguments[--i];
  this[fun.name] = fun;
  if (i > 0)
    fun._doc_ = arguments[--i];
}
defun._doc_ = "defun([opt] docstring, fun) defines a function 'fun' on "+
  "the current global object and installs the optional 'docstring' on 'fun'";
  
//----------------------------------------------------------------------
// noop


defun(
  "A function that does nothing.",
  function noop() {});

//----------------------------------------------------------------------
// makeServiceGetter

defun(
  "Returns a function that returns and caches the service (clsid,itf)",
  function makeServiceGetter(clsid, itf) {
    var theService;
    return function() {
      if (!theService) {
        theService = Components.classes[clsid].getService(itf);
      }
      return theService;
    };
  });
