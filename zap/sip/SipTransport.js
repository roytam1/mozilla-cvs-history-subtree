/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 moz-jssh-buffer-globalobj: "Components.classes['@mozilla.org/moz/jsloader;1'].getService(Components.interfaces.xpcIJSComponentLoader).importModule('rel:SipTransport.js')" -*- */
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

Components.utils.importModule("resource:/jscodelib/JSComponentUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ClassUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ArrayUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/StringUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/ObjectUtils.js");
Components.utils.importModule("resource:/jscodelib/zap/SipUtils.js");

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
// Helpers:

// Helper to log transport events:
function log(mes, level) {
  if (!level)
    level = Components.interfaces.zapILoggingService.INFO;
  gLoggingService.log("SIP TRANSPORT", level, mes);
}


////////////////////////////////////////////////////////////////////////
// SipTCPConnection

var SipTCPConnection = makeClass("SipTCPConnection", SupportsImpl);
SipTCPConnection.addInterfaces(Components.interfaces.nsIInputStreamCallback,
                               Components.interfaces.zapISipConnection);

SipTCPConnection.fun(
  function init(socket, transceiver) {
    this._readBuffer = "";
    this._pendingMessage = null;
    this._pendingBytes = 0;
    
    this._transceiver = transceiver;
    this._socket = socket;
    this._outputStream = socket.openOutputStream(0, 0, 0);
    this._inputStream = socket.openInputStream(0, 0, 0);
    this._inputStream.QueryInterface(ITF_ASYNC_INPUT_STREAM);
    this._scriptableInputStream = CLASS_SCRIPT_INPUT_STREAM.createInstance();
    this._scriptableInputStream.QueryInterface(ITF_SCRIPT_INPUT_STREAM);
    this._scriptableInputStream.init(this._inputStream);
    this._proxy = getProxyOnSIPThread(this,
                                      Components.interfaces.nsIInputStreamCallback);
    this._inputStream.asyncWait(this._proxy, 0, 0, null);
  });

//----------------------------------------------------------------------
// nsIInputStreamCallback implementation

SipTCPConnection.fun(
  function onInputStreamReady(stream) {
    try {
      var oldLength = this._readBuffer.length;
      this._readBuffer += this._scriptableInputStream.read(this._scriptableInputStream.available());
      this._dump("Data received from "+this._socket.host+":"+this._socket.port);
      this._processBuffer(oldLength);
      this._inputStream.asyncWait(this._proxy, 0, 0, null);
    }
    catch (e) {
      this._dump("stream closed or other error: "+e);
      this._transceiver._connectionClosed(this.protocol, this.host, this.port);      
      delete this._proxy;
    }
  });

//----------------------------------------------------------------------
// zapISipConnection implementation

SipTCPConnection.obj("protocol", "tcp");

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
          var message = gSyntaxFactory.deserializeMessage(this._readBuffer.substring(0, endp));
          var l = 0;
          var cl = message.getSingleHeader("Content-Length");
          if (cl)
            l = cl.contentLength;
          if (l>0) {
            // wait for body octets:
            this._pendingMessage = message;
            this._pendingBytes = l;
          }
          else {
            this._handleMessage(message);
          }
        }
        catch(e) {
          // parse error
          this._warning("Malformed message: "+e);
          
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
          this._readBuffer = this._readBuffer.substring(this._pendingBytes);
          start = 0;
          this._handleMessage(this._pendingMessage);
          this._pendingMessage = null;
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
  function _handleMessage(message) {
    this._transceiver._handleReceivedMessage("tcp", this.host, this.port,
                                           message, this);
  });



////////////////////////////////////////////////////////////////////////
// SipTransceiver : sip transport layer backend

var SipTransceiver = makeClass("SipTransceiver", SupportsImpl);
SipTransceiver.addInterfaces(Components.interfaces.zapISipTransceiver,
                           Components.interfaces.nsIUDPReceiver,
                           Components.interfaces.nsIServerSocketListener);

SipTransceiver.appendCtor(
  function() {
    // hash of listening socket indexed by protocol+port
    this._listeningSockets = {};

    // hash of persistent connections indexed by protocol+peer
    // address+peer port
    this._connections = {};
  });

//----------------------------------------------------------------------
// zapISipTransceiver implementation:

// void openListeningSocket(in ACString protocol, in unsigned long port);
SipTransceiver.fun(
  function openListeningSocket(protocol, port) {
    protocol = protocol.toLowerCase();
    var key = protocol+":"+port;
    if (hashhas(this._listeningSockets, key))
      this._error("Already open");

    var socket;
    if (protocol == "udp") {
      socket = CLASS_UDP_SOCKET.createInstance();
      socket.QueryInterface(ITF_UDP_SOCKET);
      socket.init(port);
      socket.setReceiver(this);
    }
    else if (protocol == "tcp") {
      socket = CLASS_SERVER_SOCKET.createInstance();
      socket.QueryInterface(ITF_SERVER_SOCKET);
      socket.init(port, false, -1);
      socket.asyncListen(this);
    }
    else {
      this._error("Unsupported protocol");
    }
    
    hashset(this._listeningSockets, key, socket);
  });

// void closeListeningSocket(in ACString protocol, in unsigned long port);
SipTransceiver.fun(
  function closeListeningSocket(protocol, port) {
    protocol = protocol.toLowerCase();
    var key = protocol+":"+port;
    var socket;
    if (!(socket = hashget(this._listeningSockets, key)))
      this._error("Socket not open");
    if (protocol == "udp" || protocol == "tcp")
      socket.close();
    else
      this._error("Unsupported protocol");
    
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

// zapISipConnection sendMessage(in zapISipMessage message, in ACString protocol,
//                               in ACString destAddress, in ACString destPort,
//                               in zapISipConnection connection);
SipTransceiver.fun(
  function sendMessage(message, protocol, destAddress, destPort, connection) {
    if (protocol == "udp") {
      this._assert(connection==null,
                   "Connection provided for connection-less protocol!");
      var socket = this._getUDPSendSocket();
      if (!socket) this._error("No sending socket");
      log("Sending UDP packet to "+destAddress+":"+destPort+ " :\n"+message.serialize());
      socket.send(message.serialize(), destAddress, destPort);
    }
    else if (protocol == "tcp") {
      // if a connection has been provided, use it:
      if (connection && connection.isAlive()) {
        this._dump("using provided connection");
      }
      else {
        // check if we have an open connection to the given port
        // already:
        if (!(connection = hashget(this._connections,
                                   "tcp:"+destAddress+":"+destPort))) {
          // no. -> open a new one:
          var socket = getSTS().createTransport([], 0, destAddress, destPort, null);
          connection = this._createTCPConnection(socket);
        }
      }
      log("Sending packet via TCP :\n"+message.serialize());
      connection.send(message.serialize());
    }
    else
      this._error("Unsupported protocol");
    return connection;
  });

// void setTransceiverSink(in zapISipTransceiverSink receiver);
SipTransceiver.fun(
  function setTransceiverSink(receiver) {
    this._receiver = receiver;
  });

// void shutdown();
SipTransceiver.fun(
  function shutdown() {
    // close all listening sockets:
    hashmap(this._listeningSockets, function(k,s) {s.close(); return false;});
    // close all open connections:
    hashmap(this._connections, function(k,c) {c.close(); return false;});
  });

//----------------------------------------------------------------------
// nsIUDPReceiver implementation:

SipTransceiver.fun(
  function handleDatagram(socket, data) {
//    this._dump("Received a udp datagram on port "+socket.port);
//    this._dump("Sender: "+data.address+" Port: "+data.port);
    try {
      var message = gSyntaxFactory.deserializeMessage(data.data);
    }
    catch(e) {
      this._warning("Rejecting malformed datagram from"+data.address+":"+
                    data.port+", received on port "+socket.port+". Error was:"+e);
      return;
    }

    //XXX make sure content is not longer than indicated by Content-Length
    
    this._handleReceivedMessage("udp", data.address, data.port, message, null);
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

// all messages received on tcp or udp connections will be routed
// through this function:
SipTransceiver.fun(
  function _handleReceivedMessage(protocol, sourceAddress, sourcePort,
                                  message, connection) {
    this._dump("Message received via "+protocol+
               " from "+sourceAddress+":"+sourcePort);
    if (this._receiver)
      this._receiver.handleSipMessage(message,
                                      { transport: protocol,
                                        address:   sourceAddress,
                                        port:      sourcePort
                                      },
                                      connection);
  });

// create a new tcp connection from the given socket and enter it into
// our connection cache:
SipTransceiver.fun(
  function _createTCPConnection(socket) {
    var connection = SipTCPConnection.instantiate();
    connection.init(socket, this);
    hashset(this._connections, "tcp:"+socket.host+":"+socket.port,
            connection);
    return connection;
  });

// callback for zapISipConnections to inform us of closure
SipTransceiver.fun(
  function _connectionClosed(protocol, host, port) {
    // remove from connections hash
    hashdel(this._connections, protocol+":"+host+":"+port);
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

////////////////////////////////////////////////////////////////////////
// SipTransport : sip transport layer frontend

var SipTransport = makeClass("SipTransport", SupportsImpl);
SipTransport.addInterfaces(Components.interfaces.zapISipTransceiverSink);

SipTransport.appendCtor(
  function ctor() {
    this._sinks = [];
    this._transceiver = SipTransceiver.instantiate();
    this._transceiver.setTransceiverSink(this);
    this._transceiver.openListeningSocket("udp", 5060);
    this._transceiver.openListeningSocket("tcp", 5060);
  });

//----------------------------------------------------------------------
// zapISipTransport

SipTransport.fun(
  function shutdown() {
    this._transceiver.shutdown();
  });

SipTransport.fun(
  function appendTransportSink(sink) {
    this._sinks.push(sink);
  });

SipTransport.fun(
  "Send a request (RFC3261 Section 18.1.1)",
  function sendRequest(request, endpoint, connection) {
    // XXX possibly switch transports if request is too large.

    // Insert IP address or host name and port into 'sent-by' of top
    // Via (RFC3261 18.1.1)
    var topVia = request.getTopViaHeader();
    // XXX for now don't use the fqdn, so that we can do simple manual
    // NAT traversal. Use the request from address instead:
    
    //topVia.host = this._getMyFQDN();
    topVia.host = request.getFromHeader().address.uri.QueryInterface(Components.interfaces.zapISipSIPURI).host;
      
    // XXX add port if not default. distinguish between sip/sips, etc. etc.
    
    // Add transport portion of 'sent-protocol':
    topVia.transport = endpoint.transport.toUpperCase();
    
    return this._transceiver.sendMessage(request, endpoint.transport,
                                         endpoint.address,
                                         endpoint.port, connection);
  });

SipTransport.fun(
  "Sends a response (RFC3261 Section 18.2.2)",
  function sendResponse(response, connection) {
    // XXX implement procedures of section 18.2.2 properly
    
    // Examine top via to determine transport, address and port:
    var topVia = response.getTopViaHeader();
    
    var transport = topVia.transport.toLowerCase();

    // Check for 'received' parameter first:
    // XXX don't do this for now, so that we can sort-of negotiate NATs
    // var address = topVia.getParameter("received");
    var address = null;
    if (!address)
      address = topVia.host;

    var port = topVia.port;
    if (!port)
      port = "5060";
    
    return this._transceiver.sendMessage(response, transport,
                                         address,
                                         port,
                                         connection);
  });

//----------------------------------------------------------------------
//zapISipTransceiverSink

SipTransport.fun(
  function handleSipMessage(message, endpoint, connection) {
    if (message.isRequest) {
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
      var topVia = message.getTopViaHeader();
      if (topVia.host != endpoint.address) {
        topVia.setParameter("received", endpoint.address);
      }
    }
    else {
      // XXX don't do this for now, so that we can have simple manual
      // NAT traversal
      
      // RFC3261 18.1.2:
      // When a response is received, the client transport examines
      // the top Via header field value.  If the value of the
      // "sent-by" parameter in that header field value does not
      // correspond to a value that the client transport is configured
      // to insert into requests, the response MUST be silently
      // discarded.
      // var topVia = message.getTopViaHeader();
//       if (topVia.host != this._getMyFQDN() ||
//           topVia.port) {
//         this._dump("Response from "+endpoint.address+":"+endpoint.port+
//                    " to request from "+topVia.host+":"+topVia.port+
//                    " discarded!");
//         return;
//      }
    }

    log("Message received via "+endpoint.transport+
        " from "+endpoint.address+":"+endpoint.port+" :\n"+message.serialize());
    
    // hand messages to transport sinks until one returns true:
    this._sinks.some(function(s) {
                       return s.handleSipMessage(message, endpoint, connection); });
  });

//----------------------------------------------------------------------
// implementation helpers:

SipTransport.fun(
  function _getMyFQDN() {
    if (!this._fqdn) {
      // XXX gDNSService.myHostName might not be a FQDN, so we
      // better resolve it:
      this._fqdn = gDNSService.resolve(gDNSService.myHostName,0).getNextAddrAsString();
    }
    return this._fqdn;
  });


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = ComponentUtils.generateNSGetModule(
  [
   { className  : "ZAP SIP Transceiver",
     cid        : Components.ID("{a872ed47-0696-46d5-b6ed-12e2ee691737}"),
     contractID : "@mozilla.org/zap/siptransceiver;1",
     factory    : ComponentUtils.generateFactory(function() { return SipTransceiver.instantiate(); })
   },
   { className  : "ZAP SIP Transport",
     cid        : Components.ID("{564b569b-4350-420c-b247-2581ace9e780}"),
     contractID : "@mozilla.org/zap/siptransport;1",
     factory    : ComponentUtils.generateFactory(function() { return SipTransport.instantiate(); })
   }
  ]);
