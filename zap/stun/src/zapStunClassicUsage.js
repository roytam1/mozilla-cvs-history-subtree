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
 * Portions created by the Initial Developer are Copyright (C) 2005-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Eilon Yardeni <eyardeni@8x8.com> (original author)
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

Components.utils.import("resource://gre/components/zapXPCOMUtils.jsm");
Components.utils.import("resource://gre/components/ClassUtils.jsm");
Components.utils.import("resource://gre/components/ArrayUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");
Components.utils.import("resource://gre/components/ObjectUtils.jsm");
Components.utils.import("resource://gre/components/StringUtils.jsm");

//----------------------------------------------------------------------
// Udp sockets

var CLASS_UDP_SOCKET = Components.classes["@mozilla.org/network/udp-socket;1"];
var ITF_UDP_SOCKET = Components.interfaces.nsIUDPSocket;

//----------------------------------------------------------------------
// Stun Message

var CLASS_MESSAGE = Components.classes["@mozilla.org/zap/stun-message2;1"];
var ITF_MESSAGE = Components.interfaces.zapIStunMessage2;

//----------------------------------------------------------------------
// Stun Address Attribute

var ITF_ATTRIBUTE = Components.interfaces.zapIStunAttribute;
var ITF_ADDRESS_ATTRIBUTE = Components.interfaces.zapIStunAddressAttribute;

var PB = makePropertyBag2Proxy;

var gDNSResolver = 
  Components.utils.import("resource://gre/components/zapStunDNSResolver.js", 
                          null).theStunDNSResolver;

////////////////////////////////////////////////////////////////////////
// zapStunClassicUsage

var zapStunClassicUsage = makeClass("zapStunClassicUsage", SupportsImpl);
zapStunClassicUsage.addInterfaces(Components.interfaces.zapIStunResolver,
                                  Components.interfaces.zapIStunResponseHandler,
                                  Components.interfaces.zapIStunTransportSink,
                                  Components.interfaces.nsIUDPReceiver,
                                  Components.interfaces.zapIStunDNSResolverListener);
                                  
zapStunClassicUsage.appendCtor(
  function ctor() {
    this.client = Components.classes["@mozilla.org/zap/stun-client2;1"].createInstance();
    this.client = this.client.QueryInterface(Components.interfaces.zapIStunClient2);
    this.client.init(PB({$supportRFC3489:true, $validMethods:"1"}));
    
    this.client.transport.transportSink = this;
    
    this.udpSocket = CLASS_UDP_SOCKET.createInstance().QueryInterface(ITF_UDP_SOCKET);
    this.udpSocket.init(0);
    this.udpSocket.setReceiver(this);
  });                                  

zapStunClassicUsage.fun(
  function clean() {
    this.client.transport.transportSink = null;
    this.client.shutdown();
    this.udpSocket.close();    
  });

//----------------------------------------------------------------------
// zapIStunResolver implementation:
 
zapStunClassicUsage.fun(
  function resolveMappedAddress(listener, stunServer) {
    var message = CLASS_MESSAGE.createInstance().QueryInterface(ITF_MESSAGE);
    message.messageMethod = ITF_MESSAGE.BINDING;

    this.listener = listener;
    this.message = message;

    gDNSResolver.asyncResolve(stunServer, "UDP", this);
  });
  
//----------------------------------------------------------------------
// zapIStunDNSResolverListener implementation:
   
zapStunClassicUsage.fun(
  function onResolveComplete(request, enumerator) {
    if (!this.client.sendRequest(this.message, "", enumerator, this)) {
      this.listener.onAddressResolveComplete("", Components.results.NS_ERROR_FAILURE);
      this.clean();
    }
  });

//----------------------------------------------------------------------
// zapIStunResponseHandler implementation:

zapStunClassicUsage.fun(
  function handleSuccessResponse(response, request, address) {
    var addrAttr = response.getAttribute(ITF_ATTRIBUTE.XOR_MAPPED_ADDRESS);
    if (addrAttr) {     
      addrAttr = addrAttr.QueryInterface(ITF_ADDRESS_ATTRIBUTE);
      this.listener.onAddressResolveComplete(addrAttr.address, Components.results.NS_OK);
      this.clean();
      return;
    }
    
    addrAttr = response.getAttribute(ITF_ATTRIBUTE.MAPPED_ADDRESS);
    if (addrAttr) {
      addrAttr = addrAttr.QueryInterface(ITF_ADDRESS_ATTRIBUTE);
      this.listener.onAddressResolveComplete(addrAttr.address, Components.results.NS_OK);
    }
    else {
      this.listener.onAddressResolveComplete("", Components.results.NS_ERROR_FAILURE);
    }
    this.clean();
  });

zapStunClassicUsage.fun(
  function handleErrorResponse(response, request, address) {
    this.listener.onAddressResolveComplete("", Components.results.NS_ERROR_FAILURE);
    this.clean();
  });

zapStunClassicUsage.fun(
  function handleFailure(request) {
    this.listener.onAddressResolveComplete("", Components.results.NS_ERROR_FAILURE);
    this.clean();
  });
  
//----------------------------------------------------------------------
// zapIStunTransportSink implementation:

zapStunClassicUsage.fun(
  function sendPacket(data, protocol, address, port) {
    this.udpSocket.send(data, address, port);   
  });
  
//----------------------------------------------------------------------
// nsIUDPReceiver implementation:

zapStunClassicUsage.fun(
  function handleDatagram(socket, datagram) {
    this.client.transport.handlePacket(datagram.data, "UDP", datagram.address, datagram.port);
  });
        
////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = zapXPCOMUtils.generateNSGetModule(
  [
   { className  : "ZAP STUN Resolver",
     cid        : Components.ID("{8975b754-8be0-4e3b-8aa5-0d8b29dfb154}"),
     contractID : "@mozilla.org/zap/stun-resolver;2",
     factory    : zapXPCOMUtils.generateFactory(function() { return zapStunClassicUsage.instantiate(); })
   }
  ]);                                  
