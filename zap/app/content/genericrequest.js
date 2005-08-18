// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "getWindows()[0]" -*-
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

//----------------------------------------------------------------------
// imports

Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");

//----------------------------------------------------------------------
// window-global data:
var wLogElement;
var wDestAddressElement;
var wMethodNameElement;
var wRequestElement;
var wSipClient;
var wCallHandler;
var wVerboseErrorService;

//----------------------------------------------------------------------
// Initialization:

function windowInit() {
  wLogElement = document.getElementById("log");
  wDestAddressElement = document.getElementById("dest-address");
  wMethodNameElement = document.getElementById("method-name");
  wRequestElement = document.getElementById("request");
  wSipClient = opener.SipClient;
  wVerboseErrorService = opener.wVerboseErrorService;
}

function windowClose() {
}

//----------------------------------------------------------------------
// Logging:

// helper to log ui events:
function log(mes) {
  wLogElement.value += mes + "\n";
  wLogElement.mInputField.scrollTop = wLogElement.mInputField.scrollHeight;
}

//----------------------------------------------------------------------
// UI Commands:

function cmdGenerateRequest() {
  var callerAddress;
  try {
    callerAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wSipClient.getCallerAddress());
  } catch(e) {
    alert(wSipClient.getCallerAddress()+" is not a valid sip address! Please check Identity setup in main app window and try again.");
    return;
  }
  
  var destAddress;
  try {
    destAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wDestAddressElement.value);
  } catch(e) {
    alert(wDestAddressElement.value+" is not a valid sip address! Please try again.");
    return;
  }

  var request = wSipClient.sipStack.formulateGenericRequest(wMethodNameElement.value, destAddress.uri, destAddress, callerAddress);

  wRequestElement.value = request.serialize();
}

function cmdCall() {
  if (wCallHandler) {
    alert("Call is already in progress!");
    return;
  }
  var requestTxt = wRequestElement.value;
  requestTxt = requestTxt.replace(/\r/g, "");
  requestTxt = requestTxt.replace(/\n/g, "\r\n");
  try {
    var request = wSipClient.sipStack.syntaxFactory.deserializeMessage(requestTxt);
  }catch(e) {
    if (wVerboseErrorService.isVerboseError(e.result)) {
      log(wVerboseErrorService.retrieveVerboseErrorMessage(e.result));
    }
    else {
      log(e);
    }
    return;
  }
  wCallHandler = CallHandler.instantiate();
  
  log("trying ...");
  var genericMC = wSipClient.sipStack.createGenericMC(request, wCallHandler);
  genericMC.execute();  
}

function cmdClearLog() {
  wLogElement.value = "";
}


////////////////////////////////////////////////////////////////////////
// CallHandler:

var CallHandler = makeClass("CallHandler", SupportsImpl);
CallHandler.addInterfaces(Components.interfaces.zapISipGenericMCH)

CallHandler.fun(
  function destroy() {
    wCallHandler = null;
  });

// zapISipGenericMCH implementation:

CallHandler.fun(
  function calling(method, endpoint) {
    log(">>> calling "+endpoint.address+":"+endpoint.port+" via "+endpoint.transport);
  });

CallHandler.fun(
  function finalResponse(method, response) {
    log(">>> final response: \n"+response.serialize());
  });

CallHandler.fun(
  function provisionalResponse(method, response) {
    log(response.statusCode+" ("+response.reasonPhrase+")");
  });

CallHandler.fun(
  function terminated(method) {
    log("... end");
    this.destroy();
  });

