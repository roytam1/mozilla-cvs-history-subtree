// -*- moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:zapICE.js', null)" -*-
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


debug("*** loading zapICE\n");

Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:ArrayUtils.js");
Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:ObjectUtils.js");
Components.utils.importModule("gre:FunctionUtils.js");

// name our global object:
function toString() { return "[zapICE.js]"; }

EXPORTED_SYMBOLS = ["produceICECandidates",
                    "encodeICECandidates",
                    "extractICECandidates",
                    "createCandidatePairList"];

var PB = makePropertyBag;

var getNetUtils = makeServiceGetter("@mozilla.org/zap/netutils;1",
                                    Components.interfaces.zapINetUtils);

function getRandom32() {
  // Obtain a 32bit random number:
  // XXX we should use a better random number generator
  return Math.floor(Math.random()*4294967296);
}

function generateICEPassword() {
  // XXX could use base64
  var password = "";
  password += getRandom32().toString(36);
  password += getRandom32().toString(36);
  password += getRandom32().toString(36);
  password += getRandom32().toString(36);

  return password;
}

function generateCandidateId() {
  // XXX could use base64, also 24 bits of randomness sufficient
  var r = getRandom32();
  return r.toString(36);
}

////////////////////////////////////////////////////////////////////////
// zapICETransport
//
// Abstraction of a transport address over which we can send or receive
// media streams.
// Behaviour is ammended by subclasses.

var zapICETransport = makeClass("zapICETransport", ErrorReporter);

// ip address. 
// zapICETransport.obj("address", null);

// port
// zapICETransport.obj("port", null);

// protocol
// zapICETransport.obj("protocol", null);

// free resources attached to this transport:
zapICETransport.fun(
  function destroy() {
  });

zapICETransport.spec(
  function toString() {
    if (!this.address) return this._zapICETransport_toString();
    var rv = "["+this._class_.toString()+" "+this.address+":"+this.port+"]";
    return rv;
  });

////////////////////////////////////////////////////////////////////////
// zapICENativeUDPTransport
//
// UDP transport belonging to us (as opposed to a remote transport offered
// by our peer).
// Behaviour is ammended by subclasses.

var zapICENativeUDPTransport = makeClass("zapICENativeUDPTransport",
                                         zapICETransport);

// protocol
zapICENativeUDPTransport.obj("protocol", "UDP");

// id of multiplexer in the media graph
zapICENativeUDPTransport.fun(
  function getLocalMuxId() {
    this._error("to be implemented by subclass");
  });

// id of demultiplexer in the media graph
zapICENativeUDPTransport.fun(
  function getLocalDemuxId() {
    this._error("to be implemented by subclass");
  });


////////////////////////////////////////////////////////////////////////
// zapICELocalUDPTransport
//
// A native transport local to the machine we're running on

var zapICELocalUDPTransport = makeClass("zapICELocalUDPTransport",
                                        zapICENativeUDPTransport);

// ctor expects an arg { address, portbase, mediaGraph }
// binds to a STUN-able UDP socket at 'address' and the first free port
// with a port number greater than or equal to 'portbase'
zapICELocalUDPTransport.appendCtor(
  function zapICELocalUDPTransportCtor(args) {
    this.address = args.address;
    this.mediaGraph = args.mediaGraph;

    var g = this.mediaGraph;
    
    // XXX we currently bind to 0.0.0.0 instead of address
    this.socket = g.addNode("udp-socket", PB({$portbase: args.portbase}));
    this.socketCtl = g.getNode(this.socket,
                               Components.interfaces.zapIUDPSocket, true);
    this.demux = g.addNode("stun-demuxer", null);
    this.mux = g.addNode("stream-merger", null);
    this.stunClient = g.addNode("stun-client", null);
    this.stunClientCtl = g.getNode(this.stunClient,
                                   Components.interfaces.zapIStunClient,
                                   true);
    this.stunServer = g.addNode("stun-server",
                                PB({$source_addr: args.address,
                                    $source_port: this.socketCtl.port}));
    this.stunServerCtl = g.getNode(this.stunServer,
                                   Components.interfaces.zapIStunServer,
                                   true);

    g.connect(this.socket, null, this.demux, null);
    g.connect(this.demux, PB({$name:"stun-res"}), this.stunClient, null);
    g.connect(this.demux, PB({$name:"stun-req"}), this.stunServer, null);

    g.connect(this.stunClient, null, this.mux, null);
    g.connect(this.stunServer, null, this.mux, null);
    g.connect(this.mux, null, this.socket, null);
  });

zapICELocalUDPTransport.fun(
  function getLocalMuxId() {
    return this.mux;
  });

zapICELocalUDPTransport.fun(
  function getLocalDemuxId() {
    return this.demux;
  });

zapICELocalUDPTransport.fun(
  function destroy() {
    var g = this.mediaGraph;
    g.removeNode(this.socket);
    g.removeNode(this.demux);
    g.removeNode(this.mux);
    g.removeNode(this.stunClient);
    g.removeNode(this.stunServer);
    
    delete this.socketCtl;
    delete this.stunClientCtl;
    delete this.stunServerCtl;
    delete this.mediaGraph;
  });

// zapICELocalUDPTransport.getter("socket", ...) implicit in ctor

zapICELocalUDPTransport.getter(
  "port",
  function get_port() {
    return this.socketCtl.port;
  });

////////////////////////////////////////////////////////////////////////
// zapICEDerivedUDPTransport
//
// A native derived UDP transport

var zapICEDerivedUDPTransport = makeClass("zapICEDerivedUDPTransport",
                                          zapICENativeUDPTransport);

zapICEDerivedUDPTransport.appendCtor(
  function zapICEDerivedUDPTransportCtor(args) {
    this.address = args.address;
    this.port = args.port;
    this.localTransport = args.localTransport;
  });

zapICEDerivedUDPTransport.fun(
  function getLocalMuxId() {
    return this.localTransport.getLocalMuxId();
  });

zapICEDerivedUDPTransport.fun(
  function getLocalDemuxId() {
    return this.localTransport.getLocalDemuxId();
  });

////////////////////////////////////////////////////////////////////////
// zapICEPeerUDPTransport
//
// UDP transport offered by our peer

var zapICEPeerUDPTransport = makeClass("zapICEPeerUDPTransport",
                                       zapICETransport);

zapICEPeerUDPTransport.appendCtor(
  function zapICEPeerUDPTransportCtor(args) {
    this.address = args.address;
    this.port = args.port;
  });

// protocol
zapICEPeerUDPTransport.obj("protocol", "UDP");


////////////////////////////////////////////////////////////////////////
// zapICECandidate
// holds an array of ICE transports ('components') that act together as
// a unit.
//

var zapICECandidate = makeClass("zapICECandidate",
                                ErrorReporter);

// ctor expects an arg { components }
zapICECandidate.appendCtor(
  function zapICECandidateCtor(args) {
    this.components = args.components;
    this.id = args.id;
    this.q = args.q;
  });

zapICECandidate.fun(
  function destroy() {
    for (var i=0,l=this.components.length; i<l; ++i)
      this.components[i].destroy();
  });

zapICECandidate.spec(
  function toString() {
    if (!this.components) return this._zapICECandidate_toString();

    var rv = "["+this._class_.toString()+" "+this.id+" "+this.q+" ( ";
    for (var i=0,l=this.components.length; i<l; ++i)
      rv += this.components[i]+" ";
    rv += " )";
    return rv;
  });

zapICECandidate.getter(
  "componentCount",
  function get_componentCount() {
    return this.components.length;
  });

zapICECandidate.fun(
  function componentAt(n) {
    return this.components[n];
  });

// unique candidate id
// zapICECandidate.obj("id", null);

// relative priority of this candidate
// zapICECandidate.obj("q", null);
  

////////////////////////////////////////////////////////////////////////
// local udp candidates

function makeICELocalUDPCandidate(address, portbase,
                                  componentCount, mediaGraph)
{
  var components = [];
  for (var i=0; i<componentCount; ++i)
    components.push(zapICELocalUDPTransport.instantiate(
                      {address:    address,
                       portbase:   portbase++,
                       mediaGraph: mediaGraph}));
  
  return zapICECandidate.instantiate(
    {
      components: components,
      id: generateCandidateId(),
      q: "0.5"
    });
}

////////////////////////////////////////////////////////////////////////
// zapICEDerivedCandidate
//
// adds a field 'localCandidate' to zapICECandidate

var zapICEDerivedCandidate = makeClass("zapICEDerivedCandidate",
                                       zapICECandidate);

// ctor expects an arg { localCandidate } in addition to args of base
// ctor
zapICEDerivedCandidate.appendCtor(
  function zapICEDerivedCandidateCtor(args) {
    this.localCandidate = args.localCandidate;
  });


////////////////////////////////////////////////////////////////////////
// zapICEStunReflexiveCandidateFactory
//

var zapICEStunReflexiveCandidateFactory = makeClass(
  "zapICEStunReflexiveCandidateFactory", Scheduler, SupportsImpl);

zapICEStunReflexiveCandidateFactory.addInterfaces(
  Components.interfaces.zapIStunClientListener);

// 'done' will be called when the production processes is finished (or
// has been terminated)
// 'candidateProduced(candidate)' will be called whenever a candidate has been
// produced
zapICEStunReflexiveCandidateFactory.fun(
  function produceCandidates(done, candidateProduced,
                             maxDuration, paceDuration,
                             mediaGraph, localCandidates, stunServers) {
    this._assert(!this.Terminated, "can only be used once!");
    this.done = done;
    this.candidateProduced = candidateProduced;
    
    // map of (obid(localCandidate) : stun server) ->
    //   { count : components gathered for this candidate so far,
    //     components : [{address,port}...] }
    this.partialCandidates = {};
    
    if (!localCandidates.length || !stunServers.length) {
      this.schedule(this.stop, 0);
      return;
    }

    this.paceDuration = paceDuration;
    this.mediaGraph = mediaGraph;
    this.localCandidates = localCandidates;
    this.stunServers = stunServers;

    // indexes for current component, candidate, server:
    this.currentComponent = 0;
    this.currentCandidate = 0;
    this.currentServer = 0;
    
    this.lastRequestSent = false;

    // activeRequests is a map of
    // stun request -> [localCandidate, component, stun server]
    this.activeRequests = {};
    
    this.activeRequestCount = 0;
    
    this.schedule(this.stop, maxDuration);
    this.next();
  });

zapICEStunReflexiveCandidateFactory.fun(
  function stop() {
    this.Terminated = true;
    this.done();
  });

zapICEStunReflexiveCandidateFactory.fun(
  function next() {
    var lc = this.localCandidates[this.currentCandidate];
    var comp = lc.componentAt(this.currentComponent);
    var serv = this.stunServers[this.currentServer];
    try {
      var req = comp.stunClientCtl.sendBindingRequest(this, serv, null, null);
      this.activeRequests[Components.utils.getObjectId(req, true)] =
        [lc, this.currentComponent, serv, req]; // It is important here to keep
                                                // 'req' in our activeRequests hash to
                                                // persist it through GC.
                                                // Otherwise we might get a different
                                                // (proxied,wrapped) req pointer
                                                // passed into bindingRequestComplete()
                                                // and getObjectId() will return a
                                                // different value to the one used as
                                                // key in activeRequests.

      ++this.activeRequestCount;
    }
    catch(e) {
      // The binding request has failed immediatly; probably because
      // the stun server is invalid. Let's continue as normal.
    }
    
    // we iterate through components first, then candidates, then servers
    if (++this.currentComponent >= lc.componentCount) {
      this.currentComponent = 0;
      if (++this.currentCandidate >= this.localCandidates.length) {
        this.currentCandidate = 0;
        if (++this.currentServer >= this.stunServers.length) {
          this.lastRequestSent = true;
          return; // don't reschedule. The last binding response or
                  // the maxDuration timer will terminate us now
        }
      }
    }
    this.schedule(this.next, this.paceDuration);
  });

//----------------------------------------------------------------------
// zapIStunClientListener methods:

zapICEStunReflexiveCandidateFactory.fun(
  function bindingRequestComplete(finalMessage, request) {
    var rid = Components.utils.getObjectId(request, true);
    var rinfo = this.activeRequests[rid];
    this._assert(rinfo, "uh oh, callback couldn't be matched");

    delete this.activeRequests[rid];
    --this.activeRequestCount;

    if (finalMessage &&
        finalMessage.messageType ==
        Components.interfaces.zapIStunMessage.BINDING_RESPONSE_MESSAGE &&
        (finalMessage.hasXORMappedAddressAttrib ||
         finalMessage.hasMappedAddressAttrib)) {
      var reflAddr, reflPort;
      if (finalMessage.hasXORMappedAddressAttrib) {
        reflAddr = finalMessage.XORMappedAddress;
        reflPort = finalMessage.XORMappedAddressPort;
      }
      else {
        reflAddr = finalMessage.mappedAddress;
        reflPort = finalMessage.mappedAddressPort;
      }
      
      // we have a new STUN server reflexive address:
      this._dump("Obtained refl addr "+reflAddr+":"+reflPort+
                 " from STUN server "+rinfo[2]+"\n");
      // insert it into our partial candidate map:
      var partialCandidateID = Components.utils.getObjectId(rinfo[0], true) +
        ":" + rinfo[2];
      var partialCandidateInfo = this.partialCandidates[partialCandidateID];
      if (!partialCandidateInfo) {
        partialCandidateInfo = { count : 0,
                                 components : new Array(rinfo[0].componentCount) };
        this.partialCandidates[partialCandidateID] = partialCandidateInfo;
      }
      var component = zapICEDerivedUDPTransport.instantiate(
        { address: reflAddr,
          port:  reflPort,
          localTransport: rinfo[0].componentAt(rinfo[1])
        });
      partialCandidateInfo.components[rinfo[1]] = component;

      // check if we have a full candidate:
      if (++partialCandidateInfo.count == rinfo[0].componentCount) {
        // yup. -> make a new candidate:
        this._dump("we have a full candidate\n");
        this.candidateProduced(zapICEDerivedCandidate.instantiate(
                                 { components: partialCandidateInfo.components,
                                   localCandidate: rinfo[0],
                                   q: "0.5",
                                   id: generateCandidateId()}));
        delete this.partialCandidates[partialCandidateID];
      }
    }
    else {
      this._dump("Error communicating with STUN server "+rinfo[2]+
                 " from local udp transport "+
                 rinfo[0].componentAt(rinfo[1]).address+":"+
                 rinfo[0].componentAt(rinfo[1]).port);      
    }
    
    // clear up after last request send & last response received:
    if (this.lastRequestSent && this.activeRequestCount == 0)
      this.stop();
  });


////////////////////////////////////////////////////////////////////////
// helpers

// returns an array of local candidates in mediaGraph each having
// 'componentCount' components
function gatherLocalICECandidates(componentCount, udpPortbase, mediaGraph)
{
  // XXX sockets currently bind to 0.0.0.0 and we have no reliable way
  // of enumerating all interfaces, so this will currently just return
  // ONE local candidate, bound to ALL interfaces.
  // ICE should still function normally.
  return [
    makeICELocalUDPCandidate(
      getNetUtils().getPrimaryHostAddress(),
      udpPortbase,
      componentCount,
      mediaGraph)
    ];
}

// produce an array of (unique) derived candidates for an array of
// (unique) local candidates. 
// done() will be called with the array of derived candidates when the
// production process is complete:
function produceDerivedCandidates(done, maxDuration, paceDuration,
                                  mediaGraph, localCandidates,
                                  stunServers) {
  var candidates = [];
  
  function candidateProduced(cand) {
    // make sure the candidate is unique:
    for (var i=0,l=candidates.length; i<l; ++i) {
      if (Components.utils.getObjectId(cand.localCandidate, true) !=
          Components.utils.getObjectId(candidates[i].localCandidate, true))
        continue;
      if (cand.componentCount != candidates[i].componentCount) continue;
      for (var j=0,m=cand.componentCount; j<m; ++j) {
        var comp1 = cand.componentAt(j);
        var comp2 = candidates[i].componentAt(j);
        if (comp1.address != comp2.address) break;
        if (comp1.port != comp2.port) break;
        if (comp1.protocol != comp2.protocol) break;
      }
      dump("pruning duplicate candidate\n");
      if (j==m) return; // identical candidate already in array
    }
    candidates.push(cand);
  }

  var factoriesRunning = 0;
  function productionDone() {
    if (--factoriesRunning == 0)
      done(candidates);
  }

  ++factoriesRunning;
  zapICEStunReflexiveCandidateFactory.instantiate().produceCandidates(
    productionDone, candidateProduced, maxDuration, paceDuration,
    mediaGraph, localCandidates, stunServers);
  
  // XXX this is the place to hook in other candidate factories.  
}

////////////////////////////////////////////////////////////////////////
// zapICECandidates
//

var zapICECandidates = makeClass("zapICECandidates", ErrorReporter);

zapICECandidates.appendCtor(
  function zapICECandidatesCtor(args) {
    this.password = args.password;
    this.candidates = args.candidates;
    this.activeCandidate = args.activeCandidate;
  });

zapICECandidates.fun(
  function destroy() {
    for (var i=0,l=this.candidates.length; i<l; ++i) {
      this.candidates[i].destroy();
    }
    delete this.candidates;
    delete this.activeCandidate;
  });

// continuation done() will be called with an zapICECandidates object
function produceICECandidates(done, componentCount, udpPortbase,
                              maxDuration, paceDuration,
                              mediaGraph, stunServers, bestGuessAddress)
{
  // generate local candidates first:
  var cands = gatherLocalICECandidates(componentCount, udpPortbase, mediaGraph);
  // asynchronously produce derived candidates and proceed with
  // continuation done():
  function productionDone(cl) {
    var pw = generateICEPassword();
    cands = cands.concat(cl);

    // Active candidate selection:
    // We return the first candidate that matches our 'bestGuessAddress', or
    // the first candidate if non matches:
    var ac = findif(function(c) { return c.componentAt(0).address == bestGuessAddress; },
                    cands);
    if (!ac)
      ac = cands[0];
    
    done(zapICECandidates.instantiate({ password: pw,
                                        candidates: cands,
                                        activeCandidate: ac}));
  }

  produceDerivedCandidates(productionDone, maxDuration, paceDuration,
                           mediaGraph, cands, stunServers);
}

// encode zapICECandidates object into an sdp media offer/answer
function encodeICECandidates(candidates, session, media) {
  session.appendAttrib("ice-pwd:" + candidates.password);

  // Encode the candidates as media-level attributes:
  for (var i=0, l=candidates.candidates.length; i<l; ++i) {
    var cand = candidates.candidates[i];
    for (var j=0, m=cand.componentCount; j<m; ++j) {
      var comp = cand.componentAt(j);
      media.appendAdditionalAttrib("candidate:"+
                                   cand.id+" "+
                                   (j+1)+" "+
                                   comp.protocol+" "+
                                   cand.q+" "+
                                   comp.address+" "+
                                   comp.port);
    }
  }
}

// extract ICECandidates object from sdp media offer/answer (or null if the
// sdp doesn't contain ICE-specific fields):
function extractICECandidates(session, media) {
  // firstly we expect an ice-pwd attrib at the session level:
  var sessionAttribs = session.getAttribs({});
  var pwd = findif(/^ice-pwd:/, sessionAttribs);
  if (!pwd ||
      !(pwd = /ice-pwd:(.*)/(pwd)[1])) {
    dump("No ICE password\n");
    return null; // no password attrib -> no ICE
  }
  dump("EXTRACTED ICE PASSWORD = "+pwd+"\n");

  // secondly, we need some candidates:
  // XXX currently hardcoded for rtp/avp
  var comps = {}; // candidate components indexed by candidate id
  var mediaAttribs = media.getAdditionalAttribs({});
  if (!mediaAttribs) {
    dump("No ICE candidates\n");
    return null; // no candidates -> no ICE
  }
  mediaAttribs.forEach(
    function(a) {
      var c = /^candidate:([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*) ([^ ]*)$/(a);
      if (!c) return;
      var entry;
      if (!(entry = comps[c[1]])) {
        entry = comps[c[1]] = [];
      }
      entry.push({componentID:c[2], transport:c[3], address:c[5], port:c[6], q:c[4]});
    });

  // Construct proper candidates from the components hash:
  var cands = [];
  for (var c in comps) {
    dump("ICE PROCESSING CANDIDATE "+c+"\n");
    var candcomps = comps[c];
    var address = candcomps[0].address;
    var q = candcomps[0].q;
    // sort in ascending component id order:
    candcomps.sort(function(a,b) { return a.componentID - b.componentID; });
    var transports = [];
    for (var i=0,l=candcomps.length; i<l; ++i) {
      // sanity checks:
      if (candcomps[i].componentID != i+1 ||
          candcomps[i].address != address ||
          candcomps[i].q != q ||
          candcomps[i].transport != "UDP") {
        dump("ICE CANDIDATE PARSE ERROR 2\n");
        return null;
      }
      transports.push(zapICEPeerUDPTransport.instantiate(
                        {
                          address : candcomps[i].address,
                          port : candcomps[i].port
                        }));
    }
    cands.push(zapICECandidate.instantiate(
      { components: transports,
        id : c,
        q : q
      }));
  }
  dump("WE'VE GOT "+cands.length+" ICE CANDIDATES\n");
  return zapICECandidates.instantiate({ password : pwd,
                                        candidates: cands,
                                        activeCandidate: null});
}

////////////////////////////////////////////////////////////////////////
// zapICECandidatePair

var zapICECandidatePair = makeClass("zapICECandidatePair", ErrorReporter);

zapICECandidatePair.appendCtor(
  function zapICECandidatePairCtor(args) {
    this.local = args.local;
    this.remote = args.remote;
  });

zapICECandidatePair.getter(
  "id",
  function get_id() {
    //XXX
  });

////////////////////////////////////////////////////////////////////////
// zapICECandidatePairList

var zapICECandidatePairList = makeClass("zapICECandidatePairList", ErrorReporter);

zapICECandidatePairList.appendCtor(
  function zapICECandidatePairListCtor(args) {
    this.candidatePairs = args.candidatePairs;
  });


// forms an ICECandidatePairList from two zapICECandidates objects:
function createCandidatePairList(localCandidates, remoteCandidates)
{
  var pairs = [];
  for (var l=0,ll=localCandidates.candidates.length; l<ll; ++l) {
    for (var r=0,rl=remoteCandidates.candidates.length; r<rl; ++r) {
      var lc = localCandidates.candidates[l];
      var rc = remoteCandidates.candidates[r];
      //XXX we're assuming for the time being that the transport protocols match
      pairs.push(zapICECandidatePair.instantiate(
                  {
                    local :  lc,
                    remote : rc
                  }));
    }
  }
  return zapICECandidatePairList.instantiate(
    {
      candidatePairs : pairs
    });
}
