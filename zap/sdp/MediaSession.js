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
// Class MediaSession
// Maintains an SDP-negotiated media session

var MediaSession = makeClass("MediaSession", SupportsImpl);
MediaSession.addInterfaces(Components.interfaces.zapIMediaSession);

MediaSession.appendCtor(
  function() {
    this.mediaGraph = Components.classes["@mozilla.org/zap/mediagraph;1"].createInstance(Components.interfaces.zapIMediaGraph);
    this.socketpair = this.mediaGraph.addNode("udp-socket-pair", null);
  });    

//----------------------------------------------------------------------
// zapIMediaSession

MediaSession.fun(
  function start(remoteHost, remoteRTPPort,
                 remoteRTCPPort, remotePayloadFormat) {
    this.ain = this.mediaGraph.addNode("audioin", null);
    this.aout = this.mediaGraph.addNode("audioout", null);
    this.enc = this.mediaGraph.addNode("speex-encoder", null);
    this.dec = this.mediaGraph.addNode("speex-decoder", null);
    this.buf1 = this.mediaGraph.addNode("buffer", PB({$prefill_size:2, $max_size:10}));
    this.buf2 = this.mediaGraph.addNode("buffer", PB({$prefill_size:2, $max_size:10}));
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
                                     this.buf1, null);
    this.E = this.mediaGraph.connect(this.buf1, null,
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

MediaSession.getter(
  "localRTPPort",
  function getLocalRTPPort() {
    var sp = this.mediaGraph.getNode(this.socketpair, Components.interfaces.zapIUDPSocketPair, true);
    return sp.portA;
  });

MediaSession.getter(
  "localRTCPPort",
  function getLocalRTCPPort() {
    var sp = this.mediaGraph.getNode(this.socketpair, Components.interfaces.zapIUDPSocketPair, true);
    return sp.portB;
  });


MediaSession.fun(
  function shutdown() {
    this.mediaGraph.shutdown();
  });

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [{ className  : "ZAP Media Session",
     cid        : Components.ID("{6ec361e6-eb0d-40ae-a2a5-5bcc942784b6}"),
     contractID : "@mozilla.org/zap/mediasession;1",
     factory    : ComponentUtils.generateFactory(function() { return MediaSession.instantiate(); })
  }]);
