// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:zapUUIDGenerator.js')" -*-
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

debug("*** loading zapUUIDGenerator.js\n");

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");

// name our global object:
function toString() { return "[zapUUIDGenerator.js]"; }

// object to hold component's documentation:
var _doc_ = {};

// helpers:

function getRandom32() {
  // Obtain a 32bit random number:
  // XXX we should use a better random number generator
  return Math.floor(Math.random()*4294967296);
}



////////////////////////////////////////////////////////////////////////
// Class zapUUIDGenerator

var zapUUIDGenerator = makeClass("zapUUIDGenerator", SupportsImpl);
zapUUIDGenerator.addInterfaces(Components.interfaces.zapIUUIDGenerator);

//----------------------------------------------------------------------
// zapIUUIDGenerator implementation:

// ACString generateUUIDString();
zapUUIDGenerator.fun(
  function generateUUIDString() {
    // generate a 'version 4'-type UUID as specified in RFC4122
    var time_low_32 = getRandom32();
    var time_mid_16 = getRandom32() & 0xFFFF;
    var time_high_and_version_16 = 0x4000 | (getRandom32() & 0x0FFF);
    var clock_seq_hi_and_reserved_8 = 0x80 | (getRandom32() & 0x3F);
    var clock_seq_low_8 = getRandom32() & 0xFF;
    var node1_16 = getRandom32() & 0xFFFF;
    var node2_32 = getRandom32();
  
    return padleft(time_low_32.toString(16), "0", 8) + "-" +
      padleft(time_mid_16.toString(16), "0", 4) + "-" +
      padleft(time_high_and_version_16.toString(16), "0", 4) + "-" +
      padleft(clock_seq_hi_and_reserved_8.toString(16), "0", 2) +
      padleft(clock_seq_low_8.toString(16), "0", 2) + "-" +
      padleft(node1_16.toString(16), "0", 4) +
      padleft(node2_32.toString(16), "0", 8);    
  });

// ACString generateUUIDURNString();
zapUUIDGenerator.fun(
  function generateUUIDURNString() {
    return "urn:uuid:"+this.generateUUIDString();
  });

//----------------------------------------------------------------------
// global service object:

var theUUIDGenerator = zapUUIDGenerator.instantiate();


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP UUID Generator",
     cid        : Components.ID("{a119ec9d-ebdf-4df9-aec6-a659f6e4d674}"),
     contractID : "@mozilla.org/zap/uuid-generator;1",
     factory    : ComponentUtils.generateFactory(function() { return theUUIDGenerator; })
  }]);
