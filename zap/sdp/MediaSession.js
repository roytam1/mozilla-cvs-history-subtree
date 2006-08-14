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
Components.utils.importModule("gre:zapICE.js");
Components.utils.importModule("gre:zapCodecRegistry.js");

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

// This attrib will be set to false, should it be apparent that our
// peer doesn't do ICE
MediaSession.obj("usingICE", true);

//----------------------------------------------------------------------
// zapIMediaSession
// INIT_PENDING --> INITIALIZED --> NEGOTIATED --> RUNNING --> TERMINATED

//  void init(in ACString originUsername,
//            in ACString originAddress,
//            in ACString callAudioIn,
//            in ACString callAudioOut,
//            in ACString callTEventIn,
//            [array, size_is(codec_count)] in string codecs,
//            in unsigned long codec_count,
//            [array, size_is(stunServer_count)] in string stunServers,
//            in unsigned long stunServer_count,
//            in zapIMediaSessionListener listener);
MediaSession.fun(
  function init(originUsername, originAddress, 
                callAudioIn, callAudioOut, callTEventIn,
                codecs, codec_count, stunServers, stunServer_count,
                listener) {
    this.changeState("INIT_PENDING");
    
    this.originUsername = originUsername;
    this.originAddress = originAddress;
    this.callAudioIn = callAudioIn;
    this.callAudioOut = callAudioOut;
    this.callTEventIn = callTEventIn;

    this.stunServers = stunServers;
    this.listener = listener;
    
    this.supportedRTPAVPFormats = [];
    for (var i=0; i < codec_count; ++i) {
      var codec = createCodecInstance(codecs[i]);
      if (codec) {
        this.supportedRTPAVPFormats.push(codec);
      }
      else {
        this._dump("Unknown codec '"+codecs[i]+"'");
      }
    }
    
    // We use getService here to retrieve the global media graph that
    // should have been set up by our client application:
    this.mediaGraph = Components.classes["@mozilla.org/zap/mediagraph;1"].getService(Components.interfaces.zapIMediaGraph);

    // asynchronously gather ICE candidates. Trigger 'INITIALIZED'
    // status from continuation.
    var me = this;
    function iceCandidatesProduced(candidates) {
      if (me.currentState == "TERMINATED") return; // we have already terminated
      me.iceCandidates = candidates;
      me.changeState("INITIALIZED");
      if (me.listener)
        me.listener.mediaSessionInitialized(me);
    }
    produceICECandidates(iceCandidatesProduced, 2, 49152, 5000, 50,
                         this.mediaGraph, this.stunServers,
                         this.originAddress);
  });

//  boolean isOfferAcceptable(in zapISdpSessionDescription offer);
MediaSession.fun(
  function isOfferAcceptable(offer) {
    try {
      var mediaDescriptions = offer.getMediaDescriptions({});
      // XXX assuming that there is only one media description for the moment:
      var d = mediaDescriptions[0].QueryInterface(Components.interfaces.zapISdpRtpAvpMediaDescription); 
      
      if (!this.matchRemotePayloadFormats(d.getRtpAvpFormats({})))
        throw("No matching payload formats!");
    }
    catch(e) {
      this._dump("Offer unacceptable ("+e+"). Offer: "+offer.serialize());
      return false;
    }
    return true;
  });

//  zapISdpSessionDescription generateSDPOffer();
MediaSession.statefun(
  "INITIALIZED",
  function generateSDPOffer() {
    // m=
    var mediaDescription = gSdpService.createRtpAvpMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.iceCandidates.activeCandidate.componentAt(0).port;
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
                                              this.iceCandidates.activeCandidate.componentAt(0).address,
                                              [mediaDescription], 1
                                              );

    if (this.usingICE) {
      encodeICECandidates(this.iceCandidates, offer, mediaDescription);
    }
    
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
MediaSession.statefun(
  "INITIALIZED",
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


    // parse ICE info:
    if (this.usingICE) {
      this.remoteICECandidates = extractICECandidates(offer, d);
      if (!this.remoteICECandidates)
        this.usingICE = false;
      else {
        this._dump("We have ICE candidates!");
        for (var i=0,l=this.remoteICECandidates.candidates.length; i<l; ++i)
          this._dump(this.remoteICECandidates.candidates[i]);
      }
    }
    
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
    answer.connection.address = this.iceCandidates.activeCandidate.componentAt(0).address;
    // t=
    var time = gSdpService.createTime();
    time.value = "0 0";
    answer.setTimes([time], 1);

    // media descriptions:
    var mediaDescription = gSdpService.createRtpAvpMediaDescription();
    mediaDescription.media = "audio";
    mediaDescription.port = this.iceCandidates.activeCandidate.componentAt(0).port;
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

    if (this.usingICE) {
      encodeICECandidates(this.iceCandidates, answer, mediaDescription);
    }
    
    return answer;
  });

//  void processSDPAnswer(in zapISdpSessionDescription answer);
MediaSession.statefun(
  "INITIALIZED",
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

    // parse remote ICE info:
    if (this.usingICE) {
      this.remoteICECandidates = extractICECandidates(answer, d);
      if (!this.remoteICECandidates)
        this.usingICE = false;
      else {
        this._dump("We have ICE candidates!");
        for (var i=0,l=this.remoteICECandidates.candidates.length; i<l; ++i)
          this._dump(this.remoteICECandidates.candidates[i]);

        // create the candidate pair list:
        this.iceCandidatePairs = createCandidatePairList(this.iceCandidates,
                                                         this.remoteICECandidates);
        this._dump("pair list has "+this.iceCandidatePairs.candidatePairs.length+ " entries");
      }
    }
    
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

//  void shutdown();
MediaSession.fun(
  function shutdown() {
    delete this.listener;

    // remove all the nodes we added to the graph:
    if (this.rtpsession) {
      this.mediaGraph.removeNode(this.buf);
      this.mediaGraph.removeNode(this.rtpsession);
      this.mediaGraph.removeNode(this.outboundSwitch);
      delete this.outboundSwitchCtl;
      this.mediaGraph.removeNode(this.outboundMerger);
      this.mediaGraph.removeNode(this.inboundMerger);
      this.mediaGraph.removeNode(this.demuxer);

      if (this.iceCandidates) {
        this.iceCandidates.destroy();
        delete this.iceCandidates;
      }
      
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
                            this.iceCandidates.activeCandidate.componentAt(0).getLocalMuxId(),
                            null);
    // decoder pipe:
    this.mediaGraph.connect(this.iceCandidates.activeCandidate.componentAt(0).getLocalDemuxId(), PB({$name:"other"}),
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
