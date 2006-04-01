// -*- moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:MediaSession.js', null)" -*-
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

Components.utils.importModule("gre:ComponentUtils.jsm");
Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:ArrayUtils.js");
Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:ObjectUtils.js");

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
var gSdpService = Components.utils.importModule('gre:SdpService.js', null).theSdpService;


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
//            in ACString connectionAddress,
//            in ACString callAudioIn,
//            in ACString callAudioOut,
//            in ACString callTEventIn);
MediaSession.fun(
  function init(originUsername, originAddress, connectionAddress,
                callAudioIn, callAudioOut, callTEventIn,
                codecs, codec_count) {
    this.originUsername = originUsername;
    this.originAddress = originAddress;
    this.connectionAddress = connectionAddress;
    this.callAudioIn = callAudioIn;
    this.callAudioOut = callAudioOut;
    this.callTEventIn = callTEventIn;

    //--------------------------------------------------
    var SpeexFormatDescriptor = {};
    SpeexFormatDescriptor.localPayloadType = "97";
    SpeexFormatDescriptor.remotePayloadType = "";
    SpeexFormatDescriptor.sdpformat = gSdpService.createRtpAvpMediaFormat();
    SpeexFormatDescriptor.sdpformat.payloadType = SpeexFormatDescriptor.localPayloadType;
    SpeexFormatDescriptor.sdpformat.encodingName = "speex";
    SpeexFormatDescriptor.sdpformat.clockRate = "8000";
    
    SpeexFormatDescriptor.connect = function speex_connect(session) {
      if (!this.remotePayloadType) return;
      
      this.enc = session.mediaGraph.addNode("speex-encoder", null);
      this.dec = session.mediaGraph.addNode("speex-decoder", null);
      this.speex2rtp = session.mediaGraph.addNode("speex-rtp-packetizer",
                                              PB({$payload_type:this.remotePayloadType}));
      this.rtp2speex = session.mediaGraph.addNode("speex-rtp-depacketizer",
                                                  null);

      session.mediaGraph.connect(session.outboundSwitch,
                                 PB({$id:this.localPayloadType}),
                                 this.enc, null);
      session.mediaGraph.connect(this.enc, null, this.speex2rtp, null);    
      session.mediaGraph.connect(this.speex2rtp, null,
                                 session.outboundMerger,
                                 null);

      session.mediaGraph.connect(session.demuxer,
                                 PB({$payload_type:this.localPayloadType}),
                                 this.rtp2speex, null);
      session.mediaGraph.connect(this.rtp2speex, null, this.dec, null);
      session.mediaGraph.connect(this.dec, null, session.inboundMerger, null);
    };
    
    SpeexFormatDescriptor.disconnect = function speex_disconnect(session) {
      if (!this.remotePayloadType) return;
      
      session.mediaGraph.removeNode(this.enc);
      session.mediaGraph.removeNode(this.dec);
      session.mediaGraph.removeNode(this.speex2rtp);
      session.mediaGraph.removeNode(this.rtp2speex);
    };

    //--------------------------------------------------
    var PCMUFormatDescriptor = {};
    PCMUFormatDescriptor.localPayloadType = "0";
    PCMUFormatDescriptor.remotePayloadType = "";
    PCMUFormatDescriptor.sdpformat = gSdpService.createRtpAvpMediaFormat();
    PCMUFormatDescriptor.sdpformat.payloadType = PCMUFormatDescriptor.localPayloadType;
    PCMUFormatDescriptor.sdpformat.encodingName = "PCMU";
    PCMUFormatDescriptor.sdpformat.clockRate = "8000";
    
    PCMUFormatDescriptor.connect = function PCMU_connect(session) {
      if (!this.remotePayloadType) return;
      
      this.enc = session.mediaGraph.addNode("g711-encoder", PB({$type:"audio/pcmu"}));
      this.dec = session.mediaGraph.addNode("g711-decoder", null);
      this.g7112rtp = session.mediaGraph.addNode("g711-rtp-packetizer",
                                              PB({$payload_type:this.remotePayloadType}));
      this.rtp2g711 = session.mediaGraph.addNode("g711-rtp-depacketizer", PB({$type:"audio/pcmu"}));

      session.mediaGraph.connect(session.outboundSwitch,
                                 PB({$id:this.localPayloadType}),
                                 this.enc, null);
      session.mediaGraph.connect(this.enc, null, this.g7112rtp, null);    
      session.mediaGraph.connect(this.g7112rtp, null,
                                 session.outboundMerger,
                                 null);

      session.mediaGraph.connect(session.demuxer,
                                 PB({$payload_type:this.localPayloadType}),
                                 this.rtp2g711, null);
      session.mediaGraph.connect(this.rtp2g711, null, this.dec, null);
      session.mediaGraph.connect(this.dec, null, session.inboundMerger, null);
    };
    
    PCMUFormatDescriptor.disconnect = function PCMU_disconnect(session) {
      if (!this.remotePayloadType) return;
      
      session.mediaGraph.removeNode(this.enc);
      session.mediaGraph.removeNode(this.dec);
      session.mediaGraph.removeNode(this.g7112rtp);
      session.mediaGraph.removeNode(this.rtp2g711);
    };

    //--------------------------------------------------
    var PCMAFormatDescriptor = {};
    PCMAFormatDescriptor.localPayloadType = "8";
    PCMAFormatDescriptor.remotePayloadType = "";
    PCMAFormatDescriptor.sdpformat = gSdpService.createRtpAvpMediaFormat();
    PCMAFormatDescriptor.sdpformat.payloadType = PCMAFormatDescriptor.localPayloadType;
    PCMAFormatDescriptor.sdpformat.encodingName = "PCMA";
    PCMAFormatDescriptor.sdpformat.clockRate = "8000";

    PCMAFormatDescriptor.connect = function PCMA_connect(session) {

      if (!this.remotePayloadType) return;
      
      this.enc = session.mediaGraph.addNode("g711-encoder", PB({$type:"audio/pcma"}));
      this.dec = session.mediaGraph.addNode("g711-decoder", null);
      this.g7112rtp = session.mediaGraph.addNode("g711-rtp-packetizer",
                                              PB({$payload_type:this.remotePayloadType}));
      this.rtp2g711 = session.mediaGraph.addNode("g711-rtp-depacketizer", PB({$type:"audio/pcma"}));

      session.mediaGraph.connect(session.outboundSwitch,
                                 PB({$id:this.localPayloadType}),
                                 this.enc, null);
      session.mediaGraph.connect(this.enc, null, this.g7112rtp, null);    
      session.mediaGraph.connect(this.g7112rtp, null,
                                 session.outboundMerger,
                                 null);

      session.mediaGraph.connect(session.demuxer,
                                 PB({$payload_type:this.localPayloadType}),
                                 this.rtp2g711, null);
      session.mediaGraph.connect(this.rtp2g711, null, this.dec, null);
      session.mediaGraph.connect(this.dec, null, session.inboundMerger, null);
    };
    
    PCMAFormatDescriptor.disconnect = function PCMA_disconnect(session) {

      if (!this.remotePayloadType) return;
      
      session.mediaGraph.removeNode(this.enc);
      session.mediaGraph.removeNode(this.dec);
      session.mediaGraph.removeNode(this.g7112rtp);
      session.mediaGraph.removeNode(this.rtp2g711);
    };

    
    //--------------------------------------------------
    var TEventFormatDescriptor = {};
    TEventFormatDescriptor.localPayloadType = "101";
    TEventFormatDescriptor.remotePayloadType = "";
    TEventFormatDescriptor.sdpformat = gSdpService.createRtpAvpMediaFormat();
    TEventFormatDescriptor.sdpformat.payloadType = TEventFormatDescriptor.localPayloadType;
    TEventFormatDescriptor.sdpformat.encodingName = "telephone-event";
    TEventFormatDescriptor.sdpformat.clockRate = "8000";
    TEventFormatDescriptor.sdpformat.fmtParameters = "0-15";

    TEventFormatDescriptor.connect = function TEvent_connect(session) {
      if (!this.remotePayloadType) return;
      
      this.tevent2rtp = session.mediaGraph.addNode("tevent-rtp-packetizer",
                                                   PB({$payload_type:this.remotePayloadType}));
      this.sync = session.mediaGraph.addNode("stream-syncer", null);
      
      session.mediaGraph.connect(session.callTEventIn, null, this.sync, PB({$name:"input"}));
      session.mediaGraph.connect(session.callAudioIn, null, this.sync, PB({$name:"timebase"}));
      session.mediaGraph.connect(this.sync, null, this.tevent2rtp, null);
      session.mediaGraph.connect(this.tevent2rtp, null,
                                 session.outboundMerger,
                                 null);
    };

    TEventFormatDescriptor.disconnect = function TEvent_disconnect(session) {
      if (!this.remotePayloadType) return;
      
      session.mediaGraph.removeNode(this.sync);
      session.mediaGraph.removeNode(this.tevent2rtp);      
    };
    
    //--------------------------------------------------
    this.supportedRTPAVPFormats = [];
    for (var i=0; i < codec_count; ++i) {
      switch(codecs[i]) {
        case "PCMU":
          this.supportedRTPAVPFormats.push(PCMUFormatDescriptor);
          break;
        case "PCMA":
          this.supportedRTPAVPFormats.push(PCMAFormatDescriptor);
          break;
        case "speex":
          this.supportedRTPAVPFormats.push(SpeexFormatDescriptor);
          break;
        case "telephone-event":
          this.supportedRTPAVPFormats.push(TEventFormatDescriptor);
          break;
      }
    }
    
    // We use getService here to retrieve the global media graph that
    // should have been set up by our client application:
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

    // construct array of offered formats:
    var formatsOffered = [];
    for (var i=0,l=this.supportedRTPAVPFormats.length; i<l; ++i) {
      formatsOffered.push(this.supportedRTPAVPFormats[i].sdpformat);
    }
    mediaDescription.setRtpAvpFormats(formatsOffered,
                                      formatsOffered.length);
    
    var offer = gSdpService.formulateSDPOffer(this.originUsername,
                                              this.originAddress,
                                              this.connectionAddress,
                                              [mediaDescription], 1
                                              );

    return offer;
  });

// find matching payload formats among our list of supported formats
// and set remotePayloadType as appropriate. Returns true if there are
// matching formats
MediaSession.fun(
  function matchRemotePayloadFormats(remoteFormats) {
    var hasMatchingFormats = false;
    for (var i=0,l=remoteFormats.length; i<l; ++i) {
      for (var j=0,m=this.supportedRTPAVPFormats.length; j<m; ++j) {
        if (remoteFormats[i].encodingName == this.supportedRTPAVPFormats[j].sdpformat.encodingName &&
            remoteFormats[i].clockRate == this.supportedRTPAVPFormats[j].sdpformat.clockRate &&
            (remoteFormats[i].encodingParameters == "1" ||
             !remoteFormats[i].encodingParameters)) {
          this.supportedRTPAVPFormats[j].remotePayloadType = remoteFormats[i].payloadType;
          hasMatchingFormats = true;
          break;
        }
      }
    }
    return hasMatchingFormats;
  });

//  zapISdpSessionDescription processSDPOffer(in zapISdpSessionDescription offer);
MediaSession.fun(
  function processSDPOffer(offer) {
    var remoteHost, remoteRTPPort, remoteRTCPPort;
    
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

      if (!this.matchRemotePayloadFormats(d.getRtpAvpFormats({})))
        throw("No matching payload formats!");
    }
    catch(e) {
      this._verboseError("Session negotiation failed for offer ["+offer.serialize()+"]: "+e);
    }

    // The offer is acceptable.
    // Set session parameters:
    this.remoteHost = remoteHost;
    this.remoteRTPPort = remoteRTPPort;
    this.remoteRTCPPort = remoteRTCPPort;

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
    // construct array of accepted formats:
    var formatsAccepted = [];
    for (var i=0,l=this.supportedRTPAVPFormats.length; i<l; ++i) {
      if (this.supportedRTPAVPFormats[i].remotePayloadType)
        formatsAccepted.push(this.supportedRTPAVPFormats[i].sdpformat);
    }

    
    mediaDescription.setRtpAvpFormats(formatsAccepted,
                                      formatsAccepted.length);
    
    answer.setMediaDescriptions([mediaDescription], 1);

    return answer;
  });

//  void processSDPAnswer(in zapISdpSessionDescription answer);
MediaSession.fun(
  function processSDPAnswer(answer) {
    var remoteHost, remoteRTPPort, remoteRTCPPort;
    
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

      if (!this.matchRemotePayloadFormats(d.getRtpAvpFormats({})))
        throw("No matching payload formats!");
    }
    catch(e) {
      this._verboseError("Session negotiation failed for answer ["+answer.serialize()+"]: "+e);
    }

    // The answer is acceptable.
    // Set session parameters:
    this.remoteHost = remoteHost;
    this.remoteRTPPort = remoteRTPPort;
    this.remoteRTCPPort = remoteRTCPPort;

    // change state:
    this.changeState("NEGOTIATED");
  });

//  void startSession();
MediaSession.statefun(
  "NEGOTIATED",
  function startSession() {
    this.start(this.remoteHost, this.remoteRTPPort,
               this.remoteRTCPPort);
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
      this.mediaGraph.removeNode(this.buf);
      this.mediaGraph.removeNode(this.rtpsession);
      this.mediaGraph.removeNode(this.outboundSwitch);
      delete this.outboundSwitchCtl;
      this.mediaGraph.removeNode(this.outboundMerger);
      this.mediaGraph.removeNode(this.inboundMerger);
      this.mediaGraph.removeNode(this.demuxer);
      
      // disconnect our codecs:
      for (var i=0,l=this.supportedRTPAVPFormats.length; i<l; ++i) {
        this.supportedRTPAVPFormats[i].disconnect(this);
      }
    }
    delete this.mediaGraph;
    
    this.changeState("TERMINATED");
  });

//----------------------------------------------------------------------
// Implementation helpers:

MediaSession.fun(
  function start(remoteHost, remoteRTPPort,
                 remoteRTCPPort) {    
    this.buf = this.mediaGraph.addNode("buffer", PB({$lift_count:15, $drop_count:15, $max_size:30}));
    this.outboundSwitch = this.mediaGraph.addNode("stream-switch", null);
    this.outboundSwitchCtl = this.mediaGraph.getNode(this.outboundSwitch,
                                                     Components.interfaces.zapIStreamSwitch,
                                                     true);
    this.outboundMerger = this.mediaGraph.addNode("stream-merger", null);
    this.inboundMerger = this.mediaGraph.addNode("stream-merger", null);
    this.demuxer = this.mediaGraph.addNode("rtp-demuxer", null);
    this.rtpsession = this.mediaGraph.addNode("rtp-session", null);

    this.rtptransmitter = this.mediaGraph.addNode("rtp-transmitter",
                                              PB({$address:remoteHost,
                                                  $port:remoteRTPPort}));
    this.rtpreceiver = this.mediaGraph.addNode("rtp-receiver", null);
    
    // encoder pipe:
    this.mediaGraph.connect(this.callAudioIn, null,
                            this.outboundSwitch, null);
    this.mediaGraph.connect(this.outboundMerger, null,
                            this.rtpsession, PB({$name:"local2remote-rtp"}));
    this.mediaGraph.connect(this.rtpsession, PB({$name:"local2remote-rtp"}),
                            this.rtptransmitter, null);
    this.mediaGraph.connect(this.rtptransmitter, null,
                            this.socketpair, PB({$name:"socket-a"}));
    // decoder pipe:
    this.mediaGraph.connect(this.socketpair, PB({$name:"socket-a"}),
                            this.rtpreceiver, null);
    this.mediaGraph.connect(this.rtpreceiver, null,
                            this.rtpsession, PB({$name:"remote2local-rtp"}));
    this.mediaGraph.connect(this.rtpsession, PB({$name:"remote2local-rtp"}),
                            this.demuxer, null);
    this.mediaGraph.connect(this.inboundMerger, null, 
                            this.buf, null);
    this.mediaGraph.connect(this.buf, null, this.callAudioOut, null);

    // connect our codecs:
    var haveOutbound = false;
    for (var i=0,l=this.supportedRTPAVPFormats.length; i<l; ++i) {
      this.supportedRTPAVPFormats[i].connect(this);
      if (!haveOutbound && this.supportedRTPAVPFormats[i].remotePayloadType) {
        this.outboundSwitchCtl.selectOutput(this.supportedRTPAVPFormats[i].localPayloadType);
        haveOutbound = true;
      } 
    }
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Media Session",
     cid        : Components.ID("{6ec361e6-eb0d-40ae-a2a5-5bcc942784b6}"),
     contractID : "@mozilla.org/zap/mediasession;1",
     factory    : ComponentUtils.generateFactory(function() { return MediaSession.instantiate(); })
  }]);
