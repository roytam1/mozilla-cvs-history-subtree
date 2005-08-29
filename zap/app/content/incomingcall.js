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
var wCallerAddressElement;
var wCallStatusElement;
var wSipClient;
var wCallHandler;
var wDialog;

//----------------------------------------------------------------------
// Initialization:

function windowInit() {
  wLogElement = document.getElementById("log");
  wCallerAddressElement = document.getElementById("caller-address");
  wCallStatusElement = document.getElementById("call-status");
  wSipClient = opener.SipClient;
  wCallHandler = window.arguments[0];
  // install our own log method, so that log messages land in our window:
  wCallHandler.log = log;
//   wCallHandler.destroyHook = function() {
//     wCallStatusElement.value = "Terminated";
//  };
  wCallHandler.successHook = function(dialog, mediasession) {
    dialog.listener = {
      notifyDialogTerminated:function(d) {
        mediasession.shutdown();
        wCallStatusElement.value = "Terminated";
      }
    };
    wDialog = dialog;
    wCallStatusElement.value = "Confirmed";
  };
  wCallHandler.noACKHook = function(dialog, mediasession) {
    dialog.listener = {
      notifyDialogTerminated:function(d) {
        mediasession.shutdown();
        wCallStatusElement.value = "Terminated";
      }
    };
    wDialog = dialog;
    wCallStatusElement.value = "Established (not confirmed)";
  };
  wCallHandler.failHook = function() {
    wCallStatusElement.value = "Call failed";
  };
  // display call info:
  wCallerAddressElement.value = wCallHandler.rs.request.getFromHeader().serialize();
  
  log("Ringing");
  wCallStatusElement.value = "Ringing";
  wCallHandler.rs.sendResponse(wCallHandler.rs.formulateResponse("180"));
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

function cmdAccept() {
  if (wCallStatusElement.value == "Ringing") {
    log("Accepting");
    var r = wCallHandler.rs.formulateResponse("200");
    
    r.setContent("application", "sdp", wCallHandler.answer.serialize());
    wCallHandler.rs.sendResponse(r);
    wCallHandler.changeState("ACCEPTED");
    wCallHandler.mediaSession.startSession();
  }
}

function cmdHangup() {
  if (wCallStatusElement.value == "Ringing") {
    log("Rejecting");
    wCallHandler.rs.sendResponse(wCallHandler.rs.formulateResponse("603"));
  }
  else if (wCallStatusElement.value == "Established (not confirmed)" ||
           wCallStatusElement.value == "Confirmed") {
    log("Hanging up");
    rc = wDialog.createNonInviteRequestClient();
    rc.sendRequest(rc.formulateRequest("BYE"));
  }
}

function cmdClearLog() {
  wLogElement.value = "";
}


