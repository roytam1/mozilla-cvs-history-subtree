// -*- moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:MediaSession.js')" -*-
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


debug("*** loading MediaSession\n");

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");

// name our global object:
function toString() { return "[MediaSession.js]"; }

// object to hold component's documentation:
var _doc_ = {};

var PB = makePropertyBag;

////////////////////////////////////////////////////////////////////////
// gSdpService: global sdp service instance

// access the sdp service via xpcom (more overhead, all syntax
// objects need to be xpcom-wrapped):
//var gSdpService =
//  Components.classes['@mozilla.org/zap/sdpservice;1']
//      .getService(Components.interfaces.zapISdpService);

// access the sdp service directly (less overhead, no type safety,
// access to non-xpcom interface):
var gSdpService = Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SdpService.js').theSdpService;


////////////////////////////////////////////////////////////////////////
// Class MediaSession
// Maintains an SDP-negotiated media session

var MediaSession = makeClass("MediaSession",
                             SupportsImpl, Unwrappable, StateMachine);
MediaSession.addInterfaces(Components.interfaces.zapIMediaSession);

//----------------------------------------------------------------------
// zapIMediaSession
// INITIALIZED --> NEGOTIATED --> RUNNING --> TERMINATED

//  void init(in ACString originUsername,
//            in ACString originAddress,
//            in ACString connectionAddress);
MediaSession.fun(
  function init(originUsername, originAddress, connectionAddress) {
    this.originUsername = originUsername;
    this.originAddress = originAddress;
    this.connectionAddress = connectionAddress;

    this.mediaGraph = Components.classes["@mozilla.org/zap/mediagraph;1"].createInstance(Components.interfaces.zapIMediaGraph);
    this.socketpair = this.mediaGraph.addNode("udp-socket-pair", null);
    this.changeState("INITIALIZED");
  });

//  zapISdpSessionDescription generateSDPOffer();
MediaSession.fun(
  function generateSDPOffer() {
    var offer = gSdpService.createSessionDescription();
    // o=
    offer.username = this.originUsername;
    offer.sessionID = "0";
    offer.sessionVersion = "0";
    offer.originAddressType = "IP4";
    offer.originAddress = this.originAddress;
    // s=
    offer.sessionName = " ";
    // c=
    offer.connection = gSdpService.createConnection();
    offer.connection.addressType = "IP4";
    offer.connection.address = this.connectionAddress;
    // t=
    var time = gSdpService.createTime();
    time.value = "0 0";
    offer.setTimes([time], 1);
    // m=
    var mediaDescription = gSdpService.createMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.localRTPPort;
    mediaDescription.portCount = "";
    mediaDescription.protocol = "RTP/AVP";
    var fmt = gSdpService.createMediaFormat();
    fmt.format  ="97";
    mediaDescription.setFormats([fmt], 1);
    // a=
    var attribs = [];
    var attrib = gSdpService.createAttrib();
    attrib.value = "rtpmap:97 speex/8000";
    attribs.push(attrib);
    // //  attrib.value = "rtpmap:97 iLBC/8000";
// //  var attrib = wSipClient.sdpService.createAttrib();
// //  attrib.value = "fmtp:97 mode=30";
// //  attribs.push(attrib);
    mediaDescription.setAttribs(attribs, attribs.length);

    offer.setMediaDescriptions([mediaDescription], 1);

    return offer;
  });

//  zapISdpSessionDescription processSDPOffer(in zapISdpSessionDescription offer);
MediaSession.fun(
  function processSDPOffer(offer) {
    var remoteHost, remoteRTPPort, remoteRTCPPort, remotePayloadFormat;
    
    try {
      var mediaDescriptions = offer.getMediaDescriptions({});
      // XXX assuming that there is only one media description for the moment:
        
      remoteRTPPort = mediaDescriptions[0].port;
      remoteRTCPPort = remoteRTPPort+1;
      
      var connection = mediaDescriptions[0].connection;
      if (!connection)
           connection = offer.connection;
      remoteHost = connection.address;
        
// //         // find media description that contains an a=rtpmap:xx iLBC/8000
// //         // attribute to determine payload type:
// //         var attribs = mediaDescriptions[0].getAttribs({});
// //         var match;
// //         for (var i=0,l=attribs.length; i<l; ++i)
// //           if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i].value))) {
// //             remotePayloadFormat = match[1];
// //             break;
// //           }
        
      // find media description that contains an a=rtpmap:xx speex/8000
      // attribute to determine payload type:
      var attribs = mediaDescriptions[0].getAttribs({});
      var match;
      for (var i=0,l=attribs.length; i<l; ++i)
        if ((match = /rtpmap:(\d+) speex\/8000/(attribs[i].value))) {
          remotePayloadFormat = match[1];
          break;
        }
    }
    catch(e) {
      this._verboseError("Session negotiation failed for offer ["+offer.serialize()+"]: "+e);
    }

    // The offer is acceptable.
    // Set session parameters:
    this.remoteHost = remoteHost;
    this.remoteRTPPort = remoteRTPPort;
    this.remoteRTCPPort = remoteRTCPPort;
    this.remotePayloadFormat = remotePayloadFormat;

    // change state:
    this.changeState("NEGOTIATED");
        
    // Generate answer:
    // XXX do this properly.
    var answer = gSdpService.createSessionDescription();
    // o=
    answer.username = this.originUsername;
    answer.sessionID = "0";
    answer.sessionVersion = "0";
    answer.originAddressType = "IP4";
    answer.originAddress = this.originAddress;
    // s=
    answer.sessionName = " ";
    // c=
    answer.connection = gSdpService.createConnection();
    answer.connection.addressType = "IP4";
    answer.connection.address = this.connectionAddress;
    // t=
    var time = gSdpService.createTime();
    time.value = "0 0";
    answer.setTimes([time], 1);
    // m=
    var mediaDescription = gSdpService.createMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.localRTPPort;
    mediaDescription.portCount = "";
    mediaDescription.protocol = "RTP/AVP";
    var fmt = gSdpService.createMediaFormat();
    fmt.format = "97";
    mediaDescription.setFormats([fmt], 1);
    
    // a=
    var attribs = [];
    var attrib = gSdpService.createAttrib();
//    attrib.value = "rtpmap:97 iLBC/8000";
    attrib.value = "rtpmap:97 speex/8000";
    attribs.push(attrib);
//     var attrib = wSipClient.sdpService.createAttrib();
//     attrib.value = "fmtp:97 mode=30";
//     attribs.push(attrib);
    
    mediaDescription.setAttribs(attribs, attribs.length);
    
    answer.setMediaDescriptions([mediaDescription], 1);

    return answer;
  });

//  void processSDPAnswer(in zapISdpSessionDescription answer);
MediaSession.fun(
  function processSDPAnswer(answer) {
    var remoteHost, remoteRTPPort, remoteRTCPPort, remotePayloadFormat;
    
    try {
      var mediaDescriptions = answer.getMediaDescriptions({});
      // XXX assuming that there is only one media description for the moment:
        
      remoteRTPPort = mediaDescriptions[0].port;
      remoteRTCPPort = remoteRTPPort+1;
      
      var connection = mediaDescriptions[0].connection;
      if (!connection)
           connection = answer.connection;
      remoteHost = connection.address;
        
// //         // find media description that contains an a=rtpmap:xx iLBC/8000
// //         // attribute to determine payload type:
// //         var attribs = mediaDescriptions[0].getAttribs({});
// //         var match;
// //         for (var i=0,l=attribs.length; i<l; ++i)
// //           if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i].value))) {
// //             remotePayloadFormat = match[1];
// //             break;
// //           }
        
      // find media description that contains an a=rtpmap:xx speex/8000
      // attribute to determine payload type:
      var attribs = mediaDescriptions[0].getAttribs({});
      var match;
      for (var i=0,l=attribs.length; i<l; ++i)
        if ((match = /rtpmap:(\d+) speex\/8000/(attribs[i].value))) {
          remotePayloadFormat = match[1];
          break;
        }
    }
    catch(e) {
      this._verboseError("Session negotiation failed for answer ["+answer.serialize()+"]: "+e);
    }

    // The answer is acceptable.
    // Set session parameters:
    this.remoteHost = remoteHost;
    this.remoteRTPPort = remoteRTPPort;
    this.remoteRTCPPort = remoteRTCPPort;
    this.remotePayloadFormat = remotePayloadFormat;

    // change state:
    this.changeState("NEGOTIATED");
  });

//  void startSession();
MediaSession.statefun(
  "NEGOTIATED",
  function startSession() {
    this.start(this.remoteHost, this.remoteRTPPort,
               this.remoteRTCPPort, this.remotePayloadFormat);
    this.changeState("RUNNING");
  });

//  readonly attribute unsigned short localRTPPort;
MediaSession.getter(
  "localRTPPort",
  function getLocalRTPPort() {
    var sp = this.mediaGraph.getNode(this.socketpair, Components.interfaces.zapIUDPSocketPair, true);
    return sp.portA;
  });

//  readonly attribute unsigned short localRTCPPort;
MediaSession.getter(
  "localRTCPPort",
  function getLocalRTCPPort() {
    var sp = this.mediaGraph.getNode(this.socketpair, Components.interfaces.zapIUDPSocketPair, true);
    return sp.portB;
  });


//  void shutdown();
MediaSession.fun(
  function shutdown() {
    this.mediaGraph.shutdown();
    this.changeState("TERMINATED");
  });

//----------------------------------------------------------------------
// Implementation helpers:

MediaSession.fun(
  function start(remoteHost, remoteRTPPort,
                 remoteRTCPPort, remotePayloadFormat) {
    this.ain = this.mediaGraph.addNode("audioin", null);
    this.aout = this.mediaGraph.addNode("audioout", null);
    this.enc = this.mediaGraph.addNode("speex-encoder", null);
    this.dec = this.mediaGraph.addNode("speex-decoder", null);
//    this.buf1 = this.mediaGraph.addNode("buffer", PB({$prefill_size:1, $max_size:10}));
    this.buf2 = this.mediaGraph.addNode("buffer", PB({$prefill_size:4, $max_size:30}));
    this.speex2rtp = this.mediaGraph.addNode("speex-rtp-packetizer",
                                             PB({$payload_type:remotePayloadFormat}));
    this.rtp2speex = this.mediaGraph.addNode("speex-rtp-depacketizer", null);
    this.rtpsession = this.mediaGraph.addNode("rtp-session",
                                              PB({$address:remoteHost,
                                                  $rtp_port:remoteRTPPort,
                                                  $rtcp_port:remoteRTCPPort}));
    // encoder pipe:
    this.A = this.mediaGraph.connect(this.ain, null, this.enc, null);
    this.B = this.mediaGraph.connect(this.enc, null, this.speex2rtp, null);
    this.C = this.mediaGraph.connect(this.speex2rtp, null,
                                     this.rtpsession, PB({$name:"local-rtp"}));
    this.D = this.mediaGraph.connect(this.rtpsession, PB({$name:"remote-rtp"}),
//                                     this.buf1, null);
//    this.E = this.mediaGraph.connect(this.buf1, null,
                                     this.socketpair, PB({$name:"socket-a"}));
    // decoder pipe:
    this.F = this.mediaGraph.connect(this.socketpair, PB({$name:"socket-a"}),
                                     this.buf2, null);
    this.G = this.mediaGraph.connect(this.buf2, null,
                                     this.rtpsession, PB({$name:"remote-rtp"}));
    this.H = this.mediaGraph.connect(this.rtpsession, PB({$name:"local-rtp"}),
                                     this.rtp2speex, null);
    this.I = this.mediaGraph.connect(this.rtp2speex, null, this.dec, null);
    this.J = this.mediaGraph.connect(this.dec, null, this.aout, null);
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Media Session",
     cid        : Components.ID("{6ec361e6-eb0d-40ae-a2a5-5bcc942784b6}"),
     contractID : "@mozilla.org/zap/mediasession;1",
     factory    : ComponentUtils.generateFactory(function() { return MediaSession.instantiate(); })
  }]);
