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

    // We use getService here to retrieve the global media graph that
    // should have been set up by our client application:
    // This graph must contain an 'ain' and an 'aout' node:
    this.mediaGraph = Components.classes["@mozilla.org/zap/mediagraph;1"].getService(Components.interfaces.zapIMediaGraph);
    this.socketpair = this.mediaGraph.addNode("udp-socket-pair", null);
    this.changeState("INITIALIZED");
  });

//  zapISdpSessionDescription generateSDPOffer();
MediaSession.fun(
  function generateSDPOffer() {
    // m=
    var mediaDescription = gSdpService.createRtpAvpMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.localRTPPort;
    mediaDescription.portCount = "";
    mediaDescription.protocol = "RTP/AVP";
    var formatPCMU = gSdpService.createRtpAvpMediaFormat();
    formatPCMU.payloadType = "0";
    formatPCMU.encodingName = "pcmu";
    formatPCMU.clockRate = "8000";
    formatPCMU.encodingParameters = "1";
//    mediaDescription.setFormats(["97"], 1);
    mediaDescription.setRtpAvpFormats([formatPCMU], 1);
    // a=
//    var attribs = [];
    //attribs.push("rtpmap:97 speex/8000");
//    attribs.push("rtpmap:0 pcmu/8000/1");
    //attribs.push("rtpmap:97 iLBC/8000");
// //attribs.push("fmtp:97 mode=30");
//    mediaDescription.setAttribs(attribs, attribs.length);
    
    var offer = gSdpService.formulateSDPOffer(this.originUsername,
                                              this.originAddress,
                                              this.connectionAddress,
                                              [mediaDescription], 1
                                              );

    return offer;
  });

//  zapISdpSessionDescription processSDPOffer(in zapISdpSessionDescription offer);
MediaSession.fun(
  function processSDPOffer(offer) {
    var remoteHost, remoteRTPPort, remoteRTCPPort, remotePayloadFormat;
    
    try {
      var mediaDescriptions = offer.getMediaDescriptions({});
      // XXX assuming that there is only one media description for the moment:
      var d = mediaDescriptions[0].QueryInterface(Components.interfaces.zapISdpRtpAvpMediaDescription); 
      remoteRTPPort = d.port;
      remoteRTCPPort = remoteRTPPort+1;
      
      var connection = d.connection;
      if (!connection)
           connection = offer.connection;
      remoteHost = connection.address;
        
// //         // find media description that contains an a=rtpmap:xx iLBC/8000
// //         // attribute to determine payload type:
// //         var attribs = mediaDescriptions[0].getAttribs({});
// //         var match;
// //         for (var i=0,l=attribs.length; i<l; ++i)
// //           if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i]))) {
// //             remotePayloadFormat = match[1];
// //             break;
// //           }
        
      // find media description that contains an a=rtpmap:xx speex/8000
      // attribute to determine payload type:
      var formats = d.getRtpAvpFormats({});
      for (var i=0,l=formats.length;i<l; ++i)
        if (formats[i].encodingName == "pcmu" &&
            formats[i].clockRate == "8000") {
          remotePayloadFormat = formats[i].payloadType;
          break;
        }
//       var attribs = d.getAttribs({});
//       var match;
//       for (var i=0,l=attribs.length; i<l; ++i)
//         if ((match = /rtpmap:(\d+) pcmu\/8000/i(attribs[i]))) {
//           remotePayloadFormat = match[1];
//           break;
//         }
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

    // media descriptions:
    var mediaDescription = gSdpService.createRtpAvpMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.localRTPPort;
    mediaDescription.portCount = "";
    mediaDescription.protocol = "RTP/AVP";
    var formatPCMU = gSdpService.createRtpAvpMediaFormat();
    formatPCMU.payloadType = "0";
    formatPCMU.encodingName = "pcmu";
    formatPCMU.clockRate = "8000";
    formatPCMU.encodingParameters = "1";
    mediaDescription.setRtpAvpFormats([formatPCMU], 1);
    
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
      var d = mediaDescriptions[0].QueryInterface(Components.interfaces.zapISdpRtpAvpMediaDescription);
        
      remoteRTPPort = d.port;
      remoteRTCPPort = remoteRTPPort+1;
      
      var connection = d.connection;
      if (!connection)
           connection = answer.connection;
      remoteHost = connection.address;
        
// //         // find media description that contains an a=rtpmap:xx iLBC/8000
// //         // attribute to determine payload type:
// //         var attribs = mediaDescriptions[0].getAttribs({});
// //         var match;
// //         for (var i=0,l=attribs.length; i<l; ++i)
// //           if ((match = /rtpmap:(\d+) iLBC\/8000/(attribs[i]))) {
// //             remotePayloadFormat = match[1];
// //             break;
// //           }
      var formats = d.getRtpAvpFormats({});
      for (var i=0,l=formats.length;i<l; ++i)
        if (formats[i].encodingName == "pcmu" &&
            formats[i].clockRate == "8000") {
          remotePayloadFormat = formats[i].payloadType;
          break;
        }        
//       // find media description that contains an a=rtpmap:xx speex/8000
//       // attribute to determine payload type:
//       var attribs = d.getAttribs({});
//       var match;
//       for (var i=0,l=attribs.length; i<l; ++i)
//         if ((match = /rtpmap:(\d+) pcmu\/8000/i(attribs[i]))) {
//           remotePayloadFormat = match[1];
//           break;
//         }
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
    // remove all the nodes we added to the graph:
    this.mediaGraph.removeNode(this.socketpair);
    if (this.rtpsession) {
      this.mediaGraph.removeNode(this.enc);
      this.mediaGraph.removeNode(this.dec);
//    this.mediaGraph.removeNode(this.buf1);
      this.mediaGraph.removeNode(this.buf2);
      this.mediaGraph.removeNode(this.speex2rtp);
      this.mediaGraph.removeNode(this.rtp2speex);
      this.mediaGraph.removeNode(this.rtpsession);
    }
    delete this.mediaGraph;
    
    this.changeState("TERMINATED");
  });

//----------------------------------------------------------------------
// Implementation helpers:

MediaSession.fun(
  function start(remoteHost, remoteRTPPort,
                 remoteRTCPPort, remotePayloadFormat) {
//    this.enc = this.mediaGraph.addNode("speex-encoder", null);
//    this.dec = this.mediaGraph.addNode("speex-decoder", null);
    this.enc = this.mediaGraph.addNode("g711-encoder", PB({$type:"audio/pcmu"}));
    this.dec = this.mediaGraph.addNode("g711-decoder", null);
//    this.buf1 = this.mediaGraph.addNode("buffer", PB({$prefill_size:1, $max_size:10}));
    this.buf2 = this.mediaGraph.addNode("buffer", PB({$prefill_size:4, $max_size:30}));
//     this.speex2rtp = this.mediaGraph.addNode("speex-rtp-packetizer",
//                                              PB({$payload_type:remotePayloadFormat}));
//     this.rtp2speex = this.mediaGraph.addNode("speex-rtp-depacketizer", null);
    this.speex2rtp = this.mediaGraph.addNode("g711-rtp-packetizer",
                                             PB({$payload_type:remotePayloadFormat}));
    this.rtp2speex = this.mediaGraph.addNode("g711-rtp-depacketizer", PB({$type:"audio/pcmu"}));
    this.rtpsession = this.mediaGraph.addNode("rtp-session",
                                              PB({$address:remoteHost,
                                                  $rtp_port:remoteRTPPort,
                                                  $rtcp_port:remoteRTCPPort}));
    // encoder pipe:
    this.A = this.mediaGraph.connect("ain", null, this.enc, null);
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
    this.J = this.mediaGraph.connect(this.dec, null, "aout", null);
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Media Session",
     cid        : Components.ID("{6ec361e6-eb0d-40ae-a2a5-5bcc942784b6}"),
     contractID : "@mozilla.org/zap/mediasession;1",
     factory    : ComponentUtils.generateFactory(function() { return MediaSession.instantiate(); })
  }]);
