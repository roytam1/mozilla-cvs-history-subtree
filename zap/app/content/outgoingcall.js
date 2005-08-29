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
var wSipClient;
var wCallHandler;
var wMediaSession;
var wDialog;

//----------------------------------------------------------------------
// Initialization:

function windowInit() {
  wLogElement = document.getElementById("log");
  wDestAddressElement = document.getElementById("dest-address");
  wSipClient = opener.SipClient;
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

function cmdCall() {
  if (wCallHandler) {
    alert("Call is already in progress!");
    return;
  }
  if (wDialog) {
    alert("Call established - hang up first!");
    return;
  }
  
  try {
    destAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wDestAddressElement.value);
  } catch(e) {
    alert(wDestAddressElement.value+" is not a valid sip address! Please try again.");
    return;
  }

  
  wCallHandler = CallHandler.instantiate();
  wCallHandler.call(destAddress);
}

function cmdHangup() {
  if (!wDialog) {
    alert("Call not active!");
    return;
  }
  var rc = wDialog.createNonInviteRequestClient();
  rc.sendRequest(rc.formulateRequest("BYE"));
}

function cmdClearLog() {
  wLogElement.value = "";
}


// ////////////////////////////////////////////////////////////////////////
// // CallHandler:

var CallHandler = makeClass("CallHandler", StateMachine, SupportsImpl);
CallHandler.addInterfaces(Components.interfaces.zapISipInviteRCListener,
                          Components.interfaces.zapISipInviteResponseHandler);

CallHandler.fun(
  function call(to) {
    var rc = wSipClient.sipStack.createInviteRequestClient(to);
    rc.listener = this;

    wMediaSession = Components.classes["@mozilla.org/zap/mediasession;1"].createInstance(Components.interfaces.zapIMediaSession);
    wMediaSession.init(wSipClient.getUserName(),
                       wSipClient.getHostName(),
                       wSipClient.sipStack.hostAddress);
    var offer = wMediaSession.generateSDPOffer();
    
    var request = rc.formulateInvite();
    request.setContent("application", "sdp", offer.serialize());
    log("Sending invite to "+to.serialize()+"...");

    rc.sendInvite(request, this);
    
    this.changeState("CALLING");
  });
  
//----------------------------------------------------------------------
// zapISipInviteRCListener

CallHandler.statefun(
  "CALLING",
  function notifyResponseReceived(rc, dialog, response) {
    log("Response received: "+response.statusCode+" ("+response.reasonPhrase+")");
  });

CallHandler.statefun(
  "CALLING",
  function notifyTerminated(rc) {
    this.changeState("TERMINATED");
    wCallHandler = null;
    if (!wDialog) {
      log("Call failed");
      wMediaSession.shutdown();
      wMediaSession = null;
    }
    log("...Done");
  });

//----------------------------------------------------------------------
// zapISipInviteResponseHandler

CallHandler.statefun(
  "CALLING",
  function handle2XXResponse(rc, dialog, response, ack) {
    if (wDialog) {
      alert("uh-oh, we already have a dialog for this call. multiple dialogs/call not supported yet!");
      return null;
    }

    var sd;
    try {
      sd = wSipClient.sdpService.deserializeSessionDescription(response.body);
    }
    catch(e) {
      log("Can't parse other party's session description: "+response.body);
      return null;
    }
    
    wDialog = dialog;
    wMediaSession.processSDPAnswer(sd);
    wMediaSession.startSession();

    // add listener to dialog to shutdown media session:
    wDialog.listener = {
      notifyDialogTerminated : function(d) {
        wMediaSession.shutdown();
        wMediaSession = null;
        wDialog = null;
        log("Call terminated");
      }
    };
    
    return ack;
  });


