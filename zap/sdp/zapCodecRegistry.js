// -*- moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:zapCodecRegistry.js', null)" -*-
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
 * Portions created by the Initial Developer are Copyright (C) 2006
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


debug("*** loading zapCodecRegistry.js\n");

Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:ObjectUtils.js");

EXPORTED_SYMBOLS = [ "createCodecInstance" ];

// name our global object:
function toString() { return "[zapCodecRegistry.js]"; }

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
// gCodecRegistry: global codec registry

var gCodecRegistry = {};

function createCodecInstance(name) {
  var codec = hashget(gCodecRegistry, name);
  if (!codec) return null;
  return codec.instantiate();
}

//----------------------------------------------------------------------
// Speex

var SpeexCodec = makeClass("SpeexCodec");
hashset(gCodecRegistry, "speex", SpeexCodec);

SpeexCodec.obj("localPayloadType", "97");

SpeexCodec.getter(
  "sdpformat",
  function get_sdpformat() {
    if (!this._sdpformat) {
      this._sdpformat = gSdpService.createRtpAvpMediaFormat();
      this._sdpformat.payloadType = this.localPayloadType;
      this._sdpformat.encodingName = "speex";
      this._sdpformat.clockRate = "8000";
    }
    return this._sdpformat;
  });

SpeexCodec.fun(
  function connect(session) {
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
  });

SpeexCodec.fun(
  function disconnect(session) {
    if (!this.remotePayloadType) return;
    
    session.mediaGraph.removeNode(this.enc);
    session.mediaGraph.removeNode(this.dec);
    session.mediaGraph.removeNode(this.speex2rtp);
    session.mediaGraph.removeNode(this.rtp2speex);    
  });

//----------------------------------------------------------------------
// PCMU

var PCMUCodec = makeClass("PCMUCodec");
hashset(gCodecRegistry, "PCMU", PCMUCodec);

PCMUCodec.obj("localPayloadType", "0");

PCMUCodec.getter(
  "sdpformat",
  function get_sdpformat() {
    if (!this._sdpformat) {
      this._sdpformat = gSdpService.createRtpAvpMediaFormat();
      this._sdpformat.payloadType = this.localPayloadType;
      this._sdpformat.encodingName = "PCMU";
      this._sdpformat.clockRate = "8000";
    }
    return this._sdpformat;
  });

PCMUCodec.fun(
  function connect(session) {
    if (!this.remotePayloadType) return;
    this.enc = session.mediaGraph.addNode("g711-encoder", PB({$type:"audio/pcmu"}));
    this.dec = session.mediaGraph.addNode("g711-decoder", null);
    this.g7112rtp = session.mediaGraph.addNode("g711-rtp-packetizer",
                                               PB({$payload_type:this.remotePayloadType}));
    this.rtp2g711 = session.mediaGraph.addNode("g711-rtp-depacketizer",
                                               PB({$type:"audio/pcmu"}));
    
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
  });

PCMUCodec.fun(
  function disconnect(session) {
    if (!this.remotePayloadType) return;
    
    session.mediaGraph.removeNode(this.enc);
    session.mediaGraph.removeNode(this.dec);
    session.mediaGraph.removeNode(this.g7112rtp);
    session.mediaGraph.removeNode(this.rtp2g711);    
  });

//----------------------------------------------------------------------
// PCMA

var PCMACodec = makeClass("PCMACodec");
hashset(gCodecRegistry, "PCMA", PCMACodec);

PCMACodec.obj("localPayloadType", "8");

PCMACodec.getter(
  "sdpformat",
  function get_sdpformat() {
    if (!this._sdpformat) {
      this._sdpformat = gSdpService.createRtpAvpMediaFormat();
      this._sdpformat.payloadType = this.localPayloadType;
      this._sdpformat.encodingName = "PCMA";
      this._sdpformat.clockRate = "8000";
    }
    return this._sdpformat;
  });

PCMACodec.fun(
  function connect(session) {
    if (!this.remotePayloadType) return;
    this.enc = session.mediaGraph.addNode("g711-encoder", PB({$type:"audio/pcma"}));
    this.dec = session.mediaGraph.addNode("g711-decoder", null);
    this.g7112rtp = session.mediaGraph.addNode("g711-rtp-packetizer",
                                               PB({$payload_type:this.remotePayloadType}));
    this.rtp2g711 = session.mediaGraph.addNode("g711-rtp-depacketizer",
                                               PB({$type:"audio/pcma"}));
    
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
  });

PCMACodec.fun(
  function disconnect(session) {
    if (!this.remotePayloadType) return;
    
    session.mediaGraph.removeNode(this.enc);
    session.mediaGraph.removeNode(this.dec);
    session.mediaGraph.removeNode(this.g7112rtp);
    session.mediaGraph.removeNode(this.rtp2g711);    
  });

//----------------------------------------------------------------------
// telephone-event

var TEventCodec = makeClass("TEventCodec");
hashset(gCodecRegistry, "telephone-event", TEventCodec);

TEventCodec.obj("localPayloadType", "101");

TEventCodec.getter(
  "sdpformat",
  function get_sdpformat() {
    if (!this._sdpformat) {
      this._sdpformat = gSdpService.createRtpAvpMediaFormat();
      this._sdpformat.payloadType = this.localPayloadType;
      this._sdpformat.encodingName = "telephone-event";
      this._sdpformat.clockRate = "8000";
      this._sdpformat.fmtParameters = "0-15";
    }
    return this._sdpformat;
  });

TEventCodec.fun(
  function connect(session) {
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
  });

TEventCodec.fun(
  function disconnect(session) {
    if (!this.remotePayloadType) return;
    
    session.mediaGraph.removeNode(this.sync);
    session.mediaGraph.removeNode(this.tevent2rtp);      
  });

