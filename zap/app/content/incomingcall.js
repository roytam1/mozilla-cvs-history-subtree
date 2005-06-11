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

var wCodelib = Components.classes['@mozilla.org/jscodelib;1'].getService(Components.interfaces.mozIJSCodeLib);
wCodelib.importModule("resource:/jscodelib/zap/ClassUtils.js");

//----------------------------------------------------------------------
// window-global data:
var wLogElement;
var wCallerAddressElement;
var wCallStatusElement;
var wSipClient;
var wCallHandler;

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
  wCallHandler.destroyHook = function() {
    wCallStatusElement.value = "Terminated";
  };
  wCallHandler.callSuccessHook = function() {
    wCallStatusElement.value = "Established";
  };

  // display call info:
  wCallerAddressElement.value = wCallHandler.request.getFromHeader().serialize();
  
  log("Ringing");
  wCallStatusElement.value = "Ringing";
  var r = wSipClient.sipStack.formulateResponse("180", "Ringing", wCallHandler.request, false);
  wCallHandler.ms.sendResponse(r);
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
    var r = wCallHandler.ms.formulate200Response();

    // set media offer:
    var callerAddress;
    try {
      callerAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wSipClient.getCallerAddress());
    } catch(e) {
      alert(wSipClient.getCallerAddress()+" is not a valid sip address! Please check Identity setup in main app window and try again.");
      return;
    }
    var myURI = callerAddress.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
    var offer = wSipClient.sdpService.createSessionDescription();
    // o=
    offer.username = myURI.userinfo;
    offer.sessionID = "0";
    offer.sessionVersion = "0";
    offer.originAddressType = "IP4";
    offer.originAddress = myURI.host;
    // s=
    offer.sessionName = " ";
    // c=
    offer.connection = wSipClient.sdpService.createConnection();
    offer.connection.addressType = "IP4";
    offer.connection.address = wSipClient.sipStack.hostAddress;
    // t=
    var time = wSipClient.sdpService.createTime();
    time.value = "0 0";
    offer.setTimes([time], 1);
    // m=
    var mediaDescription = wSipClient.sdpService.createMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = wCallHandler.rtpBase;
    mediaDescription.portCount = "";
    mediaDescription.protocol = "RTP/AVP";
    var fmt = wSipClient.sdpService.createMediaFormat();
    fmt.format = "97";
    mediaDescription.setFormats([fmt], 1);
    
    // a=
    var attribs = [];
    var attrib = wSipClient.sdpService.createAttrib();
//    attrib.value = "rtpmap:97 iLBC/8000";
    attrib.value = "rtpmap:97 speex/8000";
    attribs.push(attrib);
//     var attrib = wSipClient.sdpService.createAttrib();
//     attrib.value = "fmtp:97 mode=30";
//     attribs.push(attrib);
    
    mediaDescription.setAttribs(attribs, attribs.length);
    
    offer.setMediaDescriptions([mediaDescription], 1);
    
    r.setContent("application", "sdp", offer.serialize());
    wCallHandler.ms.sendResponse(r);
  }
}

function cmdHangup() {
  if (wCallStatusElement.value == "Ringing") {
    log("Rejecting");
    var r = wSipClient.sipStack.formulateResponse("603", "Decline", wCallHandler.request, true);
    wCallHandler.ms.sendResponse(r);
  }
  else if (wCallStatusElement.value == "Established") {
    log("Hanging up");
    wCallHandler.dialog.bye();
  }
}

function cmdClearLog() {
  wLogElement.value = "";
}


