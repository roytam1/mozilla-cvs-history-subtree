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

EXPORTED_SYMBOLS = ["produceICECandidates"];

// object to hold component's documentation:
var _doc_ = {};

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

////////////////////////////////////////////////////////////////////////
// zapICETransport
//
// Abstraction of a transport address over which we can send or receive
// media streams.
// Behaviour is ammended by subclasses.
// UDP is hardcoded atm.

var zapICETransport = makeClass("zapICETransport", ErrorReporter);

// ip address. 
// zapICETransport.obj("address", null);

// port
// zapICETransport.obj("port", null);

// protocol
zapICETransport.obj("protocol", "udp");

// free resources attached to this transport:
zapICETransport.fun(
  function destroy() {
  });

////////////////////////////////////////////////////////////////////////
// zapICELocalUDPTransport
//
// A local UDP transport

var zapICELocalUDPTransport = makeClass("zapICELocalUDPTransport",
                                        zapICETransport);

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
  });

zapICECandidate.fun(
  function destroy() {
    for (var i=0,l=this.components.length; i<l; ++i)
      this.components[i].destroy();
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
zapICECandidate.fun(
  function getCandidateID() {
    if (!this._candidateID) {
      // XXX doesn't guarantee uniqueness of local candidate ids!
      var r = getRandom32();
      // XXX could use base64, also 24 bits of randomness sufficient
      this._candidateID = r.toString(36);
    }
    return this._candidateID;
  });

// relative priority of this candidate
zapICECandidate.obj("q", "0.5");
  

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
  
  return zapICECandidate.instantiate({components: components});
}

////////////////////////////////////////////////////////////////////////
// zapICEDerivedCandidate
//
// adds a field 'localCandidate' to zapICECandidate

var zapICEDerivedCandidate = makeClass("zapICEDerivedCandidate",
                                       zapICECandidate);

// ctor expects an arg { localCandidate }
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
    var req = comp.stunClientCtl.sendBindingRequest(this, serv, null, null);
    this.activeRequests[Components.utils.getObjectId(req)] = [lc, this.currentComponent, serv];
    ++this.activeRequestCount;

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
    var rid = Components.utils.getObjectId(request);
    var rinfo = this.activeRequests[rid];
    if (!rinfo) {
      // XXX not quite sure why, but we hit this when a stun client fails to
      // contact the server. In this case the request arg will have a different
      // obj-id than that originally used as ket in activeRequests.
      // Possibly this is some interaction between the autoproxy/timer
      // callback/xpconnect wrapping code.
      dump("no rinfo for request "+request+"\n");
      return;
    }
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
      dump("Obtained refl addr "+reflAddr+":"+reflPort+
           " from STUN server "+rinfo[2]+"\n");
      // insert it into our partial candidate map:
      var partialCandidateID = Components.utils.getObjectId(rinfo[0]) +
        ":" + rinfo[2];
      var partialCandidateInfo = this.partialCandidates[partialCandidateID];
      if (!partialCandidateInfo) {
        partialCandidateInfo = { count : 0,
                                 components : new Array(rinfo[0].componentCount) };
        this.partialCandidates[partialCandidateID] = partialCandidateInfo;
      }
      var component = zapICETransport.instantiate();
      component.address = reflAddr;
      component.port = reflPort;
      partialCandidateInfo.components[rinfo[1]] = component;

      // check if we have a full candidate:
      if (++partialCandidateInfo.count == rinfo[0].componentCount) {
        // yup. -> make a new candidate:
        dump("we have a full candidate\n");
        this.candidateProduced(zapICEDerivedCandidate.instantiate(
                                 { components: partialCandidateInfo.components,
                                   localCandidate: rinfo[0] }));
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
      if (Components.utils.getObjectId(cand.localCandidate) !=
          Components.utils.getObjectId(candidates[i].localCandidate))
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
// ICECandidates
//

var ICECandidates = makeClass("ICECandidates", ErrorReporter);

ICECandidates.appendCtor(
  function ICECandidatesCtor(args) {
    this.password = args.password;
    this.candidates = args.candidates;
    this.activeCandidate = args.activeCandidate;
  });


// continuation done() will be called with an ICECandidates object
function produceICECandidates(done, componentCount, udpPortbase,
                              maxDuration, paceDuration,
                              mediaGraph, stunServers)
{

  // generate local candidates first:
  var locals = gatherLocalICECandidates(componentCount, udpPortbase, mediaGraph);
  
  // asynchronously produce derived candidates and proceed with
  // continuation done():
  function productionDone(cl) {
    var pw = generateICEPassword();
    done(ICECandidates.instantiate({ password: pw, candidates: cl.concat(locals) , activeCandidate: cl[0]}));
  }

  produceDerivedCandidates(productionDone, maxDuration, paceDuration,
                           mediaGraph, locals, stunServers);
}
