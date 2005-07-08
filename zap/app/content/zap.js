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
  this.wLogger.log("UI", level, mes);
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
  }
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
    SipClient.sipStack.init(gInviteMSHFactory);
    
    log("Getting Media Service");
    SipClient.mediaService = Components.classes["@mozilla.org/zap/mediaservice;1"].
      getService(Components.interfaces.zapIMediaService);
    
    // initialize host-name to our ip address:
    wHostNameElement.value = SipClient.sipStack.hostAddress;

    log("...Sip Client Initialization done.");
  }
  catch(e) {
    log("Error during Sip Client initialization: "+e);
  }
}

////////////////////////////////////////////////////////////////////////
// INVITE Method Server Handler:

var InviteMSH = makeClass("InviteMSH", SupportsImpl);
InviteMSH.addInterfaces(Components.interfaces.zapISipInviteMSH,
                        Components.interfaces.zapISipDialogHandler);

InviteMSH.appendCtor(
  function() {
    log("Create media stream");
    var thread = Components.classes["@mozilla.org/thread;1"].createInstance(Components.interfaces.nsIThread);
    this.mediaStream = SipClient.mediaService.createMediaStream(this.rtpBase, this.rtpBase+1);
    thread.init(this.mediaStream, 0, 1, 1, 1);
  });


InviteMSH.obj(
  "dialog", null);

// this will be overwritten by the incomingcall window:
InviteMSH.obj("log", log);

// this will be overwritten by the incomingcall window:
InviteMSH.obj("destroyHook", null);

// this will be overwritten by the incomingcall window:
InviteMSH.obj("callSuccessHook", null);

InviteMSH.obj(
  "rtpBase", 6000);


InviteMSH.fun(
  function destroy() {
     if (this.mediaStream) {
       this.mediaStream.close();
       this.mediaStream = null;
     }
    this.dialog = null;
    if (this.destroyHook)
      this.destroyHook();
  });


//----------------------------------------------------------------------
// zapISipInviteMSH implementation:
InviteMSH.fun(
  function invite(ms, request) {
    if (wCallRejectElement.checked) {
      log("Rejecting INVITE");
      var r = SipClient.sipStack.formulateResponse("603", "Decline", request, true);
      ms.sendResponse(r);
      this.destroy();
    }
    else {

      // parse the INVITE's sdp to see if it is acceptable:
      var sd;
      try {
        sd = SipClient.sdpService.deserializeSessionDescription(request.body);
      }
      catch(e) {
        log("Can't parse other party's session description: "+request.body);
        var r = SipClient.sipStack.formulateResponse("603", "Decline", request, true);
        ms.sendResponse(r);
        this.destroy();
        return;
      }
      try {
        var mediaDescriptions = sd.getMediaDescriptions({});
        // XXX assuming that there is only one media description for the moment:
        
        this.remoteRtpPort = mediaDescriptions[0].port;
        
        var connection = mediaDescriptions[0].connection;
        if (!connection)
          connection = sd.connection;
        this.remoteMediaHost = connection.address;
        
//         // find media description that contains an a=rtpmap:xx iLBC/8000
//         // attribute to determine payload type:
//         var attribs = mediaDescriptions[0].getAttribs({});
//         var match;
//         for (var i=0,l=attribs.length; i<l; ++i)
//           if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i].value))) {
//             this.remotePayloadFormat = match[1];
//             break;
//           }
        
        // find media description that contains an a=rtpmap:xx speex/8000
        // attribute to determine payload type:
        var attribs = mediaDescriptions[0].getAttribs({});
        var match;
        for (var i=0,l=attribs.length; i<l; ++i)
          if ((match = /rtpmap:(\d+) speex\/8000/(attribs[i].value))) {
            this.remotePayloadFormat = match[1];
            break;
          }
      }
      catch(e) {
        log("Session negotiation for offer ["+sd.serialize()+"] failed: "+e);
        var r = SipClient.sipStack.formulateResponse("603", "Decline", request, true);
        ms.sendResponse(r);
        this.destroy();
        return;
      }
      // if we get to here, the remote host's offering is acceptable
      this.ms = ms;
      this.request = request;
      window.openDialog("chrome://zap/content/incomingcall.xul", "call"+(wCount++), "chrome, resizable", this);
    }
  });

InviteMSH.fun(
  function dialogSpawned(method, dialog) {
    // handle new dialog in success()
  });

InviteMSH.fun(
  function failure(method) {
    log("Call terminated");
    this.destroy();
  });

InviteMSH.fun(
  function success(method, dialog) {
    log("Call succeeded");
    this.dialog = dialog;
    dialog.setDialogHandler(this);
    if (this.callSuccessHook)
      this.callSuccessHook();
    // start sending/receiving audio:
    this.mediaStream.startReceive();
    this.mediaStream.startSend(this.remoteMediaHost,
                               this.remoteRtpPort,
                               this.remoteRtpPort+1,
                               this.remotePayloadFormat);

    this.log("Established speex/8000 ("+this.remotePayloadFormat+") call to "+this.remoteMediaHost+":"+this.remoteRtpPort);
  });

// zaISipDialogHandler implementation:

InviteMSH.fun(
  function dialogConfirmed(d) {
    log("Dialog established - we don't hit this, since we only handle already confirmed dialogs");
  });

InviteMSH.fun(
  function dialogTerminated(d) {
    log("Call Terminated");
    this.destroy();
  });


////////////////////////////////////////////////////////////////////////
// Method Server Handler Factory:

var gInviteMSHFactory = {
  createInviteMSH : function() {
    return InviteMSH.instantiate();
  }
};

