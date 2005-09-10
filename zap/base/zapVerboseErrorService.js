// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:zapVerboseErrorService.js')" -*-
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

debug("*** loading zapVerboseErrorService.js\n");

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");

// name our global object:
function toString() { return "[zapVerboseErrorService.js]"; }

// object to hold component's documentation:
var _doc_ = {};


// XXX 51 is the GENERAL error module. We should define our own
// NS_ERROR_MODULE_ZAP in xpcom/base/nsError.h at some point
var ERROR_MODULE = 51;

function getErrorCode(err) {
  return err & 0xFFFF;
}

function getErrorModule(err) {
  return ((err >> 16) - 0x45) & 0x1FFF;
}

function generateError(code) {
  return 0x80000000 + ((ERROR_MODULE + 0x45)<<16) + (code&0xFFFF);
}

////////////////////////////////////////////////////////////////////////
// Class zapVerboseErrorService

var zapVerboseErrorService = makeClass("zapVerboseErrorService", SupportsImpl);
zapVerboseErrorService.addInterfaces(Components.interfaces.zapIVerboseErrorService);

zapVerboseErrorService.appendCtor(
  function() {
    this._currentMessage = "";
    this._currentCode = 0;
  });

//----------------------------------------------------------------------
// zapIVerboseErrorService implementation:

// AString retrieveVerboseErrorMessage(in unsigned long errorCode);
zapVerboseErrorService.fun(
  function retrieveVerboseErrorMessage(errorCode) {
    if (getErrorModule(errorCode) != ERROR_MODULE) {
      return "";
    }
    if (getErrorCode(errorCode) != this._currentCode) {
      return "Error message lost";
    }
    return this._currentMessage;
  });

//  readonly attribute AString lastMessage;
zapVerboseErrorService.getter(
  "lastMessage",
  function get_lastMessage() {
    return this._currentMessage;
  });

// unsigned long setVerboseErrorMessage(in AString message);
zapVerboseErrorService.fun(
  function setVerboseErrorMessage(message) {
    this._currentMessage = message;
    this._currentCode = (this._currentCode + 1) % 65536;
    return generateError(this._currentCode);
  });

// boolean isVerboseError(in unsigned long errorCode);
zapVerboseErrorService.fun(
  function isVerboseError(errorCode) {
    return (getErrorModule(errorCode) == ERROR_MODULE);
  });

//----------------------------------------------------------------------
// global service object:

var theVerboseErrorService = zapVerboseErrorService.instantiate();


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Verbose Error Service",
     cid        : Components.ID("{071b583f-174e-452c-a558-697601e04e20}"),
     contractID : "@mozilla.org/zap/verbose-error-service;1",
     factory    : ComponentUtils.generateFactory(function() { return theVerboseErrorService; })
  }]);
