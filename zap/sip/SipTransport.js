/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.utils.import('resource://gre/components/SipTransport.js', null)" -*- */
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

debug("*** loading SipTransport\n");

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/SipUtils.jsm");
Components.utils.import("resource://gre/components/AsyncUtils.jsm");

debug("*** SipTransport 1\n");

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
var ITF_SCRIPT_INPUT_STREAM = Components.interfaces.nsIScriptableInputStreamEx;

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
// Helpers:

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP TRANSPORT", level, mes);
}

debug("*** SipTransport 2\n");

////////////////////////////////////////////////////////////////////////
// SipTCPConnection

var SipTCPConnection = makeClass("SipTCPConnection", SupportsImpl);
SipTCPConnection.addInterfaces(Components.interfaces.nsIInputStreamCallback,
                               Components.interfaces.zapISipConnection);

SipTCPConnection.fun(
  function init(socket, transceiver) {
    this._readBuffer = "";
    this._pendingMessage = null;
    this._pendingMessageBytes = null;
    this._pendingBytes = 0;
    
    this._transceiver = transceiver;
    this._socket = socket;
    this._outputStream = socket.openOutputStream(0, 0, 0);
    this._inputStream = socket.openInputStream(0, 0, 0);
    this._inputStream.QueryInterface(ITF_ASYNC_INPUT_STREAM);
    this._scriptableInputStream = CLASS_SCRIPT_INPUT_STREAM.createInstance();
    this._scriptableInputStream.QueryInterface(ITF_SCRIPT_INPUT_STREAM);
    this._scriptableInputStream.init(this._inputStream);
    this._inputStream.asyncWait(this, 0, 0, getMainThread());
  });

//----------------------------------------------------------------------
// nsIInputStreamCallback implementation

SipTCPConnection.fun(
  function onInputStreamReady(stream) {
    try {
      var oldLength = this._readBuffer.length;
      this._readBuffer += this._scriptableInputStream.readEx(this._scriptableInputStream.available());
      this._dump("Data received from "+this._socket.host+":"+this._socket.port);
      this._processBuffer(oldLength);
      this._inputStream.asyncWait(this, 0, 0, getMainThread());
    }
    catch (e) {
      this._dump("stream closed or other error: "+e);
      this._transceiver._connectionClosed(this.protocol, this.host, this.port);      
    }
  });

//----------------------------------------------------------------------
// zapISipConnection implementation

SipTCPConnection.obj("protocol", "TCP");

SipTCPConnection.getter(
  "host",
  function get_host() {
    return this._socket.host;
  });

SipTCPConnection.getter(
  "port",
  function get_port() {
    return this._socket.port;
  });

SipTCPConnection.fun(
  function isAlive() {
    return this._socket.isAlive();
  });

SipTCPConnection.gettersetter(
  "timeout",
  function get_timeout() {
    return this._socket.getTimeout(Components.interfaces.nsISocketTransport.TIMEOUT_READ_WRITE);
  },
  function set_timeout(seconds) {
    this._socket.setTimeout(Components.interfaces.nsISocketTransport.TIMEOUT_READ_WRITE,
                            seconds);
  });

SipTCPConnection.fun(
  function send(data) {
    this._outputStream.write(data, data.length);
    this._outputStream.flush();
  });

SipTCPConnection.fun(
  function close() {
    this._socket.close(Components.results.NS_OK);
    // Our nsIInputStreamCallback will do the rest
  });

//----------------------------------------------------------------------
// implementation helpers

SipTCPConnection.fun(
  function _processBuffer(start) {
    while (this._readBuffer.length) {
      if (!this._pendingMessage) {
        if (start == 0) {
          // remove leading whitespace:
          this._readBuffer = /^\s*([^]*)$/(this._readBuffer)[1];
        }
        // check if we have a complete header by looking for CRLFCRLF
        // in new data:
        // we need to start checking 3 bytes *before* start because
        // part of the CRLFRCLF sequence might be be in the old buffer
        // data:
        start -= 3;
        if (start < 0) start = 0;
        
        var match = /\r\n\r\n/(this._readBuffer.substring(start));
        if (!match) {
          this._dump("incomplete header. buffer size="+this._readBuffer.length);
          break; // no complete header -> bail out of loop
        }
        
        // create a new message from the header data (body will be
        // added later):
        var endp = start + match.index + 4;
        try {
          var messageBytes = this._readBuffer.substring(0, endp);
          var message = gSyntaxFactory.deserializeMessage(messageBytes);
          var l = 0;
          var cl = message.getSingleHeader("Content-Length").QueryInterface(Components.interfaces.zapISipContentLengthHeader);
          if (cl)
            l = cl.contentLength;
          if (l>0) {
            // wait for body octets:
            this._pendingMessageBytes = messageBytes;
            this._pendingMessage = message;
            this._pendingBytes = l;
          }
          else {
            this._handleMessage(messageBytes, message);
          }
        }
        catch(e) {
          // parse error
          this._warning("Malformed message: "+e);
          this._handleMessage(messageBytes, null);
          
        }
        finally {
          // remove consumed bytes from buffer
          this._readBuffer = this._readBuffer.substring(endp);
          this._dump(this._readBuffer.length+" bytes left in the read buffer");
          start = 0;
        }
      }
      else {
        // we have a pending message, waiting for body octets
        if (this._pendingBytes<=this._readBuffer.length) {
          this._pendingMessage.body = this._readBuffer.substring(0, this._pendingBytes);
          this._pendingMessageBytes += this._readBuffer.substring(0, this._pendingBytes);
          this._readBuffer = this._readBuffer.substring(this._pendingBytes);
          start = 0;
          this._handleMessage(this._pendingMessageBytes, this._pendingMessage);
          this._pendingMessage = null;
          this._pendingMessageBytes = null;
          this._pendingBytes = 0;
        }
        else {
          this._dump("incomplete body. waiting for "+
                     (this._pendingBytes-this._readBuffer.length) +
                     " bytes");
          // still not enough bytes -> bail out of loop
          break;
        }
      }
    }
  });

SipTCPConnection.fun(
  function _handleMessage(bytes, message) {
    this._transceiver._dispatchPacket(bytes,
                                      Components.interfaces.zapISipTransceiverSink.APPLICATION_SIP,
                                      "TCP",
                                      "xxx-local-address",
                                      this._socket.port,
                                      this.host,
                                      this.port,
                                      this);
  });



////////////////////////////////////////////////////////////////////////
// SipTransceiver : sip transport layer backend

var SipTransceiver = makeClass("SipTransceiver", SupportsImpl);
SipTransceiver.addInterfaces(Components.interfaces.zapISipTransceiver,
                             Components.interfaces.nsIUDPReceiver,
                             Components.interfaces.nsIServerSocketListener);

SipTransceiver.appendCtor(
  function() {
    // hash of listening socket indexed by transportProtocol+port
    this._listeningSockets = {};

    // hash of persistent connections indexed by transportProtocol+peer
    // address+peer port
    this._connections = {};

    // stack of transceiver sinks. 
    this._sinks = [];
  });

//----------------------------------------------------------------------
// zapISipTransceiver implementation:

// void openListeningSocket(in ACString transportProtocol,
//                          in unsigned long port);
SipTransceiver.fun(
  function openListeningSocket(transportProtocol, port) {
    transportProtocol = transportProtocol.toUpperCase();
    var key = transportProtocol+":"+port;
    if (hashhas(this._listeningSockets, key))
      this._error("SipTransceiver: "+transportProtocol+
                  " listening socket at "+port+" already open");

    var socket;
    if (transportProtocol == "UDP") {
      socket = CLASS_UDP_SOCKET.createInstance();
      socket.QueryInterface(ITF_UDP_SOCKET);
      socket.init(port);
      socket.setReceiver(this);
    }
    else if (transportProtocol == "TCP") {
      socket = CLASS_SERVER_SOCKET.createInstance();
      socket.QueryInterface(ITF_SERVER_SOCKET);
      socket.init(port, false, -1);
      socket.asyncListen(this);
    }
    else {
      this._error("SipTransceiver: Unsupported transport protocol '"+transportProtocol+"'");
    }
    
    hashset(this._listeningSockets, key, socket);
  });

// void closeListeningSocket(in ACString transportProtocol,
//                           in unsigned long port);
SipTransceiver.fun(
  function closeListeningSocket(transportProtocol, port) {
    transportProtocol = transportProtocol.toUpperCase();
    var key = transportProtocol+":"+port;
    var socket;
    if (!(socket = hashget(this._listeningSockets, key)))
      this._error("SipTransceiver: Trying to close unknown "+
                  transportProtocol+" socket at "+port);
    if (transportProtocol == "UDP" || transportProtocol == "TCP")
      socket.close();
    else
      this._error("SipTransceiver: Unsupported transport protocol '"+transportProtocol+"'");
    
    hashdel(this._listeningSockets, key);
  });

// void getListeningSockets(out unsigned long count,
//                          [retval, array, size_is(count)] out zapISipListeningSocket sockets);
SipTransceiver.fun(
  function getListeningSockets(count) {
    var rv =
      hashkeys(this._listeningSockets).map(
        function(s) {
          var match = /^([^:]*):(\d*)$/(s);
          return { protocol : match[1],
                   port     : parseInt(match[2]) };
        });

    if (count)
      count.value = rv.length;
    return rv;
  });

// zapISipConnection sendPacket(in ACString packet,
//                              in ACString transportProtocol,
//                              in ACString destAddress, in ACString destPort,
//                              in zapISipConnection connection);
SipTransceiver.fun(
  function sendPacket(packet, transportProtocol, destAddress, destPort, connection) {
    var localPort;
    var localAddress;
    if (transportProtocol == "UDP") {
      this._assert(connection==null,
                   "Connection provided for connection-less protocol!");
      var socket = this._getUDPSendSocket();
      if (!socket) this._error("SipTransceiver: No UDP socket available");
      socket.send(packet, destAddress, destPort);
      localPort = socket.port;
      localAddress = socket.address;
    }
    else if (transportProtocol == "TCP") {
      // if a connection has been provided, use it:
      if (connection && connection.isAlive()) {
        this._dump("using provided connection");
      }
      else {
        // check if we have an open connection to the given port
        // already:
        if (!(connection = hashget(this._connections,
                                   "TCP:"+destAddress+":"+destPort))) {
          // no. -> open a new one:
          this._dump("opening new tcp socket");
          var socket = getSTS().createTransport([], 0, destAddress, destPort, null);
          connection = this._createTCPConnection(socket);
        }
      }
      connection.send(packet);
      localPort = connection._socket.port;
      localAddress = "xxx-local-address";
    }
    else
      this._error("SipTransceiver: Unsupported protocol '"+transportProtocol+"'");

    if (this.trafficMonitor)
      this.trafficMonitor.notifyPacket(packet,
                                       transportProtocol,
                                       localAddress,
                                       localPort,
                                       destAddress, destPort,
                                       true);
    return connection;
  });

//  void prependTransceiverSink(in zapISipTransceiverSink receiver);
SipTransceiver.fun(
  function prependTransceiverSink(receiver) {
    this._sinks.splice(0, 0, receiver);
  });

//  void removeTransceiverSink(in zapISipTransceiverSink receiver);
SipTransceiver.fun(
  function removeTransceiverSink(receiver) {
    for (var i=0, l=this._sinks; i<l; ++i)
      if (this._sinks[i] == receiver) {
        this._sinks.splice(i, 1);
        return;
      }
  });

// attribute zapISipTrafficMonitor trafficMonitor;
SipTransceiver.obj("trafficMonitor", null);

// void shutdown();
SipTransceiver.fun(
  function shutdown() {
    // close all listening sockets:
    hashmap(this._listeningSockets, function(k,s) {s.close(); return false;});
    // close all open connections:
    hashmap(this._connections, function(k,c) {c.close(); return false;});

    this.trafficMonitor = null;
  });

//----------------------------------------------------------------------
// nsIUDPReceiver implementation:

SipTransceiver.fun(
  function handleDatagram(socket, data) {
    var application = Components.interfaces.zapISipTransceiverSink.APPLICATION_SIP;
    if (getNetUtils().snoopStunPacket(data.data) != -2) {
      // data appears to be a STUN packet
      application = Components.interfaces.zapISipTransceiverSink.APPLICATION_STUN;
    }
    
    this._dispatchPacket(data.data,
                         application,
                         "UDP",
                         socket.address,
                         socket.port,
                         data.address,
                         data.port,
                         null);
  });

//----------------------------------------------------------------------
// nsIServerSocketListener implementation:

SipTransceiver.fun(
  function onSocketAccepted(server, socket) {
    this._dump("Accepting new connection on TCP port "+server.port);
    this._createTCPConnection(socket);
  });

SipTransceiver.fun(
  function onStopListening(server, status) {
    this._dump("Stopping listening on TCP port "+server.port);
  });

//----------------------------------------------------------------------
// implementation helpers:

// create a new tcp connection from the given socket and enter it into
// our connection cache:
SipTransceiver.fun(
  function _createTCPConnection(socket) {
    var connection = SipTCPConnection.instantiate();
    connection.init(socket, this);
    hashset(this._connections, "TCP:"+socket.host+":"+socket.port,
            connection);
    return connection;
  });

// callback for zapISipConnections to inform us of closure
SipTransceiver.fun(
  function _connectionClosed(transportProtocol, host, port) {
    // remove from connections hash
    hashdel(this._connections, transportProtocol+":"+host+":"+port);
  });

SipTransceiver.fun(
  function _getUDPSendSocket() {
    return hashmap(this._listeningSockets,
                   function(k, s) {
                     try {
                       s.QueryInterface(ITF_UDP_SOCKET);
                       return s;
                     }
                     catch(e) {
                       // QI throws an exception if s is not a udp socket
                       // -> ignore, move on to next
                     }
                     return false;
                   });
  });

// all incoming packets will be routed through this function:
SipTransceiver.fun(
  function _dispatchPacket(data,
                           application, transportProtocol,
                           localAddress, localPort,
                           remoteAddress, remotePort,
                           connection) {
    if (this.trafficMonitor)
      this.trafficMonitor.notifyPacket(data,
                                       transportProtocol,
                                       localAddress,
                                       localPort,
                                       remoteAddress, remotePort,
                                       false);
    var handled = this._sinks.some(function(s) {
                                     return s.handlePacket(data,
                                                           application,
                                                           transportProtocol,
                                                           localAddress,
                                                           localPort,
                                                           remoteAddress,
                                                           remotePort,
                                                           connection); });
    if (!handled)
      this._dump("unhandled "+ transportProtocol+" packet (app="+
                 application+", length="+data.length+") from "+remoteAddress+
                 ":"+remotePort+" received on "+localAddress+":"+
                 localPort);
  });

////////////////////////////////////////////////////////////////////////
// StunMonitorTransaction
// helper class for monitoring sip flows

var StunMonitorTransaction = makeClass("StunMonitorTransaction",
                                       Scheduler);

StunMonitorTransaction.fun(
  function execute(monitorScheduler) {
    this._assert(!this.Terminated, "transaction terminated");
    this.monitorScheduler = monitorScheduler;
    this.flow = monitorScheduler.flow;
    this.request = getNetUtils().createStunMessage();
    this.request.messageType = this.request.BINDING_REQUEST_MESSAGE;
    this.request.initTransactionID();
    this.tries = 0;
    this.interval = 50; // this will be multiplied by 2 to give our
                        // first 'real' interval of 100
    this.sendRequest();
  });

StunMonitorTransaction.fun(
  function sendRequest() {
    if (this.tries == 9) {
      // we've transmitted 9 requests and received no response.
      // -> failure
      this.Terminated = true;
      this.monitorScheduler.notifyRequestFailure(Components.interfaces.zapISipFlowMonitor.FLOW_FAILED);
    }
    else {
      ++ this.tries;
      this.flow.transport.transceiver.sendPacket(this.request.serialize(),
                                                 this.flow.transportProtocol,
                                                 this.flow.remoteAddress,
                                                 this.flow.remotePort,
                                                 this.flow.connection);
      this.interval = Math.min(2*this.interval, 1600);
      this.schedule(this.sendRequest, this.interval);
    }
  });

StunMonitorTransaction.fun(
  function cancel() {
    // setting Terminated to true will automatically clear the
    // schedule timer
    this.Terminated = true;
  });

StunMonitorTransaction.fun(
  function handleStunPacket(packet) {
    this._assert(this.tries > 0, "hmm, handleStunPacket called, but we haven't even sent the request???");
    try {
      var message = getNetUtils().deserializeStunPacket(packet, {}, {});
    }
    catch(e) {
      // error parsing this as a STUN packet. Maybe it wasn't one?
      // -> don't handle
      this._dump("not handling malformed STUN packet");
      return false;
    }
    switch (message.messageType) {
      case Components.interfaces.zapIStunMessage.BINDING_RESPONSE_MESSAGE:
        if (message.transactionID != this.request.transactionID) {
          // not a response to our current query; possibly a
          // retransmission. let it fall through in case there are
          // other stun handlers down the chain:
          return false;
        }
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
          this.Terminated = true;
          this.monitorScheduler.notifyRequestFailure(Components.interfaces.zapISipFlowMonitor.MONITOR_PROTOCOL_ERROR);
          return true;
        }
        this.Terminated = true;
        this.monitorScheduler.notifyRequestSuccess(address, port);
        return true;
        break;
      case Components.interfaces.zapIStunMessage.BINDING_ERROR_RESPONSE_MESSAGE:
        if (message.transactionID != this.request.transactionID) {
          // not a response to our current query; possibly a
          // retransmission. let it fall through in case there are
          // other stun handlers down the chain:
          return false;
        }
        // else...
        this.Terminated = true;
        this.monitorScheduler.notifyRequestFailure(Components.interfaces.zapISipFlowMonitor.MONITOR_PROTOCOL_ERROR);
        return true;
        break;
    }

    // probably not meant for us:
    return false;
  });

////////////////////////////////////////////////////////////////////////
// StunMonitorScheduler
// helper class for monitoring sip flows

var StunMonitorScheduler = makeClass("StunMonitorScheduler", Scheduler);

StunMonitorScheduler.fun(
  function start(flow) {
    this.flow = flow;
    this.sendRequest();
  });

StunMonitorScheduler.fun(
  function getRequestInterval() {
    // see draft-ietf-sip-outbound-01 4.2
    if (this.flow.transportProtocol == "UDP")
      return 24000+Math.floor(Math.random()*5000);
    else
      return 95000+Math.floor(Math.random()*7000);
  });

StunMonitorScheduler.fun(
  function terminate() {
    this.Terminated = true;
  });

StunMonitorScheduler.fun(
  function handleStunPacket(packet) {
    if (this.currentRequest)
      return this.currentRequest.handleStunPacket(packet);
    return false;
  });

StunMonitorScheduler.fun(
  function notifyRequestSuccess(address, port) {
    delete this.currentRequest;
    if (!this.address) {
      this.address = address;
      this.port = port;
    }
    else {
      var changeFlags = 0;
      // check if the flow has changed in some way:
      if (address != this.address) changeFlags |= Components.interfaces.zapISipFlowMonitor.ADDRESS_CHANGED;
      if (port != this.port) changeFlags |= Components.interfaces.zapISipFlowMonitor.PORT_CHANGED;
      if (changeFlags != 0) {
        this.flow.notifyMonitors(changeFlags);
        this.address = address;
        this.port = port;
      }
    }   
    // reschedule:
    this.schedule(this.sendRequest, this.getRequestInterval());
  });

StunMonitorScheduler.fun(
  function notifyRequestFailure(flags) {
    delete this.currentRequest;
    this.flow.notifyMonitors(flags);
  });

StunMonitorScheduler.fun(
  function sendRequest() {
    this.currentRequest = StunMonitorTransaction.instantiate();
    this.currentRequest.execute(this);
  });

////////////////////////////////////////////////////////////////////////
// OptionsMonitorScheduler

var OptionsMonitorScheduler = makeClass("OptionsMonitorScheduler",
                                        Scheduler, SupportsImpl);
OptionsMonitorScheduler.addInterfaces(Components.interfaces.zapISipNonInviteRCListener);

OptionsMonitorScheduler.fun(
  function start(flow) {
    this.flow = flow;

    // XXX temporary hack to get some local address when the flow is from 0.0.0.0:
    var localAddress = flow.localAddress;
    if (localAddress == "0.0.0.0") {
      localAddress = flow.transport.UAStack.hostAddress;
    }
    
    var to = gSyntaxFactory.deserializeAddress("sip:" +
                                               flow.remoteAddress + ":" +
                                               flow.remotePort);
    
    var from = gSyntaxFactory.deserializeAddress("sip:NATkeepalive@" +
                                                 localAddress + ":" +
                                                 flow.localPort);

    this.rc = flow.transport.UAStack.createNonInviteRequestClient(to, from,
                                                                  "OPTIONS",
                                                                  [], 0, 0);
    this.schedule(this.sendRequest, this.getRequestInterval());
  });

OptionsMonitorScheduler.fun(
  function getRequestInterval() {
    // we use the same interval as the stun monitor
    // see draft-ietf-sip-outbound-01 4.2
    if (this.flow.transportProtocol == "UDP")
      return 24000+Math.floor(Math.random()*5000);
    else
      return 95000+Math.floor(Math.random()*7000);
  });

OptionsMonitorScheduler.fun(
  function terminate() {
    this.Terminated = true;
  });

OptionsMonitorScheduler.fun(
  function handleStunPacket(packet) {
    return false;
  });

OptionsMonitorScheduler.fun(
  function sendRequest() {
    this.rc.sendRequest(this);
  });

// zapISipNonInviteRCListener
OptionsMonitorScheduler.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    if (response.statusCode[0] == "1") return; // just a provisional response

    // we'll interpret anything apart from a timeout (408) as 'success':
    if (response.statusCode == "408")
      this.flow.notifyMonitors(Components.interfaces.zapISipFlowMonitor.FLOW_FAILED);

    // XXX be smart in checking for changes in the flow

    // reschedule:
    this.schedule(this.sendRequest, this.getRequestInterval());
  });

////////////////////////////////////////////////////////////////////////
// SipFlow

var SipFlow = makeClass("SipFlow", SupportsImpl);
SipFlow.addInterfaces(Components.interfaces.zapISipFlow);

SipFlow.fun(
  function init(transport,
                localAddress, localPort,
                remoteAddress, remotePort,
                transportProtocol, connection) {
    this.transport = transport;
    this.localAddress = localAddress;
    this.localPort = localPort;
    this.remoteAddress = remoteAddress;
    this.remotePort = remotePort;
    this.transportProtocol = transportProtocol;
    this.connection = connection;
  });

function makeFlowID(localAddress, localPort,
                    remoteAddress, remotePort,
                    transportProtocol) {
  return localAddress+":"+localPort+":"+remoteAddress+":"+transportProtocol;
}

SipFlow.getter(
  "flowID",
  function get_flowID() {
    if (!this._flowID) {
      this._flowID = makeFlowID(this.localAddress, this.localPort,
                                this.remoteAddress, this.remotePort,
                                this.transportProtocol);
    }
    return this._flowID;
  });

SipFlow.fun(
  function startMonitoring(type) {
    this._assert(!this.monitorScheduler, "already monitored");
    if (type == Components.interfaces.zapISipFlow.IETF_SIP_OUTBOUND_01_MONITOR)
      this.monitorScheduler = StunMonitorScheduler.instantiate();
    else if (type == Components.interfaces.zapISipFlow.OPTIONS_MONITOR) {
      this._assert(this.transport.UAStack, "can't use OPTIONS monitoring without UA stack");
      this.monitorScheduler = OptionsMonitorScheduler.instantiate();
    }
    this.monitorScheduler.start(this);
  });

SipFlow.fun(
  function stopMonitoring() {
    this._assert(this.monitorScheduler, "flow not monitored??");
    this._dump("stopping monitoring");
    this.monitorScheduler.terminate();
    delete this.monitorScheduler;
  });

SipFlow.fun(
  function handleStunPacket(packet) {
    if (this.monitorScheduler)
      return this.monitorScheduler.handleStunPacket(packet);
    return false;
  });

SipFlow.fun(
  function notifyMonitors(flags) {
    if (!this._monitors) {
      this._dump("no monitors!");
      return;
    }
    // do the notification robustly, so that monitors can remove
    // themselves reentrantly:
    for (var i=this._monitors.length-1; i>=0; --i) {
      this._monitors[i].flowChanged(this, flags);
      if (i>this._monitors.length) i = this._monitors.length;
    }
  });


//----------------------------------------------------------------------
// zapISipFlow

//  readonly attribute ACString localAddress;
SipFlow.obj("localAddress", null);

//  readonly attribute unsigned short localPort;
SipFlow.obj("localPort", null);

//  readonly attribute ACString remoteAddress;
SipFlow.obj("remoteAddress", null);

//  readonly attribute unsigned short remotePort;
SipFlow.obj("remotePort", null);

//  readonly attribute ACString transportProtocol;
SipFlow.obj("transportProtocol", null);

//  readonly attribute zapISipConnection connection;
SipFlow.obj("connection", null);

//  void addFlowMonitor(in zapISipFlowMonitor monitor, in unsigned short type);
SipFlow.fun(
  function addFlowMonitor(monitor, type) {
    if (!this._monitors) this._monitors = [];
    var monitors = this._monitors;

    // make sure we don't get redundant registrations:
    for (var i=0,l=monitors.length; i<l; ++i) {
      if (monitors[i] == monitor)
        this._error("Monitor already set");
    }
    
    this._monitors.push(monitor);
    this._dump("Added monitor "+monitor+" to flow "+this.flowID);
    if (monitors.length == 1) {
      this.startMonitoring(type);
    }
  });

//  void removeFlowMonitor(in zapISipFlowMonitor monitor);
SipFlow.fun(
  function removeFlowMonitor(monitor) {
    var monitors = this._monitors;
    for (var i=monitors.length-1; i>=0; --i) {
      if (monitors[i] == monitor) {
        monitors.splice(i, 1);
        this._dump("Removing monitor "+monitor+" from flow "+this.flowID);
        break;
      }
    }
    if (monitors.length == 0) {
      this.stopMonitoring();
      this._dump("Stopping monitoring of flow "+this.flowID);
    }
  });


////////////////////////////////////////////////////////////////////////
// SipTransport : sip transport layer frontend

var SipTransport = makeClass("SipTransport", SupportsImpl);
SipTransport.addInterfaces(Components.interfaces.zapISipTransceiverSink);

SipTransport.appendCtor(
  function ctor() {
    this._sinks = [];

    // hash of currently monitored flows:
    this._flows = WeakHash.instantiate();
  });

//----------------------------------------------------------------------
// zapISipTransport

SipTransport.fun(
  function init(config) {
    this._assert(!this.transceiver, "already initialized");
    this.transceiver = SipTransceiver.instantiate();
    this.transceiver.prependTransceiverSink(this);

    var port = 5060;
    if (config) {
      try {
        port = config.getProperty("port_base");
      }catch(e){}
    }

    // find the first port >= listeningPort that is available for both
    // udp and tcp:
    while (port < 65536) {
      try {
        this.transceiver.openListeningSocket("UDP", port);
      }
      catch(e) {
        ++port;
        continue;
      }

      try {
        this.transceiver.openListeningSocket("TCP", port);
      }
      catch(e) {
        this.transceiver.closeListeningSocket("UDP", port);
        ++port;
        continue;
      }
      // success:
      this.listeningPort = port;
      return;
    }
    // failure:
    this._error("SipTransport: failure obtaining listening socket.");
  });


SipTransport.fun(
  function shutdown() {
    this.transceiver.shutdown();
    this.UAStack = null;
  });

SipTransport.fun(
  function appendTransportSink(sink) {
    this._sinks.push(sink);
  });

SipTransport.obj("UAStack", null);

//  readonly attribute zapISipTransceiver transceiver;
SipTransport.obj("transceiver", null);

SipTransport.obj("listeningPort", 0);

SipTransport.fun(
  "Send a request (RFC3261 Section 18.1.1)",
  function sendRequest(request, endpoint, connection) {
    this._assert(endpoint, "no endpoint for sending request. backtrace:"+this._backtrace());

    // Insert IP address or host name and port into 'sent-by' of top
    // Via (RFC3261 18.1.1)
    var topVia = request.getTopViaHeader();
    topVia.host = this._getMyFQDN();
    topVia.port = this.listeningPort;
    
    // Add transport portion of 'sent-protocol':
    var transport = endpoint.transport.toUpperCase();
    topVia.transport = transport;

    if (transport == "TCP") {
      // make sure we have a content-length header
      request.ensureContentLengthHeader();
    }
    
    return this.transceiver.sendPacket(request.serialize(),
                                       endpoint.transport,
                                       endpoint.address,
                                       endpoint.port, connection);
  });

SipTransport.fun(
  "Sends a response (RFC3261 Section 18.2.2)",
  function sendResponse(response, connection) {
    // XXX implement procedures of section 18.2.2 properly
    
    // Examine top via to determine transport, address and port:
    var topVia = response.getTopViaHeader();
    
    var transport = topVia.transport.toUpperCase();

    if (transport == "TCP") {
      // make sure we have a content-length header
      response.ensureContentLengthHeader();
    }
    
    // Check for 'received' parameter first:
    var address = topVia.getParameter("received");
    if (!address)
      address = topVia.host;

    // RFC3581 4: check for rport parameter
    var port = topVia.getParameter("rport");
    if (!port)
      port = topVia.port;
    if (!port)
      port = "5060";
    
    return this.transceiver.sendPacket(response.serialize(),
                                       transport,
                                       address,
                                       port,
                                       connection);
  });

SipTransport.fun(
  function getFlow(localAddress, localPort, remoteAddress, remotePort,
                   transportProtocol) {
    // try to find a corresponding flow or create a new one:
    var id = makeFlowID(localAddress, localPort,
                        remoteAddress, remotePort, transportProtocol);

    var flow = this._flows.get(id);
    if (!flow) {
      flow = SipFlow.instantiate();
      flow.init(this, localAddress, localPort, remoteAddress, remotePort,
                transportProtocol, null);
      this._flows.set(id, flow);
    }
    return flow;
  });

//----------------------------------------------------------------------
//zapISipTransceiverSink

SipTransport.fun(
  function handlePacket(data, application,
                        transportProtocol, localAddress, localPort,
                        remoteAddress, remotePort, connection) {
    // try to find a corresponding flow or create a new one:
    var id = makeFlowID(localAddress, localPort,
                        remoteAddress, remotePort, transportProtocol);

    var flow = this._flows.get(id);
    if (!flow) {
      flow = SipFlow.instantiate();
      flow.init(this, localAddress, localPort, remoteAddress, remotePort,
                transportProtocol, connection);
      this._flows.set(id, flow);
    }
    
    if (application == Components.interfaces.zapISipTransceiverSink.APPLICATION_STUN) {
      // give the flow a chance to handle this STUN packet:
      return flow.handleStunPacket(data);       
    }
      
    // from here on we only handle SIP traffic:
    if (application != Components.interfaces.zapISipTransceiverSink.APPLICATION_SIP)
      return false;
    
    try {
      var message = gSyntaxFactory.deserializeMessage(data);
      //XXX make sure content is not longer than indicated by Content-Length  
    }
    catch(e) {
      this._warning("Rejecting malformed datagram from "+remoteAddress+":"+
                    remotePort+", received on "+localAddress+":"+localPort+
                    ". Error was: "+e);
      return false;
    }
    
    if (message.isRequest) {
      // Process according to RFC3261 18.2.1 and RFC3581 4:
      var topVia = message.getTopViaHeader();
      if (topVia.hasParameter("rport")) {
        // RFC3581 4: unconditionally insert rport & received parameters
        topVia.setParameter("received", remoteAddress);
        topVia.setParameter("rport", remotePort);
      }
      else {
        // RFC3261 18.2.1:      
        // When the server transport receives a request over any
        // transport, it MUST examine the value of the "sent-by"
        // parameter in the top Via header field value.  If the host
        // portion of the "sent-by" parameter contains a domain name, or
        // if it contains an IP address that differs from the packet
        // source address, the server MUST add a "received" parameter to
        // that Via header field value.  This parameter MUST contain the
        // source address from which the packet was received.  This is
        // to assist the server transport layer in sending the response,
        // since it must be sent to the source IP address from which the
        // request came.
        if (topVia.host != remoteAddress) {
          topVia.setParameter("received", remoteAddress);
        }
      }
    }
    else {
      // RFC3261 18.1.2:
      // When a response is received, the client transport examines
      // the top Via header field value.  If the value of the
      // "sent-by" parameter in that header field value does not
      // correspond to a value that the client transport is configured
      // to insert into requests, the response MUST be silently
      // discarded.
       var topVia = message.getTopViaHeader();
       var ViaPort = topVia.port ? topVia.port : 5060;
       if (topVia.host != this._getMyFQDN() ||
           ViaPort != this.listeningPort ) {
         this._dump("Response from "+remoteAddress+":"+remotePort+
                    " to request from "+topVia.host+":"+topVia.port+
                    " discarded!");
         return true;
      }
    }
    
    // hand messages to transport sinks until one returns true:
    this._sinks.some(function(s) {
                       return s.handleSipMessage(message, flow); });
    return true;
  });

//----------------------------------------------------------------------
// implementation helpers:

SipTransport.fun(
  function _getMyFQDN() {
    if (!this._fqdn) {
      // try to get a proper FQDN:
      try { 
        this._fqdn = getDNSService().resolve(getDNSService().myHostName,0).getNextAddrAsString();
      }
      catch(e) {
        this._warning("Error resolving FQDN. Will use just hostname instead."+
                      "Error was: "+e);
        this._fqdn = getDNSService().myHostName;
      }
    }
    return this._fqdn;
  });

debug("*** SipTransport 3\n");

////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [
   { className  : "ZAP SIP Transceiver",
     cid        : Components.ID("{a872ed47-0696-46d5-b6ed-12e2ee691737}"),
     contractID : "@mozilla.org/zap/siptransceiver;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipTransceiver.instantiate(); })
   },
   { className  : "ZAP SIP Transport",
     cid        : Components.ID("{564b569b-4350-420c-b247-2581ace9e780}"),
     contractID : "@mozilla.org/zap/siptransport;1",
     factory    : zapXPCOMUtils.generateFactory(function() { return SipTransport.instantiate(); })
   }
  ]);

debug("*** SipTransport 4\n");
