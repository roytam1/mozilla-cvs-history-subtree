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
var wLogger;
var wLogElement;
var wDisplayNameElement;
var wUserNameElement;
var wHostNameElement;
var wCallRejectElement;
var wVerboseErrorService;

//----------------------------------------------------------------------
// Initialization:

function windowInit() {
  wLogElement = document.getElementById("log");
  wDisplayNameElement = document.getElementById("display-name");
  wUserNameElement = document.getElementById("user-name");
  wHostNameElement = document.getElementById("host-name");
  wCallRejectElement = document.getElementById("call-reject");
  wLogger = Components.classes["@mozilla.org/zap/loggingservice;1"].
    getService(Components.interfaces.zapILoggingService);
  wLogger.addLogListener(logListener);
  wVerboseErrorService = Components.classes["@mozilla.org/zap/verbose-error-service;1"].
    getService(Components.interfaces.zapIVerboseErrorService);
  initSipClient();
}

function windowClose() {
  wLogger.removeLogListener(logListener);
}

//----------------------------------------------------------------------
// Logging:

var logListener = {
  log : function(logModule, level, message) {
    if (level == Components.interfaces.zapILoggingService.WARNING)
      logModule += " WARNING";
    else if (level == Components.interfaces.zapILoggingService.ERROR)
      logModule += " ERROR";
    wLogElement.value += "[" + logModule + "] " + message + "\n";
    wLogElement.mInputField.scrollTop = wLogElement.mInputField.scrollHeight;
  }
};

// helper to log ui events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  wLogger.log("UI", level, mes);
}

//----------------------------------------------------------------------
// UI Commands:

function cmdExit() {
  goQuitApplication();
}

var wCount = 1;

function cmdCall() {
  
  window.open("chrome://zap/content/outgoingcall.xul", "call"+(wCount++), "chrome, resizable");
}

function cmdRequest() {
  window.open("chrome://zap/content/genericrequest.xul", "request"+(wCount++),
              "chrome, resizable");
}

function cmdClearLog() {
  wLogElement.value = "";
}


////////////////////////////////////////////////////////////////////////
// SipClient:

var SipClient = {
  getCallerAddress : function() {
    return wDisplayNameElement.value+" <sip:"+
           wUserNameElement.value+"@"+
           wHostNameElement.value+">";
  },
  sipStack: null,
  getUserName : function() { return wUserNameElement.value; },
  getHostName : function() { return wHostNameElement.value; }
};

function initSipClient()
{
  log("Initializing Sip Client...");
  try {
    log("Getting SDP Service");
    SipClient.sdpService = Components.classes["@mozilla.org/zap/sdpservice;1"].
      getService(Components.interfaces.zapISdpService);
    
    log("Constructing Sip UA Stack");
    SipClient.sipStack = Components.classes["@mozilla.org/zap/sipstack;1"].createInstance();
    SipClient.sipStack.QueryInterface(Components.interfaces.zapISipUAStack);
    SipClient.sipStack.init(gUAHandler,
                            makePropertyBag({$port_base:5060,
                                             $methods: "OPTIONS,INVITE,ACK,BYE"}));
    
    // initialize host-name to our ip address:
    wHostNameElement.value = SipClient.sipStack.hostAddress + ":" +
      SipClient.sipStack.listeningPort;

    log("...Sip Client Initialization done.");
  }
  catch(e) {
    log("Error during Sip Client initialization: "+e);
  }
}

////////////////////////////////////////////////////////////////////////
// InviteRSHandler: INVITE request server handler

var InviteRSHandler = makeClass("InviteRSHandler",
                                SupportsImpl, StateMachine);
InviteRSHandler.addInterfaces(Components.interfaces.zapISipInviteRSListener);

InviteRSHandler.fun(
  function init(rs) {
    this.changeState("INITIALIZED");
    this.rs = rs;
    rs.listener = this;
    // check for outright call rejection:
    if (wCallRejectElement.checked) {
      this.log("Declining INVITE");
      rs.sendResponse(rs.formulateResponse("603"));
      this.changeState("REJECTED");
      return;
    }
      
    this.mediaSession = Components.classes["@mozilla.org/zap/mediasession;1"].createInstance(Components.interfaces.zapIMediaSession);
    this.mediaSession.init(wUserNameElement.value,
                           wHostNameElement.value,
                           SipClient.sipStack.hostAddress);
    
    // parse offer:
    try {
      var offer = SipClient.sdpService.deserializeSessionDescription(rs.request.body);
      this.answer = this.mediaSession.processSDPOffer(offer);
    }
    catch(e) {
      // Session negotiation failed.
      // Send 488 (Not acceptable) (RFC3261 13.3.1.3)
      // XXX set a Warning header
      this.changeState("REJECTED");
      rs.sendResponse(rs.formulateResponse("488"));

      // log error:
      if (wVerboseErrorService.isVerboseError(e.result))
        this.log(wVerboseErrorService.retrieveVerboseErrorMessage(e.result));
      else
        this.log("Exception: "+e);
      
      return;
    }

    // the offer is acceptable; we have an answer at hand.
    // hand over to incomingcall:
    window.openDialog("chrome://zap/content/incomingcall.xul", "call"+(wCount++), "chrome, resizable", this);
  });

//----------------------------------------------------------------------
// hooks: will be overwritten by the incomingcall window

InviteRSHandler.obj("log", log);
InviteRSHandler.obj("failHook", null);
InviteRSHandler.obj("successHook", null);
InviteRSHandler.obj("noACKHook", null);

//----------------------------------------------------------------------
// zapISipInviteRSListener

//  void notifyTerminated(in zapISipInviteRS requestServer);
InviteRSHandler.statefun(
  "*",
  function notifyTerminated(rs) {
    this.log("Call failed");
    if (this.failHook)
      this.failHook();
    if (this.mediaSession)
      this.mediaSession.shutdown();
  });
InviteRSHandler.statefun(
  "ACCEPTED",
  function notifyTerminated(rs) {
    this.log("Call succeeded, but no ACK");
    if (this.noACKHook)
      this.noACKHook(this.rs.dialog, this.mediaSession);
    this.changeState("TERMINATED");
  });
InviteRSHandler.statefun(
  "CONFIRMED",
  function notifyTerminated(rs) {
    this.log("Call succeeded");
    if (this.successHook)
      this.successHook(this.rs.dialog, this.mediaSession);
    this.changeState("TERMINATED");
  });

    
//  void notifyACKReceived(in zapISipInviteRS requestServer,
//                         in zapISipRequest ACKRequest);
InviteRSHandler.statefun(
  "ACCEPTED",
  function notifyACKReceived(rs, request) {
    this.log("ACK received");
    this.changeState("CONFIRMED");
  });

////////////////////////////////////////////////////////////////////////
// UA Handler:

// XXX request handler
var gUAHandler = {

  handleNonInviteRequest: function(rs) {
//     if (rs.request.method == "OPTIONS") {
//       var response = rs.formulateResponse("200");
//        var allowed = ["INVITE", "ACK", "OPTIONS", "BYE"];
//        allowed.forEach(
//          function(m) {
//            var h = SipClient.sipStack.syntaxFactory.createAllowHeader(m);
//            response.appendHeader(h);
//          });
//       rs.sendResponse(response);
//       return true;
//     }
    return false;
  },

  handleInviteRequest: function(rs) {
    var handler = InviteRSHandler.instantiate();
    handler.init(rs);
    return true;
  }
};

