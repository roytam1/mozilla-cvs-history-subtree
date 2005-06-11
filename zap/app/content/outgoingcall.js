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
var wDestAddressElement;
var wSipClient;
var wCallHandler;

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
  
  log("Parsing caller address");
  var callerAddress;
  try {
    callerAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wSipClient.getCallerAddress());
  } catch(e) {
    alert(wSipClient.getCallerAddress()+" is not a valid sip address! Please check Identity setup in main app window and try again.");
    return;
  }
  
  log("Parsing destination address");
  var destAddress;
  try {
    destAddress = wSipClient.sipStack.syntaxFactory.deserializeAddress(wDestAddressElement.value);
  } catch(e) {
    alert(wDestAddressElement.value+" is not a valid sip address! Please try again.");
    return;
  }

  log("Creating call handler");
  wCallHandler = CallHandler.instantiate();
  
  log("Formulating SDP offer");
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
//  attrib.value = "rtpmap:97 iLBC/8000";
  attrib.value = "rtpmap:97 speex/8000";
  attribs.push(attrib);
//  var attrib = wSipClient.sdpService.createAttrib();
//  attrib.value = "fmtp:97 mode=30";
//  attribs.push(attrib);
    
  mediaDescription.setAttribs(attribs, attribs.length);
    
  offer.setMediaDescriptions([mediaDescription], 1);

  log("Formulating Invite request");
  var request = wSipClient.sipStack.formulateInviteRequest(destAddress, callerAddress);
  request.setContent("application", "sdp", offer.serialize());

  log("Sending invite to "+destAddress.serialize());
  var inviteMC = wSipClient.sipStack.createInviteMC(request, wCallHandler);
  inviteMC.execute();  
}

function cmdHangup() {
  if (!wCallHandler) {
    alert("Call not active!");
    return;
  }
  wCallHandler.dialog.bye();
}

function cmdClearLog() {
  wLogElement.value = "";
}


////////////////////////////////////////////////////////////////////////
// CallHandler:

var CallHandler = makeClass("CallHandler", SupportsImpl);
CallHandler.addInterfaces(Components.interfaces.zapISipInviteMCH,
                          Components.interfaces.zapISipDialogHandler);

CallHandler.obj(
  "dialog", null);

CallHandler.obj(
  "rtpBase", 6000);

CallHandler.appendCtor(
  function() {
    log("Create media stream");
    var thread = Components.classes["@mozilla.org/thread;1"].createInstance(Components.interfaces.nsIThread);
    this.mediaStream = wSipClient.mediaService.createMediaStream(this.rtpBase, this.rtpBase+1);
    thread.init(this.mediaStream, 0, 1, 1, 1);
  });

CallHandler.fun(
  function destroy() {
    if (this.mediaStream) {
      this.mediaStream.close();
      this.mediaStream = null;
    }
    this.dialog = null;
    wCallHandler = null;
  });

// zapISipInviteMCH implementation:

CallHandler.fun(
  function dialogSpawned(method, dialog) {
    log("Early dialog established");
  });

CallHandler.fun(
  function calling(method, endpoint) {
    log("Calling "+endpoint.address+":"+endpoint.port+" via "+endpoint.transport);
  });

CallHandler.fun(
  function failureResponse(method, response) {
    log("Failure: "+response.statusCode+" ("+response.reasonPhrase+")");
  });

CallHandler.fun(
  function provisionalResponse(method, response) {
    log(response.statusCode+" ("+response.reasonPhrase+")");
  });

CallHandler.fun(
  function successResponse(method, response) {
    log("Call accepted: "+response.statusCode+" ("+response.reasonPhrase+")");
    // store response, so that we perform sdp negotiation in success()
    this.acceptResponse = response;
  });

CallHandler.fun(
  function failure(method) {
    log("Call failed!");
    this.destroy();
  });

CallHandler.fun(
  function success(method, dialog) {
    this.dialog = dialog;
    dialog.setDialogHandler(this);
    
    // parse the accept response's sdp to determine remoteHost,
    // rtp/rtcp ports and payload format:
    var remoteHost;
    var rtpPort;
    var payloadFormat;
    
    var sd;
    try {
      sd = wSipClient.sdpService.deserializeSessionDescription(this.acceptResponse.body);
    }
    catch(e) {
      log("Can't parse other party's session description: "+this.acceptResponse.body);
      this.destroy();
      return;
    }
    try {
      var mediaDescriptions = sd.getMediaDescriptions({});
      // XXX assuming that there is only one media description for the moment:
      
      rtpPort = mediaDescriptions[0].port;

      var connection = mediaDescriptions[0].connection;
      if (!connection)
        connection = sd.connection;
      remoteHost = connection.address;
      
//         // find media description that contains an a=rtpmap:xx iLBC/8000
//         // attribute to determine payload type:
//         var attribs = mediaDescriptions[0].getAttribs({});
//       var match;
//       for (var i=0,l=attribs.length; i<l; ++i)
//         if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i].value))) {
//           payloadFormat = match[1];
//           break;
//         }
      // find media description that contains an a=rtpmap:xx speex/8000
      // attribute to determine payload type:
      var attribs = mediaDescriptions[0].getAttribs({});
      var match;
      for (var i=0,l=attribs.length; i<l; ++i)
        if ((match = /rtpmap:(\d+) speex\/8000/(attribs[i].value))) {
          payloadFormat = match[1];
          break;
        }
    }
    catch(e) {
      log("Session negotiation failed: "+e);
      this.destroy();
      return;
    }
    
    // start sending/receiving audio:
    this.mediaStream.startReceive();
    this.mediaStream.startSend(remoteHost,
                               rtpPort,
                               rtpPort+1,
                               payloadFormat);

    log("Established speex/8000 ("+payloadFormat+") call to "+remoteHost+":"+rtpPort);
  });


// zapISipDialogHandler implementation:

CallHandler.fun(
  function dialogConfirmed(d) {
    log("Dialog established - we don't hit this, since we only handle already confirmed dialogs");
  });

CallHandler.fun(
  function dialogTerminated(d) {
    log("Call Terminated");
    this.destroy();
  });
