/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.importModule('gre:zapStunResolver.js', null)" -*- */
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

debug("*** loading zapStunResolver\n");

Components.utils.importModule("gre:ComponentUtils.jsm");
Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:ArrayUtils.js");
Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:ObjectUtils.js");
Components.utils.importModule("gre:FunctionUtils.js");
Components.utils.importModule("gre:AsyncUtils.js");

// object to hold component's documentation:
var _doc_ = {};

////////////////////////////////////////////////////////////////////////
// globals

//----------------------------------------------------------------------
// udp sockets

var CLASS_UDP_SOCKET = Components.classes["@mozilla.org/network/udp-socket;1"];
var ITF_UDP_SOCKET = Components.interfaces.nsIUDPSocket;

//----------------------------------------------------------------------
// tcp server socket

var CLASS_SERVER_SOCKET = Components.classes["@mozilla.org/network/server-socket;1"];
var ITF_SERVER_SOCKET = Components.interfaces.nsIServerSocket;

//----------------------------------------------------------------------
// streams

var ITF_ASYNC_INPUT_STREAM = Components.interfaces.nsIAsyncInputStream;

var CLASS_SCRIPT_INPUT_STREAM = Components.classes["@mozilla.org/scriptableinputstream;1"];
var ITF_SCRIPT_INPUT_STREAM = Components.interfaces.nsIScriptableInputStream;

//----------------------------------------------------------------------
// socket transport service

var CLASS_STS = Components.classes["@mozilla.org/network/socket-transport-service;1"];
var ITF_STS = Components.interfaces.nsISocketTransportService;

var _gSTS; // global socket transport service

function getSTS() {
  if (!_gSTS) {
    _gSTS = CLASS_STS.getService(ITF_STS);
  }
  return _gSTS;
}

//----------------------------------------------------------------------
// getDNSService: get the global dns service instance

var getDNSService = makeServiceGetter("@mozilla.org/network/dns-service;1",
                                      Components.interfaces.nsIDNSService);

//----------------------------------------------------------------------
// getNetUtils

var getNetUtils = makeServiceGetter("@mozilla.org/zap/netutils;1",
                                    Components.interfaces.zapINetUtils);


////////////////////////////////////////////////////////////////////////
// zapStunResolver

var zapStunResolver = makeClass("zapStunResolver", SupportsImpl);
zapStunResolver.addInterfaces(Components.interfaces.zapIStunResolver,
                              Components.interfaces.nsIDNSListener,
                              Components.interfaces.nsIUDPReceiver);

// zapIStunResolver::resolveMappedAddress
zapStunResolver.fun(
  function resolveMappedAddress(listener, stunServer) {
    this.listener = listener;
    // XXX parse the server address properly
    var parsed_address = stunServer.split(":");
    var host = parsed_address[0];
    if (parsed_address.length == 2)
      this.defaultPort = parsed_address[1];
    else
      this.defaultPort = 3478;
    // firstly look up the stun server address:
    getDNSService().asyncResolve(host, 0, this, getMainThread());
    // -> onLookupComplete
  });

// nsIDNSListener::onLookupComplete
zapStunResolver.fun(
  function onLookupComplete(aRequest, aRecord, aStatus) {
    var stunServerCandiates = [];
    if (!aRecord) {
      this.failure();
      return;
    }
    // collect the server candidates:
    while (aRecord.hasMore()) {
      stunServerCandiates.push({address: aRecord.getNextAddrAsString(),
                                port: this.defaultPort});
    }
    
    this.stunServerIterator = makeArrayIterator(stunServerCandiates);

    // We now have a list of stun servers that we can try.    
    // Init a UDP socket, stun request object:
    
    this.udpSocket = CLASS_UDP_SOCKET.createInstance().QueryInterface(ITF_UDP_SOCKET);
    var port = 3478;
    while (port < 65536) {
      try {
        this.udpSocket.init(port);
        break;
      }
      catch(e) { ++port; }
    }
    if (port == 65536)
      this.failure();
    
    this.udpSocket.setReceiver(this);
    
    this.stunRequest = getNetUtils().createStunMessage();
    this.stunRequest.messageType = this.stunRequest.BINDING_REQUEST_MESSAGE;
    
    this.tryNextStunServer();
  });

zapStunResolver.fun(
  function tryNextStunServer() {
    if (!this.stunServerIterator.hasMore()) {
      this.failure();
      return;
    }

    this.currentStunServer = this.stunServerIterator.getNext();
    this.stunRequest.initTransactionID();

    // send STUN packet & start timer:
    this.udpSocket.send(this.stunRequest.serialize(),
                        this.currentStunServer.address,
                        this.currentStunServer.port);
    this.resends = 0;
    if (!this.resendTimer)
      this.resendTimer = makeOneShotTimer(this, 100);
    else
      resetOneShotTimer(this.resendTimer, 100);
  });

// nsIUDPReceiver::handleDatagram
zapStunResolver.fun(
  function handleDatagram(socket, data) {

    if (data.address != this.currentStunServer.address ||
        data.port != this.currentStunServer.port) {
      // packet is not from the server we are currently contacting.
      // -> ignore
      this._dump("ignoring packet from unknown source");
      return;
    }
    try {
      var message = getNetUtils().deserializeStunPacket(data.data, {}, {});
    }
    catch(e) {
      // error parsing the STUN response
      // -> ignore
      this._dump("can't parse STUN packet");
      return;
    }
    
    if (message.transactionID != this.stunRequest.transactionID) {
      // not a response to our current query; possibly a
      // retransmission.
      // -> ignore
      this._dump("STUN packet has a different transactionID");
      return;
    }
        
    this.resendTimer.cancel();

    switch (message.messageType) {
      case Components.interfaces.zapIStunMessage.BINDING_RESPONSE_MESSAGE:
        var address, port;
        if (message.hasXORMappedAddressAttrib) {
          address = message.XORMappedAddress;
          port = message.XORMappedAddressPort;
        }
        else if (message.hasMappedAddressAttrib) {
          address = message.mappedAddress;
          port = message.mappedAddressPort;
        }
        else {
          // the response is malformed
          // -> try next server
          this._dump("malformed STUN packet");
          this.tryNextStunServer();
          return;
        }
        this.success(address, port);
        break;
      case Components.interfaces.zapIStunMessage.BINDING_ERROR_RESPONSE_MESSAGE:
          this._dump("received an error response");
      default:
        this.tryNextStunServer();
        break;
    }
  });

// nsITimerCallback::notify
zapStunResolver.fun(
  function notify(timer) {
    // see rfc3489bis 9.3 for information on the retransmission logic
    if (this.resends == 8) {
      // failure to get an answer from this server.
      // -> try next server
      this.tryNextStunServer();
    }
    else {
      // resend the request & reset timer:
      this.udpSocket.send(this.stunRequest.serialize(),
                          this.currentStunServer.address,
                          this.currentStunServer.port);
      ++this.resends;
      resetOneShotTimer(this.resendTimer, Math.min(2*this.resendTimer.delay, 1600));
    }
  });

zapStunResolver.fun(
  function failure() {
    this._dump("failure");
    if (this.listener) {
      this.listener.onAddressResolveComplete("",
                                             Components.results.NS_ERROR_FAILURE);
    }
    this.cleanup();
  });

zapStunResolver.fun(
  function success(addr, port) {
    this._dump("success:"+addr+":"+port);
    if (this.listener) {
      this.listener.onAddressResolveComplete(addr, Components.results.NS_OK);
    }
    this.cleanup();
  });

zapStunResolver.fun(
  function cleanup() {
    if (this.udpSocket)
      this.udpSocket.close();
    if (this.resendTimer) {
      this.resendTimer.cancel();
      delete this.resendTimer;
    }
    delete this.listener;
    delete this.udpSocket;
    delete this.stunRequest;
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [
   { className  : "ZAP STUN Resolver",
     cid        : Components.ID("{3e2e9ff8-89b8-442a-a190-8cecabb93c78}"),
     contractID : "@mozilla.org/zap/stun-resolver;1",
     factory    : ComponentUtils.generateFactory(function() { return zapStunResolver.instantiate(); })
   }
  ]);
